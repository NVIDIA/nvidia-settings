#ifndef __NVCTRL_H
#define __NVCTRL_H

/**************************************************************************/
/*
 * Integer attributes; these are settable/gettable via
 * XNVCTRLSetAttribute() and XNVCTRLQueryAttribute, respectively.
 * Some attributes may only be read; some may require a display_mask
 * argument.  This information is encoded in the "permission" comment
 * after each attribute #define, and can be queried at run time with
 * XNVCTRLQueryValidAttributeValues().
 *
 * Key to Integer Attribute "Permissions":
 *
 * R: The attribute is readable (in general, all attributes will be
 *    readable)
 *
 * W: The attribute is writable (attributes may not be writable for
 *    various reasons: they represent static system information, they
 *    can only be changed by changing an XF86Config option, etc).
 *
 * D: The attribute requires the display mask argument.  The
 *    attributes NV_CTRL_CONNECTED_DISPLAYS and NV_CTRL_ENABLED_DISPLAYS
 *    will be a bitmask of what display devices are connected and what
 *    display devices are enabled for use in X, respectively.  Each bit
 *    in the bitmask represents a display device; it is these bits which
 *    should be used as the display_mask when dealing with attributes
 *    designated with "D" below.  For attributes that do not require the
 *    display mask, the argument is ignored.
 */


/**************************************************************************/


/*
 * NV_CTRL_FLATPANEL_SCALING - the current flatpanel scaling state;
 * possible values are:
 *
 * 0: default (the driver will use whatever state is current)
 * 1: native (the driver will use the panel's scaler, if possible)
 * 2: scaled (the driver will use the GPU's scaler, if possible)
 * 3: centered (the driver will center the image)
 * 4: aspect scaled (scale with the GPU's scaler, but keep the aspect
 *    ratio correct)
 */

#define NV_CTRL_FLATPANEL_SCALING                               2  /* RWD */
#define NV_CTRL_FLATPANEL_SCALING_DEFAULT                       0
#define NV_CTRL_FLATPANEL_SCALING_NATIVE                        1
#define NV_CTRL_FLATPANEL_SCALING_SCALED                        2
#define NV_CTRL_FLATPANEL_SCALING_CENTERED                      3
#define NV_CTRL_FLATPANEL_SCALING_ASPECT_SCALED                 4


/*
 * NV_CTRL_FLATPANEL_DITHERING - the current flatpanel dithering
 * state; possible values are:
 *
 * 0: default  (the driver will decide when to dither)
 * 1: enabled  (the driver will always dither when possible)
 * 2: disabled (the driver will never dither)
 */

#define NV_CTRL_FLATPANEL_DITHERING                             3  /* RWD */
#define NV_CTRL_FLATPANEL_DITHERING_DEFAULT                     0
#define NV_CTRL_FLATPANEL_DITHERING_ENABLED                     1
#define NV_CTRL_FLATPANEL_DITHERING_DISABLED                    2


/*
 * NV_CTRL_DIGITAL_VIBRANCE - sets the digital vibrance level for the
 * specified display device.
 */

#define NV_CTRL_DIGITAL_VIBRANCE                                4  /* RWD */


/*
 * NV_CTRL_BUS_TYPE - returns the Bus type through which the GPU
 * driving the specified X screen is connected to the computer.
 */

#define NV_CTRL_BUS_TYPE                                        5  /* R-- */
#define NV_CTRL_BUS_TYPE_AGP                                    0
#define NV_CTRL_BUS_TYPE_PCI                                    1
#define NV_CTRL_BUS_TYPE_PCI_EXPRESS                            2


/*
 * NV_CTRL_VIDEO_RAM - returns the amount of video ram on the GPU
 * driving the specified X screen.
 */

#define NV_CTRL_VIDEO_RAM                                       6  /* R-- */


/*
 * NV_CTRL_IRQ - returns the interrupt request line used by the GPU
 * driving the specified X screen.
 */

#define NV_CTRL_IRQ                                             7  /* R-- */


/*
 * NV_CTRL_OPERATING_SYSTEM - returns the operating system on which
 * the X server is running.
 */

#define NV_CTRL_OPERATING_SYSTEM                                8  /* R-- */
#define NV_CTRL_OPERATING_SYSTEM_LINUX                          0
#define NV_CTRL_OPERATING_SYSTEM_FREEBSD                        1
#define NV_CTRL_OPERATING_SYSTEM_SUNOS                          2


/*
 * NV_CTRL_SYNC_TO_VBLANK - enables sync to vblank for OpenGL clients.
 * This setting is only applied to OpenGL clients that are started
 * after this setting is applied.
 */

#define NV_CTRL_SYNC_TO_VBLANK                                  9  /* RW- */
#define NV_CTRL_SYNC_TO_VBLANK_OFF                              0
#define NV_CTRL_SYNC_TO_VBLANK_ON                               1


/*
 * NV_CTRL_LOG_ANISO - enables anisotropic filtering for OpenGL
 * clients; on some NVIDIA hardware, this can only be enabled or
 * disabled; on other hardware different levels of anisotropic
 * filtering can be specified.  This setting is only applied to OpenGL
 * clients that are started after this setting is applied.
 */

#define NV_CTRL_LOG_ANISO                                       10 /* RW- */


/*
 * NV_CTRL_FSAA_MODE - the FSAA setting for OpenGL clients; possible
 * FSAA modes:
 * 
 * NV_CTRL_FSAA_MODE_2x     "2x Bilinear Multisampling"
 * NV_CTRL_FSAA_MODE_2x_5t  "2x Quincunx Multisampling"
 * NV_CTRL_FSAA_MODE_15x15  "1.5 x 1.5 Supersampling"
 * NV_CTRL_FSAA_MODE_2x2    "2 x 2 Supersampling"
 * NV_CTRL_FSAA_MODE_4x     "4x Bilinear Multisampling"
 * NV_CTRL_FSAA_MODE_4x_9t  "4x Gaussian Multisampling"
 * NV_CTRL_FSAA_MODE_8x     "2x Bilinear Multisampling by 4x Supersampling"
 * NV_CTRL_FSAA_MODE_16x    "4x Bilinear Multisampling by 4x Supersampling"
 * NV_CTRL_FSAA_MODE_8xS    "4x Multisampling by 2x Supersampling"
 *
 * This setting is only applied to OpenGL clients that are started
 * after this setting is applied.
 */

#define NV_CTRL_FSAA_MODE                                       11 /* RW- */
#define NV_CTRL_FSAA_MODE_NONE                                  0
#define NV_CTRL_FSAA_MODE_2x                                    1
#define NV_CTRL_FSAA_MODE_2x_5t                                 2
#define NV_CTRL_FSAA_MODE_15x15                                 3
#define NV_CTRL_FSAA_MODE_2x2                                   4
#define NV_CTRL_FSAA_MODE_4x                                    5
#define NV_CTRL_FSAA_MODE_4x_9t                                 6
#define NV_CTRL_FSAA_MODE_8x                                    7
#define NV_CTRL_FSAA_MODE_16x                                   8
#define NV_CTRL_FSAA_MODE_8xS                                   9
#define NV_CTRL_FSAA_MODE_MAX NV_CTRL_FSAA_MODE_8xS


/*
 * NV_CTRL_TEXTURE_SHARPEN - enables texture sharpening for OpenGL
 * clients.  This setting is only applied to OpenGL clients that are
 * started after this setting is applied.
 */

#define NV_CTRL_TEXTURE_SHARPEN                                 12 /* RW- */
#define NV_CTRL_TEXTURE_SHARPEN_OFF                             0
#define NV_CTRL_TEXTURE_SHARPEN_ON                              1


/*
 * NV_CTRL_UBB - returns whether UBB is enabled for the specified X
 * screen.
 */

#define NV_CTRL_UBB                                             13 /* R-- */
#define NV_CTRL_UBB_OFF                                         0
#define NV_CTRL_UBB_ON                                          1


/*
 * NV_CTRL_OVERLAY - returns whether the RGB overlay is enabled for
 * the specified X screen.
 */

#define NV_CTRL_OVERLAY                                         14 /* R-- */
#define NV_CTRL_OVERLAY_OFF                                     0
#define NV_CTRL_OVERLAY_ON                                      1


/*
 * NV_CTRL_STEREO - returns whether stereo (and what type) is enabled
 * for the specified X screen.
 */

#define NV_CTRL_STEREO                                          16 /* R-- */
#define NV_CTRL_STEREO_OFF                                      0
#define NV_CTRL_STEREO_DDC                                      1
#define NV_CTRL_STEREO_BLUELINE                                 2
#define NV_CTRL_STEREO_DIN                                      3
#define NV_CTRL_STEREO_TWINVIEW                                 4


/*
 * NV_CTRL_EMULATE - controls OpenGL software emulation of future
 * NVIDIA GPUs.
 */

#define NV_CTRL_EMULATE                                         17 /* RW- */
#define NV_CTRL_EMULATE_NONE                                    0


/*
 * NV_CTRL_TWINVIEW - returns whether TwinView is enabled for the
 * specified X screen.
 */

#define NV_CTRL_TWINVIEW                                        18 /* R-- */
#define NV_CTRL_TWINVIEW_NOT_ENABLED                            0
#define NV_CTRL_TWINVIEW_ENABLED                                1


/*
 * NV_CTRL_CONNECTED_DISPLAYS - returns a display mask indicating what
 * display devices are connected to the GPU driving the specified X
 * screen.
 */

#define NV_CTRL_CONNECTED_DISPLAYS                              19 /* R-- */


/*
 * NV_CTRL_ENABLED_DISPLAYS - returns a display mask indicating what
 * display devices are enabled for use on the specified X screen.
 */

#define NV_CTRL_ENABLED_DISPLAYS                                20 /* R-- */

/**************************************************************************/
/*
 * Integer attributes specific to configuring FrameLock on boards that
 * support it.
 */


/*
 * NV_CTRL_FRAMELOCK - returns whether this X screen supports
 * FrameLock.  All of the other FrameLock attributes are only
 * applicable if NV_CTRL_FRAMELOCK is _SUPPORTED.
 */

#define NV_CTRL_FRAMELOCK                                       21 /* R-- */
#define NV_CTRL_FRAMELOCK_NOT_SUPPORTED                         0
#define NV_CTRL_FRAMELOCK_SUPPORTED                             1


/*
 * NV_CTRL_FRAMELOCK_MASTER - get/set whether this X screen is the
 * FrameLock master for the entire sync group.  Note that only one
 * node in the sync group should be configured as the master.
 */

#define NV_CTRL_FRAMELOCK_MASTER                                22 /* RW- */
#define NV_CTRL_FRAMELOCK_MASTER_FALSE                          0
#define NV_CTRL_FRAMELOCK_MASTER_TRUE                           1


/*
 * NV_CTRL_FRAMELOCK_POLARITY - sync either to the rising edge of the
 * framelock pulse, or both the rising and falling edges of the
 * framelock pulse.
 */

#define NV_CTRL_FRAMELOCK_POLARITY                              23 /* RW- */
#define NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE                  0x1
#define NV_CTRL_FRAMELOCK_POLARITY_FALLING_EDGE                 0x2
#define NV_CTRL_FRAMELOCK_POLARITY_BOTH_EDGES                   0x3


/*
 * NV_CTRL_FRAMELOCK_SYNC_DELAY - delay between the framelock pulse
 * and the GPU sync.  This is an 11 bit value which is multipled by
 * 7.81 to determine the sync delay in microseconds.
 */

#define NV_CTRL_FRAMELOCK_SYNC_DELAY                            24 /* RW- */
#define NV_CTRL_FRAMELOCK_SYNC_DELAY_MAX                        2047
#define NV_CTRL_FRAMELOCK_SYNC_DELAY_FACTOR                     7.81

/*
 * NV_CTRL_FRAMELOCK_SYNC_INTERVAL - how many house sync pulses
 * between the FrameLock sync generation (0 == sync every house sync);
 * this only applies to the master when receiving house sync.
 */

#define NV_CTRL_FRAMELOCK_SYNC_INTERVAL                         25 /* RW- */


/*
 * NV_CTRL_FRAMELOCK_PORT0_STATUS - status of the rj45 port0.
 */

#define NV_CTRL_FRAMELOCK_PORT0_STATUS                          26 /* R-- */
#define NV_CTRL_FRAMELOCK_PORT0_STATUS_INPUT                    0
#define NV_CTRL_FRAMELOCK_PORT0_STATUS_OUTPUT                   1


/*
 * NV_CTRL_FRAMELOCK_PORT1_STATUS - status of the rj45 port1.
 */

#define NV_CTRL_FRAMELOCK_PORT1_STATUS                          27 /* R-- */
#define NV_CTRL_FRAMELOCK_PORT1_STATUS_INPUT                    0
#define NV_CTRL_FRAMELOCK_PORT1_STATUS_OUTPUT                   1


/*
 * NV_CTRL_FRAMELOCK_HOUSE_STATUS - status of the house input (the BNC
 * connector).
 */

#define NV_CTRL_FRAMELOCK_HOUSE_STATUS                          28 /* R-- */
#define NV_CTRL_FRAMELOCK_HOUSE_STATUS_NOT_DETECTED             0
#define NV_CTRL_FRAMELOCK_HOUSE_STATUS_DETECTED                 1


/*
 * NV_CTRL_FRAMELOCK_SYNC - enable/disable the syncing of the
 * specified display devices to the FrameLock pulse.
 */

#define NV_CTRL_FRAMELOCK_SYNC                                  29 /* RWD */
#define NV_CTRL_FRAMELOCK_SYNC_DISABLE                          0
#define NV_CTRL_FRAMELOCK_SYNC_ENABLE                           1


/*
 * NV_CTRL_FRAMELOCK_SYNC_READY - reports whether a slave FrameLock
 * board is receiving sync (regardless of whether or not any display
 * devices are using the sync).
 */

#define NV_CTRL_FRAMELOCK_SYNC_READY                            30 /* R-- */
#define NV_CTRL_FRAMELOCK_SYNC_READY_FALSE                      0
#define NV_CTRL_FRAMELOCK_SYNC_READY_TRUE                       1


/*
 * NV_CTRL_FRAMELOCK_STEREO_SYNC - this indicates that the GPU stereo
 * signal is in sync with the framelock stereo signal.
 */

#define NV_CTRL_FRAMELOCK_STEREO_SYNC                           31 /* R-- */
#define NV_CTRL_FRAMELOCK_STEREO_SYNC_FALSE                     0
#define NV_CTRL_FRAMELOCK_STEREO_SYNC_TRUE                      1


/*
 * NV_CTRL_FRAMELOCK_TEST_SIGNAL - to test the connections in the sync
 * group, tell the master to enable a test signal, then query port[01]
 * status and sync_ready on all slaves.  When done, tell the master to
 * disable the test signal.  Test signal should only be manipulated
 * while NV_CTRL_FRAMELOCK_SYNC is enabled.
 *
 * The TEST_SIGNAL is also used to reset the Universal Frame Count (as
 * returned by the glXQueryFrameCountNV() function in the
 * GLX_NV_swap_group extension).  Note: for best accuracy of the
 * Universal Frame Count, it is recommended to toggle the TEST_SIGNAL
 * on and off after enabling FrameLock.
 */

#define NV_CTRL_FRAMELOCK_TEST_SIGNAL                           32 /* RW- */
#define NV_CTRL_FRAMELOCK_TEST_SIGNAL_DISABLE                   0
#define NV_CTRL_FRAMELOCK_TEST_SIGNAL_ENABLE                    1


/*
 * NV_CTRL_FRAMELOCK_ETHERNET_DETECTED - The FrameLock boards are
 * cabled together using regular cat5 cable, connecting to rj45 ports
 * on the backplane of the card.  There is some concern that users may
 * think these are ethernet ports and connect them to a
 * router/hub/etc.  The hardware can detect this and will shut off to
 * prevent damage (either to itself or to the router).
 * NV_CTRL_FRAMELOCK_ETHERNET_DETECTED may be called to find out if
 * ethernet is connected to one of the rj45 ports.  An appropriate
 * error message should then be displayed.  The _PORT0 and PORT1
 * values may be or'ed together.
 */

#define NV_CTRL_FRAMELOCK_ETHERNET_DETECTED                     33 /* R-- */
#define NV_CTRL_FRAMELOCK_ETHERNET_DETECTED_NONE                0
#define NV_CTRL_FRAMELOCK_ETHERNET_DETECTED_PORT0               0x1
#define NV_CTRL_FRAMELOCK_ETHERNET_DETECTED_PORT1               0x2


/*
 * NV_CTRL_FRAMELOCK_VIDEO_MODE - get/set the video mode of the house
 * input.
 */

#define NV_CTRL_FRAMELOCK_VIDEO_MODE                            34 /* RW- */
#define NV_CTRL_FRAMELOCK_VIDEO_MODE_NONE                       0
#define NV_CTRL_FRAMELOCK_VIDEO_MODE_TTL                        1
#define NV_CTRL_FRAMELOCK_VIDEO_MODE_NTSCPALSECAM               2
#define NV_CTRL_FRAMELOCK_VIDEO_MODE_HDTV                       3

/*
 * During FRAMELOCK bring-up, the above values were redefined to
 * these:
 */

#define NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_AUTO             0
#define NV_CTRL_FRAMELOCK_VIDEO_MODE_TTL                        1
#define NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_BI_LEVEL         2
#define NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_TRI_LEVEL        3


/*
 * NV_CTRL_FRAMELOCK_SYNC_RATE - this is the refresh rate that the
 * framelock board is sending to the GPU, in milliHz.
 */

#define NV_CTRL_FRAMELOCK_SYNC_RATE                             35 /* R-- */



/**************************************************************************/

/*
 * NV_CTRL_FORCE_GENERIC_CPU - inhibit the use of CPU specific
 * features such as MMX, SSE, or 3DNOW! for OpenGL clients; this
 * option may result in performance loss, but may be useful in
 * conjunction with software such as the Valgrind memory debugger.
 * This setting is only applied to OpenGL clients that are started
 * after this setting is applied.
 */

#define NV_CTRL_FORCE_GENERIC_CPU                               37 /* RW- */
#define NV_CTRL_FORCE_GENERIC_CPU_DISABLE                        0
#define NV_CTRL_FORCE_GENERIC_CPU_ENABLE                         1


/*
 * NV_CTRL_OPENGL_AA_LINE_GAMMA - for OpenGL clients, allow
 * Gamma-corrected antialiased lines to consider variances in the
 * color display capabilities of output devices when rendering smooth
 * lines.  Only available on recent Quadro GPUs.  This setting is only
 * applied to OpenGL clients that are started after this setting is
 * applied.
 */

#define NV_CTRL_OPENGL_AA_LINE_GAMMA                            38 /* RW- */
#define NV_CTRL_OPENGL_AA_LINE_GAMMA_DISABLE                     0
#define NV_CTRL_OPENGL_AA_LINE_GAMMA_ENABLE                      1


/*
 * NV_CTRL_FRAMELOCK_TIMING - this is TRUE when the framelock board is
 * receiving timing input.
 */

#define NV_CTRL_FRAMELOCK_TIMING                                39 /* RW- */
#define NV_CTRL_FRAMELOCK_TIMING_FALSE                           0
#define NV_CTRL_FRAMELOCK_TIMING_TRUE                            1

/*
 * NV_CTRL_FLIPPING_ALLOWED - when TRUE, OpenGL will swap by flipping
 * when possible; when FALSE, OpenGL will alway swap by blitting.  XXX
 * can this be enabled dynamically?
 */

#define NV_CTRL_FLIPPING_ALLOWED                                40 /* RW- */
#define NV_CTRL_FLIPPING_ALLOWED_FALSE                           0
#define NV_CTRL_FLIPPING_ALLOWED_TRUE                            1

/*
 * NV_CTRL_ARCHITECTURE - returns the architecture on which the X server is
 * running.
 */

#define NV_CTRL_ARCHITECTURE                                    41  /* R-- */
#define NV_CTRL_ARCHITECTURE_X86                                 0
#define NV_CTRL_ARCHITECTURE_X86_64                              1
#define NV_CTRL_ARCHITECTURE_IA64                                2


/*
 * NV_CTRL_TEXTURE_CLAMPING - texture clamping mode in OpenGL.  By
 * default, NVIDIA's OpenGL implementation uses CLAMP_TO_EDGE, which
 * is not strictly conformant, but some applications rely on the
 * non-conformant behavior, and not all GPUs support conformant
 * texture clamping in hardware.  _SPEC forces OpenGL texture clamping
 * to be conformant, but may introduce slower performance on older
 * GPUS, or incorrect texture clamping in certain applications.
 */

#define NV_CTRL_TEXTURE_CLAMPING                                42  /* RW- */
#define NV_CTRL_TEXTURE_CLAMPING_EDGE                            0
#define NV_CTRL_TEXTURE_CLAMPING_SPEC                            1



#define NV_CTRL_CURSOR_SHADOW                                   43  /* RW- */
#define NV_CTRL_CURSOR_SHADOW_DISABLE                            0
#define NV_CTRL_CURSOR_SHADOW_ENABLE                             1

#define NV_CTRL_CURSOR_SHADOW_ALPHA                             44  /* RW- */
#define NV_CTRL_CURSOR_SHADOW_RED                               45  /* RW- */
#define NV_CTRL_CURSOR_SHADOW_GREEN                             46  /* RW- */
#define NV_CTRL_CURSOR_SHADOW_BLUE                              47  /* RW- */

#define NV_CTRL_CURSOR_SHADOW_X_OFFSET                          48  /* RW- */
#define NV_CTRL_CURSOR_SHADOW_Y_OFFSET                          49  /* RW- */



/*
 * When Application Control for FSAA is enabled, then what the
 * application requests is used, and NV_CTRL_FSAA_MODE is ignored.  If
 * this is disabled, then any application setting is overridden with
 * NV_CTRL_FSAA_MODE
 */

#define NV_CTRL_FSAA_APPLICATION_CONTROLLED                     50  /* RW- */
#define NV_CTRL_FSAA_APPLICATION_CONTROLLED_ENABLED              1
#define NV_CTRL_FSAA_APPLICATION_CONTROLLED_DISABLED             0


/*
 * When Application Control for LogAniso is enabled, then what the
 * application requests is used, and NV_CTRL_LOG_ANISO is ignored.  If
 * this is disabled, then any application setting is overridden with
 * NV_CTRL_LOG_ANISO
 */

#define NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED                51  /* RW- */
#define NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED_ENABLED         1
#define NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED_DISABLED        0


/*
 * IMAGE_SHARPENING adjusts the sharpness of the display's image
 * quality by amplifying high frequency content.  Valid values will
 * normally be in the range [0,32).  Only available on GeForceFX or
 * newer.
 */

#define NV_CTRL_IMAGE_SHARPENING                                52  /* RWD */


/*
 * NV_CTRL_TV_OVERSCAN adjusts the amount of overscan on the specified
 * display device.
 */

#define NV_CTRL_TV_OVERSCAN                                     53  /* RWD */


/*
 * NV_CTRL_TV_FLICKER_FILTER adjusts the amount of flicker filter on
 * the specified display device.
 */

#define NV_CTRL_TV_FLICKER_FILTER                               54  /* RWD */


/*
 * NV_CTRL_TV_BRIGHTNESS adjusts the amount of brightness on the
 * specified display device.
 */

#define NV_CTRL_TV_BRIGHTNESS                                   55  /* RWD */


/*
 * NV_CTRL_TV_HUE adjusts the amount of hue on the specified display
 * device.
 */

#define NV_CTRL_TV_HUE                                          56  /* RWD */


/*
 * NV_CTRL_TV_CONTRAST adjusts the amount of contrast on the specified
 * display device.
 */

#define NV_CTRL_TV_CONTRAST                                     57  /* RWD */


/*
 * NV_CTRL_TV_SATURATION adjusts the amount of saturation on the
 * specified display device.
 */

#define NV_CTRL_TV_SATURATION                                   58  /* RWD */


/*
 * NV_CTRL_TV_RESET_SETTINGS - this write-only attribute can be used
 * to request that all TV Settings be reset to their default values;
 * typical usage would be that this attribute be sent, and then all
 * the TV attributes be queried to retrieve their new values.
 */

#define NV_CTRL_TV_RESET_SETTINGS                               59  /* -WD */


/*
 * NV_CTRL_GPU_CORE_TEMPERATURE reports the current core temperature
 * of the GPU driving the X screen.
 */

#define NV_CTRL_GPU_CORE_TEMPERATURE                            60  /* R-- */


/*
 * NV_CTRL_GPU_CORE_THRESHOLD reports the current GPU core slowdown
 * threshold temperature, NV_CTRL_GPU_DEFAULT_CORE_THRESHOLD and
 * NV_CTRL_GPU_MAX_CORE_THRESHOLD report the default and MAX core
 * slowdown threshold temperatures.
 *
 * NV_CTRL_GPU_CORE_THRESHOLD reflects the temperature at which the
 * GPU is throttled to prevent overheating.
 */

#define NV_CTRL_GPU_CORE_THRESHOLD                              61  /* R-- */
#define NV_CTRL_GPU_DEFAULT_CORE_THRESHOLD                      62  /* R-- */
#define NV_CTRL_GPU_MAX_CORE_THRESHOLD                          63  /* R-- */


/*
 * NV_CTRL_AMBIENT_TEMPERATURE reports the current temperature in the
 * immediate neighbourhood of the GPU driving the X screen.
 */

#define NV_CTRL_AMBIENT_TEMPERATURE                             64  /* R-- */


/*
 * NV_CTRL_PBUFFER_SCANOUT_SUPPORTED - returns whether this X screen
 * supports scanout of FP pbuffers;
 * 
 * if this screen does not support PBUFFER_SCANOUT, then all other
 * PBUFFER_SCANOUT attributes are unavailable.
 *
 * PBUFFER_SCANOUT is supported if and only if:
 * - Twinview is configured with clone mode.  The secondary screen is used to 
 *   scanout the pbuffer.  
 * - The desktop is running in with 16 bits per pixel.
 */
#define NV_CTRL_PBUFFER_SCANOUT_SUPPORTED                       65  /* R-- */
#define NV_CTRL_PBUFFER_SCANOUT_FALSE                           0
#define NV_CTRL_PBUFFER_SCANOUT_TRUE                            1

/*
 * NV_CTRL_PBUFFER_SCANOUT_XID indicates the XID of the pbuffer used for
 * scanout.
 */
#define NV_CTRL_PBUFFER_SCANOUT_XID                             66  /* RW- */

/**************************************************************************/
/*
 * The NV_CTRL_GVO_* integer attributes are used to configure GVO
 * (Graphics to Video Out).  This functionality is available, for
 * example, on the Quadro FX 4000 SDI graphics board.
 *
 * Here is a normal usage pattern for the GVO attributes:
 *
 * - query NV_CTRL_GVO_SUPPORTED to determine if the X screen supports GV0.
 *
 * - specify NV_CTRL_GVO_SYNC_MODE (one of FREE_RUNNING, GENLOCK, or
 *   FRAMELOCK)
 * 
 * - Use NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED and
 *   NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED to detect what input syncs are
 *   present.
 * 
 *         (If no analog sync is detected but it is known
 *           that a thought to be valid bi-level or tri-level
 *         sync is connected set
 *         NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE
 *         appropriately and retest with
 *         NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED).
 *
 * - specify NV_CTRL_GVO_SYNC_SOURCE, if appropriate
 *
 * - if syncing to input sync, query the NV_CTRL_GVO_INPUT_VIDEO_FORMAT
 * 
 * - query the valid NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT values; the valid
 *   values will be a function of what is specified for
 *   NV_CTRL_GVO_SYNC_MODE and what INPUT_VIDEO_FORMAT mode is
 *   detected.
 *
 * - specify the NV_CTRL_GVO_DATA_FORMAT
 *
 * - if using the GLX_NV_video_out extension to display one or more
 * pbuffers, call glXGetVideoDeviceNV() to lock the GVO output for use
 * by the GLX client; then bind the pbuffer(s) to the GVO output with
 * glXBindVideoImageNV() and send pbuffers to the GVO output with
 * glXSendPbufferToVideoNV(); see the GLX_NV_video_out spec for more
 * details.
 *
 * - if, rather than using the GLX_NV_video_out extension to display
 * GLX pbuffers on the GVO output, you wish display the X screen on
 * the GVO output, set NV_CTRL_GVO_DISPLAY_X_SCREEN to
 * NV_CTRL_GVO_DISPLAY_X_SCREEN_ENABLE.
 *
 * Note that setting most GVO attributes only causes the value to be
 * cached in the X server.  The values will be flushed to the hardware
 * either when NV_CTRL_GVO_DISPLAY_X_SCREEN is enabled, or when a GLX
 * pbuffer is bound to the GVO output (with glXBindVideoImageNV()).
 *
 * Note that GLX_NV_video_out and NV_CTRL_GVO_DISPLAY_X_SCREEN are
 * mutually exclusive.  If NV_CTRL_GVO_DISPLAY_X_SCREEN is enabled,
 * then glXGetVideoDeviceNV will fail.  Similarly, if a GLX client has
 * locked the GVO output (via glXGetVideoDeviceNV), then
 * NV_CTRL_GVO_DISPLAY_X_SCREEN will fail.  The NV_CTRL_GVO_LOCKED
 * event will be sent when a GLX client locks the GVO output.
 *
 */


/*
 * NV_CTRL_GVO_SUPPORTED - returns whether this X screen supports GVO;
 * if this screen does not support GVO output, then all other GVO
 * attributes are unavailable.
 */

#define NV_CTRL_GVO_SUPPORTED                                   67  /* R-- */
#define NV_CTRL_GVO_SUPPORTED_FALSE                             0
#define NV_CTRL_GVO_SUPPORTED_TRUE                              1


/*
 * NV_CTRL_GVO_SYNC_MODE - selects the GVO sync mode; possible values
 * are:
 *
 * FREE_RUNNING - GVO does not sync to any external signal
 *
 * GENLOCK - the GVO output is genlocked to an incoming sync signal;
 * genlocking locks at hsync.  This requires that the output video
 * format exactly match the incoming sync video format.
 *
 * FRAMELOCK - the GVO output is framelocked to an incoming sync
 * signal; framelocking locks at vsync.  This requires that the output
 * video format have the same refresh rate as the incoming sync video
 * format.
 */

#define NV_CTRL_GVO_SYNC_MODE                                   68  /* RW- */
#define NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING                      0
#define NV_CTRL_GVO_SYNC_MODE_GENLOCK                           1
#define NV_CTRL_GVO_SYNC_MODE_FRAMELOCK                         2


/*
 * NV_CTRL_GVO_SYNC_SOURCE - if NV_CTRL_GVO_SYNC_MODE is set to either
 * GENLOCK or FRAMELOCK, this controls which sync source is used as
 * the incoming sync signal (either Composite or SDI).  If
 * NV_CTRL_GVO_SYNC_MODE is FREE_RUNNING, this attribute has no
 * effect.
 */

#define NV_CTRL_GVO_SYNC_SOURCE                                 69  /* RW- */
#define NV_CTRL_GVO_SYNC_SOURCE_COMPOSITE                       0
#define NV_CTRL_GVO_SYNC_SOURCE_SDI                             1


/*
 * NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT - specifies the output video
 * format.  Note that the valid video formats will vary depending on
 * the NV_CTRL_GVO_SYNC_MODE and the incoming sync video format.  See
 * the definition of NV_CTRL_GVO_SYNC_MODE.
 */

#define NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT                         70  /* RW- */

#define NV_CTRL_GVO_VIDEO_FORMAT_NONE                           0
#define NV_CTRL_GVO_VIDEO_FORMAT_480I_59_94_SMPTE259_NTSC       1
#define NV_CTRL_GVO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL        2
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_59_94_SMPTE296            3
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_60_00_SMPTE296            4
#define NV_CTRL_GVO_VIDEO_FORMAT_1035I_59_94_SMPTE260           5
#define NV_CTRL_GVO_VIDEO_FORMAT_1035I_60_00_SMPTE260           6
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE295           7
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE274           8
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_59_94_SMPTE274           9
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_60_00_SMPTE274           10
#define NV_CTRL_GVO_VIDEO_FORMAT_1080P_23_976_SMPTE274          11
#define NV_CTRL_GVO_VIDEO_FORMAT_1080P_24_00_SMPTE274           12
#define NV_CTRL_GVO_VIDEO_FORMAT_1080P_25_00_SMPTE274           13
#define NV_CTRL_GVO_VIDEO_FORMAT_1080P_29_97_SMPTE274           14
#define NV_CTRL_GVO_VIDEO_FORMAT_1080P_30_00_SMPTE274           15
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_50_00_SMPTE296            16
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_24_00_SMPTE274           17 
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_23_98_SMPTE274           18
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_30_00_SMPTE296            19 
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_29_97_SMPTE296            20  
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_25_00_SMPTE296            21 
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_24_00_SMPTE296            22 
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_23_98_SMPTE296            23  


/*
 * NV_CTRL_GVO_INPUT_VIDEO_FORMAT - indicates the input video format
 * detected; the possible values are the NV_CTRL_GVO_VIDEO_FORMAT
 * constants.
 */

#define NV_CTRL_GVO_INPUT_VIDEO_FORMAT                          71  /* R-- */


/*
 * NV_CTRL_GVO_DATA_FORMAT - This controls how the data in the source
 * (either the X screen or the GLX pbuffer) is interpretted and
 * displayed.
 */

#define NV_CTRL_GVO_DATA_FORMAT                                 72  /* RW- */
#define NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB444              0
#define NV_CTRL_GVO_DATA_FORMAT_R8G8B8A8_TO_YCRCBA4444          1
#define NV_CTRL_GVO_DATA_FORMAT_R8G8B8Z10_TO_YCRCBZ4444         2
#define NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB422              3
#define NV_CTRL_GVO_DATA_FORMAT_R8G8B8A8_TO_YCRCBA4224          4
#define NV_CTRL_GVO_DATA_FORMAT_R8G8B8Z10_TO_YCRCBZ4224         5
#define NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_RGB444                6
#define NV_CTRL_GVO_DATA_FORMAT_R8G8B8A8_TO_RGBA4444            7
#define NV_CTRL_GVO_DATA_FORMAT_R8G8B8Z10_TO_RGBZ4444           8
#define NV_CTRL_GVO_DATA_FORMAT_Y10CR10CB10_TO_YCRCB444         9
#define NV_CTRL_GVO_DATA_FORMAT_Y10CR8CB8_TO_YCRCB444           10
#define NV_CTRL_GVO_DATA_FORMAT_Y10CR8CB8A10_TO_YCRCBA4444      11
#define NV_CTRL_GVO_DATA_FORMAT_Y10CR8CB8Z10_TO_YCRCBZ4444      12
#define NV_CTRL_GVO_DATA_FORMAT_DUAL_R8G8B8_TO_DUAL_YCRCB422    13
#define NV_CTRL_GVO_DATA_FORMAT_DUAL_Y8CR8CB8_TO_DUAL_YCRCB422  14


/*
 * NV_CTRL_GVO_DISPLAY_X_SCREEN - enable/disable GVO output of the X
 * screen.  At this point, all the GVO attributes that have been
 * cached in the X server are flushed to the hardware and GVO is
 * enabled.  Note that this attribute can fail to be set if a GLX
 * client has locked the GVO output (via glXGetVideoDeviceNV).  Note
 * that due to the inherit race conditions in this locking strategy,
 * NV_CTRL_GVO_DISPLAY_X_SCREEN can fail unexpectantly.  In the
 * failing situation, X will not return an X error.  Instead, you
 * should query the value of NV_CTRL_GVO_DISPLAY_X_SCREEN after
 * setting it to confirm that the setting was applied.
 */

#define NV_CTRL_GVO_DISPLAY_X_SCREEN                            73  /* RW- */
#define NV_CTRL_GVO_DISPLAY_X_SCREEN_ENABLE                     1
#define NV_CTRL_GVO_DISPLAY_X_SCREEN_DISABLE                    0


/*
 * NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED - indicates whether
 * Composite Sync input is detected.
 */

#define NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED               74  /* R-- */
#define NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED_FALSE         0
#define NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED_TRUE          1


/*
 * NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE - get/set the
 * Composite Sync input detect mode.  XXX is this needed?
 */

#define NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE            75  /* RW- */
#define NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_AUTO       0
#define NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_BI_LEVEL   1
#define NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_TRI_LEVEL  2


/*
 * NV_CTRL_GVO_SYNC_INPUT_DETECTED - indicates whether SDI Sync input
 * is detected, and what type.
 */

#define NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED                     76  /* R-- */
#define NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_NONE                0
#define NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_HD                  1
#define NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_SD                  2


/*
 * NV_CTRL_GVO_VIDEO_OUTPUTS - indicates which GVO video output
 * connectors are currently outputing data.
 */

#define NV_CTRL_GVO_VIDEO_OUTPUTS                               77  /* R-- */
#define NV_CTRL_GVO_VIDEO_OUTPUTS_NONE                          0
#define NV_CTRL_GVO_VIDEO_OUTPUTS_VIDEO1                        1
#define NV_CTRL_GVO_VIDEO_OUTPUTS_VIDEO2                        2
#define NV_CTRL_GVO_VIDEO_OUTPUTS_VIDEO_BOTH                    3


/*
 * NV_CTRL_GVO_FPGA_VERSION - indicates the version of the Firmware on
 * the GVO device.  XXX would this be better as a string attribute?
 */

#define NV_CTRL_GVO_FIRMWARE_VERSION                            78  /* R-- */


/*
 * NV_CTRL_GVO_SYNC_DELAY_PIXELS - controls the delay between the
 * input sync and the output sync in numbers of pixels from hsync;
 * this is a 12 bit value.
 */

#define NV_CTRL_GVO_SYNC_DELAY_PIXELS                           79  /* RW- */


/*
 * NV_CTRL_GVO_SYNC_DELAY_LINES - controls the delay between the input
 * sync and the output sync in numbers of lines from vsync; this is a
 * 12 bit value.
 */

#define NV_CTRL_GVO_SYNC_DELAY_LINES                            80  /* RW- */


/*
 * NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE - must be set for a period
 * of about 2 seconds for the new InputVideoFormat to be properly
 * locked to.  In nvidia-settings, we do a reacquire whenever genlock
 * or framelock mode is entered into, when the user clicks the
 * "detect" button.  This value can be written, but always reads back
 * _FALSE.
 */

#define NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE                81  /* -W- */
#define NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE_FALSE          0
#define NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE_TRUE           1


/*
 * NV_CTRL_GVO_LOCKED - indicates that GVO configurability is locked;
 * this occurs when the GLX_NV_video_out function calls
 * glXGetVideoDeviceNV().  All GVO output resources are locked until
 * either glXReleaseVideoDeviceNV() is called or the X Display used
 * when calling glXGetVideoDeviceNV() is closed.
 *
 * When GVO is locked; all GVO NV-CONTROL attributes are read only.
 */

#define NV_CTRL_GVO_GLX_LOCKED                                  82  /* R-- */
#define NV_CTRL_GVO_GLX_LOCKED_FALSE                            0
#define NV_CTRL_GVO_GLX_LOCKED_TRUE                             1



/*
 * NV_CTRL_GVO_VIDEO_FORMAT_{WIDTH,HEIGHT,REFRESH_RATE} - query the
 * width, height, and refresh rate for the specified
 * NV_CTRL_GVO_VIDEO_FORMAT_*.  So that this can be queried with
 * existing interfaces, XNVCTRLQueryAttribute() should be used, and
 * the video format specified in the display_mask field; eg:
 *
 * XNVCTRLQueryAttribute (dpy,
 *                        screen, 
 *                        NV_CTRL_GVO_VIDEO_FORMAT_480I_59_94_SMPTE259_NTSC
 *                        NV_CTRL_GVO_VIDEO_FORMAT_WIDTH,
 *                        &value);
 *
 * Note that Refresh Rate is in 1/1000 Hertz values
 */

#define NV_CTRL_GVO_VIDEO_FORMAT_WIDTH                          83  /* R-- */
#define NV_CTRL_GVO_VIDEO_FORMAT_HEIGHT                         84  /* R-- */
#define NV_CTRL_GVO_VIDEO_FORMAT_REFRESH_RATE                   85  /* R-- */


/*
 * NV_CTRL_GVO_X_SCREEN_PAN_[XY] - when GVO output of the X screen is
 * enabled, the pan x/y attributes control which portion of the X
 * screen is displayed by GVO.  These attributes can be updated while
 * GVO output is enabled, or before enabling GVO output.  The pan
 * values will be clamped so that GVO output is not panned beyond the
 * end of the X screen.
 */

#define NV_CTRL_GVO_X_SCREEN_PAN_X                              86  /* RW- */
#define NV_CTRL_GVO_X_SCREEN_PAN_Y                              87  /* RW- */


/*
 * XXX Still to do: GVO Color Conversion
 */

/*
 * XXX what sync error attributes do we need to expose?
 */


/**************************************************************************/


#define NV_CTRL_LAST_ATTRIBUTE NV_CTRL_GVO_X_SCREEN_PAN_Y




/**************************************************************************/

/*
 * String Attributes:
 */


/*
 * NV_CTRL_STRING_PRODUCT_NAME - the GPU product name on which the
 * specified X screen is running.
 */

#define NV_CTRL_STRING_PRODUCT_NAME                             0  /* R-- */


/*
 * NV_CTRL_STRING_VBIOS_VERSION - the video bios version on the GPU on
 * which the specified X screen is running.
 */

#define NV_CTRL_STRING_VBIOS_VERSION                            1  /* R-- */


/*
 * NV_CTRL_STRING_NVIDIA_DRIVER_VERSION - string representation of the
 * NVIDIA driver version number for the NVIDIA X driver in use.
 */

#define NV_CTRL_STRING_NVIDIA_DRIVER_VERSION                    3  /* R-- */


/*
 * NV_CTRL_STRING_DISPLAY_DEVICE_NAME - name of the display device
 * specified in the display_mask argument.
 */

#define NV_CTRL_STRING_DISPLAY_DEVICE_NAME                      4  /* R-D */


/*
 * NV_CTRL_STRING_TV_ENCODER_NAME - name of the TV encoder used by the
 * specified display device; only valid if the display device is a TV.
 */

#define NV_CTRL_STRING_TV_ENCODER_NAME                          5  /* R-D */
 
#define NV_CTRL_STRING_LAST_ATTRIBUTE NV_CTRL_STRING_TV_ENCODER_NAME



/**************************************************************************/
/*
 * CTRLAttributeValidValuesRec -
 *
 * structure and related defines used by
 * XNVCTRLQueryValidAttributeValues() to describe the valid values of
 * a particular attribute.  The type field will be one of:
 *
 * ATTRIBUTE_TYPE_INTEGER : the attribute is an integer value; there
 * is no fixed range of valid values.
 *
 * ATTRIBUTE_TYPE_BITMASK : the attribute is an integer value,
 * interpretted as a bitmask.
 *
 * ATTRIBUTE_TYPE_BOOL : the attribute is a boolean, valid values are
 * either 1 (on/true) or 0 (off/false).
 *
 * ATTRIBUTE_TYPE_RANGE : the attribute can have any integer value
 * between NVCTRLAttributeValidValues.u.range.min and
 * NVCTRLAttributeValidValues.u.range.max (inclusive).
 *
 * ATTRIBUTE_TYPE_INT_BITS : the attribute can only have certain
 * integer values, indicated by which bits in
 * NVCTRLAttributeValidValues.u.bits.ints are on (for example: if bit
 * 0 is on, then 0 is a valid value; if bit 5 is on, then 5 is a valid
 * value, etc).  This is useful for attributes like NV_CTRL_FSAA_MODE,
 * which can only have certain values, depending on GPU.
 *
 *
 * The permissions field of NVCTRLAttributeValidValuesRec is a bitmask
 * that may contain:
 *
 * ATTRIBUTE_TYPE_READ
 * ATTRIBUTE_TYPE_WRITE
 * ATTRIBUTE_TYPE_DISPLAY
 *
 * See 'Key to Integer Attribute "Permissions"' at the top of this
 * file for a description of what these three permission bits mean.
 */

#define ATTRIBUTE_TYPE_UNKNOWN   0
#define ATTRIBUTE_TYPE_INTEGER   1
#define ATTRIBUTE_TYPE_BITMASK   2
#define ATTRIBUTE_TYPE_BOOL      3
#define ATTRIBUTE_TYPE_RANGE     4
#define ATTRIBUTE_TYPE_INT_BITS  5

#define ATTRIBUTE_TYPE_READ      0x1
#define ATTRIBUTE_TYPE_WRITE     0x2
#define ATTRIBUTE_TYPE_DISPLAY   0x4

typedef struct _NVCTRLAttributeValidValues {
    int type;
    union {
        struct {
            int min;
            int max;
        } range;
        struct {
            unsigned int ints;
        } bits;
    } u;
    unsigned int permissions;
} NVCTRLAttributeValidValuesRec;



#define ATTRIBUTE_CHANGED_EVENT 0


#endif /* __NVCTRL_H */
