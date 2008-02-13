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
 * parse.h - prototypes and definitions for the parser.
 */

#ifndef __PARSE_H__
#define __PARSE_H__


/*
 * Flag values used in the flags field of the ParsedAttribute struct.
 */

#define NV_PARSER_HAS_X_DISPLAY         (1<<0)
#define NV_PARSER_HAS_X_SCREEN          (1<<2)
#define NV_PARSER_HAS_DISPLAY_DEVICE    (1<<3)
#define NV_PARSER_HAS_VAL               (1<<4)

/*
 * Flag values used in the flags field of the AttributeTableEntry struct.
 */

#define NV_PARSER_TYPE_FRAMELOCK        (1<<16)
#define NV_PARSER_TYPE_COLOR_ATTRIBUTE  (1<<17)
#define NV_PARSER_TYPE_NO_CONFIG_WRITE  (1<<18)
#define NV_PARSER_TYPE_GUI_ATTRIUBUTE   (1<<19)
#define NV_PARSER_TYPE_XVIDEO_ATTRIBUTE (1<<20)

#define NV_PARSER_ASSIGNMENT 0
#define NV_PARSER_QUERY 1

#define NV_PARSER_MAX_NAME_LEN 256

#define DISPLAY_NAME_SEPARATOR '/'


/*
 * error codes returned by nv_parse_attribute_string().
 */

#define NV_PARSER_STATUS_SUCCESS                0
#define NV_PARSER_STATUS_BAD_ARGUMENT           1
#define NV_PARSER_STATUS_EMPTY_STRING           2
#define NV_PARSER_STATUS_ATTR_NAME_TOO_LONG     3
#define NV_PARSER_STATUS_ATTR_NAME_MISSING      4
#define NV_PARSER_STATUS_BAD_DISPLAY_DEVICE     5
#define NV_PARSER_STATUS_MISSING_EQUAL_SIGN     6
#define NV_PARSER_STATUS_NO_VALUE               7
#define NV_PARSER_STATUS_TRAILING_GARBAGE       8
#define NV_PARSER_STATUS_UNKNOWN_ATTR_NAME      9

/*
 * define useful types
 */

typedef unsigned int uint32;


/*
 * The valid attribute names, and their corresponding protocol
 * attribute identifiers are stored in an array of
 * AttributeTableEntries.
 */

typedef struct _AttributeTableEntry {
    char *name;
    int attr;
    uint32 flags;
} AttributeTableEntry;


/*
 * ParsedAttribute - struct filled out by
 * nv_ParseAttributeString().
 */

typedef struct _ParsedAttribute {
    char *display;
    char *name;
    int screen;
    int attr;
    int val;
    float fval; /* XXX put in a union with val? */
    uint32 display_device_mask;
    uint32 flags;
    struct _ParsedAttribute *next;
} ParsedAttribute;



/*
 * Attribute table; defined in parse.c
 */

extern AttributeTableEntry attributeTable[];

/*
 * nv_parse_attribute_string() - this function parses an attribute
 * string, the syntax for which is:
 *
 * {host}:{display}.{screen}/{attribute name}[{display devices}]={value}
 *
 * {host}:{display}.{screen} indicates which X server and X screen to
 * interact with; this is optional.  If no X server is specified, then
 * the default X server is used.  If no X screen is specified, then
 * all X screens on the X server are used.
 *
 * {screen}/ may be specified by itself (ie: without the
 * "{host}:{display}." part).
 *
 * {attribute name} should be a string without any whitespace (a case
 * insensitive compare will be done to find a match in the
 * attributeTable in parse.c).  {value} should be an integer.
 *
 * {display devices} is optional.  If no display mask is specified,
 * then all display devices are assumed.
 *
 * The query parameter controls whether the attribute string is parsed
 * for setting or querying.  If query == NV_PARSER_SET, then the
 * attribute string will be interpretted as described above.  If query
 * == NV_PARSER_QUERY, the "={value}" portion of the string should be
 * omitted.
 *
 * If successful, NV_PARSER_STATUS_SUCCESS will be returned and the
 * ParsedAttribute struct will contain the attribute id corresponding
 * to the attribute name.  If the X server or display devices were
 * given in the string, then those fields will be appropriately
 * assigned in the ParsedAttribute struct, and the
 * NV_PARSER_HAS_X_SERVER and NV_PARSER_HAS_DISPLAY_DEVICE_MASK bits
 * will be set in the flags field.
 */

int nv_parse_attribute_string(const char *, int, ParsedAttribute *);


/*
 * nv_assign_default_display() - assigns the display name to the
 * ParsedAttribute struct.  As a side affect, also assigns the screen
 * field, if a screen is specified in the display name.
 */

void nv_assign_default_display(ParsedAttribute *a, const char *display);


/*
 * nv_parse_strerror() - given the error status returned by
 * nv_parse_attribute_string(), this function returns a string
 * describing the error.
 */

char *nv_parse_strerror(int);

int nv_strcasecmp(const char *, const char *);


char *remove_spaces(const char *o);

/*
 * diaplay_mask/display_name conversions: the NV-CONTROL X extension
 * identifies a display device by a bit in a display device mask.  The
 * below functions translate between a display mask, and a string
 * describing the display devices.
 */

#define BITSHIFT_CRT  0
#define BITSHIFT_TV   8
#define BITSHIFT_DFP 16

#define BITMASK_ALL_CRT (0xff << BITSHIFT_CRT)
#define BITMASK_ALL_TV  (0xff << BITSHIFT_TV)
#define BITMASK_ALL_DFP (0xff << BITSHIFT_DFP)

#define INVALID_DISPLAY_DEVICE_MASK   (0xffffffff)

#define VALID_DISPLAY_DEVICES_MASK    (0x00ffffff)
#define DISPLAY_DEVICES_WILDCARD_MASK (0xff000000)

#define DISPLAY_DEVICES_WILDCARD_CRT  (1 << 24)
#define DISPLAY_DEVICES_WILDCARD_TV   (1 << 25)
#define DISPLAY_DEVICES_WILDCARD_DFP  (1 << 26)



char *display_device_mask_to_display_device_name(const uint32);
uint32 display_device_name_to_display_device_mask(const char *);

uint32 expand_display_device_mask_wildcards(const uint32, const uint32);

ParsedAttribute *nv_parsed_attribute_init(void);

void nv_parsed_attribute_add(ParsedAttribute *head, ParsedAttribute *a);

void nv_parsed_attribute_free(ParsedAttribute *p);
void nv_parsed_attribute_clean(ParsedAttribute *p);

char *nv_get_attribute_name(const int attr);

char *nv_standardize_screen_name(const char *display_name, int screen);

#endif /* __PARSE_H__ */
