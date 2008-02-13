#ifndef __NVCONTROL_H
#define __NVCONTROL_H

#define NV_CONTROL_ERRORS 0
#define NV_CONTROL_EVENTS 2
#define NV_CONTROL_NAME "NV-CONTROL"

#define NV_CONTROL_MAJOR 1
#define NV_CONTROL_MINOR 13

#define X_nvCtrlQueryExtension                      0
#define X_nvCtrlIsNv                                1
#define X_nvCtrlQueryAttribute                      2
#define X_nvCtrlSetAttribute                        3
#define X_nvCtrlQueryStringAttribute                4
#define X_nvCtrlQueryValidAttributeValues           5
#define X_nvCtrlSelectNotify                        6
#define X_nvCtrlSetGvoColorConversion_deprecated    7
#define X_nvCtrlQueryGvoColorConversion_deprecated  8
#define X_nvCtrlSetStringAttribute                  9
#define X_nvCtrlQueryDDCCILutSize                   10
#define X_nvCtrlQueryDDCCISinglePointLutOperation   11
#define X_nvCtrlSetDDCCISinglePointLutOperation     12
#define X_nvCtrlQueryDDCCIBlockLutOperation         13
#define X_nvCtrlSetDDCCIBlockLutOperation           14
#define X_nvCtrlSetDDCCIRemoteProcedureCall         15
#define X_nvCtrlQueryDDCCIDisplayControllerType     16
#define X_nvCtrlQueryDDCCICapabilities              17
#define X_nvCtrlQueryDDCCITimingReport              18
#define X_nvCtrlSetAttributeAndGetStatus            19
#define X_nvCtrlQueryBinaryData                     20
#define X_nvCtrlSetGvoColorConversion               21
#define X_nvCtrlQueryGvoColorConversion             22
#define X_nvCtrlSelectTargetNotify                  23
#define X_nvCtrlQueryTargetCount                    24
#define X_nvCtrlStringOperation                     25
#define X_nvCtrlLastRequest  (X_nvCtrlStringOperation + 1)


/* Define 32 bit floats */
typedef float FLOAT32;
#ifndef F32
#define F32
#endif


typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
} xnvCtrlQueryExtensionReq;
#define sz_xnvCtrlQueryExtensionReq 4

typedef struct {
    BYTE type;   /* X_Reply */
    CARD8 padb1;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD16 major B16;
    CARD16 minor B16;
    CARD32 padl4 B32;
    CARD32 padl5 B32;
    CARD32 padl6 B32;
    CARD32 padl7 B32;
    CARD32 padl8 B32;
} xnvCtrlQueryExtensionReply;
#define sz_xnvCtrlQueryExtensionReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
} xnvCtrlIsNvReq;
#define sz_xnvCtrlIsNvReq 8

typedef struct {
    BYTE type;   /* X_Reply */
    CARD8 padb1;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 isnv B32;
    CARD32 padl4 B32;
    CARD32 padl5 B32;
    CARD32 padl6 B32;
    CARD32 padl7 B32;
    CARD32 padl8 B32;
} xnvCtrlIsNvReply;
#define sz_xnvCtrlIsNvReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 target_type B32;
} xnvCtrlQueryTargetCountReq;
#define sz_xnvCtrlQueryTargetCountReq 8

typedef struct {
    BYTE type;   /* X_Reply */
    CARD8 padb1;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 count B32;
    CARD32 padl4 B32;
    CARD32 padl5 B32;
    CARD32 padl6 B32;
    CARD32 padl7 B32;
    CARD32 padl8 B32;
} xnvCtrlQueryTargetCountReply;
#define sz_xnvCtrlQueryTargetCountReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD16 target_id B16;    /* X screen number or GPU number */
    CARD16 target_type B16;  /* X screen or GPU */
    CARD32 display_mask B32;
    CARD32 attribute B32;
} xnvCtrlQueryAttributeReq;
#define sz_xnvCtrlQueryAttributeReq 16

typedef struct {
    BYTE type;
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    INT32  value B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xnvCtrlQueryAttributeReply;
#define sz_xnvCtrlQueryAttributeReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD16 target_id B16;
    CARD16 target_type B16;
    CARD32 display_mask B32;
    CARD32 attribute B32;
    INT32 value B32;
} xnvCtrlSetAttributeReq;
#define sz_xnvCtrlSetAttributeReq 20

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
    CARD32 attribute B32;
    INT32 value B32;
} xnvCtrlSetAttributeAndGetStatusReq;
#define sz_xnvCtrlSetAttributeAndGetStatusReq 20

typedef struct {
    BYTE type;
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xnvCtrlSetAttributeAndGetStatusReply;
#define sz_xnvCtrlSetAttributeAndGetStatusReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD16 target_id B16;    /* X screen number or GPU number */
    CARD16 target_type B16;  /* X screen or GPU */
    CARD32 display_mask B32;
    CARD32 attribute B32;
} xnvCtrlQueryStringAttributeReq;
#define sz_xnvCtrlQueryStringAttributeReq 16

typedef struct {
    BYTE type;
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    CARD32 n B32;    /* Length of string */
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xnvCtrlQueryStringAttributeReply;
#define sz_xnvCtrlQueryStringAttributeReply 32


typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
    CARD32 attribute B32;
    CARD32 num_bytes B32;
} xnvCtrlSetStringAttributeReq;
#define sz_xnvCtrlSetStringAttributeReq 20

typedef struct {
    BYTE type;
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xnvCtrlSetStringAttributeReply;
#define sz_xnvCtrlSetStringAttributeReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD16 target_id B16;    /* X screen number or GPU number */
    CARD16 target_type B16;  /* X screen or GPU */
    CARD32 display_mask B32;
    CARD32 attribute B32;
} xnvCtrlQueryValidAttributeValuesReq;
#define sz_xnvCtrlQueryValidAttributeValuesReq 16

typedef struct {
    BYTE type;
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    INT32  attr_type B32;
    INT32  min B32;
    INT32  max B32;
    CARD32 bits B32;
    CARD32 perms B32;
} xnvCtrlQueryValidAttributeValuesReply;
#define sz_xnvCtrlQueryValidAttributeValuesReply 32

/* Set GVO Color Conversion request (deprecated) */
typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    FLOAT32 row1_col1 F32;
    FLOAT32 row1_col2 F32;
    FLOAT32 row1_col3 F32;
    FLOAT32 row1_col4 F32;
    FLOAT32 row2_col1 F32;
    FLOAT32 row2_col2 F32;
    FLOAT32 row2_col3 F32;
    FLOAT32 row2_col4 F32;
    FLOAT32 row3_col1 F32;
    FLOAT32 row3_col2 F32;
    FLOAT32 row3_col3 F32;
    FLOAT32 row3_col4 F32;
} xnvCtrlSetGvoColorConversionDeprecatedReq;
#define sz_xnvCtrlSetGvoColorConversionDeprecatedReq 56

/* Query GVO Color Conversion request (deprecated) */
typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
} xnvCtrlQueryGvoColorConversionDeprecatedReq;
#define sz_xnvCtrlQueryGvoColorConversionDeprecatedReq 8

/* Query GVO Color Conversion reply (deprecated) */
typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlQueryGvoColorConversionDeprecatedReply;
#define sz_xnvCtrlQueryGvoColorConversionDeprecatedReply 32


/* Set GVO Color Conversion request */
typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;

    FLOAT32 cscMatrix_y_r F32;
    FLOAT32 cscMatrix_y_g F32;
    FLOAT32 cscMatrix_y_b F32;

    FLOAT32 cscMatrix_cr_r F32;
    FLOAT32 cscMatrix_cr_g F32;
    FLOAT32 cscMatrix_cr_b F32;

    FLOAT32 cscMatrix_cb_r F32;
    FLOAT32 cscMatrix_cb_g F32;
    FLOAT32 cscMatrix_cb_b F32;
    
    FLOAT32 cscOffset_y  F32;
    FLOAT32 cscOffset_cr F32;
    FLOAT32 cscOffset_cb F32;
    
    FLOAT32 cscScale_y  F32;
    FLOAT32 cscScale_cr F32;
    FLOAT32 cscScale_cb F32;
    
} xnvCtrlSetGvoColorConversionReq;
#define sz_xnvCtrlSetGvoColorConversionReq 68

/* Query GVO Color Conversion request */
typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
} xnvCtrlQueryGvoColorConversionReq;
#define sz_xnvCtrlQueryGvoColorConversionReq 8

/* Query GVO Color Conversion reply */
typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlQueryGvoColorConversionReply;
#define sz_xnvCtrlQueryGvoColorConversionReply 32


typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
} xnvCtrlQueryDDCCILutSizeReq;
#define sz_xnvCtrlQueryDDCCILutSizeReq 12

typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlQueryDDCCILutSizeReply;
#define sz_xnvCtrlQueryDDCCILutSizeReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
    CARD32 offset B32;
    CARD32 red_value B32;
    CARD32 green_value B32;
    CARD32 blue_value B32;
} xnvCtrlSetDDCCISinglePointLutOperationReq;
#define sz_xnvCtrlSetDDCCISinglePointLutOperationReq 28

typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlSetDDCCISinglePointLutOperationReply;
#define sz_xnvCtrlSetDDCCISinglePointLutOperationReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
    CARD32 offset B32;
} xnvCtrlQueryDDCCISinglePointLutOperationReq;
#define sz_xnvCtrlQueryDDCCISinglePointLutOperationReq 16

typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlQueryDDCCISinglePointLutOperationReply;
#define sz_xnvCtrlQueryDDCCISinglePointLutOperationReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
    CARD32 size B32;
    CARD32 color B32;
    CARD32 offset B32;
} xnvCtrlQueryDDCCIBlockLutOperationReq;
#define sz_xnvCtrlQueryDDCCIBlockLutOperationReq 24

typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 num_bytes B32;
    CARD32 flags B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlQueryDDCCIBlockLutOperationReply;
#define sz_xnvCtrlQueryDDCCIBlockLutOperationReply 32


typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
    CARD32 color B32;
    CARD32 offset B32;
    CARD32 size B32;
    CARD32 num_bytes B32;
} xnvCtrlSetDDCCIBlockLutOperationReq;
#define sz_xnvCtrlSetDDCCIBlockLutOperationReq 28

typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 num_bytes B32;
    CARD32 flags B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlSetDDCCIBlockLutOperationReply;
#define sz_xnvCtrlSetDDCCIBlockLutOperationReply 32


typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
    CARD32 num_bytes B32;
    CARD32 size B32;
    CARD32 offset B32;
} xnvCtrlSetDDCCIRemoteProcedureCallReq;
#define sz_xnvCtrlSetDDCCIRemoteProcedureCallReq 24

typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 num_bytes B32;
    CARD32 flags B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlSetDDCCIRemoteProcedureCallReply;
#define sz_xnvCtrlSetDDCCIRemoteProcedureCallReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
} xnvCtrlQueryDDCCIDisplayControllerTypeReq;
#define sz_xnvCtrlQueryDDCCIDisplayControllerTypeReq 12

typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 controller_type B32;
    CARD32 size B32;
    CARD32 flags B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xnvCtrlQueryDDCCIDisplayControllerTypeReply;
#define sz_xnvCtrlQueryDDCCIDisplayControllerTypeReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
} xnvCtrlQueryDDCCICapabilitiesReq;
#define sz_xnvCtrlQueryDDCCICapabilitiesReq 12

typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 num_bytes B32;
    CARD32 flags B32;
    CARD32 possible_val_len B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlQueryDDCCICapabilitiesReply;
#define sz_xnvCtrlQueryDDCCICapabilitiesReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD32 display_mask B32;
} xnvCtrlQueryDDCCITimingReportReq;
#define sz_xnvCtrlQueryDDCCITimingReportReq 12

typedef struct {
    BYTE type;   /* X_Reply */
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xnvCtrlQueryDDCCITimingReportReply;
#define sz_xnvCtrlQueryDDCCITimingReportReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD16 target_id B16;    /* X screen number or GPU number */
    CARD16 target_type B16;  /* X screen or GPU */
    CARD32 display_mask B32;
    CARD32 attribute B32;
} xnvCtrlQueryBinaryDataReq;
#define sz_xnvCtrlQueryBinaryDataReq 16

typedef struct {
    BYTE type;
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    CARD32 n B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xnvCtrlQueryBinaryDataReply;
#define sz_xnvCtrlQueryBinaryDataReply 32


typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD16 notifyType B16;
    CARD16 onoff B16;
} xnvCtrlSelectNotifyReq;
#define sz_xnvCtrlSelectNotifyReq 12

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD16 target_id B16;    /* X screen number or GPU number */
    CARD16 target_type B16;  /* X screen or GPU */
    CARD32 display_mask B32;
    CARD32 attribute B32;
    CARD32 num_bytes B32; /* Length of string */
} xnvCtrlStringOperationReq;
#define sz_xnvCtrlStringOperationReq 20

typedef struct {
    BYTE type;   /* X_Reply */
    CARD8 padb1;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 ret B32;
    CARD32 num_bytes B32; /* Length of string */
    CARD32 padl4 B32;
    CARD32 padl5 B32;
    CARD32 padl6 B32;
    CARD32 padl7 B32;
} xnvCtrlStringOperationReply;
#define sz_xnvCtrlStringOperationReply 32

typedef struct {
    union {
        struct {
            BYTE type;
            BYTE detail;
            CARD16 sequenceNumber B16;
        } u;
        struct {
            BYTE type;
            BYTE detail;
            CARD16 sequenceNumber B16;
            CARD32 time B32;
            CARD32 screen B32;
            CARD32 display_mask B32;
            CARD32 attribute B32;
            CARD32 value B32;
            CARD32 pad0 B32;
            CARD32 pad1 B32;
        } attribute_changed;
    } u;
} xnvctrlEvent;


/*
 * Leave target_type before target_id for the
 * xnvCtrlSelectTargetNotifyReq and xnvctrlEventTarget
 * structures, even though other request protocol structures
 * store target_id in the bottom 16-bits of the second DWORD of the
 * structures.  The event-related structures were added in version
 * 1.8, and so there is no prior version with which to maintain
 * compatibility.
 */
typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD16 target_type B16; /* Don't swap these */
    CARD16 target_id B16;
    CARD16 notifyType B16;
    CARD16 onoff B16;
} xnvCtrlSelectTargetNotifyReq;
#define sz_xnvCtrlSelectTargetNotifyReq 12

typedef struct {
    union {
        struct {
            BYTE type;
            BYTE detail;
            CARD16 sequenceNumber B16;
        } u;
        struct {
            BYTE type;
            BYTE detail;
            CARD16 sequenceNumber B16;
            CARD32 time B32;
            CARD16 target_type B16; /* Don't swap these */
            CARD16 target_id B16;
            CARD32 display_mask B32;
            CARD32 attribute B32;
            CARD32 value B32;
            CARD32 pad0 B32;
            CARD32 pad1 B32;
        } attribute_changed;
    } u;
} xnvctrlEventTarget;


#endif /* __NVCONTROL_H */
