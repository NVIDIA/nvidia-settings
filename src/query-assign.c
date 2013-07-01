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
#include <string.h>
#include <inttypes.h>

#include <X11/Xlib.h>
#include "NVCtrlLib.h"

#include "parse.h"
#include "msg.h"
#include "query-assign.h"
#include "common-utils.h"

extern int __verbosity;
extern int __terse;
extern int __display_device_string;

/* local prototypes */

static int process_attribute_queries(int, char**, const char *,
                                     CtrlHandlesArray *);

static int process_attribute_assignments(int, char**, const char *,
                                     CtrlHandlesArray *);

static int query_all(const char *, CtrlHandlesArray *);
static int query_all_targets(const char *display_name, const int target_index,
                             CtrlHandlesArray *);

static void print_valid_values(const char *, int, uint32,
                               NVCTRLAttributeValidValuesRec);

static void print_additional_info(const char *name,
                                  int attr,
                                  NVCTRLAttributeValidValuesRec valid,
                                  const char *indent);

static int validate_value(CtrlHandleTarget *t, ParsedAttribute *a, uint32 d,
                          int target_type, char *whence);

static CtrlHandles *nv_alloc_ctrl_handles(const char *display);

static void nv_free_ctrl_handles(CtrlHandles *h);

/*
 * nv_process_assignments_and_queries() - process any assignments or
 * queries specified on the commandline.  If an error occurs, return
 * NV_FALSE.  On success return NV_TRUE.
 */

int nv_process_assignments_and_queries(const Options *op, 
                                       CtrlHandlesArray *handles_array)
{
    int ret;

    if (op->num_queries) {
        ret = process_attribute_queries(op->num_queries,
                                        op->queries, op->ctrl_display,
                                        handles_array);
        if (!ret) return NV_FALSE;
    }

    if (op->num_assignments) {
        ret = process_attribute_assignments(op->num_assignments,
                                            op->assignments,
                                            op->ctrl_display,
                                            handles_array);
        if (!ret) return NV_FALSE;
    }
    
    return NV_TRUE;

} /* nv_process_assignments_and_queries() */



/*!
 * Queries the NV-CONTROL string attribute and returns the string as a simple
 * char *.  This is useful to avoid having to track how strings are allocated
 * so we can cleanup all strings via nvfree().
 *
 * \param[in]  t     The CtrlHandleTarget to query the string on.
 * \param[in]  attr  The NV-CONTROL string to query.
 *
 * \return  Return a nvalloc()'ed copy of the NV-CONTROL string; else, returns
 *          NULL.
 */

static char *query_x_name(const CtrlHandleTarget *t, int attr)
{
    ReturnStatus status;
    char *x_str;
    char *str = NULL;

    status = NvCtrlGetStringAttribute(t->h, attr, &x_str);
    if (status == NvCtrlSuccess) {
        str = nvstrdup(x_str);
        XFree(x_str);
    }

    return str;
}



/*!
 * Retrieves and adds all the display device names for the given target.
 *
 * \param[in/out]  t  The CtrlHandleTarget to load names for.
 */

static void load_display_target_proto_names(CtrlHandleTarget *t)
{
    t->protoNames[NV_DPY_PROTO_NAME_TYPE_BASENAME] =
        query_x_name(t, NV_CTRL_STRING_DISPLAY_NAME_TYPE_BASENAME);

    t->protoNames[NV_DPY_PROTO_NAME_TYPE_ID] =
        query_x_name(t, NV_CTRL_STRING_DISPLAY_NAME_TYPE_ID);

    t->protoNames[NV_DPY_PROTO_NAME_DP_GUID] =
        query_x_name(t, NV_CTRL_STRING_DISPLAY_NAME_DP_GUID);

    t->protoNames[NV_DPY_PROTO_NAME_EDID_HASH] =
        query_x_name(t, NV_CTRL_STRING_DISPLAY_NAME_EDID_HASH);

    t->protoNames[NV_DPY_PROTO_NAME_TARGET_INDEX] =
        query_x_name(t, NV_CTRL_STRING_DISPLAY_NAME_TARGET_INDEX);

    t->protoNames[NV_DPY_PROTO_NAME_RANDR] =
        query_x_name(t, NV_CTRL_STRING_DISPLAY_NAME_RANDR);
}



/*!
 * Adds the default name for the given target to the list of protocol names at
 * the given proto name index.
 *
 * \param[in/out]  t          The CtrlHandleTarget to load names for.
 * \param[in]      proto_idx  The name index where to add the name.
 */

static void load_default_target_proto_name(CtrlHandleTarget *t,
                                           const int proto_idx)
{
    const int target_type = NvCtrlGetTargetType(t->h);
    const int target_id = NvCtrlGetTargetId(t->h);

    const TargetTypeEntry *targetTypeEntry =
        nv_get_target_type_entry_by_nvctrl(target_type);

    if (proto_idx >= NV_PROTO_NAME_MAX) {
        return;
    }

    if (targetTypeEntry) {
        t->protoNames[proto_idx] = nvasprintf("%s-%d",
                                              targetTypeEntry->parsed_name,
                                              target_id);
        nvstrtoupper(t->protoNames[proto_idx]);
    }
}



/*!
 * Adds the GPU names to the given target to the list of protocol names.
 *
 * \param[in/out]  t  The CtrlHandleTarget to load names for.
 */

static void load_gpu_target_proto_names(CtrlHandleTarget *t)
{
    load_default_target_proto_name(t, NV_GPU_PROTO_NAME_TYPE_ID);

    t->protoNames[NV_GPU_PROTO_NAME_UUID] =
        query_x_name(t, NV_CTRL_STRING_GPU_UUID);
}



/*!
 * Adds the all the appropriate names for the given target to the list of
 * protocol names.
 *
 * \param[in/out]  t  The CtrlHandleTarget to load names for.
 */

static void load_target_proto_names(CtrlHandleTarget *t)
{
    const int target_type = NvCtrlGetTargetType(t->h);

    switch (target_type) {
    case NV_CTRL_TARGET_TYPE_DISPLAY:
        load_display_target_proto_names(t);
        break;

    case NV_CTRL_TARGET_TYPE_GPU:
        load_gpu_target_proto_names(t);
        break;

    default:
        load_default_target_proto_name(t, 0);
        break;
    }
}



/*!
 * Returns the CtrlHandleTarget from CtrlHandles with the given target type/
 * target id.
 *
 * \param[in]  handles      Container for all the CtrlHandleTargets to search.
 * \param[in]  target_type  The target type of the CtrlHandleTarget to search.
 * \param[in]  target_id    The target id of the CtrlHandleTarget to search.
 *
 * \return  Returns the matching CtrlHandleTarget from CtrlHandles on success;
 *          else, returns NULL.
 */

static CtrlHandleTarget *nv_get_target(const CtrlHandles *handles,
                                       int target_type,
                                       int target_id)
{
    const CtrlHandleTargets *targets;
    int i;

    if (target_type < 0 || target_type >= MAX_TARGET_TYPES) {
        return NULL;
    }

    targets = handles->targets + target_type;
    for (i = 0; i < targets->n; i++) {
        CtrlHandleTarget *target = targets->t + i;
        if (NvCtrlGetTargetId(target->h) == target_id) {
            return target;
        }
    }

    return NULL;
}



/*!
 * Returns the NvCtrlAttributeHandle from CtrlHandles with the given target
 * type/target id.
 *
 * \param[in]  handles      Container for all the CtrlHandleTargets to search.
 * \param[in]  target_type  The target type of the CtrlHandleTarget to search.
 * \param[in]  target_id    The target id of the CtrlHandleTarget to search.
 *
 * \return  Returns the NvCtrlAttributeHandle from the matching
 *          CtrlHandleTarget from CtrlHandles on success; else, returns NULL.
 */

NvCtrlAttributeHandle *nv_get_target_handle(const CtrlHandles *handles,
                                            int target_type,
                                            int target_id)
{
    CtrlHandleTarget *target;

    target = nv_get_target(handles, target_type, target_id);
    if (target) {
        return target->h;
    }

    return NULL;
}



/*!
 * Returns the first node (sentry) for tracking CtrlHandleTarget lists.
 *
 * \return  Returns the sentry node to be used for tracking CtrlHandleTarget
 *          list.
 */

static CtrlHandleTargetNode *nv_target_list_init(void)
{
    return nvalloc(sizeof(CtrlHandleTargetNode));
}



/*!
 * Appends the given CtrlHandleTarget 'target' to the end of the
 * CtrlHandleTarget list 'head' if 'target' is not already in the list.
 *
 * \param[in/out]  head    The first node in the CtrlHandleTarget list, to which
 *                         target should be inserted.
 * \param[in]      target  The CtrlHandleTarget to add to the list.
 */

static void nv_target_list_add(CtrlHandleTargetNode *head,
                               CtrlHandleTarget *target)
{
    CtrlHandleTargetNode *t;

    for (t = head; t->next; t = t->next) {
        if (t->t == target) {
            /* Already in list, ignore */
            return;
        }
    }

    t->next = nv_target_list_init();
    t->t = target;
}



/*!
 * Frees the memory used for tracking a list of CtrlHandleTargets.
 *
 * \param[in\out]  head  The first node in the CtrlHandleTarget list that is to
 *                       be freed.
 */

void nv_target_list_free(CtrlHandleTargetNode *head)
{
    CtrlHandleTargetNode *n;

    while (head) {
        n = head->next;
        free(head);
        head = n;
    }
}



/*!
 * Adds all the targets (of target type 'targetType') that are both present
 * in the CtrlHandle, and known to be associated to 't' by querying the list
 * of associated targets to 't' from NV-CONTROL via binary attribute 'attr',
 * and possibly reciprocally adding the reverse relationship.
 *
 * \param[in\out]  h                    The list of all managed targets from
 *                                      which associations are to be made to/
 *                                      from.
 * \param[in\out]  t                    The target to which association(s) are
 *                                      being made.
 * \param[in]      targetType           The target type of the associated
 *                                      targets being considered (queried.)
 * \param[in]      attr                 The NV-CONTROL binary attribute that
 *                                      should be queried to retrieve the list
 *                                      of 'targetType' targets that are
 *                                      associated to the (CtrlHandleTarget)
 *                                      target 't'.
 * \param[in]      implicit_reciprocal  Whether or not to reciprocally add the
 *                                      reverse relationship to the matching
 *                                      targets.
 */

static void add_target_relationships(const CtrlHandles *h, CtrlHandleTarget *t,
                                     int targetType, int attr,
                                     int implicit_reciprocal)
{
    ReturnStatus status;
    int *pData;
    int len;
    int i;

    status =
        NvCtrlGetBinaryAttribute(t->h, 0,
                                 attr,
                                 (unsigned char **)(&pData), &len);
    if ((status != NvCtrlSuccess) || !pData) {
        nv_error_msg("Error querying target relations");
        return;
    }

    for (i = 0; i < pData[0]; i++) {
        int targetId = pData[i+1];
        CtrlHandleTarget *r;

        r = nv_get_target(h, targetType, targetId);
        if (r) {
            nv_target_list_add(t->relations, r);

            if (implicit_reciprocal == NV_TRUE) {
                nv_target_list_add(r->relations, t);
            }
        }
    }

    XFree(pData);
}



/*!
 * Adds all associations to/from an X screen target.
 *
 * \param[in\out]  h  The list of all managed targets from which associations
 *                    are to be made to/from.
 * \param[in\out]  t  The X screen target to which association(s) are being
 *                    made.
 */

static void load_screen_target_relationships(CtrlHandles *h,
                                             CtrlHandleTarget *t)
{
    add_target_relationships(h, t, NV_CTRL_TARGET_TYPE_GPU,
                             NV_CTRL_BINARY_DATA_GPUS_USED_BY_LOGICAL_XSCREEN,
                             NV_TRUE);
    add_target_relationships(h, t, NV_CTRL_TARGET_TYPE_DISPLAY,
                             NV_CTRL_BINARY_DATA_DISPLAYS_ASSIGNED_TO_XSCREEN,
                             NV_TRUE);
}



/*!
 * Adds all associations to/from a GPU target.
 *
 * \param[in\out]  h  The list of all managed targets from which associations
 *                    are to be made to/from.
 * \param[in\out]  t  The GPU target to which association(s) are being made.
 */

static void load_gpu_target_relationships(CtrlHandles *h, CtrlHandleTarget *t)
{
    add_target_relationships(h, t, NV_CTRL_TARGET_TYPE_FRAMELOCK,
                             NV_CTRL_BINARY_DATA_FRAMELOCKS_USED_BY_GPU,
                             NV_FALSE);
    add_target_relationships(h, t, NV_CTRL_TARGET_TYPE_VCSC,
                             NV_CTRL_BINARY_DATA_VCSCS_USED_BY_GPU,
                             NV_FALSE);
    add_target_relationships(h, t, NV_CTRL_TARGET_TYPE_COOLER,
                             NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
                             NV_TRUE);
    add_target_relationships(h, t, NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,
                             NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU,
                             NV_TRUE);
    add_target_relationships(h, t, NV_CTRL_TARGET_TYPE_DISPLAY,
                             NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU,
                             NV_TRUE);
}



/*!
 * Adds all associations to/from a FrameLock target.
 *
 * \param[in\out]  h  The list of all managed targets from which associations
 *                    are to be made to/from.
 * \param[in\out]  t  The FrameLock target to which association(s) are being
 *                    made.
 */

static void load_framelock_target_relationships(CtrlHandles *h,
                                                CtrlHandleTarget *t)
{
    add_target_relationships(h, t, NV_CTRL_TARGET_TYPE_GPU,
                             NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK,
                             NV_FALSE);
}



/*!
 * Adds all associations to/from a VCS target.
 *
 * \param[in\out]  h  The list of all managed targets from which associations
 *                    are to be made to/from.
 * \param[in\out]  t  The VCS target to which association(s) are being made.
 */

static void load_vcs_target_relationships(CtrlHandles *h, CtrlHandleTarget *t)
{
    add_target_relationships(h, t, NV_CTRL_TARGET_TYPE_GPU,
                             NV_CTRL_BINARY_DATA_GPUS_USING_VCSC,
                             NV_FALSE);
}



/*!
 * Adds all associations to/from a target.
 *
 * \param[in\out]  h  The list of all managed targets from which associations
 *                    are to be made to/from.
 * \param[in\out]  t  The target to which association(s) are being made.
 */

static void load_target_relationships(CtrlHandles *h, CtrlHandleTarget *t)
{
    int target_type = NvCtrlGetTargetType(t->h);

    switch (target_type) {
    case NV_CTRL_TARGET_TYPE_X_SCREEN:
        load_screen_target_relationships(h, t);
        break;

    case NV_CTRL_TARGET_TYPE_GPU:
        load_gpu_target_relationships(h, t);
        break;

    case NV_CTRL_TARGET_TYPE_FRAMELOCK:
        load_framelock_target_relationships(h, t);
        break;

    case NV_CTRL_TARGET_TYPE_VCSC:
        load_vcs_target_relationships(h, t);
        break;

    default:
        break;
    }
}



/*!
 * Determines if the target 't' has the name 'name'.
 *
 * \param[in]  t     The target being considered.
 * \param[in]  name  The name to match against.
 *
 * \return  Returns NV_TRUE if the given target 't' has the name 'name'; else
 *          returns NV_FALSE.
 */

static int nv_target_has_name(const CtrlHandleTarget *t, const char *name)
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
 * target name.
 *
 * \param[in]  t     The target being considered.
 * \param[in]  name  The name to match against.
 *
 * \return  Returns NV_TRUE if the given target 't' has the name 'name'; else
 *          returns NV_FALSE.
 */

static int target_has_qualification(const CtrlHandleTarget *t,
                                    int matchTargetType,
                                    int matchTargetId,
                                    const char *matchTargetName)
{
    const CtrlHandleTargetNode *n;

    /* If no qualifications given, all targets match */
    if ((matchTargetType < 0) && (matchTargetId < 0) && (!matchTargetName)) {
        return NV_TRUE;
    }

    /* Look for any matching relationship */
    for (n = t->relations;
         n->next;
         n = n->next) {
        const CtrlHandleTarget *r = n->t;

        if (matchTargetType >= 0 &&
            (matchTargetType != NvCtrlGetTargetType(r->h))) {
            continue;
        }

        if ((matchTargetId >= 0) &&
            matchTargetId != NvCtrlGetTargetId(r->h)) {
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
 * \param[in]  sAAA        If sBBB is NULL, this is either the target name,
 *                         or a target type.
 * \param[in]  sBBB        If not NULL, this is the target type as a string.
 * \param[out] targetType  Assigned the target type, or -1 for all target types.
 * \param[out] targetId    Assigned the target id, or -1 for all targets.
 * \param[out] targetName  Assigned the target name, or NULL for all target
 *                         names.
 *
 * \return  Returns NV_PARSER_STATUS_SUCCESS if sAAA and sBBB were successfully
 *          parsed into at target specification; else, returns
 *          NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET if a parsing failure
 *          occurred.
 */

static int parse_single_target_specification(const char *sAAA,
                                             const char *sBBB,
                                             int *targetType,
                                             int *targetId,
                                             const char **targetName)
{
    const TargetTypeEntry *targetTypeEntry;

    *targetType = -1;   // Match all target types
    *targetId = -1;     // Match all target ids
    *targetName = NULL; // Match all target names

    if (sBBB) {

        /* If BBB is specified, then it must name a target type */
        targetTypeEntry = nv_get_target_type_entry_by_name(sBBB);
        if (!targetTypeEntry) {
            return NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET;
        }
        *targetType = targetTypeEntry->nvctrl;

        /* AAA can either be a name, or a target id */
        if (!nv_parse_numerical(sAAA, NULL, targetId)) {
            *targetName = sAAA;
        }

    } else {

        /* If BBB is unspecified, then AAA could possibly name a target type */
        targetTypeEntry = nv_get_target_type_entry_by_name(sAAA);
        if (targetTypeEntry) {
            *targetType = targetTypeEntry->nvctrl;
        } else {
            /* If not, then AAA is a target name */
            *targetName = sAAA;
        }
    }

    return NV_PARSER_STATUS_SUCCESS;
}




/*!
 * Computes the list of targets from the list of CtrlHandles 'h' that match the
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
 * \param[in/ou]  a  The ParsedAttribute whose target specification string
 *                   should be analyzed and converted into a list of targets.
 * \param[in]     h  The list of targets to choose from.
 *
 * \return  Returns NV_PARSER_STATUS_SUCCESS if the ParsedAttribute's target
 *          specification string was successfully parsed into a list of targets
 *          (though this list could be empty, if no targets are found to
 *          match!); else, returns one of the other NV_PARSER_STATUS_XXX
 *          error codes that detail the particular parsing error.
 */

static int nv_infer_targets_from_specification(ParsedAttribute *a,
                                               CtrlHandles *h)
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
    char *tmp;

    int i, j;

    int matchTargetType;
    int matchTargetId;
    const char *matchTargetName;

    int matchQualifierTargetType;
    int matchQualifierTargetId;
    const char *matchQualifierTargetName;


    tmp = nvstrdup(a->target_specification);

    /* Parse for 'YYY.XXX' or 'XXX' */
    s = strchr(tmp, '.');
    if (s) {
        *s = '\0';
        sXXX = s+1;
        sYYY = tmp;
    } else {
        sXXX = tmp;
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
                                            &matchTargetType,
                                            &matchTargetId,
                                            &matchTargetName);
    if (ret != NV_PARSER_STATUS_SUCCESS) {
        goto done;
    }

    /* Get target qualifier matching criteria */

    ret = parse_single_target_specification(sCCC, sDDD,
                                            &matchQualifierTargetType,
                                            &matchQualifierTargetId,
                                            &matchQualifierTargetName);
    if (ret != NV_PARSER_STATUS_SUCCESS) {
        goto done;
    }

    /* Iterate over the target types */
    for (i = 0; i < targetTypeTableLen; i++) {
        const TargetTypeEntry *targetTypeEntry = &(targetTypeTable[i]);
        CtrlHandleTargets *hts;

        if (matchTargetType >= 0 &&
            (matchTargetType != targetTypeEntry->nvctrl)) {
            continue;
        }

        /* For each target of this type, match the id and/or name */
        hts = &(h->targets[targetTypeEntry->target_index]);
        for (j = 0; j < hts->n; j++) {
            CtrlHandleTarget *t = &(hts->t[j]);

            if ((matchTargetId >= 0) &&
                matchTargetId != NvCtrlGetTargetId(t->h)) {
                continue;
            }
            if (matchTargetName &&
                !nv_target_has_name(t, matchTargetName)) {
                continue;
            }

            if (!target_has_qualification(t,
                                          matchQualifierTargetType,
                                          matchQualifierTargetId,
                                          matchQualifierTargetName)) {
                continue;
            }

            /* Target matches, add it to the list */
            nv_target_list_add(a->targets, t);
            a->flags |= NV_PARSER_HAS_TARGET;
        }
    }

 done:
    if (tmp) {
        free(tmp);
    }
    return ret;
}



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
    int i, j, val, d, c, len;
    char *tmp;
    int *pData = NULL;

    /* allocate the CtrlHandles struct */

    h = nvalloc(sizeof(CtrlHandles));

    /* store any given X display name */

    if (display) {
        h->display = strdup(display);
    } else {
        h->display = NULL;
    }

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

    for (j = 0; j < targetTypeTableLen; j++) {
        const TargetTypeEntry *targetTypeEntry = &targetTypeTable[j];
        int target = targetTypeEntry->target_index;
        CtrlHandleTargets *targets = &(h->targets[target]);

        /*
         * get the number of targets of this type; if this is an X
         * screen target, just use Xlib's ScreenCount() (note: to
         * support Xinerama: we'll want to use
         * NvCtrlQueryTargetCount() rather than ScreenCount()); for
         * other target types, use NvCtrlQueryTargetCount().
         */

        if (target == X_SCREEN_TARGET) {

            targets->n = ScreenCount(h->dpy);

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
                    ((major > targetTypeEntry->major) ||
                     ((major == targetTypeEntry->major) &&
                      (minor >= targetTypeEntry->minor)))) {

                    if (target != DISPLAY_TARGET) {
                        status = NvCtrlQueryTargetCount
                            (pQueryHandle, targetTypeEntry->nvctrl,
                             &val);
                    } else {
                        /* For targets that aren't simply enumerated,
                         * query the list of valid IDs in pData which
                         * will be used below
                         */
                        status =
                            NvCtrlGetBinaryAttribute(pQueryHandle, 0,
                                                     NV_CTRL_BINARY_DATA_DISPLAY_TARGETS,
                                                     (unsigned char **)(&pData), &len);
                        if (status == NvCtrlSuccess) {
                            val = pData[0];
                        }
                    }
                } else {
                    status = NvCtrlMissingExtension;
                }
            } else {
                status = NvCtrlMissingExtension;
            }
            
            if (status != NvCtrlSuccess) {
                nv_warning_msg("Unable to determine number of NVIDIA "
                             "%ss on '%s'.",
                             targetTypeEntry->name,
                             XDisplayName(h->display));
                val = 0;
            }

            targets->n = val;
        }

        /* if there are no targets of this type, skip */

        if (targets->n == 0) {
            goto next_target_type;
        }

        /* allocate an array of CtrlHandleTarget's */

        targets->t = nvalloc(targets->n * sizeof(CtrlHandleTarget));

        /*
         * loop over all the targets of this type and setup the
         * CtrlHandleTarget's
         */

        for (i = 0; i < targets->n; i++) {
            CtrlHandleTarget *t = &(targets->t[i]);
            int targetId;

            switch (target) {
            case DISPLAY_TARGET:
                /* Grab the target Id from the pData list */
                targetId = pData[i+1];
                break;
            case X_SCREEN_TARGET:
            case GPU_TARGET:
            case FRAMELOCK_TARGET:
            case VCS_TARGET:
            case GVI_TARGET:
            case COOLER_TARGET:
            case THERMAL_SENSOR_TARGET:
            case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
            default:
                targetId = i;
            }

            /* allocate the handle */

            handle = NvCtrlAttributeInit(h->dpy,
                                         targetTypeEntry->nvctrl, targetId,
                                         NV_CTRL_ATTRIBUTES_ALL_SUBSYSTEMS);

            t->h = handle;

            /*
             * silently fail: this might happen if not all X screens
             * are NVIDIA X screens
             */

            if (!handle) {
                continue;
            }

            /*
             * get a name for this target; in the case of
             * X_SCREEN_TARGET targets, just use the string returned
             * from NvCtrlGetDisplayName(); for other target types,
             * append a target specification.
             */
            
            tmp = NvCtrlGetDisplayName(handle);
            
            if (target == X_SCREEN_TARGET) {
                t->name = tmp;
            } else {
                len = strlen(tmp) + strlen(targetTypeEntry->parsed_name) +16;
                t->name = nvalloc(len);

                if (t->name) {
                    snprintf(t->name, len, "%s[%s:%d]",
                             tmp, targetTypeEntry->parsed_name, targetId);
                    free(tmp);
                } else {
                    t->name = tmp;
                }
            }

            load_target_proto_names(t);
            t->relations = nv_target_list_init();


            /*
             * get the enabled display device mask; for X screens and
             * GPUs we query NV-CONTROL; for anything else
             * (framelock), we just assign this to 0.
             */

            if (targetTypeEntry->uses_display_devices) {
                
                status = NvCtrlGetAttribute(handle,
                                            NV_CTRL_ENABLED_DISPLAYS, &d);
        
                if (status != NvCtrlSuccess) {
                    nv_error_msg("Error querying enabled displays on "
                                 "%s %d (%s).", targetTypeEntry->name, i,
                                 NvCtrlAttributesStrError(status));
                    d = 0;
                }
                
                status = NvCtrlGetAttribute(handle,
                                            NV_CTRL_CONNECTED_DISPLAYS, &c);
        
                if (status != NvCtrlSuccess) {
                    nv_error_msg("Error querying connected displays on "
                                 "%s %d (%s).", targetTypeEntry->name, i,
                                 NvCtrlAttributesStrError(status));
                    c = 0;
                }
            } else {
                d = 0;
                c = 0;
            }

            t->d = d;
            t->c = c;

            /*
             * store this handle so that we can use it to query other
             * target counts later
             */
            
            if (!pQueryHandle) pQueryHandle = handle;
        }

    next_target_type:
        if (pData) {
            XFree(pData);
            pData = NULL;
        }
    }


    /* Load relationships to other targets */

    for (i = 0; i < MAX_TARGET_TYPES; i++) {
        CtrlHandleTargets *targets = &(h->targets[i]);
        for (j = 0; j < targets->n; j++) {
            CtrlHandleTarget *t = &(targets->t[j]);
            load_target_relationships(h, t);
        }
    }

    return h;

} /* nv_alloc_ctrl_handles() */


/*
 * nv_alloc_ctrl_handles_and_add_to_array() - if it doesn't exist, allocate a new 
 * CtrlHandles structure via nv_alloc_ctrl_handles and add it to the 
 * CtrlHandlesArray given and return the newly allocated handle. If it
 * does exist, simply return the existing handle.
 */

CtrlHandles *
    nv_alloc_ctrl_handles_and_add_to_array(const char *display, 
                                           CtrlHandlesArray *handles_array)
{
    CtrlHandles *handle = nv_get_ctrl_handles(display, handles_array);

    if (handle == NULL) {
        handle = nv_alloc_ctrl_handles(display);

        if (handle) {
            handles_array->array = nvrealloc(handles_array->array, 
                                             sizeof(CtrlHandles *)  
                                             * (handles_array->n + 1));
            handles_array->array[handles_array->n] = handle;
            handles_array->n++;
        }
    }       
    
    return handle;
}



/*
 * nv_free_ctrl_handles() - free the CtrlHandles structure allocated
 * by nv_alloc_ctrl_handles()
 */

static void nv_free_ctrl_handles(CtrlHandles *h)
{
    if (!h) return;

    if (h->display) free(h->display);

    if (h->dpy) {
        int i;

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

        for (i = 0; i < targetTypeTableLen; i++) {

            CtrlHandleTargets *targets =
                &(h->targets[targetTypeTable[i].target_index]);
            int j;

            for (j = 0; j < targets->n; j++) {
                CtrlHandleTarget *t = &(targets->t[j]);
                int n;

                NvCtrlAttributeClose(t->h);
                free(t->name);
                for (n = 0; n < NV_PROTO_NAME_MAX; n++) {
                    if (t->protoNames[n]) {
                        XFree(t->protoNames[n]);
                    }
                }

                memset(t, 0, sizeof(*t));
            }
            free(targets->t);
        }
    }
    free(h);

} /* nv_free_ctrl_handles() */


/*
 * nv_free_ctrl_handles_array() - free an array of CtrlHandles. 
 */

void nv_free_ctrl_handles_array(CtrlHandlesArray *handles_array)
{
    int i;

    for (i = 0; i < handles_array->n; i++) {
        nv_free_ctrl_handles(handles_array->array[i]);
    }

    if (handles_array->array) {
        free(handles_array->array);
    }

    handles_array->n = 0;
    handles_array->array = NULL;
}    


/*
 * nv_get_ctrl_handles() - return the CtrlHandles matching the given string.
 */
CtrlHandles *nv_get_ctrl_handles(const char *display, 
                                 CtrlHandlesArray *handles_array)
{
    int i;

    for (i=0; i < handles_array->n; i++) {
        if (nv_strcasecmp(display, handles_array->array[i]->display)) {
            return handles_array->array[i];
        }
    }

    return NULL;
}


/*!
 * Adds all the targets of the target type (specified via a target type index)
 * to the list of targets to process for the ParsedAttribute.
 *
 * \param[in/out]  a          The ParsedAttribute to add targets to.
 * \param[in]      h          The list of targets to add from.
 * \param[in]      targetIdx  The target type index of the targets to add.
 */

static void include_target_idx_targets(ParsedAttribute *a, const CtrlHandles *h,
                                       int targetIdx)
{
    const CtrlHandleTargets *targets = &(h->targets[targetIdx]);
    int i;

    for (i = 0; i < targets->n; i++) {
        CtrlHandleTarget *target = &(targets->t[i]);
        nv_target_list_add(a->targets, target);
        a->flags |= NV_PARSER_HAS_TARGET;
    }
}


/*!
 * Queries the permissions for the given attribute.
 * \param[in]   a      The attribute to query permissions for.
 * \param[in]   h      CtrlHandles used to communicate with the X server.
 * \param[out]  perms  The permissions of the attribute.
 *
 * \return  Returns TRUE if the permissions were queried successfully; else,
 *          returns FALSE.
 */
static Bool query_attribute_perms(ParsedAttribute *a, CtrlHandles *h,
                                  NVCTRLAttributePermissionsRec *perms)
{
    memset(perms, 0, sizeof(*perms));

    if (a->flags & NV_PARSER_TYPE_COLOR_ATTRIBUTE) {
        /* Allow non NV-CONTROL attributes to be read/written on X screen
         * targets
         */
        perms->type = ATTRIBUTE_TYPE_INTEGER;
        perms->permissions =
            ATTRIBUTE_TYPE_READ |
            ATTRIBUTE_TYPE_WRITE |
            ATTRIBUTE_TYPE_X_SCREEN;

        return NV_TRUE;
    }

    if (a->flags & NV_PARSER_TYPE_STRING_ATTRIBUTE) {
        return XNVCTRLQueryStringAttributePermissions(h->dpy, a->attr, perms);
    }

    return XNVCTRLQueryAttributePermissions(h->dpy, a->attr, perms);
}



/*!
 * Converts the ParsedAttribute 'a''s target specification (and/or target type
 * + id) into a list of CtrlHandleTarget to operate on.  If the ParsedAttribute
 * has a target specification set, this is used to generate the list; Otherwise,
 * the target type and target id are used.  If nothing is specified, all the
 * valid targets for the attribute are included.
 *
 * \param[in/out]  a  ParsedAttribute to resolve.
 * \param[in]      h  CtrlHandles to resolve the target specification against.
 *
 * \return  Return NV_PARSER_STATUS_SUCCESS if the attribute's target
 *          specification was successfully parsed into a list of targets to
 *          operate on; else, returns one of the other NV_PARSER_STATUS_XXX
 *          error codes that detail the particular parsing error.
 */

static int resolve_attribute_targets(ParsedAttribute *a, CtrlHandles *h)
{
    NVCTRLAttributePermissionsRec perms;
    Bool status;
    int ret = NV_PARSER_STATUS_SUCCESS;
    int i;

    if (a->targets) {
        // Oops already parsed?
        // XXX thrown another error here?
        return NV_PARSER_STATUS_BAD_ARGUMENT;
    }

    a->targets = nv_target_list_init();



    /* If a target specification string was given, use that to determine the
     * list of targets to include.
     */
    if (a->target_specification) {
        ret = nv_infer_targets_from_specification(a, h);
        goto done;
    }


    /* If the target type and target id was given, use that. */
    if (a->target_type >= 0 && a->target_id >= 0) {
        CtrlHandleTarget *target = nv_get_target(h, a->target_type,
                                                 a->target_id);
        if (!target) {
            return NV_PARSER_STATUS_TARGET_SPEC_NO_TARGETS;
        }

        nv_target_list_add(a->targets, target);
        a->flags |= NV_PARSER_HAS_TARGET;
        goto done;
    }


    /* If a target type was given, but no target id, process all the targets
     * of that type.
     */
    if (a->target_type >= 0) {
        const TargetTypeEntry *targetTypeEntry =
            nv_get_target_type_entry_by_nvctrl(a->target_type);

        if (!targetTypeEntry) {
            return NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET;
        }

        include_target_idx_targets(a, h, targetTypeEntry->target_index);
        goto done;
    }


    /* If no target type was given, assume all the appropriate targets for the
     * attribute by querying its permissions.
     */
    status = query_attribute_perms(a, h, &perms);
    if (!status) {
        // XXX Throw other error here...?
        return NV_PARSER_STATUS_TARGET_SPEC_NO_TARGETS;
    }

    for (i = 0; i < MAX_TARGET_TYPES; i++) {
        int permBit = targetTypeTable[i].permission_bit;

        if (!(perms.permissions & permBit)) {
            continue;
        }

        /* Add all targets of type that are valid for this attribute */
        include_target_idx_targets(a, h, i);
    }


 done:
    /* Make sure at least one target was resolved */
    if (ret == NV_PARSER_STATUS_SUCCESS) {
        if (!(a->flags & NV_PARSER_HAS_TARGET)) {
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

static int process_attribute_queries(int num, char **queries,
                                     const char *display_name,
                                     CtrlHandlesArray *handles_array)
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
            query_all(display_name, handles_array);
            continue;
        }

        /* special case the target type queries */
        
        if (nv_strcasecmp(queries[query], "screens") ||
            nv_strcasecmp(queries[query], "xscreens")) {
            query_all_targets(display_name, X_SCREEN_TARGET, handles_array);
            continue;
        }
        
        if (nv_strcasecmp(queries[query], "gpus")) {
            query_all_targets(display_name, GPU_TARGET, handles_array);
            continue;
        }

        if (nv_strcasecmp(queries[query], "framelocks")) {
            query_all_targets(display_name, FRAMELOCK_TARGET, handles_array);
            continue;
        }

        if (nv_strcasecmp(queries[query], "vcs")) {
            query_all_targets(display_name, VCS_TARGET, handles_array);
            continue;
        }

        if (nv_strcasecmp(queries[query], "gvis")) {
            query_all_targets(display_name, GVI_TARGET, handles_array);
            continue;
        }

        if (nv_strcasecmp(queries[query], "fans")) {
            query_all_targets(display_name, COOLER_TARGET, handles_array);
            continue;
        }

        if (nv_strcasecmp(queries[query], "thermalsensors")) {
            query_all_targets(display_name, THERMAL_SENSOR_TARGET, handles_array);
            continue;
        }

        if (nv_strcasecmp(queries[query], "svps")) {
            query_all_targets(display_name, 
                              NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET, 
                              handles_array);
            continue;
        }

        if (nv_strcasecmp(queries[query], "dpys")) {
            query_all_targets(display_name, DISPLAY_TARGET, handles_array);
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

        h = nv_alloc_ctrl_handles_and_add_to_array(a.display, handles_array);
        if (!h) {
            goto done;
        }

        /* call the processing engine to process the parsed query */

        ret = nv_process_parsed_attribute(&a, h, NV_FALSE, NV_FALSE,
                                          "in query '%s'", queries[query]);
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
 */

static int process_attribute_assignments(int num, char **assignments,
                                         const char *display_name,
                                         CtrlHandlesArray *handles_array)
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

        h = nv_alloc_ctrl_handles_and_add_to_array(a.display, handles_array);
        if (!h) {
            goto done;
        }

        /* call the processing engine to process the parsed assignment */

        ret = nv_process_parsed_attribute(&a, h, NV_TRUE, NV_TRUE,
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
 * validate_value() - query the valid values for the specified
 * attribute, and check that the value to be assigned is valid.
 */

static int validate_value(CtrlHandleTarget *t, ParsedAttribute *a, uint32 d,
                          int target_type, char *whence)
{
    int bad_val = NV_FALSE;
    NVCTRLAttributeValidValuesRec valid;
    ReturnStatus status;
    char d_str[256];
    char *tmp_d_str;
    const TargetTypeEntry *targetTypeEntry;

    status = NvCtrlGetValidDisplayAttributeValues(t->h, d, a->attr, &valid);
   
    if (status != NvCtrlSuccess) {
        nv_error_msg("Unable to query valid values for attribute %s (%s).",
                     a->name, NvCtrlAttributesStrError(status));
        return NV_FALSE;
    }

    if (target_type != NV_CTRL_TARGET_TYPE_DISPLAY &&
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
        if ((a->val.i < 0) || (a->val.i > 1)) {
            bad_val = NV_TRUE;
        }
        break;
    case ATTRIBUTE_TYPE_RANGE:
        if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            if (((a->val.i >> 16) < (valid.u.range.min >> 16)) ||
                ((a->val.i >> 16) > (valid.u.range.max >> 16)) ||
                ((a->val.i & 0xffff) < (valid.u.range.min & 0xffff)) ||
                ((a->val.i & 0xffff) > (valid.u.range.max & 0xffff)))
                bad_val = NV_TRUE;
        } else {
            if ((a->val.i < valid.u.range.min) ||
                (a->val.i > valid.u.range.max))
                bad_val = NV_TRUE;
        }
        break;
    case ATTRIBUTE_TYPE_INT_BITS:
        if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            unsigned int u, l;

             u = (((unsigned int) a->val.i) >> 16);
             l = (a->val.i & 0xffff);

             if ((u > 15) || ((valid.u.bits.ints & (1 << u << 16)) == 0) ||
                 (l > 15) || ((valid.u.bits.ints & (1 << l)) == 0)) {
                bad_val = NV_TRUE;
            }
        } else {
            if ((a->val.i > 31) || (a->val.i < 0) ||
                ((valid.u.bits.ints & (1<<a->val.i)) == 0)) {
                bad_val = NV_TRUE;
            }
        }
        break;
    default:
        bad_val = NV_TRUE;
        break;
    }

    /* is this value available for this target type? */

    targetTypeEntry = nv_get_target_type_entry_by_nvctrl(target_type);
    if (!targetTypeEntry ||
        !(targetTypeEntry->permission_bit & valid.permissions)) {
        bad_val = NV_TRUE;
    }

    /* if the value is bad, print why */

    if (bad_val) {
        if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            nv_warning_msg("The value pair %d,%d for attribute '%s' (%s%s) "
                           "specified %s is invalid.",
                           a->val.i >> 16, a->val.i & 0xffff,
                           a->name, t->name,
                           d_str, whence);
        } else {
            nv_warning_msg("The value %d for attribute '%s' (%s%s) "
                           "specified %s is invalid.",
                           a->val.i, a->name, t->name,
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

static void print_valid_values(const char *name, int attr, uint32 flags,
                               NVCTRLAttributeValidValuesRec valid)
{
    int bit, print_bit, last, last2, n, i;
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

    for (i = 0; i < targetTypeTableLen; i++) {
        if (valid.permissions & targetTypeTable[i].permission_bit) {
            if (n > 0) c += sprintf(c, ", ");
            c += sprintf(c, "%s", targetTypeTable[i].name);
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

    if ((NvCtrlGetTargetType(t->h) != NV_CTRL_TARGET_TYPE_DISPLAY) &&
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

static int query_all(const char *display_name, CtrlHandlesArray *handles_array)
{
    int bit, entry, val, i;
    uint32 mask;
    ReturnStatus status;
    NVCTRLAttributeValidValuesRec valid;
    CtrlHandles *h;

    h = nv_alloc_ctrl_handles_and_add_to_array(display_name, handles_array);
    if (!h) {
        return NV_FALSE;
    }

#define INDENT "  "

    /*
     * Loop through all target types.
     */

    for (i = 0; i < targetTypeTableLen; i++) {
        CtrlHandleTargets *targets =
            &(h->targets[targetTypeTable[i].target_index]);
        int j;

        for (j = 0; j < targets->n; j++) {

            CtrlHandleTarget *t = &targets->t[j];

            if (!t->h) continue;

            nv_msg(NULL, "Attributes queryable via %s:", t->name);

            if (!__terse) nv_msg(NULL, "");

            for (entry = 0; attributeTable[entry].name; entry++) {
                const AttributeTableEntry *a = &attributeTable[entry];

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

                    if (targetTypeTable[i].uses_display_devices &&
                        ((t->d & mask) == 0x0) && (t->d)) continue;

                    if (a->flags & NV_PARSER_TYPE_STRING_ATTRIBUTE) {
                        char *tmp_str = NULL;

                        status = NvCtrlGetValidStringDisplayAttributeValues(t->h, mask,
                                                                            a->attr, &valid);
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

                        status = NvCtrlGetStringDisplayAttribute(t->h, mask, a->attr, &tmp_str);

                        if (status == NvCtrlAttributeNotAvailable) {
                            goto exit_bit_loop;
                        }

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

                    break; /* XXX force us out of the display device loop */

                } /* bit */

            } /* entry */

        } /* j (targets) */

    } /* i (target types) */

#undef INDENT

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
        nv_msg("        ", "%s (%s: 0x%.8X)", name, tmp_d_str, bit);
        
        free(name);
        free(tmp_d_str);
    }
    nv_msg(NULL, "");

    return NV_TRUE;

} /* print_target_display_connections() */



/*
 * get_product_name() Returns the (GPU, X screen, display device or VCS)
 * product name of the given target.
 */

static char * get_product_name(NvCtrlAttributeHandle *h, int attr)
{
    char *product_name;
    ReturnStatus status;

    status = NvCtrlGetStringAttribute(h, attr, &product_name);

    if (status != NvCtrlSuccess) return strdup("Unknown");

    return product_name;

} /* get_product_name() */



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
                    get_product_name(h->targets[target_index].t[ pData[i] ].h,
                                     NV_CTRL_STRING_PRODUCT_NAME);
                break;
                
            case VCS_TARGET:
                product_name =
                    get_product_name(h->targets[target_index].t[ pData[i] ].h,
                                     NV_CTRL_STRING_VCSC_PRODUCT_NAME);
                break;

            case DISPLAY_TARGET:
                product_name =
                    get_product_name(h->targets[target_index].t[ pData[i] ].h,
                                     NV_CTRL_STRING_DISPLAY_DEVICE_NAME);
                break;

            case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
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

static int query_all_targets(const char *display_name, const int target_index,
                             CtrlHandlesArray *handles_array)
{
    CtrlHandles *h;
    CtrlHandleTarget *t;
    char *str, *name;
    int i;
    const TargetTypeEntry *targetTypeEntry;

    targetTypeEntry = &(targetTypeTable[target_index]);

    /* create handles */

    h = nv_alloc_ctrl_handles_and_add_to_array(display_name, handles_array);
    if (!h) {
        return NV_FALSE;
    }

    /* build the standard X server name */
    
    str = nv_standardize_screen_name(XDisplayName(h->display), -2);
    
    /* warn if we don't have any of the target type */
    
    if (h->targets[target_index].n <= 0) {
        nv_warning_msg("No %ss on %s", targetTypeEntry->name, str);
        free(str);
        return NV_FALSE;
    }
    
    /* print how many of the target type we have */
    
    nv_msg(NULL, "%d %s%s on %s",
           h->targets[target_index].n,
           targetTypeEntry->name,
           (h->targets[target_index].n > 1) ? "s" : "",
           str);
    nv_msg(NULL, "");
    
    free(str);

    /* print information per target */

    for (i = 0; i < h->targets[target_index].n; i++) {
        char product_name[32];
        Bool is_x_name = NV_FALSE;
        char *x_name = NULL;

        t = &h->targets[target_index].t[i];

        str = NULL;
        if (target_index == NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET) {
            snprintf(product_name, 32, "3D Vision Pro Transceiver %d", i);
        } else if (target_index == THERMAL_SENSOR_TARGET) {
            snprintf(product_name, 32, "Thermal Sensor %d", i);
        } else if (target_index == COOLER_TARGET) {
            snprintf(product_name, 32, "Fan %d", i);
        } else if (target_index == FRAMELOCK_TARGET) {
            snprintf(product_name, 32, "Quadro Sync %d", i);
        } else if (target_index == VCS_TARGET) {
            x_name = get_product_name(t->h, NV_CTRL_STRING_VCSC_PRODUCT_NAME);
            is_x_name = NV_TRUE;
        } else if (target_index == GVI_TARGET) {
            snprintf(product_name, 32, "SDI Input %d", i);
        } else if (target_index == DISPLAY_TARGET) {
            x_name = get_product_name(t->h, NV_CTRL_STRING_DISPLAY_DEVICE_NAME);
            is_x_name = NV_TRUE;
        } else {
            x_name = get_product_name(t->h, NV_CTRL_STRING_PRODUCT_NAME);
            is_x_name = NV_TRUE;
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

        nv_msg("    ", "[%d] %s (%s)", i, name,
               is_x_name ? x_name : product_name);
        nv_msg(NULL, "");

        if (x_name) {
            XFree(x_name);
        }

        /* Print connectivity information */
        if (__verbosity >= VERBOSITY_ALL) {
            if (targetTypeEntry->uses_display_devices) {
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
                print_target_connections
                    (h, t, NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU,
                     GPU_TARGET);
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

    if (target_type != NV_CTRL_TARGET_TYPE_DISPLAY &&
        valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {
        tmp_d_str = display_device_mask_to_display_device_name(d);
        sprintf(str, ", display device: %s", tmp_d_str);
        free(tmp_d_str);
    } else {
        str[0] = '\0';
    }

    if (assign) {
        if (a->flags & NV_PARSER_TYPE_STRING_ATTRIBUTE) {
            status = NvCtrlSetStringAttribute(t->h, a->attr, a->val.str, NULL);
        } else {

            ret = validate_value(t, a, d, target_type, whence);
            if (!ret) return NV_FALSE;

            status = NvCtrlSetDisplayAttribute(t->h, d, a->attr, a->val.i);

            if (status != NvCtrlSuccess) {
                nv_error_msg("Error assigning value %d to attribute '%s' "
                             "(%s%s) as specified %s (%s).",
                             a->val.i, a->name, t->name, str, whence,
                             NvCtrlAttributesStrError(status));
                return NV_FALSE;
            }

            if (verbose) {
                if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
                    nv_msg("  ", "Attribute '%s' (%s%s) assigned value %d,%d.",
                           a->name, t->name, str,
                           a->val.i >> 16, a->val.i & 0xffff);
                } else {
                    nv_msg("  ", "Attribute '%s' (%s%s) assigned value %d.",
                           a->name, t->name, str, a->val.i);
                }
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
            status = NvCtrlGetDisplayAttribute(t->h, d, a->attr, &a->val.i);

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
                print_queried_value(t, &valid, a->val.i, a->flags, a->name, d,
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
 * To accommodate the differences in processing needed for each of the
 * callers of this function, the parameters 'assign' and 'verbose'
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
    int bit, ret, val;
    char *whence, *tmp_d_str0, *tmp_d_str1;
    uint32 display_devices;
    ReturnStatus status;
    CtrlHandleTargetNode *n;

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

    /* Resolve any target specifications against the CtrlHandles that were
     * allocated.
     */

    ret = resolve_attribute_targets(a, h);
    if (ret != NV_PARSER_STATUS_SUCCESS) {
        nv_error_msg("Error resolving target specification '%s' "
                     "(%s), specified %s.",
                     a->target_specification ? a->target_specification : "",
                     nv_parse_strerror(ret),
                     whence);
        goto done;
    }

    /* loop over the requested targets */

    for (n = a->targets; n->next; n = n->next) {

        CtrlHandleTarget *t = n->t;
        int target_type;
        const TargetTypeEntry *targetTypeEntry;

        if (!t->h) continue; /* no handle on this target; silently skip */

        target_type = NvCtrlGetTargetType(t->h);
        targetTypeEntry = nv_get_target_type_entry_by_nvctrl(target_type);

        /* validate any specified display device mask */

        if ((a->flags & NV_PARSER_HAS_DISPLAY_DEVICE) &&
            !(a->flags & NV_PARSER_TYPE_HIJACK_DISPLAY_DEVICE)) {

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
            /* Just use the display device mask for this target */
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
             * assign a->val.f to all values in the array; a->attr will
             * tell NvCtrlSetColorAttributes() which indices in the
             * array to use
             */

            v[0] = v[1] = v[2] = a->val.f;

            status = NvCtrlSetColorAttributes(t->h, v, v, v, a->attr);

            if (status != NvCtrlSuccess) {
                nv_error_msg("Error assigning %f to attribute '%s' on %s "
                             "specified %s (%s)", a->val.f, a->name,
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
                    a->val.i = t->c;
                } else {
                    a->val.i = t->d;
                }
            }

            /*
             * if we are hotkey switching, check against all connected
             * displays; otherwise, check against the currently active
             * display devices
             */

            if (a->flags & NV_PARSER_TYPE_VALUE_IS_SWITCH_DISPLAY) {
                check_mask = t->c;
                display_device_descriptor = "connected";
            } else {
                check_mask = t->d;
                display_device_descriptor = "enabled";
            }

            if ((check_mask & a->val.i) != a->val.i) {

                tmp_d_str0 =
                    display_device_mask_to_display_device_name(a->val.i);

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

            if (!a->val.i) {
                
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
            (target_type != NV_CTRL_TARGET_TYPE_FRAMELOCK)) {
            int available;
            
            status = NvCtrlGetAttribute(t->h, NV_CTRL_FRAMELOCK, &available);
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
            target_type != NV_CTRL_TARGET_TYPE_GVI) {
            int available;

            status = NvCtrlGetAttribute(t->h, NV_CTRL_GVO_SUPPORTED,
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

        /* Special case the GVO CSC attribute */

        if (a->flags & NV_PARSER_TYPE_SDI_CSC) {
            float colorMatrix[3][3];
            float colorOffset[3];
            float colorScale[3];
            int r, c;

            if (assign) {

                /* Make sure the standard is known */
                if (!a->val.pf) {
                    nv_error_msg("The attribute '%s' specified %s cannot be "
                                 "assigned; valid values are \"ITU_601\", "
                                 "\"ITU_709\", \"ITU_177\", and \"Identity\".",
                                 a->name, whence);
                    continue;
                }

                for (r = 0; r < 3; r++) {
                    for (c = 0; c < 3; c++) {
                        colorMatrix[r][c] = a->val.pf[r*5 + c];
                    }
                    colorOffset[r] = a->val.pf[r*5 + 3];
                    colorScale[r] = a->val.pf[r*5 + 4];
                }

                status = NvCtrlSetGvoColorConversion(t->h,
                                                     colorMatrix,
                                                     colorOffset,
                                                     colorScale);
            } else {
                status = NvCtrlGetGvoColorConversion(t->h,
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


        /* loop over the display devices */

        for (bit = 0; bit < 24; bit++) {
            uint32 mask;
            NVCTRLAttributeValidValuesRec valid;

            
            mask = (1 << bit);

            if (((mask & display_devices) == 0x0) &&
                (targetTypeEntry->uses_display_devices) &&
                (t->d)) {
                continue;
            }

            /*
             * special case the display device mask in the case that it
             * was "hijacked" for something other than a display device:
             * assign mask here so that it will be passed through to
             * process_parsed_attribute_internal() unfiltered
             */

            if (a->flags & NV_PARSER_TYPE_HIJACK_DISPLAY_DEVICE) {
                mask = a->display_device_mask;
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

            ret = process_parsed_attribute_internal(t, a, mask, target_type,
                                                    assign, verbose, whence,
                                                    valid);
            if (ret == NV_FALSE) goto done;

            /*
             * if this attribute is not per-display device, or this
             * target does not know about display devices, or this target
             * does not have display devices, then once through this loop
             * is enough.
             */

            if ((!(valid.permissions & ATTRIBUTE_TYPE_DISPLAY)) ||
                (!(targetTypeEntry->uses_display_devices)) ||
                (!(t->d))) {
                break;
            }

        } /* bit */

    } /* done looping over requested targets */

    val = NV_TRUE;

 done:
    if (whence) free(whence);
    return val;

} /* nv_process_parsed_attribute() */
