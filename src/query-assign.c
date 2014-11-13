/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

/*
 * query-assign.c - this source file contains functions for querying
 * and assigning attributes, as specified on the command line.  Some
 * of this functionality is also shared with the config file
 * reader/writer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>

#include <X11/Xlib.h>
#include "NVCtrlLib.h"

#include "parse.h"
#include "msg.h"
#include "query-assign.h"
#include "common-utils.h"

/* local prototypes */

#define PRODUCT_NAME_LEN 64

static int process_attribute_queries(const Options *,
                                     int, char**, const char *,
                                     CtrlSystemList *);

static int process_attribute_assignments(const Options *,
                                         int, char**, const char *,
                                         CtrlSystemList *);

static int query_all(const Options *, const char *, CtrlSystemList *);
static int query_all_targets(const char *display_name, const int target_type,
                             CtrlSystemList *);

static void print_valid_values(const Options *op, const AttributeTableEntry *a,
                               NVCTRLAttributeValidValuesRec valid);

static void print_additional_info(const char *name,
                                  int attr,
                                  NVCTRLAttributeValidValuesRec valid,
                                  const char *indent);

static ReturnStatus get_framelock_sync_state(CtrlTarget *target,
                                             int *enabled);

/*
 * nv_process_assignments_and_queries() - process any assignments or
 * queries specified on the commandline.  If an error occurs, return
 * NV_FALSE.  On success return NV_TRUE.
 */

int nv_process_assignments_and_queries(const Options *op,
                                       CtrlSystemList *systems)
{
    int ret;

    if (op->num_queries) {
        ret = process_attribute_queries(op,
                                        op->num_queries,
                                        op->queries, op->ctrl_display,
                                        systems);
        if (!ret) return NV_FALSE;
    }

    if (op->num_assignments) {
        ret = process_attribute_assignments(op,
                                            op->num_assignments,
                                            op->assignments,
                                            op->ctrl_display,
                                            systems);
        if (!ret) return NV_FALSE;
    }
    
    return NV_TRUE;

} /* nv_process_assignments_and_queries() */



/*!
 * Determines if the target 't' has the name 'name'.
 *
 * \param[in]  t     The target being considered.
 * \param[in]  name  The name to match against.
 *
 * \return  Returns NV_TRUE if the given target 't' has the name 'name'; else
 *          returns NV_FALSE.
 */

static int nv_target_has_name(const CtrlTarget *t, const char *name)
{
    int n;

    for (n = 0; n < NV_PROTO_NAME_MAX; n++) {
        if (t->protoNames[n] &&
            nv_strcasecmp(t->protoNames[n], name)) {
            return NV_TRUE;
        }
    }

    return NV_FALSE;
}



/*!
 * Determines if the target 't' matches a given target type, target id, and/or
 * target name.  If a match criteria is invalid, it is not matched against.
 *
 * \param[in]  t                    The target being considered.
 * \param[in]  matchTargetTypeInfo  The target type to match to
 * \param[in]  matchTargetId        The target id to match to
 * \param[in]  matchTargetName      The target name to match to
 *
 * \return  Returns NV_TRUE if the given target 't' matches the given
 *          qualifiers; else returns NV_FALSE.
 */

static int target_has_qualification(const CtrlTarget *t,
                                    const CtrlTargetTypeInfo *matchTargetTypeInfo,
                                    int matchTargetId,
                                    const char *matchTargetName)
{
    const CtrlTargetNode *n;

    /* If no qualifications given, all targets match */
    if (!matchTargetTypeInfo &&
        (matchTargetId < 0) &&
        !matchTargetName) {
        return NV_TRUE;
    }

    /* Look for any matching relationship */
    for (n = t->relations; n; n = n->next) {
        const CtrlTarget *r = n->t;

        if (matchTargetTypeInfo &&
            (matchTargetTypeInfo != r->targetTypeInfo)) {
            continue;
        }

        if ((matchTargetId >= 0) &&
            matchTargetId != NvCtrlGetTargetId(r)) {
            continue;
        }

        if (matchTargetName &&
            !nv_target_has_name(r, matchTargetName)) {
            continue;
        }

        return NV_TRUE;
    }

    return NV_FALSE;
}



/*!
 * Resolves the two given strings sAAA and sBBB into a target type, target id,
 * and/or target name.  The following target specifications are supported:
 *
 * "AAA" (BBB = NULL)
 *    - All targets of type AAA, if AAA names a target type, or
 *    - All targets named AAA if AAA does not name a target type.
 *
 * "BBB:AAA"
 *    - All targets of type BBB with either target id AAA if AAA is a numerical
 *      value, or target(s) named AAA otherwise.
 *
 * \param[in]  sAAA             If sBBB is NULL, this is either the target name,
 *                              or a target type.
 * \param[in]  sBBB             If not NULL, this is the target type as a
 *                              string.
 * \param[out] pTargetTypeInfo  Assigned the target type, or NULL for all target
 *                              types.
 * \param[out] pTargetId        Assigned the target id, or -1 for all targets.
 * \param[out] pTargetName      Assigned the target name, or NULL for all target
 *                              names.
 *
 * \return  Returns NV_PARSER_STATUS_SUCCESS if sAAA and sBBB were successfully
 *          parsed into at target specification; else, returns
 *          NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET if a parsing failure
 *          occurred.
 */

static int parse_single_target_specification(const char *sAAA,
                                             const char *sBBB,
                                             const CtrlTargetTypeInfo **pTargetTypeInfo,
                                             int *pTargetId,
                                             const char **pTargetName)
{
    const CtrlTargetTypeInfo *targetTypeInfo;

    *pTargetTypeInfo = NULL; // Match all target types
    *pTargetId = -1;         // Match all target ids
    *pTargetName = NULL;     // Match all target names

    if (sBBB) {

        /* If BBB is specified, then it must name a target type */
        targetTypeInfo = NvCtrlGetTargetTypeInfoByName(sBBB);
        if (!targetTypeInfo) {
            return NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET;
        }
        *pTargetTypeInfo = targetTypeInfo;

        /* AAA can either be a name, or a target id */
        if (!nv_parse_numerical(sAAA, NULL, pTargetId)) {
            *pTargetName = sAAA;
        }

    } else {

        /* If BBB is unspecified, then AAA could possibly name a target type */
        targetTypeInfo = NvCtrlGetTargetTypeInfoByName(sAAA);
        if (targetTypeInfo) {
            *pTargetTypeInfo = targetTypeInfo;
        } else {
            /* If not, then AAA is a target name */
            *pTargetName = sAAA;
        }
    }

    return NV_PARSER_STATUS_SUCCESS;
}




/*!
 * Computes the list of targets from the given CtrlSystem that match the
 * ParsedAttribute's target specification string.
 *
 * The following target specifications string formats are supported:
 *
 * "AAA"
 *    - All targets of type AAA, if AAA names a target type, or
 *    - All targets named AAA if AAA does not name a target type.
 *
 * "BBB:AAA"
 *    - All targets of type BBB with either target id AAA if AAA is a numerical
 *      value, or target(s) named AAA otherwise.
 *
 * "CCC.AAA"
 *    - All target types (and/or names) AAA that have a relation to any target
 *      types (and/or names) CCC.
 *
 * "CCC.BBB:AAA"
 *    - All the targets named (or target id) AAA of the target type BBB that are
 *      related to target type (and or name) CCC.
 *
 * "DDD:CCC.AAA"
 *    - All target types (and/or names) AAA that are related to targets named
 *      (or target id) CCC of the target type DDD.
 *
 * "DDD:CCC.BBB:AAA"
 *    - All the targets named (or target id) AAA of the target type BBB that are
 *      related to targets named (or target id) CCC of the target type DDD.
 *
 * \param[in/out]  p       The ParsedAttribute whose target specification string
 *                         should be analyzed and converted into a list of
 *                         targets.
 * \param[in]      system  The list of targets to choose from.
 *
 * \return  Returns NV_PARSER_STATUS_SUCCESS if the ParsedAttribute's target
 *          specification string was successfully parsed into a list of targets
 *          (though this list could be empty, if no targets are found to
 *          match!); else, returns one of the other NV_PARSER_STATUS_XXX
 *          error codes that detail the particular parsing error.
 */

static int nv_infer_targets_from_specification(ParsedAttribute *p,
                                               CtrlSystem *system)
{
    int ret = NV_PARSER_STATUS_SUCCESS;

    /* specification as: "XXX.YYY" or "XXX" */
    char *sXXX = NULL; // Target1 string
    char *sYYY = NULL; // Target2 string

    /* XXX as: "BBB:AAA" or "AAA" */
    char *sAAA = NULL; // Target1 name
    char *sBBB = NULL; // Target1 type name

    /* YYY as: "DDD:CCC" or "CCC" */
    char *sCCC = NULL; // Target2 name
    char *sDDD = NULL; // Target2type name

    char *s;
    char *specification;

    int target_type;

    const CtrlTargetTypeInfo *matchTargetTypeInfo;
    int matchTargetId;
    const char *matchTargetName;

    const CtrlTargetTypeInfo *matchQualifierTargetTypeInfo;
    int matchQualifierTargetId;
    const char *matchQualifierTargetName;


    specification = nvstrdup(p->target_specification);

    /* Parse for 'YYY.XXX' or 'XXX' */
    s = strchr(specification, '.');

    /* Skip over periods that are followed by a numerical value since
     * target names and target types cannot start with these, and thus
     * these are part of the name.
     */
    while (s) {
        if (!isdigit(*(s+1))) {
            break;
        }
        s = strchr(s+1, '.');
    }

    if (s) {
        *s = '\0';
        sXXX = s+1;
        sYYY = specification;
    } else {
        sXXX = specification;
    }

    /* Parse for 'BBB:AAA' or 'AAA' (in XXX above) */
    s = strchr(sXXX, ':');
    if (s) {
        *s = '\0';
        sBBB = sXXX;
        sAAA = s+1;
    } else {
        /* AAA is either a target type name, or a target name */
        sAAA = sXXX;
    }

    /* Parse for 'DDD:CCC' or 'CCC' (in YYY above) */
    if (sYYY) {
        s = strchr(sYYY, ':');
        if (s) {
            *s = '\0';
            sDDD = sYYY;
            sCCC = s+1;
        } else {
            /* CCC is either a target type name, or a target name */
            sCCC = sYYY;
        }
    }

    /* Get target matching criteria */

    ret = parse_single_target_specification(sAAA, sBBB,
                                            &matchTargetTypeInfo,
                                            &matchTargetId,
                                            &matchTargetName);
    if (ret != NV_PARSER_STATUS_SUCCESS) {
        goto done;
    }

    /* Get target qualifier matching criteria */

    ret = parse_single_target_specification(sCCC, sDDD,
                                            &matchQualifierTargetTypeInfo,
                                            &matchQualifierTargetId,
                                            &matchQualifierTargetName);
    if (ret != NV_PARSER_STATUS_SUCCESS) {
        goto done;
    }

    /* Iterate over the target types */
    for (target_type = 0;
         target_type < MAX_TARGET_TYPES;
         target_type++) {
        const CtrlTargetTypeInfo *targetTypeInfo =
            NvCtrlGetTargetTypeInfo(target_type);
        CtrlTargetNode *node;

        if (matchTargetTypeInfo &&
            (matchTargetTypeInfo != targetTypeInfo)) {
            continue;
        }

        /* For each target of this type, match the id and/or name */
        for (node = system->targets[target_type]; node; node = node->next) {
            CtrlTarget *t = node->t;

            if ((matchTargetId >= 0) &&
                matchTargetId != NvCtrlGetTargetId(t)) {
                continue;
            }
            if (matchTargetName &&
                !nv_target_has_name(t, matchTargetName)) {
                continue;
            }

            if (!target_has_qualification(t,
                                          matchQualifierTargetTypeInfo,
                                          matchQualifierTargetId,
                                          matchQualifierTargetName)) {
                continue;
            }

            /* Target matches, add it to the list */
            NvCtrlTargetListAdd(&(p->targets), t, TRUE);
            p->parser_flags.has_target = NV_TRUE;
        }
    }

 done:
    free(specification);
    return ret;
}



/*!
 * Adds all the targets of the specified target type to the list of targets to
 * process for the ParsedAttribute.
 *
 * \param[in/out]  p             The ParsedAttribute to add targets to.
 * \param[in]      system        The CtrlSystem who's targets to include.
 * \param[in]      target_type   The target type of the targets to add.
 */

static void include_target_type_targets(ParsedAttribute *p,
                                         const CtrlSystem *system,
                                         int target_type)
{
    CtrlTargetNode *node;

    for (node = system->targets[target_type]; node; node = node->next) {
        CtrlTarget *target = node->t;

        NvCtrlTargetListAdd(&(p->targets), target, TRUE);
        p->parser_flags.has_target = NV_TRUE;
    }
}



/*!
 * Converts the ParsedAttribute 'a''s target list to include the display targets
 * (related to the intial non-display targets in the list) as specified by the
 * display mask string (or all related displays if the mask is not specified.)
 *
 * \param[in/out]  p  ParsedAttribute to resolve.
 */

static void resolve_display_mask_string(ParsedAttribute *p, const char *whence)
{
    CtrlTargetNode *head = NULL;
    CtrlTargetNode *n;

    int bit, i;

    char **name_toks = NULL;
    int num_names = 0;


    /* Resolve the display device specification into a list of display names */
    if (p->parser_flags.has_display_device) {

        if (p->display_device_specification) {

            /* Split up the list of names */
            name_toks = nv_strtok(p->display_device_specification, ',',
                                  &num_names);

        } else if (p->display_device_mask) {

            /* Warn that this usage is deprecated */

            nv_deprecated_msg("Display mask usage as specified %s has been "
                              "deprecated and will be removed in the future.  "
                              "Please use display names and/or display target "
                              "specification instead.",
                              whence);

            /* Convert the bitmask into a list of names */

            num_names = count_number_of_bits(p->display_device_mask);
            name_toks = nvalloc(num_names * sizeof(char *));

            i = 0;
            for (bit = 0; bit < 24; bit++) {
                uint32 mask = (1 << bit);

                if (!(mask & p->display_device_mask)) {
                    continue;
                }

                name_toks[i++] =
                    display_device_mask_to_display_device_name(mask);
                if (i >= num_names) {
                    break;
                }
            }
        }
    }

    /* Look at attribute's target list... */
    for (n = p->targets; n; n = n->next) {
        CtrlTarget *t = n->t;
        CtrlTargetNode *n_other;

        if (!t->h) {
            continue;
        }

        /* Include display targets that were previously resolved */
        if (NvCtrlGetTargetType(t) == DISPLAY_TARGET) {
            NvCtrlTargetListAdd(&head, t, TRUE);
            continue;
        }

        /* Include non-display target's related display targets, if any */
        for (n_other = t->relations; n_other;  n_other = n_other->next) {
            CtrlTarget *t_other = n_other->t;

            if (!t_other->h) {
                continue;
            }
            if (NvCtrlGetTargetType(t_other) != DISPLAY_TARGET) {
                continue;
            }

            /* Include all displays if no specification was given */
            if (!p->parser_flags.has_display_device) {
                NvCtrlTargetListAdd(&head, t_other, TRUE);
                continue;
            }

            for (i = 0; i < num_names; i++) {
                if (nv_target_has_name(t_other, name_toks[i])) {
                    NvCtrlTargetListAdd(&head, t_other, TRUE);
                    break;
                }
            }
        }
    }

    /* Cleanup */
    if (name_toks) {
        nv_free_strtoks(name_toks, num_names);
    }

    /* Apply the new targets list */
    NvCtrlTargetListFree(p->targets);
    p->targets = head;
}



/*!
 * Converts the ParsedAttribute 'a''s target specification (and/or target type
 * + id) into a list of CtrlTarget to operate on.  If the ParsedAttribute has a
 * target specification set, this is used to generate the list; Otherwise, the
 * target type and target id are used.  If nothing is specified, all the valid
 * targets for the attribute are included.
 *
 * If this is a display attribute, and a (legacy) display mask string is given,
 * all non-display targets are further resolved into the corresponding display
 * targets that match the names represented by the display mask string.
 *
 * \param[in/out]  p       ParsedAttribute to resolve.
 * \param[in]      system  CtrlSystem to resolve the target specification
 *                         against.
 *
 * \return  Return NV_PARSER_STATUS_SUCCESS if the attribute's target
 *          specification was successfully parsed into a list of targets to
 *          operate on; else, returns one of the other NV_PARSER_STATUS_XXX
 *          error codes that detail the particular parsing error.
 */

static int resolve_attribute_targets(ParsedAttribute *p, CtrlSystem *system,
                                     const char *whence)
{
    CtrlAttributePerms perms;
    CtrlTarget *ctrl_target;
    ReturnStatus status;
    int ret = NV_PARSER_STATUS_SUCCESS;
    int i;

    const AttributeTableEntry *a = p->attr_entry;

    if (p->targets) {
        // Oops already parsed?
        // XXX thrown another error here?
        return NV_PARSER_STATUS_BAD_ARGUMENT;
    }

    ctrl_target = NvCtrlGetDefaultTarget(system);
    if (ctrl_target == NULL) {
        return NV_PARSER_STATUS_TARGET_SPEC_NO_TARGETS;
    }

    status = NvCtrlGetAttributePerms(ctrl_target, a->type, a->attr, &perms);
    if (status != NvCtrlSuccess) {
        // XXX Throw other error here...?
        return NV_PARSER_STATUS_TARGET_SPEC_NO_TARGETS;
    }


    p->targets = NULL;


    /* If a target specification string was given, use that to determine the
     * list of targets to include.
     */
    if (p->target_specification) {
        ret = nv_infer_targets_from_specification(p, system);
        goto done;
    }


    /* If the target type and target id was given, use that. */
    if (NvCtrlIsTargetTypeValid(p->target_type) && p->target_id >= 0) {
        CtrlTarget *target = NvCtrlGetTarget(system, p->target_type,
                                             p->target_id);

        if (!target) {
            return NV_PARSER_STATUS_TARGET_SPEC_NO_TARGETS;
        }

        NvCtrlTargetListAdd(&(p->targets), target, TRUE);
        p->parser_flags.has_target = NV_TRUE;
        goto done;
    }


    /* If a target type was given, but no target id, process all the targets
     * of that type.
     */
    if (NvCtrlIsTargetTypeValid(p->target_type)) {
        include_target_type_targets(p, system, p->target_type);
        goto done;
    }


    /* If no target type was given, assume all the appropriate targets for the
     * attribute based on its permissions.
     */
    for (i = 0; i < MAX_TARGET_TYPES; i++) {
        if (!(perms.valid_targets & CTRL_TARGET_PERM_BIT(i))) {
            continue;
        }

        /* Add all targets of type that are valid for this attribute */
        include_target_type_targets(p, system, i);
    }

 done:

    /* If this attribute is a display attribute, include the related display
     * targets of the (resolved) non display targets.  If a display mask is
     * specified via either name string or value, use that to limit the
     * displays added, otherwise include all related display targets.
     */
    if (!(a->flags.hijack_display_device) &&
        (perms.valid_targets & CTRL_TARGET_PERM_BIT(DISPLAY_TARGET))) {
        resolve_display_mask_string(p, whence);
    }

    /* Make sure at least one target was resolved */
    if (ret == NV_PARSER_STATUS_SUCCESS) {
        if (!(p->parser_flags.has_target)) {
            return NV_PARSER_STATUS_TARGET_SPEC_NO_TARGETS;
        }
    }

    return ret;
}



/*
 * process_attribute_queries() - parse the list of queries, and call
 * nv_ctrl_process_parsed_attribute() to process each query.
 *
 * If any errors are encountered, an error message is printed and
 * NV_FALSE is returned.  Otherwise, NV_TRUE is returned.
 */

static int process_attribute_queries(const Options *op,
                                     int num, char **queries,
                                     const char *display_name,
                                     CtrlSystemList *systems)
{
    int query, ret, val;
    ParsedAttribute a;
    CtrlSystem *system;

    val = NV_FALSE;
    
    /* print a newline before we begin */

    if (!op->terse) {
        nv_msg(NULL, "");
    }

    /* loop over each requested query */

    for (query = 0; query < num; query++) {
        
        /* special case the "all" query */

        if (nv_strcasecmp(queries[query], "all")) {
            query_all(op, display_name, systems);
            continue;
        }

        /* special case the target type queries */
        
        if (nv_strcasecmp(queries[query], "screens") ||
            nv_strcasecmp(queries[query], "xscreens")) {
            query_all_targets(display_name, X_SCREEN_TARGET, systems);
            continue;
        }
        
        if (nv_strcasecmp(queries[query], "gpus")) {
            query_all_targets(display_name, GPU_TARGET, systems);
            continue;
        }

        if (nv_strcasecmp(queries[query], "framelocks")) {
            query_all_targets(display_name, FRAMELOCK_TARGET, systems);
            continue;
        }

        if (nv_strcasecmp(queries[query], "vcs")) {
            query_all_targets(display_name, VCS_TARGET, systems);
            continue;
        }

        if (nv_strcasecmp(queries[query], "gvis")) {
            query_all_targets(display_name, GVI_TARGET, systems);
            continue;
        }

        if (nv_strcasecmp(queries[query], "fans")) {
            query_all_targets(display_name, COOLER_TARGET, systems);
            continue;
        }

        if (nv_strcasecmp(queries[query], "thermalsensors")) {
            query_all_targets(display_name, THERMAL_SENSOR_TARGET, systems);
            continue;
        }

        if (nv_strcasecmp(queries[query], "svps")) {
            query_all_targets(display_name, 
                              NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET, 
                              systems);
            continue;
        }

        if (nv_strcasecmp(queries[query], "dpys")) {
            query_all_targets(display_name, DISPLAY_TARGET, systems);
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

        /* connect to all the systems */

        system = NvCtrlConnectToSystem(a.display, systems);
        if (!system) {
            goto done;
        }

        /* call the processing engine to process the parsed query */

        ret = nv_process_parsed_attribute(op, &a, system, NV_FALSE, NV_FALSE,
                                          "in query '%s'", queries[query]);
        if (ret == NV_FALSE) goto done;

        /* print a newline at the end */
        if (!op->terse) {
            nv_msg(NULL, "");
        }

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
 */

static int process_attribute_assignments(const Options *op,
                                         int num, char **assignments,
                                         const char *display_name,
                                         CtrlSystemList *systems)
{
    int assignment, ret, val;
    ParsedAttribute a;
    CtrlSystem *system;

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

        /* allocate the CtrlSystem */

        system = NvCtrlConnectToSystem(a.display, systems);
        if (!system) {
            goto done;
        }

        /* call the processing engine to process the parsed assignment */

        ret = nv_process_parsed_attribute(op, &a, system, NV_TRUE, NV_TRUE,
                                          "in assignment '%s'",
                                          assignments[assignment]);
        if (ret == NV_FALSE) goto done;

        /* print a newline at the end */

        nv_msg(NULL, "");

    } /* assignment */

    val = NV_TRUE;

 done:

    return val;

} /* nv_process_attribute_assignments() */



/*
 * validate_value() - query the valid values for the specified integer
 * attribute, and check that the value to be assigned is valid.
 */

static int validate_value(const Options *op, CtrlTarget *t,
                          ParsedAttribute *p, uint32 d, int target_type,
                          char *whence)
{
    int bad_val = NV_FALSE;
    NVCTRLAttributeValidValuesRec valid;
    ReturnStatus status;
    char d_str[256];
    char *tmp_d_str;
    const CtrlTargetTypeInfo *targetTypeInfo;
    const AttributeTableEntry *a = p->attr_entry;

    if (a->type != CTRL_ATTRIBUTE_TYPE_INTEGER) {
        return NV_FALSE;
    }

    status = NvCtrlGetValidDisplayAttributeValues(t, d, a->attr, &valid);
    if (status != NvCtrlSuccess) {
        nv_error_msg("Unable to query valid values for attribute %s (%s).",
                     a->name, NvCtrlAttributesStrError(status));
        return NV_FALSE;
    }

    if (target_type != DISPLAY_TARGET &&
        valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
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
        if ((p->val.i < 0) || (p->val.i > 1)) {
            bad_val = NV_TRUE;
        }
        break;
    case ATTRIBUTE_TYPE_RANGE:
        if (a->f.int_flags.is_packed) {
            if (((p->val.i >> 16) < (valid.u.range.min >> 16)) ||
                ((p->val.i >> 16) > (valid.u.range.max >> 16)) ||
                ((p->val.i & 0xffff) < (valid.u.range.min & 0xffff)) ||
                ((p->val.i & 0xffff) > (valid.u.range.max & 0xffff)))
                bad_val = NV_TRUE;
        } else {
            if ((p->val.i < valid.u.range.min) ||
                (p->val.i > valid.u.range.max))
                bad_val = NV_TRUE;
        }
        break;
    case ATTRIBUTE_TYPE_INT_BITS:
        if (a->f.int_flags.is_packed) {
            unsigned int u, l;

             u = (((unsigned int) p->val.i) >> 16);
             l = (p->val.i & 0xffff);

             if ((u > 15) || ((valid.u.bits.ints & (1 << u << 16)) == 0) ||
                 (l > 15) || ((valid.u.bits.ints & (1 << l)) == 0)) {
                bad_val = NV_TRUE;
            }
        } else {
            if ((p->val.i > 31) || (p->val.i < 0) ||
                ((valid.u.bits.ints & (1<<p->val.i)) == 0)) {
                bad_val = NV_TRUE;
            }
        }
        break;
    default:
        bad_val = NV_TRUE;
        break;
    }

    /* is this value available for this target type? */

    targetTypeInfo = NvCtrlGetTargetTypeInfo(target_type);
    if (!targetTypeInfo ||
        !(targetTypeInfo->permission_bit & valid.permissions)) {
        bad_val = NV_TRUE;
    }

    /* if the value is bad, print why */

    if (bad_val) {
        if (a->f.int_flags.is_packed) {
            nv_warning_msg("The value pair %d,%d for attribute '%s' (%s%s) "
                           "specified %s is invalid.",
                           p->val.i >> 16, p->val.i & 0xffff,
                           a->name, t->name,
                           d_str, whence);
        } else {
            nv_warning_msg("The value %d for attribute '%s' (%s%s) "
                           "specified %s is invalid.",
                           p->val.i, a->name, t->name,
                           d_str, whence);
        }
        print_valid_values(op, a, valid);
        return NV_FALSE;
    }
    return NV_TRUE;

} /* validate_value() */



/*
 * print_valid_values() - prints the valid values for the specified
 * attribute.
 */

static void print_valid_values(const Options *op, const AttributeTableEntry *a,
                               NVCTRLAttributeValidValuesRec valid)
{
    int bit, print_bit, last, last2, n, i;
    char str[256];
    char *c;
    char str2[256];
    char *c2; 
    char **at;
    const char *name = a->name;

    /* do not print any valid values information when we are in 'terse' mode */

    if (op->terse) {
        return;
    }

#define INDENT "    "

    switch (valid.type) {
    case ATTRIBUTE_TYPE_STRING:
        nv_msg(INDENT, "'%s' is a string attribute.", name);
        break;

    case ATTRIBUTE_TYPE_64BIT_INTEGER:
        nv_msg(INDENT, "'%s' is a 64 bit integer attribute.", name);
        break;

    case ATTRIBUTE_TYPE_INTEGER:
        if ((a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
            a->f.int_flags.is_packed) {
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
        if ((a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
            a->f.int_flags.is_packed) {
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
                if ((bit > 15) &&
                    (a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
                    a->f.int_flags.is_packed) {
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

            if ((bit > 15) &&
                (a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
                a->f.int_flags.is_packed) {
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

        if ((a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
            a->f.int_flags.is_packed) {
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

    for (i = 0; i < MAX_TARGET_TYPES; i++) {
        const CtrlTargetTypeInfo *targetTypeInfo =
            NvCtrlGetTargetTypeInfo(i);

        if (valid.permissions & targetTypeInfo->permission_bit) {
            if (n > 0) c += sprintf(c, ", ");
            c += sprintf(c, "%s", targetTypeInfo->name);
            n++;
        }
    }

    if (n == 0) sprintf(c, "None");

    nv_msg(INDENT, "'%s' can use the following target types: %s.",
           name, str);

    if (nv_get_verbosity() >= NV_VERBOSITY_ALL) {
        if (a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) {
            print_additional_info(name, a->attr, valid, INDENT);
        }
    }

#undef INDENT

} /* print_valid_values() */



/*
 * print_queried_value() - print the (integer) attribute value that we queried
 * from NV-CONTROL
 */

typedef enum {
    VerboseLevelTerse,
    VerboseLevelAbbreviated,
    VerboseLevelVerbose,
} VerboseLevel;

static void print_queried_value(const Options *op,
                                CtrlTarget *target,
                                NVCTRLAttributeValidValuesRec *v,
                                int val,
                                const AttributeTableEntry *a,
                                uint32 mask,
                                const char *indent,
                                const VerboseLevel level)
{
    char d_str[64], val_str[64], *tmp_d_str;

    if (a->type != CTRL_ATTRIBUTE_TYPE_INTEGER) {
        return;
    }

    /* assign val_str */

    if (a->f.int_flags.is_display_id && op->dpy_string) {
        const char *name = NvCtrlGetDisplayConfigName(target->system, val);
        if (name) {
            snprintf(val_str, 64, "%s", name);
        } else {
            snprintf(val_str, 64, "%d", val);
        }
    } else if (a->f.int_flags.is_display_mask && op->dpy_string) {
        tmp_d_str = display_device_mask_to_display_device_name(val);
        snprintf(val_str, 64, "%s", tmp_d_str);
        free(tmp_d_str);
    } else if (a->f.int_flags.is_100Hz) {
        snprintf(val_str, 64, "%.2f Hz", ((float) val) / 100.0);
    } else if (a->f.int_flags.is_1000Hz) {
        snprintf(val_str, 64, "%.3f Hz", ((float) val) / 1000.0);
    } else if (v->type == ATTRIBUTE_TYPE_BITMASK) {
        snprintf(val_str, 64, "0x%08x", val);
    } else if (a->f.int_flags.is_packed) {
        snprintf(val_str, 64, "%d,%d", val >> 16, val & 0xffff);
    } else {
        snprintf(val_str, 64, "%d", val);
    }

    /* append the display device name, if necessary */

    if ((NvCtrlGetTargetType(target) != DISPLAY_TARGET) &&
        v->permissions & ATTRIBUTE_TYPE_DISPLAY) {
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
        nv_msg(indent, "%s: %s", a->name, val_str);
        break;

    case VerboseLevelVerbose:
        /*
         * or print the value along with other information about the
         * attribute
         */
        nv_msg(indent,  "Attribute '%s' (%s%s): %s.",
               a->name, target->name, d_str, val_str);
        break;
    }

} /* print_queried_value() */



/*
 * print_additional_stereo_info() - print the available stereo modes
 */

static void print_additional_stereo_info(const char *name,
                                         unsigned int valid_stereo_modes,
                                         const char *indent)
{
    int bit;

    nv_msg(indent, "\n");
    nv_msg(indent, "Valid '%s' Values\n", name);
    nv_msg(indent, "  value - description\n");

    for (bit = 0; bit < 32; bit++) {
        if (valid_stereo_modes & (1 << bit)) {
            nv_msg(indent, "   %2u   -   %s\n", 
                   bit, NvCtrlGetStereoModeName(bit));
        }
    }
}


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

    case NV_CTRL_STEREO:
        if (valid.type == ATTRIBUTE_TYPE_INT_BITS) {
            print_additional_stereo_info(name, valid.u.bits.ints, indent);
        }
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

static int query_all(const Options *op, const char *display_name,
                     CtrlSystemList *systems)
{
    int bit, entry, val, i;
    uint32 mask;
    ReturnStatus status;
    NVCTRLAttributeValidValuesRec valid;
    CtrlSystem *system;

    system = NvCtrlConnectToSystem(display_name, systems);
    if (!system) {
        return NV_FALSE;
    }

#define INDENT "  "

    /*
     * Loop through all target types.
     */

    for (i = 0; i < MAX_TARGET_TYPES; i++) {
        CtrlTargetNode *node;
        const CtrlTargetTypeInfo *targetTypeInfo = NvCtrlGetTargetTypeInfo(i);


        for (node = system->targets[i]; node; node = node->next) {
            CtrlTarget *t = node->t;

            if (!t->h) continue;

            nv_msg(NULL, "Attributes queryable via %s:", t->name);

            if (!op->terse) {
                nv_msg(NULL, "");
            }

            for (entry = 0; entry < attributeTableLen; entry++) {
                const AttributeTableEntry *a = &attributeTable[entry];

                /* skip the color attributes */

                if (a->type == CTRL_ATTRIBUTE_TYPE_COLOR) {
                    continue;
                }

                /* skip attributes that shouldn't be queried here */

                if (a->flags.no_query_all) {
                    continue;
                }

                for (bit = 0; bit < 24; bit++) {
                    mask = 1 << bit;

                    /*
                     * if this bit is not present in the screens's enabled
                     * display device mask (and the X screen has enabled
                     * display devices), skip to the next bit
                     */

                    if (targetTypeInfo->uses_display_devices &&
                        ((t->d & mask) == 0x0) && (t->d)) continue;

                    if (a->type == CTRL_ATTRIBUTE_TYPE_STRING) {
                        char *tmp_str = NULL;

                        status =
                            NvCtrlGetValidStringDisplayAttributeValues(t,
                                                                       mask,
                                                                       a->attr,
                                                                       &valid);
                        if (status == NvCtrlAttributeNotAvailable) {
                            goto exit_bit_loop;
                        }

                        if (status != NvCtrlSuccess) {
                            nv_error_msg("Error while querying valid values for "
                                         "attribute '%s' on %s (%s).",
                                         a->name, t->name,
                                         NvCtrlAttributesStrError(status));
                            goto exit_bit_loop;
                        }

                        status = NvCtrlGetStringDisplayAttribute(t,
                                                                 mask,
                                                                 a->attr,
                                                                 &tmp_str);

                        if (status == NvCtrlAttributeNotAvailable) {
                            goto exit_bit_loop;
                        }

                        if (status != NvCtrlSuccess) {
                            nv_error_msg("Error while querying attribute '%s' "
                                         "on %s (%s).", a->name, t->name,
                                         NvCtrlAttributesStrError(status));
                            goto exit_bit_loop;
                        }

                        if (op->terse) {
                            nv_msg("  ", "%s: %s", a->name, tmp_str);
                        } else {
                            nv_msg("  ",  "Attribute '%s' (%s%s): %s ",
                                   a->name, t->name, "", tmp_str);
                        }
                        free(tmp_str);
                        tmp_str = NULL;

                    } else {

                        status = NvCtrlGetValidDisplayAttributeValues(t,
                                                                      mask,
                                                                      a->attr,
                                                                      &valid);

                        if (status == NvCtrlAttributeNotAvailable) {
                            goto exit_bit_loop;
                        }

                        if (status != NvCtrlSuccess) {
                            nv_error_msg("Error while querying valid values for "
                                         "attribute '%s' on %s (%s).",
                                         a->name, t->name,
                                         NvCtrlAttributesStrError(status));
                            goto exit_bit_loop;
                        }

                        status = NvCtrlGetDisplayAttribute(t, mask, a->attr,
                                                           &val);

                        if (status == NvCtrlAttributeNotAvailable) {
                            goto exit_bit_loop;
                        }

                        if (status != NvCtrlSuccess) {
                            nv_error_msg("Error while querying attribute '%s' "
                                         "on %s (%s).", a->name, t->name,
                                         NvCtrlAttributesStrError(status));
                            goto exit_bit_loop;
                        }

                        print_queried_value(op, t, &valid, val, a, mask,
                                            INDENT, op->terse ?
                                            VerboseLevelAbbreviated :
                                            VerboseLevelVerbose);

                    }
                    print_valid_values(op, a, valid);

                    if (!op->terse) {
                        nv_msg(NULL,"");
                    }

                    if (valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
                        continue;
                    }

                    /* fall through to exit_bit_loop */

exit_bit_loop:

                    break; /* XXX force us out of the display device loop */

                } /* bit */

            } /* entry */

        } /* j (targets) */

    } /* i (target types) */

#undef INDENT

    return NV_TRUE;

} /* query_all() */



/*
 * get_product_name() Returns the (GPU, X screen, display device or VCS)
 * product name of the given target.
 */

static char *get_product_name(CtrlTarget *ctrl_target, int attr)
{
    char *product_name = NULL;
    ReturnStatus status;

    status = NvCtrlGetStringAttribute(ctrl_target, attr, &product_name);

    if (status != NvCtrlSuccess) {
        return strdup("Unknown");
    }

    return product_name;
}



/*
 * Returns the (RandR) display device name to use for printing the given
 * display target.
 */

static char *get_display_product_name(CtrlTarget *t)
{
    return nvstrdup(t->protoNames[NV_DPY_PROTO_NAME_RANDR]);
}



/*
 * Returns the connection and enabled state of the display device as a comma
 * separated string.
 */

static char *get_display_state_str(CtrlTarget *t)
{
    char *str = NULL;

    if (t->display.connected) {
        str = nvstrdup("connected");
    }

    if (t->display.enabled) {
        nv_append_sprintf(&str, "%s%s",
                          str ? ", " : "",
                          "enabled");
    }

    return str;
}



/*
 * print_target_connections() Prints a list of all targets connected
 * to a given target using print_func if the connected targets require
 * special handling.
 */

static int print_target_connections(CtrlTarget *t,
                                    const char *relation_str,
                                    const char *null_relation_str,
                                    unsigned int attrib,
                                    unsigned int target_type)
{
    int *pData;
    int len, i;
    ReturnStatus status;
    const CtrlTargetTypeInfo *targetTypeInfo;

    targetTypeInfo = NvCtrlGetTargetTypeInfo(target_type);


    /* Query the connected targets */

    status =
        NvCtrlGetBinaryAttribute(t, 0, attrib,
                                 (unsigned char **) &pData,
                                 &len);
    if (status != NvCtrlSuccess) return NV_FALSE;

    if (pData[0] == 0) {
        nv_msg("      ", "%s any %s.",
               null_relation_str,
               targetTypeInfo->name);
        nv_msg(NULL, "");

        free(pData);
        return NV_TRUE;
    }

    nv_msg("      ", "%s the following %s%s:",
           relation_str,
           targetTypeInfo->name,
           ((pData[0] > 1) ? "s" : ""));

    /* List the connected targets */

    for (i = 1; i <= pData[0]; i++) {
        CtrlTarget *other = NvCtrlGetTarget(t->system, target_type, pData[i]);
        char *target_name = NULL;
        char *product_name = NULL;
        Bool is_x_name = NV_FALSE;
        char *extra_str = NULL;


        if (other) {
            target_name = other->name;

            switch (target_type) {
            case GPU_TARGET:
                product_name =
                    get_product_name(other, NV_CTRL_STRING_PRODUCT_NAME);
                is_x_name = NV_TRUE;
                break;

            case VCS_TARGET:
                product_name =
                    get_product_name(other, NV_CTRL_STRING_VCSC_PRODUCT_NAME);
                is_x_name = NV_TRUE;
                break;

            case DISPLAY_TARGET:
                product_name = get_display_product_name(other);
                extra_str = get_display_state_str(other);
                break;

            case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
            case THERMAL_SENSOR_TARGET:
            case COOLER_TARGET:
            case FRAMELOCK_TARGET:
            case GVI_TARGET:
            case X_SCREEN_TARGET:
            default:
                break;
            }
        }

        if (!target_name) {
            nv_msg("        ", "Unknown %s",
                   targetTypeInfo->name);

        } else if (product_name) {
            nv_msg("        ", "%s (%s)%s%s%s",
                   target_name, product_name,
                   extra_str ? " (" : "",
                   extra_str ? extra_str : "",
                   extra_str ? ")" : "");
        } else {
            nv_msg("        ", "%s (%s %d)",
                   target_name,
                   targetTypeInfo->name,
                   pData[i]);
        }

        if (product_name) {
            if (is_x_name) {
                free(product_name);
            } else {
                nvfree(product_name);
            }
        }
        if (extra_str) {
            nvfree(extra_str);
        }
    }
    nv_msg(NULL, "");

    free(pData);
    return NV_TRUE;

} /* print_target_connections() */



/*
 * query_all_targets() - print a list of all the targets (of the
 * specified type) accessible via the Display connection.
 */

static int query_all_targets(const char *display_name, const int target_type,
                             CtrlSystemList *systems)
{
    CtrlSystem *system;
    CtrlTargetNode *node;
    CtrlTarget *t;
    char *str, *name;
    const CtrlTargetTypeInfo *targetTypeInfo;
    int target_count;
    int idx;

    targetTypeInfo = NvCtrlGetTargetTypeInfo(target_type);

    /* create handles */

    system = NvCtrlConnectToSystem(display_name, systems);
    if (!system) {
        return NV_FALSE;
    }

    /* build the standard X server name */

    str = nv_standardize_screen_name(XDisplayName(system->display), -2);

    /* warn if we don't have any of the target type */

    if (!system->targets[target_type]) {
        nv_warning_msg("No %ss on %s", targetTypeInfo->name, str);
        free(str);
        return NV_FALSE;
    }

    /* print how many of the target type we have */

    target_count = NvCtrlGetTargetTypeCount(system, target_type);
    nv_msg(NULL, "%d %s%s on %s",
           target_count,
           targetTypeInfo->name,
           (target_count > 1) ? "s" : "",
           str);
    nv_msg(NULL, "");

    free(str);

    /* print information per target */

    for (node = system->targets[target_type], idx = 0;
         node;
         node = node->next, idx++) {
        char buff[PRODUCT_NAME_LEN];
        char *product_name = buff;
        char *extra_str = NULL;
        int target_id;

        t = node->t;

        target_id = NvCtrlGetTargetId(t);

        str = NULL;

        switch (target_type) {
        case FRAMELOCK_TARGET:
        case GVI_TARGET:
        case COOLER_TARGET:
        case THERMAL_SENSOR_TARGET:
        case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
             snprintf(product_name, PRODUCT_NAME_LEN, "%s %d",
                     targetTypeInfo->name, target_id);
             break;
        case VCS_TARGET:
            product_name = get_product_name(t, NV_CTRL_STRING_VCSC_PRODUCT_NAME);
            break;
        case DISPLAY_TARGET:
            product_name = get_display_product_name(t);
            extra_str = get_display_state_str(t);
            break;
        default:
            product_name = get_product_name(t, NV_CTRL_STRING_PRODUCT_NAME);
            break;
        }

        /*
         * use the name for the target handle, or "Not NVIDIA" if we
         * don't have a target handle name (this can happen for a
         * non-NVIDIA X screen)
         */

        if (t->name) {
            name = t->name;
        } else {
            name = "Not NVIDIA";
        }

        nv_msg("    ", "[%d] %s (%s)%s%s%s",
               idx,
               name,
               product_name,
               extra_str ? " (" : "",
               extra_str ? extra_str : "",
               extra_str ? ")" : "");
        nv_msg(NULL, "");

        if (product_name != buff) {
            free(product_name);
        }
        if (extra_str) {
            nvfree(extra_str);
        }

        /* Print the valid protocal names for the target */
        if (t->protoNames[0]) {
            int idx;

            nv_msg("      ", "Has the following name%s:",
                   t->protoNames[1] ? "s" : "");
            for (idx = 0; idx < NV_PROTO_NAME_MAX; idx++) {
                if (t->protoNames[idx]) {
                    nv_msg("        ", "%s", t->protoNames[idx]);
                }
            }

            nv_msg(NULL, "");
        }

        /* Print connectivity information */
        if (nv_get_verbosity() >= NV_VERBOSITY_ALL) {
            switch (target_type) {
            case GPU_TARGET:
                print_target_connections
                    (t,
                     "Is driving",
                     "Is not driving",
                     NV_CTRL_BINARY_DATA_XSCREENS_USING_GPU,
                     X_SCREEN_TARGET);
                print_target_connections
                    (t,
                     "Supports",
                     "Does not support",
                     NV_CTRL_BINARY_DATA_DISPLAYS_ON_GPU,
                     DISPLAY_TARGET);
                print_target_connections
                    (t,
                     "Is connected to",
                     "Is not connected to",
                     NV_CTRL_BINARY_DATA_FRAMELOCKS_USED_BY_GPU,
                     FRAMELOCK_TARGET);
                print_target_connections
                    (t,
                     "Is connected to",
                     "Is not connected to",
                     NV_CTRL_BINARY_DATA_VCSCS_USED_BY_GPU,
                     VCS_TARGET);
                print_target_connections
                    (t,
                     "Is connected to",
                     "Is not connected to",
                     NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
                     COOLER_TARGET);
                print_target_connections
                    (t,
                     "Is connected to",
                     "Is not connected to",
                     NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU,
                     THERMAL_SENSOR_TARGET);
                break;

            case X_SCREEN_TARGET:
                print_target_connections
                    (t,
                     "Is driven by",
                     "Is not driven by",
                     NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                     GPU_TARGET);
                print_target_connections
                    (t,
                     "Is assigned",
                     "Is not assigned",
                     NV_CTRL_BINARY_DATA_DISPLAYS_ASSIGNED_TO_XSCREEN,
                     DISPLAY_TARGET);
                break;

            case FRAMELOCK_TARGET:
                print_target_connections
                    (t,
                     "Is connected to",
                     "Is not connected to",
                     NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK,
                     GPU_TARGET);
                break;

            case VCS_TARGET:
                print_target_connections
                    (t,
                     "Is connected to",
                     "Is not connected to",
                     NV_CTRL_BINARY_DATA_GPUS_USING_VCSC,
                     GPU_TARGET);
                break;

            case COOLER_TARGET:
                print_target_connections
                    (t,
                     "Is connected to",
                     "Is not connected to",
                     NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
                     COOLER_TARGET);
                break;

            case THERMAL_SENSOR_TARGET:
                print_target_connections
                    (t,
                     "Is connected to",
                     "Is not connected to",
                     NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU,
                     THERMAL_SENSOR_TARGET);
                break;

            default:
                break;
            }
        }
    }

    return NV_TRUE;

} /* query_all_targets() */



/*
 * process_parsed_attribute_internal() - this function does the actual
 * attribute processing for nv_process_parsed_attribute().
 *
 * If an error occurs, an error message is printed and NV_FALSE is
 * returned; if successful, NV_TRUE is returned.
 */

static int process_parsed_attribute_internal(const Options *op,
                                             const CtrlSystem *system,
                                             CtrlTarget *t,
                                             ParsedAttribute *p, uint32 d,
                                             int target_type, int assign,
                                             int verbose, char *whence,
                                             NVCTRLAttributeValidValuesRec
                                             valid)
{
    ReturnStatus status;
    char str[32], *tmp_d_str;
    int ret;
    const AttributeTableEntry *a = p->attr_entry;

    if (target_type != DISPLAY_TARGET &&
        valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
        tmp_d_str = display_device_mask_to_display_device_name(d);
        sprintf(str, ", display device: %s", tmp_d_str);
        free(tmp_d_str);
    } else {
        str[0] = '\0';
    }

    if (assign) {
        if (a->type == CTRL_ATTRIBUTE_TYPE_STRING) {
            status = NvCtrlSetStringAttribute(t, a->attr, p->val.str);
        } else {

            ret = validate_value(op, t, p, d, target_type, whence);
            if (!ret) return NV_FALSE;

            status = NvCtrlSetDisplayAttribute(t, d, a->attr, p->val.i);

            if (status != NvCtrlSuccess) {
                nv_error_msg("Error assigning value %d to attribute '%s' "
                             "(%s%s) as specified %s (%s).",
                             p->val.i, a->name, t->name, str, whence,
                             NvCtrlAttributesStrError(status));
                return NV_FALSE;
            }

            if (verbose) {
                if (a->f.int_flags.is_packed) {
                    nv_msg("  ", "Attribute '%s' (%s%s) assigned value %d,%d.",
                           a->name, t->name, str,
                           p->val.i >> 16, p->val.i & 0xffff);
                } else {
                    nv_msg("  ", "Attribute '%s' (%s%s) assigned value %d.",
                           a->name, t->name, str, p->val.i);
                }
            }
        }

    } else { /* query */
        if (a->type == CTRL_ATTRIBUTE_TYPE_STRING) {
            char *tmp_str = NULL;
            status = NvCtrlGetStringDisplayAttribute(t, d, a->attr,
                                                     &tmp_str);

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

                if (op->terse) {
                    nv_msg(NULL, "%s", tmp_str);
                } else {
                    nv_msg("  ",  "Attribute '%s' (%s%s): %s",
                           a->name, t->name, str, tmp_str);
                }
                free(tmp_str);
                tmp_str = NULL;
            }
        } else { 
            status = NvCtrlGetDisplayAttribute(t, d, a->attr, &p->val.i);

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
                print_queried_value(op, t, &valid, p->val.i, a, d,
                                    "  ", op->terse ?
                                    VerboseLevelTerse : VerboseLevelVerbose);
                print_valid_values(op, a, valid);
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
 * To accommodate the differences in processing needed for each of the
 * callers of this function, the parameters 'assign' and 'verbose'
 * are used; if assign is TRUE, then the attribute will be assigned
 * during processing, otherwise it will be queried.  If verbose is
 * TRUE, then a message will be printed out during each assignment (or
 * query).
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

int nv_process_parsed_attribute(const Options *op,
                                ParsedAttribute *p, CtrlSystem *system,
                                int assign, int verbose,
                                char *whence_fmt, ...)
{
    int ret, val;
    char *whence, *tmp_d_str0, *tmp_d_str1;
    ReturnStatus status;
    CtrlTargetNode *n;
    NVCTRLAttributeValidValuesRec valid;
    const AttributeTableEntry *a = p->attr_entry;


    val = NV_FALSE;

    /* build the whence string */

    NV_VSNPRINTF(whence, whence_fmt);

    if (!whence) whence = strdup("\0");

    /* if we don't have a Display connection, abort now */

    if (system == NULL || system->dpy == NULL) {
        nv_error_msg("Unable to %s attribute %s specified %s (no Display "
                     "connection).", assign ? "assign" : "query",
                     a->name, whence);
        goto done;
    }

    /* Print deprecation messages */
    if (strncmp(a->desc, "DEPRECATED", 10) == 0) {
        const char *str = a->desc + 10;
        while (*str &&
               (*str == ':' || *str == '.')) {
            str++;
        }
        nv_deprecated_msg("The attribute '%s' is deprecated%s%s",
                          a->name,
                          *str ? "," : ".",
                          *str ? str : "");
    }

    /* Print not supported messages */
    if (strncmp(a->desc, "NOT SUPPORTED", 13) == 0) {
        const char *str = a->desc + 13;
        while (*str &&
               (*str == ':' || *str == '.')) {
            str++;
        }
        nv_deprecated_msg("The attribute '%s' is no longer supported%s%s",
                          a->name,
                          *str ? "," : ".",
                          *str ? str : "");
        goto done;
    }
    /* Resolve any target specifications against the CtrlSystem that was
     * allocated.
     */

    ret = resolve_attribute_targets(p, system, whence);
    if (ret != NV_PARSER_STATUS_SUCCESS) {
        nv_error_msg("Error resolving target specification '%s' "
                     "(%s), specified %s.",
                     p->target_specification ? p->target_specification : "",
                     nv_parse_strerror(ret),
                     whence);
        goto done;
    }

    if (!p->targets) {
        nv_warning_msg("Failed to match any targets for target specification "
                       "'%s', specified %s.",
                       p->target_specification ? p->target_specification : "",
                       whence);
    }

    /* loop over the requested targets */

    for (n = p->targets; n; n = n->next) {

        CtrlTarget *t = n->t;
        int target_type;
        uint32 mask;

        if (!t->h) continue; /* no handle on this target; silently skip */

        target_type = NvCtrlGetTargetType(t);


        if (op->list_targets) {
            const char *name = t->protoNames[0];
            const char *randr_name = NULL;

            if (target_type == DISPLAY_TARGET) {
                name = t->protoNames[NV_DPY_PROTO_NAME_TARGET_INDEX];
                randr_name =t->protoNames[NV_DPY_PROTO_NAME_RANDR];
            }

            nv_msg(TAB, "%s '%s' on %s%s%s%s\n",
                   assign ? "Assign" : "Query",
                   a->name,
                   name,
                   randr_name ? " (" : "",
                   randr_name ? randr_name : "",
                   randr_name ? ")" : ""
                   );
            continue;
        }


        /* special case the color attributes */

        if (a->type == CTRL_ATTRIBUTE_TYPE_COLOR) {
            float v[3];
            if (!assign) {
                nv_msg(NULL, "Attribute '%s' cannot be queried.", a->name);
                goto done;
            }

            /*
             * assign p->val.f to all values in the array; a->attr will
             * tell NvCtrlSetColorAttributes() which indices in the
             * array to use
             */

            v[0] = v[1] = v[2] = p->val.f;

            status = NvCtrlSetColorAttributes(t, v, v, v, a->attr);

            if (status != NvCtrlSuccess) {
                nv_error_msg("Error assigning %f to attribute '%s' on %s "
                             "specified %s (%s)", p->val.f, a->name,
                             t->name, whence,
                             NvCtrlAttributesStrError(status));
                goto done;
            }

            continue;
        }

        /*
         * If we are assigning, and the value for this attribute is a
         * display device mask, then we need to validate the value against
         * either the mask of enabled display devices or the mask of
         * connected display devices.
         */

        if (assign &&
            (a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
            a->f.int_flags.is_display_mask) {
            char *display_device_descriptor = NULL;
            uint32 check_mask;

            /* use the complete mask if requested */

            if (p->parser_flags.assign_all_displays) {
                if (a->f.int_flags.is_switch_display) {
                    p->val.i = t->c;
                } else {
                    p->val.i = t->d;
                }
            }

            /*
             * if we are hotkey switching, check against all connected
             * displays; otherwise, check against the currently active
             * display devices
             */

            if (a->f.int_flags.is_switch_display) {
                check_mask = t->c;
                display_device_descriptor = "connected";
            } else {
                check_mask = t->d;
                display_device_descriptor = "enabled";
            }

            if ((check_mask & p->val.i) != p->val.i) {

                tmp_d_str0 =
                    display_device_mask_to_display_device_name(p->val.i);

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
         * If we are assigning, and the value for this attribute is a display
         * device id/name string, then we need to validate and convert the
         * given string against list of available display devices.  The resulting
         * id, if any, is then fed back into the ParsedAttrinute's value as an
         * int which is ultimately written out to NV-CONTROL.
         */
        if (assign &&
            (a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
            a->f.int_flags.is_display_id) {
            CtrlTargetNode *dpy_node;
            int found = NV_FALSE;
            int multi_match = NV_FALSE;
            int is_id;
            char *tmp = NULL;
            int id;


            /* See if value is a simple number */
            id = strtol(p->val.str, &tmp, 0);
            is_id = (tmp &&
                     (tmp != p->val.str) &&
                     (*tmp == '\0'));

            for (dpy_node = system->targets[DISPLAY_TARGET];
                 dpy_node;
                 dpy_node = dpy_node->next) {
                CtrlTarget *dpy_target = dpy_node->t;

                if (is_id) {
                    /* Value given as display device (target) ID */
                    if (id == NvCtrlGetTargetId(dpy_target)) {
                        found = NV_TRUE;
                        break;
                    }
                } else {
                    /* Value given as display device name */
                    if (nv_target_has_name(dpy_target, p->val.str)) {
                        if (found) {
                            multi_match = TRUE;
                            break;
                        }
                        id = NvCtrlGetTargetId(dpy_target);
                        found = NV_TRUE;

                        /* Keep looking to make sure the name doesn't alias to
                         * another display device.
                         */
                        continue;
                    }
                }
            }

            if (multi_match) {
                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "assigned the value of '%s' (This name matches "
                             "multiple display devices, please use a non-"
                             "ambiguous name.",
                             a->name, whence, p->val.str);
                continue;
            }

            if (!found) {
                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "assigned the value of '%s' (This does not "
                             "name an available display device).",
                             a->name, whence, p->val.str);
                continue;
            }

            /* Put converted id back into a->val */
            p->val.i = id;
        }


        /*
         * If we are assigning, and the value for this attribute is
         * not allowed to be zero, check that the value is not zero.
         */

        if (assign &&
            (a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
            a->f.int_flags.no_zero) {

            /* value must be non-zero */

            if (!p->val.i) {

                if (a->f.int_flags.is_display_mask) {
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

        if (a->flags.is_framelock_attribute &&
            (target_type != FRAMELOCK_TARGET)) {
            int available;

            status = NvCtrlGetAttribute(t, NV_CTRL_FRAMELOCK, &available);
            if (status != NvCtrlSuccess) {
                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "%s; error querying frame lock availability on "
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

            if (assign &&
                (a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
                (a->attr != NV_CTRL_FRAMELOCK_SYNC)) {
                int enabled;

                status = get_framelock_sync_state(t, &enabled);

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

        if (a->flags.is_sdi_attribute &&
            target_type != GVI_TARGET) {
            int available;

            status = NvCtrlGetAttribute(t, NV_CTRL_GVO_SUPPORTED,
                                        &available);
            if (status != NvCtrlSuccess) {
                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "%s; error querying SDI availability on "
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

        /* Handle string operations */

        if (a->type == CTRL_ATTRIBUTE_TYPE_STRING_OPERATION) {
            char *ptrOut = NULL;

            /* NOTE: You can only assign string operations */
            if (!assign) {
                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "queried;  String operations can only be "
                             "assigned.",
                             a->name, whence);
                continue;
            }

            status = NvCtrlStringOperation(t, 0, a->attr, p->val.str,
                                           &ptrOut);
            if (status != NvCtrlSuccess) {
                nv_error_msg("Error processing attribute '%s' (%s %s) "
                             "specified %s (%s).",
                             a->name, t->name, p->val.str, whence,
                             NvCtrlAttributesStrError(status));
                continue;
            }
            if (op->terse) {
                nv_msg(NULL, "%s", ptrOut);
            } else {
                nv_msg("  ",  "Attribute '%s' (%s %s): %s",
                       a->name, t->name, p->val.str, ptrOut);
            }
            free(ptrOut);
            continue;
        }



        /* Special case the GVO CSC attribute */

        if (a->type == CTRL_ATTRIBUTE_TYPE_SDI_CSC) {
            float colorMatrix[3][3];
            float colorOffset[3];
            float colorScale[3];
            int r, c;

            if (assign) {

                /* Make sure the standard is known */
                if (!p->val.pf) {
                    nv_error_msg("The attribute '%s' specified %s cannot be "
                                 "assigned; valid values are \"ITU_601\", "
                                 "\"ITU_709\", \"ITU_177\", and \"Identity\".",
                                 a->name, whence);
                    continue;
                }

                for (r = 0; r < 3; r++) {
                    for (c = 0; c < 3; c++) {
                        colorMatrix[r][c] = p->val.pf[r*5 + c];
                    }
                    colorOffset[r] = p->val.pf[r*5 + 3];
                    colorScale[r] = p->val.pf[r*5 + 4];
                }

                status = NvCtrlSetGvoColorConversion(t,
                                                     colorMatrix,
                                                     colorOffset,
                                                     colorScale);
            } else {
                status = NvCtrlGetGvoColorConversion(t,
                                                     colorMatrix,
                                                     colorOffset,
                                                     colorScale);
            }

            if (status != NvCtrlSuccess) {
                nv_error_msg("The attribute '%s' specified %s cannot be "
                             "%s; error on %s (%s).",
                             a->name, whence,
                             assign ? "assigned" : "queried",
                             t->name, NvCtrlAttributesStrError(status));
                continue;
            }


            /* Print results */
            if (!assign) {
#define INDENT "    "

                nv_msg(INDENT, "   Red       Green     Blue      Offset    Scale");
                nv_msg(INDENT, "----------------------------------------------------");
                nv_msg(INDENT, " Y % -.6f % -.6f % -.6f % -.6f % -.6f",
                       colorMatrix[0][0],
                       colorMatrix[0][1],
                       colorMatrix[0][2],
                       colorOffset[0],
                       colorScale[0]);
                nv_msg(INDENT, "Cr % -.6f % -.6f % -.6f % -.6f % -.6f",
                       colorMatrix[1][0],
                       colorMatrix[1][1],
                       colorMatrix[1][2],
                       colorOffset[1],
                       colorScale[1]);
                nv_msg(INDENT, "Cb % -.6f % -.6f % -.6f % -.6f % -.6f",
                       colorMatrix[2][0],
                       colorMatrix[2][1],
                       colorMatrix[2][2],
                       colorOffset[2],
                       colorScale[2]);
#undef INDENT
            }

            continue;
        }


        /*
         * special case the display device mask in the case that it
         * was "hijacked" for something other than a display device:
         * assign mask here so that it will be passed through to
         * process_parsed_attribute_internal() unfiltered
         */

        if (a->flags.hijack_display_device) {
            mask = p->display_device_mask;
        } else {
            mask = 0;
        }

        if (a->type == CTRL_ATTRIBUTE_TYPE_STRING) {
            status = NvCtrlGetValidStringDisplayAttributeValues(t,
                                                                mask,
                                                                a->attr,
                                                                &valid);
        } else {
            status = NvCtrlGetValidDisplayAttributeValues(t,
                                                          mask,
                                                          a->attr,
                                                          &valid);
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
            continue;
        }

        /*
         * if this attribute is going to be assigned, then check
         * that the attribute is writable; if it's not, give up
         */

        if (assign && !(valid.permissions & ATTRIBUTE_TYPE_WRITE)) {
            nv_error_msg("The attribute '%s' specified %s cannot be "
                         "assigned (it is a read-only attribute).",
                         a->name, whence);
            continue;
        }

        ret = process_parsed_attribute_internal(op, system, t, p, mask,
                                                target_type, assign, verbose,
                                                whence, valid);
        if (ret == NV_FALSE) {
            continue;
        }
    } /* done looping over requested targets */

    val = NV_TRUE;

 done:
    if (whence) free(whence);
    return val;

} /* nv_process_parsed_attribute() */



static ReturnStatus get_framelock_sync_state(CtrlTarget *ctrl_target,
                                             int *enabled)
{
    CtrlTargetNode *node;
    CtrlTarget *query_target;

    /* NV_CTRL_FRAMELOCK_SYNC should be queried on a GPU target,
     * so use the display's connected GPU when querying via a display
     * target.
     */
    switch (NvCtrlGetTargetType(ctrl_target)) {
    case GPU_TARGET:
        query_target = ctrl_target;
        break;

    case DISPLAY_TARGET:
        for (node = ctrl_target->relations; node; node = node->next) {
            if (NvCtrlGetTargetType(node->t) == GPU_TARGET) {
                query_target = node->t;
                goto query;
            }
        }
        /* Failed to find the GPU driving the display! */
        return NvCtrlError;

    default:
        /* Can't query NV_CTRL_FRAMELOCK_SYNC on any other target */
        return NvCtrlAttributeNotAvailable;
    }

query:
    return NvCtrlGetAttribute(query_target, NV_CTRL_FRAMELOCK_SYNC, enabled);
}
