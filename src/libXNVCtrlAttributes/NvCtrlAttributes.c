/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004,2012 NVIDIA Corporation.
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

#include "parse.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h> /* pow(3) */

#include <sys/utsname.h>




/*
 * This table stores the associated values for each target type.  The
 * CtrlSystem->targets[] array shadows this table where the XXX_TARGET #defines
 * can be used to index into both arrays.
 */

const CtrlTargetTypeInfo targetTypeInfoTable[] = {

    [X_SCREEN_TARGET] =
    { "X Screen",                                                    /* name */
      "screen",                                                      /* parsed_name */
      NV_CTRL_TARGET_TYPE_X_SCREEN,                                  /* nvctrl */
      CTRL_TARGET_PERM_BIT(X_SCREEN_TARGET),                         /* permission_bit */
      NV_TRUE,                                                       /* uses_display_devices */
      1, 6 },                                                        /* required major,minor protocol rev */

    [GPU_TARGET] =
    { "GPU",                                                         /* name */
      "gpu",                                                         /* parsed_name */
      NV_CTRL_TARGET_TYPE_GPU,                                       /* nvctrl */
      CTRL_TARGET_PERM_BIT(GPU_TARGET),                              /* permission_bit */
      NV_TRUE,                                                       /* uses_display_devices */
      1, 10 },                                                       /* required major,minor protocol rev */

    [FRAMELOCK_TARGET] =
    { "Frame Lock Device",                                           /* name */
      "framelock",                                                   /* parsed_name */
      NV_CTRL_TARGET_TYPE_FRAMELOCK,                                 /* nvctrl */
      CTRL_TARGET_PERM_BIT(FRAMELOCK_TARGET),                        /* permission_bit */
      NV_FALSE,                                                      /* uses_display_devices */
      1, 10 },                                                       /* required major,minor protocol rev */

    [VCS_TARGET] =
    { "VCS",                                                         /* name */
      "vcs",                                                         /* parsed_name */
      NV_CTRL_TARGET_TYPE_VCSC,                                      /* nvctrl */
      CTRL_TARGET_PERM_BIT(VCS_TARGET),                              /* permission_bit */
      NV_FALSE,                                                      /* uses_display_devices */
      1, 12 },                                                       /* required major,minor protocol rev */

    [GVI_TARGET] =
    { "SDI Input Device",                                            /* name */
      "gvi",                                                         /* parsed_name */
      NV_CTRL_TARGET_TYPE_GVI,                                       /* nvctrl */
      CTRL_TARGET_PERM_BIT(GVI_TARGET),                              /* permission_bit */
      NV_FALSE,                                                      /* uses_display_devices */
      1, 18 },                                                       /* required major,minor protocol rev */

    [COOLER_TARGET] =
    { "Fan",                                                         /* name */
      "fan",                                                         /* parsed_name */
      NV_CTRL_TARGET_TYPE_COOLER,                                    /* nvctrl */
      CTRL_TARGET_PERM_BIT(COOLER_TARGET),                           /* permission_bit */
      NV_FALSE,                                                      /* uses_display_devices */
      1, 20 },                                                       /* required major,minor protocol rev */

    [THERMAL_SENSOR_TARGET] =
    { "Thermal Sensor",                                              /* name */
      "thermalsensor",                                               /* parsed_name */
      NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,                            /* nvctrl */
      CTRL_TARGET_PERM_BIT(THERMAL_SENSOR_TARGET),                   /* permission_bit */
      NV_FALSE,                                                      /* uses_display_devices */
      1, 23 },                                                       /* required major,minor protocol rev */

    [NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET] =
    { "3D Vision Pro Transceiver",                                   /* name */
      "svp",                                                         /* parsed_name */
      NV_CTRL_TARGET_TYPE_3D_VISION_PRO_TRANSCEIVER,                 /* nvctrl */
      CTRL_TARGET_PERM_BIT(NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET), /* permission_bit */
      NV_FALSE,                                                      /* uses_display_devices */
      1, 25 },                                                       /* required major,minor protocol rev */

    [DISPLAY_TARGET] =
    { "Display Device",                                              /* name */
      "dpy",                                                         /* parsed_name */
      NV_CTRL_TARGET_TYPE_DISPLAY,                                   /* nvctrl */
      CTRL_TARGET_PERM_BIT(DISPLAY_TARGET),                          /* permission_bit */
      NV_FALSE,                                                      /* uses_display_devices */
      1, 27 },                                                       /* required major,minor protocol rev */
};

const int targetTypeInfoTableLen = ARRAY_LEN(targetTypeInfoTable);


/*
 * List of unique event handles to track
 */
static NvCtrlEventPrivateHandleNode *__event_handles = NULL;


Bool NvCtrlIsTargetTypeValid(CtrlTargetType target_type)
{
    switch (target_type) {
        case X_SCREEN_TARGET:
        case GPU_TARGET:
        case FRAMELOCK_TARGET:
        case VCS_TARGET:
        case GVI_TARGET:
        case COOLER_TARGET:
        case THERMAL_SENSOR_TARGET:
        case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
        case DISPLAY_TARGET:
            return TRUE;
        default:
            return FALSE;
    }
}


const CtrlTargetTypeInfo *NvCtrlGetTargetTypeInfo(CtrlTargetType target_type)
{
    if (!NvCtrlIsTargetTypeValid(target_type)) {
        return NULL;
    }

    return &(targetTypeInfoTable[target_type]);
}


const CtrlTargetTypeInfo *NvCtrlGetTargetTypeInfoByName(const char *name)
{
    int i;

    for (i = 0; i < targetTypeInfoTableLen; i++) {
        if (nv_strcasecmp(targetTypeInfoTable[i].parsed_name, name)) {
            return &targetTypeInfoTable[i];
        }
    }

    return NULL;
}



/*
 * Initializes the control panel backend; this includes probing for the
 * various extensions, downloading the initial state of attributes, etc.
 * Takes a System pointer and target info, and returns an opaque handle
 * on success; returns NULL if the backend cannot use this handle.
 */

NvCtrlAttributeHandle *NvCtrlAttributeInit(CtrlSystem *system,
                                           CtrlTargetType target_type,
                                           int target_id,
                                           unsigned int subsystems)
{
    NvCtrlAttributePrivateHandle *h = NULL;

    h = nvalloc(sizeof (NvCtrlAttributePrivateHandle));

    /* initialize the display and screen to the parameter values */

    h->dpy = system->dpy;
    h->target_type = target_type;
    h->target_id = target_id;

    /* initialize the NV-CONTROL attributes */

    if (subsystems & NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM) {
        h->nv = NvCtrlInitNvControlAttributes(h);

        /* Give up if it failed and target needs NV-CONTROL */
        if (!h->nv && TARGET_TYPE_NEEDS_NVCONTROL(target_type)) {
            goto failed;
        }
    }

    /*
     * initialize X Screen specific attributes for X Screen
     * target types.
     */

    if (target_type == X_SCREEN_TARGET) {

        /*
         * initialize the XF86VidMode attributes; it is OK if this
         * fails
         */
        
        if (subsystems & NV_CTRL_ATTRIBUTES_XF86VIDMODE_SUBSYSTEM) {
            h->vm = NvCtrlInitVidModeAttributes(h);
        }
        
        /*
         * initialize the XVideo extension and attributes; it is OK if
         * this fails
         */
        
        if (subsystems & NV_CTRL_ATTRIBUTES_XVIDEO_SUBSYSTEM) {
            h->xv = NvCtrlInitXvAttributes(h);
        }
        
        /*
         * initialize the GLX extension and attributes; it is OK if
         * this fails
         */
        
        if (subsystems & NV_CTRL_ATTRIBUTES_GLX_SUBSYSTEM) {
            h->glx = NvCtrlInitGlxAttributes(h);
        }
    } /* X Screen target type attribute subsystems */

    /*
     * initialize the XRandR extension and attributes; this does not
     * require an X screen and it is OK if this fails
     */
    if (subsystems & NV_CTRL_ATTRIBUTES_XRANDR_SUBSYSTEM) {
        h->xrandr = NvCtrlInitXrandrAttributes(h);
    }

    /*
     * initialize NVML-specific attributes for NVML-related target types.
     */

    if ((subsystems & NV_CTRL_ATTRIBUTES_NVML_SUBSYSTEM) &&
        TARGET_TYPE_IS_NVML_COMPATIBLE(target_type)) {

        h->nvml = NvCtrlInitNvmlAttributes(h);
    }

    return (NvCtrlAttributeHandle *) h;

 failed:
    if (h) free (h);
    return NULL;

} /* NvCtrlAttributeInit() */

/*
 * Rebuild specified private subsystem handles
 */
void NvCtrlRebuildSubsystems(CtrlTarget *ctrl_target, unsigned int subsystem)
{
    NvCtrlAttributePrivateHandle *h = getPrivateHandle(ctrl_target);

    if (h == NULL) {
        return;
    }

    if (subsystem & NV_CTRL_ATTRIBUTES_XRANDR_SUBSYSTEM) {
        NvCtrlXrandrAttributesClose(h);
        h->xrandr = NvCtrlInitXrandrAttributes(h);
    }

}



/*
 * NvCtrlGetDisplayName() - return a string of the form:
 * 
 * [host]:[display].[screen]
 *
 * that describes the X screen associated with this CtrlTarget.  This
 * is done by getting the string that describes the display connection,
 * and then substituting the correct screen number.  If no hostname is
 * present in the display string, uname.nodename is prepended.  Returns NULL if
 * any error occurs.
 */

char *NvCtrlGetDisplayName(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    char *display_name;

    if (h == NULL || h->dpy == NULL) {
        return NULL;
    }

    display_name = DisplayString(h->dpy);

    if (h->target_type != X_SCREEN_TARGET) {
        /* Return the display name and # without a screen number */
        return nv_standardize_screen_name(display_name, -2);
    }

    return nv_standardize_screen_name(display_name, h->target_id);
    
} /* NvCtrlGetDisplayName() */


/*
 * NvCtrlDisplayPtr() - returns the Display pointer associated with
 * this NvCtrlAttributeHandle.
 */
  
Display *NvCtrlGetDisplayPtr(CtrlTarget *ctrl_target)
{
    NvCtrlAttributePrivateHandle *h = getPrivateHandle(ctrl_target);
        
    if (h == NULL) {
        return NULL;
    }
    
    return h->dpy;

} /* NvCtrlDisplayPtr() */


/*
 * NvCtrlGetScreen() - returns the screen number associated with this
 * CtrlTarget.
 */

int NvCtrlGetScreen(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->target_type != X_SCREEN_TARGET) {
        return -1;
    }
    
    return h->target_id;

} /* NvCtrlGetScreen() */


/*
 * NvCtrlGetTargetType() - returns the target type associated with this
 * CtrlTarget.
 */

int NvCtrlGetTargetType(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return -1;
    }

    return h->target_type;

} /* NvCtrlGetTargetType() */


/*
 * NvCtrlGetTargetId() - returns the target id number associated with this
 * CtrlTarget.
 */

int NvCtrlGetTargetId(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return -1;
    }

    return h->target_id;

} /* NvCtrlGetTargetId() */


/*
 * NvCtrlGetScreenWidth() - return the width of the screen associated
 * with this CtrlTarget.
 */

int NvCtrlGetScreenWidth(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->target_type != X_SCREEN_TARGET) {
        return -1;
    }

    if (h->target_id >= NvCtrlGetScreenCount(ctrl_target)) {
        return -1;
    }

    return DisplayWidth(h->dpy, h->target_id);

} /* NvCtrlGetScreenWidth() */


/*
 * NvCtrlGetScreenHeight() - return the height of the screen
 * associated with this CtrlTarget.
 */

int NvCtrlGetScreenHeight(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->target_type != X_SCREEN_TARGET) {
        return -1;
    }

    if (h->target_id >= NvCtrlGetScreenCount(ctrl_target)) {
        return -1;
    }

    return DisplayHeight(h->dpy, h->target_id);

} /* NvCtrlGetScreenHeight() */


/*
 * NvCtrlGetServerVendor() - return the server vendor
 * information string associated with this CtrlTarget.
 */

char *NvCtrlGetServerVendor(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->dpy == NULL) {
        return NULL;
    }

    return ServerVendor(h->dpy);

} /* NvCtrlGetServerVendor() */


/*
 * NvCtrlGetVendorRelease() - return the server vendor
 * release number associated with this CtrlTarget.
 */

int NvCtrlGetVendorRelease(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->dpy == NULL) {
        return -1;
    }

    return VendorRelease(h->dpy);

} /* NvCtrlGetVendorRelease() */


/*
 * NvCtrlGetScreenCount() - return the number of (logical)
 * X Screens associated with this CtrlTarget.
 */

int NvCtrlGetScreenCount(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->dpy == NULL) {
        return -1;
    }

    return ScreenCount(h->dpy);

} /* NvCtrlGetScreenCount() */


/*
 * NvCtrlGetProtocolVersion() - Returns the majoy version
 * number of the X protocol (server).
 */

int NvCtrlGetProtocolVersion(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->dpy == NULL) {
        return -1;
    }

    return ProtocolVersion(h->dpy);

} /* NvCtrlGetProtocolVersion() */


/*
 * NvCtrlGetProtocolRevision() - Returns the revision number
 * of the X protocol (server).
 */

int NvCtrlGetProtocolRevision(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->dpy == NULL) {
        return -1;
    }

    return ProtocolRevision(h->dpy);

} /* NvCtrlGetProtocolRevision() */


/*
 * NvCtrlGetScreenWidthMM() - return the width (in Millimeters) of the
 * screen associated with this CtrlTarget.
 */

int NvCtrlGetScreenWidthMM(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->target_type != X_SCREEN_TARGET) {
        return -1;
    }

    if (h->target_id >= NvCtrlGetScreenCount(ctrl_target)) {
        return -1;
    }

    return DisplayWidthMM(h->dpy, h->target_id);

} /* NvCtrlGetScreenWidthMM() */


/*
 * NvCtrlGetScreenHeightMM() - return the height (in Millimeters) of the
 * screen associated with this CtrlTarget.
 */

int NvCtrlGetScreenHeightMM(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->target_type != X_SCREEN_TARGET) {
        return -1;
    }

    if (h->target_id >= NvCtrlGetScreenCount(ctrl_target)) {
        return -1;
    }

    return DisplayHeightMM(h->dpy, h->target_id);

} /* NvCtrlGetScreenHeightMM() */


/*
 * NvCtrlGetScreenPlanes() - return the number of planes (the depth)
 * of the screen associated with this CtrlTarget.
 */

int NvCtrlGetScreenPlanes(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL || h->target_type != X_SCREEN_TARGET) {
        return -1;
    }

    if (h->target_id >= NvCtrlGetScreenCount(ctrl_target)) {
        return -1;
    }

    return DisplayPlanes(h->dpy, h->target_id);

} /* NvCtrlGetScreenPlanes() */



ReturnStatus NvCtrlQueryTargetCount(const CtrlTarget *ctrl_target,
                                    CtrlTargetType target_type,
                                    int *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    switch (target_type) {
        case GPU_TARGET:
        case THERMAL_SENSOR_TARGET:
        case COOLER_TARGET:
            {
                ReturnStatus ret = NvCtrlNvmlQueryTargetCount(ctrl_target,
                                                              target_type,
                                                              val);
                if ((ret != NvCtrlMissingExtension) &&
                    (ret != NvCtrlNotSupported)) {
                    return ret;
                }
                /* Fall through */
            }
        case DISPLAY_TARGET:
        case X_SCREEN_TARGET:
        case FRAMELOCK_TARGET:
        case VCS_TARGET:
        case GVI_TARGET:
        case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
            return NvCtrlNvControlQueryTargetCount(h, target_type, val);
        default:
            return NvCtrlBadHandle;
    }
} /* NvCtrlQueryTargetCount() */

ReturnStatus NvCtrlGetAttribute(const CtrlTarget *ctrl_target,
                                int attr, int *val)
{
    return NvCtrlGetDisplayAttribute(ctrl_target, 0, attr, val);
    
} /* NvCtrlGetAttribute() */


ReturnStatus NvCtrlSetAttribute(CtrlTarget *ctrl_target,
                                int attr, int val)
{
    return NvCtrlSetDisplayAttribute(ctrl_target, 0, attr, val);

} /* NvCtrlSetAttribute() */


ReturnStatus NvCtrlGetAttribute64(const CtrlTarget *ctrl_target,
                                  int attr, int64_t *val)
{
    return NvCtrlGetDisplayAttribute64(ctrl_target, 0, attr, val);

} /* NvCtrlGetAttribute64() */


ReturnStatus NvCtrlGetVoidAttribute(const CtrlTarget *ctrl_target,
                                    int attr, void **ptr)
{
    return NvCtrlGetVoidDisplayAttribute(ctrl_target, 0, attr, ptr);
    
} /* NvCtrlGetVoidAttribute() */


ReturnStatus NvCtrlGetValidAttributeValues(const CtrlTarget *ctrl_target,
                                           int attr,
                                           CtrlAttributeValidValues *val)
{
    return NvCtrlGetValidDisplayAttributeValues(ctrl_target, 0, attr, val);
    
} /* NvCtrlGetValidAttributeValues() */


ReturnStatus NvCtrlGetAttributePerms(const CtrlTarget *ctrl_target,
                                     CtrlAttributeType attr_type,
                                     int attr,
                                     CtrlAttributePerms *perms)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    if (perms == NULL) {
        return NvCtrlBadArgument;
    }

    switch (attr_type) {
        case CTRL_ATTRIBUTE_TYPE_INTEGER:
        case CTRL_ATTRIBUTE_TYPE_STRING:
        case CTRL_ATTRIBUTE_TYPE_BINARY_DATA:
        case CTRL_ATTRIBUTE_TYPE_STRING_OPERATION:
            return NvCtrlNvControlGetAttributePerms(h, attr_type, attr, perms);

        case CTRL_ATTRIBUTE_TYPE_COLOR:
        case CTRL_ATTRIBUTE_TYPE_SDI_CSC:
            /*
             * Allow non NV-CONTROL attributes to be read/written on X screen
             * targets
             */
            perms->read = NV_TRUE;
            perms->write = NV_TRUE;
            perms->valid_targets = CTRL_TARGET_PERM_BIT(X_SCREEN_TARGET);
            return NvCtrlSuccess;

        default:
            return NvCtrlBadArgument;
    }
}



ReturnStatus NvCtrlGetStringAttribute(const CtrlTarget *ctrl_target,
                                      int attr, char **ptr)
{
    return NvCtrlGetStringDisplayAttribute(ctrl_target, 0, attr, ptr);

} /* NvCtrlGetStringAttribute() */


ReturnStatus NvCtrlSetStringAttribute(CtrlTarget *ctrl_target,
                                      int attr, const char *ptr)
{
    return NvCtrlSetStringDisplayAttribute(ctrl_target, 0, attr, ptr);

} /* NvCtrlSetStringAttribute() */


ReturnStatus NvCtrlGetDisplayAttribute64(const CtrlTarget *ctrl_target,
                                         unsigned int display_mask,
                                         int attr, int64_t *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    if ((attr >= NV_CTRL_ATTR_EXT_BASE) &&
        (attr <= NV_CTRL_ATTR_EXT_LAST_ATTRIBUTE)) {
        switch (attr) {
          case NV_CTRL_ATTR_EXT_NV_PRESENT:
            *val = (h->nv) ? True : False; break;
          case NV_CTRL_ATTR_EXT_VM_PRESENT:
            *val = (h->vm) ? True : False; break;
          case NV_CTRL_ATTR_EXT_XV_OVERLAY_PRESENT:
            *val = h->xv && h->xv->overlay; break;
          case NV_CTRL_ATTR_EXT_XV_TEXTURE_PRESENT:
            *val = h->xv && h->xv->texture; break;
          case NV_CTRL_ATTR_EXT_XV_BLITTER_PRESENT:
            *val = h->xv && h->xv->blitter; break;
          default:
            return NvCtrlNoAttribute;
        }
        return NvCtrlSuccess;
    }

    if (attr >= NV_CTRL_ATTR_RANDR_BASE &&
        attr <= NV_CTRL_ATTR_RANDR_LAST_ATTRIBUTE) {
        return NvCtrlXrandrGetAttribute(h, display_mask, attr, val);
    }

    if (((attr >= 0) && (attr <= NV_CTRL_LAST_ATTRIBUTE)) ||
        ((attr >= NV_CTRL_ATTR_NV_BASE) &&
         (attr <= NV_CTRL_ATTR_NV_LAST_ATTRIBUTE))) {

        switch (h->target_type) {
            case GPU_TARGET:
            case THERMAL_SENSOR_TARGET:
            case COOLER_TARGET:
                {
                    ReturnStatus ret = NvCtrlNvmlGetAttribute(ctrl_target,
                                                              attr,
                                                              val);
                    if ((ret != NvCtrlMissingExtension) &&
                        (ret != NvCtrlNotSupported)) {
                        return ret;
                    }
                    /* Fall through */
                }
            case DISPLAY_TARGET:
            case X_SCREEN_TARGET:
            case FRAMELOCK_TARGET:
            case VCS_TARGET:
            case GVI_TARGET:
            case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
                if (!h->nv) return NvCtrlMissingExtension;
                return NvCtrlNvControlGetAttribute(h, display_mask, attr, val);
            default:
                return NvCtrlBadHandle;
        }
    }

    return NvCtrlNoAttribute;
    
} /* NvCtrlGetDisplayAttribute64() */

ReturnStatus NvCtrlGetDisplayAttribute(const CtrlTarget *ctrl_target,
                                       unsigned int display_mask,
                                       int attr, int *val)
{
    ReturnStatus status;
    int64_t value_64;

    status = NvCtrlGetDisplayAttribute64(ctrl_target, display_mask, attr,
                                         &value_64);
    if (status == NvCtrlSuccess) {
        *val = value_64;
    }

    return status;

} /* NvCtrlGetDisplayAttribute() */


ReturnStatus NvCtrlSetDisplayAttribute(CtrlTarget *ctrl_target,
                                       unsigned int display_mask,
                                       int attr, int val)
{
    NvCtrlAttributePrivateHandle *h = getPrivateHandle(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    if ((attr >= 0) && (attr <= NV_CTRL_LAST_ATTRIBUTE)) {
        switch (h->target_type) {
            case GPU_TARGET:
            case THERMAL_SENSOR_TARGET:
            case COOLER_TARGET:
                {
                    ReturnStatus ret;
                    ret = NvCtrlNvmlSetAttribute(ctrl_target,
                                                 attr,
                                                 display_mask,
                                                 val);
                    if ((ret != NvCtrlMissingExtension) &&
                        (ret != NvCtrlNotSupported)) {
                        return ret;
                    }
                    /* Fall through */
                }
            case DISPLAY_TARGET:
            case X_SCREEN_TARGET:
            case FRAMELOCK_TARGET:
            case VCS_TARGET:
            case GVI_TARGET:
            case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
                if (!h->nv) {
                    return NvCtrlMissingExtension;
                }
                return NvCtrlNvControlSetAttribute(h, display_mask, attr, val);
            default:
                return NvCtrlBadHandle;
        }
    }

    return NvCtrlNoAttribute;
}


ReturnStatus NvCtrlGetVoidDisplayAttribute(const CtrlTarget *ctrl_target,
                                           unsigned int display_mask,
                                           int attr, void **ptr)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    if ( attr >= NV_CTRL_ATTR_GLX_BASE &&
         attr >= NV_CTRL_ATTR_GLX_LAST_ATTRIBUTE ) {
        if ( !(h->glx) ) return NvCtrlMissingExtension;
        return NvCtrlGlxGetVoidAttribute(h, display_mask, attr, ptr);
    }

    return NvCtrlNoAttribute;

} /* NvCtrlGetVoidDisplayAttribute() */


ReturnStatus
NvCtrlGetValidDisplayAttributeValues(const CtrlTarget *ctrl_target,
                                     unsigned int display_mask, int attr,
                                     CtrlAttributeValidValues *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }
    
    if ((attr >= 0) && (attr <= NV_CTRL_LAST_ATTRIBUTE)) {
        switch (h->target_type) {
            case GPU_TARGET:
            case THERMAL_SENSOR_TARGET:
            case COOLER_TARGET:
                {
                    ReturnStatus ret;
                    ret = NvCtrlNvmlGetValidAttributeValues(ctrl_target,
                                                            attr,
                                                            val);
                    if ((ret != NvCtrlMissingExtension) &&
                        (ret != NvCtrlNotSupported)) {
                        return ret;
                    }
                    /* Fall through */
                }
            case DISPLAY_TARGET:
            case X_SCREEN_TARGET:
            case FRAMELOCK_TARGET:
            case VCS_TARGET:
            case GVI_TARGET:
            case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
                if (!h->nv) {
                    return NvCtrlMissingExtension;
                }
                return NvCtrlNvControlGetValidAttributeValues(h, display_mask,
                                                              attr, val);
            default:
                return NvCtrlBadHandle;
        }
    }

    return NvCtrlNoAttribute;
    
} /* NvCtrlGetValidDisplayAttributeValues() */


/*
 * GetValidStringDisplayAttributeValuesExtraAttr() -fill the
 * CtrlAttributeValidValues strucure for extra string atrributes i.e.
 * NvCtrlNvControl*, NvCtrlGlx*, NvCtrlXrandr*, NvCtrlVidMode*, or NvCtrlXv*.
 */

static ReturnStatus
GetValidStringDisplayAttributeValuesExtraAttr(CtrlAttributeValidValues *val)
{
    if (val) {
        memset(val, 0, sizeof(*val));
        val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_STRING;
        val->permissions.read = NV_TRUE;
        val->permissions.valid_targets = CTRL_TARGET_PERM_BIT(X_SCREEN_TARGET);
        return NvCtrlSuccess;
    } else {
        return NvCtrlBadArgument;
    }
} /* GetValidStringDisplayAttributeValuesExtraAttr() */


/*
 * NvCtrlGetValidStringDisplayAttributeValues() -fill the
 * CtrlAttributeValidValues structure for String attributes
 */

ReturnStatus
NvCtrlGetValidStringDisplayAttributeValues(const CtrlTarget *ctrl_target,
                                           unsigned int display_mask, int attr,
                                           CtrlAttributeValidValues *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    if ((attr >= 0) && (attr <= NV_CTRL_STRING_LAST_ATTRIBUTE)) {
        switch (h->target_type) {
            case GPU_TARGET:
            case THERMAL_SENSOR_TARGET:
            case COOLER_TARGET:
                {
                    ReturnStatus ret;
                    ret = NvCtrlNvmlGetValidStringAttributeValues(ctrl_target,
                                                                  attr,
                                                                  val);
                    if ((ret != NvCtrlMissingExtension) &&
                        (ret != NvCtrlNotSupported)) {
                        return ret;
                    }
                    /* Fall through */
                }
            case DISPLAY_TARGET:
            case X_SCREEN_TARGET:
            case FRAMELOCK_TARGET:
            case VCS_TARGET:
            case GVI_TARGET:
            case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
                if (!h->nv) {
                    return NvCtrlMissingExtension;
                }
                return NvCtrlNvControlGetValidStringDisplayAttributeValues(
                           h, display_mask, attr, val);
            default:
                return NvCtrlBadHandle;
        }
    }

    /*
     * The below string attributes are only available for X screen target
     * types
     */

    if (h->target_type != X_SCREEN_TARGET) {
        return NvCtrlAttributeNotAvailable;
    }
    
    if ((attr >= NV_CTRL_STRING_NV_CONTROL_BASE) &&
        (attr <= NV_CTRL_STRING_NV_CONTROL_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    if ((attr >= NV_CTRL_STRING_GLX_BASE) &&
        (attr <= NV_CTRL_STRING_GLX_LAST_ATTRIBUTE)) {
        if (!h->glx) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    if ((attr >= NV_CTRL_STRING_XRANDR_BASE) &&
        (attr <= NV_CTRL_STRING_XRANDR_LAST_ATTRIBUTE)) {
        if (!h->xrandr) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    if ((attr >= NV_CTRL_STRING_XF86VIDMODE_BASE) &&
        (attr <= NV_CTRL_STRING_XF86VIDMODE_LAST_ATTRIBUTE)) {
        if (!h->vm) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    if ((attr >= NV_CTRL_STRING_XV_BASE) &&
        (attr <= NV_CTRL_STRING_XV_LAST_ATTRIBUTE)) {
        if (!h->xv) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    return NvCtrlNoAttribute;

} /* NvCtrlGetValidStringDisplayAttributeValues() */


ReturnStatus NvCtrlGetStringDisplayAttribute(const CtrlTarget *ctrl_target,
                                             unsigned int display_mask,
                                             int attr, char **ptr)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    switch (h->target_type) {
        case GPU_TARGET:
        case THERMAL_SENSOR_TARGET:
        case COOLER_TARGET:
            {
                ReturnStatus ret = NvCtrlNvmlGetStringAttribute(ctrl_target,
                                                                attr,
                                                                ptr);
                if ((ret != NvCtrlMissingExtension) &&
                    (ret != NvCtrlNotSupported)) {
                    return ret;
                }
                /* Fall through */
            }
        case DISPLAY_TARGET:
        case X_SCREEN_TARGET:
        case FRAMELOCK_TARGET:
        case VCS_TARGET:
        case GVI_TARGET:
        case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
            if ((attr >= 0) && (attr <= NV_CTRL_STRING_LAST_ATTRIBUTE)) {
                if (!h->nv) return NvCtrlMissingExtension;
                return NvCtrlNvControlGetStringAttribute(h, display_mask, attr, ptr);
            }

            if ((attr >= NV_CTRL_STRING_NV_CONTROL_BASE) &&
                (attr <= NV_CTRL_STRING_NV_CONTROL_LAST_ATTRIBUTE)) {
                if (!h->nv) return NvCtrlMissingExtension;
                return NvCtrlNvControlGetStringAttribute(h, display_mask, attr, ptr);
            }

            if ((attr >= NV_CTRL_STRING_GLX_BASE) &&
                (attr <= NV_CTRL_STRING_GLX_LAST_ATTRIBUTE)) {
                if (!h->glx) return NvCtrlMissingExtension;
                return NvCtrlGlxGetStringAttribute(h, display_mask, attr, ptr);
            }

            if ((attr >= NV_CTRL_STRING_XRANDR_BASE) &&
                (attr <= NV_CTRL_STRING_XRANDR_LAST_ATTRIBUTE)) {
                if (!h->xrandr) return NvCtrlMissingExtension;
                return NvCtrlXrandrGetStringAttribute(h, display_mask, attr, ptr);
            }

            if ((attr >= NV_CTRL_STRING_XF86VIDMODE_BASE) &&
                (attr <= NV_CTRL_STRING_XF86VIDMODE_LAST_ATTRIBUTE)) {
                if (!h->vm) return NvCtrlMissingExtension;
                return NvCtrlVidModeGetStringAttribute(h, display_mask, attr, ptr);
            }

            if ((attr >= NV_CTRL_STRING_XV_BASE) &&
                (attr <= NV_CTRL_STRING_XV_LAST_ATTRIBUTE)) {
                if (!h->xv) return NvCtrlMissingExtension;
                return NvCtrlXvGetStringAttribute(h, display_mask, attr, ptr);
            }

            return NvCtrlNoAttribute;

        default:
            return NvCtrlBadHandle;
    }

} /* NvCtrlGetStringDisplayAttribute() */


ReturnStatus NvCtrlSetStringDisplayAttribute(CtrlTarget *ctrl_target,
                                             unsigned int display_mask,
                                             int attr, const char *ptr)
{
    NvCtrlAttributePrivateHandle *h = getPrivateHandle(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    if ((attr >= 0) && (attr <= NV_CTRL_STRING_LAST_ATTRIBUTE)) {
        switch (h->target_type) {
            case GPU_TARGET:
            case THERMAL_SENSOR_TARGET:
            case COOLER_TARGET:
                {
                    ReturnStatus ret = NvCtrlNvmlSetStringAttribute(ctrl_target,
                                                                    attr,
                                                                    ptr);
                    if ((ret != NvCtrlMissingExtension) &&
                        (ret != NvCtrlNotSupported)) {
                        return ret;
                    }
                    /* Fall through */
                }
            case DISPLAY_TARGET:
            case X_SCREEN_TARGET:
            case FRAMELOCK_TARGET:
            case VCS_TARGET:
            case GVI_TARGET:
            case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
                if (!h->nv) return NvCtrlMissingExtension;
                return NvCtrlNvControlSetStringAttribute(h, display_mask, attr,
                                                         ptr);
            default:
                return NvCtrlBadHandle;
        }
    }

    return NvCtrlNoAttribute;
}


ReturnStatus NvCtrlGetBinaryAttribute(const CtrlTarget *ctrl_target,
                                      unsigned int display_mask, int attr,
                                      unsigned char **data, int *len)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    switch (h->target_type) {
        case GPU_TARGET:
        case THERMAL_SENSOR_TARGET:
        case COOLER_TARGET:
            {
                ReturnStatus ret = NvCtrlNvmlGetBinaryAttribute(ctrl_target,
                                                                attr,
                                                                data,
                                                                len);
                if ((ret != NvCtrlMissingExtension) &&
                    (ret != NvCtrlNotSupported)) {
                    return ret;
                }
                /* Fall through */
            }
        case DISPLAY_TARGET:
        case X_SCREEN_TARGET:
        case FRAMELOCK_TARGET:
        case VCS_TARGET:
        case GVI_TARGET:
        case NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET:
            return NvCtrlNvControlGetBinaryAttribute(h, display_mask, attr, data, len);
        default:
            return NvCtrlBadHandle;
    }

} /* NvCtrlGetBinaryAttribute() */


ReturnStatus NvCtrlStringOperation(CtrlTarget *ctrl_target,
                                   unsigned int display_mask, int attr,
                                   const char *ptrIn, char **ptrOut)
{
    NvCtrlAttributePrivateHandle *h = getPrivateHandle(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    if ((attr >= 0) && (attr <= NV_CTRL_STRING_OPERATION_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlStringOperation(h, display_mask, attr, ptrIn,
                                              ptrOut);
    }

    return NvCtrlNoAttribute;
}


char *NvCtrlAttributesStrError(ReturnStatus status)
{
    switch (status) {
      case NvCtrlSuccess:
          return "Success"; break;
      case NvCtrlBadArgument:
          return "Bad argument"; break;
      case NvCtrlBadHandle:
          return "Bad handle"; break;
      case NvCtrlNoAttribute:
          return "No such attribute"; break;
      case NvCtrlMissingExtension:
          return "Missing Extension"; break;
      case NvCtrlReadOnlyAttribute:
          return "Read only attribute"; break;
      case NvCtrlWriteOnlyAttribute:
          return "Write only attribute"; break;
      case NvCtrlAttributeNotAvailable:
          return "Attribute not available"; break;
      case NvCtrlNotSupported:
          return "Operation not supported"; break;
      case NvCtrlError: /* fall through to default */
      default:
        return "Unknown Error"; break;
    }
} /* NvCtrlAttributesStrError() */


void NvCtrlAttributeClose(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h = (NvCtrlAttributePrivateHandle *)handle;
    
    if (h == NULL) {
        return;
    }

    /*
     * XXX should free any additional resources allocated by each
     * subsystem
     */

    if ( h->glx ) {
        NvCtrlGlxAttributesClose(h);   
    }
    if ( h->xrandr ) {
        NvCtrlXrandrAttributesClose(h);   
    }
    if ( h->xv ) {
        NvCtrlXvAttributesClose(h);
    }
    if ( h->nvml ) {
        NvCtrlNvmlAttributesClose(h);
    }

    free(h);
} /* NvCtrlAttributeClose() */


/*
 * NvCtrlGetMultisampleModeName() - lookup a string describing the
 * NV-CONTROL constant.
*/

const char *NvCtrlGetMultisampleModeName(int multisample_mode)
{
    static const char *mode_names[] = {

        "Off",                 /* FSAA_MODE_NONE  */
        "2x (2xMS)",           /* FSAA_MODE_2x    */
        "2x Quincunx",         /* FSAA_MODE_2x_5t */
        "1.5 x 1.5",           /* FSAA_MODE_15x15 */
        "2 x 2 Supersampling", /* FSAA_MODE_2x2   */
        "4x (4xMS)",           /* FSAA_MODE_4x    */
        "4x, 9-tap Gaussian",  /* FSAA_MODE_4x_9t */
        "8x (4xMS, 4xCS)",     /* FSAA_MODE_8x    */
        "16x (4xMS, 12xCS)",   /* FSAA_MODE_16x   */
        "8x (4xSS, 2xMS)",     /* FSAA_MODE_8xS   */
        "8x (8xMS)",           /* FSAA_MODE_8xQ   */
        "16x (4xSS, 4xMS)",    /* FSAA_MODE_16xS  */
        "16x (8xMS, 8xCS)",    /* FSAA_MODE_16xQ  */
        "32x (4xSS, 8xMS)",    /* FSAA_MODE_32xS  */
        "32x (8xMS, 24xCS)",   /* FSAA_MODE_32x   */
        "64x (16xSS, 4xMS)",   /* FSAA_MODE_64xS  */
    };

    if ((multisample_mode < NV_CTRL_FSAA_MODE_NONE) ||
        (multisample_mode > NV_CTRL_FSAA_MODE_MAX)) {
        return "Unknown Multisampling";
    }

    return mode_names[multisample_mode];

} /* NvCtrlGetMultisampleModeName  */

/*
 * NvCtrlGetStereoModeNameIfExists() - lookup string describing the STEREO
 * Mode. If is doesn't exist, return NULL.
 */

const char *NvCtrlGetStereoModeNameIfExists(int stereo_mode)
{
    static const char *stereo_modes[] = {
        [NV_CTRL_STEREO_OFF]                          = "Disabled",
        [NV_CTRL_STEREO_DDC]                          = "DDC",
        [NV_CTRL_STEREO_BLUELINE]                     = "Blueline",
        [NV_CTRL_STEREO_DIN]                          = "Onboard DIN",
        [NV_CTRL_STEREO_PASSIVE_EYE_PER_DPY]          = "Passive One-Eye-per-Display",
        [NV_CTRL_STEREO_VERTICAL_INTERLACED]          = "Vertical Interlaced",
        [NV_CTRL_STEREO_COLOR_INTERLACED]             = "Color Interleaved",
        [NV_CTRL_STEREO_HORIZONTAL_INTERLACED]        = "Horizontal Interlaced",
        [NV_CTRL_STEREO_CHECKERBOARD_PATTERN]         = "Checkerboard Pattern",
        [NV_CTRL_STEREO_INVERSE_CHECKERBOARD_PATTERN] = "Inverse Checkerboard",
        [NV_CTRL_STEREO_3D_VISION]                    = "NVIDIA 3D Vision",
        [NV_CTRL_STEREO_3D_VISION_PRO]                = "NVIDIA 3D Vision Pro",
        [NV_CTRL_STEREO_HDMI_3D]                      = "HDMI 3D",
    };


    if ((stereo_mode < 0) || (stereo_mode >= ARRAY_LEN(stereo_modes))) {
        return (const char *) -1;
    }

    return stereo_modes[stereo_mode];
}

/*
 * NvCtrlGetStereoModeName() - lookup string describing the STEREO
 * Mode. If is doesn't exist, return descriptive text that the mode
 * was not found.
 */

const char *NvCtrlGetStereoModeName(int stereo_mode)
{
    const char *c = NvCtrlGetStereoModeNameIfExists(stereo_mode);

    if (!c || (c == (const char *) -1)) {
        c = "Unknown Mode";
    }

    return c;
}


ReturnStatus NvCtrlGetColorAttributes(const CtrlTarget *ctrl_target,
                                      float contrast[3],
                                      float brightness[3],
                                      float gamma[3])
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    switch (h->target_type) {
    case X_SCREEN_TARGET:
        return NvCtrlVidModeGetColorAttributes(h, contrast, brightness, gamma);
    case DISPLAY_TARGET:
        return NvCtrlXrandrGetColorAttributes(h, contrast, brightness, gamma);
    default:
        return NvCtrlBadHandle;
    }
}

ReturnStatus NvCtrlSetColorAttributes(CtrlTarget *ctrl_target,
                                      float c[3],
                                      float b[3],
                                      float g[3],
                                      unsigned int bitmask)
{
    ReturnStatus status;
    int val = 0;

    NvCtrlAttributePrivateHandle *h = getPrivateHandle(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    status = NvCtrlGetAttribute(ctrl_target,
                                NV_CTRL_ATTR_RANDR_GAMMA_AVAILABLE,
                                &val);

    if (status == NvCtrlSuccess && val) {
        switch (h->target_type) {
        case X_SCREEN_TARGET:
            return NvCtrlVidModeSetColorAttributes(h, c, b, g, bitmask);
        case DISPLAY_TARGET:
            return NvCtrlXrandrSetColorAttributes(h, c, b, g, bitmask);
        default:
            return NvCtrlBadHandle;
        }
    } else if ((status != NvCtrlSuccess || !val) &&
               h->target_type == NV_CTRL_TARGET_TYPE_X_SCREEN) {
        return NvCtrlVidModeSetColorAttributes(h, c, b, g, bitmask);
    }

    return NvCtrlError;
}


ReturnStatus NvCtrlGetColorRamp(const CtrlTarget *ctrl_target,
                                unsigned int channel,
                                uint16_t **lut,
                                int *n)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    switch (h->target_type) {
    case X_SCREEN_TARGET:
        return NvCtrlVidModeGetColorRamp(h, channel, lut, n);
    case DISPLAY_TARGET:
        return NvCtrlXrandrGetColorRamp(h, channel, lut, n);
    default:
        return NvCtrlBadHandle;
    }
}


ReturnStatus NvCtrlReloadColorRamp(CtrlTarget *ctrl_target)
{
    NvCtrlAttributePrivateHandle *h = getPrivateHandle(ctrl_target);

    if (h == NULL) {
        return NvCtrlBadHandle;
    }

    switch (h->target_type) {
    case X_SCREEN_TARGET:
        return NvCtrlVidModeReloadColorRamp(h);
    case DISPLAY_TARGET:
        return NvCtrlXrandrReloadColorRamp(h);
    default:
        return NvCtrlBadHandle;
    }
}


/* helper functions private to the libXNVCtrlAttributes backend */

void NvCtrlInitGammaInputStruct(NvCtrlGammaInput *pGammaInput)
{
    int ch;

    for (ch = FIRST_COLOR_CHANNEL; ch <= LAST_COLOR_CHANNEL; ch++) {
        pGammaInput->brightness[ch] = BRIGHTNESS_DEFAULT;
        pGammaInput->contrast[ch]   = CONTRAST_DEFAULT;
        pGammaInput->gamma[ch]      = GAMMA_DEFAULT;
    }
}

/*
 * Compute the gammaRamp entry given its index, and the contrast,
 * brightness, and gamma.
 */
static unsigned short ComputeGammaRampVal(int gammaRampSize,
                                          int i,
                                          float contrast,
                                          float brightness,
                                          float gamma)
{
    double j, half, scale;
    int shift, val, num;

    num = gammaRampSize - 1;
    shift = 16 - (ffs(gammaRampSize) - 1);

    scale = (double) num / 3.0; /* how much brightness and contrast
                                   affect the value */
    j = (double) i;

    /* contrast */

    contrast *= scale;

    if (contrast > 0.0) {
        half = ((double) num / 2.0) - 1.0;
        j -= half;
        j *= half / (half - contrast);
        j += half;
    } else {
        half = (double) num / 2.0;
        j -= half;
        j *= (half + contrast) / half;
        j += half;
    }

    /* brightness */

    brightness *= scale;

    j += brightness;
    if (j > (double)num) {
        j = (double)num;
    }
    if (j < 0.0) {
        j = 0.0;
    }

    /* gamma */

    gamma = 1.0 / (double) gamma;

    if (gamma == 1.0) {
        val = (int) j;
    } else {
        val = (int) (pow (j / (double)num, gamma) * (double)num + 0.5);
    }

    val <<= shift;
    return (unsigned short) val;
}

void NvCtrlUpdateGammaRamp(const NvCtrlGammaInput *pGammaInput,
                           int gammaRampSize,
                           unsigned short *gammaRamp[3],
                           unsigned int bitmask)
{
    int i, ch;

    /* update the requested channels within the gammaRamp */

    for (ch = FIRST_COLOR_CHANNEL; ch <= LAST_COLOR_CHANNEL; ch++) {

        /* only update requested channels */

        if ((bitmask & (1 << ch)) == 0) {
            continue;
        }

        for (i = 0; i < gammaRampSize; i++) {
            gammaRamp[ch][i] =
                ComputeGammaRampVal(gammaRampSize,
                                    i,
                                    pGammaInput->contrast[ch],
                                    pGammaInput->brightness[ch],
                                    pGammaInput->gamma[ch]);
        }
    }
}

void NvCtrlAssignGammaInput(NvCtrlGammaInput *pGammaInput,
                            const float inContrast[3],
                            const float inBrightness[3],
                            const float inGamma[3],
                            const unsigned int bitmask)
{
    int ch;

    /* clamp input, but only the input specified in the bitmask */

    for (ch = FIRST_COLOR_CHANNEL; ch <= LAST_COLOR_CHANNEL; ch++) {

        /* only update requested channels */

        if ((bitmask & (1 << ch)) == 0) {
            continue;
        }

        if (bitmask & CONTRAST_VALUE) {
            if (inContrast[ch] > CONTRAST_MAX) {
                pGammaInput->contrast[ch] = CONTRAST_MAX;
            } else if (inContrast[ch] < CONTRAST_MIN) {
                pGammaInput->contrast[ch] = CONTRAST_MIN;
            } else {
                pGammaInput->contrast[ch] = inContrast[ch];
            }
        }

        if (bitmask & BRIGHTNESS_VALUE) {
            if (inBrightness[ch] > BRIGHTNESS_MAX) {
                pGammaInput->brightness[ch] = BRIGHTNESS_MAX;
            } else if (inBrightness[ch] < BRIGHTNESS_MIN) {
                pGammaInput->brightness[ch] = BRIGHTNESS_MIN;
            } else {
                pGammaInput->brightness[ch] = inBrightness[ch];
            }
        }

        if (bitmask & GAMMA_VALUE) {
            if (inGamma[ch] > GAMMA_MAX) {
                pGammaInput->gamma[ch] = GAMMA_MAX;
            } else if (inGamma[ch] < GAMMA_MIN) {
                pGammaInput->gamma[ch] = GAMMA_MIN;
            } else {
                pGammaInput->gamma[ch] = inGamma[ch];
            }
        }
    }
}


NvCtrlEventHandle *NvCtrlGetEventHandle(const CtrlTarget *ctrl_target)
{
    NvCtrlEventPrivateHandle *evt_h;
    NvCtrlEventPrivateHandleNode *evt_hnode;
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return NULL;
    }

    /* Look for the event handle */
    evt_h = NULL;
    for (evt_hnode = __event_handles;
         evt_hnode;
         evt_hnode = evt_hnode->next) {

        if (evt_hnode->handle->dpy == h->dpy) {
            evt_h = evt_hnode->handle;
            break;
        }
    }

    /* If not found, create a new one */
    if (!evt_h) {
        evt_h = nvalloc(sizeof(*evt_h));
        evt_h->dpy = h->dpy;
        evt_h->fd = ConnectionNumber(h->dpy);
        evt_h->nvctrl_event_base = (h->nv) ? h->nv->event_base : -1;
        evt_h->xrandr_event_base = (h->xrandr) ? h->xrandr->event_base : -1;

        /* Add it to the list of event handles */
        evt_hnode = nvalloc(sizeof(*evt_hnode));
        evt_hnode->handle = evt_h;
        evt_hnode->next = __event_handles;
        __event_handles = evt_hnode;
    }

    /*
     * This next bit of code is to make sure that the xrandr_event_base
     * for this event handle is valid in the case where a NON X Screen
     * target type handle is used to create the initial event handle
     * (Resulting in xrandr_event_base being == -1), followed by an
     * X Screen target type handle registering itself to receive
     * XRandR events on the existing dpy/event handle.
     */
    if (evt_h->xrandr_event_base == -1 &&
        h->target_type == X_SCREEN_TARGET &&
        h->xrandr) {

        evt_h->xrandr_event_base = h->xrandr->event_base;
    }

    return (NvCtrlEventHandle *)evt_h;
}

ReturnStatus
NvCtrlCloseEventHandle(NvCtrlEventHandle *handle)
{
    NvCtrlEventPrivateHandleNode *evt_hnode;

    if (!handle) {
        return NvCtrlBadArgument;
    }

    /* Look for the event handle */
    if (__event_handles) {
        NvCtrlEventPrivateHandleNode *prev;

        if (__event_handles->handle == handle) {
            evt_hnode = __event_handles;
            __event_handles = __event_handles->next;
            goto free_handle;
        }

        prev = __event_handles;
        for (evt_hnode = __event_handles->next;
             evt_hnode;
             evt_hnode = evt_hnode->next) {

            if (evt_hnode->handle == handle) {
                prev->next = evt_hnode->next;
                goto free_handle;
            }

            prev = evt_hnode;
        }
    }

    return NvCtrlBadHandle;

free_handle:
    free(handle);
    free(evt_hnode);

    return NvCtrlSuccess;
}

ReturnStatus
NvCtrlEventHandleGetFD(NvCtrlEventHandle *handle, int *fd)
{
    NvCtrlEventPrivateHandle *evt_h;

    if (!handle) {
        return NvCtrlBadArgument;
    }

    evt_h = (NvCtrlEventPrivateHandle*)handle;

    *fd = evt_h->fd;

    return NvCtrlSuccess;
}

ReturnStatus
NvCtrlEventHandlePending(NvCtrlEventHandle *handle, Bool *pending)
{
    NvCtrlEventPrivateHandle *evt_h;

    if (!handle) {
        return NvCtrlBadArgument;
    }

    evt_h = (NvCtrlEventPrivateHandle*)handle;

    if (XPending(evt_h->dpy)) {
        *pending = TRUE;
    } else {
        *pending = FALSE;
    }

    return NvCtrlSuccess;
}

static int get_screen_of_root(Display *dpy, Window root)
{
    int screen = -1;

    /* Find the screen the window belongs to */
    screen = XScreenCount(dpy);

    while (screen > 0) {
        screen--;
        if (root == RootWindow(dpy, screen)) {
            break;
        }
    }
    
    return screen;
}

ReturnStatus
NvCtrlEventHandleNextEvent(NvCtrlEventHandle *handle, CtrlEvent *event)
{
    NvCtrlEventPrivateHandle *evt_h;
    XEvent xevent;

    if (!handle) {
        return NvCtrlBadArgument;
    }

    evt_h = (NvCtrlEventPrivateHandle*)handle;

    memset(event, 0, sizeof(CtrlEvent));


    /*
     * if NvCtrlEventHandleNextEvent() is called, then
     * NvCtrlEventHandlePending() returned TRUE, so we
     * know there is an event pending
     */
    XNextEvent(evt_h->dpy, &xevent);


    /*
     * Handle NV-CONTROL events
     */
    if (evt_h->nvctrl_event_base != -1) {

        int xevt_type = xevent.type - evt_h->nvctrl_event_base;

        /* 
         * Handle the ATTRIBUTE_CHANGED_EVENT event
         */
        if (xevt_type == ATTRIBUTE_CHANGED_EVENT) {

            XNVCtrlAttributeChangedEvent *nvctrlevent =
                (XNVCtrlAttributeChangedEvent *) &xevent;

            event->type        = CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE;
            event->target_type = X_SCREEN_TARGET;
            event->target_id   = nvctrlevent->screen;

            event->int_attr.attribute               = nvctrlevent->attribute;
            event->int_attr.value                   = nvctrlevent->value;
            event->int_attr.is_availability_changed = FALSE;

            return NvCtrlSuccess;
        }

        /* 
         * Handle the TARGET_ATTRIBUTE_CHANGED_EVENT event
         */
        if (xevt_type == TARGET_ATTRIBUTE_CHANGED_EVENT) {

            XNVCtrlAttributeChangedEventTarget *nvctrlevent =
                (XNVCtrlAttributeChangedEventTarget *) &xevent;

            event->type        = CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE;
            event->target_type = nvctrlevent->target_type;
            event->target_id   = nvctrlevent->target_id;

            event->int_attr.attribute               = nvctrlevent->attribute;
            event->int_attr.value                   = nvctrlevent->value;
            event->int_attr.is_availability_changed = FALSE;

            return NvCtrlSuccess;
        }

        /*
         * Handle the TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT event
         */
        if (xevt_type == TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT) {

            XNVCtrlAttributeChangedEventTargetAvailability *nvctrlevent =
                (XNVCtrlAttributeChangedEventTargetAvailability *) &xevent;

            event->type        = CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE;
            event->target_type = nvctrlevent->target_type;
            event->target_id   = nvctrlevent->target_id;

            event->int_attr.attribute               = nvctrlevent->attribute;
            event->int_attr.value                   = nvctrlevent->value;
            event->int_attr.is_availability_changed = TRUE;
            event->int_attr.availability            = nvctrlevent->availability;

            return NvCtrlSuccess;
        }

        /*
         * Handle the TARGET_STRING_ATTRIBUTE_CHANGED_EVENT event
         */
        if (xevt_type == TARGET_STRING_ATTRIBUTE_CHANGED_EVENT) {

            XNVCtrlStringAttributeChangedEventTarget *nvctrlevent =
                (XNVCtrlStringAttributeChangedEventTarget *) &xevent;

            event->type        = CTRL_EVENT_TYPE_STRING_ATTRIBUTE;
            event->target_type = nvctrlevent->target_type;
            event->target_id   = nvctrlevent->target_id;

            event->str_attr.attribute = nvctrlevent->attribute;

            return NvCtrlSuccess;
        }

        /*
         * Handle the TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT event
         */
        if (xevt_type == TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT) {

            XNVCtrlBinaryAttributeChangedEventTarget *nvctrlevent =
                (XNVCtrlBinaryAttributeChangedEventTarget *) &event;

            event->type        = CTRL_EVENT_TYPE_BINARY_ATTRIBUTE;
            event->target_type = nvctrlevent->target_type;
            event->target_id   = nvctrlevent->target_id;

            event->bin_attr.attribute = nvctrlevent->attribute;

            return NvCtrlSuccess;
        }
    }


    /*
     * Handle XRandR events
     */
    if (evt_h->xrandr_event_base != -1) {

        int rrevt_type = xevent.type - evt_h->xrandr_event_base;

        /*
         * Handle the RRScreenChangeNotify event
         */
        if (rrevt_type == RRScreenChangeNotify) {

            XRRScreenChangeNotifyEvent *xrandrevent =
                (XRRScreenChangeNotifyEvent *)&xevent;

            event->type        = CTRL_EVENT_TYPE_SCREEN_CHANGE;
            event->target_type = X_SCREEN_TARGET;
            event->target_id   = get_screen_of_root(xrandrevent->display,
                                                    xrandrevent->root);

            event->screen_change.width   = xrandrevent->width;
            event->screen_change.height  = xrandrevent->height;
            event->screen_change.mwidth  = xrandrevent->mwidth;
            event->screen_change.mheight = xrandrevent->mheight;

            return NvCtrlSuccess;
        }
    }


    /*
     * Trap events that get registered but are not handled
     * properly.
     */
    nv_warning_msg("Unknown event type %d.", xevent.type);
    
    return NvCtrlSuccess;
}

