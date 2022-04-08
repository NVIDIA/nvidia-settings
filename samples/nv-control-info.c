/*
 * Copyright (c) 2004 NVIDIA, Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * nv-control-info.c - trivial sample NV-CONTROL client that
 * demonstrates how to determine if the NV-CONTROL extension is
 * present.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"

#define ARRAY_LEN(_arr) (sizeof(_arr) / sizeof(_arr[0]))

// Used to convert the NV-CONTROL #defines to human readable text.
#define MAKE_ENTRY(ATTRIBUTE) [ATTRIBUTE] = #ATTRIBUTE

// grep 'define.*\/\*' NVCtrl.h | grep -v NV_CTRL_TARGET_TYPE_ | grep -v "not supported" | grep -v "renamed" | grep -v "deprecated" | grep -v NV_CTRL_STRING_ | grep -v NV_CTRL_BINARY_DATA_ | sed 's/.*define \([^ ]*\).*/    MAKE_ENTRY(\1),/'
static const char *attr_int_table[NV_CTRL_LAST_ATTRIBUTE + 1] = {
    MAKE_ENTRY(NV_CTRL_DITHERING),
    MAKE_ENTRY(NV_CTRL_DIGITAL_VIBRANCE),
    MAKE_ENTRY(NV_CTRL_BUS_TYPE),
    MAKE_ENTRY(NV_CTRL_TOTAL_GPU_MEMORY),
    MAKE_ENTRY(NV_CTRL_IRQ),
    MAKE_ENTRY(NV_CTRL_OPERATING_SYSTEM),
    MAKE_ENTRY(NV_CTRL_SYNC_TO_VBLANK),
    MAKE_ENTRY(NV_CTRL_LOG_ANISO),
    MAKE_ENTRY(NV_CTRL_FSAA_MODE),
    MAKE_ENTRY(NV_CTRL_UBB),
    MAKE_ENTRY(NV_CTRL_OVERLAY),
    MAKE_ENTRY(NV_CTRL_STEREO),
    MAKE_ENTRY(NV_CTRL_TWINVIEW),
    MAKE_ENTRY(NV_CTRL_ENABLED_DISPLAYS),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_POLARITY),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_DELAY),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_INTERVAL),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_PORT0_STATUS),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_PORT1_STATUS),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_HOUSE_STATUS),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_READY),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_STEREO_SYNC),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_TEST_SIGNAL),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_ETHERNET_DETECTED),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_VIDEO_MODE),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_RATE),
    MAKE_ENTRY(NV_CTRL_OPENGL_AA_LINE_GAMMA),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_TIMING),
    MAKE_ENTRY(NV_CTRL_FLIPPING_ALLOWED),
    MAKE_ENTRY(NV_CTRL_ARCHITECTURE),
    MAKE_ENTRY(NV_CTRL_TEXTURE_CLAMPING),
    MAKE_ENTRY(NV_CTRL_FSAA_APPLICATION_CONTROLLED),
    MAKE_ENTRY(NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED),
    MAKE_ENTRY(NV_CTRL_IMAGE_SHARPENING),
    MAKE_ENTRY(NV_CTRL_GPU_CORE_TEMPERATURE),
    MAKE_ENTRY(NV_CTRL_GPU_CORE_THRESHOLD),
    MAKE_ENTRY(NV_CTRL_GPU_DEFAULT_CORE_THRESHOLD),
    MAKE_ENTRY(NV_CTRL_GPU_MAX_CORE_THRESHOLD),
    MAKE_ENTRY(NV_CTRL_AMBIENT_TEMPERATURE),
    MAKE_ENTRY(NV_CTRL_GPU_CURRENT_CLOCK_FREQS),
    MAKE_ENTRY(NV_CTRL_FLATPANEL_CHIP_LOCATION),
    MAKE_ENTRY(NV_CTRL_FLATPANEL_LINK),
    MAKE_ENTRY(NV_CTRL_FLATPANEL_SIGNAL),
    MAKE_ENTRY(NV_CTRL_USE_HOUSE_SYNC),
    MAKE_ENTRY(NV_CTRL_EDID_AVAILABLE),
    MAKE_ENTRY(NV_CTRL_FORCE_STEREO),
    MAKE_ENTRY(NV_CTRL_IMAGE_SETTINGS),
    MAKE_ENTRY(NV_CTRL_XINERAMA),
    MAKE_ENTRY(NV_CTRL_XINERAMA_STEREO),
    MAKE_ENTRY(NV_CTRL_BUS_RATE),
    MAKE_ENTRY(NV_CTRL_XV_SYNC_TO_DISPLAY),
    MAKE_ENTRY(NV_CTRL_CURRENT_XV_SYNC_TO_DISPLAY_ID),
    MAKE_ENTRY(NV_CTRL_PROBE_DISPLAYS),
    MAKE_ENTRY(NV_CTRL_REFRESH_RATE),
    MAKE_ENTRY(NV_CTRL_CURRENT_SCANLINE),
    MAKE_ENTRY(NV_CTRL_INITIAL_PIXMAP_PLACEMENT),
    MAKE_ENTRY(NV_CTRL_PCI_BUS),
    MAKE_ENTRY(NV_CTRL_PCI_DEVICE),
    MAKE_ENTRY(NV_CTRL_PCI_FUNCTION),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_FPGA_REVISION),
    MAKE_ENTRY(NV_CTRL_MAX_SCREEN_WIDTH),
    MAKE_ENTRY(NV_CTRL_MAX_SCREEN_HEIGHT),
    MAKE_ENTRY(NV_CTRL_MAX_DISPLAYS),
    MAKE_ENTRY(NV_CTRL_DYNAMIC_TWINVIEW),
    MAKE_ENTRY(NV_CTRL_MULTIGPU_DISPLAY_OWNER),
    MAKE_ENTRY(NV_CTRL_FSAA_APPLICATION_ENHANCED),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_RATE_4),
    MAKE_ENTRY(NV_CTRL_HWOVERLAY),
    MAKE_ENTRY(NV_CTRL_NUM_GPU_ERRORS_RECOVERED),
    MAKE_ENTRY(NV_CTRL_REFRESH_RATE_3),
    MAKE_ENTRY(NV_CTRL_GPU_POWER_SOURCE),
    MAKE_ENTRY(NV_CTRL_GLYPH_CACHE),
    MAKE_ENTRY(NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL),
    MAKE_ENTRY(NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE),
    MAKE_ENTRY(NV_CTRL_DEPTH_30_ALLOWED),
    MAKE_ENTRY(NV_CTRL_MODE_SET_EVENT),
    MAKE_ENTRY(NV_CTRL_OPENGL_AA_LINE_GAMMA_VALUE),
    MAKE_ENTRY(NV_CTRL_DISPLAYPORT_LINK_RATE),
    MAKE_ENTRY(NV_CTRL_STEREO_EYES_EXCHANGE),
    MAKE_ENTRY(NV_CTRL_NO_SCANOUT),
    MAKE_ENTRY(NV_CTRL_X_SERVER_UNIQUE_ID),
    MAKE_ENTRY(NV_CTRL_PIXMAP_CACHE),
    MAKE_ENTRY(NV_CTRL_PIXMAP_CACHE_ROUNDING_SIZE_KB),
    MAKE_ENTRY(NV_CTRL_PCI_ID),
    MAKE_ENTRY(NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE),
    MAKE_ENTRY(NV_CTRL_IMAGE_SHARPENING_DEFAULT),
    MAKE_ENTRY(NV_CTRL_PCI_DOMAIN),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_DELAY_RESOLUTION),
    MAKE_ENTRY(NV_CTRL_GPU_COOLER_MANUAL_CONTROL),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_LEVEL),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_LEVEL_SET_DEFAULT),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_CONTROL_TYPE),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_TARGET),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_SUPPORTED),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_STATUS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_CONFIGURATION),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_SINGLE_BIT_ERRORS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_AGGREGATE_SINGLE_BIT_ERRORS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_RESET_ERROR_STATUS),
    MAKE_ENTRY(NV_CTRL_GPU_POWER_MIZER_MODE),
    MAKE_ENTRY(NV_CTRL_GPU_PCIE_GENERATION),
    MAKE_ENTRY(NV_CTRL_ACCELERATE_TRAPEZOIDS),
    MAKE_ENTRY(NV_CTRL_GPU_CORES),
    MAKE_ENTRY(NV_CTRL_GPU_MEMORY_BUS_WIDTH),
    MAKE_ENTRY(NV_CTRL_COLOR_SPACE),
    MAKE_ENTRY(NV_CTRL_COLOR_RANGE),
    MAKE_ENTRY(NV_CTRL_DITHERING_MODE),
    MAKE_ENTRY(NV_CTRL_CURRENT_DITHERING),
    MAKE_ENTRY(NV_CTRL_CURRENT_DITHERING_MODE),
    MAKE_ENTRY(NV_CTRL_THERMAL_SENSOR_READING),
    MAKE_ENTRY(NV_CTRL_THERMAL_SENSOR_PROVIDER),
    MAKE_ENTRY(NV_CTRL_THERMAL_SENSOR_TARGET),
    MAKE_ENTRY(NV_CTRL_GPU_PCIE_MAX_LINK_SPEED),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_RESET_TRANSCEIVER_TO_FACTORY_SETTINGS),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_MODE),
    MAKE_ENTRY(NV_CTRL_SYNCHRONOUS_PALETTE_UPDATES),
    MAKE_ENTRY(NV_CTRL_DITHERING_DEPTH),
    MAKE_ENTRY(NV_CTRL_CURRENT_DITHERING_DEPTH),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_FREQUENCY),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_QUALITY),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_COUNT),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_PAIR_GLASSES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_UNPAIR_GLASSES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_DISCOVER_GLASSES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_IDENTIFY_GLASSES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_SYNC_CYCLE),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_MISSED_SYNC_CYCLES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_BATTERY_LEVEL),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_PAIR_EVENT),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_UNPAIR_EVENT),
    MAKE_ENTRY(NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH),
    MAKE_ENTRY(NV_CTRL_GPU_PCIE_CURRENT_LINK_SPEED),
    MAKE_ENTRY(NV_CTRL_CURRENT_METAMODE_ID),
    MAKE_ENTRY(NV_CTRL_DISPLAY_ENABLED),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_INCOMING_HOUSE_SYNC_RATE),
    MAKE_ENTRY(NV_CTRL_FXAA),
    MAKE_ENTRY(NV_CTRL_DISPLAY_RANDR_OUTPUT_ID),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_DISPLAY_CONFIG),
    MAKE_ENTRY(NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY),
    MAKE_ENTRY(NV_CTRL_USED_DEDICATED_GPU_MEMORY),
    MAKE_ENTRY(NV_CTRL_DPY_HDMI_3D),
    MAKE_ENTRY(NV_CTRL_BASE_MOSAIC),
    MAKE_ENTRY(NV_CTRL_MULTIGPU_PRIMARY_POSSIBLE),
    MAKE_ENTRY(NV_CTRL_GPU_POWER_MIZER_DEFAULT_MODE),
    MAKE_ENTRY(NV_CTRL_XV_SYNC_TO_DISPLAY_ID),
    MAKE_ENTRY(NV_CTRL_BACKLIGHT_BRIGHTNESS),
    MAKE_ENTRY(NV_CTRL_GPU_LOGO_BRIGHTNESS),
    MAKE_ENTRY(NV_CTRL_GPU_SLI_LOGO_BRIGHTNESS),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_SPEED),
    MAKE_ENTRY(NV_CTRL_PALETTE_UPDATE_EVENT),
    MAKE_ENTRY(NV_CTRL_VIDEO_ENCODER_UTILIZATION),
    MAKE_ENTRY(NV_CTRL_VRR_ALLOWED),
    MAKE_ENTRY(NV_CTRL_GPU_NVCLOCK_OFFSET),
    MAKE_ENTRY(NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET),
    MAKE_ENTRY(NV_CTRL_VIDEO_DECODER_UTILIZATION),
    MAKE_ENTRY(NV_CTRL_GPU_OVER_VOLTAGE_OFFSET),
    MAKE_ENTRY(NV_CTRL_GPU_CURRENT_CORE_VOLTAGE),
    MAKE_ENTRY(NV_CTRL_CURRENT_COLOR_SPACE),
    MAKE_ENTRY(NV_CTRL_CURRENT_COLOR_RANGE),
    MAKE_ENTRY(NV_CTRL_SHOW_VRR_VISUAL_INDICATOR),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_CURRENT_LEVEL),
    MAKE_ENTRY(NV_CTRL_STEREO_SWAP_MODE),
    MAKE_ENTRY(NV_CTRL_DISPLAY_VRR_MODE),
    MAKE_ENTRY(NV_CTRL_DISPLAY_VRR_MIN_REFRESH_RATE),
};

// grep 'define.*\/\*' NVCtrl.h | grep -v NV_CTRL_TARGET_TYPE_ | grep -v "not supported" | grep -v "renamed" | grep -v "deprecated" | grep NV_CTRL_STRING_ | grep -v NV_CTRL_STRING_OPERATION_ | sed 's/.*define \([^ ]*\).*/    MAKE_ENTRY(\1),/'
static const char *attr_str_table[NV_CTRL_STRING_LAST_ATTRIBUTE + 1] = {
    MAKE_ENTRY(NV_CTRL_STRING_PRODUCT_NAME),
    MAKE_ENTRY(NV_CTRL_STRING_VBIOS_VERSION),
    MAKE_ENTRY(NV_CTRL_STRING_NVIDIA_DRIVER_VERSION),
    MAKE_ENTRY(NV_CTRL_STRING_DISPLAY_DEVICE_NAME),
    MAKE_ENTRY(NV_CTRL_STRING_CURRENT_MODELINE),
    MAKE_ENTRY(NV_CTRL_STRING_ADD_MODELINE),
    MAKE_ENTRY(NV_CTRL_STRING_DELETE_MODELINE),
    MAKE_ENTRY(NV_CTRL_STRING_CURRENT_METAMODE),
    MAKE_ENTRY(NV_CTRL_STRING_ADD_METAMODE),
    MAKE_ENTRY(NV_CTRL_STRING_DELETE_METAMODE),
    MAKE_ENTRY(NV_CTRL_STRING_MOVE_METAMODE),
    MAKE_ENTRY(NV_CTRL_STRING_VALID_HORIZ_SYNC_RANGES),
    MAKE_ENTRY(NV_CTRL_STRING_VALID_VERT_REFRESH_RANGES),
    MAKE_ENTRY(NV_CTRL_STRING_SCREEN_RECTANGLE),
    MAKE_ENTRY(NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER),
    MAKE_ENTRY(NV_CTRL_STRING_SLI_MODE),
    MAKE_ENTRY(NV_CTRL_STRING_PERFORMANCE_MODES),
    MAKE_ENTRY(NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_HARDWARE_REVISION),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_VERSION_A),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_DATE_A),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_VERSION_B),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_DATE_B),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_ADDRESS),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_GLASSES_FIRMWARE_VERSION_A),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_GLASSES_FIRMWARE_DATE_A),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_GLASSES_ADDRESS),
    MAKE_ENTRY(NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME),
    MAKE_ENTRY(NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2),
    MAKE_ENTRY(NV_CTRL_STRING_DISPLAY_NAME_TYPE_BASENAME),
    MAKE_ENTRY(NV_CTRL_STRING_DISPLAY_NAME_TYPE_ID),
    MAKE_ENTRY(NV_CTRL_STRING_DISPLAY_NAME_DP_GUID),
    MAKE_ENTRY(NV_CTRL_STRING_DISPLAY_NAME_EDID_HASH),
    MAKE_ENTRY(NV_CTRL_STRING_DISPLAY_NAME_TARGET_INDEX),
    MAKE_ENTRY(NV_CTRL_STRING_DISPLAY_NAME_RANDR),
    MAKE_ENTRY(NV_CTRL_STRING_GPU_UUID),
    MAKE_ENTRY(NV_CTRL_STRING_GPU_UTILIZATION),
    MAKE_ENTRY(NV_CTRL_STRING_MULTIGPU_MODE),
};

// grep 'define.*\/\*' NVCtrl.h | grep -v NV_CTRL_TARGET_TYPE_ | grep -v "not supported" | grep -v "renamed" | grep -v "deprecated" | grep NV_CTRL_BINARY_DATA_ | sed 's/.*define \([^ ]*\).*/    MAKE_ENTRY(\1),/'
static const char *attr_bin_table[NV_CTRL_BINARY_DATA_LAST_ATTRIBUTE + 1] = {
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_EDID),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_MODELINES),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_METAMODES),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_XSCREENS_USING_GPU),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_DISPLAY_VIEWPORT),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_FRAMELOCKS_USED_BY_GPU),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_GPUS_USED_BY_LOGICAL_XSCREEN),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_GLASSES_PAIRED_TO_3D_VISION_PRO_TRANSCEIVER),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_DISPLAY_TARGETS),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_METAMODES_VERSION_2),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_DISPLAYS_ASSIGNED_TO_XSCREEN),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_GPU_FLAGS),
    MAKE_ENTRY(NV_CTRL_BINARY_DATA_DISPLAYS_ON_GPU),
};

// grep 'define.*\/\*' NVCtrl.h | grep -v NV_CTRL_TARGET_TYPE_ | grep -v "not supported" | grep -v "renamed" | grep -v "deprecated" | grep NV_CTRL_STRING_OPERATION_ | sed 's/.*define \([^ ]*\).*/    MAKE_ENTRY(\1),/'
static const char *attr_strop_table[NV_CTRL_STRING_OPERATION_LAST_ATTRIBUTE + 1] = {
    MAKE_ENTRY(NV_CTRL_STRING_OPERATION_ADD_METAMODE),
    MAKE_ENTRY(NV_CTRL_STRING_OPERATION_GTF_MODELINE),
    MAKE_ENTRY(NV_CTRL_STRING_OPERATION_CVT_MODELINE),
    MAKE_ENTRY(NV_CTRL_STRING_OPERATION_BUILD_MODEPOOL),
    MAKE_ENTRY(NV_CTRL_STRING_OPERATION_PARSE_METAMODE),
};


static void print_perms(NVCTRLAttributePermissionsRec *perms)
{
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_READ)                      ? 'R' : '_');
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_WRITE)                     ? 'W' : '_');
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_DISPLAY)                   ? 'D' : '_');
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_GPU)                       ? 'G' : '_');
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_FRAMELOCK)                 ? 'F' : '_');
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_X_SCREEN)                  ? 'X' : '_');
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_XINERAMA)                  ? 'I' : '_');
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_COOLER)                    ? 'C' : '_');
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_THERMAL_SENSOR)            ? 'T' : '_');
    printf("%c", (perms->permissions & ATTRIBUTE_TYPE_3D_VISION_PRO_TRANSCEIVER) ? '3' : '_');
}

/* Used to stringify NV_CTRL_XXX #defines */

#define ADD_NVCTRL_CASE(FMT) \
case (FMT):                  \
    return #FMT;

static const char *GetAttrTypeName(int value)
{
    switch (value) {
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_UNKNOWN);
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_INTEGER);
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_BITMASK);
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_BOOL);
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_RANGE);
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_INT_BITS);
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_64BIT_INTEGER);
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_STRING);
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_BINARY_DATA);
        ADD_NVCTRL_CASE(ATTRIBUTE_TYPE_STRING_OPERATION);
    default:
        return "Invalid Value";
    }
}


static void print_table_entry(NVCTRLAttributePermissionsRec *perms,
                              const int index,
                              const char *name)
{
    /*
     * Don't print the attribute if *both* the permissions are empty
     * and the attribute table was missing an entry for the attribute.
     * Either condition by itself is acceptable:
     *
     * - Event-only attributes (e.g., NV_CTRL_MODE_SET_EVENT)
     *   don't have any permissions.
     *
     * - A missing table entry could just mean the table is stale
     *   relative to NVCtrl.h.
     */
    if ((perms->permissions == 0) && (name == NULL)) {
        return;
    }

    printf("  (%3d) [Perms: ", index);
    print_perms(perms);
    printf("] [ ");
    printf("%-32s", GetAttrTypeName(perms->type));
    printf("] - %s\n", name ? name : "Unknown");
}



int main(void)
{
    Display *dpy;
    Bool ret;
    int event_base, error_base, major, minor, screens, i;
    char *str;
    NVCTRLAttributePermissionsRec perms;

    /*
     * open a connection to the X server indicated by the DISPLAY
     * environment variable
     */
    
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display '%s'.\n", XDisplayName(NULL));
        return 1;
    }
    
    /*
     * check if the NV-CONTROL X extension is present on this X server
     */

    ret = XNVCTRLQueryExtension(dpy, &event_base, &error_base);
    if (ret != True) {
        fprintf(stderr, "The NV-CONTROL X extension does not exist on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }

    /*
     * query the major and minor extension version
     */

    ret = XNVCTRLQueryVersion(dpy, &major, &minor);
    if (ret != True) {
        fprintf(stderr, "The NV-CONTROL X extension does not exist on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }

    /*
     * print statistics thus far
     */

    printf("NV-CONTROL X extension present\n");
    printf("  version        : %d.%d\n", major, minor);
    printf("  event base     : %d\n", event_base);
    printf("  error base     : %d\n", error_base);
    
    /*
     * loop over each screen, and determine if each screen is
     * controlled by the NVIDIA X driver (and thus supports the
     * NV-CONTROL X extension); then, query the string attributes on
     * the screen.
     */

    screens = ScreenCount(dpy);
    for (i = 0; i < screens; i++) {
        if (XNVCTRLIsNvScreen(dpy, i)) {
            printf("Screen %d supports the NV-CONTROL X extension\n", i);

            ret = XNVCTRLQueryStringAttribute(dpy, i,
                                              0, /* XXX not currently used */
                                              NV_CTRL_STRING_PRODUCT_NAME,
                                              &str);
            if (ret) {
                printf("  GPU            : %s\n", str);
                XFree(str);
            }
            
            ret = XNVCTRLQueryStringAttribute(dpy, i,
                                              0, /* XXX not currently used */
                                              NV_CTRL_STRING_VBIOS_VERSION,
                                              &str);
            
            if (ret) {
                printf("  VideoBIOS      : %s\n", str);
                XFree(str);
            }

            ret = XNVCTRLQueryStringAttribute(dpy, i,
                                              0, /* XXX not currently used */
                                              NV_CTRL_STRING_NVIDIA_DRIVER_VERSION,
                                              &str);

            if (ret) {
                printf("  Driver version : %s\n", str);
                XFree(str);
            }
        }
    }

    /*
     * print attribute permission and type information.
     */

    printf("Attributes (Integers):\n");
    for (i = 0; i < ARRAY_LEN(attr_int_table); i++) {
        memset(&perms, 0, sizeof(NVCTRLAttributePermissionsRec));
        if (XNVCTRLQueryAttributePermissions(dpy, i, &perms)) {
            print_table_entry(&perms, i, attr_int_table[i]);
        }
    }

    printf("Attributes (Strings):\n");
    for (i = 0; i < ARRAY_LEN(attr_str_table); i++) {
        memset(&perms, 0, sizeof(NVCTRLAttributePermissionsRec));
        if (XNVCTRLQueryStringAttributePermissions(dpy, i, &perms)) {
            print_table_entry(&perms, i, attr_str_table[i]);
        }
    }

    printf("Attributes (Binary Data):\n");
    for (i = 0; i < ARRAY_LEN(attr_bin_table); i++) {
        memset(&perms, 0, sizeof(NVCTRLAttributePermissionsRec));
        if (XNVCTRLQueryBinaryDataAttributePermissions(dpy, i, &perms)) {
            print_table_entry(&perms, i, attr_bin_table[i]);
        }
    }

    printf("Attributes (String Operations):\n");
    for (i = 0; i < ARRAY_LEN(attr_strop_table); i++) {
        memset(&perms, 0, sizeof(NVCTRLAttributePermissionsRec));
        if (XNVCTRLQueryStringOperationAttributePermissions(dpy, i, &perms)) {
            print_table_entry(&perms, i, attr_strop_table[i]);
        }
    }

    /*
     * close the display connection
     */

    XCloseDisplay(dpy);

    return 0;
}
