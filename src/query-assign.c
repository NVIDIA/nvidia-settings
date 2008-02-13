/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See Version 2
 * of the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *           Free Software Foundation, Inc.
 *           59 Temple Place - Suite 330
 *           Boston, MA 02111-1307, USA
 *
 */

/*
 * query-assign.c - this source file contains functions for querying
 * and assigning attributes, as specified on the command line.  Some
 * of this functionality is also shared with the config file
 * reader/writer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "msg.h"
#include "query-assign.h"

/* local prototypes */

static int process_attribute_queries(int, char**, const char *);

static int process_attribute_assignments(int, char**, const char *);

static int query_all(const char *);

static void print_valid_values(char *, uint32, NVCTRLAttributeValidValuesRec);

static int validate_value(CtrlHandles *h, ParsedAttribute *a, uint32 d,
                          int screen, char *whence);

/*
 * nv_process_assignments_and_queries() - process any assignments or
 * queries specified on the commandline.  If an error occurs, return
 * NV_FALSE.  On success return NV_TRUE.
 */

int nv_process_assignments_and_queries(Options *op)
{
    int ret;

    if (op->num_queries) {
        ret = process_attribute_queries(op->num_queries,
                                        op->queries, op->ctrl_display);
        if (!ret) return NV_FALSE;
    }

    if (op->num_assignments) {
        ret = process_attribute_assignments(op->num_assignments,
                                            op->assignments,
                                            op->ctrl_display);
        if (!ret) return NV_FALSE;
    }
    
    return NV_TRUE;

} /* nv_process_assignments_and_queries() */



/*
 * nv_get_enabled_display_devices() - allocate an array of unsigned
 * ints of length n, and query the X server for each screen's enabled
 * display device mask, saving each X screen's display mask into the
 * appropriate index in the array.
 *
 * This is just an optimization to avoid redundant round trips to the
 * X server later in NvCtrlProcessParsedAttribute().
 *
 * On success a freshly malloc'ed array of length n is returned (it is
 * the caller's responsibility to free this memory).  On error an
 * error message is printed and NULL is returned.
 */

uint32 *nv_get_enabled_display_devices(int n, NvCtrlAttributeHandle **h)
{
    ReturnStatus status;
    int *d, screen;
    
    d = malloc(sizeof(int) * n);
    
    for (screen = 0; screen < n; screen++) {
        if (!h[screen]) {
            d[screen] = 0x0;
            continue;
        }

        status = NvCtrlGetAttribute(h[screen], NV_CTRL_ENABLED_DISPLAYS,
                                    &d[screen]);
        if (status != NvCtrlSuccess) {
            nv_error_msg("Error querying enabled displays on "
                         "screen %d (%s).", screen,
                         NvCtrlAttributesStrError(status));
            free(d);
            return NULL;
        }
    }

    return (uint32 *)d;

} /* nv_get_enabled_display_devices() */



/*
 * nv_alloc_ctrl_handles() - allocate a new CtrlHandles structure,
 * connect to the X server identified by display, and initialize an
 * NvCtrlAttributeHandle for each X screen.
 */

CtrlHandles *nv_alloc_ctrl_handles(const char *display)
{
    CtrlHandles *h;
    int i;

    h = malloc(sizeof(CtrlHandles));

    if (display) h->display = strdup(display);
    else h->display = NULL;
    
    h->dpy = XOpenDisplay(h->display);

    if (h->dpy) {
        
        h->num_screens = ScreenCount(h->dpy);
        
        h->h = malloc(h->num_screens * sizeof(NvCtrlAttributeHandle *));
        h->screen_names = malloc(h->num_screens * sizeof(char *));

        for (i = 0; i < h->num_screens; i++) {
            h->h[i] = NvCtrlAttributeInit
                (h->dpy, i, NV_CTRL_ATTRIBUTES_ALL_SUBSYSTEMS);
            h->screen_names[i] = NvCtrlGetDisplayName(h->h[i]);
        }
            
        h->d = nv_get_enabled_display_devices(h->num_screens, h->h);
        
    } else {

        nv_error_msg("Cannot open display '%s'.", XDisplayName(h->display));
        
        h->num_screens = 0;
        h->h = NULL;
        h->d = NULL;
        h->screen_names = NULL;
    }

    return h;

} /* nv_alloc_ctrl_handles() */


/*
 * nv_free_ctrl_handles() - free the CtrlHandles structure allocated
 * by nv_alloc_ctrl_handles()
 */

void nv_free_ctrl_handles(CtrlHandles *h)
{
    int i;

    if (!h) return;

    if (h->display) free(h->display);

    if (h->dpy) {

        /*
         * XXX It is unfortunate that the display connection needs
         * to be closed before the backends have had a chance to
         * tear down their state. If future backends need to send
         * protocol in this case or perform similar tasks, we'll
         * have to add e.g. NvCtrlAttributeTearDown(), which would
         * need to be called before XCloseDisplay().
         */
        XCloseDisplay(h->dpy);
        h->dpy = NULL;
        
        for (i = 0; i < h->num_screens; i++) {
            NvCtrlAttributeClose(h->h[i]);
            if (h->screen_names[i]) free(h->screen_names[i]);
        }
        
        if (h->d) free(h->d);
        if (h->h) free(h->h);
        if (h->screen_names) free(h->screen_names);
    }
    
    free(h);
    
} /* nv_free_ctrl_handles() */



/*
 * process_attribute_queries() - parse the list of queries, and
 * call NvCtrlProcessParsedAttribute() to process each query.
 *
 * If any errors are encountered, an error message is printed and
 * NV_FALSE is returned.  Otherwise, NV_TRUE is returned.
 *
 * XXX rather than call nv_alloc_ctrl_handles()/nv_free_ctrl_handles()
 * for every query, we should share the code in
 * process_config_file_attributes() to collapse the list of handles.
 */

static int process_attribute_queries(int num, char **queries,
                                     const char *display_name)
{
    int query, ret, val;
    ParsedAttribute a;
    CtrlHandles *h;
    
    val = NV_FALSE;
    
    /* loop over each requested query */

    for (query = 0; query < num; query++) {
        
        /* special case the "all" query */

        if (nv_strcasecmp(queries[query], "all")) {
            query_all(display_name);
            continue;
        }
        
        /* call the parser to parse queries[query] */

        ret = nv_parse_attribute_string(queries[query], NV_PARSER_QUERY, &a);
        if (ret != NV_PARSER_STATUS_SUCCESS) {
            nv_error_msg("Error parsing query '%s' (%s).",
                         queries[query], nv_parse_strerror(ret));
            goto done;
        }
        
        /* make sure we have a display */

        nv_assign_default_display(&a, display_name);

        /* allocate the CtrlHandles */

        h = nv_alloc_ctrl_handles(a.display);

        /* call the processing engine to process the parsed query */

        ret = nv_process_parsed_attribute(&a, h, NV_FALSE, NV_FALSE,
                                          "in query '%s'", queries[query]);
        /* free the CtrlHandles */

        nv_free_ctrl_handles(h);

        if (ret == NV_FALSE) goto done;
        
    } /* query */

    val = NV_TRUE;
    
 done:
    
    return val;
    
} /* process_attribute_queries() */



/*
 * process_attribute_assignments() - parse the list of
 * assignments, and call nv_process_parsed_attribute() to process
 * each assignment.
 *
 * If any errors are encountered, an error message is printed and
 * NV_FALSE is returned.  Otherwise, NV_TRUE is returned.
 *
 * XXX rather than call nv_alloc_ctrl_handles()/nv_free_ctrl_handles()
 * for every assignment, we should share the code in
 * process_config_file_attributes() to collapse the list of handles.
 */

static int process_attribute_assignments(int num, char **assignments,
                                         const char *display_name)
{
    int assignment, ret, val;
    ParsedAttribute a;
    CtrlHandles *h;

    val = NV_FALSE;

    /* loop over each requested assignment */

    for (assignment = 0; assignment < num; assignment++) {
        
        /* call the parser to parse assignments[assignment] */

        ret = nv_parse_attribute_string(assignments[assignment],
                                        NV_PARSER_ASSIGNMENT, &a);

        if (ret != NV_PARSER_STATUS_SUCCESS) {
            nv_error_msg("Error parsing assignment '%s' (%s).",
                         assignments[assignment], nv_parse_strerror(ret));
            goto done;
        }
        
        /* make sure we have a display */

        nv_assign_default_display(&a, display_name);

        /* allocate the CtrlHandles */

        h = nv_alloc_ctrl_handles(a.display);
        
        /* call the processing engine to process the parsed assignment */

        ret = nv_process_parsed_attribute(&a, h, NV_TRUE, NV_TRUE,
                                          "in assignment '%s'",
                                          assignments[assignment]);
        /* free the CtrlHandles */

        nv_free_ctrl_handles(h);

        if (ret == NV_FALSE) goto done;

    } /* assignment */

    val = NV_TRUE;

 done:

    return val;

} /* nv_process_attribute_assignments() */



/*
 * validate_value() - query the valid values for the specified
 * attribute, and check that the value to be assigned is valid.
 */

static int validate_value(CtrlHandles *h, ParsedAttribute *a, uint32 d,
                          int screen, char *whence)
{
    int bad_val = NV_FALSE;
    NVCTRLAttributeValidValuesRec valid;
    ReturnStatus status;
    char d_str[256];
    char *tmp_d_str;

    status = NvCtrlGetValidDisplayAttributeValues(h->h[screen], d,
                                                  a->attr, &valid);
   
    if (status != NvCtrlSuccess) {
        nv_error_msg("Unable to query valid values for attribute %s (%s).",
                     a->name, NvCtrlAttributesStrError(status));
        return NV_FALSE;
    }
    
    if (valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
        tmp_d_str = display_device_mask_to_display_device_name(d);
        sprintf(d_str, ", display device: %s", tmp_d_str);
        free(tmp_d_str);
    } else {
        d_str[0] = '\0';
    }
    
    switch (valid.type) {
    case ATTRIBUTE_TYPE_INTEGER:
    case ATTRIBUTE_TYPE_BITMASK:
        /* don't do any checks on integer or bitmask values */
        break;
    case ATTRIBUTE_TYPE_BOOL:
        if ((a->val < 0) || (a->val > 1)) {
            bad_val = NV_TRUE;
        }
        break;
    case ATTRIBUTE_TYPE_RANGE:
        if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            if (((a->val >> 16) < (valid.u.range.min >> 16)) ||
                ((a->val >> 16) > (valid.u.range.max >> 16)) ||
                ((a->val & 0xffff) < (valid.u.range.min & 0xffff)) ||
                ((a->val & 0xffff) > (valid.u.range.max & 0xffff)))
                bad_val = NV_TRUE;
        } else {
            if ((a->val < valid.u.range.min) || (a->val > valid.u.range.max))
                bad_val = NV_TRUE;
        }
        break;
    case ATTRIBUTE_TYPE_INT_BITS:
        if ((a->val > 31) || (a->val < 0) ||
            ((valid.u.bits.ints & (1<<a->val)) == 0)) {
            bad_val = NV_TRUE;
        }
        break;
    default:
        bad_val = NV_TRUE;
        break;
    }

    if (bad_val) {
        if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            nv_warning_msg("The value pair %d,%d for attribute '%s' (%s%s) "
                           "specified %s is invalid.",
                           a->val >> 16, a->val & 0xffff,
                           a->name, h->screen_names[screen],
                           d_str, whence);
        } else {
            nv_warning_msg("The value %d for attribute '%s' (%s%s) "
                           "specified %s is invalid.",
                           a->val, a->name, h->screen_names[screen],
                           d_str, whence);
        }
        print_valid_values(a->name, a->flags, valid);
        return NV_FALSE;
    }
    return NV_TRUE;

} /* validate_value() */



/*
 * print_valid_values() - prints the valid values for the specified
 * attribute.
 */

static void print_valid_values(char *name, uint32 flags,
                               NVCTRLAttributeValidValuesRec valid)
{
    int bit, first, last;
    char str[256];
    char *c;

#define INDENT "    "

    switch (valid.type) {
    case ATTRIBUTE_TYPE_INTEGER:
        if (flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            nv_msg(INDENT, "'%s' is a packed integer attribute.", name);
        } else {
            nv_msg(INDENT, "'%s' is an integer attribute.", name);
        }
        break;
        
    case ATTRIBUTE_TYPE_BITMASK:
        nv_msg(INDENT, "'%s' is a bitmask attribute.", name);
        break;
        
    case ATTRIBUTE_TYPE_BOOL:
        nv_msg(INDENT, "'%s' is a boolean attribute; valid values are: "
               "1 (on/true) and 0 (off/false).", name);
        break;
        
    case ATTRIBUTE_TYPE_RANGE:
        if (flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            nv_msg(INDENT, "The valid values for '%s' are in the ranges "
                   "%d - %d, %d - %d (inclusive).", name,
                   valid.u.range.min >> 16, valid.u.range.max >> 16,
                   valid.u.range.min & 0xffff, valid.u.range.max & 0xffff);
        } else {
            nv_msg(INDENT, "The valid values for '%s' are in the range "
                   "%d - %d (inclusive).", name,
                   valid.u.range.min, valid.u.range.max);
        }
        break;

    case ATTRIBUTE_TYPE_INT_BITS:
        first = last = -1;

        /* scan through the bitmask once to get first and last */

        for (bit = 0; bit < 32; bit++) {
            if (valid.u.bits.ints & (1 << bit)) {
                if (first == -1) first = bit;
                last = bit;
            }
        }

        /* now, scan through the bitmask again, building the string */

        str[0] = '\0';
        c = str;
        for (bit = 0; bit < 32; bit++) {
            if (valid.u.bits.ints & (1 << bit)) {
                if (bit == first) {
                    c += sprintf (c, "%d", bit);
                } else if (bit == last) {
                    c += sprintf (c, " and %d", bit);
                } else {
                    c += sprintf (c, ", %d", bit);
                }
            }
        }
        
        nv_msg(INDENT, "Valid values for '%s' are: %s.", name, str);
        break;
    }

    if (!(valid.permissions & ATTRIBUTE_TYPE_WRITE)) {
        nv_msg(INDENT, "'%s' is a read-only attribute.", name);
    }

    if (valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
        nv_msg(INDENT, "'%s' is display device specific.", name);
    }
   
#undef INDENT

} /* print_valid_values() */



/*
 * query_all() - loop through all screens, and query all attributes
 * for those screens.  The current attribute values for all display
 * devices on all screens are printed, along with the valid values for
 * each attribute.
 *
 * If an error occurs, an error message is printed and NV_FALSE is
 * returned; if successful, NV_TRUE is returned.
 */

static int query_all(const char *display_name)
{
    int bit, entry, screen, val;
    uint32 mask;
    ReturnStatus status;
    AttributeTableEntry *a;
    char *tmp_d_str;
    const char *fmt, *fmt_display;
    NVCTRLAttributeValidValuesRec valid;
    CtrlHandles *h;

    static const char *__fmt_str_int =
        "Attribute '%s' (screen: %s): %d.";
    static const char *__fmt_str_int_display =
        "Attribute '%s' (screen: %s; display: %s): %d.";
    static const char *__fmt_str_hex =
        "Attribute '%s' (screen: %s): 0x%08x.";
    static const char *__fmt_str_hex_display =
        "Attribute '%s' (screen: %s; display: %s): 0x%08x.";
    static const char *__fmt_str_packed_int =
        "Attribute '%s' (screen: %s): %d,%d.";
    static const char *__fmt_str_packed_int_display =
        "Attribute '%s' (screen: %s; display: %s): %d,%d.";
    
    h = nv_alloc_ctrl_handles(display_name);

#define INDENT "  "
    
    for (screen = 0; screen < h->num_screens; screen++) {

        if (!h->h[screen]) continue;

        nv_msg(NULL, "Attributes for %s:", h->screen_names[screen]);

        for (entry = 0; attributeTable[entry].name; entry++) {

            a = &attributeTable[entry];
            
            /* skip the color attributes */

            if (a->flags & NV_PARSER_TYPE_COLOR_ATTRIBUTE) continue;

            for (bit = 0; bit < 24; bit++) {
                mask = 1 << bit;

                if ((h->d[screen] & mask) == 0x0) continue;
                
                status =
                    NvCtrlGetValidDisplayAttributeValues(h->h[screen], mask,
                                                         a->attr, &valid);

                if (status == NvCtrlAttributeNotAvailable) goto exit_bit_loop;
                
                if (status != NvCtrlSuccess) {
                    nv_error_msg("Error while querying valid values for "
                                 "attribute '%s' on %s (%s).",
                                 a->name, h->screen_names[screen],
                                 NvCtrlAttributesStrError(status));
                    goto exit_bit_loop;
                }

                status = NvCtrlGetDisplayAttribute(h->h[screen], mask,
                                                   a->attr, &val);

                if (status == NvCtrlAttributeNotAvailable) goto exit_bit_loop;

                if (status != NvCtrlSuccess) {
                    nv_error_msg("Error while querying attribute '%s' "
                                 "on %s (%s).",
                                 a->name, h->screen_names[screen],
                                 NvCtrlAttributesStrError(status));
                    goto exit_bit_loop;
                }
                
                if (valid.type == ATTRIBUTE_TYPE_BITMASK) {
                    fmt = __fmt_str_hex;
                    fmt_display = __fmt_str_hex_display;
                } else if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
                    fmt = __fmt_str_packed_int;
                    fmt_display = __fmt_str_packed_int_display;
                } else {
                    fmt = __fmt_str_int;
                    fmt_display = __fmt_str_int_display;
                }

                if (valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
                    
                    tmp_d_str =
                        display_device_mask_to_display_device_name(mask);
                    
                    if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
                        nv_msg(INDENT, fmt_display, a->name,
                               h->screen_names[screen], tmp_d_str,
                               val >> 16, val & 0xffff);
                    } else {
                        nv_msg(INDENT, fmt_display, a->name,
                               h->screen_names[screen], tmp_d_str, val);
                    }
                    
                    free(tmp_d_str);

                    print_valid_values(a->name, a->flags, valid);

                    continue;

                } else {
                    if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
                        nv_msg(INDENT, fmt, a->name, h->screen_names[screen],
                               val >> 16, val & 0xffff);
                    } else {
                        nv_msg(INDENT, fmt, a->name, h->screen_names[screen],val);
                    }

                    print_valid_values(a->name, a->flags, valid);

                    /* fall through to exit_bit_loop */
                }
                
            exit_bit_loop:

                bit = 25; /* XXX force us out of the display device loop */
                
            } /* bit */
            
        } /* entry */
        
    } /* screen */

#undef INDENT

    nv_free_ctrl_handles(h);

    return NV_TRUE;

} /* query_all() */



/*
 * process_parsed_attribute_internal() - this function does the
 * actually attribute processing for NvCtrlProcessParsedAttribute().
 *
 * If an error occurs, an error message is printed and NV_FALSE is
 * returned; if successful, NV_TRUE is returned.
 */

static int process_parsed_attribute_internal(CtrlHandles *h,
                                             ParsedAttribute *a, uint32 d,
                                             int screen, int assign,
                                             int verbose, char *whence,
                                             NVCTRLAttributeValidValuesRec
                                             valid)
{
    ReturnStatus status;
    char str[32], *tmp_d_str;
    int ret;
    
    if (valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
        tmp_d_str = display_device_mask_to_display_device_name(d);
        sprintf(str, ", display device: %s", tmp_d_str);
        free(tmp_d_str);
    } else {
        str[0] = '\0';
    }

    if (assign) {
        ret = validate_value(h, a, d, screen, whence);
        if (!ret) return NV_FALSE;

        status = NvCtrlSetDisplayAttribute(h->h[screen], d, a->attr, a->val);
        if (status != NvCtrlSuccess) {
            nv_error_msg("Error assigning value %d to attribute '%s' "
                         "(%s%s) as specified %s (%s).",
                         a->val, a->name, h->screen_names[screen], str, whence,
                         NvCtrlAttributesStrError(status));
            return NV_FALSE;
        }
        
        if (verbose) {
            if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
                nv_msg("  ", "Attribute '%s' (%s%s) assigned value %d,%d.",
                       a->name, h->screen_names[screen], str,
                       a->val >> 16, a->val & 0xffff);
            } else {
                nv_msg("  ", "Attribute '%s' (%s%s) assigned value %d.",
                       a->name, h->screen_names[screen], str, a->val);
            }
        }

    } else { /* query */
        
        status = NvCtrlGetDisplayAttribute(h->h[screen], d, a->attr, &a->val);

        if (status == NvCtrlAttributeNotAvailable) {
            nv_warning_msg("Error querying attribute '%s' specified %s; "
                           "'%s' is not available on %s%s.",
                           a->name, whence, a->name,
                           h->screen_names[screen], str);
        } else if (status != NvCtrlSuccess) {
            nv_error_msg("Error while querying attribute '%s' "
                         "(%s%s) specified %s (%s).",
                         a->name, h->screen_names[screen], str, whence,
                         NvCtrlAttributesStrError(status));
            return NV_FALSE;
        } else {
            if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
                nv_msg("  ", "Attribute '%s' (%s%s): %d,%d.",
                       a->name, h->screen_names[screen], str,
                       a->val >> 16, a->val & 0xffff);
            } else {
                nv_msg("  ", "Attribute '%s' (%s%s): %d.",
                       a->name, h->screen_names[screen], str, a->val);
            }
            print_valid_values(a->name, a->flags, valid);
        }
    }

    return NV_TRUE;

} /* process_parsed_attribute_internal() */



/*
 * nv_process_parsed_attribute() - this is the processing engine for
 * all parsed attributes.
 *
 * A parsed attribute may or may not specify an X screen; if an X
 * screen was specified, we validate that screen and process the
 * attribute just for that screen.  If a screen was not specified, we
 * process the attribute for all valid screens.
 *
 * A parsed attribute may or may not specify one or more display
 * devices.  For attributes that require that a display device be
 * specified: if a display device mask is specified, we validate it
 * and process the attribute just for the display devices in the mask.
 * If a display device mask was not specified, then we process the
 * attribute for all enabled display devices on each of the screens
 * that have been requested.
 *
 * "Processing" a parsed attribute means either querying for the
 * current value of the attribute on all requested screens and display
 * devices (see above), or assigning the attribute on all requested
 * screens and display devices (see above).
 *
 * The majority of the work (determining which screens, which display
 * devices) is the same, regardless of what sort of processing we
 * actually need to do (thus this shared function).
 *
 * To accomodate the differences in processing needed for each of the
 * callers of this function, the paramenters 'assign' and 'verbose'
 * are used; if assign is TRUE, then the attribute will be assigned
 * during processing, otherwise it will be queried.  If verbose is
 * TRUE, then a message will be printed out during each assignment (or
 * query).
 *
 * The CtrlHandles argument contains an array of
 * NvCtrlAttributeHandle's (one per X screen on this X server), as
 * well as the number of X screens, an array of enabled display
 * devices for each screen, and a string description of each screen.
 *
 * The whence_fmt and following varargs are used by the callee to
 * describe where the attribute came from.  A whence string should be
 * something like "on line 12 of config file ~/.nvidia-settings-rc" or
 * "in query ':0.0/fsaa'".  Whence is used in the case of error to
 * indicate where the error came from.
 *
 * If successful, the processing determined by 'assign' and 'verbose'
 * will be done and NV_TRUE will be returned.  If an error occurs, an
 * error message will be printed and NV_FALSE will be returned.
 */

int nv_process_parsed_attribute(ParsedAttribute *a, CtrlHandles *h,
                                int assign, int verbose,
                                char *whence_fmt, ...)
{
    int screen, start_screen, end_screen, bit, ret, val;
    char *whence, *tmp_d_str0, *tmp_d_str1;
    uint32 display_devices, mask;
    ReturnStatus status;
    NVCTRLAttributeValidValuesRec valid;
    
    val = NV_FALSE;

    /* build the description */

    NV_VSNPRINTF(whence, whence_fmt);
    
    if (!whence) whence = strdup("\0");

    /* if we don't have a Display connection, abort now */

    if (!h->dpy) {
        nv_error_msg("Unable to %s attribute %s specified %s (no Display "
                     "connection).", assign ? "assign" : "query",
                     a->name, whence);
        goto done;
    }
    
    /* if a screen was specified, make sure it is valid */

    if (a->flags & NV_PARSER_HAS_X_SCREEN) {
        
        if (a->screen >= h->num_screens) {
            if (h->num_screens == 1) {
                nv_error_msg("Invalid X screen %d specified %s "
                             "(there is only 1 screen on this Display).",
                             a->screen, whence);
            } else {
                nv_error_msg("Invalid X screen %d specified %s "
                             "(there are only %d screens on this Display).",
                             a->screen, whence, h->num_screens);
            }
            goto done;
        }

        if (!h->h[a->screen]) {
            nv_error_msg("Invalid screen %d specified %s "
                         "(NV-CONTROL extension not supported on "
                         "screen %d).", a->screen, whence, a->screen);
        }
        
        /*
         * assign start_screen and end_screen such that the screen
         * loop only uses this screen.
         */
        
        start_screen = a->screen;
        end_screen = a->screen + 1;
    } else {
        
        /*
         * no screen was specified; assign start_screen and end_screen
         * such that we loop over all the screens
         */
        
        start_screen = 0;
        end_screen = h->num_screens;
    }

    /* loop over the requested screens */

    for (screen = start_screen; screen < end_screen; screen++) {
            
        if (!h->h[screen]) continue; /* no handle on this screen; skip */

        if (a->flags & NV_PARSER_HAS_DISPLAY_DEVICE) {

            /* Expand any wildcards in the display device mask */
        
            display_devices = expand_display_device_mask_wildcards
                (a->display_device_mask, h->d[screen]);
            
            if ((display_devices == 0) || (display_devices & ~h->d[screen])) {

                /*
                 * use a->display_device_mask rather than
                 * display_devices when building the string (so that
                 * display_device_mask_to_display_device_name() can
                 * use wildcards if present).
                 */

                tmp_d_str0 = display_device_mask_to_display_device_name
                    (a->display_device_mask);
                tmp_d_str1 = display_device_mask_to_display_device_name
                    (h->d[screen]);

                nv_error_msg("Invalid display device %s specified "
                             "%s (the currently enabled display devices "
                             "are %s on %s).",
                             tmp_d_str0, whence, tmp_d_str1,
                             h->screen_names[screen]);
                free(tmp_d_str0);
                free(tmp_d_str1);
                
                goto done;
            }
            
        } else {
            
            display_devices = h->d[screen];
        }
        
        /* special case the color attributes */
        
        if (a->flags & NV_PARSER_TYPE_COLOR_ATTRIBUTE) {
            float v[3];
            if (!assign) {
                nv_error_msg("Cannot query attribute '%s'", a->name);
                goto done;
            }
            
            /*
             * assign fval to all values in the array; a->attr will
             * tell NvCtrlSetColorAttributes() which indices in the
             * array to use
             */

            v[0] = v[1] = v[2] = a->fval;

            status = NvCtrlSetColorAttributes(h->h[screen], v, v, v, a->attr);
            if (status != NvCtrlSuccess) {
                nv_error_msg("Error assigning %f to attribute '%s' on %s "
                             "specified %s (%s)", a->fval, a->name,
                             h->screen_names[screen], whence,
                             NvCtrlAttributesStrError(status));
                goto done;
            }

            continue;
        }

        for (bit = 0; bit < 24; bit++) {
            
            mask = (1 << bit);

            if ((mask & display_devices) == 0x0) continue;
            
            status = NvCtrlGetValidDisplayAttributeValues(h->h[screen], mask,
                                                          a->attr, &valid);
            if (status != NvCtrlSuccess) {
                if(status == NvCtrlAttributeNotAvailable) {
                    nv_warning_msg("Attribute '%s' specified %s is not "
                                   "available on %s.",
                                   a->name, whence, h->screen_names[screen]);
                } else {
                    nv_error_msg("Error querying valid values for attribute "
                                 "'%s' on %s specified %s (%s).",
                                 a->name, h->screen_names[screen], whence,
                                 NvCtrlAttributesStrError(status));
                }
                goto done;
            }
            
            /*
             * if this attribute is going to be assigned, then check
             * that the attribute is writable; if it's not, give up
             */

            if ((assign) && (!(valid.permissions & ATTRIBUTE_TYPE_WRITE))) {
                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "assigned (it is a read-only attribute).",
                             a->name, whence);
                goto done;
            }
            
            ret = process_parsed_attribute_internal(h, a, mask, screen,
                                                    assign, verbose,
                                                    whence, valid);
            if (ret == NV_FALSE) goto done;
            
            if (!(valid.permissions & ATTRIBUTE_TYPE_DISPLAY)) bit = 25;

        } /* bit */
        
    } /* screen */

    val = NV_TRUE;
    
 done:
    if (whence) free(whence);
    return val;

} /* nv_process_parsed_attribute() */
