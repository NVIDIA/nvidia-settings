/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2014 NVIDIA Corporation.
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
 *  NVML backend
 */

#include <stdlib.h> /* 64 bit malloc */
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"

#include "msg.h"
#include "parse.h"

#include "NVCtrlLib.h"

#include "nvml.h"


#define MAX_NVML_STR_LEN 64

static inline const NvCtrlNvmlAttributes *
getNvmlHandleConst(const NvCtrlAttributePrivateHandle *h)
{
    if ((h == NULL) || (h->nvml == NULL)) {
        return NULL;
    }

    return h->nvml;
}

static void printNvmlError(nvmlReturn_t error)
{
    switch (error) {
        case NVML_SUCCESS:
            break;

        case NVML_ERROR_UNINITIALIZED:
            nv_error_msg("NVML was not first initialized with nvmlInit()");
            break;

        case NVML_ERROR_INVALID_ARGUMENT:
            nv_error_msg("A supplied argument is invalid");
            break;

        case NVML_ERROR_NOT_SUPPORTED:
            break;

        case NVML_ERROR_NO_PERMISSION:
            nv_error_msg("The current user does not have permission for "
                         "operation");
            break;

        case NVML_ERROR_ALREADY_INITIALIZED:
            nv_error_msg("Deprecated: Multiple initializations are now allowed "
                         "through ref counting");
            break;

        case NVML_ERROR_NOT_FOUND:
            nv_error_msg("A query to find an object was unsuccessful");
            break;

        case NVML_ERROR_INSUFFICIENT_SIZE:
            nv_error_msg("An input argument is not large enough");
            break;

        case NVML_ERROR_INSUFFICIENT_POWER:
            nv_error_msg("A device's external power cables are not properly "
                         "attached");
            break;

        case NVML_ERROR_DRIVER_NOT_LOADED:
            nv_error_msg("NVIDIA driver is not loaded");
            break;

        case NVML_ERROR_TIMEOUT:
            nv_error_msg("User provided timeout passed");
            break;

        case NVML_ERROR_IRQ_ISSUE:
            nv_error_msg("NVIDIA Kernel detected an interrupt issue with a "
                         "GPU");
            break;

        case NVML_ERROR_LIBRARY_NOT_FOUND:
            nv_error_msg("NVML Shared Library couldn't be found or loaded");
            break;

        case NVML_ERROR_FUNCTION_NOT_FOUND:
            nv_error_msg("Local version of NVML doesn't implement this "
                         "function");
            break;

        case NVML_ERROR_CORRUPTED_INFOROM:
            nv_error_msg("infoROM is corrupted");
            break;

        case NVML_ERROR_GPU_IS_LOST:
            nv_error_msg("The GPU has fallen off the bus or has otherwise "
                         "become inaccessible");
            break;

        case NVML_ERROR_RESET_REQUIRED:
            nv_error_msg("The GPU requires a reset before it can be used "
                         "again");
            break;

        case NVML_ERROR_OPERATING_SYSTEM:
            nv_error_msg("The GPU control device has been blocked by the "
                         "operating system/cgroups");
            break;

        default:
            nv_error_msg("An internal driver error occurred");
            break;
    }
}


static Bool NvmlMissing(const CtrlTarget *ctrl_target)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (h == NULL) {
        return False;
    }

    return (h->nvml == NULL);
}


/*
 * Unload the NVML library if it was successfully loaded.
 */
static void UnloadNvml(NvCtrlNvmlAttributes *nvml)
{
    if (nvml == NULL) {
        return;
    }

    if (nvml->lib.handle == NULL) {
        return;
    }

    if (nvml->lib.shutdown != NULL) {
        nvmlReturn_t ret = nvml->lib.shutdown();
        if (ret != NVML_SUCCESS) {
            printNvmlError(ret);
        }
    }

    dlclose(nvml->lib.handle);

    memset(&nvml->lib, 0, sizeof(nvml->lib));
}

/*
 * Load and initializes the NVML library.
 */
static Bool LoadNvml(NvCtrlNvmlAttributes *nvml)
{
    nvmlReturn_t ret;

    nvml->lib.handle = dlopen("libnvidia-ml.so.1", RTLD_LAZY);

    if (nvml->lib.handle == NULL) {
        goto fail;
    }

#define GET_SYMBOL_REQUIRED(_proc, _name)             \
    nvml->lib._proc = dlsym(nvml->lib.handle, _name); \
    if (nvml->lib._proc == NULL) {                    \
        goto fail;                                    \
    }

    GET_SYMBOL_REQUIRED(init,                           "nvmlInit");
    GET_SYMBOL_REQUIRED(shutdown,                       "nvmlShutdown");
    GET_SYMBOL_REQUIRED(deviceGetHandleByIndex,         "nvmlDeviceGetHandleByIndex");
    GET_SYMBOL_REQUIRED(deviceGetUUID,                  "nvmlDeviceGetUUID");
    GET_SYMBOL_REQUIRED(deviceGetCount,                 "nvmlDeviceGetCount");
    GET_SYMBOL_REQUIRED(deviceGetTemperature,           "nvmlDeviceGetTemperature");
    GET_SYMBOL_REQUIRED(deviceGetName,                  "nvmlDeviceGetName");
    GET_SYMBOL_REQUIRED(deviceGetVbiosVersion,          "nvmlDeviceGetVbiosVersion");
    GET_SYMBOL_REQUIRED(deviceGetMemoryInfo,            "nvmlDeviceGetMemoryInfo");
    GET_SYMBOL_REQUIRED(deviceGetPciInfo,               "nvmlDeviceGetPciInfo");
    GET_SYMBOL_REQUIRED(deviceGetCurrPcieLinkWidth,     "nvmlDeviceGetCurrPcieLinkWidth");
    GET_SYMBOL_REQUIRED(deviceGetMaxPcieLinkGeneration, "nvmlDeviceGetMaxPcieLinkGeneration");
    GET_SYMBOL_REQUIRED(deviceGetMaxPcieLinkWidth,      "nvmlDeviceGetMaxPcieLinkWidth");
    GET_SYMBOL_REQUIRED(deviceGetVirtualizationMode,    "nvmlDeviceGetVirtualizationMode");
    GET_SYMBOL_REQUIRED(deviceGetUtilizationRates,      "nvmlDeviceGetUtilizationRates");
    GET_SYMBOL_REQUIRED(deviceGetTemperatureThreshold,  "nvmlDeviceGetTemperatureThreshold");
    GET_SYMBOL_REQUIRED(deviceGetFanSpeed_v2,           "nvmlDeviceGetFanSpeed_v2");
    GET_SYMBOL_REQUIRED(systemGetDriverVersion,         "nvmlSystemGetDriverVersion");
    GET_SYMBOL_REQUIRED(deviceGetEccMode,               "nvmlDeviceGetEccMode");
    GET_SYMBOL_REQUIRED(deviceSetEccMode,               "nvmlDeviceSetEccMode");
    GET_SYMBOL_REQUIRED(deviceGetTotalEccErrors,        "nvmlDeviceGetTotalEccErrors");
    GET_SYMBOL_REQUIRED(deviceClearEccErrorCounts,      "nvmlDeviceClearEccErrorCounts");
    GET_SYMBOL_REQUIRED(systemGetNVMLVersion,           "nvmlSystemGetNVMLVersion");
    GET_SYMBOL_REQUIRED(deviceGetMemoryErrorCounter,    "nvmlDeviceGetMemoryErrorCounter");
    GET_SYMBOL_REQUIRED(deviceGetNumGpuCores,           "nvmlDeviceGetNumGpuCores");
    GET_SYMBOL_REQUIRED(deviceGetMemoryBusWidth,        "nvmlDeviceGetMemoryBusWidth");
    GET_SYMBOL_REQUIRED(deviceGetIrqNum,                "nvmlDeviceGetIrqNum");
    GET_SYMBOL_REQUIRED(deviceGetPowerSource,           "nvmlDeviceGetPowerSource");
    GET_SYMBOL_REQUIRED(deviceGetNumFans,               "nvmlDeviceGetNumFans");
    GET_SYMBOL_REQUIRED(deviceGetDefaultEccMode,        "nvmlDeviceGetDefaultEccMode");
#undef GET_SYMBOL_REQUIRED
    
/* Do not fail with older drivers */
#define GET_SYMBOL_OPTIONAL(_proc, _name)             \
    nvml->lib._proc = dlsym(nvml->lib.handle, _name); 
    
    GET_SYMBOL_OPTIONAL(deviceGetGridLicensableFeatures, "nvmlDeviceGetGridLicensableFeatures_v4");
    GET_SYMBOL_OPTIONAL(deviceGetGspFirmwareMode,        "nvmlDeviceGetGspFirmwareMode");
    GET_SYMBOL_OPTIONAL(deviceGetMemoryInfo_v2,          "nvmlDeviceGetMemoryInfo_v2");
    GET_SYMBOL_OPTIONAL(deviceSetFanSpeed_v2,            "nvmlDeviceSetFanSpeed_v2");
    GET_SYMBOL_OPTIONAL(deviceGetTargetFanSpeed,         "nvmlDeviceGetTargetFanSpeed");
    GET_SYMBOL_OPTIONAL(deviceGetMinMaxFanSpeed,         "nvmlDeviceGetMinMaxFanSpeed");
    GET_SYMBOL_OPTIONAL(deviceSetFanControlPolicy,       "nvmlDeviceSetFanControlPolicy");
    GET_SYMBOL_OPTIONAL(deviceGetFanControlPolicy_v2,    "nvmlDeviceGetFanControlPolicy_v2");
#undef GET_SYMBOL_OPTIONAL

    ret = nvml->lib.init();

    if (ret != NVML_SUCCESS) {
        printNvmlError(ret);
        goto fail;
    }

    return True;

fail:
    UnloadNvml(nvml);
    return False;
}


/*
 * Creates and fills an IDs dictionary so we can translate from NV-CONTROL IDs
 * to NVML indexes
 *
 * XXX Needed while using NV-CONTROL as fallback during the migration process
 */

static Bool matchNvCtrlWithNvmlIds(const NvCtrlNvmlAttributes *nvml,
                                   const NvCtrlAttributePrivateHandle *h,
                                   int nvmlGpuCount,
                                   unsigned int **idsDictionary)
{
    char nvmlUUID[MAX_NVML_STR_LEN];
    char *nvctrlUUID = NULL;
    nvmlDevice_t device;
    int i, j;
    int nvctrlGpuCount = 0;

    /* Get the gpu count returned by NV-CONTROL. */
    if (h->nv && !XNVCTRLQueryTargetCount(h->dpy, NV_CTRL_TARGET_TYPE_GPU,
                                          &nvctrlGpuCount)) {
        return FALSE;
    }

    *idsDictionary = nvalloc(nvmlGpuCount * sizeof(unsigned int));

    /* Fallback case is to use same id either for NV-CONTROL and NVML */
    for (i = 0; i < nvmlGpuCount; i++) {
        (*idsDictionary)[i] = i;
    }

    if (h->nv != NULL) {
        for (i = 0; i < nvctrlGpuCount; i++) {
            Bool gpuUUIDMatchFound = FALSE;

            /* Get GPU UUID through NV-CONTROL */
            if (!XNVCTRLQueryTargetStringAttribute(h->dpy,
                                                   NV_CTRL_TARGET_TYPE_GPU,
                                                   i, 0,
                                                   NV_CTRL_STRING_GPU_UUID,
                                                   &nvctrlUUID)) {
                goto fail;
            }

            if (nvctrlUUID == NULL) {
                goto fail;
            }

            /* Look for the same UUID through NVML */
            for (j = 0; j < nvmlGpuCount; j++) {
                if (NVML_SUCCESS != nvml->lib.deviceGetHandleByIndex(j, &device)) {
                    continue;
                }

                if (NVML_SUCCESS != nvml->lib.deviceGetUUID(device, nvmlUUID,
                                                            MAX_NVML_STR_LEN)) {
                    continue;
                }

                if (strcmp(nvctrlUUID, nvmlUUID) == 0) {
                    /* We got a match */
                    gpuUUIDMatchFound = TRUE;
                    (*idsDictionary)[i] = j;
                    break;
                }
            }

            XFree(nvctrlUUID);

            /* Fail if mismatch between gpu UUID returned by NV-CONTROL and
             * NVML
             */
            if (!gpuUUIDMatchFound) {
                goto fail;
            }
        }
    }
    return TRUE;

fail:
    nvfree(*idsDictionary);
    return FALSE;
}



/*
 * Initializes an NVML private handle to hold some information to be used later
 * on
 */

NvCtrlNvmlAttributes *NvCtrlInitNvmlAttributes(NvCtrlAttributePrivateHandle *h)
{
    NvCtrlNvmlAttributes *nvml = NULL;
    unsigned int count;
    unsigned int *nvctrlToNvmlId;
    int i;
    int nvctrlCoolerCount;

    /* Check parameters */
    if (h == NULL || !TARGET_TYPE_IS_NVML_COMPATIBLE(h->target_type)) {
        goto fail;
    }

    /* Create storage for NVML attributes */
    nvml = nvalloc(sizeof(NvCtrlNvmlAttributes));

    if (!LoadNvml(nvml)) {
        goto fail;
    }

    /* Initialize NVML attributes */
    if (nvml->lib.deviceGetCount(&count) != NVML_SUCCESS) {
        goto fail;
    }
    nvml->deviceCount = count;

    nvml->sensorCountPerGPU = nvalloc(count * sizeof(unsigned int));
    nvml->sensorCount = 0;
    nvml->coolerCountPerGPU = nvalloc(count * sizeof(unsigned int));
    nvml->coolerCount = 0;

    /* Fill the NV-CONTROL to NVML IDs dictionary */
    if (!matchNvCtrlWithNvmlIds(nvml, h, count, &nvctrlToNvmlId)) {
        goto fail;
    }

    /*
     * Fill 'sensorCountPerGPU', 'coolerCountPerGPU' and properly set
     * 'deviceIdx'
     */
    nvml->deviceIdx = h->target_id; /* Fallback */

    if (h->target_type == GPU_TARGET) {
        nvml->deviceIdx = nvctrlToNvmlId[h->target_id];
    }

    for (i = 0; i < count; i++) {
        int devIdx = nvctrlToNvmlId[i];
        nvmlDevice_t device;
        nvmlReturn_t ret = nvml->lib.deviceGetHandleByIndex(devIdx, &device);
        if (ret == NVML_SUCCESS) {
            unsigned int temp;
            unsigned int fans;

            /*
             * XXX Currently, NVML only allows to get the GPU temperature so
             *     check for nvmlDeviceGetTemperature() success to figure
             *     out if that sensor is available.
             */
            ret = nvml->lib.deviceGetTemperature(device, NVML_TEMPERATURE_GPU,
                                                 &temp);
            if (ret == NVML_SUCCESS) {
                if ((h->target_type == THERMAL_SENSOR_TARGET) &&
                    (h->target_id == nvml->sensorCount)) {

                    nvml->deviceIdx = devIdx;
                }

                nvml->sensorCountPerGPU[i] = 1;
                nvml->sensorCount++;
            }

            ret = nvml->lib.deviceGetNumFans(device, &fans);
            if (ret == NVML_SUCCESS) {
                if ((h->target_type == COOLER_TARGET) &&
                    (h->target_id == nvml->coolerCount)) {

                    nvml->deviceIdx = devIdx;
                }

                nvml->coolerCountPerGPU[i] = fans;
                nvml->coolerCount += fans;
            }
        }
    }

    /*
     * Consistency check between X/NV-CONTROL and NVML.
     */
    if (h->nv &&
        (!XNVCTRLQueryTargetCount(h->dpy, NV_CTRL_TARGET_TYPE_COOLER,
                                   &nvctrlCoolerCount) ||
         (nvctrlCoolerCount != nvml->coolerCount))) {
        nv_warning_msg("Inconsistent number of fans detected.");
    }

    nvfree(nvctrlToNvmlId);

    return nvml;

 fail:
    UnloadNvml(nvml);
    nvfree(nvml->sensorCountPerGPU);
    nvfree(nvml->coolerCountPerGPU);
    nvfree(nvml);
    return NULL;
}



/*
 * Frees any resource hold by the NVML private handle
 */

void NvCtrlNvmlAttributesClose(NvCtrlAttributePrivateHandle *h)
{
    /* Check parameters */
    if (h == NULL || h->nvml == NULL) {
        return;
    }

    UnloadNvml(h->nvml);
    nvfree(h->nvml->sensorCountPerGPU);
    nvfree(h->nvml->coolerCountPerGPU);
    nvfree(h->nvml);
    h->nvml = NULL;
}



/*
 * Get the number of 'target_type' targets according to NVML
 */

ReturnStatus NvCtrlNvmlQueryTargetCount(const CtrlTarget *ctrl_target,
                                        int target_type, int *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);

    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    /*
     * This shouldn't be reached for target types that are not handled through
     * NVML (Keep TARGET_TYPE_IS_NVML_COMPATIBLE in NvCtrlAttributesPrivate.h up
     * to date).
     */
    assert(TARGET_TYPE_IS_NVML_COMPATIBLE(target_type));

    if ((h == NULL) || (h->nvml == NULL)) {
        return NvCtrlBadHandle;
    }

    switch (target_type) {
        case GPU_TARGET:
            *val = (int)(h->nvml->deviceCount);
            break;
        case THERMAL_SENSOR_TARGET:
            *val = (int)(h->nvml->sensorCount);
            break;
        case COOLER_TARGET:
            *val = (int)(h->nvml->coolerCount);
            break;
        default:
            return NvCtrlBadArgument;
    }

    return NvCtrlSuccess;
}



/*
 * Get NVML String Attribute Values that do not require a control target.
 */

static ReturnStatus NvCtrlNvmlGetGeneralStringAttribute(const CtrlTarget *ctrl_target,
                                                        int attr, char **ptr)
{
    char res[MAX_NVML_STR_LEN];
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    nvmlReturn_t ret;
    *ptr = NULL;

    if ((h == NULL) || (h->nvml == NULL)) {
        return NvCtrlBadHandle;
    }

    switch (attr) {
        case NV_CTRL_STRING_NVIDIA_DRIVER_VERSION:
            ret = h->nvml->lib.systemGetDriverVersion(res, MAX_NVML_STR_LEN);
            break;

        case NV_CTRL_STRING_NVML_VERSION:
            ret = h->nvml->lib.systemGetNVMLVersion(res, MAX_NVML_STR_LEN);
            break;

        default:
            /* This is expected to fall through silently. */
            return NvCtrlNotSupported;
    }

    if (ret == NVML_SUCCESS) {
        *ptr = strdup(res);
        return NvCtrlSuccess;
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}



/*
 * Get NVML String Attribute Values
 */

static ReturnStatus NvCtrlNvmlGetGPUStringAttribute(const CtrlTarget *ctrl_target,
                                                    int attr, char **ptr)
{
    char res[MAX_NVML_STR_LEN];
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    nvmlDevice_t device;
    nvmlReturn_t ret;
    *ptr = NULL;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    ret = nvml->lib.deviceGetHandleByIndex(nvml->deviceIdx, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_STRING_PRODUCT_NAME:
                ret = nvml->lib.deviceGetName(device, res, MAX_NVML_STR_LEN);
                break;

            case NV_CTRL_STRING_VBIOS_VERSION:
                ret = nvml->lib.deviceGetVbiosVersion(device, res, MAX_NVML_STR_LEN);
                break;

            case NV_CTRL_STRING_GPU_UUID:
                ret = nvml->lib.deviceGetUUID(device, res, MAX_NVML_STR_LEN);
                break;

            case NV_CTRL_STRING_GPU_UTILIZATION:
            {
                nvmlUtilization_t util;

                if (ctrl_target->system->has_nv_control) {
                    /*
                     * Not all utilization types are currently available via
                     * NVML so, if accessible, get the rates from X.
                     */
                    return NvCtrlNotSupported;
                }

                ret = nvml->lib.deviceGetUtilizationRates(device, &util);

                if (ret != NVML_SUCCESS) {
                    break;
                }

                snprintf(res, sizeof(res),
                         "graphics=%d, memory=%d",
                         util.gpu, util.memory);

                break;
            }
            case NV_CTRL_STRING_SLI_MODE:
            case NV_CTRL_STRING_PERFORMANCE_MODES:
            case NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS:
            case NV_CTRL_STRING_MULTIGPU_MODE:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlNotSupported;

            default:
                /* Did we forget to handle a GPU string attribute? */
                nv_warning_msg("Unhandled string attribute %s (%d) of GPU (%d)",
                               STR_ATTRIBUTE_NAME(attr), attr,
                               NvCtrlGetTargetId(ctrl_target));
                return NvCtrlNotSupported;
        }

        if (ret == NVML_SUCCESS) {
            *ptr = strdup(res);
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}



ReturnStatus NvCtrlNvmlGetStringAttribute(const CtrlTarget *ctrl_target,
                                          int attr, char **ptr)
{
    ReturnStatus ret;

    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    /*
     * Check for attributes that don't require a target, else continue to
     * supported target types.
     */
    ret = NvCtrlNvmlGetGeneralStringAttribute(ctrl_target, attr, ptr);

    if (ret == NvCtrlSuccess) {
        return NvCtrlSuccess;
    }

    switch (NvCtrlGetTargetType(ctrl_target)) {
        case GPU_TARGET:
            return NvCtrlNvmlGetGPUStringAttribute(ctrl_target, attr, ptr);

        case THERMAL_SENSOR_TARGET:
            /* Did we forget to handle a sensor string attribute? */
            nv_warning_msg("Unhandled string attribute %s (%d) of Thermal "
                           "sensor (%d)", STR_ATTRIBUTE_NAME(attr), attr,
                           NvCtrlGetTargetId(ctrl_target));
            return NvCtrlNotSupported;

        case COOLER_TARGET:
            /* Did we forget to handle a cooler string attribute? */
            nv_warning_msg("Unhandled string attribute %s (%d) of Fan (%d)",
                           STR_ATTRIBUTE_NAME(attr), attr,
                           NvCtrlGetTargetId(ctrl_target));
            return NvCtrlNotSupported;

        default:
            return NvCtrlBadHandle;
    }
}



/*
 * Set NVML String Attribute Values
 */



static ReturnStatus NvCtrlNvmlSetGPUStringAttribute(CtrlTarget *ctrl_target,
                                                    int attr, const char *ptr)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    ret = nvml->lib.deviceGetHandleByIndex(nvml->deviceIdx, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlNotSupported;

            default:
                /* Did we forget to handle a GPU string attribute? */
                nv_warning_msg("Unhandled string attribute %s (%d) of GPU (%d) "
                               "(set to '%s')", STR_ATTRIBUTE_NAME(attr), attr,
                               NvCtrlGetTargetId(ctrl_target), (ptr ? ptr : ""));
                return NvCtrlNotSupported;
        }

        if (ret == NVML_SUCCESS) {
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}



ReturnStatus NvCtrlNvmlSetStringAttribute(CtrlTarget *ctrl_target,
                                          int attr, const char *ptr)
{
    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    /*
     * This shouldn't be reached for target types that are not handled through
     * NVML (Keep TARGET_TYPE_IS_NVML_COMPATIBLE in NvCtrlAttributesPrivate.h up
     * to date).
     */
    assert(TARGET_TYPE_IS_NVML_COMPATIBLE(NvCtrlGetTargetType(ctrl_target)));

    switch (NvCtrlGetTargetType(ctrl_target)) {
        case GPU_TARGET:
            return NvCtrlNvmlSetGPUStringAttribute(ctrl_target, attr, ptr);

        case THERMAL_SENSOR_TARGET:
            /* Did we forget to handle a sensor string attribute? */
            nv_warning_msg("Unhandled string attribute %s (%d) of Thermal "
                           "sensor (%d) (set to '%s')",
                           STR_ATTRIBUTE_NAME(attr), attr,
                           NvCtrlGetTargetId(ctrl_target), (ptr ? ptr : ""));
            return NvCtrlNotSupported;

        case COOLER_TARGET:
            /* Did we forget to handle a cooler string attribute? */
            nv_warning_msg("Unhandled string attribute %s (%d) of Fan (%d) "
                           "(set to '%s')", STR_ATTRIBUTE_NAME(attr), attr,
                           NvCtrlGetTargetId(ctrl_target), (ptr ? ptr : ""));
            return NvCtrlNotSupported;

        default:
            return NvCtrlBadHandle;
    }
}



/*
 * Get NVML Attribute Values
 */

static ReturnStatus NvCtrlNvmlGetGPUAttribute(const CtrlTarget *ctrl_target,
                                              int attr, int64_t *val)
{
    unsigned int res = 0;
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    ret = nvml->lib.deviceGetHandleByIndex(nvml->deviceIdx, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY:
            case NV_CTRL_USED_DEDICATED_GPU_MEMORY:
                {
                    if (nvml->lib.deviceGetMemoryInfo_v2) {
                        nvmlMemory_v2_t memory;
                        memory.version = nvmlMemory_v2;
                        ret = nvml->lib.deviceGetMemoryInfo_v2(device, &memory);
                        if (ret == NVML_SUCCESS) {
                            switch (attr) {
                                case NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY:
                                    res = memory.total >> 20; // bytes --> MB
                                    break;
                                case NV_CTRL_USED_DEDICATED_GPU_MEMORY:
                                    res = memory.used >> 20; // bytes --> MB
                                    break;
                            }
                        }
                    } else {
                        nvmlMemory_t memory;
                        ret = nvml->lib.deviceGetMemoryInfo(device, &memory);
                        if (ret == NVML_SUCCESS) {
                            switch (attr) {
                                case NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY:
                                    res = memory.total >> 20; // bytes --> MB
                                    break;
                                case NV_CTRL_USED_DEDICATED_GPU_MEMORY:
                                    res = memory.used >> 20; // bytes --> MB
                                    break;
                            }
                        }
                    }
                }
                break;

            case NV_CTRL_PCI_DOMAIN:
            case NV_CTRL_PCI_BUS:
            case NV_CTRL_PCI_DEVICE:
            case NV_CTRL_PCI_FUNCTION:
            case NV_CTRL_PCI_ID:
                {
                    nvmlPciInfo_t pci;
                    ret = nvml->lib.deviceGetPciInfo(device, &pci);
                    if (ret == NVML_SUCCESS) {
                        switch (attr) {
                            case NV_CTRL_PCI_DOMAIN:
                                res = pci.domain;
                                break;
                            case NV_CTRL_PCI_BUS:
                                res = pci.bus;
                                break;
                            case NV_CTRL_PCI_DEVICE:
                                res = pci.device;
                                break;
                            case NV_CTRL_PCI_FUNCTION:
                                {
                                    char *f = strrchr(pci.busId, '.');
                                    if (f != NULL) {
                                        res = atoi(f + 1);
                                    }
                                    else {
                                        res = 0;
                                    }
                                }
                                break;
                            case NV_CTRL_PCI_ID:
                                res = ((pci.pciDeviceId << 16) & 0xffff0000) |
                                      ((pci.pciDeviceId >> 16) & 0x0000ffff);
                                break;
                        }
                    }
                }
                break;

            case NV_CTRL_GPU_PCIE_GENERATION:
                ret = nvml->lib.deviceGetMaxPcieLinkGeneration(device, &res);
                break;

            case NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH:
                ret = nvml->lib.deviceGetCurrPcieLinkWidth(device, &res);
                break;
            case NV_CTRL_GPU_PCIE_MAX_LINK_WIDTH:
                ret = nvml->lib.deviceGetMaxPcieLinkWidth(device, &res);
                break;
            case NV_CTRL_GPU_SLOWDOWN_THRESHOLD:
                ret = nvml->lib.deviceGetTemperatureThreshold(device,
                          NVML_TEMPERATURE_THRESHOLD_SLOWDOWN ,&res);
                break;
            case NV_CTRL_GPU_SHUTDOWN_THRESHOLD:
                ret = nvml->lib.deviceGetTemperatureThreshold(device,
                          NVML_TEMPERATURE_THRESHOLD_SHUTDOWN ,&res);
                break;
            case NV_CTRL_GPU_CORE_TEMPERATURE:
                ret = nvml->lib.deviceGetTemperature(device,
                                                     NVML_TEMPERATURE_GPU,
                                                     &res);
                break;

            case NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED:
            case NV_CTRL_GPU_ECC_SUPPORTED:
                {
                    nvmlEnableState_t current, pending;
                    ret = nvml->lib.deviceGetEccMode(device, &current, &pending);
                    switch (attr) {
                        case NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED:
                            res = (ret == NVML_SUCCESS) ?
                                NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED_TRUE :
                                NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED_FALSE;
                            break;
                        case NV_CTRL_GPU_ECC_SUPPORTED:
                            res = (ret == NVML_SUCCESS) ?
                                NV_CTRL_GPU_ECC_SUPPORTED_TRUE :
                                NV_CTRL_GPU_ECC_SUPPORTED_FALSE;
                            break;
                    }

                    // Avoid printing misleading error message
                    if (ret == NVML_ERROR_NOT_SUPPORTED) { 
                        ret = NVML_SUCCESS;
                    }
                }
                break;

            case NV_CTRL_GPU_ECC_CONFIGURATION:
            case NV_CTRL_GPU_ECC_STATUS:
                {
                    nvmlEnableState_t current, pending;
                    ret = nvml->lib.deviceGetEccMode(device, &current, &pending);
                    if (ret == NVML_SUCCESS) {
                        switch (attr) {
                            case NV_CTRL_GPU_ECC_STATUS:
                                res = current;
                                break;
                            case NV_CTRL_GPU_ECC_CONFIGURATION:
                                res = pending;
                                break;
                        }
                    }
                }
                break;

            case NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION:
                {
                    nvmlEnableState_t defaultMode;
                    ret = nvml->lib.deviceGetDefaultEccMode(device, &defaultMode);
                    if (ret == NVML_SUCCESS) {
                        res = defaultMode;
                    }
                }
                break;

            case NV_CTRL_GPU_ECC_SINGLE_BIT_ERRORS:
            case NV_CTRL_GPU_ECC_AGGREGATE_SINGLE_BIT_ERRORS:
            case NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS:
            case NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS:
                {
                    unsigned long long eccCounts;
                    nvmlEccCounterType_t counterType;
                    nvmlMemoryErrorType_t errorType;
                    switch (attr) {
                        case NV_CTRL_GPU_ECC_SINGLE_BIT_ERRORS:
                            errorType = NVML_MEMORY_ERROR_TYPE_CORRECTED;
                            counterType = NVML_VOLATILE_ECC;
                            break;
                        case NV_CTRL_GPU_ECC_AGGREGATE_SINGLE_BIT_ERRORS:
                            errorType = NVML_MEMORY_ERROR_TYPE_CORRECTED;
                            counterType = NVML_AGGREGATE_ECC;
                            break;
                        case NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS:
                            errorType = NVML_MEMORY_ERROR_TYPE_UNCORRECTED;
                            counterType = NVML_VOLATILE_ECC;
                            break;
                        case NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS:
                            errorType = NVML_MEMORY_ERROR_TYPE_UNCORRECTED;
                            counterType = NVML_AGGREGATE_ECC;
                            break;
                    }

                    ret = nvml->lib.deviceGetTotalEccErrors(device, errorType,
                                                        counterType, &eccCounts);
                    if (ret == NVML_SUCCESS) {
                        if (val) {
                            *val = eccCounts;
                        }
                        return NvCtrlSuccess;
                    }
                }
                break;

            case NV_CTRL_GPU_CORES:
                ret = nvml->lib.deviceGetNumGpuCores(device, &res);
                break;
            case NV_CTRL_GPU_MEMORY_BUS_WIDTH:
                ret = nvml->lib.deviceGetMemoryBusWidth(device, &res);
                break;
            case NV_CTRL_IRQ:
                ret = nvml->lib.deviceGetIrqNum(device, &res);
                break;
            case NV_CTRL_GPU_POWER_SOURCE:
                assert(NV_CTRL_GPU_POWER_SOURCE_AC == NVML_POWER_SOURCE_AC);
                assert(NV_CTRL_GPU_POWER_SOURCE_BATTERY == NVML_POWER_SOURCE_BATTERY);
                ret = nvml->lib.deviceGetPowerSource(device, &res);
                break;

            case NV_CTRL_GPU_COOLER_MANUAL_CONTROL:
                {
                    nvmlFanControlPolicy_t policy;

                    /* Return early if GPU has no fan */
                    if (nvml->coolerCount == 0) {
                        return NvCtrlNotSupported;
                    }

                    /* Get cooler control policy */
                    ret = nvml->lib.deviceGetFanControlPolicy_v2(device, 0, &policy);
                    res = (policy == NVML_FAN_POLICY_MANUAL) ?
                        NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE :
                        NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE;
                }
                break;

            case NV_CTRL_VIDEO_RAM:
            case NV_CTRL_GPU_PCIE_MAX_LINK_SPEED:
            case NV_CTRL_GPU_PCIE_CURRENT_LINK_SPEED:
            case NV_CTRL_BUS_TYPE:
            case NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL:
            case NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE:
            case NV_CTRL_GPU_POWER_MIZER_MODE:
            case NV_CTRL_GPU_POWER_MIZER_DEFAULT_MODE:
            case NV_CTRL_ENABLED_DISPLAYS:
            case NV_CTRL_CONNECTED_DISPLAYS:
            case NV_CTRL_MAX_SCREEN_WIDTH:
            case NV_CTRL_MAX_SCREEN_HEIGHT:
            case NV_CTRL_MAX_DISPLAYS:
            case NV_CTRL_DEPTH_30_ALLOWED:
            case NV_CTRL_MULTIGPU_PRIMARY_POSSIBLE:
            case NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE:
            case NV_CTRL_BASE_MOSAIC:
            case NV_CTRL_XINERAMA:
            case NV_CTRL_ATTR_NV_MAJOR_VERSION:
            case NV_CTRL_ATTR_NV_MINOR_VERSION:
            case NV_CTRL_OPERATING_SYSTEM:
            case NV_CTRL_NO_SCANOUT:
            case NV_CTRL_AMBIENT_TEMPERATURE:
            case NV_CTRL_GPU_CURRENT_CLOCK_FREQS:
            case NV_CTRL_VIDEO_ENCODER_UTILIZATION:
            case NV_CTRL_VIDEO_DECODER_UTILIZATION:
            case NV_CTRL_FRAMELOCK:
            case NV_CTRL_DITHERING:
            case NV_CTRL_CURRENT_DITHERING:
            case NV_CTRL_DITHERING_MODE:
            case NV_CTRL_CURRENT_DITHERING_MODE:
            case NV_CTRL_DITHERING_DEPTH:
            case NV_CTRL_CURRENT_DITHERING_DEPTH:
            case NV_CTRL_DIGITAL_VIBRANCE:
            case NV_CTRL_IMAGE_SHARPENING_DEFAULT:
            case NV_CTRL_REFRESH_RATE:
            case NV_CTRL_REFRESH_RATE_3:
            case NV_CTRL_COLOR_SPACE:
            case NV_CTRL_COLOR_RANGE:
            case NV_CTRL_SYNCHRONOUS_PALETTE_UPDATES:
            case NV_CTRL_DPY_HDMI_3D:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlNotSupported;

            case NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE:
                {
                    nvmlGpuVirtualizationMode_t mode;
                    ret = nvml->lib.deviceGetVirtualizationMode(device, &mode);
                    res = mode;
                }
                break;

            case NV_CTRL_ATTR_NVML_GPU_GRID_LICENSE_SUPPORTED:
                if (nvml->lib.deviceGetGridLicensableFeatures) {
                    nvmlGridLicensableFeatures_t gridLicensableFeatures;
                    ret = nvml->lib.deviceGetGridLicensableFeatures(device,
                                                          &gridLicensableFeatures);
                    res = !!(gridLicensableFeatures.isGridLicenseSupported);
                } else {
                    /* return NvCtrlNotSupported against older driver */
                    ret = NVML_ERROR_FUNCTION_NOT_FOUND;
                }

                break;

            default:
                /* Did we forget to handle a GPU integer attribute? */
                nv_info_msg("", "Unhandled integer attribute %s (%d) of GPU "
                            "(%d)", INT_ATTRIBUTE_NAME(attr), attr,
                            NvCtrlGetTargetId(ctrl_target));
                return NvCtrlNotSupported;
        }

        if (ret == NVML_SUCCESS) {
            if (val) {
                *val = res;
            }
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}

static ReturnStatus NvCtrlNvmlGetGridLicensableFeatures(const CtrlTarget *ctrl_target,
                                                    int attr, nvmlGridLicensableFeatures_t **val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    ret = nvml->lib.deviceGetHandleByIndex(nvml->deviceIdx, &device);
        if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_ATTR_NVML_GPU_GRID_LICENSABLE_FEATURES:
                if (nvml->lib.deviceGetGridLicensableFeatures) {
                    nvmlGridLicensableFeatures_t *gridLicensableFeatures;
                    gridLicensableFeatures = (nvmlGridLicensableFeatures_t *)nvalloc(sizeof(nvmlGridLicensableFeatures_t));
                    ret = nvml->lib.deviceGetGridLicensableFeatures(device,
                                                                    gridLicensableFeatures);
                    if (ret == NVML_SUCCESS) {
                        *val = gridLicensableFeatures;
                        return NvCtrlSuccess;
                    }
                } else {
                    /* return NvCtrlNotSupported against older driver */
                    ret = NVML_ERROR_FUNCTION_NOT_FOUND;
                }

                break;

            default:
                /* Did we forget to handle a GPU integer attribute? */
                nv_warning_msg("Unhandled integer attribute %s (%d) of GPU "
                               "(%d)", INT_ATTRIBUTE_NAME(attr), attr,
                               NvCtrlGetTargetId(ctrl_target));
                return NvCtrlNotSupported;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}

static ReturnStatus NvCtrlNvmlDeviceGetGspFeatures(const CtrlTarget *ctrl_target, int attr,
                                                       unsigned int *isEnabled, unsigned int *defaultMode)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    ret = nvml->lib.deviceGetHandleByIndex(nvml->deviceIdx, &device);
    if (ret == NVML_SUCCESS) {
    switch (attr) {
        case NV_CTRL_ATTR_NVML_GSP_FIRMWARE_MODE:
            if (nvml->lib.deviceGetGspFirmwareMode) {
                unsigned int isEnabled_t = 0;
                unsigned int defaultMode_t = 0;
                ret = nvml->lib.deviceGetGspFirmwareMode(device,
                                                         &isEnabled_t, &defaultMode_t);
                if (ret == NVML_SUCCESS) {
                    *isEnabled = isEnabled_t;
                    *defaultMode = defaultMode_t;
                    return NvCtrlSuccess;
                }
            } else {
                /* return NvCtrlNotSupported against older driver */
                ret = NVML_ERROR_FUNCTION_NOT_FOUND;
            }

            break;

        default:
            /* Did we forget to handle a GPU integer attribute? */
            nv_warning_msg("Unhandled integer attribute %s (%d) of GPU "
                           "(%d)", INT_ATTRIBUTE_NAME(attr), attr,
                           NvCtrlGetTargetId(ctrl_target));
            return NvCtrlNotSupported;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}

static void getDeviceAndTargetIndex(const NvCtrlAttributePrivateHandle *h,
                                    unsigned int targetCount,
                                    const unsigned int *targetCountPerGPU,
                                    int *deviceIdx, int *targetIdx)
{
    int i, count;
    *targetIdx = -1;

    if ((h->target_id < 0) || (h->target_id >= targetCount)) {
        return;
    }

    count = 0;
    for (i = 0; i < h->nvml->deviceCount; i++) {
        int tmp = count + targetCountPerGPU[i];
        *deviceIdx = i;
        if (h->target_id < tmp) {
            *targetIdx = (h->target_id - count);
            return;
        }
        count = tmp;
    }

    return;
}


static ReturnStatus NvCtrlNvmlGetThermalAttribute(const CtrlTarget *ctrl_target,
                                                  int attr, int64_t *val)
{
    unsigned int res;
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    int deviceId, sensorId;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    /* Get the proper device according to the sensor ID */
    getDeviceAndTargetIndex(h, nvml->sensorCount, nvml->sensorCountPerGPU,
                            &deviceId, &sensorId);
    if (sensorId == -1) {
        return NvCtrlBadHandle;
    }


    ret = nvml->lib.deviceGetHandleByIndex(deviceId, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_THERMAL_SENSOR_READING:
               
            case NV_CTRL_THERMAL_SENSOR_PROVIDER:
            case NV_CTRL_THERMAL_SENSOR_TARGET:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlNotSupported;

            default:
                /* Did we forget to handle a sensor integer attribute? */
                nv_info_msg("", "Unhandled integer attribute %s (%d) of "
                            "Thermal sensor (%d)", INT_ATTRIBUTE_NAME(attr),
                            attr, NvCtrlGetTargetId(ctrl_target));
                return NvCtrlNotSupported;
        }

        if (ret == NVML_SUCCESS) {
            *val = res;
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}

static ReturnStatus NvCtrlNvmlGetCoolerAttribute(const CtrlTarget *ctrl_target,
                                                 int attr, int64_t *val)
{
    unsigned int res;
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    int deviceId, coolerId;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    /* Get the proper device according to the cooler ID */
    getDeviceAndTargetIndex(h, nvml->coolerCount, nvml->coolerCountPerGPU,
                            &deviceId, &coolerId);
    if (coolerId == -1) {
        return NvCtrlBadHandle;
    }

    ret = nvml->lib.deviceGetHandleByIndex(deviceId, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_THERMAL_COOLER_LEVEL:
                ret = nvml->lib.deviceGetTargetFanSpeed(device, coolerId, &res);
                break;
            case NV_CTRL_THERMAL_COOLER_CURRENT_LEVEL:
                ret = nvml->lib.deviceGetFanSpeed_v2(device, coolerId, &res);
                break;

            case NV_CTRL_THERMAL_COOLER_SPEED:
            case NV_CTRL_THERMAL_COOLER_CONTROL_TYPE:
            case NV_CTRL_THERMAL_COOLER_TARGET:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlNotSupported;

            default:
                /* Did we forget to handle a cooler integer attribute? */
                nv_info_msg("", "Unhandled integer attribute %s (%d) of Fan "
                            "(%d)", INT_ATTRIBUTE_NAME(attr), attr,
                            NvCtrlGetTargetId(ctrl_target));
                return NvCtrlNotSupported;
        }

        if (ret == NVML_SUCCESS) {
            *val = res;
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}



ReturnStatus NvCtrlNvmlGetAttribute(const CtrlTarget *ctrl_target,
                                    int attr, int64_t *val)
{
    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    /*
     * This shouldn't be reached for target types that are not handled through
     * NVML (Keep TARGET_TYPE_IS_NVML_COMPATIBLE in NvCtrlAttributesPrivate.h up
     * to date).
     */
    assert(TARGET_TYPE_IS_NVML_COMPATIBLE(NvCtrlGetTargetType(ctrl_target)));

    switch (NvCtrlGetTargetType(ctrl_target)) {
        case GPU_TARGET:
            return NvCtrlNvmlGetGPUAttribute(ctrl_target, attr, val);
        case THERMAL_SENSOR_TARGET:
            return NvCtrlNvmlGetThermalAttribute(ctrl_target, attr, val);
        case COOLER_TARGET:
            return NvCtrlNvmlGetCoolerAttribute(ctrl_target, attr, val);
        default:
            return NvCtrlNotSupported;
    }
}


ReturnStatus NvCtrlNvmlGetGridLicenseAttributes(const CtrlTarget *ctrl_target,
                                                int attr, nvmlGridLicensableFeatures_t **val)
{
    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    /*
     * This shouldn't be reached for target types that are not handled through
     * NVML (Keep TARGET_TYPE_IS_NVML_COMPATIBLE in NvCtrlAttributesPrivate.h up
     * to date).
     */
    assert(TARGET_TYPE_IS_NVML_COMPATIBLE(NvCtrlGetTargetType(ctrl_target)));

    if (NvCtrlGetTargetType(ctrl_target) == GPU_TARGET) {
        return NvCtrlNvmlGetGridLicensableFeatures(ctrl_target, attr, val);
    }
    else {
            return NvCtrlBadHandle;
    }
}


ReturnStatus NvCtrlNvmlDeviceGetGspAttributes(const CtrlTarget *ctrl_target, int attr,
                                              unsigned int *isEnabled, unsigned int *defaultMode)
{
    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    /*
     * This shouldn't be reached for target types that are not handled through
     * NVML (Keep TARGET_TYPE_IS_NVML_COMPATIBLE in NvCtrlAttributesPrivate.h up
     * to date).
     */
    assert(TARGET_TYPE_IS_NVML_COMPATIBLE(NvCtrlGetTargetType(ctrl_target)));

    if (NvCtrlGetTargetType(ctrl_target) == GPU_TARGET) {
        return NvCtrlNvmlDeviceGetGspFeatures(ctrl_target, attr, isEnabled, defaultMode);
    }
    else {
            return NvCtrlBadHandle;
    }
}


/*
 * Set NVML Attribute Values
 */



static ReturnStatus NvCtrlNvmlSetGPUAttribute(CtrlTarget *ctrl_target,
                                              int attr, int index, int val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    ret = nvml->lib.deviceGetHandleByIndex(nvml->deviceIdx, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_GPU_ECC_CONFIGURATION:
                ret = nvml->lib.deviceSetEccMode(device, val);
                break;

            case NV_CTRL_GPU_ECC_RESET_ERROR_STATUS:
                {
                    nvmlEccCounterType_t counterType = 0;
                    switch (val) {
                        case NV_CTRL_GPU_ECC_RESET_ERROR_STATUS_VOLATILE:
                            counterType = NVML_VOLATILE_ECC;
                            break;
                        case NV_CTRL_GPU_ECC_RESET_ERROR_STATUS_AGGREGATE:
                            counterType = NVML_AGGREGATE_ECC;
                            break;
                    }
                    ret = nvml->lib.deviceClearEccErrorCounts(device,
                                                              counterType);
                }
                break;

            case NV_CTRL_GPU_COOLER_MANUAL_CONTROL:
                {
                    int i = 0;
                    int count = nvml->coolerCountPerGPU[nvml->deviceIdx];

                    for (i = 0; i < count; i++) {
                        ret = nvml->lib.deviceSetFanControlPolicy(device, i, val);
                    }
                }
                break;

            case NV_CTRL_GPU_CURRENT_CLOCK_FREQS:
            case NV_CTRL_GPU_POWER_MIZER_MODE:
            case NV_CTRL_DITHERING:
            case NV_CTRL_DITHERING_MODE:
            case NV_CTRL_DITHERING_DEPTH:
            case NV_CTRL_DIGITAL_VIBRANCE:
            case NV_CTRL_COLOR_SPACE:
            case NV_CTRL_COLOR_RANGE:
            case NV_CTRL_SYNCHRONOUS_PALETTE_UPDATES:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlNotSupported;

            default:
                /* Did we forget to handle a GPU integer attribute? */
                nv_info_msg("", "Unhandled integer attribute %s (%d) of GPU "
                            "(%d) (set to %d)", INT_ATTRIBUTE_NAME(attr),
                            attr, NvCtrlGetTargetId(ctrl_target), val);
                return NvCtrlNotSupported;
        }

        if (ret == NVML_SUCCESS) {
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}

static ReturnStatus NvCtrlNvmlSetCoolerAttribute(CtrlTarget *ctrl_target,
                                                 int attr, int val)
{
    NvCtrlAttributePrivateHandle *h = getPrivateHandle(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    int deviceId, coolerId;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    /* Get the proper device according to the cooler ID */
    getDeviceAndTargetIndex(h, nvml->coolerCount, nvml->coolerCountPerGPU,
                            &deviceId, &coolerId);
    if (coolerId == -1) {
        return NvCtrlBadHandle;
    }

    ret = nvml->lib.deviceGetHandleByIndex(deviceId, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_THERMAL_COOLER_LEVEL:
                ret = nvml->lib.deviceSetFanSpeed_v2(device, coolerId, val);
                break;

            case NV_CTRL_THERMAL_COOLER_LEVEL_SET_DEFAULT:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlNotSupported;

            default:
                /* Did we forget to handle a cooler integer attribute? */
                nv_info_msg("", "Unhandled integer attribute %s (%d) of Fan "
                            "(%d) (set to %d)", INT_ATTRIBUTE_NAME(attr),
                            attr, NvCtrlGetTargetId(ctrl_target), val);
                return NvCtrlNotSupported;
        }

        if (ret == NVML_SUCCESS) {
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}



ReturnStatus NvCtrlNvmlSetAttribute(CtrlTarget *ctrl_target, int attr,
                                    int index, int val)
{
    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    /*
     * This shouldn't be reached for target types that are not handled through
     * NVML (Keep TARGET_TYPE_IS_NVML_COMPATIBLE in NvCtrlAttributesPrivate.h up
     * to date).
     */
    assert(TARGET_TYPE_IS_NVML_COMPATIBLE(NvCtrlGetTargetType(ctrl_target)));

    switch (NvCtrlGetTargetType(ctrl_target)) {
        case GPU_TARGET:
            return NvCtrlNvmlSetGPUAttribute(ctrl_target, attr, index, val);

        case THERMAL_SENSOR_TARGET:
            /* Did we forget to handle a sensor integer attribute? */
            nv_info_msg("", "Unhandled integer attribute %s (%d) of Thermal "
                        "sensor (%d) (set to %d)", INT_ATTRIBUTE_NAME(attr),
                        attr, NvCtrlGetTargetId(ctrl_target), val);
            return NvCtrlNotSupported;

        case COOLER_TARGET:
            return NvCtrlNvmlSetCoolerAttribute(ctrl_target, attr, val);
        default:
            return NvCtrlBadHandle;
    }
}



static nvmlReturn_t getDeviceMemoryCounts(const CtrlTarget *ctrl_target,
                                          const NvCtrlNvmlAttributes *nvml,
                                          nvmlDevice_t device,
                                          nvmlMemoryErrorType_t errorType,
                                          nvmlEccCounterType_t counterType,
                                          unsigned char **data, int *len)
{
    unsigned long long count;
    int *counts = (int *) nvalloc(sizeof(int) * NVML_MEMORY_LOCATION_COUNT);
    nvmlReturn_t ret, anySuccess = NVML_ERROR_NOT_SUPPORTED;
    int i;

    for (i = NVML_MEMORY_LOCATION_L1_CACHE;
         i < NVML_MEMORY_LOCATION_COUNT;
         i++) {

        ret = nvml->lib.deviceGetMemoryErrorCounter(device, errorType,
                                                    counterType, i, &count);
        if (ret == NVML_SUCCESS) {
            anySuccess = NVML_SUCCESS;
            counts[i] = (int) count;
        } else {
            counts[i] = -1;
        }
    }

    *data = (unsigned char *) counts;
    *len  = sizeof(int) * NVML_MEMORY_LOCATION_COUNT;

    return anySuccess;
}

/*
 * Get NVML Binary Attribute Values
 */



static ReturnStatus
NvCtrlNvmlGetGPUBinaryAttribute(const CtrlTarget *ctrl_target,
                                int attr, unsigned char **data, int *len)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    ret = nvml->lib.deviceGetHandleByIndex(nvml->deviceIdx, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU:
            {
                unsigned int *fan_data;
                unsigned int count = 0;
                int offset = 0;
                int i = 0;

                ret = nvml->lib.deviceGetNumFans(device, &count);
                if (ret != NVML_SUCCESS) {
                    return NvCtrlNotSupported;
                }

                /*
                 * The format of the returned data is:
                 *
                 * int       unsigned int number of COOLERS
                 * int * n   unsigned int COOLER indices
                 */

                *len = (count + 1) * sizeof(unsigned int);
                fan_data = (unsigned int *) nvalloc(*len);
                memset(fan_data, 0, *len);
                *data = (unsigned char *) fan_data;

                /* Calculate global fan index offset for this GPU */
                for (i = 0; i < nvml->deviceIdx; i++) {
                    offset += nvml->coolerCountPerGPU[i];
                }

                fan_data[0] = count;
                for (i = 0; i < count; i++) {
                    fan_data[i+1] = i + offset;
                }
                break;
            }
            case NV_CTRL_BINARY_DATA_GPU_ECC_DETAILED_ERRORS_SINGLE_BIT:
                ret = getDeviceMemoryCounts(ctrl_target, nvml, device,
                                            NVML_MEMORY_ERROR_TYPE_CORRECTED,
                                            NVML_VOLATILE_ECC,
                                            data, len);
                break;
            case NV_CTRL_BINARY_DATA_GPU_ECC_DETAILED_ERRORS_DOUBLE_BIT:
                ret = getDeviceMemoryCounts(ctrl_target, nvml, device,
                                            NVML_MEMORY_ERROR_TYPE_UNCORRECTED,
                                            NVML_VOLATILE_ECC,
                                            data, len);
                break;
            case NV_CTRL_BINARY_DATA_GPU_ECC_DETAILED_ERRORS_SINGLE_BIT_AGGREGATE:
                ret = getDeviceMemoryCounts(ctrl_target, nvml, device,
                                            NVML_MEMORY_ERROR_TYPE_CORRECTED,
                                            NVML_AGGREGATE_ECC,
                                            data, len);
                break;
            case NV_CTRL_BINARY_DATA_GPU_ECC_DETAILED_ERRORS_DOUBLE_BIT_AGGREGATE:
                ret = getDeviceMemoryCounts(ctrl_target, nvml, device,
                                            NVML_MEMORY_ERROR_TYPE_UNCORRECTED,
                                            NVML_AGGREGATE_ECC,
                                            data, len);
                break;
            case NV_CTRL_BINARY_DATA_FRAMELOCKS_USED_BY_GPU:
            case NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU:
            case NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU:
            case NV_CTRL_BINARY_DATA_DISPLAYS_ON_GPU:
            case NV_CTRL_BINARY_DATA_GPU_FLAGS:
            case NV_CTRL_BINARY_DATA_XSCREENS_USING_GPU:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlNotSupported;

            default:
                /* Did we forget to handle a GPU binary attribute? */
                nv_warning_msg("Unhandled binary attribute %s (%d) of GPU (%d)",
                               BIN_ATTRIBUTE_NAME(attr), attr,
                               NvCtrlGetTargetId(ctrl_target));
                return NvCtrlNotSupported;
        }

        if (ret == NVML_SUCCESS) {
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNotSupported;
}



ReturnStatus
NvCtrlNvmlGetBinaryAttribute(const CtrlTarget *ctrl_target,
                             int attr, unsigned char **data, int *len)
{
    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    /*
     * This shouldn't be reached for target types that are not handled through
     * NVML (Keep TARGET_TYPE_IS_NVML_COMPATIBLE in NvCtrlAttributesPrivate.h up
     * to date).
     */
    assert(TARGET_TYPE_IS_NVML_COMPATIBLE(NvCtrlGetTargetType(ctrl_target)));

    switch (NvCtrlGetTargetType(ctrl_target)) {
        case GPU_TARGET:
            return NvCtrlNvmlGetGPUBinaryAttribute(ctrl_target,
                                                   attr,
                                                   data,
                                                   len);

        case THERMAL_SENSOR_TARGET:
            /* Did we forget to handle a sensor binary attribute? */
            nv_warning_msg("Unhandled binary attribute %s (%d) of Thermal "
                           "sensor (%d)", BIN_ATTRIBUTE_NAME(attr), attr,
                           NvCtrlGetTargetId(ctrl_target));
            return NvCtrlNotSupported;

        case COOLER_TARGET:
            /* Did we forget to handle a cooler binary attribute? */
            nv_warning_msg("Unhandled binary attribute %s (%d) of Fan (%d)",
                           BIN_ATTRIBUTE_NAME(attr), attr,
                           NvCtrlGetTargetId(ctrl_target));
            return NvCtrlNotSupported;

        default:
            return NvCtrlBadHandle;
    }
}



/*
 * Get NVML Valid String Attribute Values
 */



static ReturnStatus
NvCtrlNvmlGetGPUValidStringAttributeValues(int attr,
                                           CtrlAttributeValidValues *val)
{
    switch (attr) {
        case NV_CTRL_STRING_NVIDIA_DRIVER_VERSION:
        case NV_CTRL_STRING_PRODUCT_NAME:
        case NV_CTRL_STRING_VBIOS_VERSION:
        case NV_CTRL_STRING_GPU_UUID:
            val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_STRING;
            return NvCtrlSuccess;

        case NV_CTRL_STRING_SLI_MODE:
        case NV_CTRL_STRING_PERFORMANCE_MODES:
        case NV_CTRL_STRING_MULTIGPU_MODE:
        case NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS:
        case NV_CTRL_STRING_GPU_UTILIZATION:
            /*
             * XXX We'll eventually need to add support for this attributes. For
             *     string attributes, NV-CONTROL only sets the attribute type
             *     and permissions so no actual NVML call will be needed.
             */
            return NvCtrlAttributeNotAvailable;

        default:
            /* The attribute queried is not GPU-targeted */
            return NvCtrlAttributeNotAvailable;
    }

    return NvCtrlSuccess;
}



static ReturnStatus convertValidTypeToAttributeType(CtrlAttributeValidType val,
                                                   CtrlAttributeType *type)
{
    switch (val) {
        case CTRL_ATTRIBUTE_VALID_TYPE_INTEGER:
        case CTRL_ATTRIBUTE_VALID_TYPE_BOOL:
        case CTRL_ATTRIBUTE_VALID_TYPE_BITMASK:
        case CTRL_ATTRIBUTE_VALID_TYPE_64BIT_INTEGER:
        case CTRL_ATTRIBUTE_VALID_TYPE_RANGE:
            *type = CTRL_ATTRIBUTE_TYPE_INTEGER;
            break;
        case CTRL_ATTRIBUTE_VALID_TYPE_STRING:
            *type = CTRL_ATTRIBUTE_TYPE_STRING;
            break;
        case CTRL_ATTRIBUTE_VALID_TYPE_STRING_OPERATION:
            *type = CTRL_ATTRIBUTE_TYPE_STRING_OPERATION;
            break;
        case CTRL_ATTRIBUTE_VALID_TYPE_BINARY_DATA:
            *type = CTRL_ATTRIBUTE_TYPE_BINARY_DATA;
            break;
        default:
            return NvCtrlAttributeNotAvailable;
    }

    return NvCtrlSuccess;
}



ReturnStatus
NvCtrlNvmlGetValidStringAttributeValues(const CtrlTarget *ctrl_target,
                                        int attr,
                                        CtrlAttributeValidValues *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    ReturnStatus ret = NvCtrlAttributeNotAvailable;
    CtrlAttributeType attr_type;

    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    if (val) {
        memset(val, 0, sizeof(*val));
    } else {
        return NvCtrlBadArgument;
    }

    switch (NvCtrlGetTargetType(ctrl_target)) {
        case GPU_TARGET:
            ret = NvCtrlNvmlGetGPUValidStringAttributeValues(attr, val);
            break;

        case THERMAL_SENSOR_TARGET:
        case COOLER_TARGET:
            /* The attribute queried is not sensor nor fan-targeted */
            ret = NvCtrlAttributeNotAvailable;
            break;

        default:
            ret = NvCtrlBadHandle;
            break;
    }

    if (ret == NvCtrlSuccess) {
        val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_STRING;
    }

    ret = convertValidTypeToAttributeType(val->valid_type, &attr_type);

    if (ret != NvCtrlSuccess) {
        return ret;
    }

    return NvCtrlNvmlGetAttributePerms(h, attr_type, attr, &val->permissions);
}



/*
 * Get NVML Valid Attribute Values
 */



static ReturnStatus
NvCtrlNvmlGetGPUValidAttributeValues(const CtrlTarget *ctrl_target, int attr,
                                     CtrlAttributeValidValues *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    val->permissions.write = NV_FALSE;

    ret = nvml->lib.deviceGetHandleByIndex(nvml->deviceIdx, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY:
            case NV_CTRL_USED_DEDICATED_GPU_MEMORY:
            case NV_CTRL_PCI_DOMAIN:
            case NV_CTRL_PCI_BUS:
            case NV_CTRL_PCI_DEVICE:
            case NV_CTRL_PCI_FUNCTION:
            case NV_CTRL_PCI_ID:
            case NV_CTRL_GPU_PCIE_GENERATION:
            case NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH:
            case NV_CTRL_GPU_PCIE_MAX_LINK_WIDTH:
            case NV_CTRL_GPU_SLOWDOWN_THRESHOLD:
            case NV_CTRL_GPU_SHUTDOWN_THRESHOLD:
            case NV_CTRL_GPU_CORE_TEMPERATURE:
            case NV_CTRL_GPU_MEMORY_BUS_WIDTH:
            case NV_CTRL_GPU_CORES:
            case NV_CTRL_IRQ:
            case NV_CTRL_GPU_POWER_SOURCE:
                val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_INTEGER;
                break;

            case NV_CTRL_GPU_ECC_SUPPORTED:
            case NV_CTRL_GPU_ECC_STATUS:
            case NV_CTRL_GPU_ECC_CONFIGURATION:
            case NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED:
            case NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION:
                val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_BOOL;
                break;

            case NV_CTRL_GPU_ECC_SINGLE_BIT_ERRORS:
            case NV_CTRL_GPU_ECC_AGGREGATE_SINGLE_BIT_ERRORS:
            case NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS:
            case NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS:
                val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_64BIT_INTEGER;
                break;

            case NV_CTRL_GPU_ECC_RESET_ERROR_STATUS:
                val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_BITMASK;
                break;

            case NV_CTRL_GPU_COOLER_MANUAL_CONTROL:
                val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_BOOL;
                break;

            case NV_CTRL_VIDEO_RAM:
            case NV_CTRL_GPU_PCIE_MAX_LINK_SPEED:
            case NV_CTRL_GPU_PCIE_CURRENT_LINK_SPEED:
            case NV_CTRL_BUS_TYPE:
            case NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL:
            case NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE:
            case NV_CTRL_GPU_POWER_MIZER_MODE:
            case NV_CTRL_GPU_POWER_MIZER_DEFAULT_MODE:
            case NV_CTRL_ENABLED_DISPLAYS:
            case NV_CTRL_CONNECTED_DISPLAYS:
            case NV_CTRL_MAX_SCREEN_WIDTH:
            case NV_CTRL_MAX_SCREEN_HEIGHT:
            case NV_CTRL_MAX_DISPLAYS:
            case NV_CTRL_DEPTH_30_ALLOWED:
            case NV_CTRL_MULTIGPU_PRIMARY_POSSIBLE:
            case NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE:
            case NV_CTRL_BASE_MOSAIC:
            case NV_CTRL_XINERAMA:
            case NV_CTRL_ATTR_NV_MAJOR_VERSION:
            case NV_CTRL_ATTR_NV_MINOR_VERSION:
            case NV_CTRL_OPERATING_SYSTEM:
            case NV_CTRL_NO_SCANOUT:
            case NV_CTRL_AMBIENT_TEMPERATURE:
            case NV_CTRL_GPU_CURRENT_CLOCK_FREQS:
            case NV_CTRL_VIDEO_ENCODER_UTILIZATION:
            case NV_CTRL_VIDEO_DECODER_UTILIZATION:
            case NV_CTRL_FRAMELOCK:
            case NV_CTRL_DITHERING:
            case NV_CTRL_CURRENT_DITHERING:
            case NV_CTRL_DITHERING_MODE:
            case NV_CTRL_CURRENT_DITHERING_MODE:
            case NV_CTRL_DITHERING_DEPTH:
            case NV_CTRL_CURRENT_DITHERING_DEPTH:
            case NV_CTRL_DIGITAL_VIBRANCE:
            case NV_CTRL_IMAGE_SHARPENING_DEFAULT:
            case NV_CTRL_REFRESH_RATE:
            case NV_CTRL_REFRESH_RATE_3:
            case NV_CTRL_COLOR_SPACE:
            case NV_CTRL_COLOR_RANGE:
            case NV_CTRL_SYNCHRONOUS_PALETTE_UPDATES:
            case NV_CTRL_DPY_HDMI_3D:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlAttributeNotAvailable;

            default:
                /* The attribute queried is not GPU-targeted */
                return NvCtrlAttributeNotAvailable;
        }

        if (ret == NVML_SUCCESS) {
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNoAttribute;
}

static ReturnStatus
NvCtrlNvmlGetThermalValidAttributeValues(const CtrlTarget *ctrl_target,
                                         int attr,
                                         CtrlAttributeValidValues *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    int deviceId, sensorId;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    /* Get the proper device and sensor ID according to the target ID */
    getDeviceAndTargetIndex(h, nvml->sensorCount, nvml->sensorCountPerGPU,
                            &deviceId, &sensorId);
    if (sensorId == -1) {
        return NvCtrlBadHandle;
    }


    ret = nvml->lib.deviceGetHandleByIndex(deviceId, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_THERMAL_SENSOR_READING:
            case NV_CTRL_THERMAL_SENSOR_PROVIDER:
            case NV_CTRL_THERMAL_SENSOR_TARGET:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlAttributeNotAvailable;

            default:
                /* The attribute queried is not sensor-targeted */
                return NvCtrlAttributeNotAvailable;
        }

        if (ret == NVML_SUCCESS) {
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNoAttribute;
}

static ReturnStatus
NvCtrlNvmlGetCoolerValidAttributeValues(const CtrlTarget *ctrl_target,
                                        int attr,
                                        CtrlAttributeValidValues *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    const NvCtrlNvmlAttributes *nvml;
    int deviceId, coolerId;
    unsigned int minSpeed, maxSpeed;
    nvmlDevice_t device;
    nvmlReturn_t ret;

    nvml = getNvmlHandleConst(h);
    if (nvml == NULL) {
        return NvCtrlBadHandle;
    }

    /* Get the proper device and cooler ID according to the target ID */
    getDeviceAndTargetIndex(h, nvml->coolerCount, nvml->coolerCountPerGPU,
                            &deviceId, &coolerId);
    if (coolerId == -1) {
        return NvCtrlBadHandle;
    }


    ret = nvml->lib.deviceGetHandleByIndex(deviceId, &device);
    if (ret == NVML_SUCCESS) {
        switch (attr) {
            case NV_CTRL_THERMAL_COOLER_CURRENT_LEVEL:
                /* Range as a percent */
                val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_RANGE;
                val->range.min = 0;
                val->range.max = 100;
                return NvCtrlSuccess;

            case NV_CTRL_THERMAL_COOLER_LEVEL:
                ret = nvml->lib.deviceGetMinMaxFanSpeed(device, &minSpeed, &maxSpeed);
                if (ret == NVML_SUCCESS) {
                    /* Range as a percent */
                    val->valid_type = CTRL_ATTRIBUTE_VALID_TYPE_RANGE;
                    val->range.min = minSpeed;
                    val->range.max = maxSpeed;
                }
                break;

            case NV_CTRL_THERMAL_COOLER_SPEED:
            case NV_CTRL_THERMAL_COOLER_CONTROL_TYPE:
            case NV_CTRL_THERMAL_COOLER_TARGET:
                /*
                 * XXX We'll eventually need to add support for this attributes
                 *     through NVML
                 */
                return NvCtrlAttributeNotAvailable;

            default:
                /* The attribute queried is not fan-targeted */
                return NvCtrlAttributeNotAvailable;
        }

        if (ret == NVML_SUCCESS) {
            return NvCtrlSuccess;
        }
    }

    /* An NVML error occurred */
    printNvmlError(ret);
    return NvCtrlNoAttribute;
}



ReturnStatus
NvCtrlNvmlGetValidAttributeValues(const CtrlTarget *ctrl_target,
                                  int attr,
                                  CtrlAttributeValidValues *val)
{
    const NvCtrlAttributePrivateHandle *h = getPrivateHandleConst(ctrl_target);
    ReturnStatus ret = NvCtrlAttributeNotAvailable;
    CtrlAttributeType attr_type;

    if (NvmlMissing(ctrl_target)) {
        return NvCtrlMissingExtension;
    }

    /*
     * This shouldn't be reached for target types that are not handled through
     * NVML (Keep TARGET_TYPE_IS_NVML_COMPATIBLE in NvCtrlAttributesPrivate.h up
     * to date).
     */
    assert(TARGET_TYPE_IS_NVML_COMPATIBLE(NvCtrlGetTargetType(ctrl_target)));

    switch (NvCtrlGetTargetType(ctrl_target)) {
        case GPU_TARGET:
            ret = NvCtrlNvmlGetGPUValidAttributeValues(ctrl_target,
                                                       attr,
                                                       val);
            break;

        case THERMAL_SENSOR_TARGET:
            ret = NvCtrlNvmlGetThermalValidAttributeValues(ctrl_target,
                                                           attr,
                                                           val);
            break;

        case COOLER_TARGET:
            ret = NvCtrlNvmlGetCoolerValidAttributeValues(ctrl_target,
                                                          attr,
                                                          val);
            break;

        default:
            ret = NvCtrlBadHandle;
    }

    if (ret != NvCtrlSuccess) {
        return ret;
    }

    ret = convertValidTypeToAttributeType(val->valid_type, &attr_type);

    if (ret != NvCtrlSuccess) {
        return ret;
    }

    return NvCtrlNvmlGetAttributePerms(h, attr_type, attr, &val->permissions);
}

static ReturnStatus NvCtrlNvmlGetIntegerAttributePerms(int attr,
                                                       CtrlAttributePerms *perms)
{
    /* Set write permissions */
    switch (attr) {
        case NV_CTRL_GPU_COOLER_MANUAL_CONTROL:
        case NV_CTRL_THERMAL_COOLER_LEVEL:
        case NV_CTRL_GPU_ECC_CONFIGURATION:
        case NV_CTRL_GPU_ECC_RESET_ERROR_STATUS:
            perms->write = NV_TRUE;
            break;
        default:
            perms->write = NV_FALSE;
    }

    /* Valid target and read permissions */
    switch (attr) {
        case NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY:
        case NV_CTRL_USED_DEDICATED_GPU_MEMORY:
        case NV_CTRL_PCI_DOMAIN:
        case NV_CTRL_PCI_BUS:
        case NV_CTRL_PCI_DEVICE:
        case NV_CTRL_PCI_FUNCTION:
        case NV_CTRL_PCI_ID:
        case NV_CTRL_GPU_PCIE_GENERATION:
        case NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH:
        case NV_CTRL_GPU_PCIE_MAX_LINK_WIDTH:
        case NV_CTRL_GPU_SLOWDOWN_THRESHOLD:
        case NV_CTRL_GPU_SHUTDOWN_THRESHOLD:
        case NV_CTRL_GPU_CORE_TEMPERATURE:
        case NV_CTRL_GPU_MEMORY_BUS_WIDTH:
        case NV_CTRL_GPU_CORES:
        case NV_CTRL_IRQ:
        case NV_CTRL_GPU_POWER_SOURCE:
        case NV_CTRL_GPU_COOLER_MANUAL_CONTROL:
        /* CTRL_ATTRIBUTE_VALID_TYPE_BOOL */
        case NV_CTRL_GPU_ECC_SUPPORTED:
        case NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED:
        case NV_CTRL_GPU_ECC_STATUS:
        case NV_CTRL_GPU_ECC_CONFIGURATION:
        /* CTRL_ATTRIBUTE_VALID_TYPE_64BIT_INTEGER */
        case NV_CTRL_GPU_ECC_SINGLE_BIT_ERRORS:
        case NV_CTRL_GPU_ECC_AGGREGATE_SINGLE_BIT_ERRORS:
        case NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS:
        case NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS:
            perms->read = NV_TRUE;
            perms->valid_targets = CTRL_TARGET_PERM_BIT(GPU_TARGET);
            break;

        /* GPU_TARGET non-readable attribute */
        case NV_CTRL_GPU_ECC_RESET_ERROR_STATUS:
            perms->read  = NV_FALSE;
            perms->valid_targets = CTRL_TARGET_PERM_BIT(GPU_TARGET);
            break;

        case NV_CTRL_THERMAL_COOLER_LEVEL:
        case NV_CTRL_THERMAL_COOLER_CURRENT_LEVEL:
            perms->valid_targets = CTRL_TARGET_PERM_BIT(COOLER_TARGET);
            perms->read = NV_TRUE;
            break;

        default:
            perms->valid_targets = 0;
            perms->read = NV_FALSE;
            return NvCtrlNotSupported;
    }

    return NvCtrlSuccess;
}

static ReturnStatus NvCtrlNvmlGetStringAttributePerms(int attr,
                                                      CtrlAttributePerms *perms)
{
    perms->write = NV_FALSE;

    switch (attr) {
        case NV_CTRL_STRING_NVIDIA_DRIVER_VERSION:
        case NV_CTRL_STRING_PRODUCT_NAME:
        case NV_CTRL_STRING_VBIOS_VERSION:
        case NV_CTRL_STRING_GPU_UUID:
            perms->valid_targets = CTRL_TARGET_PERM_BIT(GPU_TARGET);
            perms->read = NV_TRUE;
            break;
        default:
            perms->valid_targets = 0;
            perms->read = NV_FALSE;
            return NvCtrlNotSupported;
    }
    return NvCtrlSuccess;
}


ReturnStatus
NvCtrlNvmlGetAttributePerms(const NvCtrlAttributePrivateHandle *h,
                            CtrlAttributeType attr_type, int attr,
                            CtrlAttributePerms *perms)
{
    ReturnStatus ret = NvCtrlSuccess;

    if (!h->nvml) {
        return NvCtrlBadArgument;
    }

    switch (attr_type) {
        case CTRL_ATTRIBUTE_TYPE_INTEGER:
            ret = NvCtrlNvmlGetIntegerAttributePerms(attr, perms);
            break;

        case CTRL_ATTRIBUTE_TYPE_STRING:
            ret = NvCtrlNvmlGetStringAttributePerms(attr, perms);
            break;

        case CTRL_ATTRIBUTE_TYPE_BINARY_DATA:
            return NvCtrlBadArgument;
            break;

        default:
            return NvCtrlBadArgument;
    }

    return ret;

}

