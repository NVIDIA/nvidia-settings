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

#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"

#include "NVCtrlLib.h"

#include "common-utils.h"
#include "msg.h"

#include <stdlib.h>
#include <string.h>

/*
 * NvCtrlInitNvControlAttributes() - check for the NV-CONTROL
 * extension and make sure we have an adequate version.  Returns a
 * malloced and initialized NvCtrlNvControlAttributes structure if
 * successful, or NULL otherwise.
 */

NvCtrlNvControlAttributes *
NvCtrlInitNvControlAttributes (NvCtrlAttributePrivateHandle *h)
{
    NvCtrlNvControlAttributes *nv;
    int ret, major, minor, event, error;
    const CtrlTargetTypeInfo *targetTypeInfo;
    
    ret = XNVCTRLQueryExtension (h->dpy, &event, &error);
    if (ret != True) {
        nv_warning_msg("NV-CONTROL extension not found on this Display.");
        return NULL;
    }
    
    ret = XNVCTRLQueryVersion (h->dpy, &major, &minor);
    if (ret != True) {
        nv_error_msg("Failed to query NV-CONTROL extension version.");
        return NULL;
    }

    if (NV_VERSION2(major, minor) < NV_VERSION2(NV_MINMAJOR, NV_MINMINOR)) {
        nv_error_msg("NV-CONTROL extension version %d.%d is too old; "
                     "the minimum required version is %d.%d.",
                     major, minor, NV_MINMAJOR, NV_MINMINOR);
        return NULL;
    }

    if (h->target_type == X_SCREEN_TARGET) {
        ret = XNVCTRLIsNvScreen (h->dpy, h->target_id);
        if (ret != True) {
            nv_warning_msg("NV-CONTROL extension not present on screen %d "
                           "of this Display.", h->target_id);
            return NULL;
        }
    }

    nv = nvalloc(sizeof(NvCtrlNvControlAttributes));

    targetTypeInfo = NvCtrlGetTargetTypeInfo(h->target_type);
    if (targetTypeInfo == NULL) {
        nv_error_msg("Invalid or unknown target type");
        return NULL;
    }

    ret = XNVCtrlSelectTargetNotify(h->dpy,
                                    targetTypeInfo->nvctrl,
                                    h->target_id,
                                    TARGET_ATTRIBUTE_CHANGED_EVENT,
                                    True);
    if (ret != True) {
        nv_warning_msg("Unable to select attribute changed NV-CONTROL "
                       "events.");
    }

    /*
     * TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT was added in NV-CONTROL
     * 1.15
     */

    if (NV_VERSION2(major, minor) >= NV_VERSION2(1, 15)) {
        ret = XNVCtrlSelectTargetNotify(h->dpy,
                                        targetTypeInfo->nvctrl,
                                        h->target_id,
                                        TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT,
                                        True);
        if (ret != True) {
            nv_warning_msg("Unable to select attribute changed NV-CONTROL "
                           "events.");
        }
    }
    
    /*
     * TARGET_STRING_ATTRIBUTE_CHANGED_EVENT was added in NV-CONTROL
     * 1.16
     */
    
    if (NV_VERSION2(major, minor) >= NV_VERSION2(1, 16)) {
        ret = XNVCtrlSelectTargetNotify(h->dpy,
                                        targetTypeInfo->nvctrl,
                                        h->target_id,
                                        TARGET_STRING_ATTRIBUTE_CHANGED_EVENT,
                                        True);
        if (ret != True) {
            nv_warning_msg("Unable to select attribute changed NV-CONTROL string"
                           "events.");
        }
    }

    /*
     * TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT was added in NV-CONTROL
     * 1.17
     */
    if (NV_VERSION2(major, minor) >= NV_VERSION2(1, 17)) {
        ret = XNVCtrlSelectTargetNotify(h->dpy,
                                        targetTypeInfo->nvctrl,
                                        h->target_id,
                                        TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT,
                                        True);
        if (ret != True) {
            nv_warning_msg("Unable to select attribute changed NV-CONTROL binary"
                           "events.");
        }
    }

    nv->event_base = event;
    nv->error_base = error;
    nv->major_version = major;
    nv->minor_version = minor;

    return (nv);

} /* NvCtrlInitNvControlAttributes() */


ReturnStatus
NvCtrlNvControlQueryTargetCount(const NvCtrlAttributePrivateHandle *h,
                                CtrlTargetType target_type, int *val)
{
    int ret;
    const CtrlTargetTypeInfo *targetTypeInfo;

    targetTypeInfo = NvCtrlGetTargetTypeInfo(target_type);
    if (targetTypeInfo == NULL) {
        return NvCtrlBadArgument;
    }

    ret = XNVCTRLQueryTargetCount(h->dpy, targetTypeInfo->nvctrl, val);
    return (ret) ? NvCtrlSuccess : NvCtrlError;

} /* NvCtrlNvControlQueryTargetCount() */


ReturnStatus NvCtrlNvControlGetAttribute(const NvCtrlAttributePrivateHandle *h,
                                         unsigned int display_mask,
                                         int attr, int64_t *val)
{
    ReturnStatus status;
    int value_32;
    int major, minor;

    major = h->nv->major_version;
    minor = h->nv->minor_version;

    if (attr <= NV_CTRL_LAST_ATTRIBUTE) {
        const CtrlTargetTypeInfo *targetTypeInfo;
        targetTypeInfo = NvCtrlGetTargetTypeInfo(h->target_type);
        if (targetTypeInfo == NULL) {
            return NvCtrlBadHandle;
        }

        if (NV_VERSION2(major, minor) > NV_VERSION2(1, 20)) {
            status = XNVCTRLQueryTargetAttribute64(h->dpy,
                                                   targetTypeInfo->nvctrl,
                                                   h->target_id,
                                                   display_mask, attr,
                                                   val);
        } else {
            status = XNVCTRLQueryTargetAttribute(h->dpy,
                                                 targetTypeInfo->nvctrl,
                                                 h->target_id,
                                                 display_mask, attr,
                                                 &value_32);
            *val = value_32;
        }
        if (status) {
            return NvCtrlSuccess;
        } else {
            return NvCtrlAttributeNotAvailable;
        }
    }

    if ((attr >= NV_CTRL_ATTR_NV_BASE) &&
        (attr <= NV_CTRL_ATTR_NV_LAST_ATTRIBUTE)) {

        if (!h->nv) return NvCtrlMissingExtension;

        switch (attr) {
        case NV_CTRL_ATTR_NV_MAJOR_VERSION:
            *val = major;
            return NvCtrlSuccess;
        case NV_CTRL_ATTR_NV_MINOR_VERSION:
            *val = minor;
            return NvCtrlSuccess;
        }
    }

    return NvCtrlNoAttribute;
    
} /* NvCtrlNvControlGetAttribute() */


ReturnStatus NvCtrlNvControlSetAttribute (NvCtrlAttributePrivateHandle *h,
                                          unsigned int display_mask,
                                          int attr, int val)
{
    if (attr <= NV_CTRL_LAST_ATTRIBUTE) {
        Bool bRet;
        const CtrlTargetTypeInfo *targetTypeInfo;

        targetTypeInfo = NvCtrlGetTargetTypeInfo(h->target_type);
        if (targetTypeInfo == NULL) {
            return NvCtrlBadHandle;
        }

        bRet = XNVCTRLSetTargetAttributeAndGetStatus(h->dpy,
                                                     targetTypeInfo->nvctrl,
                                                     h->target_id,
                                                     display_mask, attr, val);
        if (!bRet) {
            return NvCtrlError;
        }
        return NvCtrlSuccess;
    }

    return NvCtrlNoAttribute;
}


/*
 * Helper function for converting NV-CONTROL specific permission data into
 * CtrlAttributePerms (API agnostic) permission data that the front-end can use.
 */

static void convertFromNvCtrlPermissions(CtrlAttributePerms *dst,
                                         unsigned int permissions)
{
    memset(dst, 0, sizeof(*dst));

    dst->read = (permissions & ATTRIBUTE_TYPE_READ) ? NV_TRUE : NV_FALSE;
    dst->write = (permissions & ATTRIBUTE_TYPE_WRITE) ? NV_TRUE : NV_FALSE;

    if (permissions & ATTRIBUTE_TYPE_X_SCREEN) {
        dst->valid_targets |= CTRL_TARGET_PERM_BIT(X_SCREEN_TARGET);
    }
    if (permissions & ATTRIBUTE_TYPE_DISPLAY) {
        dst->valid_targets |= CTRL_TARGET_PERM_BIT(DISPLAY_TARGET);
    }
    if (permissions & ATTRIBUTE_TYPE_GPU) {
        dst->valid_targets |= CTRL_TARGET_PERM_BIT(GPU_TARGET);
    }
    if (permissions & ATTRIBUTE_TYPE_FRAMELOCK) {
        dst->valid_targets |= CTRL_TARGET_PERM_BIT(FRAMELOCK_TARGET);
    }
    if (permissions & ATTRIBUTE_TYPE_COOLER) {
        dst->valid_targets |= CTRL_TARGET_PERM_BIT(COOLER_TARGET);
    }
    if (permissions & ATTRIBUTE_TYPE_THERMAL_SENSOR) {
        dst->valid_targets |= CTRL_TARGET_PERM_BIT(THERMAL_SENSOR_TARGET);
    }
    if (permissions & ATTRIBUTE_TYPE_3D_VISION_PRO_TRANSCEIVER) {
        dst->valid_targets |=
            CTRL_TARGET_PERM_BIT(NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET);
    }
}


ReturnStatus
NvCtrlNvControlGetAttributePerms(const NvCtrlAttributePrivateHandle *h,
                                 CtrlAttributeType attr_type, int attr,
                                 CtrlAttributePerms *perms)
{
    NVCTRLAttributePermissionsRec nvctrlPerms;

    switch (attr_type) {
        case CTRL_ATTRIBUTE_TYPE_INTEGER:
            XNVCTRLQueryAttributePermissions(h->dpy, attr, &nvctrlPerms);
            convertFromNvCtrlPermissions(perms, nvctrlPerms.permissions);
            break;

        case CTRL_ATTRIBUTE_TYPE_STRING:
            XNVCTRLQueryStringAttributePermissions(h->dpy, attr, &nvctrlPerms);
            convertFromNvCtrlPermissions(perms, nvctrlPerms.permissions);
            break;

        case CTRL_ATTRIBUTE_TYPE_BINARY_DATA:
            XNVCTRLQueryBinaryDataAttributePermissions(h->dpy, attr, &nvctrlPerms);
            convertFromNvCtrlPermissions(perms, nvctrlPerms.permissions);
            break;

        case CTRL_ATTRIBUTE_TYPE_STRING_OPERATION:
            XNVCTRLQueryStringOperationAttributePermissions(h->dpy, attr,
                                                            &nvctrlPerms);
            convertFromNvCtrlPermissions(perms, nvctrlPerms.permissions);
            break;

        default:
            return NvCtrlBadArgument;
    }

    return NvCtrlSuccess;

}


/*
 * Helper function for converting NV-CONTROL specific valid values data into
 * CtrlAttributeValidValues (API agnostic) data that the front-end can use.
 */

static void
convertFromNvCtrlValidValues(CtrlAttributeValidValues *dst,
                             const NVCTRLAttributeValidValuesRec *src)
{
    memset(dst, 0, sizeof(*dst));

    switch (src->type) {
        case ATTRIBUTE_TYPE_INTEGER:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_INTEGER;
            break;
        case ATTRIBUTE_TYPE_BITMASK:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_BITMASK;
            break;
        case ATTRIBUTE_TYPE_BOOL:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_BOOL;
            break;
        case ATTRIBUTE_TYPE_RANGE:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_RANGE;
            break;
        case ATTRIBUTE_TYPE_INT_BITS:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS;
            break;
        case ATTRIBUTE_TYPE_64BIT_INTEGER:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_64BIT_INTEGER;
            break;
        case ATTRIBUTE_TYPE_STRING:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_STRING;
            break;
        case ATTRIBUTE_TYPE_BINARY_DATA:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_BINARY_DATA;
            break;
        case ATTRIBUTE_TYPE_STRING_OPERATION:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_STRING_OPERATION;
            break;
        default:
            dst->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_UNKNOWN;
    }

    if (src->type == ATTRIBUTE_TYPE_RANGE) {
        dst->range.min = src->u.range.min;
        dst->range.max = src->u.range.max;
    }
    else if (src->type == ATTRIBUTE_TYPE_INT_BITS) {
        dst->allowed_ints = src->u.bits.ints;
    }

    convertFromNvCtrlPermissions(&(dst->permissions), src->permissions);
}

ReturnStatus
NvCtrlNvControlGetValidAttributeValues(const NvCtrlAttributePrivateHandle *h,
                                       unsigned int display_mask,
                                       int attr,
                                       CtrlAttributeValidValues *val)
{
    if (attr <= NV_CTRL_LAST_ATTRIBUTE) {
        const CtrlTargetTypeInfo *targetTypeInfo;
        NVCTRLAttributeValidValuesRec valid;

        targetTypeInfo = NvCtrlGetTargetTypeInfo(h->target_type);
        if (targetTypeInfo == NULL) {
            return NvCtrlBadHandle;
        }

        if (XNVCTRLQueryValidTargetAttributeValues(h->dpy,
                                                   targetTypeInfo->nvctrl,
                                                   h->target_id, display_mask,
                                                   attr, &valid)) {
            if (val != NULL) {
                convertFromNvCtrlValidValues(val, &valid);
            }
            return NvCtrlSuccess;
        } else {
            return NvCtrlAttributeNotAvailable;
        }
    }

    return NvCtrlNoAttribute;

} /* NvCtrlNvControlGetValidAttributeValues() */


ReturnStatus
NvCtrlNvControlGetValidStringDisplayAttributeValues
                                       (const NvCtrlAttributePrivateHandle *h,
                                        unsigned int display_mask,
                                        int attr,
                                        CtrlAttributeValidValues *val)
{
    if (attr <= NV_CTRL_STRING_LAST_ATTRIBUTE) {
        if (NV_VERSION2(h->nv->major_version, h->nv->minor_version)
            >= NV_VERSION2(1, 22)) {

            const CtrlTargetTypeInfo *targetTypeInfo;
            NVCTRLAttributeValidValuesRec valid;

            targetTypeInfo = NvCtrlGetTargetTypeInfo(h->target_type);
            if (targetTypeInfo == NULL) {
                return NvCtrlBadHandle;
            }

            if (XNVCTRLQueryValidTargetStringAttributeValues(h->dpy,
                                                             targetTypeInfo->nvctrl,
                                                             h->target_id,
                                                             display_mask,
                                                             attr, &valid)) {
                if (val != NULL) {
                    convertFromNvCtrlValidValues(val, &valid);
                }
                return NvCtrlSuccess;
            } else {
                return NvCtrlAttributeNotAvailable;
            }
        } else {
            if (val) {
                memset(val, 0, sizeof(*val));
                val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_STRING;
                val->permissions.read = NV_TRUE;
                val->permissions.valid_targets = CTRL_TARGET_PERM_BIT(X_SCREEN_TARGET);
                return NvCtrlSuccess;
            } else {
                return NvCtrlBadArgument;
            }
        }
    }

    return NvCtrlNoAttribute;

} /* NvCtrlNvControlGetValidStringDisplayAttributeValues() */


ReturnStatus
NvCtrlNvControlGetStringAttribute(const NvCtrlAttributePrivateHandle *h,
                                  unsigned int display_mask,
                                  int attr, char **ptr)
{
    /* Validate */
    if (!h || !h->dpy) {
        return NvCtrlBadHandle;
    }

    if (attr == NV_CTRL_STRING_NV_CONTROL_VERSION) {
        char str[24];
        if (h->target_type != X_SCREEN_TARGET) {
            return NvCtrlBadHandle;
        }
        sprintf(str, "%d.%d", h->nv->major_version, h->nv->minor_version);
        *ptr = strdup(str);
        return NvCtrlSuccess;
    }

    if (attr <= NV_CTRL_STRING_LAST_ATTRIBUTE) {
        char *tmp;
        const CtrlTargetTypeInfo *targetTypeInfo;

        targetTypeInfo = NvCtrlGetTargetTypeInfo(h->target_type);
        if (targetTypeInfo == NULL) {
            return NvCtrlBadHandle;
        }

        if (XNVCTRLQueryTargetStringAttribute(h->dpy,
                                              targetTypeInfo->nvctrl,
                                              h->target_id, display_mask,
                                              attr, &tmp)) {
            if (tmp) {
                *ptr = strdup(tmp);
                XFree(tmp);
            } else {
                *ptr = NULL;
            }
            return NvCtrlSuccess;
        } else {
            return NvCtrlAttributeNotAvailable;
        }
    }

    return NvCtrlNoAttribute;

} /* NvCtrlNvControlGetStringAttribute() */


ReturnStatus
NvCtrlNvControlSetStringAttribute (NvCtrlAttributePrivateHandle *h,
                                   unsigned int display_mask,
                                   int attr, const char *ptr)
{
    if (attr <= NV_CTRL_LAST_ATTRIBUTE) {
        Bool ret;

        /* NV-CONTROL 1.19 and above has support for setting string attributes
         * on targets other than X screens.
         */
        if (NV_VERSION2(h->nv->major_version, h->nv->minor_version) >=
            NV_VERSION2(1, 19)) {

            const CtrlTargetTypeInfo *targetTypeInfo;
            targetTypeInfo = NvCtrlGetTargetTypeInfo(h->target_type);
            if (targetTypeInfo == NULL) {
                return NvCtrlBadHandle;
            }

            ret = XNVCTRLSetTargetStringAttribute(h->dpy,
                                                  targetTypeInfo->nvctrl,
                                                  h->target_id, display_mask,
                                                  attr, ptr);
        } else {
            if (h->target_type != X_SCREEN_TARGET) {
                return NvCtrlBadHandle;
            }
            ret =
                XNVCTRLSetStringAttribute(h->dpy, h->target_id, display_mask,
                                          attr, ptr);
        }

        if ( ret ) {
            return NvCtrlSuccess;
        } else {
            return NvCtrlAttributeNotAvailable;
        }
    }

    return NvCtrlNoAttribute;

} /* NvCtrlNvControlSetStringAttribute() */


ReturnStatus
NvCtrlNvControlGetBinaryAttribute(const NvCtrlAttributePrivateHandle *h,
                                  unsigned int display_mask, int attr,
                                  unsigned char **data, int *len)
{
    unsigned char *tmp;
    Bool ret;
    int localLen;
    const CtrlTargetTypeInfo *targetTypeInfo;

    if (!h->nv) return NvCtrlMissingExtension;

    /* the X_nvCtrlQueryBinaryData opcode was added in 1.7 */

    if (NV_VERSION2(h->nv->major_version, h->nv->minor_version) <
        NV_VERSION2(1, 7)) {
        return NvCtrlNoAttribute;
    }

    if (len == NULL) {
        len = &localLen;
    }

    targetTypeInfo = NvCtrlGetTargetTypeInfo(h->target_type);
    if (targetTypeInfo == NULL) {
        return NvCtrlBadHandle;
    }

    ret = XNVCTRLQueryTargetBinaryData(h->dpy,
                                       targetTypeInfo->nvctrl,
                                       h->target_id,
                                       display_mask, attr, &tmp, len);
    if (!ret) {
        return NvCtrlError;
    }

    if (tmp == NULL) {
        *data = NULL;
        return NvCtrlSuccess;
    }

    *data = malloc(*len);
    if (*data == NULL) {
        *len = 0;
        XFree(tmp);
        return NvCtrlError;
    }

    memcpy(*data, tmp, *len);
    XFree(tmp);

    return NvCtrlSuccess;
}


ReturnStatus
NvCtrlNvControlStringOperation(NvCtrlAttributePrivateHandle *h,
                               unsigned int display_mask, int attr,
                               const char *ptrIn, char **ptrOut)
{
    char *tmp;
    if (attr <= NV_CTRL_STRING_OPERATION_LAST_ATTRIBUTE) {
        const CtrlTargetTypeInfo *targetTypeInfo;
        targetTypeInfo = NvCtrlGetTargetTypeInfo(h->target_type);
        if (targetTypeInfo == NULL) {
            return NvCtrlBadHandle;
        }

        if (XNVCTRLStringOperation(h->dpy, targetTypeInfo->nvctrl,
                                   h->target_id, display_mask,
                                   attr, ptrIn, &tmp)) {
            if (tmp) {
                *ptrOut = strdup(tmp);
                XFree(tmp);
            } else {
                *ptrOut = NULL;
            }
            return NvCtrlSuccess;
        } else {
            return NvCtrlAttributeNotAvailable;
        }
    }

    return NvCtrlNoAttribute;

} /* NvCtrlNvControlStringOperation() */
