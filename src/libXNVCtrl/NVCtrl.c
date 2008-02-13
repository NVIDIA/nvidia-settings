/*
 * Make sure that XTHREADS is defined, so that the
 * LockDisplay/UnlockDisplay macros are expanded properly and the
 * libXNVCtrl library properly protects the Display connection.
 */

#if !defined(XTHREADS)
#define XTHREADS
#endif /* XTHREADS */

#define NEED_EVENTS
#define NEED_REPLIES
#include <stdlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#include "NVCtrlLib.h"
#include "nv_control.h"


static XExtensionInfo _nvctrl_ext_info_data;
static XExtensionInfo *nvctrl_ext_info = &_nvctrl_ext_info_data;
static /* const */ char *nvctrl_extension_name = NV_CONTROL_NAME;

#define XNVCTRLCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, nvctrl_extension_name, val)
#define XNVCTRLSimpleCheckExtension(dpy,i) \
  XextSimpleCheckExtension (dpy, i, nvctrl_extension_name)

static int close_display();
static Bool wire_to_event();
static /* const */ XExtensionHooks nvctrl_extension_hooks = {
    NULL,                               /* create_gc */
    NULL,                               /* copy_gc */
    NULL,                               /* flush_gc */
    NULL,                               /* free_gc */
    NULL,                               /* create_font */
    NULL,                               /* free_font */
    close_display,                      /* close_display */
    wire_to_event,                      /* wire_to_event */
    NULL,                               /* event_to_wire */
    NULL,                               /* error */
    NULL,                               /* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, nvctrl_ext_info,
                                   nvctrl_extension_name, 
                                   &nvctrl_extension_hooks,
                                   NV_CONTROL_EVENTS, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, nvctrl_ext_info)

Bool XNVCTRLQueryExtension (
    Display *dpy,
    int *event_basep,
    int *error_basep
){
    XExtDisplayInfo *info = find_display (dpy);

    if (XextHasExtension(info)) {
        if (event_basep) *event_basep = info->codes->first_event;
        if (error_basep) *error_basep = info->codes->first_error;
        return True;
    } else {
        return False;
    }
}


Bool XNVCTRLQueryVersion (
    Display *dpy,
    int *major,
    int *minor
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryExtensionReply rep;
    xnvCtrlQueryExtensionReq   *req;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryExtension, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryExtension;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    if (major) *major = rep.major;
    if (minor) *minor = rep.minor;
    UnlockDisplay (dpy);
    SyncHandle ();
    return True;
}


Bool XNVCTRLIsNvScreen (
    Display *dpy,
    int screen
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlIsNvReply rep;
    xnvCtrlIsNvReq   *req;
    Bool isnv;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlIsNv, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlIsNv;
    req->screen = screen;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    isnv = rep.isnv;
    UnlockDisplay (dpy);
    SyncHandle ();
    return isnv;
}


Bool XNVCTRLQueryTargetCount (
    Display *dpy,
    int target_type,
    int *value
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryTargetCountReply  rep;
    xnvCtrlQueryTargetCountReq   *req;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryTargetCount, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryTargetCount;
    req->target_type = target_type;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    if (value) *value = rep.count;
    UnlockDisplay (dpy);
    SyncHandle ();
    return True;
}


void XNVCTRLSetTargetAttribute (
    Display *dpy,
    int target_type,
    int target_id,
    unsigned int display_mask,
    unsigned int attribute,
    int value
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlSetAttributeReq *req;

    XNVCTRLSimpleCheckExtension (dpy, info);

    LockDisplay (dpy);
    GetReq (nvCtrlSetAttribute, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlSetAttribute;
    req->target_type = target_type;
    req->target_id = target_id;
    req->display_mask = display_mask;
    req->attribute = attribute;
    req->value = value;
    UnlockDisplay (dpy);
    SyncHandle ();
}

void XNVCTRLSetAttribute (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int attribute,
    int value
){
    XNVCTRLSetTargetAttribute (dpy, NV_CTRL_TARGET_TYPE_X_SCREEN, screen,
                               display_mask, attribute, value);
}


Bool XNVCTRLSetAttributeAndGetStatus (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int attribute,
    int value
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlSetAttributeAndGetStatusReq *req;
    xnvCtrlSetAttributeAndGetStatusReply rep;
    Bool success;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlSetAttributeAndGetStatus, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlSetAttributeAndGetStatus;
    req->screen = screen;
    req->display_mask = display_mask;
    req->attribute = attribute;
    req->value = value;
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    
    success = rep.flags;
    return success;
}



Bool XNVCTRLQueryTargetAttribute (
    Display *dpy,
    int target_type,
    int target_id,
    unsigned int display_mask,
    unsigned int attribute,
    int *value
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryAttributeReply rep;
    xnvCtrlQueryAttributeReq   *req;
    Bool exists;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryAttribute, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryAttribute;
    req->target_type = target_type;
    req->target_id = target_id;
    req->display_mask = display_mask;
    req->attribute = attribute;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    if (value) *value = rep.value;
    exists = rep.flags;
    UnlockDisplay (dpy);
    SyncHandle ();
    return exists;
}

Bool XNVCTRLQueryAttribute (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int attribute,
    int *value
){
    return XNVCTRLQueryTargetAttribute(dpy, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                       screen, display_mask, attribute, value);
}


Bool XNVCTRLQueryTargetStringAttribute (
    Display *dpy,
    int target_type,
    int target_id,
    unsigned int display_mask,
    unsigned int attribute,
    char **ptr
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryStringAttributeReply rep;
    xnvCtrlQueryStringAttributeReq   *req;
    Bool exists;
    int length, numbytes, slop;

    if (!ptr) return False;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryStringAttribute, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryStringAttribute;
    req->target_type = target_type;
    req->target_id = target_id;
    req->display_mask = display_mask;
    req->attribute = attribute;
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    length = rep.length;
    numbytes = rep.n;
    slop = numbytes & 3;
    *ptr = (char *) Xmalloc(numbytes);
    if (! *ptr) {
        _XEatData(dpy, length);
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    } else {
        _XRead(dpy, (char *) *ptr, numbytes);
        if (slop) _XEatData(dpy, 4-slop);
    }
    exists = rep.flags;
    UnlockDisplay (dpy);
    SyncHandle ();
    return exists;
}

Bool XNVCTRLQueryStringAttribute (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int attribute,
    char **ptr
){
    return XNVCTRLQueryTargetStringAttribute(dpy, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                             screen, display_mask,
                                             attribute, ptr);
}


Bool XNVCTRLSetStringAttribute (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int attribute,
    char *ptr
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlSetStringAttributeReq *req;
    xnvCtrlSetStringAttributeReply rep;
    int size;
    Bool success;
    
    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    size = strlen(ptr)+1;

    LockDisplay (dpy);
    GetReq (nvCtrlSetStringAttribute, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlSetStringAttribute;
    req->screen = screen;
    req->display_mask = display_mask;
    req->attribute = attribute;
    req->length += ((size + 3) & ~3) >> 2;
    req->num_bytes = size;
    Data(dpy, ptr, size);
    
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    
    success = rep.flags;
    return success;
}


Bool XNVCTRLQueryValidTargetAttributeValues (
    Display *dpy,
    int target_type,
    int target_id,
    unsigned int display_mask,
    unsigned int attribute,                                 
    NVCTRLAttributeValidValuesRec *values
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryValidAttributeValuesReply rep;
    xnvCtrlQueryValidAttributeValuesReq   *req;
    Bool exists;
    
    if (!values) return False;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryValidAttributeValues, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryValidAttributeValues;
    req->target_type = target_type;
    req->target_id = target_id;
    req->display_mask = display_mask;
    req->attribute = attribute;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    exists = rep.flags;
    values->type = rep.attr_type;
    if (rep.attr_type == ATTRIBUTE_TYPE_RANGE) {
        values->u.range.min = rep.min;
        values->u.range.max = rep.max;
    }
    if (rep.attr_type == ATTRIBUTE_TYPE_INT_BITS) {
        values->u.bits.ints = rep.bits;
    }
    values->permissions = rep.perms;
    UnlockDisplay (dpy);
    SyncHandle ();
    return exists;
}

Bool XNVCTRLQueryValidAttributeValues (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int attribute,                                 
    NVCTRLAttributeValidValuesRec *values
){
    return XNVCTRLQueryValidTargetAttributeValues(dpy,
                                                  NV_CTRL_TARGET_TYPE_X_SCREEN,
                                                  screen, display_mask,
                                                  attribute, values);
}


void XNVCTRLSetGvoColorConversion (
    Display *dpy,
    int screen,
    float colorMatrix[3][3],
    float colorOffset[3],
    float colorScale[3]
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlSetGvoColorConversionReq *req;

    XNVCTRLSimpleCheckExtension (dpy, info);

    LockDisplay (dpy);
    GetReq (nvCtrlSetGvoColorConversion, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlSetGvoColorConversion;
    req->screen = screen;

    req->cscMatrix_y_r = colorMatrix[0][0];
    req->cscMatrix_y_g = colorMatrix[0][1];
    req->cscMatrix_y_b = colorMatrix[0][2];

    req->cscMatrix_cr_r = colorMatrix[1][0];
    req->cscMatrix_cr_g = colorMatrix[1][1];
    req->cscMatrix_cr_b = colorMatrix[1][2];

    req->cscMatrix_cb_r = colorMatrix[2][0];
    req->cscMatrix_cb_g = colorMatrix[2][1];
    req->cscMatrix_cb_b = colorMatrix[2][2];

    req->cscOffset_y  = colorOffset[0];
    req->cscOffset_cr = colorOffset[1];
    req->cscOffset_cb = colorOffset[2];

    req->cscScale_y  = colorScale[0];
    req->cscScale_cr = colorScale[1];
    req->cscScale_cb = colorScale[2];

    UnlockDisplay (dpy);
    SyncHandle ();
}


Bool XNVCTRLQueryGvoColorConversion (
    Display *dpy,
    int screen,
    float colorMatrix[3][3],
    float colorOffset[3],
    float colorScale[3]
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryGvoColorConversionReply rep;
    xnvCtrlQueryGvoColorConversionReq *req;
    
    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);

    GetReq (nvCtrlQueryGvoColorConversion, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryGvoColorConversion;
    req->screen = screen;

    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }

    _XRead(dpy, (char *)(colorMatrix), 36);
    _XRead(dpy, (char *)(colorOffset), 12);
    _XRead(dpy, (char *)(colorScale), 12);

    UnlockDisplay (dpy);
    SyncHandle ();

    return True;
}


Bool XNVCtrlSelectTargetNotify (
    Display *dpy,
    int target_type,
    int target_id,
    int notify_type,
    Bool onoff
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlSelectTargetNotifyReq *req;

    if(!XextHasExtension (info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlSelectTargetNotify, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlSelectTargetNotify;
    req->target_type = target_type;
    req->target_id = target_id;
    req->notifyType = notify_type;
    req->onoff = onoff;
    UnlockDisplay (dpy);
    SyncHandle ();

    return True;
}


Bool XNVCtrlSelectNotify (
    Display *dpy,
    int screen,
    int type,
    Bool onoff
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlSelectNotifyReq *req;

    if(!XextHasExtension (info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlSelectNotify, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlSelectNotify;
    req->screen = screen;
    req->notifyType = type;
    req->onoff = onoff;
    UnlockDisplay (dpy);
    SyncHandle ();

    return True;
}


Bool XNVCTRLQueryDDCCILutSize (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int *red_entries,
    unsigned int *green_entries,
    unsigned int *blue_entries,
    unsigned int *red_bits_per_entries,
    unsigned int *green_bits_per_entries,
    unsigned int *blue_bits_per_entries
) {
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryDDCCILutSizeReply rep;
    xnvCtrlQueryDDCCILutSizeReq *req;
    unsigned int buf[6];
    Bool exists;
    
    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);

    GetReq (nvCtrlQueryDDCCILutSize, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryDDCCILutSize;
    req->screen = screen;
    req->display_mask = display_mask;

    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }

    _XRead(dpy, (char *)(&buf), 24);

    *red_entries = buf[0];
    *green_entries = buf[1];
    *blue_entries = buf[2];
    *red_bits_per_entries = buf[3];
    *green_bits_per_entries = buf[4];
    *blue_bits_per_entries = buf[5];
    
    UnlockDisplay (dpy);
    SyncHandle ();
    exists = rep.flags;
    return exists;
}


Bool XNVCTRLQueryDDCCISinglePointLutOperation (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int offset,
    unsigned int *red_value,
    unsigned int *green_value,
    unsigned int *blue_value
) {
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryDDCCISinglePointLutOperationReply rep;
    xnvCtrlQueryDDCCISinglePointLutOperationReq *req;
    unsigned int buf[4];
    Bool exists;
    
    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);

    GetReq (nvCtrlQueryDDCCISinglePointLutOperation, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryDDCCISinglePointLutOperation;
    req->screen = screen;
    req->display_mask = display_mask;
    req->offset = offset;
    
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }

    _XRead(dpy, (char *)(&buf), 12);

    *red_value = buf[0];
    *green_value = buf[1];
    *blue_value = buf[2];
    
    UnlockDisplay (dpy);
    SyncHandle ();
    
    exists=rep.flags;
    return exists;
}


Bool XNVCTRLSetDDCCISinglePointLutOperation (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int offset,
    unsigned int red_value,
    unsigned int green_value,
    unsigned int blue_value
) { 
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlSetDDCCISinglePointLutOperationReply rep;
    xnvCtrlSetDDCCISinglePointLutOperationReq *req;
    Bool success;
    
    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlSetDDCCISinglePointLutOperation, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlSetDDCCISinglePointLutOperation;
    req->screen = screen;
    req->display_mask = display_mask;
    req->offset = offset;
    req->red_value = red_value;
    req->green_value = green_value;
    req->blue_value = blue_value;
    
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    
    success = rep.flags;
    return success;
}


Bool XNVCTRLQueryDDCCIBlockLutOperation (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int color, // NV_CTRL_DDCCI_RED_LUT, NV_CTRL_DDCCI_GREEN_LUT, NV_CTRL_DDCCI_BLUE_LUT
    unsigned int offset,
    unsigned int size,
    unsigned int **value
) {
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryDDCCIBlockLutOperationReply rep;
    xnvCtrlQueryDDCCIBlockLutOperationReq *req;
    Bool exists;
    int length, slop;
    char *ptr;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryDDCCIBlockLutOperation, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryDDCCIBlockLutOperation;
    req->screen = screen;
    req->display_mask = display_mask;
    req->color=color;
    req->offset=offset;
    req->size=size;
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    length = rep.length;
    slop = rep.num_bytes & 3;
    ptr = (char *) Xmalloc(rep.num_bytes);
    if (! ptr) {
        _XEatData(dpy, length);
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    } else {
        _XRead(dpy, (char *) ptr, rep.num_bytes);
        if (slop) _XEatData(dpy, 4-slop);
    }
    exists = rep.flags;
    if(exists) {
        *value=(unsigned int *)ptr;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    return exists;
}


Bool XNVCTRLSetDDCCIBlockLutOperation (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int color, // NV_CTRL_DDCCI_RED_LUT, NV_CTRL_DDCCI_GREEN_LUT, NV_CTRL_DDCCI_BLUE_LUT
    unsigned int offset,
    unsigned int size,
    unsigned int *value
) {
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlSetDDCCIBlockLutOperationReq *req;
    xnvCtrlSetDDCCIBlockLutOperationReply rep;
    Bool success;
    
    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlSetDDCCIBlockLutOperation, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlSetDDCCIBlockLutOperation;
    req->screen = screen;
    req->display_mask = display_mask;
    req->color = color;
    req->offset = offset;
    req->size = size;
    req->num_bytes = size << 2;
    req->length += (req->num_bytes + 3) >> 2;
    Data(dpy, (char *)value, req->num_bytes );
    
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    
    success = rep.flags;
    return success;
}


Bool XNVCTRLSetDDCCIRemoteProcedureCall (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int offset,
    unsigned int size,
    unsigned int *red_lut,
    unsigned int *green_lut,
    unsigned int *blue_lut,
    unsigned int *increment
) {
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlSetDDCCIRemoteProcedureCallReq *req;
    xnvCtrlSetDDCCIRemoteProcedureCallReply rep;
    unsigned int nbytes;
    Bool success;
    
    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlSetDDCCIRemoteProcedureCall, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlSetDDCCIRemoteProcedureCall;
    req->screen = screen;
    req->display_mask = display_mask;
    req->size = size;
    nbytes= size << 2;
    req->num_bytes = nbytes * 4;
    req->length += (req->num_bytes + 3) >> 2;
    req->offset = offset;
    Data(dpy, (char *)red_lut, nbytes );
    Data(dpy, (char *)green_lut, nbytes );
    Data(dpy, (char *)blue_lut, nbytes );
    Data(dpy, (char *)increment, nbytes );
    
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    
    success = rep.flags;
    return success;
}

/* XXXAlternative: Instead of getting the manufacturer string from the server, 
 * get the manufacturer Id instead, and have controller_manufacturer assigned 
 * to a static string that would not need to be malloc'ed and freed.
 */
Bool XNVCTRLQueryDDCCIDisplayControllerType (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned char **controller_manufacturer,
    unsigned int *controller_type
)
{
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryDDCCIDisplayControllerTypeReply rep;
    xnvCtrlQueryDDCCIDisplayControllerTypeReq *req;
    Bool exists;
    int length, numbytes, slop;
    char *ptr;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryDDCCIDisplayControllerType, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryDDCCIDisplayControllerType;
    req->screen = screen;
    req->display_mask = display_mask;
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    length = rep.length;
    numbytes = rep.size;
    slop = numbytes & 3;
    ptr = (char *) Xmalloc(numbytes);
    if (! ptr) {
        _XEatData(dpy, length);
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    } else {
        _XRead(dpy, (char *) ptr, numbytes);
        if (slop) _XEatData(dpy, 4-slop);
    }
    exists = rep.flags;
    if(exists) {
        *controller_type=rep.controller_type;
        *controller_manufacturer=ptr;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    return exists;
}


Bool NVCTRLQueryDDCCICapabilities (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int **nvctrl_vcp_supported,
    unsigned int **possible_values_offset,
    unsigned int **possible_values_size,
    unsigned int **nvctrl_vcp_possible_values,
    unsigned int **nvctrl_string_vcp_supported
)
{
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryDDCCICapabilitiesReply rep;
    xnvCtrlQueryDDCCICapabilitiesReq *req;
    Bool exists=1;
    int length, numbytes, slop;
    char *ptr, *p;
    int len1, len2, len3, len4, len5;
     
    *nvctrl_vcp_supported=*nvctrl_vcp_possible_values=*possible_values_offset=*possible_values_size=*nvctrl_string_vcp_supported=NULL;
   
    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    
    GetReq (nvCtrlQueryDDCCICapabilities, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryDDCCICapabilities; 
    
    req->screen = screen;
    req->display_mask = display_mask;
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    length = rep.length;
    numbytes = rep.num_bytes;
    slop = numbytes & 3;
    ptr = (char *) Xmalloc(numbytes);
    if (! ptr) {
        _XEatData(dpy, length);
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    } else {
        _XRead(dpy, (char *) ptr, numbytes);
        if (slop) _XEatData(dpy, 4-slop);
    }
    exists = rep.flags;
    if(exists) {
        p = ptr;
        len1 = len2 = len3 = (NV_CTRL_DDCCI_LAST_VCP+1) << 2 ;
        len4 = rep.possible_val_len << 2;
        len5 = (NV_CTRL_STRING_LAST_ATTRIBUTE+1) << 2;
        *nvctrl_vcp_supported=(unsigned int *) Xmalloc(len1);
        *possible_values_offset=(unsigned int *) Xmalloc(len2);
        *possible_values_size=(unsigned int *) Xmalloc(len3);
        if(len4)
            *nvctrl_vcp_possible_values=(unsigned int *) Xmalloc(len4);
        *nvctrl_string_vcp_supported=(unsigned int *) Xmalloc(len5);
        memcpy((char*)*nvctrl_vcp_supported, p, len1);         p += len1;
        memcpy((char*)*possible_values_offset, p, len2);       p += len2;
        memcpy((char*)*possible_values_size, p, len3);         p += len3;
        if(len4) {
           memcpy((char*)*nvctrl_vcp_possible_values, p, len4);   p += len4;
        }
        memcpy((char*)*nvctrl_string_vcp_supported, p, len5); 
    }
    free(ptr);
    UnlockDisplay (dpy);
    SyncHandle ();
    return exists;
}

Bool XNVCTRLQueryDDCCITimingReport (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int *sync_freq_out_range, 
    unsigned int *unstable_count,
    unsigned int *positive_h_sync,
    unsigned int *positive_v_sync,
    unsigned int *h_freq,
    unsigned int *v_freq
) {
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryDDCCITimingReportReply rep;
    xnvCtrlQueryDDCCITimingReportReq *req;
    unsigned int buf[6];
    Bool exists;
    
    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);

    GetReq (nvCtrlQueryDDCCITimingReport, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryDDCCITimingReport;
    req->screen = screen;
    req->display_mask = display_mask;

    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }

    _XRead(dpy, (char *)(&buf), 24);
    
    exists = rep.flags;
    
    *sync_freq_out_range = buf[0];
    *unstable_count = buf[1];
    *positive_h_sync = buf[2];
    *positive_v_sync = buf[3];
    *h_freq = buf[4];
    *v_freq = buf[5];
    
    UnlockDisplay (dpy);
    SyncHandle ();

    return exists;
}

Bool XNVCTRLQueryTargetBinaryData (
    Display *dpy,
    int target_type,
    int target_id,
    unsigned int display_mask,
    unsigned int attribute,
    unsigned char **ptr,
    int *len
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryBinaryDataReply rep;
    xnvCtrlQueryBinaryDataReq   *req;
    Bool exists;
    int length, numbytes, slop;

    if (!ptr) return False;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryBinaryData, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryBinaryData;
    req->target_type = target_type;
    req->target_id = target_id;
    req->display_mask = display_mask;
    req->attribute = attribute;
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    length = rep.length;
    numbytes = rep.n;
    slop = numbytes & 3;
    *ptr = (char *) Xmalloc(numbytes);
    if (! *ptr) {
        _XEatData(dpy, length);
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    } else {
        _XRead(dpy, (char *) *ptr, numbytes);
        if (slop) _XEatData(dpy, 4-slop);
    }
    exists = rep.flags;
    if (len) *len = numbytes;
    UnlockDisplay (dpy);
    SyncHandle ();
    return exists;
}

Bool XNVCTRLQueryBinaryData (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int attribute,
    unsigned char **ptr,
    int *len
){
    return XNVCTRLQueryTargetBinaryData(dpy, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                        screen, display_mask,
                                        attribute, ptr, len);
}


static Bool wire_to_event (Display *dpy, XEvent *host, xEvent *wire)
{
    XExtDisplayInfo *info = find_display (dpy);
    XNVCtrlEvent *re;
    xnvctrlEvent *event;
    XNVCtrlEventTarget *reTarget;
    xnvctrlEventTarget *eventTarget;

    XNVCTRLCheckExtension (dpy, info, False);
    
    switch ((wire->u.u.type & 0x7F) - info->codes->first_event) {
    case ATTRIBUTE_CHANGED_EVENT:
        re = (XNVCtrlEvent *) host;
        event = (xnvctrlEvent *) wire;
        re->attribute_changed.type = event->u.u.type & 0x7F;
        re->attribute_changed.serial = 
            _XSetLastRequestRead(dpy, (xGenericReply*) event);
        re->attribute_changed.send_event = ((event->u.u.type & 0x80) != 0);
        re->attribute_changed.display = dpy;
        re->attribute_changed.time = event->u.attribute_changed.time;
        re->attribute_changed.screen = event->u.attribute_changed.screen;
        re->attribute_changed.display_mask =
            event->u.attribute_changed.display_mask;
        re->attribute_changed.attribute = event->u.attribute_changed.attribute;
        re->attribute_changed.value = event->u.attribute_changed.value;
        break;
    case TARGET_ATTRIBUTE_CHANGED_EVENT:
        reTarget = (XNVCtrlEventTarget *) host;
        eventTarget = (xnvctrlEventTarget *) wire;
        reTarget->attribute_changed.type = eventTarget->u.u.type & 0x7F;
        reTarget->attribute_changed.serial = 
            _XSetLastRequestRead(dpy, (xGenericReply*) eventTarget);
        reTarget->attribute_changed.send_event =
            ((eventTarget->u.u.type & 0x80) != 0);
        reTarget->attribute_changed.display = dpy;
        reTarget->attribute_changed.time =
            eventTarget->u.attribute_changed.time;
        reTarget->attribute_changed.target_type =
            eventTarget->u.attribute_changed.target_type;
        reTarget->attribute_changed.target_id =
            eventTarget->u.attribute_changed.target_id;
        reTarget->attribute_changed.display_mask =
            eventTarget->u.attribute_changed.display_mask;
        reTarget->attribute_changed.attribute =
            eventTarget->u.attribute_changed.attribute;
        reTarget->attribute_changed.value =
            eventTarget->u.attribute_changed.value;
        break;
    default:
        return False;
    }
    
    return True;
}

