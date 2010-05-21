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
#include <inttypes.h>

#include "parse.h"
#include "msg.h"
#include "query-assign.h"

extern int __verbosity;
extern int __terse;
extern int __display_device_string;

/* local prototypes */

static int process_attribute_queries(int, char**, const char *);

static int process_attribute_assignments(int, char**, const char *);

static int query_all(const char *);
static int query_all_targets(const char *display_name, const int target_index);

static void print_valid_values(char *, int, uint32, NVCTRLAttributeValidValuesRec);

static void print_additional_info(const char *name,
                                  int attr,
                                  NVCTRLAttributeValidValuesRec valid,
                                  const char *indent);

static int validate_value(CtrlHandleTarget *t, ParsedAttribute *a, uint32 d,
                          int target_type, char *whence);

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
 * nv_alloc_ctrl_handles() - allocate a new CtrlHandles structure,
 * connect to the X server identified by display, and initialize an
 * NvCtrlAttributeHandle for each possible target (X screens, gpus,
 * FrameLock devices).
 */

CtrlHandles *nv_alloc_ctrl_handles(const char *display)
{
    ReturnStatus status;
    CtrlHandles *h, *pQueryHandle = NULL;
    NvCtrlAttributeHandle *handle;
    int target, i, j, val, d, c, len;
    char *tmp;

    /* allocate the CtrlHandles struct */
    
    h = calloc(1, sizeof(CtrlHandles));
    
    /* store any given X display name */

    if (display) h->display = strdup(display);
    else h->display = NULL;
    
    /* open the X display connection */

    h->dpy = XOpenDisplay(h->display);

    if (!h->dpy) {
        nv_error_msg("Cannot open display '%s'.", XDisplayName(h->display));
        return h;
    }
    
    /*
     * loop over each target type and setup the appropriate
     * information
     */
    
    for (j = 0; targetTypeTable[j].name; j++) {
        
        /* extract the target index from the table */

        target = targetTypeTable[j].target_index;
        
        /* 
         * get the number of targets of this type; if this is an X
         * screen target, just use Xlib's ScreenCount() (note: to
         * support Xinerama: we'll want to use
         * NvCtrlQueryTargetCount() rather than ScreenCount()); for
         * other target types, use NvCtrlQueryTargetCount().
         */
        
        if (target == X_SCREEN_TARGET) {
            
            h->targets[target].n = ScreenCount(h->dpy);
            
        } else {
    
            /*
             * note: pQueryHandle should be assigned below by a
             * previous iteration of this loop; depends on X screen
             * targets getting handled first
             */
            
            if (pQueryHandle) {

                /*
                 * check that the NV-CONTROL protocol is new enough to
                 * recognize this target type
                 */

                ReturnStatus ret1, ret2;
                int major, minor;

                ret1 = NvCtrlGetAttribute(pQueryHandle,
                                          NV_CTRL_ATTR_NV_MAJOR_VERSION,
                                          &major);
                ret2 = NvCtrlGetAttribute(pQueryHandle,
                                          NV_CTRL_ATTR_NV_MINOR_VERSION,
                                          &minor);

                if ((ret1 == NvCtrlSuccess) && (ret2 == NvCtrlSuccess) &&
                    ((major > targetTypeTable[j].major) ||
                     ((major == targetTypeTable[j].major) &&
                      (minor >= targetTypeTable[j].minor)))) {

                    status = NvCtrlQueryTargetCount
                                (pQueryHandle, targetTypeTable[j].nvctrl,
                                 &val);
                } else {
                    status = NvCtrlMissingExtension;
                }
            } else {
                status = NvCtrlMissingExtension;
            }
            
            if (status != NvCtrlSuccess) {
                nv_warning_msg("Unable to determine number of NVIDIA "
                             "%ss on '%s'.",
                             targetTypeTable[j].name,
                             XDisplayName(h->display));
                val = 0;
            }
            
            h->targets[target].n = val;
        }
    
        /* if there are no targets of this type, skip */

        if (h->targets[target].n == 0) continue;
        
        /* allocate an array of CtrlHandleTarget's */

        h->targets[target].t =
            calloc(h->targets[target].n, sizeof(CtrlHandleTarget));
        
        /*
         * loop over all the targets of this type and setup the
         * CtrlHandleTarget's
         */

        for (i = 0; i < h->targets[target].n; i++) {
        
            /* allocate the handle */
            
            handle = NvCtrlAttributeInit(h->dpy,
                                         targetTypeTable[j].nvctrl, i,
                                         NV_CTRL_ATTRIBUTES_ALL_SUBSYSTEMS);
            
            h->targets[target].t[i].h = handle;
            
            /*
             * silently fail: this might happen if not all X screens
             * are NVIDIA X screens
             */
            
            if (!handle) continue;
            
            /*
             * get a name for this target; in the case of
             * X_SCREEN_TARGET targets, just use the string returned
             * from NvCtrlGetDisplayName(); for other target types,
             * append a target specification.
             */
            
            tmp = NvCtrlGetDisplayName(handle);
            
            if (target == X_SCREEN_TARGET) {
                h->targets[target].t[i].name = tmp;
            } else {
                len = strlen(tmp) + strlen(targetTypeTable[j].parsed_name) +16;
                h->targets[target].t[i].name = malloc(len);

                if (h->targets[target].t[i].name) {
                    snprintf(h->targets[target].t[i].name, len, "%s[%s:%d]",
                             tmp, targetTypeTable[j].parsed_name, i);
                    free(tmp);
                } else {
                    h->targets[target].t[i].name = tmp;
                }
            }

            /*
             * get the enabled display device mask; for X screens and
             * GPUs we query NV-CONTROL; for anything else
             * (framelock), we just assign this to 0.
             */

            if (targetTypeTable[j].uses_display_devices) {
                
                status = NvCtrlGetAttribute(handle,
                                            NV_CTRL_ENABLED_DISPLAYS, &d);
        
                if (status != NvCtrlSuccess) {
                    nv_error_msg("Error querying enabled displays on "
                                 "%s %d (%s).", targetTypeTable[j].name, i,
                                 NvCtrlAttributesStrError(status));
                    d = 0;
                }
                
                status = NvCtrlGetAttribute(handle,
                                            NV_CTRL_CONNECTED_DISPLAYS, &c);
        
                if (status != NvCtrlSuccess) {
                    nv_error_msg("Error querying connected displays on "
                                 "%s %d (%s).", targetTypeTable[j].name, i,
                                 NvCtrlAttributesStrError(status));
                    c = 0;
                }
            } else {
                d = 0;
                c = 0;
            }
             
            h->targets[target].t[i].d = d;
            h->targets[target].t[i].c = c;

            /*
             * store this handle so that we can use it to query other
             * target counts later
             */
            
            if (!pQueryHandle) pQueryHandle = handle;
        }
    }
    
    return h;

} /* nv_alloc_ctrl_handles() */


/*
 * nv_free_ctrl_handles() - free the CtrlHandles structure allocated
 * by nv_alloc_ctrl_handles()
 */

void nv_free_ctrl_handles(CtrlHandles *h)
{
    int i, j, target;

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
        
        for (j = 0; targetTypeTable[j].name; j++) {
            
            target = targetTypeTable[j].target_index;
            
            for (i = 0; i < h->targets[target].n; i++) {
                
                NvCtrlAttributeClose(h->targets[target].t[i].h);
                
                if (h->targets[target].t[i].name) {
                    free(h->targets[target].t[i].name);
                }
            }
            
            if (h->targets[target].t) free(h->targets[target].t);
        }
    }
    
    free(h);
    
} /* nv_free_ctrl_handles() */



/*
 * process_attribute_queries() - parse the list of queries, and call
 * nv_ctrl_process_parsed_attribute() to process each query.
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
    
    /* print a newline before we begin */

    if (!__terse) nv_msg(NULL, "");

    /* loop over each requested query */

    for (query = 0; query < num; query++) {
        
        /* special case the "all" query */

        if (nv_strcasecmp(queries[query], "all")) {
            query_all(display_name);
            continue;
        }

        /* special case the target type queries */
        
        if (nv_strcasecmp(queries[query], "screens") ||
            nv_strcasecmp(queries[query], "xscreens")) {
            query_all_targets(display_name, X_SCREEN_TARGET);
            continue;
        }
        
        if (nv_strcasecmp(queries[query], "gpus")) {
            query_all_targets(display_name, GPU_TARGET);
            continue;
        }

        if (nv_strcasecmp(queries[query], "framelocks")) {
            query_all_targets(display_name, FRAMELOCK_TARGET);
            continue;
        }

        if (nv_strcasecmp(queries[query], "vcs")) {
            query_all_targets(display_name, VCS_TARGET);
            continue;
        }

        if (nv_strcasecmp(queries[query], "gvis")) {
            query_all_targets(display_name, GVI_TARGET);
            continue;
        }

        if (nv_strcasecmp(queries[query], "fans")) {
            query_all_targets(display_name, COOLER_TARGET);
            continue;
        }

        if (nv_strcasecmp(queries[query], "thermalsensors")) {
            query_all_targets(display_name, THERMAL_SENSOR_TARGET);
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
        
        /* print a newline at the end */
        if (!__terse) nv_msg(NULL, "");

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

    /* print a newline before we begin */

    nv_msg(NULL, "");

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

        /* print a newline at the end */

        nv_msg(NULL, "");

    } /* assignment */

    val = NV_TRUE;

 done:

    return val;

} /* nv_process_attribute_assignments() */



/*
 * validate_value() - query the valid values for the specified
 * attribute, and check that the value to be assigned is valid.
 */

static int validate_value(CtrlHandleTarget *t, ParsedAttribute *a, uint32 d,
                          int target_type, char *whence)
{
    int i, bad_val = NV_FALSE;
    NVCTRLAttributeValidValuesRec valid;
    ReturnStatus status;
    char d_str[256];
    char *tmp_d_str;

    status = NvCtrlGetValidDisplayAttributeValues(t->h, d, a->attr, &valid);
   
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
        if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            unsigned int u, l;

             u = (((unsigned int) a->val) >> 16);
             l = (a->val & 0xffff);

             if ((u > 15) || ((valid.u.bits.ints & (1 << u << 16)) == 0) ||
                 (l > 15) || ((valid.u.bits.ints & (1 << l)) == 0)) {
                bad_val = NV_TRUE;
            }
        } else {
            if ((a->val > 31) || (a->val < 0) ||
                ((valid.u.bits.ints & (1<<a->val)) == 0)) {
                bad_val = NV_TRUE;
            }
        }
        break;
    default:
        bad_val = NV_TRUE;
        break;
    }

    /* is this value available for this target type? */

    for (i = 0; targetTypeTable[i].name; i++) {
        if ((targetTypeTable[i].target_index == target_type) &&
            !(targetTypeTable[i].permission_bit & valid.permissions)) {
            bad_val = NV_TRUE;
            break;
        }
    }

    /* if the value is bad, print why */
    
    if (bad_val) {
        if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            nv_warning_msg("The value pair %d,%d for attribute '%s' (%s%s) "
                           "specified %s is invalid.",
                           a->val >> 16, a->val & 0xffff,
                           a->name, t->name,
                           d_str, whence);
        } else {
            nv_warning_msg("The value %d for attribute '%s' (%s%s) "
                           "specified %s is invalid.",
                           a->val, a->name, t->name,
                           d_str, whence);
        }
        print_valid_values(a->name, a->attr, a->flags, valid);
        return NV_FALSE;
    }
    return NV_TRUE;

} /* validate_value() */



/*
 * print_valid_values() - prints the valid values for the specified
 * attribute.
 */

static void print_valid_values(char *name, int attr, uint32 flags,
                               NVCTRLAttributeValidValuesRec valid)
{
    int bit, print_bit, last, last2, i, n;
    char str[256];
    char *c;
    char str2[256];
    char *c2; 
    char **at;

    /* do not print any valid values information when we are in 'terse' mode */

    if (__terse) return;

#define INDENT "    "

    switch (valid.type) {
    case ATTRIBUTE_TYPE_STRING:
        nv_msg(INDENT, "'%s' is a string attribute.", name);
        break;

    case ATTRIBUTE_TYPE_64BIT_INTEGER:
        nv_msg(INDENT, "'%s' is a 64 bit integer attribute.", name);
        break;

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
                   "%" PRId64 " - %" PRId64 ", %" PRId64 " - %" PRId64
                   " (inclusive).",
                   name, valid.u.range.min >> 16, valid.u.range.max >> 16,
                   valid.u.range.min & 0xffff, valid.u.range.max & 0xffff);
        } else {
            nv_msg(INDENT, "The valid values for '%s' are in the range "
                   "%" PRId64 " - %" PRId64 " (inclusive).", name,
                   valid.u.range.min, valid.u.range.max);
        }
        break;

    case ATTRIBUTE_TYPE_INT_BITS:
        last = last2 = -1;

        /* scan through the bitmask once to get the last valid bits */

        for (bit = 0; bit < 32; bit++) {
            if (valid.u.bits.ints & (1 << bit)) {
                if ((bit > 15) && (flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE)) {
                    last2 = bit;
                } else {
                    last = bit;
                }
            }
        }

        /* now, scan through the bitmask again, building the string */

        str[0] = '\0';
        str2[0] = '\0';
        c = str;
        c2 = str2;
        for (bit = 0; bit < 32; bit++) {
            
            if ((bit > 15) && (flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE)) {
                print_bit = bit - 16;
                at = &c2;
            } else {
                print_bit = bit;
                at = &c;
            }

            if (valid.u.bits.ints & (1 << bit)) {
                if (*at == str || *at == str2) {
                    *at += sprintf(*at, "%d", print_bit);
                } else if (bit == last || bit == last2) {
                    *at += sprintf(*at, " and %d", print_bit);
                } else {
                    *at += sprintf(*at, ", %d", print_bit);
                }
            }
        }

        if (flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            nv_msg(INDENT, "Valid values for '%s' are: [%s], [%s].", name, str2,
                   str);
        } else {
            nv_msg(INDENT, "Valid values for '%s' are: %s.", name, str);
        }
        break;
    }

    if (!(valid.permissions & ATTRIBUTE_TYPE_WRITE)) {
        nv_msg(INDENT, "'%s' is a read-only attribute.", name);
    }

    if (valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
        nv_msg(INDENT, "'%s' is display device specific.", name);
    }

    /* print the valid target types */
    
    c = str;
    n = 0;
    
    for (i = 0; targetTypeTable[i].name; i++) {
        if (valid.permissions & targetTypeTable[i].permission_bit) {
            if (n > 0) c += sprintf(c, ", ");
            c += sprintf(c, targetTypeTable[i].name);
            n++;
        }
    }
        
    if (n == 0) sprintf(c, "None");
    
    nv_msg(INDENT, "'%s' can use the following target types: %s.",
           name, str);
   
    if (__verbosity >= VERBOSITY_ALL)
        print_additional_info(name, attr, valid, INDENT);

#undef INDENT

} /* print_valid_values() */



/*
 * print_queried_value() - print the attribute value that we queried
 * from NV-CONTROL
 */

typedef enum {
    VerboseLevelTerse,
    VerboseLevelAbbreviated,
    VerboseLevelVerbose,
} VerboseLevel;

static void print_queried_value(CtrlHandleTarget *t,
                                NVCTRLAttributeValidValuesRec *v,
                                int val,
                                uint32 flags,
                                char *name,
                                uint32 mask,
                                const char *indent,
                                const VerboseLevel level)
{
    char d_str[64], val_str[64], *tmp_d_str;

    /* assign val_str */

    if ((flags & NV_PARSER_TYPE_VALUE_IS_DISPLAY) &&
        (__display_device_string)) {
        tmp_d_str = display_device_mask_to_display_device_name(val);
        snprintf(val_str, 64, "%s", tmp_d_str);
        free(tmp_d_str);
    } else if (flags & NV_PARSER_TYPE_100Hz) {
        snprintf(val_str, 64, "%.2f Hz", ((float) val) / 100.0);
    } else if (flags & NV_PARSER_TYPE_1000Hz) {
        snprintf(val_str, 64, "%.3f Hz", ((float) val) / 1000.0);
    } else if (v->type == ATTRIBUTE_TYPE_BITMASK) {
        snprintf(val_str, 64, "0x%08x", val);
    } else if (flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
        snprintf(val_str, 64, "%d,%d", val >> 16, val & 0xffff);
    } else {
        snprintf(val_str, 64, "%d", val);
    }

    /* append the display device name, if necessary */

    if (v->permissions & ATTRIBUTE_TYPE_DISPLAY) {
        tmp_d_str = display_device_mask_to_display_device_name(mask);
        snprintf(d_str, 64, "; display device: %s", tmp_d_str);
        free(tmp_d_str);
    } else {
        d_str[0] = '\0';
    }

    /* print the value */

    switch (level) {

    case VerboseLevelTerse:
        /* print value alone */
        nv_msg(NULL, "%s", val_str);
        break;

    case VerboseLevelAbbreviated:
        /* print the value with indentation and the attribute name */
        nv_msg(indent, "%s: %s", name, val_str);
        break;

    case VerboseLevelVerbose:
        /*
         * or print the value along with other information about the
         * attribute
         */
        nv_msg(indent,  "Attribute '%s' (%s%s): %s.",
               name, t->name, d_str, val_str);
        break;
    }

} /* print_queried_value() */



/*
 * print_additional_fsaa_info() - print the currently available fsaa
 * modes with their corresponding names
 */

static void print_additional_fsaa_info(const char *name,
                                       unsigned int valid_fsaa_modes,
                                       const char *indent)
{
    int bit;

    nv_msg(indent, "\n");
    nv_msg(indent, "Note to assign 'FSAA' on the commandline, you may also "
           "need to assign\n");
    nv_msg(indent, "'FSAAAppControlled' and 'FSAAAppEnhanced' to 0.\n");
    nv_msg(indent, "\n");
    nv_msg(indent, "Valid '%s' Values\n", name);
    nv_msg(indent, "  value - description\n");

    for (bit = 0; bit < 32; bit++) {
        /* FSAA is not a packed attribute */
        if (valid_fsaa_modes & (1 << bit)) {
            nv_msg(indent, "   %2u   -   %s\n", 
                   bit, NvCtrlGetMultisampleModeName(bit));
        }
    }
}



/*
 * print_additional_info() - after printing the main information about
 * a queried attribute, we may want to add some more when in verbose mode.
 * This function is designed to handle this. Add a new 'case' here when
 * you want to print this additional information for a specific attr.
 */

static void print_additional_info(const char *name,
                                  int attr,
                                  NVCTRLAttributeValidValuesRec valid,
                                  const char *indent)
{
    switch (attr) {

    case NV_CTRL_FSAA_MODE:
        print_additional_fsaa_info(name, valid.u.bits.ints, indent);
        break;

    // add more here

    }

}



/*
 * query_all() - loop through all target types, and query all attributes
 * for those targets.  The current attribute values for all display
 * devices on all targets are printed, along with the valid values for
 * each attribute.
 *
 * If an error occurs, an error message is printed and NV_FALSE is
 * returned; if successful, NV_TRUE is returned.
 */

static int query_all(const char *display_name)
{
    int bit, entry, target_id, val;
    int target_type;
    uint32 mask;
    ReturnStatus status;
    AttributeTableEntry *a;
    NVCTRLAttributeValidValuesRec valid;
    CtrlHandles *h;
    CtrlHandleTarget *t;
    
    h = nv_alloc_ctrl_handles(display_name);

#define INDENT "  "
    
    /*
     * Loop through all target types.
     */

    for (target_type = 0; target_type < MAX_TARGET_TYPES; target_type++) {
        
        for (target_id = 0; target_id < h->targets[target_type].n; target_id++) {

            t = &h->targets[target_type].t[target_id];

            if (!t->h) continue;

            nv_msg(NULL, "Attributes queryable via %s:", t->name);

            if (!__terse) nv_msg(NULL, "");

            for (entry = 0; attributeTable[entry].name; entry++) {

                a = &attributeTable[entry];

                /* skip the color attributes */

                if (a->flags & NV_PARSER_TYPE_COLOR_ATTRIBUTE) continue;

                /* skip attributes that shouldn't be queried here */

                if (a->flags & NV_PARSER_TYPE_NO_QUERY_ALL) continue;

                for (bit = 0; bit < 24; bit++) {
                    mask = 1 << bit;

                    /*
                     * if this bit is not present in the screens's enabled
                     * display device mask (and the X screen has enabled
                     * display devices), skip to the next bit
                     */

                    if (((t->d & mask) == 0x0) && (t->d)) continue;

                    if (a->flags & NV_PARSER_TYPE_STRING_ATTRIBUTE) {
                        char *tmp_str = NULL;

                        status = NvCtrlGetValidStringDisplayAttributeValues(t->h, mask,
                                                                            a->attr, &valid);
                        if (status == NvCtrlAttributeNotAvailable) goto exit_bit_loop;

                        if (status != NvCtrlSuccess) {
                            nv_error_msg("Error while querying valid values for "
                                         "attribute '%s' on %s (%s).",
                                         a->name, t->name,
                                         NvCtrlAttributesStrError(status));
                            goto exit_bit_loop;
                        }
                        
                        status = NvCtrlGetStringDisplayAttribute(t->h, mask, a->attr, &tmp_str);

                        if (status == NvCtrlAttributeNotAvailable) goto exit_bit_loop;

                        if (status != NvCtrlSuccess) {
                            nv_error_msg("Error while querying attribute '%s' "
                                         "on %s (%s).", a->name, t->name,
                                         NvCtrlAttributesStrError(status));
                            goto exit_bit_loop;
                        }

                        if (__terse) {
                            nv_msg("  ", "%s: %s", a->name, tmp_str);
                        } else {
                            nv_msg("  ",  "Attribute '%s' (%s%s): %s ",
                                   a->name, t->name, "", tmp_str);
                        }
                        free(tmp_str);
                        tmp_str = NULL;

                    } else {
                        
                        status = NvCtrlGetValidDisplayAttributeValues(t->h, mask, a->attr, &valid);

                        if (status == NvCtrlAttributeNotAvailable) goto exit_bit_loop;

                        if (status != NvCtrlSuccess) {
                            nv_error_msg("Error while querying valid values for "
                                         "attribute '%s' on %s (%s).",
                                         a->name, t->name,
                                         NvCtrlAttributesStrError(status));
                            goto exit_bit_loop;
                        }

                        status = NvCtrlGetDisplayAttribute(t->h, mask, a->attr, &val);

                        if (status == NvCtrlAttributeNotAvailable) goto exit_bit_loop;

                        if (status != NvCtrlSuccess) {
                            nv_error_msg("Error while querying attribute '%s' "
                                         "on %s (%s).", a->name, t->name,
                                         NvCtrlAttributesStrError(status));
                            goto exit_bit_loop;
                        }

                        print_queried_value(t, &valid, val, a->flags, a->name,
                                            mask, INDENT, __terse ?
                                            VerboseLevelAbbreviated :
                                            VerboseLevelVerbose);

                    }
                    print_valid_values(a->name, a->attr, a->flags, valid);

                    if (!__terse) nv_msg(NULL,"");

                    if (valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
                        continue;
                    }

                    /* fall through to exit_bit_loop */

exit_bit_loop:

                    bit = 25; /* XXX force us out of the display device loop */

                } /* bit */

            } /* entry */

        } /* target_id */

    } /* target_type */

#undef INDENT

    nv_free_ctrl_handles(h);

    return NV_TRUE;

} /* query_all() */



/*
 * print_target_display_connections() - Print a list of all the
 * display devices connected to the given target (GPU/X Screen)
 */

static int print_target_display_connections(CtrlHandleTarget *t)
{
    unsigned int bit;
    char *tmp_d_str, *name;
    ReturnStatus status;
    int plural;

    if (t->c == 0) {
        nv_msg("      ", "Is not connected to any display devices.");
        nv_msg(NULL, "");
        return NV_TRUE;
    }

    plural = (t->c & (t->c - 1)); /* Is more than one bit set? */

    nv_msg("      ", "Is connected to the following display device%s:",
           plural ? "s" : "");

    for (bit = 1; bit; bit <<= 1) {
        if (!(bit & t->c)) continue;
        
        status =
            NvCtrlGetStringDisplayAttribute(t->h, bit,
                                            NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                            &name);
        if (status != NvCtrlSuccess) name = strdup("Unknown");
        
        tmp_d_str = display_device_mask_to_display_device_name(bit);
        nv_msg("        ", "%s (%s: 0x%0.8X)", name, tmp_d_str, bit);
        
        free(name);
        free(tmp_d_str);
    }
    nv_msg(NULL, "");

    return NV_TRUE;

} /* print_target_display_connections() */



/*
 * get_vcs_name() Returns the VCS product name of the given
 * VCS target.
 */

static char * get_vcs_name(NvCtrlAttributeHandle *h)
{
    char *product_name;
    ReturnStatus status;

    status = NvCtrlGetStringAttribute(h, NV_CTRL_STRING_VCSC_PRODUCT_NAME,
                                      &product_name);
    
    if (status != NvCtrlSuccess) return strdup("Unknown");
    
    return product_name;

} /* get_vcs_name() */



/*
 * get_gpu_name() Returns the GPU product name of the given
 * GPU target.
 */

static char * get_gpu_name(NvCtrlAttributeHandle *h)
{
    char *product_name;
    ReturnStatus status;

    status = NvCtrlGetStringAttribute(h, NV_CTRL_STRING_PRODUCT_NAME,
                                      &product_name);
    
    if (status != NvCtrlSuccess) return strdup("Unknown");
    
    return product_name;

} /* get_gpu_name() */



/*
 * print_target_connections() Prints a list of all targets connected
 * to a given target using print_func if the connected targets require
 * special handling.
 */

static int print_target_connections(CtrlHandles *h,
                                    CtrlHandleTarget *t,
                                    unsigned int attrib,
                                    unsigned int target_index)
{
    int *pData;
    int len, i;
    ReturnStatus status;
    char *product_name;
    char *target_name;


    /* Query the connected targets */

    status =
        NvCtrlGetBinaryAttribute(t->h, 0, attrib,
                                 (unsigned char **) &pData,
                                 &len);
    if (status != NvCtrlSuccess) return NV_FALSE;

    if (pData[0] == 0) {
        nv_msg("      ", "Is not connected to any %s.",
               targetTypeTable[target_index].name);
        nv_msg(NULL, "");

        XFree(pData);
        return NV_TRUE;
    }

    nv_msg("      ", "Is connected to the following %s%s:",
           targetTypeTable[target_index].name,
           ((pData[0] > 1) ? "s" : ""));


    /* List the connected targets */

    for (i = 1; i <= pData[0]; i++) {
        
        if (pData[i] >= 0 &&
            pData[i] < h->targets[target_index].n) {

            target_name = h->targets[target_index].t[ pData[i] ].name;

            switch (target_index) {
            case GPU_TARGET:
                product_name =
                    get_gpu_name(h->targets[target_index].t[ pData[i] ].h);
                break;
                
            case VCS_TARGET:
                product_name =
                    get_vcs_name(h->targets[target_index].t[ pData[i] ].h);
                break;
                
            case THERMAL_SENSOR_TARGET:
            case COOLER_TARGET:
            case FRAMELOCK_TARGET:
            case GVI_TARGET:
            case X_SCREEN_TARGET:
            default:
                product_name = NULL;
                break;
            }

        } else {
            target_name = NULL;
            product_name = NULL;
        }

        if (!target_name) {
            nv_msg("        ", "[?] Unknown %s",
                   targetTypeTable[target_index].name);

        } else if (product_name) {
            nv_msg("        ", "[%d] %s (%s)",
                   pData[i], target_name, product_name);

        } else {
            nv_msg("        ", "[%d] %s (%s %d)",
                   pData[i], target_name,
                   targetTypeTable[target_index].name,
                   pData[i]);
        }

        free(product_name);
    }
    nv_msg(NULL, "");

    XFree(pData);
    return NV_TRUE;

} /* print_target_connections() */



/*
 * query_all_targets() - print a list of all the targets (of the
 * specified type) accessible via the Display connection.
 */

static int query_all_targets(const char *display_name, const int target_index)
{
    CtrlHandles *h;
    CtrlHandleTarget *t;
    ReturnStatus status;
    int i, table_index;
    char *str, *name, *product_name;
    
    /* find the index into targetTypeTable[] for target_index */

    table_index = -1;
    
    for (i = 0; targetTypeTable[i].name; i++) {
        if (targetTypeTable[i].target_index == target_index) {
            table_index = i;
            break;
        }
    }

    if (table_index == -1) return NV_FALSE;

    /* create handles */

    h = nv_alloc_ctrl_handles(display_name);
    
    /* build the standard X server name */
    
    str = nv_standardize_screen_name(XDisplayName(h->display), -2);
    
    /* warn if we don't have any of the target type */
    
    if (h->targets[target_index].n <= 0) {
        
        nv_warning_msg("No %ss on %s",
                       targetTypeTable[table_index].name, str);
        
        free(str);
        nv_free_ctrl_handles(h);
        return NV_FALSE;
    }
    
    /* print how many of the target type we have */
    
    nv_msg(NULL, "%d %s%s on %s",
           h->targets[target_index].n,
           targetTypeTable[table_index].name,
           (h->targets[target_index].n > 1) ? "s" : "",
           str);
    nv_msg(NULL, "");
    
    free(str);

    /* print information per target */

    for (i = 0; i < h->targets[target_index].n; i++) {
        
        t = &h->targets[target_index].t[i];
        
        str = NULL;
        if (target_index == THERMAL_SENSOR_TARGET) {    
            /* for sensor, create the product name */

            product_name = malloc(32);
            snprintf(product_name, 32, "Thermal Sensor %d", i);

        } else if (target_index == COOLER_TARGET) {

            /* for cooler, create the product name */

            product_name = malloc(32);
            snprintf(product_name, 32, "Fan %d", i);

        } else if (target_index == FRAMELOCK_TARGET) {

            /* for framelock, create the product name */

            product_name = malloc(32);
            snprintf(product_name, 32, "G-Sync %d", i);
            
        } else if (target_index == VCS_TARGET) {

            status = NvCtrlGetStringAttribute
                (t->h, NV_CTRL_STRING_VCSC_PRODUCT_NAME, &product_name);
            
            if (status != NvCtrlSuccess) product_name = strdup("Unknown");
            
        } else if (target_index == GVI_TARGET) {
            
            /* for gvi, create the product name */

            product_name = malloc(32);
            snprintf(product_name, 32, "SDI Input %d", i);
        } else {

            /* for X_SCREEN_TARGET or GPU_TARGET, query the product name */

            status = NvCtrlGetStringAttribute
                (t->h, NV_CTRL_STRING_PRODUCT_NAME, &product_name);
            
            if (status != NvCtrlSuccess) product_name = strdup("Unknown");
            
        }
        
        /*
         * use the name for the target handle, or "Unknown" if we
         * don't have a target handle name (this can happen for a
         * non-NVIDIA X screen)
         */

        if (t->name) {
            name = t->name;
        } else {
            name = "Not NVIDIA";
        }

        nv_msg("    ", "[%d] %s (%s)", i, name, product_name);
        nv_msg(NULL, "");
        
        free(product_name);

        /* Print connectivity information */
        if (__verbosity >= VERBOSITY_ALL) {
            if (targetTypeTable[table_index].uses_display_devices) {
                print_target_display_connections(t);
            }

            switch (target_index) {
            case GPU_TARGET:
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_XSCREENS_USING_GPU,
                     X_SCREEN_TARGET);
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_FRAMELOCKS_USED_BY_GPU,
                     FRAMELOCK_TARGET);
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_VCSCS_USED_BY_GPU,
                     VCS_TARGET);
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
                     COOLER_TARGET);
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU,
                     THERMAL_SENSOR_TARGET);
                break;

            case X_SCREEN_TARGET:
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                     GPU_TARGET);
                break;

            case FRAMELOCK_TARGET:
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK,
                     GPU_TARGET);
                break;

            case VCS_TARGET:
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_GPUS_USING_VCSC,
                     GPU_TARGET);
                break;

            case COOLER_TARGET:
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
                     COOLER_TARGET);
                break;
            
            case THERMAL_SENSOR_TARGET:
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU,
                     THERMAL_SENSOR_TARGET);
                break;
            
            default:
                break;
            }
        }
    }
    
    nv_free_ctrl_handles(h);
    
    return NV_TRUE;
    
} /* query_all_targets() */



/*
 * process_parsed_attribute_internal() - this function does the actual
 * attribute processing for nv_process_parsed_attribute().
 *
 * If an error occurs, an error message is printed and NV_FALSE is
 * returned; if successful, NV_TRUE is returned.
 */

static int process_parsed_attribute_internal(CtrlHandleTarget *t,
                                             ParsedAttribute *a, uint32 d,
                                             int target_type, int assign,
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
        ret = validate_value(t, a, d, target_type, whence);
        if (!ret) return NV_FALSE;

        status = NvCtrlSetDisplayAttribute(t->h, d, a->attr, a->val);
        
        if (status != NvCtrlSuccess) {
            nv_error_msg("Error assigning value %d to attribute '%s' "
                         "(%s%s) as specified %s (%s).",
                         a->val, a->name, t->name, str, whence,
                         NvCtrlAttributesStrError(status));
            return NV_FALSE;
        }
        
        if (verbose) {
            if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
                nv_msg("  ", "Attribute '%s' (%s%s) assigned value %d,%d.",
                       a->name, t->name, str,
                       a->val >> 16, a->val & 0xffff);
            } else {
                nv_msg("  ", "Attribute '%s' (%s%s) assigned value %d.",
                       a->name, t->name, str, a->val);
            }
        }

    } else { /* query */
        if (a->flags & NV_PARSER_TYPE_STRING_ATTRIBUTE) {
            char *tmp_str = NULL;
            status = NvCtrlGetStringDisplayAttribute(t->h, d, a->attr, &tmp_str);

            if (status == NvCtrlAttributeNotAvailable) {
                nv_warning_msg("Error querying attribute '%s' specified %s; "
                               "'%s' is not available on %s%s.",
                               a->name, whence, a->name,
                               t->name, str);
            } else if (status != NvCtrlSuccess) {
                nv_error_msg("Error while querying attribute '%s' "
                             "(%s%s) specified %s (%s).",
                             a->name, t->name, str, whence,
                             NvCtrlAttributesStrError(status));
                return NV_FALSE;
            } else {

                if (__terse) {
                    nv_msg(NULL, "%s", tmp_str);
                } else {
                    nv_msg("  ",  "Attribute '%s' (%s%s): %s",
                           a->name, t->name, str, tmp_str);
                }
                free(tmp_str);  
                tmp_str = NULL;
            }
        } else { 
            status = NvCtrlGetDisplayAttribute(t->h, d, a->attr, &a->val);

            if (status == NvCtrlAttributeNotAvailable) {
                nv_warning_msg("Error querying attribute '%s' specified %s; "
                               "'%s' is not available on %s%s.",
                               a->name, whence, a->name,
                               t->name, str);
            } else if (status != NvCtrlSuccess) {
                nv_error_msg("Error while querying attribute '%s' "
                             "(%s%s) specified %s (%s).",
                             a->name, t->name, str, whence,
                             NvCtrlAttributesStrError(status));
                return NV_FALSE;
            } else {
                print_queried_value(t, &valid, a->val, a->flags, a->name, d,
                                    "  ", __terse ?
                                    VerboseLevelTerse : VerboseLevelVerbose);
                print_valid_values(a->name, a->attr, a->flags, valid);
            }
        }
    } /* query */

    return NV_TRUE;

} /* process_parsed_attribute_internal() */



/*
 * nv_process_parsed_attribute() - this is the processing engine for
 * all parsed attributes.
 *
 * A parsed attribute may or may not specify a target (X screen, GPU,
 * framelock device); if a target was specified, we validate that
 * target and process the attribute just for that target.  If a target
 * was not specified, we process the attribute for all valid X
 * screens.
 *
 * A parsed attribute may or may not specify one or more display
 * devices.  For attributes that require that a display device be
 * specified: if a display device mask is specified, we validate it
 * and process the attribute just for the display devices in the mask.
 * If a display device mask was not specified, then we process the
 * attribute for all enabled display devices on each of the targets
 * that have been requested.
 *
 * "Processing" a parsed attribute means either querying for the
 * current value of the attribute on all requested targets and display
 * devices (see above), or assigning the attribute on all requested
 * targets and display devices (see above).
 *
 * The majority of the work (determining which targets, which display
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
 * NvCtrlAttributeHandle's (one for each target on this X server), as
 * well as the number of targets, an array of enabled display devices
 * for each target, and a string description of each target.
 *
 * The whence_fmt and following varargs are used by the callee to
 * describe where the attribute came from.  A whence string should be
 * something like "on line 12 of config file ~/.nvidia-settings-rc" or
 * "in query ':0.0/fsaa'".  Whence is used in the case of an error to
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
    int i, target, start, end, bit, ret, val, target_type_index;
    char *whence, *tmp_d_str0, *tmp_d_str1, *target_type_name;
    uint32 display_devices, mask;
    ReturnStatus status;
    NVCTRLAttributeValidValuesRec valid;
    CtrlHandleTarget *t;

    val = NV_FALSE;

    /* build the whence string */

    NV_VSNPRINTF(whence, whence_fmt);
    
    if (!whence) whence = strdup("\0");

    /* if we don't have a Display connection, abort now */

    if (!h->dpy) {
        nv_error_msg("Unable to %s attribute %s specified %s (no Display "
                     "connection).", assign ? "assign" : "query",
                     a->name, whence);
        goto done;
    }
    
    /*
     * if a target was specified, make sure it is valid, and setup
     * the variables 'start', 'end', and 'target'.
     */
    
    if (a->flags & NV_PARSER_HAS_TARGET) {
        
        /*
         * look up the target index for the target type specified in
         * the ParsedAttribute
         */
        
        target = -1;
        target_type_name = "?";
        
        for (i = 0; targetTypeTable[i].name; i++) {
            if (targetTypeTable[i].nvctrl == a->target_type) {
                target = targetTypeTable[i].target_index;
                target_type_name = targetTypeTable[i].name;
                break;
            }
        }
        
        if (target == -1) {
            nv_error_msg("Invalid target specified %s.", whence);
            goto done; 
        }
        
        /* make sure the target_id is in range */
        
        if (a->target_id >= h->targets[target].n) {
            
            if (h->targets[target].n == 1) {
                nv_error_msg("Invalid %s %d specified %s (there is only "
                             "1 %s on this Display).",
                             target_type_name,
                             a->target_id, whence, target_type_name);
            } else {
                nv_error_msg("Invalid %s %d specified %s "
                             "(there are only %d %ss on this Display).",
                             target_type_name,
                             a->target_id, whence,
                             h->targets[target].n,
                             target_type_name);
            }
            goto done;
        }
        
        /*
         * make sure we have a handle for this target; missing a
         * handle should only happen for X screens because not all X
         * screens will be controlled by NVIDIA
         */

        if (!h->targets[target].t[a->target_id].h) {
            nv_warning_msg("Invalid %s %d specified %s (NV-CONTROL extension "
                         "not supported on %s %d).",
                         target_type_name,
                         a->target_id, whence,
                         target_type_name, a->target_id);
        }
        
        /*
         * assign 'start' and 'end' such that the below loop only uses
         * this target.
         */
        
        start = a->target_id;
        end = a->target_id + 1;
        
    } else {
        
        /*
         * no target was specified; assume a target type of
         * X_SCREEN_TARGET, and assign 'start' and 'end' such that we
         * loop over all the screens; we could potentially store the
         * correct default target type for each attribute and default
         * to that rather than assume X_SCREEN_TARGET.
         */
        
        target = X_SCREEN_TARGET;
        start = 0;
        end = h->targets[target].n;
    }

    /* find the target type index */

    target_type_index = 0;
    
    for (i = 0; targetTypeTable[i].name; i++) {
        if (targetTypeTable[i].target_index == target) {
            target_type_index = i;
            break;
        }
    }
    
    /* loop over the requested targets */
    
    for (i = start; i < end; i++) {
        
        t = &h->targets[target].t[i];

        if (!t->h) continue; /* no handle on this target; silently skip */

        if (a->flags & NV_PARSER_HAS_DISPLAY_DEVICE) {

            /* Expand any wildcards in the display device mask */
        
            display_devices = expand_display_device_mask_wildcards
                (a->display_device_mask, t->d);
            
            if ((display_devices == 0) || (display_devices & ~t->d)) {

                /*
                 * use a->display_device_mask rather than
                 * display_devices when building the string (so that
                 * display_device_mask_to_display_device_name() can
                 * use wildcards if present).
                 */

                tmp_d_str0 = display_device_mask_to_display_device_name
                    (a->display_device_mask);
                tmp_d_str1 = display_device_mask_to_display_device_name(t->d);

                if (tmp_d_str1 && (*tmp_d_str1 != '\0')) {
                    nv_error_msg("Invalid display device %s specified "
                                 "%s (the currently enabled display devices "
                                 "are %s on %s).",
                                 tmp_d_str0, whence, tmp_d_str1, t->name);
                } else {
                    nv_error_msg("Invalid display device %s specified "
                                 "%s (there are currently no enabled display "
                                 "devices on %s).",
                                 tmp_d_str0, whence, t->name);
                }

                free(tmp_d_str0);
                free(tmp_d_str1);
                
                goto done;
            }
            
        } else {
            
            display_devices = t->d;
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

            status = NvCtrlSetColorAttributes(t->h, v, v, v, a->attr);

            if (status != NvCtrlSuccess) {
                nv_error_msg("Error assigning %f to attribute '%s' on %s "
                             "specified %s (%s)", a->fval, a->name,
                             t->name, whence,
                             NvCtrlAttributesStrError(status));
                goto done;
            }

            continue;
        }

        /*
         * If we are assigning, and the value for this attribute is a
         * display device, then we need to validate the value against
         * either the mask of enabled display devices or the mask of
         * connected display devices.
         */
        
        if (assign && (a->flags & NV_PARSER_TYPE_VALUE_IS_DISPLAY)) {
            char *display_device_descriptor = NULL;
            uint32 check_mask;

            /* use the complete mask if requested */

            if (a->flags & NV_PARSER_TYPE_ASSIGN_ALL_DISPLAYS) {
                if (a->flags & NV_PARSER_TYPE_VALUE_IS_SWITCH_DISPLAY) {
                    a->val = t->c;
                } else {
                    a->val = t->d;
                }
            }

            /*
             * if we are hotkey switching, check against all connected
             * displays; otherwise, check agains the currently active
             * display devices
             */

            if (a->flags & NV_PARSER_TYPE_VALUE_IS_SWITCH_DISPLAY) {
                check_mask = t->c;
                display_device_descriptor = "connected";
            } else {
                check_mask = t->d;
                display_device_descriptor = "enabled";
            }

            if ((check_mask & a->val) != a->val) {

                tmp_d_str0 =
                    display_device_mask_to_display_device_name(a->val);

                tmp_d_str1 =
                    display_device_mask_to_display_device_name(check_mask);

                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "assigned the value of %s (the currently %s "
                             "display devices are %s on %s).",
                             a->name, whence, tmp_d_str0,
                             display_device_descriptor,
                             tmp_d_str1, t->name);
                
                free(tmp_d_str0);
                free(tmp_d_str1);
                
                continue;
            }
        }

        /*
         * If we are assigning, and the value for this attribute is
         * not allowed to be zero, check that the value is not zero.
         */

        if (assign && (a->flags & NV_PARSER_TYPE_NO_ZERO_VALUE)) {
            
            /* value must be non-zero */

            if (!a->val) {
                
                if (a->flags & NV_PARSER_TYPE_VALUE_IS_DISPLAY) {
                    tmp_d_str0 = "display device";
                } else {
                    tmp_d_str0 = "value";
                }

                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "assigned the value of 0 (a valid, non-zero, "
                             "%s must be specified).",
                             a->name, whence, tmp_d_str0);
                continue;
            }
        }
        
        /*
         * If we are dealing with a frame lock attribute on a non-frame lock
         * target type, make sure frame lock is available.
         *
         * Also, when setting frame lock attributes on non-frame lock targets,
         * make sure frame lock is disabled.  (Of course, don't check this for
         * the "enable frame lock" attribute, and special case the "Test
         * Signal" attribute.)
         */

        if ((a->flags & NV_PARSER_TYPE_FRAMELOCK) &&
            (NvCtrlGetTargetType(t->h) != NV_CTRL_TARGET_TYPE_FRAMELOCK)) {
            int available;
            
            status = NvCtrlGetAttribute(t->h, NV_CTRL_FRAMELOCK, &available);
            if (status != NvCtrlSuccess) {
                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "%s; error querying frame lock availablity on "
                             "%s (%s).",
                             a->name, whence, assign ? "assigned" : "queried",
                             t->name, NvCtrlAttributesStrError(status));
                continue;
            }
                
            if (available != NV_CTRL_FRAMELOCK_SUPPORTED) {
                nv_error_msg("The attribute '%s' specified %s cannot be %s; "
                             "frame lock is not supported/available on %s.",
                             a->name, whence, assign ? "assigned" : "queried",
                             t->name);
                continue;
            }

            /* Do assignments based on the frame lock sync status */

            if (assign && (a->attr != NV_CTRL_FRAMELOCK_SYNC)) {
                int enabled;

                status = NvCtrlGetAttribute(t->h, NV_CTRL_FRAMELOCK_SYNC,
                                            &enabled);
                if (status != NvCtrlSuccess) {
                    nv_error_msg("The attribute '%s' specified %s cannot be "
                                 "assigned; error querying frame lock sync "
                                 "status on %s (%s).",
                                 a->name, whence, t->name,
                                 NvCtrlAttributesStrError(status));
                    continue;
                }

                if (a->attr == NV_CTRL_FRAMELOCK_TEST_SIGNAL) {
                    if (enabled != NV_CTRL_FRAMELOCK_SYNC_ENABLE) {
                        nv_error_msg("The attribute '%s' specified %s cannot "
                                     "be assigned; frame lock sync is "
                                     "currently disabled on %s.",
                                     a->name, whence, t->name);
                        continue;
                    }
                } else if (enabled != NV_CTRL_FRAMELOCK_SYNC_DISABLE) {
                    nv_warning_msg("The attribute '%s' specified %s cannot be "
                                   "assigned; frame lock sync is currently "
                                   "enabled on %s.",
                                   a->name, whence, t->name);
                    continue;
                }
            }
        }

        /*
         * To properly handle SDI (GVO) attributes, we just need to make
         * sure that GVO is supported by the handle.
         */

        if (a->flags & NV_PARSER_TYPE_SDI &&
            target != NV_CTRL_TARGET_TYPE_GVI) {
            int available;

            status = NvCtrlGetAttribute(t->h, NV_CTRL_GVO_SUPPORTED,
                                        &available);
            if (status != NvCtrlSuccess) {
                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "%s; error querying SDI availablity on "
                             "%s (%s).",
                             a->name, whence, assign ? "assigned" : "queried",
                             t->name, NvCtrlAttributesStrError(status));
                continue;
            }

            if (available != NV_CTRL_GVO_SUPPORTED_TRUE) {
                nv_error_msg("The attribute '%s' specified %s cannot be %s; "
                             "SDI is not supported/available on %s.",
                             a->name, whence, assign ? "assigned" : "queried",
                             t->name);
                continue;
            }
        }

        /* loop over the display devices */

        for (bit = 0; bit < 24; bit++) {
            
            mask = (1 << bit);

            if (((mask & display_devices) == 0x0) &&
                (targetTypeTable[target_type_index].uses_display_devices) &&
                (t->d)) {
                continue;
            }

            if (a->flags & NV_PARSER_TYPE_STRING_ATTRIBUTE) {
                status = NvCtrlGetValidStringDisplayAttributeValues(t->h, mask, a->attr, &valid);
            } else {
                status = NvCtrlGetValidDisplayAttributeValues(t->h, mask, a->attr, &valid);
            }

            if (status != NvCtrlSuccess) {
                if (status == NvCtrlAttributeNotAvailable) {
                    nv_warning_msg("Attribute '%s' specified %s is not "
                                   "available on %s.",
                                   a->name, whence, t->name);
                } else {
                    nv_error_msg("Error querying valid values for attribute "
                                 "'%s' on %s specified %s (%s).",
                                 a->name, t->name, whence,
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
            
            ret = process_parsed_attribute_internal(t, a, mask, target, assign,
                                                    verbose, whence, valid);
            if (ret == NV_FALSE) goto done;
            
            /*
             * if this attribute is not per-display device, or this
             * target does not know about display devices, or this target
             * does not have display devices, then once through this loop
             * is enough.
             */
            
            if ((!(valid.permissions & ATTRIBUTE_TYPE_DISPLAY)) ||
                (!(targetTypeTable[target_type_index].uses_display_devices)) ||
                (!(t->d))) {
                break;
            }

        } /* bit */
        
    } /* i - done looping over requested targets */
    
    val = NV_TRUE;
    
 done:
    if (whence) free(whence);
    return val;

} /* nv_process_parsed_attribute() */
