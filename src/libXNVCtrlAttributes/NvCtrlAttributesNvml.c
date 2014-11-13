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

#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"

#include "msg.h"
#include "parse.h"

#ifdef NVML_AVAILABLE

#include <nvml.h>


#define MAX_NVML_STR_LEN 64


static Bool __isNvmlLoaded = FALSE;
static unsigned int __nvmlUsers = 0;


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
            nv_error_msg("The requested operation is not available on target "
                         "device");
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

        case NVML_ERROR_UNKNOWN:
            nv_error_msg("An internal driver error occurred");
            break;
    }
}

#endif // NVML_AVAILABLE


/*
 * Loads and initializes the NVML library
 */

ReturnStatus NvCtrlInitNvml(void)
{
#ifdef NVML_AVAILABLE

    if (!__isNvmlLoaded) {
        nvmlReturn_t ret = nvmlInit();
        if (ret != NVML_SUCCESS) {
            printNvmlError(ret);
            return NvCtrlMissingExtension;
        }

        __isNvmlLoaded = TRUE;
    }

    __nvmlUsers++;

    return NvCtrlSuccess;

#else
    return NvCtrlMissingExtension;
#endif
}



/*
 * Unloads the NVML library if it was successfully loaded.
 */

ReturnStatus NvCtrlDestroyNvml(void)
{
#ifdef NVML_AVAILABLE

    if (__isNvmlLoaded) {
        __nvmlUsers--;
        if (__nvmlUsers == 0) {
            nvmlReturn_t ret = nvmlShutdown();
            if (ret != NVML_SUCCESS) {
                printNvmlError(ret);
                return NvCtrlError;
            }
            __isNvmlLoaded = FALSE;
        }
    }
    return NvCtrlSuccess;

#else
    return NvCtrlMissingExtension;
#endif
}

