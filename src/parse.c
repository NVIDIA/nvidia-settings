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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/utsname.h>

#include "NVCtrl.h"

#include "parse.h"
#include "NvCtrlAttributes.h"
#include "query-assign.h"

#include "common-utils.h"


/* local helper functions */

static int ctoi(const char c);
static int count_number_of_chars(char *o, char d);

static uint32 display_device_name_to_display_device_mask(const char *str);


/*
 * Table of all attribute names recognized by the attribute string
 * parser.  Binds attribute names to attribute integers (for use in
 * the NvControl protocol).  The flags describe qualities of each
 * attribute.
 */

#define INT_ATTR CTRL_ATTRIBUTE_TYPE_INTEGER
#define STR_ATTR CTRL_ATTRIBUTE_TYPE_STRING
#define COL_ATTR CTRL_ATTRIBUTE_TYPE_COLOR
#define SOP_ATTR CTRL_ATTRIBUTE_TYPE_STRING_OPERATION

const AttributeTableEntry attributeTable[] = {

    /* name                              attribute                                      type    common flags   special flags                     description
     *
     *                                                                                                                         .-------------- is_100Hz
     *                                                                                                                         | .------------ is_1000Hz
     *                                                                             no_query_all -----------.                   | | .---------- is_packed
     *                                                                          no_config_write ---------. |                   | | | .-------- is_display_mask
     *                                                                    hijack_display_device -------. | |                   | | | | .------ is_display_id
     *                                                                     is_framelock_attribute ---. | | |                   | | | | | .---- no_zero
     *                                                                           is_gui_attribute -. | | | |                   | | | | | | .-- is_switch_display
     *                                                                                             | | | | |                   | | | | | | |
     */

    /* Version information */
    { "OperatingSystem",                  NV_CTRL_OPERATING_SYSTEM,                     INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The operating system on which the NVIDIA driver is running.  0-Linux, 1-FreeBSD, 2-SunOS." },
    { "NvidiaDriverVersion",              NV_CTRL_STRING_NVIDIA_DRIVER_VERSION,         STR_ATTR, {0,0,0,1,0}, {}, "The NVIDIA X driver version." },
    { "NvControlVersion",                 NV_CTRL_STRING_NV_CONTROL_VERSION,            STR_ATTR, {0,0,0,1,0}, {}, "The NV-CONTROL X driver extension version." },
    { "GLXServerVersion",                 NV_CTRL_STRING_GLX_SERVER_VERSION,            STR_ATTR, {0,0,0,1,0}, {}, "The GLX X server extension version." },
    { "GLXClientVersion",                 NV_CTRL_STRING_GLX_CLIENT_VERSION,            STR_ATTR, {0,0,0,1,0}, {}, "The GLX client version." },
    { "OpenGLVersion",                    NV_CTRL_STRING_GLX_OPENGL_VERSION,            STR_ATTR, {0,0,0,1,0}, {}, "The OpenGL version." },
    { "XRandRVersion",                    NV_CTRL_STRING_XRANDR_VERSION,                STR_ATTR, {0,0,0,1,0}, {}, "The X RandR version." },
    { "XF86VidModeVersion",               NV_CTRL_STRING_XF86VIDMODE_VERSION,           STR_ATTR, {0,0,0,1,0}, {}, "The XF86 Video Mode X extension version." },
    { "XvVersion",                        NV_CTRL_STRING_XV_VERSION,                    STR_ATTR, {0,0,0,1,0}, {}, "The Xv X extension version." },

    /* X screen */
    { "Ubb",                              NV_CTRL_UBB,                                  INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Is UBB enabled for the specified X screen." },
    { "Overlay",                          NV_CTRL_OVERLAY,                              INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Is the RGB overlay enabled for the specified X screen." },
    { "Stereo",                           NV_CTRL_STEREO,                               INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The stereo mode for the specified X screen." },
    { "TwinView",                         NV_CTRL_TWINVIEW,                             INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Is TwinView enabled for the specified X screen." },
    { "ConnectedDisplays",                NV_CTRL_CONNECTED_DISPLAYS,                   INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,1,0,0,0} }, "DEPRECATED: use \"-q dpys\" instead." },
    { "EnabledDisplays",                  NV_CTRL_ENABLED_DISPLAYS,                     INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,1,0,0,0} }, "DEPRECATED: use \"-q dpys\" instead." },
    { "AssociatedDisplays",               NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,           INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,1,0,0,0} }, "DEPRECATED: use \"-q xscreens -V all\" instead." },
    { "ProbeDisplays",                    NV_CTRL_PROBE_DISPLAYS,                       INT_ATTR, {0,0,0,0,1}, { .int_flags = {0,0,0,1,0,0,0} }, "When this attribute is queried, the X driver re-probes the hardware to detect which display devices are connected to the GPU or DPU driving the specified X screen." },
    { "InitialPixmapPlacement",           NV_CTRL_INITIAL_PIXMAP_PLACEMENT,             INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls where X pixmaps are initially created." },
    { "MultiGpuDisplayOwner",             NV_CTRL_MULTIGPU_DISPLAY_OWNER,               INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "GPU ID of the GPU that has the display device(s) used for showing the X screen." },
    { "HWOverlay",                        NV_CTRL_HWOVERLAY,                            INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "When a workstation overlay is in use, this value is 1 if the hardware overlay is used, or 0 if the overlay is emulated." },
    { "GlyphCache",                       NV_CTRL_GLYPH_CACHE,                          INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Enable or disable caching of glyphs (text) in video memory." },
    { "SwitchToDisplays",                 NV_CTRL_SWITCH_TO_DISPLAYS,                   INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,1,0,0,1} }, "DEPRECATED." },
    { "NotebookDisplayChangeLidEvent",    NV_CTRL_NOTEBOOK_DISPLAY_CHANGE_LID_EVENT,    INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "DEPRECATED." },
    { "NotebookInternalLCD",              NV_CTRL_NOTEBOOK_INTERNAL_LCD,                INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,1,0,0,0} }, "DEPRECATED." },
    { "Depth30Allowed",                   NV_CTRL_DEPTH_30_ALLOWED,                     INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns whether the NVIDIA X driver supports depth 30 on the specified X screen or GPU." },
    { "NoScanout",                        NV_CTRL_NO_SCANOUT,                           INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns whether the special \"NoScanout\" mode is enabled on the specified X screen or GPU." },
    { "XServerUniqueId",                  NV_CTRL_X_SERVER_UNIQUE_ID,                   INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns a pseudo-unique identification number for the X server." },
    { "PixmapCache",                      NV_CTRL_PIXMAP_CACHE,                         INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls whether pixmaps are allocated in a cache." },
    { "PixmapCacheRoundSizeKB",           NV_CTRL_PIXMAP_CACHE_ROUNDING_SIZE_KB,        INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the number of kilobytes to add to the pixmap cache when there is not enough room." },
    { "AccelerateTrapezoids",             NV_CTRL_ACCELERATE_TRAPEZOIDS,                INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Enable or disable GPU acceleration of RENDER Trapezoids." },
    { "ScreenPosition",                   NV_CTRL_STRING_SCREEN_RECTANGLE,              STR_ATTR, {0,0,0,1,0}, {}, "Returns the physical X Screen's initial position and size (in absolute coordinates) within the desktop as the \"token=value \" string:  \"x=#, y=#, width=#, height=#\"." },
    { "AddMetaMode",                      NV_CTRL_STRING_OPERATION_ADD_METAMODE,        SOP_ATTR, {0,0,0,1,1}, {}, "Adds the given MetaMode to the X screen." },
    { "ParseMetaMode",                    NV_CTRL_STRING_OPERATION_ADD_METAMODE,        SOP_ATTR, {0,0,0,1,1}, {}, "Parses and validates a given MetaMode." },
    { "PrimeOutputsData",                 NV_CTRL_STRING_PRIME_OUTPUTS_DATA      ,      STR_ATTR, {0,0,0,1,0}, {}, "Lists configured PRIME displays' configuration information." },

    /* OpenGL */
    { "SyncToVBlank",                     NV_CTRL_SYNC_TO_VBLANK,                       INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Enables sync to vertical blanking for OpenGL clients.  This setting only takes effect on OpenGL clients started after it is set." },
    { "LogAniso",                         NV_CTRL_LOG_ANISO,                            INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Enables anisotropic filtering for OpenGL clients; on some NVIDIA hardware, this can only be enabled or disabled; on other hardware different levels of anisotropic filtering can be specified.  This setting only takes effect on OpenGL clients started after it is set." },
    { "FSAA",                             NV_CTRL_FSAA_MODE,                            INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The full screen antialiasing setting for OpenGL clients.  This setting only takes effect on OpenGL clients started after it is set. Enabling antialiasing will disable FXAA." },
    { "ForceGenericCpu",                  NV_CTRL_FORCE_GENERIC_CPU,                    INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "NOT SUPPORTED." },
    { "GammaCorrectedAALines",            NV_CTRL_OPENGL_AA_LINE_GAMMA,                 INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "For OpenGL clients, allow gamma-corrected antialiased lines to consider variances in the color display capabilities of output devices when rendering smooth lines.  Only available on recent Quadro GPUs.  This setting only takes effect on OpenGL clients started after it is set." },
    { "TextureClamping",                  NV_CTRL_TEXTURE_CLAMPING,                     INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Define the behavior of OpenGL texture clamping for unbordered textures.  If enabled (1), the conformant behavior is used.  If disabled (0), GL_CLAMP is remapped to GL_CLAMP_TO_EDGE to avoid seams in applications that rely on this behavior, which was the only option in some very old hardware." },
    { "FXAA",                             NV_CTRL_FXAA,                                 INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Enables or disables the use of FXAA, Fast Approximate Anti-Aliasing. Enabling FXAA will disable regular antialiasing modes." },
    { "AllowFlipping",                    NV_CTRL_FLIPPING_ALLOWED,                     INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Defines the swap behavior of OpenGL.  When 1, OpenGL will swap by flipping when possible;  When 0, OpenGL will always swap by blitting." },
    { "FSAAAppControlled",                NV_CTRL_FSAA_APPLICATION_CONTROLLED,          INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "When Application Control for FSAA is enabled, then what the application requests is used, and the FSAA attribute is ignored.  If this is disabled, then any application setting is overridden with the FSAA attribute." },
    { "LogAnisoAppControlled",            NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED,     INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "When Application Control for LogAniso is enabled, then what the application requests is used, and the LogAniso attribute is ignored.  If this is disabled, then any application setting is overridden with the LogAniso attribute." },
    { "ForceStereoFlipping",              NV_CTRL_FORCE_STEREO,                         INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "When 1, OpenGL will force stereo flipping even when no stereo drawables are visible (if the device is configured to support it, see the \"Stereo\" X config option).  When 0, fall back to the default behavior of only flipping when a stereo drawable is visible." },
    { "OpenGLImageSettings",              NV_CTRL_IMAGE_SETTINGS,                       INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The image quality setting for OpenGL clients.  This setting only takes effect on OpenGL clients started after it is set." },
    { "XineramaStereoFlipping",           NV_CTRL_XINERAMA_STEREO,                      INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "When 1, OpenGL will allow stereo flipping on multiple X screens configured with Xinerama.  When 0, flipping is allowed only on one X screen at a time." },
    { "ShowSLIHUD",                       NV_CTRL_SHOW_SLI_HUD,                         INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "If this is enabled (1), the driver will draw information about the current SLI mode into a \"heads-up display\" inside OpenGL windows accelerated with SLI.  This setting only takes effect on OpenGL clients started after it is set." },
    { "ShowSLIVisualIndicator",           NV_CTRL_SHOW_SLI_VISUAL_INDICATOR,            INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "If this is enabled (1), the driver will draw information about the current SLI mode into a \"visual indicator\" inside OpenGL windows accelerated with SLI.  This setting only takes effect on OpenGL clients started after it is set." },
    { "ShowMultiGpuVisualIndicator",      NV_CTRL_SHOW_MULTIGPU_VISUAL_INDICATOR,       INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "If this is enabled (1), the driver will draw information about the current MultiGPU mode into a \"visual indicator\" inside OpenGL windows accelerated with SLI.  This setting only takes effect on OpenGL clients started after it is set." },
    { "FSAAAppEnhanced",                  NV_CTRL_FSAA_APPLICATION_ENHANCED,            INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls how the FSAA attribute is applied when FSAAAppControlled is disabled.  When FSAAAppEnhanced is disabled, OpenGL applications will be forced to use the FSAA mode specified by the FSAA attribute.  When the FSAAAppEnhanced attribute is enabled, only those applications that have selected a multisample FBConfig will be made to use the FSAA mode specified." },
    { "GammaCorrectedAALinesValue",       NV_CTRL_OPENGL_AA_LINE_GAMMA_VALUE,           INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the gamma value used by OpenGL when gamma-corrected antialiased lines are enabled." },
    { "StereoEyesExchange",               NV_CTRL_STEREO_EYES_EXCHANGE,                 INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Swaps the left and right eyes of stereo images." },
    { "SliMosaicModeAvailable",           NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE,            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns whether or not SLI Mosaic Mode is supported." },
    { "SLIMode",                          NV_CTRL_STRING_SLI_MODE,                      STR_ATTR, {0,0,0,1,0}, {}, "Returns a string describing the current SLI mode, if any." },
    { "MultiGpuMode",                     NV_CTRL_STRING_MULTIGPU_MODE,                 STR_ATTR, {0,0,0,1,0}, {}, "Returns a string describing the current MultiGPU mode, if any." },
    { "AllowGSYNC",                       NV_CTRL_VRR_ALLOWED,                          INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "DEPRECATED: use \"AllowVRR\" instead." },
    { "AllowVRR",                         NV_CTRL_VRR_ALLOWED,                          INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Enables or disables the use of G-SYNC and G-SYNC Compatible when available." },
    { "ShowGSYNCVisualIndicator",         NV_CTRL_SHOW_VRR_VISUAL_INDICATOR,            INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "DEPRECATED: use \"ShowVRRVisualIndicator\" instead." },
    { "ShowVRRVisualIndicator",           NV_CTRL_SHOW_VRR_VISUAL_INDICATOR,            INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "If this is enabled (1), the driver will draw an indicator showing whether G-SYNC or G-SYNC Compatible is in use, when an application is swapping using flipping." },
    { "StereoSwapMode",                   NV_CTRL_STEREO_SWAP_MODE,                     INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the swap mode when Quad-Buffered stereo is used." },
    { "ShowGraphicsVisualIndicator",      NV_CTRL_SHOW_GRAPHICS_VISUAL_INDICATOR,       INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "If this is enabled (1), the driver will draw information about the graphics API in use into a \"visual indicator\" inside application windows.  This setting only takes effect on clients started after it is set." },

    /* GPU */
    { "BusType",                          NV_CTRL_BUS_TYPE,                             INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the type of bus connecting the specified device to the computer.  If the target is an X screen, then it uses the GPU driving the X screen as the device." },
    { "PCIEMaxLinkSpeed",                 NV_CTRL_GPU_PCIE_MAX_LINK_SPEED,              INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the maximum speed that the PCIe link between the GPU and the system may be trained to.  This is expressed in gigatransfers per second (GT/s).  The link may be dynamically trained to a slower speed, based on the GPU's utilization and performance settings." },
    { "PCIEMaxLinkWidth",                 NV_CTRL_GPU_PCIE_MAX_LINK_WIDTH,              INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the maximum width that the PCIe link between the GPU and the system may be trained to.  This is expressed in number of lanes.  The trained link width may vary dynamically and possibly be narrower based on the GPU's utilization and performance settings." },
    { "PCIECurrentLinkSpeed",             NV_CTRL_GPU_PCIE_CURRENT_LINK_SPEED,          INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current PCIe link speed, in gigatransfers per second (GT/s)." },
    { "PCIECurrentLinkWidth",             NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH,          INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current PCIe link width of the GPU, in number of lanes." },
    { "VideoRam",                         NV_CTRL_VIDEO_RAM,                            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the total amount of memory available to the specified GPU (or the GPU driving the specified X screen).  Note: if the GPU supports TurboCache(TM), the value reported may exceed the amount of video memory installed on the GPU.  The value reported for integrated GPUs may likewise exceed the amount of dedicated system memory set aside by the system BIOS for use by the integrated GPU." },
    { "TotalDedicatedGPUMemory",          NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY,           INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the amount of total dedicated memory on the specified GPU in MB." },
    { "GPUResizableBAR",                  NV_CTRL_RESIZABLE_BAR,                        INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns whether Resizable BAR is supported." },
    { "UsedDedicatedGPUMemory",           NV_CTRL_USED_DEDICATED_GPU_MEMORY,            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the amount of dedicated memory used on the specified GPU in MB." },
    { "Irq",                              NV_CTRL_IRQ,                                  INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the interrupt request line used by the specified device.  If the target is an X screen, then it uses the GPU driving the X screen as the device." },
    { "CUDACores",                        NV_CTRL_GPU_CORES,                            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns number of CUDA cores supported by the graphics pipeline." },
    { "GPUMemoryInterface",               NV_CTRL_GPU_MEMORY_BUS_WIDTH,                 INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns bus bandwidth of the GPU's memory interface." },
    { "GPUCoreTemp",                      NV_CTRL_GPU_CORE_TEMPERATURE,                 INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports the current core temperature in Celsius of the GPU driving the X screen." },
    { "GPUAmbientTemp",                   NV_CTRL_AMBIENT_TEMPERATURE,                  INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports the current temperature in Celsius of the immediate neighborhood of the GPU driving the X screen." },
    { "GPUGraphicsClockOffset",           NV_CTRL_GPU_NVCLOCK_OFFSET,                   INT_ATTR, {0,0,1,1,1}, { .int_flags = {0,0,0,0,0,0,0} }, "This is the offset amount, in MHz, to over- or under-clock the Graphics Clock.  Specify the performance level in square brackets after the attribute name.  E.g., 'GPUGraphicsClockOffset[2]'." },
    { "GPUMemoryTransferRateOffset",      NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET,         INT_ATTR, {0,0,1,1,1}, { .int_flags = {0,0,0,0,0,0,0} }, "This is the offset amount, in MHz, to over- or under-clock the Memory Transfer Rate.  Specify the performance level in square brackets after the attribute name.  E.g., 'GPUMemoryTransferRateOffset[2]'." },
    { "GPUGraphicsClockOffsetAllPerformanceLevels",      NV_CTRL_GPU_NVCLOCK_OFFSET_ALL_PERFORMANCE_LEVELS,      INT_ATTR, {0,0,1,1,1}, { .int_flags = {0,0,0,0,0,0,0} }, "This is the offset amount, in MHz, to over- or under-clock the Graphics Clock.  The offset is applied to all performance levels.  This attribute is available starting with Pascal GPUs." }, 
    { "GPUMemoryTransferRateOffsetAllPerformanceLevels", NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET_ALL_PERFORMANCE_LEVELS, INT_ATTR, {0,0,1,1,1}, { .int_flags = {0,0,0,0,0,0,0} }, "This is the offset amount, in MHz, to over- or under-clock the Memory Transfer Rate.  The offset is applied to all performance levels.  This attribute is available starting with Pascal GPUs." },
    { "GPUCurrentCoreVoltage",            NV_CTRL_GPU_CURRENT_CORE_VOLTAGE,             INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "This attribute returns the GPU's current operating voltage, in microvolts (uV)."},
    { "GPUOverVoltageOffset",             NV_CTRL_GPU_OVER_VOLTAGE_OFFSET,              INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "This is the offset, in microvolts (uV), to apply to the GPU's operating voltage."},
    { "GPUOverclockingState",             NV_CTRL_GPU_OVERCLOCKING_STATE,               INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "NOT SUPPORTED." },
    { "GPU2DClockFreqs",                  NV_CTRL_GPU_2D_CLOCK_FREQS,                   INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "NOT SUPPORTED." },
    { "GPU3DClockFreqs",                  NV_CTRL_GPU_3D_CLOCK_FREQS,                   INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "NOT SUPPORTED." },
    { "GPUDefault2DClockFreqs",           NV_CTRL_GPU_DEFAULT_2D_CLOCK_FREQS,           INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "NOT SUPPORTED." },
    { "GPUDefault3DClockFreqs",           NV_CTRL_GPU_DEFAULT_3D_CLOCK_FREQS,           INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "NOT SUPPORTED." },
    { "GPUCurrentClockFreqs",             NV_CTRL_GPU_CURRENT_CLOCK_FREQS,              INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "Returns the current GPU and memory clocks of the graphics device driving the X screen." },
    { "BusRate",                          NV_CTRL_BUS_RATE,                             INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "If the device is on an AGP bus, then BusRate returns the configured AGP rate.  If the device is on a PCI Express bus, then this attribute returns the width of the physical link." },
    { "PCIDomain",                        NV_CTRL_PCI_DOMAIN,                           INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the PCI domain number for the specified device." },
    { "PCIBus",                           NV_CTRL_PCI_BUS,                              INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the PCI bus number for the specified device." },
    { "PCIDevice",                        NV_CTRL_PCI_DEVICE,                           INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the PCI device number for the specified device." },
    { "PCIFunc",                          NV_CTRL_PCI_FUNCTION,                         INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the PCI function number for the specified device." },
    { "PCIID",                            NV_CTRL_PCI_ID,                               INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "Returns the PCI vendor and device ID of the specified device." },
    { "PCIEGen",                          NV_CTRL_GPU_PCIE_GENERATION,                  INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the PCIe generation that this GPU, in this system, is compliant with." },
    { "GPUErrors",                        NV_CTRL_NUM_GPU_ERRORS_RECOVERED,             INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the number of GPU errors occurred." },
    { "GPUPowerSource",                   NV_CTRL_GPU_POWER_SOURCE,                     INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports the type of power source of the GPU." },
    { "GPUCurrentPerfMode",               NV_CTRL_GPU_CURRENT_PERFORMANCE_MODE,         INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "NOT SUPPORTED." },
    { "GPUCurrentPerfLevel",              NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL,        INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports the current Performance level of the GPU driving the X screen.  Each Performance level has associated NVClock and Mem Clock values." },
    { "GPUAdaptiveClockState",            NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE,             INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports if Adaptive Clocking is Enabled on the GPU driving the X screen." },
    { "GPUPowerMizerMode",                NV_CTRL_GPU_POWER_MIZER_MODE,                 INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Allows setting different GPU powermizer modes." },
    { "GPUPowerMizerDefaultMode",         NV_CTRL_GPU_POWER_MIZER_DEFAULT_MODE,         INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports the default powermizer mode of the GPU, if any." },
    { "ECCSupported",                     NV_CTRL_GPU_ECC_SUPPORTED,                    INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports whether the underlying GPU supports ECC.  All of the other ECC attributes are only applicable if this attribute indicates that ECC is supported." },
    { "ECCStatus",                        NV_CTRL_GPU_ECC_STATUS,                       INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports whether ECC is enabled." },
    { "GPULogoBrightness",                NV_CTRL_GPU_LOGO_BRIGHTNESS,                  INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls brightness of the logo on the GPU, if any.  The value is variable from 0% - 100%." },
    { "GPUSLIBridgeLogoBrightness",       NV_CTRL_GPU_SLI_LOGO_BRIGHTNESS,              INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls brightness of the logo on the SLI bridge, if any.  The value is variable from 0% - 100%." },
    { "ECCConfigurationSupported",        NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED,      INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports whether ECC whether the ECC configuration setting can be changed." },
    { "ECCConfiguration",                 NV_CTRL_GPU_ECC_CONFIGURATION,                INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current ECC configuration setting." },
    { "ECCDefaultConfiguration",          NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION,        INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the default ECC configuration setting." },
    { "ECCSingleBitErrors",               NV_CTRL_GPU_ECC_SINGLE_BIT_ERRORS,            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the number of single-bit ECC errors detected by the targeted GPU since the last system reboot." },
    { "ECCDoubleBitErrors",               NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS,            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the number of double-bit ECC errors detected by the targeted GPU since the last system reboot." },
    { "ECCAggregateSingleBitErrors",      NV_CTRL_GPU_ECC_AGGREGATE_SINGLE_BIT_ERRORS,  INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the number of single-bit ECC errors detected by the targeted GPU since the last counter reset." },
    { "ECCAggregateDoubleBitErrors",      NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS,  INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the number of double-bit ECC errors detected by the targeted GPU since the last counter reset." },
    { "GPUFanControlState",               NV_CTRL_GPU_COOLER_MANUAL_CONTROL,            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The current fan control state; the value of this attribute controls the availability of additional fan control attributes.  Note that this attribute is unavailable unless fan control support has been enabled by setting the \"Coolbits\" X config option." },
    { "GPUTargetFanSpeed",                NV_CTRL_THERMAL_COOLER_LEVEL,                 INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the GPU fan's target speed." },
    { "GPUCurrentFanSpeed",               NV_CTRL_THERMAL_COOLER_CURRENT_LEVEL,         INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the GPU fan's current speed." },
    { "GPUResetFanSpeed",                 NV_CTRL_THERMAL_COOLER_LEVEL_SET_DEFAULT,     INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Resets the GPU fan's speed to its default." },
    { "GPUCurrentFanSpeedRPM",            NV_CTRL_THERMAL_COOLER_SPEED,                 INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the GPU fan's tachometer-measured speed in rotations per minute (RPM)." },
    { "GPUFanControlType",                NV_CTRL_THERMAL_COOLER_CONTROL_TYPE,          INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns how the GPU fan is controlled.  '1' means the fan can only be toggled on and off; '2' means the fan has variable speed.  '0' means the fan is restricted and cannot be adjusted under end user control." },
    { "GPUFanTarget",                     NV_CTRL_THERMAL_COOLER_TARGET,                INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the objects the fan cools.  '1' means the GPU, '2' means video memory, '4' means the power supply, and '7' means all of the above." },
    { "ThermalSensorReading",             NV_CTRL_THERMAL_SENSOR_READING,               INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the thermal sensor's current reading." },
    { "ThermalSensorProvider",            NV_CTRL_THERMAL_SENSOR_PROVIDER,              INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the hardware device that provides the thermal sensor." },
    { "ThermalSensorTarget",              NV_CTRL_THERMAL_SENSOR_TARGET,                INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns what hardware component the thermal sensor is measuring." },  
    { "BaseMosaic",                       NV_CTRL_BASE_MOSAIC,                          INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current Base Mosaic configuration." },
    { "MultiGpuPrimaryPossible",          NV_CTRL_MULTIGPU_PRIMARY_POSSIBLE,            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns whether or not the GPU can be configured as the primary GPU for a Multi GPU configuration (SLI, SLI Mosaic, Base Mosaic, ...)." },
    { "MultiGpuMasterPossible",           NV_CTRL_MULTIGPU_PRIMARY_POSSIBLE,            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "This attribute is deprecated. Please use 'MultiGpuPrimaryPossible' instead." },
    { "VideoEncoderUtilization",          NV_CTRL_VIDEO_ENCODER_UTILIZATION,            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the video encoder engine utilization as a percentage." },
    { "VideoDecoderUtilization",          NV_CTRL_VIDEO_DECODER_UTILIZATION,            INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the video decoder engine utilization as a percentage." },
    { "GPUCurrentClockFreqsString",       NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS,       STR_ATTR, {0,0,0,1,0}, {}, "Returns the current GPU, memory and Processor clocks of the graphics device driving the X screen." },
    { "GPUPerfModes",                     NV_CTRL_STRING_PERFORMANCE_MODES,             STR_ATTR, {0,0,0,1,0}, {}, "Returns a string with all the performance modes defined for this GPU along with their associated NV Clock and Memory Clock values." },
    { "GpuUUID",                          NV_CTRL_STRING_GPU_UUID,                      STR_ATTR, {0,0,0,1,0}, {}, "Returns the global unique identifier of the GPU." },
    { "GPUUtilization",                   NV_CTRL_STRING_GPU_UTILIZATION,               STR_ATTR, {0,0,0,1,0}, {}, "Returns the current percentage utilization of the GPU components." },
    { "GPUSlowdownTempThreshold",         NV_CTRL_GPU_SLOWDOWN_THRESHOLD,               INT_ATTR, {0,0,0,1,0}, {}, "Returns the temperature above which the GPU will slowdown for hardware protection." },
    { "GPUShutdownTempThreshold",         NV_CTRL_GPU_SHUTDOWN_THRESHOLD,               INT_ATTR, {0,0,0,1,0}, {}, "Returns the temperature at which the GPU will shutdown for hardware protection." },
    { "GPUMaxOperatingTempThreshold",     NV_CTRL_GPU_MAX_OPERATING_THRESHOLD,          INT_ATTR, {0,0,0,1,0}, {}, "Returns the maximum temperature that will support normal GPU behavior." },
    { "PlatformPowerMode",                NV_CTRL_PLATFORM_POWER_MODE,                  INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Allows setting different platform power modes." },
    { "PlatformCurrentPowerMode",         NV_CTRL_PLATFORM_CURRENT_POWER_MODE,          INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current platform power mode." },

    /* Framelock */
    { "FrameLockAvailable",               NV_CTRL_FRAMELOCK,                            INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns whether the underlying GPU supports Frame Lock.  All of the other frame lock attributes are only applicable if this attribute is enabled (Supported)." },
    { "FrameLockPolarity",                NV_CTRL_FRAMELOCK_POLARITY,                   INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Sync to the rising edge of the Frame Lock pulse, the falling edge of the Frame Lock pulse, or both." },
    { "FrameLockSyncDelay",               NV_CTRL_FRAMELOCK_SYNC_DELAY,                 INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the delay between the frame lock pulse and the GPU sync.  This is an 11 bit value which is multiplied by 7.81 to determine the sync delay in microseconds." },
    { "FrameLockSyncInterval",            NV_CTRL_FRAMELOCK_SYNC_INTERVAL,              INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "This defines the number of house sync pulses for each Frame Lock sync period.  This only applies to the server, and only when receiving house sync.  A value of zero means every house sync pulse is one frame period." },
    { "FrameLockPort0Status",             NV_CTRL_FRAMELOCK_PORT0_STATUS,               INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Input/Output status of the RJ45 port0." },
    { "FrameLockPort1Status",             NV_CTRL_FRAMELOCK_PORT1_STATUS,               INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Input/Output status of the RJ45 port1." },
    { "FrameLockHouseStatus",             NV_CTRL_FRAMELOCK_HOUSE_STATUS,               INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns whether or not the house sync signal was detected on the BNC connector of the frame lock board." },
    { "FrameLockEnable",                  NV_CTRL_FRAMELOCK_SYNC,                       INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Enable/disable the syncing of display devices to the frame lock pulse as specified by previous calls to FrameLockDisplayConfig." },
    { "FrameLockSyncReady",               NV_CTRL_FRAMELOCK_SYNC_READY,                 INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Reports whether a client frame lock board is receiving sync, whether or not any display devices are using the signal." },
    { "FrameLockFirmwareUnsupported",     NV_CTRL_GPU_FRAMELOCK_FIRMWARE_UNSUPPORTED,   INT_ATTR, {1,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns true if the RTX PRO Sync card connected to this GPU has a firmware version incompatible with this GPU." },
    { "FrameLockStereoSync",              NV_CTRL_FRAMELOCK_STEREO_SYNC,                INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "This indicates that the GPU stereo signal is in sync with the frame lock stereo signal." },
    { "FrameLockTestSignal",              NV_CTRL_FRAMELOCK_TEST_SIGNAL,                INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "To test the connections in the sync group, tell the frame lock server to enable a test signal, then query port[01] status and sync_ready on all frame lock clients.  When done, tell the server to disable the test signal.  Test signal should only be manipulated while FrameLockEnable is enabled.  The FrameLockTestSignal is also used to reset the Universal Frame Count (as returned by the glXQueryFrameCountNV() function in the GLX_NV_swap_group extension).  Note: for best accuracy of the Universal Frame Count, it is recommended to toggle the FrameLockTestSignal on and off after enabling frame lock." },
    { "FrameLockEthDetected",             NV_CTRL_FRAMELOCK_ETHERNET_DETECTED,          INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The frame lock boards are cabled together using regular cat5 cable, connecting to RJ45 ports on the backplane of the card.  There is some concern that users may think these are Ethernet ports and connect them to a router/hub/etc.  The hardware can detect this and will shut off to prevent damage (either to itself or to the router).  FrameLockEthDetected may be called to find out if Ethernet is connected to one of the RJ45 ports.  An appropriate error message should then be displayed." },
    { "FrameLockVideoMode",               NV_CTRL_FRAMELOCK_VIDEO_MODE,                 INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Get/set what video mode is used to interpret the house sync signal.  This should only be set on the frame lock server." },
    { "FrameLockSyncRate",                NV_CTRL_FRAMELOCK_SYNC_RATE,                  INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the refresh rate that the frame lock board is sending to the GPU, in mHz (Millihertz) (i.e., to get the refresh rate in Hz, divide the returned value by 1000)." },
    { "FrameLockTiming",                  NV_CTRL_FRAMELOCK_TIMING,                     INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "This is 1 when the GPU is both receiving and locked to an input timing signal.  Timing information may come from the following places: another frame lock device acting as a server, the house sync signal, or the GPU's internal timing from a display device." },
    { "FramelockUseHouseSync",            NV_CTRL_USE_HOUSE_SYNC,                       INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "When 1 (input), the server frame lock device will propagate the incoming house sync signal as the outgoing frame lock sync signal.  If the frame lock device cannot detect a frame lock sync signal, it will default to using the internal timings from the GPU connected to the primary connector.  When 2 (output), the server frame lock device will generate a house sync signal from its internal timing and output this signal over the BNC connector on the frame lock device.  This is only allowed on a RTX PRO Sync frame lock device.  If an incoming house sync signal is present on the BNC connector, setting this value to 2 (output) will have no effect." },
    { "FrameLockFPGARevision",            NV_CTRL_FRAMELOCK_FPGA_REVISION,              INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the FPGA revision of the Frame Lock device." },
    { "FrameLockFirmwareVersion",         NV_CTRL_FRAMELOCK_FIRMWARE_VERSION,           INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the firmware major version of the Frame Lock device." },
    { "FrameLockFirmwareMinorVersion",    NV_CTRL_FRAMELOCK_FIRMWARE_MINOR_VERSION,     INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the firmware minor version of the Frame Lock device." },
    { "FrameLockSyncRate4",               NV_CTRL_FRAMELOCK_SYNC_RATE_4,                INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the refresh rate that the frame lock board is sending to the GPU in 1/10000 Hz (i.e., to get the refresh rate in Hz, divide the returned value by 10000)." },
    { "FrameLockSyncDelayResolution",     NV_CTRL_FRAMELOCK_SYNC_DELAY_RESOLUTION,      INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the number of nanoseconds that one unit of FrameLockSyncDelay corresponds to." },
    { "FrameLockIncomingHouseSyncRate",   NV_CTRL_FRAMELOCK_INCOMING_HOUSE_SYNC_RATE,   INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the rate of the incoming house sync signal to the frame lock board, in mHz (Millihertz) (i.e., to get the house sync rate in Hz, divide the returned value by 1000)." },
    { "FrameLockDisplayConfig",           NV_CTRL_FRAMELOCK_DISPLAY_CONFIG,             INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the FrameLock mode of operation for the display device." },

    /* Display */
    { "Brightness",                       BRIGHTNESS_VALUE|ALL_CHANNELS,                COL_ATTR, {1,0,0,1,0}, {}, "Controls the overall brightness of the display." },
    { "RedBrightness",                    BRIGHTNESS_VALUE|RED_CHANNEL,                 COL_ATTR, {1,0,0,0,0}, {}, "Controls the brightness of the color red in the display." },
    { "GreenBrightness",                  BRIGHTNESS_VALUE|GREEN_CHANNEL,               COL_ATTR, {1,0,0,0,0}, {}, "Controls the brightness of the color green in the display." },
    { "BlueBrightness",                   BRIGHTNESS_VALUE|BLUE_CHANNEL,                COL_ATTR, {1,0,0,0,0}, {}, "Controls the brightness of the color blue in the display." },
    { "Contrast",                         CONTRAST_VALUE|ALL_CHANNELS,                  COL_ATTR, {1,0,0,1,0}, {}, "Controls the overall contrast of the display." },
    { "RedContrast",                      CONTRAST_VALUE|RED_CHANNEL,                   COL_ATTR, {1,0,0,0,0}, {}, "Controls the contrast of the color red in the display." },
    { "GreenContrast",                    CONTRAST_VALUE|GREEN_CHANNEL,                 COL_ATTR, {1,0,0,0,0}, {}, "Controls the contrast of the color green in the display." },
    { "BlueContrast",                     CONTRAST_VALUE|BLUE_CHANNEL,                  COL_ATTR, {1,0,0,0,0}, {}, "Controls the contrast of the color blue in the display." },
    { "Gamma",                            GAMMA_VALUE|ALL_CHANNELS,                     COL_ATTR, {1,0,0,1,0}, {}, "Controls the overall gamma of the display." },
    { "RedGamma",                         GAMMA_VALUE|RED_CHANNEL,                      COL_ATTR, {1,0,0,0,0}, {}, "Controls the gamma of the color red in the display." },
    { "GreenGamma",                       GAMMA_VALUE|GREEN_CHANNEL,                    COL_ATTR, {1,0,0,0,0}, {}, "Controls the gamma of the color green in the display." },
    { "BlueGamma",                        GAMMA_VALUE|BLUE_CHANNEL,                     COL_ATTR, {1,0,0,0,0}, {},"Controls the gamma of the color blue in the display." },
    { "Dithering",                        NV_CTRL_DITHERING,                            INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the dithering: auto (0), enabled (1), disabled (2)." },
    { "CurrentDithering",                 NV_CTRL_CURRENT_DITHERING,                    INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current dithering state: enabled (1), disabled (0)." },
    { "DitheringMode",                    NV_CTRL_DITHERING_MODE,                       INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the dithering mode when CurrentDithering=1; auto (0), temporally repeating dithering pattern (1), static dithering pattern (2), temporally stochastic dithering (3)." },
    { "CurrentDitheringMode",             NV_CTRL_CURRENT_DITHERING_MODE,               INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current dithering mode: none (0), temporally repeating dithering pattern (1), static dithering pattern (2), temporally stochastic dithering (3)." },
    { "DitheringDepth",                   NV_CTRL_DITHERING_DEPTH,                      INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the dithering depth when CurrentDithering=1; auto (0), 6 bits per channel (1), 8 bits per channel (2)." },
    { "CurrentDitheringDepth",            NV_CTRL_CURRENT_DITHERING_DEPTH,              INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current dithering depth: none (0), 6 bits per channel (1), 8 bits per channel (2)." },
    { "DigitalVibrance",                  NV_CTRL_DIGITAL_VIBRANCE,                     INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Sets the digital vibrance level of the display device." },
    { "ImageSharpening",                  NV_CTRL_IMAGE_SHARPENING,                     INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Adjusts the sharpness of the display's image quality by amplifying high frequency content." },
    { "ImageSharpeningDefault",           NV_CTRL_IMAGE_SHARPENING_DEFAULT,             INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns default value of image sharpening." },
    { "FrontendResolution",               NV_CTRL_FRONTEND_RESOLUTION,                  INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "Returns the dimensions of the frontend (current) resolution as determined by the NVIDIA X Driver.  This attribute is a packed integer; the width is packed in the upper 16 bits and the height is packed in the lower 16-bits." },
    { "BackendResolution",                NV_CTRL_BACKEND_RESOLUTION,                   INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "Returns the dimensions of the backend resolution as determined by the NVIDIA X Driver.  The backend resolution is the resolution (supported by the display device) the GPU is set to scale to.  If this resolution matches the frontend resolution, GPU scaling will not be needed/used.  This attribute is a packed integer; the width is packed in the upper 16-bits and the height is packed in the lower 16-bits." },
    { "FlatpanelNativeResolution",        NV_CTRL_FLATPANEL_NATIVE_RESOLUTION,          INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "Returns the dimensions of the native resolution of the flat panel as determined by the NVIDIA X Driver.  The native resolution is the resolution at which a flat panel must display any image.  All other resolutions must be scaled to this resolution through GPU scaling or the DFP's native scaling capabilities in order to be displayed.  This attribute is only valid for flat panel (DFP) display devices.  This attribute is a packed integer; the width is packed in the upper 16-bits and the height is packed in the lower 16-bits." },
    { "FlatpanelBestFitResolution",       NV_CTRL_FLATPANEL_BEST_FIT_RESOLUTION,        INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,1,0,0,0,0} }, "Returns the dimensions of the resolution, selected by the X driver, from the DFP's EDID that most closely matches the frontend resolution of the current mode.  The best fit resolution is selected on a per-mode basis.  This attribute is only valid for flat panel (DFP) display devices.  This attribute is a packed integer; the width is packed in the upper 16-bits and the height is packed in the lower 16-bits." },
    { "DFPScalingActive",                 NV_CTRL_DFP_SCALING_ACTIVE,                   INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current state of DFP scaling.  DFP scaling is mode-specific (meaning it may vary depending on which mode is currently set).  DFP scaling is active if the GPU is set to scale to the best fit resolution (GPUScaling is set to use FlatpanelBestFitResolution) and the best fit and native resolutions are different." },
    { "GPUScaling",                       NV_CTRL_GPU_SCALING,                          INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,1,0,0,0,0} }, "Controls what the GPU scales to and how.  This attribute is a packed integer; the scaling target (native/best fit) is packed in the upper 16-bits and the scaling method is packed in the lower 16-bits." },
    { "GPUScalingDefaultTarget",          NV_CTRL_GPU_SCALING_DEFAULT_TARGET,           INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the default gpu scaling target for the Flatpanel." },
    { "GPUScalingDefaultMethod",          NV_CTRL_GPU_SCALING_DEFAULT_METHOD,           INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the default gpu scaling method for the Flatpanel." },
    { "GPUScalingActive",                 NV_CTRL_GPU_SCALING_ACTIVE,                   INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current state of GPU scaling.  GPU scaling is mode-specific (meaning it may vary depending on which mode is currently set).  GPU scaling is active if the frontend timing (current resolution) is different than the target resolution.  The target resolution is either the native resolution of the flat panel or the best fit resolution supported by the flat panel.  What (and how) the GPU should scale to is controlled through the GPUScaling attribute." },
    { "RefreshRate",                      NV_CTRL_REFRESH_RATE,                         INT_ATTR, {0,0,0,1,0}, { .int_flags = {1,0,0,0,0,0,0} }, "Returns the refresh rate of the specified display device in cHz (Centihertz) (to get the refresh rate in Hz, divide the returned value by 100)." },
    { "RefreshRate3",                     NV_CTRL_REFRESH_RATE_3,                       INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,1,0,0,0,0,0} }, "Returns the refresh rate of the specified display device in mHz (Millihertz) (to get the refresh rate in Hz, divide the returned value by 1000)." },
    { "OverscanCompensation",             NV_CTRL_OVERSCAN_COMPENSATION,                INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Adjust the amount of overscan compensation scaling, in pixels, to apply to the specified display device." },
    { "ColorSpace",                       NV_CTRL_COLOR_SPACE,                          INT_ATTR, {1,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Sets the preferred color space of the signal sent to the display device." },
    { "ColorRange",                       NV_CTRL_COLOR_RANGE,                          INT_ATTR, {1,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Sets the preferred color range of the signal sent to the display device." },
    { "CurrentColorSpace",                NV_CTRL_CURRENT_COLOR_SPACE,                  INT_ATTR, {1,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current color space of the signal sent to the display device." },
    { "CurrentColorRange",                NV_CTRL_CURRENT_COLOR_RANGE,                  INT_ATTR, {1,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the current color range of the signal sent to the display device." },
    { "SynchronousPaletteUpdates",        NV_CTRL_SYNCHRONOUS_PALETTE_UPDATES,          INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls whether colormap updates are synchronized with X rendering." },
    { "CurrentMetaModeID",                NV_CTRL_CURRENT_METAMODE_ID,                  INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The ID of the current MetaMode." },
    { "RandROutputID",                    NV_CTRL_DISPLAY_RANDR_OUTPUT_ID,              INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The RandR Output ID that corresponds to the display device." },
    { "Hdmi3D",                           NV_CTRL_DPY_HDMI_3D,                          INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns whether the specified display device is currently using HDMI 3D Frame Packed Stereo mode. If so, the result of refresh rate queries will be doubled." },
    { "BacklightBrightness",              NV_CTRL_BACKLIGHT_BRIGHTNESS,                 INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the backlight brightness of an internal panel." },
    { "CurrentMetaMode",                  NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2,    STR_ATTR, {0,0,0,1,0}, {}, "Controls the current MetaMode." },
    { "XineramaInfoOrder",                NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER,    STR_ATTR, {0,0,0,1,0}, {}, "Controls the nvidiaXineramaInfoOrder." },
    { "BuildModepool",                    NV_CTRL_STRING_OPERATION_BUILD_MODEPOOL,      SOP_ATTR, {0,0,0,1,1}, {}, "Build the modepool of the display device if it does not already have one." },
    { "DisplayPortConnectorType",         NV_CTRL_DISPLAYPORT_CONNECTOR_TYPE,           INT_ATTR, {0,0,0,1,0}, {}, "Returns the DisplayPort connector type."},
    { "DisplayPortIsMultiStream",         NV_CTRL_DISPLAYPORT_IS_MULTISTREAM,           INT_ATTR, {0,0,0,1,0}, {}, "Returns 1 if the DisplayPort display is a MultiStream device, and 0 otherwise."},
    { "DisplayPortSinkIsAudioCapable",    NV_CTRL_DISPLAYPORT_SINK_IS_AUDIO_CAPABLE,    INT_ATTR, {0,0,0,1,0}, {}, "Returns 1 if the DisplayPort display is capable of playing audio, and 0 otherwise."},
    { "DisplayVRRMode",                   NV_CTRL_DISPLAY_VRR_MODE,                     INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Whether the specified display device is G-SYNC or G-SYNC Compatible." },
    { "DisplayVRRMinRefreshRate",         NV_CTRL_DISPLAY_VRR_MIN_REFRESH_RATE,         INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The minimum refresh rate for the specified VRR display device." },
    { "DisplayVRREnabled",                NV_CTRL_DISPLAY_VRR_ENABLED,                  INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "If this is enabled (1), then VRR was enabled on this display at modeset time." },
    { "NumberOfHardwareHeadsUsed",        NV_CTRL_NUMBER_OF_HARDWARE_HEADS_USED,        INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the number of hardware heads currently used by this display device." },
    { "FlatpanelSignal",                  NV_CTRL_FLATPANEL_SIGNAL,                     INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Report whether the flat panel is driven by an LVDS, TMDS, DisplayPort, or HDMI FRL (fixed-rate link) signal." },

    /* TV */
    { "TVOverScan",                       NV_CTRL_TV_OVERSCAN,                          INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Adjusts the amount of overscan on the specified display device." },
    { "TVFlickerFilter",                  NV_CTRL_TV_FLICKER_FILTER,                    INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Adjusts the amount of flicker filter on the specified display device." },
    { "TVBrightness",                     NV_CTRL_TV_BRIGHTNESS,                        INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Adjusts the amount of brightness on the specified display device." },
    { "TVHue",                            NV_CTRL_TV_HUE,                               INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Adjusts the amount of hue on the specified display device." },
    { "TVContrast",                       NV_CTRL_TV_CONTRAST,                          INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Adjusts the amount of contrast on the specified display device." },
    { "TVSaturation",                     NV_CTRL_TV_SATURATION,                        INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Adjusts the amount of saturation on the specified display device." },

    /* X Video */
    { "XVideoSyncToDisplay",              NV_CTRL_XV_SYNC_TO_DISPLAY,                   INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,1,0,1,0} }, "DEPRECATED: Use \"XVideoSyncToDisplayID\" instead." },
    { "XVideoSyncToDisplayID",            NV_CTRL_XV_SYNC_TO_DISPLAY_ID,                INT_ATTR, {0,0,0,0,0}, { .int_flags = {0,0,0,0,1,0,0} }, "Controls which display device is synced to by the XVideo texture and blitter adaptors when they are set to synchronize to the vertical blanking." },
    { "CurrentXVideoSyncToDisplayID",     NV_CTRL_CURRENT_XV_SYNC_TO_DISPLAY_ID,        INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,1,0,0} }, "Returns the display device synced to by the XVideo texture and blitter adaptors when they are set to synchronize to the vertical blanking." },

    /* 3D Vision Pro */
    { "3DVisionProResetTransceiverToFactorySettings", NV_CTRL_3D_VISION_PRO_RESET_TRANSCEIVER_TO_FACTORY_SETTINGS, INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Resets the 3D Vision Pro transceiver to its factory settings."},
    { "3DVisionProTransceiverChannel",                NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL,                   INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the channel that is currently used by the 3D Vision Pro transceiver."},
    { "3DVisionProTransceiverMode",                   NV_CTRL_3D_VISION_PRO_TRANSCEIVER_MODE,                      INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the mode in which the 3D Vision Pro transceiver operates."},
    { "3DVisionProTransceiverChannelFrequency",       NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_FREQUENCY,         INT_ATTR, {0,0,1,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the frequency of the channel(in kHz) of the 3D Vision Pro transceiver."},
    { "3DVisionProTransceiverChannelQuality",         NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_QUALITY,           INT_ATTR, {0,0,1,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the quality of the channel(in percentage) of the 3D Vision Pro transceiver."},
    { "3DVisionProTransceiverChannelCount",           NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_COUNT,             INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the number of channels on the 3D Vision Pro transceiver."},
    { "3DVisionProPairGlasses",                       NV_CTRL_3D_VISION_PRO_PAIR_GLASSES,                          INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Puts the 3D Vision Pro transceiver into pairing mode to gather additional glasses."},
    { "3DVisionProUnpairGlasses",                     NV_CTRL_3D_VISION_PRO_UNPAIR_GLASSES,                        INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Tells a specific pair of glasses to unpair."},
    { "3DVisionProDiscoverGlasses",                   NV_CTRL_3D_VISION_PRO_DISCOVER_GLASSES,                      INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Tells the 3D Vision Pro transceiver about the glasses that have been paired using NV_CTRL_3D_VISION_PRO_PAIR_GLASSES_BEACON."},
    { "3DVisionProIdentifyGlasses",                   NV_CTRL_3D_VISION_PRO_IDENTIFY_GLASSES,                      INT_ATTR, {0,0,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Causes glasses LEDs to flash for a short period of time."},
    { "3DVisionProGlassesSyncCycle",                  NV_CTRL_3D_VISION_PRO_GLASSES_SYNC_CYCLE,                    INT_ATTR, {0,0,1,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Controls the sync cycle duration(in milliseconds) of the glasses."},
    { "3DVisionProGlassesMissedSyncCycles",           NV_CTRL_3D_VISION_PRO_GLASSES_MISSED_SYNC_CYCLES,            INT_ATTR, {0,0,1,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the number of state sync cycles recently missed by the glasses."},
    { "3DVisionProGlassesBatteryLevel",               NV_CTRL_3D_VISION_PRO_GLASSES_BATTERY_LEVEL,                 INT_ATTR, {0,0,1,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Returns the battery level(in percentage) of the glasses."},
    { "3DVisionProTransceiverHardwareRevision",       NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_HARDWARE_REVISION,  STR_ATTR, {0,0,0,1,0}, {}, "Returns the hardware revision of the 3D Vision Pro transceiver."},
    { "3DVisionProTransceiverFirmwareVersionA",       NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_VERSION_A, STR_ATTR, {0,0,0,1,0}, {}, "Returns the firmware version of chip A of the 3D Vision Pro transceiver."},
    { "3DVisionProTransceiverFirmwareDateA",          NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_DATE_A,    STR_ATTR, {0,0,0,1,0}, {}, "Returns the date of the firmware of chip A of the 3D Vision Pro transceiver."},
    { "3DVisionProTransceiverFirmwareVersionB",       NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_VERSION_B, STR_ATTR, {0,0,0,1,0}, {}, "Returns the firmware version of chip B of the 3D Vision Pro transceiver."},
    { "3DVisionProTransceiverFirmwareDateB",          NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_DATE_B,    STR_ATTR, {0,0,0,1,0}, {}, "Returns the date of the firmware of chip B of the 3D Vision Pro transceiver."},
    { "3DVisionProTransceiverAddress",                NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_ADDRESS,            STR_ATTR, {0,0,0,1,0}, {}, "Returns the RF address of the 3D Vision Pro transceiver."},
    { "3DVisionProGlassesFirmwareVersionA",           NV_CTRL_STRING_3D_VISION_PRO_GLASSES_FIRMWARE_VERSION_A,     STR_ATTR, {0,0,1,1,0}, {}, "Returns the firmware version of chip A of the glasses."},
    { "3DVisionProGlassesFirmwareDateA",              NV_CTRL_STRING_3D_VISION_PRO_GLASSES_FIRMWARE_DATE_A,        STR_ATTR, {0,0,1,1,0}, {}, "Returns the date of the firmware of chip A of the glasses."},
    { "3DVisionProGlassesAddress",                    NV_CTRL_STRING_3D_VISION_PRO_GLASSES_ADDRESS,                STR_ATTR, {0,0,1,1,0}, {}, "Returns the RF address of the glasses."},
    { "3DVisionProGlassesName",                       NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME,                   STR_ATTR, {0,0,1,1,0}, {}, "Controls the name the glasses should use."},

    /* Mux Control */
    { "MuxState",                                     NV_CTRL_STRING_MUX_STATE,         STR_ATTR, {0,0,0,1,0}, {}, "Controls whether the specified mux is configured to use the discrete or integrated GPU."},
    { "MuxAutoSwitch",                                NV_CTRL_MUX_AUTO_SWITCH,          INT_ATTR, {0,0,0,1,0}, {}, "Controls whether the specified mux switches automatically."},
    { "MuxIsInternal",                                NV_CTRL_MUX_IS_INTERNAL,          INT_ATTR, {0,0,0,1,0}, {}, "Returns whether the specified mux is internal."},


    /* Misc */
    { "GTFModeline",                      NV_CTRL_STRING_OPERATION_GTF_MODELINE,        SOP_ATTR, {0,0,0,1,1}, { }, "Builds a modeline using the GTF formula." },
    { "CVTModeline",                      NV_CTRL_STRING_OPERATION_CVT_MODELINE,        SOP_ATTR, {0,0,0,1,1}, { }, "Builds a modeline using the CVT formula." },

    /* Dynamic Boost */
    { "DynamicBoostSupport",              NV_CTRL_DYNAMIC_BOOST_SUPPORT,                INT_ATTR, {0,0,0,1,0}, {},  "Returns whether the system supports Dynamic Boost." },

    { "GpuGetPowerUsage",                 NV_CTRL_ATTR_NVML_GPU_GET_POWER_USAGE,        INT_ATTR, {0,0,0,1,1}, {},  "Returns the current power usage of the GPU, in Watts." },
    { "GPUMaxTGP",                        NV_CTRL_ATTR_NVML_GPU_MAX_TGP,                INT_ATTR, {0,0,0,1,1}, {},  "Returns the max TGP of the GPU, in Watts." },
    { "GPUDefaultTGP",                    NV_CTRL_ATTR_NVML_GPU_DEFAULT_TGP,            INT_ATTR, {0,0,0,1,1}, {},  "Returns the default TGP of the GPU, in Watts." },

    /* Framelock sync multiplier/divider */
    { "FrameLockMultiplyDivideValue",     NV_CTRL_FRAMELOCK_MULTIPLY_DIVIDE_VALUE,      INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "The value to multiply or divide the house sync input rate by before comparing it to this framelock board's internal sync rate." },
    { "FrameLockMultiplyDivideMode",      NV_CTRL_FRAMELOCK_MULTIPLY_DIVIDE_MODE,       INT_ATTR, {1,1,0,1,0}, { .int_flags = {0,0,0,0,0,0,0} }, "Whether the house sync rate should be multiplied or divided." },
};

const int attributeTableLen = ARRAY_LEN(attributeTable);

/*
 * When new attributes are added to NVCtrl.h, an entry should be added in the
 * above attributeTable[].  The below #if should also be updated to indicate
 * the last attribute that the table knows about.
 */

#if NV_CTRL_LAST_ATTRIBUTE != NV_CTRL_DISPLAYPORT_LINK_RATE_10MHZ
#warning "Have you forgotten to add a new integer attribute to attributeTable?"
#endif

#if NV_CTRL_STRING_LAST_ATTRIBUTE != NV_CTRL_STRING_MUX_STATE
#warning "Have you forgotten to add a new string attribute to attributeTable?"
#endif

#if NV_CTRL_STRING_OPERATION_LAST_ATTRIBUTE != NV_CTRL_STRING_OPERATION_PARSE_METAMODE
#warning "Have you forgotten to add a new string operation attribute to attributeTable?"
#endif



/*
 * returns the corresponding attribute entry for the given attribute constant.
 *
 */
const AttributeTableEntry *nv_get_attribute_entry(const int attr,
                                                  const CtrlAttributeType type)
{
    int i;

    for (i = 0; i < attributeTableLen; i++) {
        const AttributeTableEntry *a = attributeTable + i;
        if ((a->attr == attr) && (a->type == type)) {
            return a;
        }
    }

    return NULL;
}


/*
 * returns the corresponding attribute entry for the given attribute
 * name.
 *
 */
static const AttributeTableEntry *nv_get_attribute_entry_by_name(const char *name)
{
    int i;

    for (i = 0; i < attributeTableLen; i++) {
        const AttributeTableEntry *t = attributeTable + i;
        if (nv_strcasecmp(name, t->name)) {
            return t;
        }
    }

    return NULL;
}



/*!
 * Return whether the string defined by 'start' and 'end' is a simple numerical
 * value; and if so, assigns the value to where 'val' points.
 *
 * \param[in]  start  Start of the string to parse.
 * \param[in]  end    End of the string to parse, or NULL if the string is NULL-
 *                    terminated.
 * \param[out] val    Points to the integer that should hold the parsed value,
 *                    if the string is a numeric.
 *
 * \return  Return NV_TRUE if the string was a simple numerical value and
 *          'val' was assigned; else, returns NV_FALSE and 'val' is not
 *          modified.
 */

int nv_parse_numerical(const char *start, const char *end, int *val)
{
    int num = 0;
    const char *s;

    for (s = start;
         *s && (!end || (s < end));
         s++) {
        if (!isdigit(*s)) {
            return NV_FALSE;
        }
        num = (num * 10) + ctoi(*s);
    }

    *val = num;
    return NV_TRUE;
}



/*!
 * Parse the string as a (special case) X screen number.
 *
 * Return whether the string defined by 'start' and 'end' is a simple numerical
 * value that was applied to the ParsedAttribute 'p' as an X screen target
 * type/id.
 *
 * \param[in]  start  Start of the string to parse.
 * \param[in]  end    End of the string to parse, or NULL if the string is NULL-
 *                    terminated.
 * \param[out] p      ParsedAttribute to set as an X screen target if the string
 *                    is found to be a simple numeric.
 *
 * \return  Return NV_TRUE if the string was a simple numerical value and 'a'
 *          was modified; else, return NV_FALSE.
 */

static int nv_parse_special_xscreen_target(ParsedAttribute *p,
                                           const char *start,
                                           const char *end)
{
    if (!nv_parse_numerical(start, end, &(p->target_id))) {
        return FALSE;
    }

    p->parser_flags.has_target = NV_TRUE;
    p->target_type = X_SCREEN_TARGET;

    return NV_TRUE;
}



/*!
 * Parse the string as one of either: an X Display name, just an X screen, and/
 * or a target specification in which the string can be in one of the following
 * formats:
 *
 *     {screen}
 *     {host}:{display}.{screen}
 *     {host}:{display}[{target-type}:{target-id}
 *
 * This is a helper for nv_parse_attribute_string() for parsing all the text
 * before the DISPLAY_NAME_SEPARATOR.
 *
 * \param[in]  start  Start of the string to parse.
 * \param[in]  end    End of the string to parse.
 * \param[out] p      ParsedAttribute to be modified with the X Display and/or
 *                    target type + target id or generic specification
 *                    information.
 *
 * \return  Return NV_PARSER_STATUS_SUCCESS if the string was successfully
 *          parsed; Else, one of the NV_PARSER_STATUS_XXX errors that describes
 *          the parsing error encountered.
 */

static int nv_parse_display_and_target(const char *start,
                                       const char *end, /* exclusive */
                                       ParsedAttribute *p)
{
    int len;
    const char *s, *pOpen, *pClose;
    const char *startDisplayName;
    const char *endDisplayName;

    /* Set target specification related defaults */

    p->display = NULL;
    p->target_id = -1;
    p->target_type = INVALID_TARGET;
    p->target_specification = NULL;
    p->parser_flags.has_x_display = NV_FALSE;
    p->parser_flags.has_target = NV_FALSE;

    /*
     * If the string consists of digits only, then this is a special case where
     * the X screen number is being specified.
     */

    if (nv_parse_special_xscreen_target(p, start, end)) {
        return NV_PARSER_STATUS_SUCCESS;
    }

    /*
     * If we get here, then there are non-digit characters; look for a pair of
     * brackets, and treat the contents as a target specification.
     */

    pOpen = pClose = NULL;

    for (s = start; s < end; s++) {
        if (*s == '[') pOpen = s;
        if (*s == ']') pClose = s;
    }

    startDisplayName = start;
    endDisplayName = end;

    if (pOpen && pClose && (pClose > pOpen) && ((pClose - pOpen) > 1)) {

        /*
         * check that there is no stray text between the closing bracket and the
         * end of our parsable string.
         */

        if ((end - pClose) > 1) {
            return NV_PARSER_STATUS_TARGET_SPEC_TRAILING_GARBAGE;
        }

        /*
         * Save everything between the pair of brackets as the target
         * specification to be parsed (against the list of targets on the
         * specified/default X Display) later.
         */

        len = pClose - pOpen - 1;

        p->target_specification = nvstrndup(pOpen + 1, len);

        /*
         * The X Display name should end on the opening bracket of the target
         * specification.
         */

        endDisplayName = pOpen;
    }

    /* treat everything between start and end as an X Display name */

    if (startDisplayName < endDisplayName) {

        p->display = nvstrndup(startDisplayName,
                               endDisplayName - startDisplayName);
        p->parser_flags.has_x_display = NV_TRUE;

        /*
         * this will attempt to parse out any screen number from the
         * display name
         */

        nv_assign_default_display(p, NULL);
    }

    return NV_PARSER_STATUS_SUCCESS;
}



/*
 * nv_parse_attribute_string() - see comments in parse.h
 */

int nv_parse_attribute_string(const char *str, int query, ParsedAttribute *p)
{
    char *s, *tmp, *name, *start, *equal_sign, *no_spaces = NULL;
    char tmpname[NV_PARSER_MAX_NAME_LEN];
    int len, ret;
    const AttributeTableEntry *a;

#define stop(x) { if (no_spaces) free(no_spaces); return (x); }

    if (!p) {
        stop(NV_PARSER_STATUS_BAD_ARGUMENT);
    }

    /* clear the ParsedAttribute struct */

    memset((void *) p, 0, sizeof(ParsedAttribute));
    p->target_id = -1;
    p->target_type = INVALID_TARGET;

    /* remove any white space from the string, to simplify parsing */

    no_spaces = remove_spaces(str);
    if (!no_spaces) stop(NV_PARSER_STATUS_EMPTY_STRING);

    /*
     * temporarily terminate the string at the equal sign, so that the
     * DISPLAY_NAME_SEPARATOR search does not extend too far
     */
    equal_sign = NULL;
    if (query == NV_PARSER_ASSIGNMENT) {
        equal_sign = strchr(no_spaces, '=');
    }

    if (equal_sign) {
        *equal_sign = '\0';
    }

    /*
     * get the display name... i.e.,: everything before the
     * DISPLAY_NAME_SEPARATOR
     */

    s = strchr(no_spaces, DISPLAY_NAME_SEPARATOR);

    /*
     * If we found a DISPLAY_NAME_SEPARATOR, and there is some text
     * before it, parse that text as an X Display name, X screen,
     * and/or a target specification.
     */

    if ((s) && (s != no_spaces)) {

        ret = nv_parse_display_and_target(no_spaces, s, p);

        if (ret != NV_PARSER_STATUS_SUCCESS) {
            stop(ret);
        }
    }

    if (equal_sign) {
        *equal_sign = '=';
    }

    /* move past the DISPLAY_NAME_SEPARATOR */
    
    if (s) s++;
    else s = no_spaces;
    
    /* read the attribute name */

    name = s;
    len = 0;
    while (*s && isalnum(*s)) { s++; len++; }
    
    if (len == 0) stop(NV_PARSER_STATUS_ATTR_NAME_MISSING);
    if (len >= NV_PARSER_MAX_NAME_LEN)
        stop(NV_PARSER_STATUS_ATTR_NAME_TOO_LONG);

    strncpy(tmpname, name, len);
    tmpname[len] = '\0';

    /* look up the requested attribute */

    a = nv_get_attribute_entry_by_name(tmpname);
    if (!a) {
        stop(NV_PARSER_STATUS_UNKNOWN_ATTR_NAME);
    }

    p->attr_entry = a;

    /* read the display device specification */

    if (*s == '[') {
        char *mask_str;
        s++;
        start = s;
        while (*s && *s != ']') {
            s++;
        }
        tmp = nvstrndup(start, s - start);
        mask_str = remove_spaces(tmp);
        nvfree(tmp);

        p->display_device_mask = strtoul(mask_str, &tmp, 0);
        if (*mask_str != '\0' &&
            tmp &&
            *tmp == '\0') {
            /* specification given as integer */
            nvfree(mask_str);
        } else {
            /* specification given as string (list of display names) */
            if (a->flags.hijack_display_device) {
                /* If the display specification (mask) is being hijacked, the
                 * value should have been a valid integer.
                 */
                stop(NV_PARSER_STATUS_BAD_DISPLAY_DEVICE);
            }
            p->display_device_specification = mask_str;
        }
        p->parser_flags.has_display_device = NV_TRUE;
        if (*s == ']') {
            s++;
        }
    }

    if (query == NV_PARSER_ASSIGNMENT) {

        /* there should be an equal sign */

        if (*s == '=') s++;
        else stop(NV_PARSER_STATUS_MISSING_EQUAL_SIGN);

        /* read the value */

        tmp = s;
        switch (a->type) {
        case CTRL_ATTRIBUTE_TYPE_INTEGER:
            if (a->f.int_flags.is_packed) {
                /*
                 * Either a single 32-bit integer or two 16-bit integers,
                 * separated by ','.  Passing base as 0 allows packed values to
                 * be specified in hex.
                 */
                p->val.i = strtol(s, &tmp, 0);
                if (tmp && *tmp == ',') {
                    p->val.i = (p->val.i & 0xffff) << 16;
                    p->val.i |= strtol((tmp + 1), &tmp, 0) & 0xffff;
                }
            } else if (a->f.int_flags.is_display_mask) {
                /* Value is a display mask (as a string or integer */
                if (nv_strcasecmp(s, "alldisplays")) {
                    p->parser_flags.assign_all_displays = NV_TRUE;
                    tmp = s + strlen(s);
                } else {
                    uint32 mask = 0;
                    mask = display_device_name_to_display_device_mask(s);
                    if (mask && (mask != INVALID_DISPLAY_DEVICE_MASK) &&
                        ((mask & (DISPLAY_DEVICES_WILDCARD_CRT |
                                  DISPLAY_DEVICES_WILDCARD_TV |
                                  DISPLAY_DEVICES_WILDCARD_DFP)) == 0)) {
                        p->val.i = mask;
                        tmp = s + strlen(s);
                    } else {
                        p->val.i = strtol(s, &tmp, 0);
                    }
                }
            } else if (a->f.int_flags.is_display_id) {
                /* Value is Display ID that can use the display names */
                p->val.str = nvstrdup(s);
                tmp = s + strlen(s);
            } else {
                /* Read just an integer */
                p->val.i = strtol(s, &tmp, 0);
            }
            break;

        case CTRL_ATTRIBUTE_TYPE_STRING:
            /* Fall through */
        case CTRL_ATTRIBUTE_TYPE_STRING_OPERATION:
        case CTRL_ATTRIBUTE_TYPE_BINARY_DATA:
            p->val.str = nvstrdup(s);
            tmp = s + strlen(s);
            break;

        case  CTRL_ATTRIBUTE_TYPE_COLOR:
            /* color attributes are floating point */
            p->val.f = strtod(s, &tmp);
            break;
        }

        if (tmp && (s != tmp)) {
            p->parser_flags.has_value = NV_TRUE;
        }
        s = tmp;

        if (!(p->parser_flags.has_value)) {
            stop(NV_PARSER_STATUS_NO_VALUE);
        }
    }

    /* this should be the end of the string */

    if (*s != '\0') stop(NV_PARSER_STATUS_TRAILING_GARBAGE);

    stop(NV_PARSER_STATUS_SUCCESS);

} /* nv_parse_attribute_string() */



/*
 * nv_parse_strerror() - given the error status returned by
 * nv_parse_attribute_string(), return a string describing the
 * error.
 */

const char *nv_parse_strerror(int status)
{
    switch (status) {
    case NV_PARSER_STATUS_SUCCESS :
        return "No error"; break;
    case NV_PARSER_STATUS_BAD_ARGUMENT :
        return "Bad argument"; break;
    case NV_PARSER_STATUS_EMPTY_STRING :
        return "Empty string"; break;
    case NV_PARSER_STATUS_ATTR_NAME_TOO_LONG :
        return "The attribute name is too long"; break;
    case NV_PARSER_STATUS_ATTR_NAME_MISSING :
        return "Missing attribute name"; break;
    case NV_PARSER_STATUS_BAD_DISPLAY_DEVICE :
        return "Malformed display device identification"; break;
    case NV_PARSER_STATUS_MISSING_EQUAL_SIGN :
        return "Missing equal sign after attribute name"; break;
    case NV_PARSER_STATUS_NO_VALUE :
        return "No attribute value specified"; break;
    case NV_PARSER_STATUS_TRAILING_GARBAGE :
        return "Trailing garbage"; break;
    case NV_PARSER_STATUS_UNKNOWN_ATTR_NAME :
        return "Unrecognized attribute name"; break;
    case NV_PARSER_STATUS_MISSING_COMMA:
        return "Missing comma in packed integer value"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_NO_COLON:
        return "No colon in target specification"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET:
        return "Bad target in target specification"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_NO_TARGET_ID:
        return "No target ID in target specification"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET_ID:
        return "Bad target ID in target specification"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_TRAILING_GARBAGE:
        return "Trailing garbage after target specification"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_NO_TARGETS:
        return "No targets match target specification"; break;

    default:
        return "Unknown error"; break;
    }
} /* nv_parse_strerror() */



/*
 * *sigh* strcasecmp() is a BSDism, and when building with "-ansi" we
 * don't get the prototype, so reimplement it to avoid a compiler
 * warning.  Returns NV_TRUE if a match, returns NV_FALSE if there is
 * no match.
 */

int nv_strcasecmp(const char *a, const char *b)
{
    if (!a && !b) return NV_TRUE;
    if (!a &&  b) return NV_FALSE;
    if ( a && !b) return NV_FALSE;

    while (toupper(*a) == toupper(*b)) {
        a++;
        b++;
        if ((*a == '\0') && (*b == '\0')) return NV_TRUE;
    }

    return NV_FALSE;

} /* nv_strcasecmp() */



/*
 * display_name_to_display_device_mask() - parse the string that describes a
 * display device mask; the string is a comma-separated list of
 * display device names, where valid names are:
 *
 * CRT-[0,7] TV-[0,7] and DFP[0,7]
 *
 * Non-specific names ("CRT", "TV", and "DFP") are also allowed; if
 * these are specified, then the appropriate WILDCARD flag in the
 * upper-most byte of the display device mask is set:
 *
 *    DISPLAY_DEVICES_WILDCARD_CRT
 *    DISPLAY_DEVICES_WILDCARD_TV
 *    DISPLAY_DEVICES_WILDCARD_DFP
 *
 * If a parse error occurs, INVALID_DISPLAY_DEVICE_MASK is returned,
 * otherwise the display mask is returned.
 
 */

static uint32 display_device_name_to_display_device_mask(const char *str)
{
    uint32 mask = 0;
    char *s, **toks, *endptr;
    int i, n;
    unsigned long int num;

    /* sanity check */

    if (!str || !*str) return INVALID_DISPLAY_DEVICE_MASK;

    /* remove spaces from the string */

    s = remove_spaces(str);
    if (!s || !*s) {
        if (s) {
            free(s);
        }
        return INVALID_DISPLAY_DEVICE_MASK;
    }

    /*
     * can the string be interpreted as a number? if so, use the number
     * as the mask
     */

    num = strtoul(s, &endptr, 0);
    if (*endptr == '\0') {
        free(s);
        return (uint32) num;
    }

    /* break up the string by commas */

    toks = nv_strtok(s, ',', &n);
    if (!toks) {
        free(s);
        return INVALID_DISPLAY_DEVICE_MASK;
    }

    /* match each token, updating mask as appropriate */

    for (i = 0; i < n; i++) {
        
        if      (nv_strcasecmp(toks[i], "CRT-0")) mask |= ((1 << 0) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-1")) mask |= ((1 << 1) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-2")) mask |= ((1 << 2) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-3")) mask |= ((1 << 3) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-4")) mask |= ((1 << 4) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-5")) mask |= ((1 << 5) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-6")) mask |= ((1 << 6) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-7")) mask |= ((1 << 7) << 0);

        else if (nv_strcasecmp(toks[i], "TV-0" )) mask |= ((1 << 0) << 8);
        else if (nv_strcasecmp(toks[i], "TV-1" )) mask |= ((1 << 1) << 8);
        else if (nv_strcasecmp(toks[i], "TV-2" )) mask |= ((1 << 2) << 8);
        else if (nv_strcasecmp(toks[i], "TV-3" )) mask |= ((1 << 3) << 8);
        else if (nv_strcasecmp(toks[i], "TV-4" )) mask |= ((1 << 4) << 8);
        else if (nv_strcasecmp(toks[i], "TV-5" )) mask |= ((1 << 5) << 8);
        else if (nv_strcasecmp(toks[i], "TV-6" )) mask |= ((1 << 6) << 8);
        else if (nv_strcasecmp(toks[i], "TV-7" )) mask |= ((1 << 7) << 8);

        else if (nv_strcasecmp(toks[i], "DFP-0")) mask |= ((1 << 0) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-1")) mask |= ((1 << 1) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-2")) mask |= ((1 << 2) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-3")) mask |= ((1 << 3) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-4")) mask |= ((1 << 4) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-5")) mask |= ((1 << 5) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-6")) mask |= ((1 << 6) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-7")) mask |= ((1 << 7) << 16);
        
        else if (nv_strcasecmp(toks[i], "CRT"))
            mask |= DISPLAY_DEVICES_WILDCARD_CRT;
        
        else if (nv_strcasecmp(toks[i], "TV"))
            mask |= DISPLAY_DEVICES_WILDCARD_TV;
                
        else if (nv_strcasecmp(toks[i], "DFP"))
            mask |= DISPLAY_DEVICES_WILDCARD_DFP;

        else {
            mask = INVALID_DISPLAY_DEVICE_MASK;
            break;
        }
    }
    
    nv_free_strtoks(toks, n);
    
    free(s);

    return mask;
    
} /* display_name_to_display_device_mask() */



/*
 * display_device_mask_to_display_name() - construct a string
 * describing the given display device mask.  The returned pointer
 * points to a newly allocated string, so callers to this function
 * are responsible for freeing the memory.
 */

#define DISPLAY_DEVICE_STRING_LEN 256

char *display_device_mask_to_display_device_name(const uint32 mask)
{
    char *s;
    int first = NV_TRUE;
    uint32 devcnt, devmask;
    char *display_device_name_string;

    display_device_name_string = nvalloc(DISPLAY_DEVICE_STRING_LEN);

    s = display_device_name_string;

    devmask = 1 << BITSHIFT_CRT;
    devcnt = 0;
    while (devmask & BITMASK_ALL_CRT) {
        if (devmask & mask) {
            if (first) first = NV_FALSE;
            else s += sprintf(s, ", ");
            s += sprintf(s, "CRT-%X", devcnt);
        }
        devmask <<= 1;
        devcnt++;
    }

    devmask = 1 << BITSHIFT_DFP;
    devcnt = 0;
    while (devmask & BITMASK_ALL_DFP) {
        if (devmask & mask)  {
            if (first) first = NV_FALSE;
            else s += sprintf(s, ", ");
            s += sprintf(s, "DFP-%X", devcnt);
        }
        devmask <<= 1;
        devcnt++;
    }
    
    devmask = 1 << BITSHIFT_TV;
    devcnt = 0;
    while (devmask & BITMASK_ALL_TV) {
        if (devmask & mask)  {
            if (first) first = NV_FALSE;
            else s += sprintf(s, ", ");
            s += sprintf(s, "TV-%X", devcnt);
        }
        devmask <<= 1;
        devcnt++;
    }
    
    if (mask & DISPLAY_DEVICES_WILDCARD_CRT) {
        if (first) first = NV_FALSE;
        else s += sprintf(s, ", ");
        s += sprintf(s, "CRT");
    }

    if (mask & DISPLAY_DEVICES_WILDCARD_TV) {
        if (first) first = NV_FALSE;
        else s += sprintf(s, ", ");
        s += sprintf(s, "TV");
    }

    if (mask & DISPLAY_DEVICES_WILDCARD_DFP) {
        if (first) first = NV_FALSE;
        else s += sprintf(s, ", ");
        s += sprintf(s, "DFP");
    }
    
    *s = '\0';
    
    return (display_device_name_string);

} /* display_device_mask_to_display_name() */



/*
 * nv_assign_default_display() - assign an X display, if none has been
 * assigned already.  Also, parse the display name to find any
 * specified X screen.
 */

void nv_assign_default_display(ParsedAttribute *p, const char *display)
{
    char *colon, *dot;

    if (!(p->parser_flags.has_x_display)) {
        p->display = display ? nvstrdup(display) : NULL;
        p->parser_flags.has_x_display = NV_TRUE;
    }

    if (!(p->parser_flags.has_target) && p->display) {
        colon = strchr(p->display, ':');
        if (colon) {
            dot = strchr(colon, '.');
            if (dot) {

                /*
                 * if all characters after the '.' are digits, interpret it as a
                 * screen number.
                 */
                nv_parse_special_xscreen_target(p, dot + 1, NULL);
            }
        }
    }
}



/*
 * nv_parsed_attribute_init() - initialize a ParsedAttribute linked
 * list
 */

ParsedAttribute *nv_parsed_attribute_init(void)
{
    return nvalloc(sizeof(ParsedAttribute));
}



/*
 * nv_parsed_attribute_add() - add a new parsed attribute node to the
 * linked list
 */

void nv_parsed_attribute_add(ParsedAttribute *head, ParsedAttribute *p)
{
    ParsedAttribute *t;

    for (t = head; t->next; t = t->next);

    t->next                 = nvalloc(sizeof(ParsedAttribute));

    t->display              = p->display ? nvstrdup(p->display) : NULL;
    t->target_specification = p->target_specification;
    t->target_type          = p->target_type;
    t->target_id            = p->target_id;
    t->attr_entry           = p->attr_entry;
    t->val                  = p->val;
    t->display_device_mask  = p->display_device_mask;
    t->parser_flags         = p->parser_flags;
    t->targets              = p->targets;
}



/*
 * Frees memory used by the parsed attribute members
 */

static void nv_parsed_attribute_free_members(ParsedAttribute *p)
{
    const AttributeTableEntry *a = p->attr_entry;

    nvfree(p->display);
    nvfree(p->target_specification);

    if (a &&
	((a->type == CTRL_ATTRIBUTE_TYPE_STRING) ||
         ((a->type == CTRL_ATTRIBUTE_TYPE_INTEGER) &&
          (a->f.int_flags.is_display_id)))) {
        nvfree(p->val.str);
    }

    NvCtrlTargetListFree(p->targets);
}



/*
 * nv_parsed_attribute_free() - free the linked list
 */

void nv_parsed_attribute_free(ParsedAttribute *p)
{
    ParsedAttribute *n;

    while (p) {
        n = p->next;

        nv_parsed_attribute_free_members(p);

        free(p);
        p = n;
    }
}



/*
 * nv_parsed_attribute_clean() - clean out the ParsedAttribute list,
 * so that only the empty head node remains.
 */

void nv_parsed_attribute_clean(ParsedAttribute *p)
{
    nv_parsed_attribute_free(p->next);

    nv_parsed_attribute_free_members(p);

    memset(p, 0, sizeof(*p));

} /* nv_parsed_attribute_clean() */



/*
 * nv_standardize_screen_name() - standardize the X Display name, by
 * inserting the hostname (if necessary), and using the specified
 * screen number.  If 'screen' is -1, use the screen number already in
 * the string.  If 'screen' is -2, do not put a screen number in the
 * Display name.
 */

char *nv_standardize_screen_name(const char *orig, int screen)
{
    char *display_name, *screen_name, *colon, *dot, *tmp;
    struct utsname uname_buf;
    int len;
    
    /* get the string describing this display connection */
    
    if (!orig) return NULL;
    
    /* create a working copy */
    
    display_name = strdup(orig);
    if (!display_name) return NULL;
    
    /* skip past the host */
    
    colon = strchr(display_name, ':');
    if (!colon) return NULL;
    
    /* if no host is specified, prepend the local hostname */
    
    /* XXX should we try to catch "localhost"? */

    if (display_name == colon) {
        if (uname(&uname_buf) == 0) {
            len = strlen(display_name) + strlen(uname_buf.nodename) + 1;
            tmp = nvalloc(len);
            snprintf(tmp, len, "%s%s", uname_buf.nodename, display_name);
            free(display_name);
            display_name = tmp;
            colon = strchr(display_name, ':');
            if (!colon) {
                nvfree(display_name);
                return NULL;
            }
        }
    }
    
    /*
     * if the screen parameter is -1, then extract the screen number,
     * either from the string or default to 0
     */
    
    if (screen == -1) {
        dot = strchr(colon, '.');
        if (dot) {
            screen = atoi(dot + 1);
        } else {
            screen = 0;
        }
    }
    
    /*
     * find the separation between the display and the screen; if we
     * find it, then truncate the string before the screen, so that we
     * can append the correct screen number.
     */
    
    dot = strchr(colon, '.');
    if (dot) *dot = '\0';

    /*
     * if the screen parameter is -2, then do not write out a screen
     * number.
     */
    
    if (screen == -2) {
        screen_name = display_name;
    } else {
        len = strlen(display_name) + 8;
        screen_name = nvalloc(len);
        snprintf(screen_name, len, "%s.%d", display_name, screen);
        free(display_name);
    }
    
    return (screen_name);

} /* nv_standardize_screen_name() */



/*
 * allocate an output string, and copy the input string to the output
 * string, omitting whitespace
 */

char *remove_spaces(const char *o)
{
    int len;
    char *m, *no_spaces;
   
    if (!o) return (NULL);
    
    len = strlen (o);
    
    no_spaces = nvalloc(len + 1);

    m = no_spaces;
    while (*o) {
        if (!isspace (*o)) { *m++ = *o; }
        o++;
    }
    *m = '\0';
    
    len = m - no_spaces + 1;
    no_spaces = realloc (no_spaces, len);

    return (no_spaces);

} /* remove_spaces() */



/*
 * allocate an output string and copy the input string to this
 * output string, replacing any occurrences of the character
 * 'c' with the character 'r'.
 */

char *replace_characters(const char *o, const char c, const char r)
{
    int len;
    char *m, *out;

    if (!o)
        return NULL;

    len = strlen(o);

    out = nvalloc(len + 1);
    m = out;

    while (*o != '\0') {
        *m++ = (*o == c) ? r : *o;
        o++;
    }
    *m = '\0';

    len = (m - out + 1);
    out = nvrealloc(out, len);

    return out;

} /* replace_characters() */



/**************************************************************************/



/*
 * nv_strtok () - returns a dynamically allocated array of strings,
 * which are the separate segments of the passed in string, divided by
 * the character indicated.  The passed-by-reference argument num will
 * hold the number of segments found.  When you are done with the
 * array of strings, it is best to call nvFreeStrToks () to free the
 * memory allocated here.
 */

char **nv_strtok(char *s, char c, int *n)
{
    int count, i, len;
    char **delims, **tokens, *m;
    
    count = count_number_of_chars(s, c);
    
    /*
     * allocate and set pointers to each division (each instance of the
     * dividing character, and the terminating NULL of the string)
     */
    
    delims = nvalloc((count + 1) * sizeof(char *));
    m = s;
    for (i = 0; i < count; i++) {
        while (*m != c) m++;
        delims[i] = m;
        m++;
    }
    delims[count] = (char *) strchr(s, '\0');
    
    /*
     * so now, we have pointers to each delimiter; copy what's in between
     * the divisions (the tokens) into the dynamic array of strings
     */
    
    tokens = nvalloc((count + 1) * sizeof(char *));
    len = delims[0] - s;
    tokens[0] = nvstrndup(s, len);
    
    for (i = 1; i < count+1; i++) {
        len = delims[i] - delims[i-1];
        tokens[i] = nvstrndup(delims[i-1]+1, len-1);
    }
    
    free(delims);
    
    *n = count+1;
    return (tokens);
    
} /* nv_strtok() */



/*
 * nv_free_strtoks() - free an array of arrays, such as what is
 * allocated and returned by nv_strtok()
 */

void nv_free_strtoks(char **s, int n)
{
    int i;
    for (i = 0; i < n; i++) free(s[i]);
    free(s);
    
} /* nv_free_strtoks() */



/*
 * character to integer conversion
 */

static int ctoi(const char c)
{
    return (c - '0');

} /* ctoi */



/*
 * count_number_of_chars() - return the number of times the
 * character d appears in the string
 */

static int count_number_of_chars(char *o, char d)
{
    int c = 0;
    while (*o) {
        if (*o == d) c++;
        o++;
    }
    return (c);
    
} /* count_number_of_chars() */



/*
 * count_number_of_bits() - return the number of bits set
 * in the int.
 */

int count_number_of_bits(unsigned int mask)
{
    int count = 0;

    while (mask) {
        count++;
        mask &= (mask-1);
    }

    return count;

} /* count_number_of_bits() */



/** parse_skip_whitespace() ******************************************
 *
 * Returns a pointer to the start of non-whitespace chars in string 'str'
 *
 **/
const char *parse_skip_whitespace(const char *str)
{
    while (*str &&
           (*str == ' '  || *str == '\t' ||
            *str == '\n' || *str == '\r')) {
        str++;
    }
    return str;

} /* parse_skip_whitespace() */



/** parse_chop_whitespace() ******************************************
 *
 * Removes all trailing whitespace chars from the given string 'str'
 *
 ***/
void parse_chop_whitespace(char *str)
{
    char *tmp = str + strlen(str) -1;
    
    while (tmp >= str &&
           (*tmp == ' '  || *tmp == '\t' ||
            *tmp == '\n' || *tmp == '\r')) {
        *tmp = '\0';
        tmp--;
    }

} /* parse_chop_whitespace() */



/** parse_skip_integer() *********************************************
 *
 * Returns a pointer to the location just after any integer in string 'str'
 *
 **/
const char *parse_skip_integer(const char *str)
{
    if (*str == '-' || *str == '+') {
        str++;
    }
    while (*str && *str >= '0' && *str <= '9') {
        str++;
    }
    return str;

} /* parse_skip_integer() */



/** parse_read_integer() *********************************************
 *
 * Reads an integer from string str and returns a pointer
 *
 **/
const char *parse_read_integer(const char *str, int *num)
{
    str = parse_skip_whitespace(str);
    *num = atoi(str);
    str = parse_skip_integer(str);
    return parse_skip_whitespace(str);

} /* parse_read_integer() */



/** parse_read_integer_pair() ****************************************
 *
 * Reads two integers separated by 'separator' and returns a pointer
 * to the location in 'str' where parsing finished. (After the two
 * integers).  NULL is returned on failure.
 *
 **/
const char *parse_read_integer_pair(const char *str,
                                    char separator, int *a, int *b)
 {
    str = parse_read_integer(str, a);
    if (!str) return NULL;

    if (separator) {
        if (*str != separator) return NULL;
        str++;
    }
    return parse_read_integer(str, b);

} /* parse_read_integer_pair() */



/** parse_read_name() ************************************************
 *
 * Skips whitespace and copies characters up to and excluding the
 * terminating character 'term'.  The location where parsing stopped
 * is returned, or NULL on failure.
 *
 * The 'term' value 0 is used to indicate that any whitespace should
 * be treated as a terminator.
 *
 **/

static int name_terminated(const char ch, const char term)
{
    if (term == 0) {
        return (ch == ' '  || ch == '\t' || ch == '\n' || ch == '\r');
    } else {
        return (ch == term);
    }
}

const char *parse_read_name(const char *str, char **name, char term)
{
    const char *tmp;

    str = parse_skip_whitespace(str);
    tmp = str;
    while (*str && !name_terminated(*str, term)) {
        str++;
    }

    *name = nvalloc(str - tmp + 1);
    strncpy(*name, tmp, str -tmp);
    if (name_terminated(*str, term)) {
        str++;
    }
    return parse_skip_whitespace(str);

} /* parse_read_name() */



/** parse_read_display_name() ****************************************
 *
 * Convert a 'CRT-1' style display device name into a device_mask
 * '0x00000002' bitmask.  The location where parsing stopped is returned
 * or NULL if an error occurred.
 *
 **/
const char *parse_read_display_name(const char *str, unsigned int *mask)
{
    if (!str || !mask) {
        return NULL;
    }

    str = parse_skip_whitespace(str);
    if (!strncmp(str, "CRT-", 4)) {
        *mask = 1 << (atoi(str+4));

    } else if (!strncmp(str, "TV-", 3)) {
        *mask = (1 << (atoi(str+3))) << 8;

    } else if (!strncmp(str, "DFP-", 4)) {
        *mask = (1 << (atoi(str+4))) << 16;

    } else {
        return NULL;
    }

    while (*str && *str != ':') {
        str++;
    }
    if (*str == ':') {
        str++;
    }

    return parse_skip_whitespace(str);

} /* parse_read_display_name() */



/** parse_read_display_id() *****************************************
 *
 * Convert a 'DPY-#' style display device name into a display device
 * id.  The location where parsing stopped is returned or NULL if an
 * error occurred.
 *
 **/
const char *parse_read_display_id(const char *str, unsigned int *id)
{
    if (!str || !id) {
        return NULL;
    }

    str = parse_skip_whitespace(str);
    if (!strncmp(str, "DPY-", 4)) {
        *id = atoi(str+4);
    } else {
        return NULL;
    }

    while (*str && *str != ':') {
        str++;
    }
    if (*str == ':') {
        str++;
    }

    return parse_skip_whitespace(str);

} /* parse_read_display_id() */



/** parse_read_float_range() *****************************************
 *
 * Reads the maximum/minimum information from a string in the
 * following format:
 *     "MIN-MAX"
 * or
 *     "MIN"
 *
 **/
int parse_read_float_range(const char *str, float *min, float *max)
{
    if (!str) return 0;

    str = parse_skip_whitespace(str);
    *min = atof(str);
    str = strstr(str, "-");
    if (!str) {
        *max = *min;
        return 1;
    }
    str++;
    *max = atof(str);
    
    return 1;

} /* parse_read_float_range() */



/** parse_token_value_pairs() ****************************************
 *
 * Parses the given string for "token=value, token=value, ..." pairs
 * and dispatches the handling of tokens to the given function with
 * the given data as an extra argument.
 *
 * Note that the value may be in parentheses:  "token=(value), ..."
 *
 **/
int parse_token_value_pairs(const char *str, apply_token_func func,
                            void *data)
{
    char *token;
    char *value;
    char endChar;


    if (str) {

        /* Parse each token */
        while (*str) {

            /* Read the token */
            str = parse_read_name(str, &token, '=');
            if (!str) return 0;

            /* Read the value */
            if (str && *str == '(') {
                str++;
                endChar = ')';
            } else {
                endChar = ',';
            }
            str = parse_read_name(str, &value, endChar);
            if (!str) return 0;
            if (endChar == ')' && *str == ')') {
                str++;
            }
            if (*str == ',') {
                str++;
            }

            /* Remove trailing whitespace */
            parse_chop_whitespace(token);
            parse_chop_whitespace(value);

            func(token, value, data);

            free(token);
            free(value);
        }
    }

    return 1;

} /* parse_token_value_pairs() */
