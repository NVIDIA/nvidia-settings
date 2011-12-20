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

/* local helper functions */

static int nv_parse_display_and_target(char *start, char *end,
                                       ParsedAttribute *a);
static char **nv_strtok(char *s, char c, int *n);
static void nv_free_strtoks(char **s, int n);
static int ctoi(const char c);
static int count_number_of_chars(char *o, char d);
static char *nv_strndup(char *s, int n);

/*
 * Table of all attribute names recognized by the attribute string
 * parser.  Binds attribute names to attribute integers (for use in
 * the NvControl protocol).  The flags describe qualities of each
 * attribute.
 */

#define F NV_PARSER_TYPE_FRAMELOCK
#define C NV_PARSER_TYPE_COLOR_ATTRIBUTE
#define N NV_PARSER_TYPE_NO_CONFIG_WRITE
#define G NV_PARSER_TYPE_GUI_ATTRIBUTE
#define V NV_PARSER_TYPE_XVIDEO_ATTRIBUTE
#define P NV_PARSER_TYPE_PACKED_ATTRIBUTE
#define D NV_PARSER_TYPE_VALUE_IS_DISPLAY
#define A NV_PARSER_TYPE_NO_QUERY_ALL
#define Z NV_PARSER_TYPE_NO_ZERO_VALUE
#define H NV_PARSER_TYPE_100Hz
#define K NV_PARSER_TYPE_1000Hz
#define S NV_PARSER_TYPE_STRING_ATTRIBUTE
#define I NV_PARSER_TYPE_SDI
#define W NV_PARSER_TYPE_VALUE_IS_SWITCH_DISPLAY
#define M NV_PARSER_TYPE_SDI_CSC
#define T NV_PARSER_TYPE_HIJACK_DISPLAY_DEVICE

AttributeTableEntry attributeTable[] = {
   
    /* name                    constant                             flags                 description */

    /* Version information */
    { "OperatingSystem",     NV_CTRL_OPERATING_SYSTEM,             N,   "The operating system on which the X server is running.  0-Linux, 1-FreeBSD, 2-SunOS." },
    { "NvidiaDriverVersion", NV_CTRL_STRING_NVIDIA_DRIVER_VERSION, S|N, "The NVIDIA X driver version." },
    { "NvControlVersion",    NV_CTRL_STRING_NV_CONTROL_VERSION,    S|N, "The NV-CONTROL X driver extension version." },
    { "GLXServerVersion",    NV_CTRL_STRING_GLX_SERVER_VERSION,    S|N, "The GLX X server extension version." },
    { "GLXClientVersion",    NV_CTRL_STRING_GLX_CLIENT_VERSION,    S|N, "The GLX client version." },
    { "OpenGLVersion",       NV_CTRL_STRING_GLX_OPENGL_VERSION,    S|N, "The OpenGL version." },
    { "XRandRVersion",       NV_CTRL_STRING_XRANDR_VERSION,        S|N, "The X RandR version." },
    { "XF86VidModeVersion",  NV_CTRL_STRING_XF86VIDMODE_VERSION,   S|N, "The XF86 Video Mode X extension version." },
    { "XvVersion",           NV_CTRL_STRING_XV_VERSION,            S|N, "The Xv X extension version." },
 
    /* X screen */
    { "Ubb",                           NV_CTRL_UBB,                               0,     "Is UBB enabled for the specified X screen." },
    { "Overlay",                       NV_CTRL_OVERLAY,                           0,     "Is the RGB overlay enabled for the specified X screen." },
    { "Stereo",                        NV_CTRL_STEREO,                            0,     "The stereo mode for the specified X screen." },
    { "TwinView",                      NV_CTRL_TWINVIEW,                          0,     "Is TwinView enabled for the specified X screen." },
    { "ConnectedDisplays",             NV_CTRL_CONNECTED_DISPLAYS,                D,     "Display mask indicating the last cached state of the display devices connected to the GPU." },
    { "EnabledDisplays",               NV_CTRL_ENABLED_DISPLAYS,                  D,     "Display mask indicating what display devices are enabled for use on the specified X screen or GPU." },
    { "CursorShadow",                  NV_CTRL_CURSOR_SHADOW,                     0,     "Hardware cursor shadow." },
    { "CursorShadowAlpha",             NV_CTRL_CURSOR_SHADOW_ALPHA,               0,     "Hardware cursor shadow alpha (transparency) value." },
    { "CursorShadowRed",               NV_CTRL_CURSOR_SHADOW_RED,                 0,     "Hardware cursor shadow red color." },
    { "CursorShadowGreen",             NV_CTRL_CURSOR_SHADOW_GREEN,               0,     "Hardware cursor shadow green color." },
    { "CursorShadowBlue",              NV_CTRL_CURSOR_SHADOW_BLUE,                0,     "Hardware cursor shadow blue color." },
    { "CursorShadowXOffset",           NV_CTRL_CURSOR_SHADOW_X_OFFSET,            0,     "Hardware cursor shadow X offset." },
    { "CursorShadowYOffset",           NV_CTRL_CURSOR_SHADOW_Y_OFFSET,            0,     "Hardware cursor shadow Y offset." },
    { "AssociatedDisplays",            NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,        N|D,   "Display device mask indicating which display devices are \"associated\" with the specified X screen (i.e., are available for displaying the desktop)." },
    { "ProbeDisplays",                 NV_CTRL_PROBE_DISPLAYS,                    A,     "When this attribute is queried, the X driver re-probes the hardware to detect which display devices are connected to the GPU or DPU driving the specified X screen.  Returns a display mask of the currently connected display devices." },
    { "InitialPixmapPlacement",        NV_CTRL_INITIAL_PIXMAP_PLACEMENT,          N,     "Controls where X pixmaps are initially created." },
    { "DynamicTwinview",               NV_CTRL_DYNAMIC_TWINVIEW,                  N,     "Does the X screen support dynamic TwinView." },
    { "MultiGpuDisplayOwner",          NV_CTRL_MULTIGPU_DISPLAY_OWNER,            N,     "GPU ID of the GPU that has the display device(s) used for showing the X screen." },
    { "HWOverlay",                     NV_CTRL_HWOVERLAY,                         0,     "When a workstation overlay is in use, this value is 1 if the hardware overlay is used, or 0 if the overlay is emulated." },
    { "OnDemandVBlankInterrupts",      NV_CTRL_ONDEMAND_VBLANK_INTERRUPTS,        0,     "Enable/Disable/Query of on-demand vertical blanking interrupt control on the GPU.  The 'OnDemandVBlankInterrupts' X server configuration option must be enabled for this option to be available." },
    { "GlyphCache",                    NV_CTRL_GLYPH_CACHE,                       N,     "Enable or disable caching of glyphs (text) in video memory." },
    { "SwitchToDisplays",              NV_CTRL_SWITCH_TO_DISPLAYS,                D|N|W, "Used to set which displays should be active." },
    { "NotebookDisplayChangeLidEvent", NV_CTRL_NOTEBOOK_DISPLAY_CHANGE_LID_EVENT, N,     "Reports notebook lid open/close events." },
    { "NotebookInternalLCD",           NV_CTRL_NOTEBOOK_INTERNAL_LCD,             N|D,   "Returns the display device mask of the internal LCD of a notebook." },
    { "Depth30Allowed",                NV_CTRL_DEPTH_30_ALLOWED,                  N,     "Returns whether the NVIDIA X driver supports depth 30 on the specified X screen or GPU." },
    { "NoScanout",                     NV_CTRL_NO_SCANOUT,                        N,     "Returns whether the special \"NoScanout\" mode is enabled on the specified X screen or GPU." },
    { "XServerUniqueId",               NV_CTRL_X_SERVER_UNIQUE_ID,                N,     "Returns a pseudo-unique identification number for the X server." },
    { "PixmapCache",                   NV_CTRL_PIXMAP_CACHE,                      N,     "Controls whether pixmaps are allocated in a cache." },
    { "PixmapCacheRoundSizeKB",        NV_CTRL_PIXMAP_CACHE_ROUNDING_SIZE_KB,     N,     "Controls the number of kilobytes to add to the pixmap cache when there is not enough room." },
    { "AccelerateTrapezoids",          NV_CTRL_ACCELERATE_TRAPEZOIDS,             N,     "Enable or disable GPU acceleration of RENDER Trapezoids." },

    /* OpenGL */
    { "SyncToVBlank",               NV_CTRL_SYNC_TO_VBLANK,                   0,   "Enables sync to vertical blanking for OpenGL clients.  This setting only takes effect on OpenGL clients started after it is set." },
    { "LogAniso",                   NV_CTRL_LOG_ANISO,                        0,   "Enables anisotropic filtering for OpenGL clients; on some NVIDIA hardware, this can only be enabled or disabled; on other hardware different levels of anisotropic filtering can be specified.  This setting only takes effect on OpenGL clients started after it is set." },
    { "FSAA",                       NV_CTRL_FSAA_MODE,                        0,   "The full screen antialiasing setting for OpenGL clients.  This setting only takes effect on OpenGL clients started after it is set." },
    { "TextureSharpen",             NV_CTRL_TEXTURE_SHARPEN,                  0,   "Enables texture sharpening for OpenGL clients.  This setting only takes effect on OpenGL clients started after it is set." },
    { "ForceGenericCpu",            NV_CTRL_FORCE_GENERIC_CPU,                N,   "Inhibit the use of CPU-specific features such as MMX, SSE, or 3DNOW! for OpenGL clients; this option may result in performance loss, but may be useful in conjunction with software such as the Valgrind memory debugger.  This setting only takes effect on OpenGL clients started after it is set." },
    { "GammaCorrectedAALines",      NV_CTRL_OPENGL_AA_LINE_GAMMA,             0,   "For OpenGL clients, allow gamma-corrected antialiased lines to consider variances in the color display capabilities of output devices when rendering smooth lines.  Only available on recent Quadro GPUs.  This setting only takes effect on OpenGL clients started after it is set." },

    { "AllowFlipping",              NV_CTRL_FLIPPING_ALLOWED,                 0,   "Defines the swap behavior of OpenGL.  When 1, OpenGL will swap by flipping when possible;  When 0, OpenGL will always swap by blitting." },
    { "FSAAAppControlled",          NV_CTRL_FSAA_APPLICATION_CONTROLLED,      0,   "When Application Control for FSAA is enabled, then what the application requests is used, and the FSAA attribute is ignored.  If this is disabled, then any application setting is overridden with the FSAA attribute." },
    { "LogAnisoAppControlled",      NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED, 0,   "When Application Control for LogAniso is enabled, then what the application requests is used, and the LogAniso attribute is ignored.  If this is disabled, then any application setting is overridden with the LogAniso attribute." },
    { "ForceStereoFlipping",        NV_CTRL_FORCE_STEREO,                     0,   "When 1, OpenGL will force stereo flipping even when no stereo drawables are visible (if the device is configured to support it, see the \"Stereo\" X config option).  When 0, fall back to the default behavior of only flipping when a stereo drawable is visible." },
    { "OpenGLImageSettings",        NV_CTRL_IMAGE_SETTINGS,                   0,   "The image quality setting for OpenGL clients.  This setting only takes effect on OpenGL clients started after it is set." },
    { "XineramaStereoFlipping",     NV_CTRL_XINERAMA_STEREO,                  0,   "When 1, OpenGL will allow stereo flipping on multiple X screens configured with Xinerama.  When 0, flipping is allowed only on one X screen at a time." },
    { "ShowSLIHUD",                 NV_CTRL_SHOW_SLI_HUD,                     0,   "If this is enabled (1), the driver will draw information about the current SLI mode into a \"heads-up display\" inside OpenGL windows accelerated with SLI.  This setting only takes effect on OpenGL clients started after it is set." },
    { "ShowSLIVisualIndicator",     NV_CTRL_SHOW_SLI_VISUAL_INDICATOR,        0,   "If this is enabled (1), the driver will draw information about the current SLI mode into a \"visual indicator\" inside OpenGL windows accelerated with SLI.  This setting only takes effect on OpenGL clients started after it is set." },
    { "ShowMultiGpuVisualIndicator", NV_CTRL_SHOW_MULTIGPU_VISUAL_INDICATOR,  0,   "If this is enabled (1), the driver will draw information about the current MultiGPU mode into a \"visual indicator\" inside OpenGL windows accelerated with SLI.  This setting only takes effect on OpenGL clients started after it is set." },
    { "FSAAAppEnhanced",            NV_CTRL_FSAA_APPLICATION_ENHANCED,        0,   "Controls how the FSAA attribute is applied when FSAAAppControlled is disabled.  When FSAAAppEnhanced is disabled, OpenGL applications will be forced to use the FSAA mode specified by the FSAA attribute.  When the FSAAAppEnhanced attribute is enabled, only those applications that have selected a multisample FBConfig will be made to use the FSAA mode specified." },
    { "GammaCorrectedAALinesValue", NV_CTRL_OPENGL_AA_LINE_GAMMA_VALUE,       0,   "Returns the gamma value used by OpenGL when gamma-corrected antialiased lines are enabled." },
    { "StereoEyesExchange",         NV_CTRL_STEREO_EYES_EXCHANGE,             0,   "Swaps the left and right eyes of stereo images." },
    { "SLIMode",                    NV_CTRL_STRING_SLI_MODE,                  S|N, "Returns a string describing the current SLI mode, if any." },
    { "SliMosaicModeAvailable",     NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE,        N,   "Returns whether or not SLI Mosaic Mode is supported." },

    /* GPU */
    { "BusType",                NV_CTRL_BUS_TYPE,                      0,   "Returns the type of bus connecting the specified device to the computer.  If the target is an X screen, then it uses the GPU driving the X screen as the device." },
    { "PCIEMaxLinkSpeed",       NV_CTRL_GPU_PCIE_MAX_LINK_SPEED,       0,   "Returns the maximum PCI-E link speed" },
    { "VideoRam",               NV_CTRL_VIDEO_RAM,                     0,   "Returns the total amount of memory available to the specified GPU (or the GPU driving the specified X screen).  Note: if the GPU supports TurboCache(TM), the value reported may exceed the amount of video memory installed on the GPU.  The value reported for integrated GPUs may likewise exceed the amount of dedicated system memory set aside by the system BIOS for use by the integrated GPU." },
    { "Irq",                    NV_CTRL_IRQ,                           0,   "Returns the interrupt request line used by the specified device.  If the target is an X screen, then it uses the GPU driving the X screen as the device." },
    { "CUDACores",              NV_CTRL_GPU_CORES,                     N,   "Returns number of CUDA cores supported by the graphics pipeline." },
    { "GPUMemoryInterface",     NV_CTRL_GPU_MEMORY_BUS_WIDTH,          N,   "Returns bus bandwidth of the GPU's memory interface." },
    { "GPUCoreTemp",            NV_CTRL_GPU_CORE_TEMPERATURE,          N,   "Reports the current core temperature in Celsius of the GPU driving the X screen." },
    { "GPUAmbientTemp",         NV_CTRL_AMBIENT_TEMPERATURE,           N,   "Reports the current temperature in Celsius of the immediate neighborhood of the GPU driving the X screen." },
    { "GPUOverclockingState",   NV_CTRL_GPU_OVERCLOCKING_STATE,        N,   "The current overclocking state; the value of this attribute controls the availability of additional overclocking attributes.  Note that this attribute is unavailable unless overclocking support has been enabled by the system administrator." },
    { "GPU2DClockFreqs",        NV_CTRL_GPU_2D_CLOCK_FREQS,            N|P, "The GPU and memory clock frequencies when operating in 2D mode.  New clock frequencies are tested before being applied, and may be rejected.  Note that if the target clocks are too aggressive, their testing may render the system unresponsive.  Also note that while this attribute may always be queried, it cannot be set unless GPUOverclockingState is set to MANUAL.  Since the target clocks may be rejected, the requester should read this attribute after the set to determine success or failure." },
    { "GPU3DClockFreqs",        NV_CTRL_GPU_3D_CLOCK_FREQS,            N|P, "The GPU and memory clock frequencies  when operating in 3D mode.  New clock frequencies are tested before being applied, and may be rejected.  Note that if the target clocks are too aggressive, their testing may render the system unresponsive.  Also note that while this attribute may always be queried, it cannot be set unless GPUOverclockingState is set to MANUAL.  Since the target clocks may be rejected, the requester should read this attribute after the set to determine success or failure." },
    { "GPUDefault2DClockFreqs", NV_CTRL_GPU_DEFAULT_2D_CLOCK_FREQS,    N|P, "Returns the default memory and GPU core clocks when operating in 2D mode." },
    { "GPUDefault3DClockFreqs", NV_CTRL_GPU_DEFAULT_3D_CLOCK_FREQS,    N|P, "Returns the default memory and GPU core clocks when operating in 3D mode." },
    { "GPUCurrentClockFreqs",   NV_CTRL_GPU_CURRENT_CLOCK_FREQS,       N|P, "Returns the current GPU and memory clocks of the graphics device driving the X screen." },
    { "GPUCurrentProcessorClockFreqs", NV_CTRL_GPU_CURRENT_PROCESSOR_CLOCK_FREQS, N, "Returns the current processor clock of the graphics device driving the X screen." },
    { "GPUCurrentClockFreqsString", NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS, S|N, "Returns the current GPU, memory and Processor clocks of the graphics device driving the X screen." },
    { "BusRate",                NV_CTRL_BUS_RATE,                      0,   "If the device is on an AGP bus, then BusRate returns the configured AGP rate.  If the device is on a PCI Express bus, then this attribute returns the width of the physical link." },
    { "PCIDomain",              NV_CTRL_PCI_DOMAIN,                    N,   "Returns the PCI domain number for the specified device." },
    { "PCIBus",                 NV_CTRL_PCI_BUS,                       N,   "Returns the PCI bus number for the specified device." },
    { "PCIDevice",              NV_CTRL_PCI_DEVICE,                    N,   "Returns the PCI device number for the specified device." },
    { "PCIFunc",                NV_CTRL_PCI_FUNCTION,                  N,   "Returns the PCI function number for the specified device." },
    { "PCIID",                  NV_CTRL_PCI_ID,                        N|P, "Returns the PCI vendor and device ID of the specified device." },
    { "PCIEGen",                NV_CTRL_GPU_PCIE_GENERATION,           N,   "Returns the current PCI-E Bus Generation." },
    { "GPUErrors",              NV_CTRL_NUM_GPU_ERRORS_RECOVERED,      N,   "Returns the number of GPU errors occurred." },
    { "GPUPowerSource",         NV_CTRL_GPU_POWER_SOURCE,              N,   "Reports the type of power source of the GPU." },
    { "GPUCurrentPerfMode",     NV_CTRL_GPU_CURRENT_PERFORMANCE_MODE,  N,   "Reports the current performance mode of the GPU driving the X screen.  Running a 3D app, for example, will change this performance mode if Adaptive Clocking is enabled." },
    { "GPUCurrentPerfLevel",    NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL, N,   "Reports the current Performance level of the GPU driving the X screen.  Each Performance level has associated NVClock and Mem Clock values." },
    { "GPUAdaptiveClockState",  NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE,      N,   "Reports if Adaptive Clocking is Enabled on the GPU driving the X screen." },
    { "GPUPerfModes",           NV_CTRL_STRING_PERFORMANCE_MODES,      S|N, "Returns a string with all the performance modes defined for this GPU along with their associated NV Clock and Memory Clock values." },
    { "GPUPowerMizerMode",      NV_CTRL_GPU_POWER_MIZER_MODE,          0,   "Allows setting different GPU powermizer modes." },
    { "ECCSupported",           NV_CTRL_GPU_ECC_SUPPORTED,             N,   "Reports whether the underlying GPU supports ECC.  All of the other ECC attributes are only applicable if this attribute indicates that ECC is supported." },
    { "ECCStatus",              NV_CTRL_GPU_ECC_STATUS,                N,   "Reports whether ECC is enabled." },
    { "ECCConfigurationSupported", NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED, N,   "Reports whether ECC whether the ECC configuration setting can be changed." },
    { "ECCConfiguration",            NV_CTRL_GPU_ECC_CONFIGURATION,               N, "Returns the current ECC configuration setting." },
    { "ECCDefaultConfiguration",     NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION,       N, "Returns the default ECC configuration setting." },
    { "ECCDoubleBitErrors",          NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS,           N, "Returns the number of double-bit ECC errors detected by the targeted GPU since the last POST." },
    { "ECCAggregateDoubleBitErrors", NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS, N, "Returns the number of double-bit ECC errors detected by the targeted GPU since the last counter reset." },
    { "GPUFanControlState",     NV_CTRL_GPU_COOLER_MANUAL_CONTROL,        N,   "The current fan control state; the value of this attribute controls the availability of additional fan control attributes.  Note that this attribute is unavailable unless fan control support has been enabled by setting the \"Coolbits\" X config option." },
    { "GPUCurrentFanSpeed",     NV_CTRL_THERMAL_COOLER_LEVEL,             N,   "Returns the GPU fan's current speed." },
    { "GPUResetFanSpeed",       NV_CTRL_THERMAL_COOLER_LEVEL_SET_DEFAULT, N,   "Resets the GPU fan's speed to its default." },
    { "GPUFanControlType",      NV_CTRL_THERMAL_COOLER_CONTROL_TYPE,      N,   "Returns how the GPU fan is controlled.  '1' means the fan can only be toggled on and off; '2' means the fan has variable speed.  '0' means the fan is restricted and cannot be adjusted under end user control." },
    { "GPUFanTarget",           NV_CTRL_THERMAL_COOLER_TARGET,            N,   "Returns the objects the fan cools.  '1' means the GPU, '2' means video memory, '4' means the power supply, and '7' means all of the above." },
    { "ThermalSensorReading",   NV_CTRL_THERMAL_SENSOR_READING,           N,   "Returns the thermal sensor's current reading." },
    { "ThermalSensorProvider",  NV_CTRL_THERMAL_SENSOR_PROVIDER,          N,   "Returns the hardware device that provides the thermal sensor." },
    { "ThermalSensorTarget",    NV_CTRL_THERMAL_SENSOR_TARGET,            N,   "Returns what hardware component the thermal sensor is measuring." },  
    /* Framelock */
    { "FrameLockAvailable",    NV_CTRL_FRAMELOCK,                   N|F|G,   "Returns whether the underlying GPU supports Frame Lock.  All of the other frame lock attributes are only applicable if this attribute is enabled (Supported)." },
    { "FrameLockMaster",       NV_CTRL_FRAMELOCK_MASTER,            N|F|G|D, "Get/set which display device to use as the frame lock master for the entire sync group.  Note that only one node in the sync group should be configured as the master." },
    { "FrameLockPolarity",     NV_CTRL_FRAMELOCK_POLARITY,          N|F|G,   "Sync to the rising edge of the Frame Lock pulse, the falling edge of the Frame Lock pulse, or both." },
    { "FrameLockSyncDelay",    NV_CTRL_FRAMELOCK_SYNC_DELAY,        N|F|G,   "Returns the delay between the frame lock pulse and the GPU sync.  This is an 11 bit value which is multiplied by 7.81 to determine the sync delay in microseconds." },
    { "FrameLockSyncInterval", NV_CTRL_FRAMELOCK_SYNC_INTERVAL,     N|F|G,   "This defines the number of house sync pulses for each Frame Lock sync period.  This only applies to the server, and only when recieving house sync.  A value of zero means every house sync pulse is one frame period." },
    { "FrameLockPort0Status",  NV_CTRL_FRAMELOCK_PORT0_STATUS,      N|F|G,   "Input/Output status of the RJ45 port0." },
    { "FrameLockPort1Status",  NV_CTRL_FRAMELOCK_PORT1_STATUS,      N|F|G,   "Input/Output status of the RJ45 port1." },
    { "FrameLockHouseStatus",  NV_CTRL_FRAMELOCK_HOUSE_STATUS,      N|F|G,   "Returns whether or not the house sync signal was detected on the BNC connector of the frame lock board." },
    { "FrameLockEnable",       NV_CTRL_FRAMELOCK_SYNC,              N|F|G,   "Enable/disable the syncing of display devices to the frame lock pulse as specified by previous calls to FrameLockMaster and FrameLockSlaves." },
    { "FrameLockSyncReady",    NV_CTRL_FRAMELOCK_SYNC_READY,        N|F|G,   "Reports whether a slave frame lock board is receiving sync, whether or not any display devices are using the signal." },
    { "FrameLockStereoSync",   NV_CTRL_FRAMELOCK_STEREO_SYNC,       N|F|G,   "This indicates that the GPU stereo signal is in sync with the frame lock stereo signal." },
    { "FrameLockTestSignal",   NV_CTRL_FRAMELOCK_TEST_SIGNAL,       N|F|G,   "To test the connections in the sync group, tell the master to enable a test signal, then query port[01] status and sync_ready on all slaves.  When done, tell the master to disable the test signal.  Test signal should only be manipulated while FrameLockEnable is enabled.  The FrameLockTestSignal is also used to reset the Universal Frame Count (as returned by the glXQueryFrameCountNV() function in the GLX_NV_swap_group extension).  Note: for best accuracy of the Universal Frame Count, it is recommended to toggle the FrameLockTestSignal on and off after enabling frame lock." },
    { "FrameLockEthDetected",  NV_CTRL_FRAMELOCK_ETHERNET_DETECTED, N|F|G,   "The frame lock boards are cabled together using regular cat5 cable, connecting to RJ45 ports on the backplane of the card.  There is some concern that users may think these are Ethernet ports and connect them to a router/hub/etc.  The hardware can detect this and will shut off to prevent damage (either to itself or to the router).  FrameLockEthDetected may be called to find out if Ethernet is connected to one of the RJ45 ports.  An appropriate error message should then be displayed." },
    { "FrameLockVideoMode",    NV_CTRL_FRAMELOCK_VIDEO_MODE,        N|F|G,   "Get/set what video mode is used to interpret the house sync signal.  This should only be set on the master." },
    { "FrameLockSyncRate",     NV_CTRL_FRAMELOCK_SYNC_RATE,         N|F|G,   "Returns the refresh rate that the frame lock board is sending to the GPU, in mHz (Millihertz) (i.e., to get the refresh rate in Hz, divide the returned value by 1000)." },
    { "FrameLockTiming",       NV_CTRL_FRAMELOCK_TIMING,            N|F|G,   "This is 1 when the GPU is both receiving and locked to an input timing signal.  Timing information may come from the following places: another frame lock device that is set to master, the house sync signal, or the GPU's internal timing from a display device." },
    { "FramelockUseHouseSync", NV_CTRL_USE_HOUSE_SYNC,              N|F|G,   "When 1, the server (master) frame lock device will propagate the incoming house sync signal as the outgoing frame lock sync signal.  If the frame lock device cannot detect a frame lock sync signal, it will default to using the internal timings from the GPU connected to the primary connector." },
    { "FrameLockSlaves",       NV_CTRL_FRAMELOCK_SLAVES,            N|F|G|D, "Get/set whether the display device(s) given should listen or ignore the master's sync signal." },
    { "FrameLockMasterable",   NV_CTRL_FRAMELOCK_MASTERABLE,        N|F|G|D, "Returns whether the display device(s) can be set as the master of the frame lock group.  Returns a bitmask indicating which of the given display devices can be set as a frame lock master." },
    { "FrameLockSlaveable",    NV_CTRL_FRAMELOCK_SLAVEABLE,         N|F|G|D, "Returns whether the display device(s) can be set as slave(s) of the frame lock group." },
    { "FrameLockFPGARevision", NV_CTRL_FRAMELOCK_FPGA_REVISION,     N|F|G,   "Returns the FPGA revision of the Frame Lock device." },
    { "FrameLockSyncRate4",    NV_CTRL_FRAMELOCK_SYNC_RATE_4,       N|F|G,   "Returns the refresh rate that the frame lock board is sending to the GPU in 1/10000 Hz (i.e., to get the refresh rate in Hz, divide the returned value by 10000)." },
    { "FrameLockSyncDelayResolution", NV_CTRL_FRAMELOCK_SYNC_DELAY_RESOLUTION, N|F|G, "Returns the number of nanoseconds that one unit of FrameLockSyncDelay corresponds to." },

    /* GVO */
    { "GvoSupported",                    NV_CTRL_GVO_SUPPORTED,                        I|N,   "Returns whether this X screen supports GVO; if this screen does not support GVO output, then all other GVO attributes are unavailable." },
    { "GvoSyncMode",                     NV_CTRL_GVO_SYNC_MODE,                        I,     "Selects the GVO sync mode; possible values are: FREE_RUNNING - GVO does not sync to any external signal.  GENLOCK - the GVO output is genlocked to an incoming sync signal; genlocking locks at hsync.  This requires that the output video format exactly match the incoming sync video format.  FRAMELOCK - the GVO output is frame locked to an incoming sync signal; frame locking locks at vsync.  This requires that the output video format have the same refresh rate as the incoming sync video format." },
    { "GvoSyncSource",                   NV_CTRL_GVO_SYNC_SOURCE,                      I,     "If the GVO sync mode is set to either GENLOCK or FRAMELOCK, this controls which sync source is used as the incoming sync signal (either Composite or SDI).  If the GVO sync mode is FREE_RUNNING, this attribute has no effect." },
    { "GvioRequestedVideoFormat",        NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT,          I,     "Specifies the requested output video format for a GVO device, or the requested capture format for a GVI device." },
    { "GvoOutputVideoFormat",            NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT,          I|A,   "DEPRECATED: use \"GvioRequestedVideoFormat\" instead." },
    { "GviSyncOutputFormat",             NV_CTRL_GVI_SYNC_OUTPUT_FORMAT,               I|N,   "Returns the output sync signal from the GVI device." },
    { "GvioDetectedVideoFormat",         NV_CTRL_GVIO_DETECTED_VIDEO_FORMAT,           I|N,   "Returns the input video format detected by the GVO or GVI device.  For GVI devices, the jack+channel must be passed through via the display mask param where the jack number is in the lower 16 bits and the channel number is in the upper 16 bits." },
    { "GvoInputVideoFormat",             NV_CTRL_GVIO_DETECTED_VIDEO_FORMAT,           I|N|A, "DEPRECATED: use \"GvioDetectedVideoFormat\" instead." },
    { "GvoDataFormat",                   NV_CTRL_GVO_DATA_FORMAT,                      I,     "Configures how the data in the source (either the X screen or the GLX pbuffer) is interpreted and displayed by the GVO device." },
    { "GvoDisplayXScreen",               NV_CTRL_GVO_DISPLAY_X_SCREEN,                 I|N,   "Enable/disable GVO output of the X screen (in Clone mode)." },
    { "GvoCompositeSyncInputDetected",   NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED,    I|N,   "Indicates whether Composite Sync input is detected." },
    { "GvoCompositeSyncInputDetectMode", NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE, I|N,   "Get/set the Composite Sync input detect mode." },
    { "GvoSdiSyncInputDetected",         NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED,          I|N,   "Indicates whether SDI Sync input is detected, and what type." },
    { "GvoVideoOutputs",                 NV_CTRL_GVO_VIDEO_OUTPUTS,                    I|N,   "Indicates which GVO video output connectors are currently transmitting data." },
    { "GvoSyncDelayPixels",              NV_CTRL_GVO_SYNC_DELAY_PIXELS,                I,     "Controls the skew between the input sync and the output sync in numbers of pixels from hsync; this is a 12-bit value.  If the GVO Capabilities has the Advanced Sync Skew bit set, then setting this value will set a sync advance instead of a delay." },
    { "GvoSyncDelayLines",               NV_CTRL_GVO_SYNC_DELAY_LINES,                 I,     "Controls the skew between the input sync and the output sync in numbers of lines from vsync; this is a 12-bit value.  If the GVO Capabilities has the Advanced Sync Skew bit set, then setting this value will set a sync advance instead of a delay." },
    { "GvoInputVideoFormatReacquire",    NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE,     I|N,   "Forces input detection to reacquire the input format." },
    { "GvoGlxLocked",                    NV_CTRL_GVO_GLX_LOCKED,                       I|N,   "Indicates that GVO configuration is locked by GLX;  this occurs when the GLX_NV_video_out function calls glXGetVideoDeviceNV().  All GVO output resources are locked until either glXReleaseVideoDeviceNV() is called or the X Display used when calling glXGetVideoDeviceNV() is closed." },
    { "GvoXScreenPanX",                  NV_CTRL_GVO_X_SCREEN_PAN_X,                   I,     "When GVO output of the X screen is enabled, the pan x/y attributes control which portion of the X screen is displayed by GVO.  These attributes can be updated while GVO output is enabled, or before enabling GVO output.  The pan values will be clamped so that GVO output is not panned beyond the end of the X screen." },
    { "GvoXScreenPanY",                  NV_CTRL_GVO_X_SCREEN_PAN_Y,                   I,     "When GVO output of the X screen is enabled, the pan x/y attributes control which portion of the X screen is displayed by GVO.  These attributes can be updated while GVO output is enabled, or before enabling GVO output.  The pan values will be clamped so that GVO output is not panned beyond the end of the X screen." },
    { "GvoOverrideHwCsc",                NV_CTRL_GVO_OVERRIDE_HW_CSC,                  I,     "Override the SDI hardware's Color Space Conversion with the values controlled through XNVCTRLSetGvoColorConversion() and XNVCTRLGetGvoColorConversion()." },
    { "GvoCapabilities",                 NV_CTRL_GVO_CAPABILITIES,                     I|N,   "Returns a description of the GVO capabilities that differ between NVIDIA SDI products.  This value is a bitmask where each bit indicates whether that capability is available." },
    { "GvoCompositeTermination",         NV_CTRL_GVO_COMPOSITE_TERMINATION,            I,     "Enable or disable 75 ohm termination of the SDI composite input signal." },
    { "GvoFlipQueueSize",                NV_CTRL_GVO_FLIP_QUEUE_SIZE,                  I,     "Sets/Returns the GVO flip queue size.  This value is used by the GLX_NV_video_out extension to determine the size of the internal flip queue when pbuffers are sent to the video device (via glXSendPbufferToVideoNV()).  This attribute is applied to GLX when glXGetVideoDeviceNV() is called by the application." },
    { "GvoLockOwner",                    NV_CTRL_GVO_LOCK_OWNER,                       I|N,   "Indicates that the GVO device is available or in use (by GLX, Clone Mode, or TwinView)." },
    { "GvoOutputVideoLocked",            NV_CTRL_GVO_OUTPUT_VIDEO_LOCKED,              I|N,   "Returns whether or not the GVO output video is locked to the GPU output signal." },
    { "GvoSyncLockStatus",               NV_CTRL_GVO_SYNC_LOCK_STATUS,                 I|N,   "Returns whether or not the GVO device is locked to the input reference signal." },
    { "GvoANCTimeCodeGeneration",        NV_CTRL_GVO_ANC_TIME_CODE_GENERATION,         I,     "Controls whether the GVO device generates time codes in the ANC region of the SDI video output stream." },
    { "GvoComposite",                    NV_CTRL_GVO_COMPOSITE,                        I,     "Enables/Disables SDI compositing.  This attribute is only available when an SDI input source is detected and is in genlock mode." },
    { "GvoCompositeAlphaKey",            NV_CTRL_GVO_COMPOSITE_ALPHA_KEY,              I,     "When SDI compositing is enabled, this enables/disables alpha blending." },
    { "GvoCompositeNumKeyRanges",        NV_CTRL_GVO_COMPOSITE_NUM_KEY_RANGES,         I|N,   "Returns the number of ranges available for each channel (Y/Luma, Cr, and Cb) that are used SDI compositing through color keying." },
    { "GvioFirmwareVersion",             NV_CTRL_STRING_GVIO_FIRMWARE_VERSION,         I|S|N, "Indicates the version of the firmware on the GVO or GVI device." },
    { "GvoFirmwareVersion",              NV_CTRL_STRING_GVIO_FIRMWARE_VERSION,         I|S|N|A,"DEPRECATED: use \"GvioFirmwareVersion\" instead." },
    { "GvoSyncToDisplay",                NV_CTRL_GVO_SYNC_TO_DISPLAY,                  I|N,   "Controls synchronization of the non-SDI display to the SDI display when both are active." },
    { "GvoFullRangeColor",               NV_CTRL_GVO_FULL_RANGE_COLOR,                 I,     "Allow full range color data [4-1019].  If disabled, color data is clamped to [64-940]." },
    { "IsGvoDisplay",                    NV_CTRL_IS_GVO_DISPLAY,                       N|D,   "Returns whether or not the given display device is driven by the GVO device." },
    { "GvoEnableRGBData",                NV_CTRL_GVO_ENABLE_RGB_DATA,                  I,     "Indicates that RGB data is being sent via a PASSTHU mode." },
    { "GviNumJacks",                          NV_CTRL_GVI_NUM_JACKS,                            I|N, "Returns the number of input (BNC) jacks on a GVI device that can read video streams." },
    { "GviMaxLinksPerStream",                 NV_CTRL_GVI_MAX_LINKS_PER_STREAM,                 I|N, "Returns the maximum number of links that can make up a stream." },
    { "GviDetectedChannelBitsPerComponent",   NV_CTRL_GVI_DETECTED_CHANNEL_BITS_PER_COMPONENT,  I|N, "Returns the detected bits per component on the given jack+channel of the GVI device.  The jack+channel must be passed through via the display mask param where the jack number is in the lower 16 bits and the channel number is in the upper 16 bits." },
    { "GviRequestedStreamBitsPerComponent",   NV_CTRL_GVI_REQUESTED_STREAM_BITS_PER_COMPONENT,  I,   "Indicates the number of bits per component for a capture stream." },
    { "GviDetectedChannelComponentSampling",  NV_CTRL_GVI_DETECTED_CHANNEL_COMPONENT_SAMPLING,  I|N, "Returns the detected sampling format on the given jack+channel of the GVI device.  The jack+channel must be passed through via the display mask param where the jack number is in the lower 16 bits and the channel number is in the upper 16 bits." },
    { "GviRequestedStreamComponentSampling",  NV_CTRL_GVI_REQUESTED_STREAM_COMPONENT_SAMPLING,  I,   "Indicates the sampling format for a capture stream." },
    { "GviRequestedStreamChromaExpand",       NV_CTRL_GVI_REQUESTED_STREAM_CHROMA_EXPAND,       I,   "Indicates whether 4:2:2 -> 4:4:4 chroma expansion is enabled for the capture stream." },
    { "GviDetectedChannelColorSpace",         NV_CTRL_GVI_DETECTED_CHANNEL_COLOR_SPACE,         I|N, "Returns the detected color space (RGB, YCRCB, etc) for the given jack+channel of the GVI device.  The jack+channel must be passed through via the display mask param where the jack number is in the lower 16 bits and the channel number is in the upper 16 bits." },
    { "GviDetectedChannelLinkID",             NV_CTRL_GVI_DETECTED_CHANNEL_LINK_ID,             I|N, "Returns the detected link identifier for the given jack+channel of the GVI device.  The jack+channel must be passed through via the display mask param where the jack number is in the lower 16 bits and the channel number is in the upper 16 bits." },
    { "GviDetectedChannelSMPTE352Identifier", NV_CTRL_GVI_DETECTED_CHANNEL_SMPTE352_IDENTIFIER, I|N, "Returns the detected 4-byte SMPTE 352 identifier from the given jack+channel of the GVI device.  The jack+channel must be passed through via the display mask param where the jack number is in the lower 16 bits and the channel number is in the upper 16 bits." },
    { "GviGlobalIdentifier",                  NV_CTRL_GVI_GLOBAL_IDENTIFIER,                    I|N, "Returns the global identifier for the given NV-CONTROL GVI device." },
    { "GviMaxChannelsPerJack",                NV_CTRL_GVI_MAX_CHANNELS_PER_JACK,                I|N, "Returns the maximum supported number of channels per single jack on a GVI device." },
    { "GviMaxStreams",                        NV_CTRL_GVI_MAX_STREAMS,                          I|N, "Returns the maximum supported number of streams that can be configured on a GVI device." },
    { "GviNumCaptureSurfaces",                NV_CTRL_GVI_NUM_CAPTURE_SURFACES,                 I|N, "Controls the number of capture buffers for storing incoming video from the GVI device." },
    { "GviBoundGpu",                          NV_CTRL_GVI_BOUND_GPU,                            I|N, "Returns the target index of the GPU currently attached to the GVI device." },
    { "GviTestMode",                          NV_CTRL_GVI_TEST_MODE,                            I|N, "Enable or disable GVI test mode." },
    { "GvoCSCMatrix",                         0,                                                I|M|N, "Sets the GVO Color Space Conversion (CSC) matrix.  Accepted values are \"ITU_601\", \"ITU_709\", \"ITU_177\", and \"Identity\"." },

    /* Display */
    { "Brightness",                 BRIGHTNESS_VALUE|ALL_CHANNELS,         N|C|G, "Controls the overall brightness of the display." },
    { "RedBrightness",              BRIGHTNESS_VALUE|RED_CHANNEL,          C|G,   "Controls the brightness of the color red in the display." },
    { "GreenBrightness",            BRIGHTNESS_VALUE|GREEN_CHANNEL,        C|G,   "Controls the brightness of the color green in the display." },
    { "BlueBrightness",             BRIGHTNESS_VALUE|BLUE_CHANNEL,         C|G,   "Controls the brightness of the color blue in the display." },
    { "Contrast",                   CONTRAST_VALUE|ALL_CHANNELS,           N|C|G, "Controls the overall contrast of the display." },
    { "RedContrast",                CONTRAST_VALUE|RED_CHANNEL,            C|G,   "Controls the contrast of the color red in the display." },
    { "GreenContrast",              CONTRAST_VALUE|GREEN_CHANNEL,          C|G,   "Controls the contrast of the color green in the display." },
    { "BlueContrast",               CONTRAST_VALUE|BLUE_CHANNEL,           C|G,   "Controls the contrast of the color blue in the display." },
    { "Gamma",                      GAMMA_VALUE|ALL_CHANNELS,              N|C|G, "Controls the overall gamma of the display." },
    { "RedGamma",                   GAMMA_VALUE|RED_CHANNEL,               C|G,   "Controls the gamma of the color red in the display." },
    { "GreenGamma",                 GAMMA_VALUE|GREEN_CHANNEL,             C|G,   "Controls the gamma of the color green in the display." },
    { "BlueGamma",                  GAMMA_VALUE|BLUE_CHANNEL,              C|G,   "Controls the gamma of the color blue in the display." },
    { "Dithering",                  NV_CTRL_DITHERING,                     0,     "Controls the dithering: auto (0), enabled (1), disabled (2)." },
    { "CurrentDithering",           NV_CTRL_CURRENT_DITHERING,             0,     "Returns the current dithering state: enabled (1), disabled (0)." },
    { "DitheringMode",              NV_CTRL_DITHERING_MODE,                0,     "Controls the dithering mode when CurrentDithering=1; auto (0), temporally repeating dithering pattern (1), static dithering pattern (2), temporally stochastic dithering (3)." },
    { "CurrentDitheringMode",       NV_CTRL_CURRENT_DITHERING_MODE,        0,     "Returns the current dithering mode: none (0), temporally repeating dithering pattern (1), static dithering pattern (2), temporally stochastic dithering (3)." },
    { "DitheringDepth",             NV_CTRL_DITHERING_DEPTH,               0,     "Controls the dithering depth when CurrentDithering=1; auto (0), 6 bits per channel (1), 8 bits per channel (2)." },
    { "CurrentDitheringDepth",      NV_CTRL_CURRENT_DITHERING_DEPTH,       0,     "Returns the current dithering depth: none (0), 6 bits per channel (1), 8 bits per channel (2)." },
    { "DigitalVibrance",            NV_CTRL_DIGITAL_VIBRANCE,              0,     "Sets the digital vibrance level of the display device." },
    { "ImageSharpening",            NV_CTRL_IMAGE_SHARPENING,              0,     "Adjusts the sharpness of the display's image quality by amplifying high frequency content." },
    { "ImageSharpeningDefault",     NV_CTRL_IMAGE_SHARPENING_DEFAULT,      0,     "Returns default value of image sharpening." },
    { "FrontendResolution",         NV_CTRL_FRONTEND_RESOLUTION,           N|P,   "Returns the dimensions of the frontend (current) resolution as determined by the NVIDIA X Driver.  This attribute is a packed integer; the width is packed in the upper 16 bits and the height is packed in the lower 16-bits." },
    { "BackendResolution",          NV_CTRL_BACKEND_RESOLUTION,            N|P,   "Returns the dimensions of the backend resolution as determined by the NVIDIA X Driver.  The backend resolution is the resolution (supported by the display device) the GPU is set to scale to.  If this resolution matches the frontend resolution, GPU scaling will not be needed/used.  This attribute is a packed integer; the width is packed in the upper 16-bits and the height is packed in the lower 16-bits." },
    { "FlatpanelNativeResolution",  NV_CTRL_FLATPANEL_NATIVE_RESOLUTION,   N|P,   "Returns the dimensions of the native resolution of the flat panel as determined by the NVIDIA X Driver.  The native resolution is the resolution at which a flat panel must display any image.  All other resolutions must be scaled to this resolution through GPU scaling or the DFP's native scaling capabilities in order to be displayed.  This attribute is only valid for flat panel (DFP) display devices.  This attribute is a packed integer; the width is packed in the upper 16-bits and the height is packed in the lower 16-bits." },
    { "FlatpanelBestFitResolution", NV_CTRL_FLATPANEL_BEST_FIT_RESOLUTION, N|P,   "Returns the dimensions of the resolution, selected by the X driver, from the DFP's EDID that most closely matches the frontend resolution of the current mode.  The best fit resolution is selected on a per-mode basis.  This attribute is only valid for flat panel (DFP) display devices.  This attribute is a packed integer; the width is packed in the upper 16-bits and the height is packed in the lower 16-bits." },
    { "DFPScalingActive",           NV_CTRL_DFP_SCALING_ACTIVE,            N,     "Returns the current state of DFP scaling.  DFP scaling is mode-specific (meaning it may vary depending on which mode is currently set).  DFP scaling is active if the GPU is set to scale to the best fit resolution (GPUScaling is set to use FlatpanelBestFitResolution) and the best fit and native resolutions are different." },
    { "GPUScaling",                 NV_CTRL_GPU_SCALING,                   P,     "Controls what the GPU scales to and how.  This attribute is a packed integer; the scaling target (native/best fit) is packed in the upper 16-bits and the scaling method is packed in the lower 16-bits." },
    { "GPUScalingDefaultTarget",    NV_CTRL_GPU_SCALING_DEFAULT_TARGET,    0,     "Returns the default gpu scaling target for the Flatpanel." },
    { "GPUScalingDefaultMethod",    NV_CTRL_GPU_SCALING_DEFAULT_METHOD,    0,     "Returns the default gpu scaling method for the Flatpanel." },
    { "GPUScalingActive",           NV_CTRL_GPU_SCALING_ACTIVE,            N,     "Returns the current state of GPU scaling.  GPU scaling is mode-specific (meaning it may vary depending on which mode is currently set).  GPU scaling is active if the frontend timing (current resolution) is different than the target resolution.  The target resolution is either the native resolution of the flat panel or the best fit resolution supported by the flat panel.  What (and how) the GPU should scale to is controlled through the GPUScaling attribute." },
    { "RefreshRate",                NV_CTRL_REFRESH_RATE,                  N|H,   "Returns the refresh rate of the specified display device in cHz (Centihertz) (to get the refresh rate in Hz, divide the returned value by 100)." },
    { "RefreshRate3",               NV_CTRL_REFRESH_RATE_3,                N|K,   "Returns the refresh rate of the specified display device in mHz (Millihertz) (to get the refresh rate in Hz, divide the returned value by 1000)." },
    { "OverscanCompensation",       NV_CTRL_OVERSCAN_COMPENSATION,         0,     "Adjust the amount of overscan compensation scaling, in pixels, to apply to the specified display device." },
    { "ColorSpace",                 NV_CTRL_COLOR_SPACE,                   0,     "Sets the color space of the signal sent to the display device." },
    { "ColorRange",                 NV_CTRL_COLOR_RANGE,                   0,     "Sets the color range of the signal sent to the display device." },
    { "SynchronousPaletteUpdates",  NV_CTRL_SYNCHRONOUS_PALETTE_UPDATES,   0,     "Controls whether colormap updates are synchronized with X rendering." },

    /* TV */
    { "TVOverScan",      NV_CTRL_TV_OVERSCAN,       0, "Adjusts the amount of overscan on the specified display device." },
    { "TVFlickerFilter", NV_CTRL_TV_FLICKER_FILTER, 0, "Adjusts the amount of flicker filter on the specified display device." },
    { "TVBrightness",    NV_CTRL_TV_BRIGHTNESS,     0, "Adjusts the amount of brightness on the specified display device." },
    { "TVHue",           NV_CTRL_TV_HUE,            0, "Adjusts the amount of hue on the specified display device." },
    { "TVContrast",      NV_CTRL_TV_CONTRAST,       0, "Adjusts the amount of contrast on the specified display device." },
    { "TVSaturation",    NV_CTRL_TV_SATURATION,     0, "Adjusts the amount of saturation on the specified display device." },

    /* X Video */
    { "XVideoOverlaySaturation",   NV_CTRL_ATTR_XV_OVERLAY_SATURATION,     V,   "Controls the amount of saturation in the X video overlay." },
    { "XVideoOverlayContrast",     NV_CTRL_ATTR_XV_OVERLAY_CONTRAST,       V,   "Controls the amount of contrast in the X video overlay." },
    { "XVideoOverlayBrightness",   NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS,     V,   "Controls the amount of brightness in the X video overlay." },
    { "XVideoOverlayHue",          NV_CTRL_ATTR_XV_OVERLAY_HUE,            V,   "Controls the amount of hue in the X video overlay." },
    { "XVideoTextureBrightness",   NV_CTRL_ATTR_XV_TEXTURE_BRIGHTNESS,     V,   "Controls the amount of brightness in the X video texture adaptor." },
    { "XVideoTextureContrast",     NV_CTRL_ATTR_XV_TEXTURE_CONTRAST,       V,   "Controls the amount of contrast in the X video texture adaptor." },
    { "XVideoTextureHue",          NV_CTRL_ATTR_XV_TEXTURE_HUE,            V,   "Controls the amount of hue in the X video texture adaptor." },
    { "XVideoTextureSaturation",   NV_CTRL_ATTR_XV_TEXTURE_SATURATION,     V,   "Controls the amount of saturation in the X video texture adaptor." },

    { "XVideoTextureSyncToVBlank", NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK, V,   "Enables sync to vertical blanking for X video texture adaptor." },
    { "XVideoBlitterSyncToVBlank", NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK, V,   "Enables sync to vertical blanking for X video blitter adaptor." },
    { "XVideoSyncToDisplay",       NV_CTRL_XV_SYNC_TO_DISPLAY,             D|Z, "Controls which display device is synced to by the texture and blitter adaptors when they are set to synchronize to the vertical blanking." },

    /* 3D Vision Pro */
    {"3DVisionProResetTransceiverToFactorySettings", NV_CTRL_3D_VISION_PRO_RESET_TRANSCEIVER_TO_FACTORY_SETTINGS, N,     "Resets the 3D Vision Pro transceiver to its factory settings."},
    {"3DVisionProTransceiverChannel",                NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL,                   N,     "Controls the channel that is currently used by the 3D Vision Pro transceiver."},
    {"3DVisionProTransceiverMode",                   NV_CTRL_3D_VISION_PRO_TRANSCEIVER_MODE,                      N,     "Controls the mode in which the 3D Vision Pro transceiver operates."},
    {"3DVisionProTransceiverChannelFrequency",       NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_FREQUENCY,         N|T,   "Returns the frequency of the channel(in kHz) of the 3D Vision Pro transceiver."},
    {"3DVisionProTransceiverChannelQuality",         NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_QUALITY,           N|T,   "Returns the quality of the channel(in percentage) of the 3D Vision Pro transceiver."},
    {"3DVisionProTransceiverChannelCount",           NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_COUNT,             N,     "Returns the number of channels on the 3D Vision Pro transceiver."},
    {"3DVisionProPairGlasses",                       NV_CTRL_3D_VISION_PRO_PAIR_GLASSES,                          N,     "Puts the 3D Vision Pro transceiver into pairing mode to gather additional glasses."},
    {"3DVisionProUnpairGlasses",                     NV_CTRL_3D_VISION_PRO_UNPAIR_GLASSES,                        N,     "Tells a specific pair of glasses to unpair."},
    {"3DVisionProDiscoverGlasses",                   NV_CTRL_3D_VISION_PRO_DISCOVER_GLASSES,                      N,     "Tells the 3D Vision Pro transceiver about the glasses that have been paired using NV_CTRL_3D_VISION_PRO_PAIR_GLASSES_BEACON."},
    {"3DVisionProIdentifyGlasses",                   NV_CTRL_3D_VISION_PRO_IDENTIFY_GLASSES,                      N,     "Causes glasses LEDs to flash for a short period of time."},
    {"3DVisionProGlassesSyncCycle",                  NV_CTRL_3D_VISION_PRO_GLASSES_SYNC_CYCLE,                    N|T,   "Controls the sync cycle duration(in milliseconds) of the glasses."},
    {"3DVisionProGlassesMissedSyncCycles",           NV_CTRL_3D_VISION_PRO_GLASSES_MISSED_SYNC_CYCLES,            N|T,   "Returns the number of state sync cycles recently missed by the glasses."},
    {"3DVisionProGlassesBatteryLevel",               NV_CTRL_3D_VISION_PRO_GLASSES_BATTERY_LEVEL,                 N|T,   "Returns the battery level(in percentage) of the glasses."},
    {"3DVisionProTransceiverHardwareRevision",       NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_HARDWARE_REVISION,  S|N,   "Returns the hardware revision of the 3D Vision Pro transceiver."},
    {"3DVisionProTransceiverFirmwareVersionA",       NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_VERSION_A, S|N,   "Returns the firmware version of chip A of the 3D Vision Pro transceiver."},
    {"3DVisionProTransceiverFirmwareDateA",          NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_DATE_A,    S|N,   "Returns the date of the firmware of chip A of the 3D Vision Pro transceiver."},
    {"3DVisionProTransceiverFirmwareVersionB",       NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_VERSION_B, S|N,   "Returns the firmware version of chip B of the 3D Vision Pro transceiver."},
    {"3DVisionProTransceiverFirmwareDateB",          NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_FIRMWARE_DATE_B,    S|N,   "Returns the date of the firmware of chip B of the 3D Vision Pro transceiver."},
    {"3DVisionProTransceiverAddress",                NV_CTRL_STRING_3D_VISION_PRO_TRANSCEIVER_ADDRESS,            S|N,   "Returns the RF address of the 3D Vision Pro transceiver."},
    {"3DVisionProGlassesFirmwareVersionA",           NV_CTRL_STRING_3D_VISION_PRO_GLASSES_FIRMWARE_VERSION_A,     S|N|T, "Returns the firmware version of chip A of the glasses."},
    {"3DVisionProGlassesFirmwareDateA",              NV_CTRL_STRING_3D_VISION_PRO_GLASSES_FIRMWARE_DATE_A,        S|N|T, "Returns the date of the firmware of chip A of the glasses."},
    {"3DVisionProGlassesAddress",                    NV_CTRL_STRING_3D_VISION_PRO_GLASSES_ADDRESS,                S|N|T, "Returns the RF address of the glasses."},
    {"3DVisionProGlassesName",                       NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME,                   S|N|T, "Controls the name the glasses should use."},

    { NULL, 0, 0, NULL }
};

#undef F
#undef C
#undef N
#undef G
#undef V
#undef P
#undef D
#undef A
#undef Z
#undef H
#undef K
#undef S
#undef I
#undef W

/*
 * When new integer attributes are added to NVCtrl.h, an entry should
 * be added in the above attributeTable[].  The below #if should also
 * be updated to indicate the last attribute that the table knows
 * about.
 */

#if NV_CTRL_LAST_ATTRIBUTE != NV_CTRL_3D_VISION_PRO_GLASSES_UNPAIR_EVENT
#warning "Have you forgotten to add a new integer attribute to attributeTable?"
#endif



/*
 * targetTypeTable[] - this table stores an association of the values
 * for each attribute target type.
 */

TargetTypeEntry targetTypeTable[] = {

    { "X Screen",                    /* name */
      "screen",                      /* parsed_name */
      X_SCREEN_TARGET,               /* target_index */
      NV_CTRL_TARGET_TYPE_X_SCREEN,  /* nvctrl */
      ATTRIBUTE_TYPE_X_SCREEN,       /* permission_bit */
      NV_TRUE,                       /* uses_display_devices */
      1, 6 },                        /* required major,minor protocol rev */
    
    { "GPU",                         /* name */
      "gpu",                         /* parsed_name */
      GPU_TARGET,                    /* target_index */
      NV_CTRL_TARGET_TYPE_GPU,       /* nvctrl */
      ATTRIBUTE_TYPE_GPU,            /* permission_bit */
      NV_TRUE,                       /* uses_display_devices */
      1, 10 },                       /* required major,minor protocol rev */
    
    { "Frame Lock Device",           /* name */
      "framelock",                   /* parsed_name */
      FRAMELOCK_TARGET,              /* target_index */
      NV_CTRL_TARGET_TYPE_FRAMELOCK, /* nvctrl */
      ATTRIBUTE_TYPE_FRAMELOCK,      /* permission_bit */
      NV_FALSE,                      /* uses_display_devices */
      1, 10 },                       /* required major,minor protocol rev */

    { "VCS",                         /* name */
      "vcs",                         /* parsed_name */
      VCS_TARGET,                    /* target_index */
      NV_CTRL_TARGET_TYPE_VCSC,      /* nvctrl */
      ATTRIBUTE_TYPE_VCSC,           /* permission_bit */
      NV_FALSE,                      /* uses_display_devices */
      1, 12 },                       /* required major,minor protocol rev */

    { "SDI Input Device",            /* name */
      "gvi",                         /* parsed_name */
      GVI_TARGET,                    /* target_index */
      NV_CTRL_TARGET_TYPE_GVI,       /* nvctrl */
      ATTRIBUTE_TYPE_GVI,            /* permission_bit */
      NV_FALSE,                      /* uses_display_devices */
      1, 18 },                       /* required major,minor protocol rev */

    { "Fan",                         /* name */
      "fan",                         /* parsed_name */
      COOLER_TARGET,                 /* target_index */
      NV_CTRL_TARGET_TYPE_COOLER,    /* nvctrl */
      ATTRIBUTE_TYPE_COOLER,         /* permission_bit */
      NV_FALSE,                      /* uses_display_devices */
      1, 20 },                       /* required major,minor protocol rev */
    
    { "Thermal Sensor",              /* name */
      "thermalsensor",               /* parsed_name */
      THERMAL_SENSOR_TARGET,         /* target_index */
      NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,    /* nvctrl */
      ATTRIBUTE_TYPE_THERMAL_SENSOR, /* permission_bit */
      NV_FALSE,                      /* uses_display_devices */
      1, 23 },                       /* required major,minor protocol rev */

    { "3D Vision Pro Transceiver",                   /* name */
      "svp",                                         /* parsed_name */
      NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET,       /* target_index */
      NV_CTRL_TARGET_TYPE_3D_VISION_PRO_TRANSCEIVER, /* nvctrl */
      ATTRIBUTE_TYPE_3D_VISION_PRO_TRANSCEIVER,      /* permission_bit */
      NV_FALSE,                                      /* uses_display_devices */
      1, 25 },                                       /* required major,minor protocol rev */

    { "Display Device",                              /* name */
      "dpy",                                         /* parsed_name */
      DISPLAY_TARGET,                                /* target_index */
      NV_CTRL_TARGET_TYPE_DISPLAY,                   /* nvctrl */
      ATTRIBUTE_TYPE_DISPLAY,                        /* permission_bit */
      NV_FALSE,                                      /* uses_display_devices */
      1, 27 },                                       /* required major,minor protocol rev */

    { NULL, NULL, 0, 0, 0 },
};



/*
 * nv_get_sdi_csc_matrix() - see comments in parse.h
 */

static const float sdi_csc_itu601[15] = {
     0.2991,  0.5870,  0.1150, 0.0625, 0.85547, // Y
     0.5000, -0.4185, -0.0810, 0.5000, 0.87500, // Cr
    -0.1685, -0.3310,  0.5000, 0.5000, 0.87500, // Cb
};
static const float sdi_csc_itu709[15] = {
     0.2130,  0.7156,  0.0725, 0.0625, 0.85547, // Y
     0.5000, -0.4542, -0.0455, 0.5000, 0.87500, // Cr
    -0.1146, -0.3850,  0.5000, 0.5000, 0.87500, // Cb
};
static const float sdi_csc_itu177[15] = {
     0.412391, 0.357584, 0.180481, 0.0, 0.85547, // Y
     0.019331, 0.119195, 0.950532, 0.0, 0.87500, // Cr
     0.212639, 0.715169, 0.072192, 0.0, 0.87500, // Cb
};
static const float sdi_csc_identity[15] = {
     0.0000,  1.0000,  0.0000, 0.0000, 1.0, // Y (Green)
     1.0000,  0.0000,  0.0000, 0.0000, 1.0, // Cr (Red)
     0.0000,  0.0000,  1.0000, 0.0000, 1.0, // Cb (Blue)
};

const float * nv_get_sdi_csc_matrix(char *s)
{
    if (nv_strcasecmp(s, "itu_601")) {
        return sdi_csc_itu601;
    } else if (nv_strcasecmp(s, "itu_709")) {
        return sdi_csc_itu709;
    } else if (nv_strcasecmp(s, "itu_177")) {
        return sdi_csc_itu177;
    } else if (nv_strcasecmp(s, "identity")) {
        return sdi_csc_identity;
    }

    return NULL;
}



/*
 * nv_parse_attribute_string() - see comments in parse.h
 */

int nv_parse_attribute_string(const char *str, int query, ParsedAttribute *a)
{
    char *s, *tmp, *name, *start, *display_device_name, *no_spaces = NULL;
    char tmpname[NV_PARSER_MAX_NAME_LEN];
    AttributeTableEntry *t;
    int len, ret;

#define stop(x) { if (no_spaces) free(no_spaces); return (x); }
    if (!a) stop(NV_PARSER_STATUS_BAD_ARGUMENT);

    /* clear the ParsedAttribute struct */

    memset((void *) a, 0, sizeof(ParsedAttribute));

    /* remove any white space from the string, to simplify parsing */

    no_spaces = remove_spaces(str);
    if (!no_spaces) stop(NV_PARSER_STATUS_EMPTY_STRING);
    
    /*
     * get the display name... ie: everything before the
     * DISPLAY_NAME_SEPARATOR
     */

    s = strchr(no_spaces, DISPLAY_NAME_SEPARATOR);

    /*
     * If we found a DISPLAY_NAME_SEPARATOR, and there is some text
     * before it, parse that text as an X Display name, X screen,
     * and/or a target specification.
     */

    if ((s) && (s != no_spaces)) {
        
        ret = nv_parse_display_and_target(no_spaces, s, a);

        if (ret != NV_PARSER_STATUS_SUCCESS) {
            stop(ret);
        }
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
    
    /* look up the requested name */

    for (t = attributeTable; t->name; t++) {
        if (nv_strcasecmp(tmpname, t->name)) {
            a->name = t->name;
            a->attr = t->attr;
            a->flags |= t->flags;
            break;
        }
    }
    
    if (!a->name) stop(NV_PARSER_STATUS_UNKNOWN_ATTR_NAME);
    
    /* read the display device name, if any */
    
    if (*s == '[') {
        s++;
        start = s;
        while (*s && *s != ']') s++;
        display_device_name = nv_strndup(start, s - start);
        a->display_device_mask =
            display_device_name_to_display_device_mask(display_device_name);
        /*
         * stop parsing if the display device mask is invalid (and the
         * display device mask is not hijacked for something other than
         * display)
         */

        if ((a->display_device_mask == INVALID_DISPLAY_DEVICE_MASK) &&
            !(a->flags & NV_PARSER_TYPE_HIJACK_DISPLAY_DEVICE))
            stop(NV_PARSER_STATUS_BAD_DISPLAY_DEVICE);

        a->flags |= NV_PARSER_HAS_DISPLAY_DEVICE;
        if (*s == ']') s++;
    }
    
    if (query == NV_PARSER_ASSIGNMENT) {
        
        /* there should be an equal sign */
    
        if (*s == '=') s++;
        else stop(NV_PARSER_STATUS_MISSING_EQUAL_SIGN);
        
        /* read the value */
    
        tmp = s;
        if (a->flags & NV_PARSER_TYPE_COLOR_ATTRIBUTE) {
            /* color attributes are floating point */
            a->fval = strtod(s, &tmp);
        } else if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            /*
             * Either a single 32-bit integer or two 16-bit
             * integers, separated by ','.
             * Passing base as 0 allows packed values to be specified 
             * in hex (Bug 377242)
             */
            a->val = strtol(s, &tmp, 0);
            
            if (tmp && *tmp == ',') {
                a->val = (a->val & 0xffff) << 16;
                a->val |= strtol((tmp + 1), &tmp, 0) & 0xffff;
            }
        } else if (a->flags & NV_PARSER_TYPE_VALUE_IS_DISPLAY) {
            if (nv_strcasecmp(s, "alldisplays")) {
                a->flags |= NV_PARSER_TYPE_ASSIGN_ALL_DISPLAYS;
                tmp = s + strlen(s);
            } else {
                uint32 mask = 0;
                mask = display_device_name_to_display_device_mask(s);
                if (mask && (mask != INVALID_DISPLAY_DEVICE_MASK) &&
                    ((mask & (DISPLAY_DEVICES_WILDCARD_CRT |
                              DISPLAY_DEVICES_WILDCARD_TV |
                              DISPLAY_DEVICES_WILDCARD_DFP)) == 0)) {
                    a->val = mask;
                    tmp = s + strlen(s);
                } else {
                   a->val = strtol(s, &tmp, 0);
                }
            }
        } else if (a->flags & NV_PARSER_TYPE_SDI_CSC) {
            /* String that names a standard CSC matrix */
            a->pfval = nv_get_sdi_csc_matrix(s);
            tmp = s + strlen(s);
        } else {
            /* all other attributes are integer */
            a->val = strtol(s, &tmp, 0);
        }
         
        if (tmp && (s != tmp)) a->flags |= NV_PARSER_HAS_VAL;
        s = tmp;
        
        if (!(a->flags & NV_PARSER_HAS_VAL)) stop(NV_PARSER_STATUS_NO_VALUE);
    }
    
    /* this should be the end of the string */

    if (*s != '\0') stop(NV_PARSER_STATUS_TRAILING_GARBAGE);

    stop(NV_PARSER_STATUS_SUCCESS);
    
} /* nv_parse_attribute_string() */



/*
 * nv_parse_display_and_target() - helper function for
 * nv_parse_attribute_string() to parse all the text before the
 * DISPLAY_NAME_SEPARATOR.  This text is expected to be an X Display
 * name, just an X screen, and/or a target specification.
 */

static int nv_parse_display_and_target(char *start,
                                       char *end, /* exclusive */
                                       ParsedAttribute *a)
{
    int digits_only, i, target_type, target_id, len;
    char *tmp, *s, *pOpen, *pClose, *colon;

    /*
     * are all characters numeric? compute the target_id integer as we
     * scan the string to check
     */
    
    digits_only = NV_TRUE;
    target_id = 0;
    
    for (s = start; s < end; s++) {
        if (!isdigit(*s)) {
            digits_only = NV_FALSE;
            break;
        }
        target_id = (target_id * 10) + ctoi(*s);
    }

    /*
     * if all characters are numeric, assume the target type is
     * X_SCREEN, and build the target_id; we have no X Display name in
     * this case.
     */
    
    if (digits_only) {
        a->display = NULL;
        a->flags &= ~NV_PARSER_HAS_X_DISPLAY;
        a->flags |= NV_PARSER_HAS_TARGET;
        a->target_id = target_id;
        a->target_type = NV_CTRL_TARGET_TYPE_X_SCREEN;
        
        /* we are done */
        
        return NV_PARSER_STATUS_SUCCESS;
    }
    
    /*
     * if we get here, then there are non-digit characters; look for a
     * pair of brackets, and treat the contents as a target
     * specification.
     */
    
    pOpen = pClose = NULL;

    for (s = start; s < end; s++) {
        if (*s == '[') pOpen = s;
        if (*s == ']') pClose = s;
    }

    if (pOpen && pClose && (pClose > pOpen) && ((pClose - pOpen) > 1)) {

        /*
         * we have a pair of brackets and something inside the
         * brackets, pull that into a temporary string.
         */

        len = pClose - pOpen - 1;

        tmp = nv_strndup(pOpen + 1, len);

        /* find the colon within the temp string */

        colon = strchr(tmp, ':');
        
        /* no colon? give up */

        if (!colon) {
            free(tmp);
            return NV_PARSER_STATUS_TARGET_SPEC_NO_COLON;
        }
        
        /*
         * check that what is between the opening bracket and the
         * colon is a target type name
         */

        *colon = '\0';
        target_type = -1;

        for (i = 0; targetTypeTable[i].name; i++) {
            if (nv_strcasecmp(tmp, targetTypeTable[i].parsed_name)) {
                target_type = targetTypeTable[i].nvctrl;
                break;
            }
        }
        
        *colon = ':';
        
        /* if we did not find a matching target name, give up */
        
        if (target_type == -1) {
            free(tmp);
            return NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET;
        }
        
        /*
         * check that we have something between the colon and the end
         * of the temp string
         */

        if ((colon + 1 - tmp) >= len) {
            free(tmp);
            return NV_PARSER_STATUS_TARGET_SPEC_NO_TARGET_ID;
        }

        /*
         * everything after the colon should be numeric; assign it to
         * the target_id
         */
        
        target_id = 0;

        for (s = colon + 1; *s; s++) {
            if (!isdigit(*s)) {
                free(tmp);
                return NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET_ID;
            }
            target_id = (target_id * 10) + ctoi(*s);
        }
        
        a->target_type = target_type;
        a->target_id = target_id;
        
        a->flags |= NV_PARSER_HAS_TARGET;

        /* we're finally done with the temp string */

        free(tmp);
        
        /*
         * check that there is no stray text between the closing
         * bracket and the end of our parsable string
         */

        if ((end - pClose) > 1) {
            return NV_PARSER_STATUS_TARGET_SPEC_TRAILING_GARBAGE;
        }
        
        /*
         * make end now point at the start of the bracketed target
         * info for the X Display name processing below
         */
        
        end = pOpen;
    }
    

    /* treat everything between start and end as an X Display name */
    
    if (start < end) {

        a->display = nv_strndup(start, end - start);
        a->flags |= NV_PARSER_HAS_X_DISPLAY;
            
        /*
         * this will attempt to parse out any screen number from the
         * display name
         */
    
        nv_assign_default_display(a, NULL);
    }
    
    /* done */

    return NV_PARSER_STATUS_SUCCESS;
    
} /* nv_parse_display_and_target() */



/*
 * nv_parse_strerror() - given the error status returned by
 * nv_parse_attribute_string(), return a string describing the
 * error.
 */

char *nv_parse_strerror(int status)
{
    switch (status) {
    case NV_PARSER_STATUS_SUCCESS :
        return "No error"; break;
    case NV_PARSER_STATUS_BAD_ARGUMENT :
        return "Bad argument"; break;
    case NV_PARSER_STATUS_EMPTY_STRING :
        return "Emtpy string"; break;
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

uint32 display_device_name_to_display_device_mask(const char *str)
{
    uint32 mask = 0;
    char *s, **toks, *endptr;
    int i, n;
    unsigned long int num;

    /* sanity check */

    if (!str || !*str) return INVALID_DISPLAY_DEVICE_MASK;
    
    /* remove spaces from the string */

    s = remove_spaces(str);
    if (!s || !*s) return INVALID_DISPLAY_DEVICE_MASK;
    
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

    display_device_name_string = malloc(DISPLAY_DEVICE_STRING_LEN);

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
 * expand_display_device_mask_wildcards() - build a display mask by
 * taking any of the real display mask bits; if there are any wildcard
 * flags set, or in all display devices of that type into the display
 * mask.
 */

uint32 expand_display_device_mask_wildcards(const uint32 d, const uint32 e)
{
    uint32 mask = d & VALID_DISPLAY_DEVICES_MASK;

    if (d & DISPLAY_DEVICES_WILDCARD_CRT) mask |= (e & BITMASK_ALL_CRT);
    if (d & DISPLAY_DEVICES_WILDCARD_TV)  mask |= (e & BITMASK_ALL_TV);
    if (d & DISPLAY_DEVICES_WILDCARD_DFP) mask |= (e & BITMASK_ALL_DFP);
    
    return mask;

} /* expand_display_device_mask_wildcards() */



/*
 * nv_assign_default_display() - assign an X display, if none has been
 * assigned already.  Also, parse the the display name to find any
 * specified X screen.
 */

void nv_assign_default_display(ParsedAttribute *a, const char *display)
{
    char *colon, *dot, *s;
    int digits_only;

    if (!(a->flags & NV_PARSER_HAS_X_DISPLAY)) {
        if (display) a->display = strdup(display);
        else a->display = NULL;
        a->flags |= NV_PARSER_HAS_X_DISPLAY;
    }

    if (!(a->flags & NV_PARSER_HAS_TARGET) && a->display) {
        colon = strchr(a->display, ':');
        if (colon) {
            dot = strchr(colon, '.');
            if (dot) {
                
                /*
                 * if all characters afer the '.' are digits,
                 * interpret it as a screen number.
                 */

                digits_only = NV_FALSE;
                a->target_id = 0;

                for (s = dot + 1; *s; s++) {
                    if (isdigit(*s)) {
                        digits_only = NV_TRUE;
                        a->target_id = (a->target_id * 10) + ctoi(*s);
                    } else {
                        digits_only = NV_FALSE;
                        break;
                    }
                }

                if (digits_only) {
                    a->target_type = NV_CTRL_TARGET_TYPE_X_SCREEN;
                    a->flags |= NV_PARSER_HAS_TARGET;
                }
            }
        }
    }
} /* nv_assign_default_display() */



/* 
 * nv_parsed_attribute_init() - initialize a ParsedAttribute linked
 * list
 */

ParsedAttribute *nv_parsed_attribute_init(void)
{
    ParsedAttribute *p = calloc(1, sizeof(ParsedAttribute));

    p->next = NULL;

    return p;
    
} /* nv_parsed_attribute_init() */



/*
 * nv_parsed_attribute_add() - add a new parsed attribute node to the
 * linked list
 */

void nv_parsed_attribute_add(ParsedAttribute *head, ParsedAttribute *a)
{
    ParsedAttribute *p, *t;

    p = calloc(1, sizeof(ParsedAttribute));

    p->next = NULL;
    
    for (t = head; t->next; t = t->next);
    
    t->next = p;
    
    if (a->display) t->display = strdup(a->display);
    else t->display = NULL;
    
    t->target_type         = a->target_type;
    t->target_id           = a->target_id;
    t->attr                = a->attr;
    t->val                 = a->val;
    t->fval                = a->fval;
    t->display_device_mask = a->display_device_mask;
    t->flags               = a->flags;
    
} /* nv_parsed_attribute_add() */



/*
 * nv_parsed_attribute_free() - free the linked list
 */

void nv_parsed_attribute_free(ParsedAttribute *p)
{
    ParsedAttribute *n;
    
    while(p) {
        n = p->next;
        if (p->display) free(p->display);
        free(p);
        p = n;
    }

} /* nv_parsed_attribute_free() */



/*
 * nv_parsed_attribute_clean() - clean out the ParsedAttribute list,
 * so that only the empty head node remains.
 */

void nv_parsed_attribute_clean(ParsedAttribute *p)
{
    nv_parsed_attribute_free(p->next);

    if (p->display) free(p->display);
    if (p->name) free(p->name);
    
    memset(p, 0, sizeof(ParsedAttribute));

} /* nv_parsed_attribute_clean() */



/*
 * nv_get_attribute_name() - scan the attributeTable for the name that
 * corresponds to the attribute constant.
 */

const char *nv_get_attribute_name(const int attr, const int flagsMask,
                                  const int flags)
{
    int i;

    for (i = 0; attributeTable[i].name; i++) {
        if (attributeTable[i].attr == attr &&
            (attributeTable[i].flags & flagsMask) == (flags & flagsMask)) {
            return attributeTable[i].name;
        }
    }

    return NULL;
    
} /* nv_get_attribute_name() */



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
            tmp = malloc(len);
            snprintf(tmp, len, "%s%s", uname_buf.nodename, display_name);
            free(display_name);
            display_name = tmp;
            colon = strchr(display_name, ':');
            if (!colon) return NULL;
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
        screen_name = malloc(len);
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
    
    no_spaces = malloc(len + 1);

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
 * output string, replacing any occurances of the character
 * 'c' with the character 'r'.
 */

char *replace_characters(const char *o, const char c, const char r)
{
    int len;
    char *m, *out;

    if (!o)
        return NULL;

    len = strlen(o);

    out = malloc(len + 1);
    m = out;

    while (*o != '\0') {
        *m++ = (*o == c) ? r : *o;
        o++;
    }
    *m = '\0';

    len = (m - out + 1);
    out = realloc(out, len);

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

static char **nv_strtok(char *s, char c, int *n)
{
    int count, i, len;
    char **delims, **tokens, *m;
    
    count = count_number_of_chars(s, c);
    
    /*
     * allocate and set pointers to each division (each instance of the
     * dividing character, and the terminating NULL of the string)
     */
    
    delims = malloc((count + 1) * sizeof(char *));
    m = s;
    for (i = 0; i < count; i++) {
        while (*m != c) m++;
        delims[i] = m;
        m++;
    }
    delims[count] = (char *) strchr(s, '\0');
    
    /*
     * so now, we have pointers to each deliminator; copy what's in between
     * the divisions (the tokens) into the dynamic array of strings
     */
    
    tokens = malloc((count + 1) * sizeof(char *));
    len = delims[0] - s;
    tokens[0] = nv_strndup(s, len);
    
    for (i = 1; i < count+1; i++) {
        len = delims[i] - delims[i-1];
        tokens[i] = nv_strndup(delims[i-1]+1, len-1);
    }
    
    free(delims);
    
    *n = count+1;
    return (tokens);
    
} /* nv_strtok() */



/*
 * nv_free_strtoks() - free an array of arrays, such as what is
 * allocated and returned by nv_strtok()
 */

static void nv_free_strtoks(char **s, int n)
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



/*
 * nv_strndup() - this function takes a pointer to a string and a
 * length n, mallocs a new string of n+1, copies the first n chars
 * from the original string into the new, and null terminates the new
 * string.  The caller should free the string.
 */

static char *nv_strndup(char *s, int n)
{
    char *m = malloc(n + 1);
    strncpy (m, s, n);
    m[n] = '\0';
    return (m);
    
} /* nv_strndup() */



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
    while (*str && !name_terminated(*str, term))
        str++;

    *name = calloc(1, str - tmp + 1);
    if (!(*name)) {
        return NULL;
    }
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
 * or NULL if an error occured.
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



/** parse_read_float_range() *****************************************
 *
 * Reads the maximun/minimum information from a string in the
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
 * and dispatches the handeling of tokens to the given function with
 * the given data as an extra argument.
 *
 **/
int parse_token_value_pairs(const char *str, apply_token_func func,
                            void *data)
{
    char *token;
    char *value;


    if (str) {

        /* Parse each token */
        while (*str) {
            
            /* Read the token */
            str = parse_read_name(str, &token, '=');
            if (!str) return 0;
            
            /* Read the value */
            str = parse_read_name(str, &value, ',');
            if (!str) return 0;
            
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
