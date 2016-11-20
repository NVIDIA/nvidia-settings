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

#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include "parse.h"
#include "msg.h"
#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"



/*!
 * Queries the NV-CONTROL string attribute and returns the string as a simple
 * char *.  This is useful to avoid having to track how strings are allocated
 * so we can cleanup all strings via nvfree().
 *
 * \param[in]  t     The CtrlTarget to query the string on.
 * \param[in]  attr  The NV-CONTROL string to query.
 *
 * \return  Return a nvalloc()'ed copy of the NV-CONTROL string; else, returns
 *          NULL.
 */

static char *query_x_name(const CtrlTarget *t, int attr)
{
    ReturnStatus status;
    char *str = NULL;

    status = NvCtrlGetStringAttribute(t, attr, &str);
    if (status != NvCtrlSuccess) {
        return NULL;
    }

    return str;
}



static void nv_free_ctrl_target(CtrlTarget *target)
{
    int i;

    if (!target) {
        return;
    }

    NvCtrlAttributeClose(target->h);
    target->h = NULL;

    free(target->name);
    target->name = NULL;

    for (i = 0; i < NV_PROTO_NAME_MAX; i++) {
        free(target->protoNames[i]);
        target->protoNames[i] = NULL;
    }

    NvCtrlTargetListFree(target->relations);
    target->relations = NULL;

    nvfree(target);
}



static void nv_free_ctrl_system(CtrlSystem *system)
{
    int target_type;

    if (!system) {
        return;
    }

    /* close the X connection */

    if (system->dpy) {
        /*
         * XXX It is unfortunate that the display connection needs
         * to be closed before the backends have had a chance to
         * tear down their state. If future backends need to send
         * protocol in this case or perform similar tasks, we'll
         * have to add e.g. NvCtrlAttributeTearDown(), which would
         * need to be called before XCloseDisplay().
         */
        XCloseDisplay(system->dpy);
        system->dpy = NULL;
    }

    /* cleanup targets */

    for (target_type = 0;
         target_type < MAX_TARGET_TYPES;
         target_type++) {
        while (system->targets[target_type]) {
            CtrlTargetNode *node = system->targets[target_type];

            system->targets[target_type] = node->next;

            nv_free_ctrl_target(node->t);
            nvfree(node);
        }
    }

    /* cleanup physical screens */

    while (system->physical_screens) {
        CtrlTargetNode *node = system->physical_screens;

        system->physical_screens = node->next;

        nv_free_ctrl_target(node->t);
        nvfree(node);
    }

    /* cleanup everything else */

    free(system->display);
    system->display = NULL;

    free(system);
}



void NvCtrlFreeAllSystems(CtrlSystemList *systems)
{
    int i;

    if (!systems) {
        return;
    }

    for (i = 0; i < systems->n; i++) {
        nv_free_ctrl_system(systems->array[i]);
    }

    free(systems->array);
    systems->array = NULL;
    systems->n = 0;
}



/*!
 * Retrieves and adds all the display device names for the given target.
 *
 * \param[in/out]  t  The CtrlTarget to load names for.
 */

static void load_display_target_proto_names(CtrlTarget *t)
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
 * \param[in/out]  t          The CtrlTarget to load names for.
 * \param[in]      proto_idx  The name index where to add the name.
 */

static void load_default_target_proto_name(CtrlTarget *t,
                                           const int proto_idx)
{
    if (proto_idx >= NV_PROTO_NAME_MAX) {
        return;
    }

    t->protoNames[proto_idx] = nvasprintf("%s-%d",
                                          t->targetTypeInfo->parsed_name,
                                          NvCtrlGetTargetId(t));
    nvstrtoupper(t->protoNames[proto_idx]);
}



/*!
 * Adds the GPU names to the given target to the list of protocol names.
 *
 * \param[in/out]  t  The CtrlTarget to load names for.
 */

static void load_gpu_target_proto_names(CtrlTarget *t)
{
    load_default_target_proto_name(t, NV_GPU_PROTO_NAME_TYPE_ID);

    t->protoNames[NV_GPU_PROTO_NAME_UUID] =
        query_x_name(t, NV_CTRL_STRING_GPU_UUID);
}



/*!
 * Adds the all the appropriate names for the given target to the list of
 * protocol names.
 *
 * \param[in/out]  t  The CtrlTarget to load names for.
 */

static void load_target_proto_names(CtrlTarget *t)
{
    switch (NvCtrlGetTargetType(t)) {
    case DISPLAY_TARGET:
        load_display_target_proto_names(t);
        break;

    case GPU_TARGET:
        load_gpu_target_proto_names(t);
        break;

    default:
        load_default_target_proto_name(t, 0);
        break;
    }
}



int NvCtrlGetTargetTypeCount(const CtrlSystem *system, CtrlTargetType target_type)
{
    int count = 0;
    CtrlTargetNode *node;

    if (!system || !NvCtrlIsTargetTypeValid(target_type)) {
        return 0;
    }

    for (node = system->targets[target_type]; node; node = node->next) {
        count++;
    }

    return count;
}



/*!
 * Returns the CtrlTarget from a CtrlSystem with the given target type/
 * target id.
 *
 * \param[in]  system       Container for all the CtrlTargets to search.
 * \param[in]  target_type  The target type of the CtrlTarget to search.
 * \param[in]  target_id    The target id of the CtrlTarget to search.
 *
 * \return  Returns the matching CtrlTarget from CtrlSystem on success;
 *          else, returns NULL.
 */

CtrlTarget *NvCtrlGetTarget(const CtrlSystem *system,
                            CtrlTargetType target_type,
                            int target_id)
{
    CtrlTargetNode *node;

    if (!system || !NvCtrlIsTargetTypeValid(target_type)) {
        return NULL;
    }

    for (node = system->targets[target_type];
         node;
         node = node->next) {
        CtrlTarget *target = node->t;
        if (NvCtrlGetTargetId(target) == target_id) {
            return target;
        }
    }

    return NULL;
}


/*!
 * Returns the RandR name of the matching display target from the given
 * target ID and the list of target handles.
 *
 * \param[in]  system       Container for all the CtrlTargets to search.
 * \param[in]  target_id    The target id of the display CtrlTarget to search.
 *
 * \return  Returns the NV_DPY_PROTO_NAME_RANDR name string  from the matching
 *          (display) CtrlTarget from CtrlSystem on success; else, returns NULL.
 */
const char *NvCtrlGetDisplayConfigName(const CtrlSystem *system, int target_id)
{
    CtrlTarget *target;

    target = NvCtrlGetTarget(system, DISPLAY_TARGET, target_id);
    if (!target) {
        return NULL;
    }

    return target->protoNames[NV_DPY_PROTO_NAME_RANDR];
}


/*!
 * Returns any CtrlTarget of the specified target type from the CtrlSystem that
 * can be used to communicate with the system.
 */

CtrlTarget *NvCtrlGetDefaultTargetByType(const CtrlSystem *system,
                                         CtrlTargetType target_type)
{
    CtrlTargetNode *node;

    if (!system || !NvCtrlIsTargetTypeValid(target_type)) {
        return NULL;
    }

    for (node = system->targets[target_type]; node; node = node->next) {
        CtrlTarget *target = node->t;

        if (target->h) {
            return target;
        }
    }

    return NULL;
}


/*!
 * Returns any CtrlTarget from the CtrlSystem that can be used
 * to communicate with the system.
 */

CtrlTarget *NvCtrlGetDefaultTarget(const CtrlSystem *system)
{
    int i;

    if (!system) {
        return NULL;
    }

    for (i = 0; i < MAX_TARGET_TYPES; i++) {
        CtrlTarget *target = NvCtrlGetDefaultTargetByType(system, i);

        if (target) {
            return target;
        }
    }

    return NULL;
}



/*!
 * Appends the given CtrlTarget to the end of the CtrlTarget list 'head' if
 * it is not already in the list.
 *
 * \param[in/out]  head    The first node in the CtrlTarget list, to which
 *                         target should be inserted.
 * \param[in]      target  The CtrlTarget to add to the list.
 * \param[in]      enabled_display_check  Whether or not to check that, if the
 *                                        target is a display target, it is also
 *                                        enabled.
 */

void NvCtrlTargetListAdd(CtrlTargetNode **head, CtrlTarget *target,
                         Bool enabled_display_check)
{
    CtrlTargetNode *new_t;
    CtrlTargetNode *t;

    /* Do not add disabled displays to the list */
    if (enabled_display_check) {
        if ((NvCtrlGetTargetType(target) == DISPLAY_TARGET) &&
            !target->display.enabled) {
            return;
        }
    }

    new_t = nvalloc(sizeof(*new_t));
    new_t->t = target;

    t = *head;

    /* List is empty */
    if (!t) {
        *head = new_t;
        return;
    }

    while (1) {
        if (t->t == target) {
            nvfree(new_t);
            return;
        }
        if (!t->next) {
            t->next = new_t;
            return;
        }
        t = t->next;
    }
}



/*!
 * Frees the memory used for tracking a list of CtrlTargets.
 *
 * \param[in\out]  head  The first node in the CtrlTarget list that is to be
 *                       freed.
 */

void NvCtrlTargetListFree(CtrlTargetNode *head)
{
    CtrlTargetNode *n;

    while (head) {
        n = head->next;
        free(head);
        head = n;
    }
}



/*!
 * Adds all the targets of target type relating to 'target_type' that are
 * known to be associated to 'target' by querying the list of associated targets
 * for the given attribute association 'attr'.  If implicit_reciprocal is set,
 * the relatiopship is added to the relating target(s).
 *
 * \param[in\out]  target               The target to which association(s) are
 *                                      being made.
 * \param[in]      target_type          The target type of the associated
 *                                      targets being considered (queried.)
 * \param[in]      attr                 The NV-CONTROL binary attribute that
 *                                      should be queried to retrieve the list
 *                                      of 'targetType' targets that are
 *                                      associated to the (CtrlTarget) target
 *                                      't'.
 * \param[in]      implicit_reciprocal  Whether or not to reciprocally add the
 *                                      reverse relationship to the matching
 *                                      targets.
 */

static void add_target_relationships(CtrlTarget *target,
                                     CtrlTargetType target_type, int attr,
                                     int implicit_reciprocal)
{
    ReturnStatus status;
    int *pData;
    int len;
    int i;

    status = NvCtrlGetBinaryAttribute(target, 0, attr,
                                      (unsigned char **)(&pData), &len);
    if ((status != NvCtrlSuccess) || !pData) {
        nv_error_msg("Error querying target relations");
        return;
    }

    for (i = 0; i < pData[0]; i++) {
        int target_id = pData[i+1];
        CtrlTarget *other;

        other = NvCtrlGetTarget(target->system, target_type, target_id);
        if (other) {
            NvCtrlTargetListAdd(&(target->relations), other, FALSE);

            /* Track connection state of display devices */
            if (attr == NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU) {
                other->display.connected = NV_TRUE;
            }

            if (implicit_reciprocal == NV_TRUE) {
                NvCtrlTargetListAdd(&(other->relations), target, FALSE);
            }
        }
    }

    free(pData);
}



/*!
 * Adds all associations to/from an X screen target.
 *
 * \param[in\out]  target  The X screen target to which association(s) are
 *                         being made.
 */

static void load_screen_target_relationships(CtrlTarget *target)
{
    add_target_relationships(target, GPU_TARGET,
                             NV_CTRL_BINARY_DATA_GPUS_USED_BY_LOGICAL_XSCREEN,
                             NV_TRUE);
    add_target_relationships(target, DISPLAY_TARGET,
                             NV_CTRL_BINARY_DATA_DISPLAYS_ASSIGNED_TO_XSCREEN,
                             NV_TRUE);
}



/*!
 * Adds all associations to/from a GPU target.
 *
 * \param[in\out]  target  The GPU target to which association(s) are being
 *                         made.
 */

static void load_gpu_target_relationships(CtrlTarget *target)
{
    add_target_relationships(target, FRAMELOCK_TARGET,
                             NV_CTRL_BINARY_DATA_FRAMELOCKS_USED_BY_GPU,
                             NV_FALSE);
    add_target_relationships(target, VCS_TARGET,
                             NV_CTRL_BINARY_DATA_VCSCS_USED_BY_GPU,
                             NV_FALSE);
    add_target_relationships(target, COOLER_TARGET,
                             NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
                             NV_TRUE);
    add_target_relationships(target, THERMAL_SENSOR_TARGET,
                             NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU,
                             NV_TRUE);
    add_target_relationships(target, DISPLAY_TARGET,
                             NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU,
                             NV_TRUE);
    add_target_relationships(target, DISPLAY_TARGET,
                             NV_CTRL_BINARY_DATA_DISPLAYS_ON_GPU,
                             NV_TRUE);
}



/*!
 * Adds all associations to/from a FrameLock target.
 *
 * \param[in\out]  target  The FrameLock target to which association(s) are
 *                          being made.
 */

static void load_framelock_target_relationships(CtrlTarget *target)
{
    add_target_relationships(target, GPU_TARGET,
                             NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK,
                             NV_FALSE);
}



/*!
 * Adds all associations to/from a VCS target.
 *
 * \param[in\out]  target  The VCS target to which association(s) are being
 *                         made.
 */

static void load_vcs_target_relationships(CtrlTarget *target)
{
    add_target_relationships(target, GPU_TARGET,
                             NV_CTRL_BINARY_DATA_GPUS_USING_VCSC,
                             NV_FALSE);
}



/*!
 * Adds all associations to/from a target.
 *
 * \param[in\out]  target  The target to which association(s) are being made.
 */

static void load_target_relationships(CtrlTarget *target)
{
    switch (NvCtrlGetTargetType(target)) {
    case X_SCREEN_TARGET:
        load_screen_target_relationships(target);
        break;

    case GPU_TARGET:
        load_gpu_target_relationships(target);
        break;

    case FRAMELOCK_TARGET:
        load_framelock_target_relationships(target);
        break;

    case VCS_TARGET:
        load_vcs_target_relationships(target);
        break;

    default:
        break;
    }
}



/*
 * nv_alloc_ctrl_target() - Given the Display pointer, create an attribute
 * handle and initialize the handle target.
 */

static CtrlTarget *nv_alloc_ctrl_target(CtrlSystem *system,
                                        CtrlTargetType target_type,
                                        int targetId,
                                        int subsystem)
{
    CtrlTarget *t;
    NvCtrlAttributeHandle *handle;
    ReturnStatus status;
    char *tmp;
    int len, d, c;
    const CtrlTargetTypeInfo *targetTypeInfo;


    if (!system || !NvCtrlIsTargetTypeValid(target_type)) {
        return NULL;
    }

    targetTypeInfo = NvCtrlGetTargetTypeInfo(target_type);

    /* allocate the handle */

    handle = NvCtrlAttributeInit(system, target_type, targetId, subsystem);

    /*
     * silently fail: this might happen if not all X screens
     * are NVIDIA X screens
     */

    if (!handle) {
        return NULL;
    }

    t = nvalloc(sizeof(*t));
    t->h = handle;
    t->system = system;
    t->targetTypeInfo = targetTypeInfo;

    /*
     * get a name for this target; in the case of
     * X_SCREEN_TARGET targets, just use the string returned
     * from NvCtrlGetDisplayName(); for other target types,
     * append a target specification.
     */

    tmp = NvCtrlGetDisplayName(t);

    if (target_type == X_SCREEN_TARGET) {
        t->name = tmp;
    } else {
        if (tmp == NULL) {
            tmp = strdup("");
        }

        len = strlen(tmp) + strlen(targetTypeInfo->parsed_name)
            + 16;
        t->name = nvalloc(len);

        if (t->name) {
            snprintf(t->name, len, "%s[%s:%d]",
                     tmp, targetTypeInfo->parsed_name, targetId);
            free(tmp);
        } else {
            t->name = tmp;
        }
    }

    load_target_proto_names(t);
    t->relations = NULL;

    if (target_type == DISPLAY_TARGET) {
        status = NvCtrlGetAttribute(t, NV_CTRL_DISPLAY_ENABLED, &d);
        if (status != NvCtrlSuccess) {
            nv_error_msg("Error querying enabled state of display %s %d (%s).",
                         targetTypeInfo->name, targetId,
                         NvCtrlAttributesStrError(status));
            d = NV_CTRL_DISPLAY_ENABLED_FALSE;
        }
        t->display.enabled = (d == NV_CTRL_DISPLAY_ENABLED_TRUE) ? 1 : 0;
    }


    /*
     * get the enabled display device mask; for X screens and
     * GPUs we query NV-CONTROL; for anything else
     * (framelock), we just assign this to 0.
     */

    if (targetTypeInfo->uses_display_devices) {

        status = NvCtrlGetAttribute(t,
                                    NV_CTRL_ENABLED_DISPLAYS, &d);

        if (status != NvCtrlSuccess) {
            nv_error_msg("Error querying enabled displays on "
                         "%s %d (%s).", targetTypeInfo->name,
                         targetId,
                         NvCtrlAttributesStrError(status));
            d = 0;
        }

        status = NvCtrlGetAttribute(t,
                                    NV_CTRL_CONNECTED_DISPLAYS, &c);

        if (status != NvCtrlSuccess) {
            nv_error_msg("Error querying connected displays on "
                         "%s %d (%s).", targetTypeInfo->name,
                         targetId,
                         NvCtrlAttributesStrError(status));
            c = 0;
        }
    } else {
        d = 0;
        c = 0;
    }

    t->d = d;
    t->c = c;

    return t;
}


/*
 * nv_add_target() - add a CtrlTarget of the given target type to the list of
 * Targets for the given CtrlSystem.
 */

CtrlTarget *nv_add_target(CtrlSystem *system, CtrlTargetType target_type,
                          int target_id)
{
    CtrlTarget *target;
    target = nv_alloc_ctrl_target(system, target_type, target_id,
                                  NV_CTRL_ATTRIBUTES_ALL_SUBSYSTEMS);
    if (!target) {
        return NULL;
    }

    NvCtrlTargetListAdd(&(system->targets[target_type]), target, FALSE);

    return target;
}


/*
 * Returns whether the NV-CONTROL protocol version is equal or greater than
 * 'major'.'minor'
 */

static Bool is_nvcontrol_protocol_valid(const CtrlTarget *ctrl_target,
                                        int major, int minor)
{
    ReturnStatus ret1, ret2;
    int nv_major, nv_minor;

    ret1 = NvCtrlGetAttribute(ctrl_target,
            NV_CTRL_ATTR_NV_MAJOR_VERSION,
            &nv_major);
    ret2 = NvCtrlGetAttribute(ctrl_target,
            NV_CTRL_ATTR_NV_MINOR_VERSION,
            &nv_minor);

    if ((ret1 == NvCtrlSuccess) && (ret2 == NvCtrlSuccess) &&
        ((nv_major > major) || ((nv_major == major) && (nv_minor >= minor)))) {

        return TRUE;
    }

    return FALSE;
}


static Bool load_system_info(CtrlSystem *system, const char *display)
{
    ReturnStatus status;
    CtrlTarget *xscreenQueryTarget = NULL;
    CtrlTarget *nvmlQueryTarget = NULL;
    int i, target_type, val, len, target_count;
    int *pData = NULL;
    const CtrlTargetTypeInfo *targetTypeInfo;

    if (!system) {
        return FALSE;
    }

    if (display) {
        system->display = strdup(display);
    } else {
        system->display = NULL;
    }

    /* Try to open the X display connection */
    system->dpy = XOpenDisplay(system->display);

    if (system->dpy == NULL) {
        nv_error_msg("Unable to find display on any available system");
        return FALSE;
    }

    /* Try to initialize the NVML library */
    nvmlQueryTarget = nv_alloc_ctrl_target(system, GPU_TARGET, 0,
                                   NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM |
                                   NV_CTRL_ATTRIBUTES_NVML_SUBSYSTEM);

    if (nvmlQueryTarget == NULL) {
        nv_error_msg("Unable to load info from any available system");
        return FALSE;
    }

    /*
     * loop over each target type and setup the appropriate
     * information
     */

    for (target_type = 0;
         target_type < MAX_TARGET_TYPES;
         target_type++) {
        NvCtrlAttributePrivateHandle *h = getPrivateHandle(nvmlQueryTarget);
        targetTypeInfo = NvCtrlGetTargetTypeInfo(target_type);
        target_count = 0;

        /*
         * get the number of targets of this type; if this is an X
         * screen target, just use Xlib's ScreenCount() (note: to
         * support Xinerama: we'll want to use
         * NvCtrlQueryTargetCount() rather than ScreenCount()); for
         * other target types, use NvCtrlQueryTargetCount().
         */

        if (target_type == X_SCREEN_TARGET) {
            if (system->dpy != NULL) {
                target_count = ScreenCount(system->dpy);
            }
        }
        else if ((h != NULL) && (h->nvml != NULL) &&
                 TARGET_TYPE_IS_NVML_COMPATIBLE(target_type)) {

            status = NvCtrlNvmlQueryTargetCount(nvmlQueryTarget,
                                                target_type, &val);
            if (status != NvCtrlSuccess) {
                nv_warning_msg("Unable to determine number of NVIDIA %ss",
                               targetTypeInfo->name);
                val = 0;
            }
            target_count = val;
        }
        else {

            /*
             * note: xscreenQueryTarget should be assigned below by a
             * previous iteration of this loop; depends on X screen
             * targets getting handled first
             */

            if (xscreenQueryTarget) {

                /*
                 * check that the NV-CONTROL protocol is new enough to
                 * recognize this target type
                 */

                if (is_nvcontrol_protocol_valid(xscreenQueryTarget,
                                                targetTypeInfo->major,
                                                targetTypeInfo->minor)) {

                    if (target_type != DISPLAY_TARGET) {
                        status = NvCtrlQueryTargetCount(xscreenQueryTarget,
                                                        target_type,
                                                        &val);
                    } else {
                        /* For targets that aren't simply enumerated,
                         * query the list of valid IDs in pData which
                         * will be used below
                         */
                        status =
                            NvCtrlGetBinaryAttribute(xscreenQueryTarget, 0,
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
                               targetTypeInfo->name,
                               XDisplayName(system->display));
                val = 0;
            }

            target_count = val;
        }

        /* Add all the targets of this type to the CtrlSystem */

        for (i = 0; i < target_count; i++) {
            int targetId;
            CtrlTarget *target;

            switch (target_type) {
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

            target = nv_add_target(system, target_type, targetId);

            /*
             * store this handle, if it exists, so that we can use it to
             * query other target counts later
             */

            if (!xscreenQueryTarget && (target_type == X_SCREEN_TARGET) &&
                target && target->h) {

                xscreenQueryTarget = target;
            }
        }

        free(pData);
        pData = NULL;
    }

    /*
     * setup the appropriate information for physical screens
     */

    targetTypeInfo = NvCtrlGetTargetTypeInfo(X_SCREEN_TARGET);

    if (xscreenQueryTarget) {

        /*
         * check that the NV-CONTROL protocol is new enough to
         * recognize this target type
         */

        if (is_nvcontrol_protocol_valid(xscreenQueryTarget,
                                        targetTypeInfo->major,
                                        targetTypeInfo->minor)) {

            status = NvCtrlQueryTargetCount(xscreenQueryTarget,
                                            X_SCREEN_TARGET,
                                            &val);
        } else {
            status = NvCtrlMissingExtension;
        }
    } else {
        status = NvCtrlMissingExtension;
    }

    if (status != NvCtrlSuccess) {
        nv_warning_msg("Unable to determine number of NVIDIA %ss on '%s'.",
                       targetTypeInfo->name, XDisplayName(system->display));
        val = 0;
    }

    target_count = val;

    for (i = 0; i < target_count; i++) {
        CtrlTarget *target;
        target = nv_alloc_ctrl_target(system, X_SCREEN_TARGET, i,
                                      NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM);
        if (!target) {
            continue;
        }

        NvCtrlTargetListAdd(&(system->physical_screens), target, FALSE);
    }

    /* Clean up */
    if (nvmlQueryTarget != NULL) {
        nv_free_ctrl_target(nvmlQueryTarget);
    }

    return TRUE;
}


/*
 * nv_alloc_ctrl_system() - allocate a new CtrlSystem structure, connect to the
 * system (via X server identified by display), and discover/allocate/
 * initialize all the targets (GPUs, screens, Frame Lock devices, etc) found.
 */

static CtrlSystem *nv_alloc_ctrl_system(const char *display)
{
    CtrlSystem *system;
    Bool ret;
    int i;

    system = nvalloc(sizeof(*system));

    /* Connect to the system and load target information */

    ret = load_system_info(system, display);

    if (!ret) {
        nv_free_ctrl_system(system);
        return NULL;
    }

    /* Discover target relationships */

    for (i = 0; i < MAX_TARGET_TYPES; i++) {
        CtrlTargetNode *node;
        for (node = system->targets[i]; node; node = node->next) {
            load_target_relationships(node->t);
        }
    }

    return system;

} /* nv_alloc_ctrl_system() */



/*
 * Connect to (and track) a system, returning its control handles (for                                                                                                                                   
 * configuration).  If a connection was already made, return that connection's                                                                                                                           
 * handles.                                                                                                                                                                                              
 */

CtrlSystem *NvCtrlConnectToSystem(const char *display, CtrlSystemList *systems)
{
    CtrlSystem *system = NvCtrlGetSystem(display, systems);

    if (system == NULL) {
        system = nv_alloc_ctrl_system(display);

        if (system) {
            system->system_list = systems;
            systems->array = nvrealloc(systems->array,
                                       sizeof(*(systems->array))
                                       * (systems->n + 1));
            systems->array[systems->n] = system;
            systems->n++;
        }
    }

    return system;
}


/*
 * Return the CtrlSystem matching the given string.
 */
CtrlSystem *NvCtrlGetSystem(const char *display, CtrlSystemList *systems)
{
    int i;

    for (i=0; i < systems->n; i++) {
        CtrlSystem *system = systems->array[i];

        if (nv_strcasecmp(display, system->display)) {
            return system;
        }
    }

    return NULL;
}
