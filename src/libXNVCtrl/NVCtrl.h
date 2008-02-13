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
 *  (If no analog sync is detected but it is known that a
 *   valid bi-level or tri-level sync is connected set
 *   NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE appropriately and
 *   retest with NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED).
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
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_24_00_SMPTE274           17 //deprecated
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_48_00_SMPTE274           17
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_23_98_SMPTE274           18 //deprecated
#define NV_CTRL_GVO_VIDEO_FORMAT_1080I_47_96_SMPTE274           18
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_30_00_SMPTE296            19 
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_29_97_SMPTE296            20  
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_25_00_SMPTE296            21 
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_24_00_SMPTE296            22 
#define NV_CTRL_GVO_VIDEO_FORMAT_720P_23_98_SMPTE296            23  
#define NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_25_00_SMPTE274         24
#define NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_29_97_SMPTE274         25
#define NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_30_00_SMPTE274         26
#define NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_24_00_SMPTE274         27
#define NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_23_98_SMPTE274         28


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

/*
 * NV_CTRL_GPU_OVERCLOCKING_STATE - query the current or set a new
 * overclocking state; the value of this attribute controls the
 * availability of additional overclocking attributes (see below).
 *
 * Note: this attribute is unavailable unless overclocking support
 * has been enabled in the X server (by the user).
 */

#define NV_CTRL_GPU_OVERCLOCKING_STATE                          88  /* RW- */
#define NV_CTRL_GPU_OVERCLOCKING_STATE_NONE                     0
#define NV_CTRL_GPU_OVERCLOCKING_STATE_MANUAL                   1


/*
 * NV_CTRL_GPU_{2,3}D_CLOCK_FREQS - query or set the GPU and memory
 * clocks of the device driving the X screen.  New clock frequencies
 * are tested before being applied, and may be rejected.
 *
 * Note: if the target clocks are too aggressive, their testing may
 * render the system unresponsive.
 *
 * Note: while this attribute can always be queried, it can't be set
 * unless NV_CTRL_GPU_OVERCLOCKING_STATE is set to _MANUAL.  Since
 * the target clocks may be rejected, the requester should read this
 * attribute after the set to determine success or failure.
 */

#define NV_CTRL_GPU_2D_CLOCK_FREQS                              89  /* RW- */
#define NV_CTRL_GPU_3D_CLOCK_FREQS                              90  /* RW- */


/*
 * NV_CTRL_GPU_DEFAULT_{2,3}D_CLOCK_FREQS - query the default memory
 * and GPU core clocks of the device driving the X screen.
 */

#define NV_CTRL_GPU_DEFAULT_2D_CLOCK_FREQS                      91  /* R-- */
#define NV_CTRL_GPU_DEFAULT_3D_CLOCK_FREQS                      92  /* R-- */


/*
 * NV_CTRL_GPU_CURRENT_CLOCK_FREQS - query the current GPU and memory
 * clocks of the graphics device driving the X screen.
 */

#define NV_CTRL_GPU_CURRENT_CLOCK_FREQS                         93  /* R-- */


/*
 * NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS - Holds the last calculated
 * optimal 3D clock frequencies found by the
 * NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION process.  Querying this
 * attribute before having probed for the optimal clocks will return
 * NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_INVALID
 *
 * Note: unless NV_CTRL_GPU_OVERCLOCKING_STATE is set to _MANUAL, the
 * optimal clock detection process is unavailable.
 */

#define NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS                         94  /* R-- */
#define NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_INVALID                  0


/*
 * NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION - set to _START to
 * initiate testing for the optimal 3D clock frequencies.  Once
 * found, the optimal clock frequencies will be returned by the
 * NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS attribute asynchronously
 * (using an X event, see XNVCtrlSelectNotify).
 *
 * To cancel an ongoing test for the optimal clocks, set the
 * NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION attribute to _CANCEL
 *
 * Note: unless NV_CTRL_GPU_OVERCLOCKING_STATE is set to _MANUAL, the
 * optimal clock detection process is unavailable.
 */

#define NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION               95  /* -W- */
#define NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_START          0
#define NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_CANCEL         1


/*
 * NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE - query this
 * variable to know if a test is currently being run to
 * determine the optimal 3D clock frequencies.  _BUSY means a
 * test is currently running, _IDLE means the test is not running.
 *
 * Note: unless NV_CTRL_GPU_OVERCLOCKING_STATE is set to _MANUAL, the
 * optimal clock detection process is unavailable.
 */

#define NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE         96  /* R-- */
#define NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE_IDLE     0
#define NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE_BUSY     1



/*************************************************************************
 * DDC/CI VCP codes                                                      *
 * From the VESA Monitor Control Command Set (MCCS) Standard - Version 2 *
 *************************************************************************/
 
#define NV_CTRL_DDCCI_ON                                        1
#define NV_CTRL_DDCCI_OFF                                       0

/*************************************
 * DDC/CI VCP CODES: GEOMETRY ADJUST *
 *************************************/
 
/* Increasing (decreasing) this value moves the image toward the right
 * (left) side of the screen.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_HORIZONTAL_POSITION              97 /* RWD */

/* Increasing (decreasing) this value will increase (decrease) the width
 * of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_HORIZONTAL_SIZE                  98 /* RWD */

/* Increasing (decreasing) this value will cause the right and left sides
 * of the image to become more (less) convex.
 * Type: Range
 */  
#define NV_CTRL_DDCCI_GEOMETRY_HORIZONTAL_PINCUSHION            99 /* RWD */

/* Increasing (decreasing) this value will move the center section of the
 * image towards the right(left) side of the display.
 * Type: Range
 */  
#define NV_CTRL_DDCCI_GEOMETRY_HORIZONTAL_PINCUSHION_BALANCE    100/* RWD */

/* Increasing (decreasing) this value will shift the red pixels to the right
 * (left) across the image and the blue pixels left (right) across the image
 * with respect to the green pixels.
 * Type: Range
 */  
#define NV_CTRL_DDCCI_GEOMETRY_HORIZONTAL_CONVERGENCE           101/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * density of pixels in the image center.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_HORIZONTAL_LINEARITY             102/* RWD */

/* Increasing (decreasing) this value shifts the density of pixels from
 * the left (right) side to the right (left) side of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_HORIZONTAL_LINEARITY_BALANCE     103/* RWD */

/* Increasing (decreasing) this value moves the image toward the
 * top (bottom) edge of the display.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_VERTICAL_POSITION                104/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * height of the image
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_VERTICAL_SIZE                    105/* RWD */

/* Increasing (decreasing) this value will cause the top and bottom
 * edges of the image to become more (less) convex.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_VERTICAL_PINCUSHION              106/* RWD */

/* Increasing (decreasing) this value will move the center section of
 * the image toward the top (bottom) edge of the display.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_VERTICAL_PINCUSHION_BALANCE      107/* RWD */

/* Increasing (decreasing) this value shifts the red pixels up (down)
 * across the image and the blue pixels down (up) across the image
 * with respect to the green pixels.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_VERTICAL_CONVERGENCE             108/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * density of scan lines in the image center.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_VERTICAL_LINEARITY               109/* RWD */

/* Increasing (decreasing) this value shifts the density of scan lines
 * from the top (bottom) end to the bottom (top) end of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_VERTICAL_LINEARITY_BALANCE       110/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * degree of keystone distortion in the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_KEYSTONE                         111/* RWD */

/* Increasing (decreasing) this value shifts the top section of the
 * image to the right (left) with respect to the bottom section of the
 * image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_KEY_BALANCE                      112/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * degree of trapezoid distortion in the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_TRAPEZOID                        113/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * ratio between the horizontal size at the top of the image and the
 * horizontal size at the bottom of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_HORIZONTAL_TRAPEZOID             114/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * ratio between the vertical size at the left of the image and the
 * vertical size at the right of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_VERTICAL_TRAPEZOID               115/* RWD */

/* Increasing (decreasing) this value rotates the image (counter)
  clockwise about the center point of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_TILT                             116/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * distance between the left and right sides at the top of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_TOP_CORNER                       117/* RWD */

/* Increasing (decreasing) this value moves the top end of the
 * image to the right (left).
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_TOP_CORNER_BALANCE               118/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * distance between the left and right sides at the bottom of the
 * image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_BOTTOM_CORNER                    119/* RWD */

/* Increasing (decreasing) this value moves the bottom end of the
 * image to the right (left).
 * Type: Range
 */
#define NV_CTRL_DDCCI_GEOMETRY_BOTTOM_CORNER_BALANCE            120/* RWD */


/**************************************
 * DDC/CI VCP CODES: PRESET SELECTION *
 **************************************/

/* Restore all factory presets including brightness / contrast,
 * geometry, color and TV defaults.
 * NV__CTRL_DDCCI_ON causes defaults to be restored 
 * A value of NV_CTRL_DDCCI_OFF shall be ignored 
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_PRESET_RESTORE_DEFAULTS                   121/* -WD */

/* Restores factory defaults for brightness and contrast adjustments.
 * NV_CTRL_DDCCI_ON causes defaults to be restored 
 * A value of NV_CTRL_DDCCI_OFF shall be ignored 
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_PRESET_RESTORE_DEFAULTS_BRIGHTNESS_CONTRAST 122/* -WD */

/* Restore factory defaults for geometry adjustments.
 * NV_CTRL_DDCCI_ON causes defaults to be restored 
 * A value of NV_CTRL_DDCCI_OFF shall be ignored 
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_PRESET_RESTORE_DEFAULTS_GEOMETRY          123/* -WD */

/* Restore factory defaults for color settings.
 * NV_CTRL_DDCCI_ON causes defaults to be restored 
 * A value of NV_CTRL_DDCCI_OFF shall be ignored 
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_PRESET_RESTORE_DEFAULTS_COLOR             124/* -WD */

/* Restore factory defaults for TV functions.
 * NV_CTRL_DDCCI_ON causes defaults to be restored 
 * A value of NV_CTRL_DDCCI_OFF shall be ignored 
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_PRESET_RESTORE_DEFAULTS_TV                125/* -WD */

/* Store / Restore the user saved values for current mode.
 * - NV_CTRL_DDCCI_PRESET_SETTINGS_STORE_CURRENT:
 *   Store current settings in the monitor.
 * - NV_CTRL_DDCCI_PRESET_SETTINGS_RESTORE_FACTORY_DEFAULTS:
 *  Restore factory defaults for current mode. If no factory
 *  defaults then restore user values for current mode.
 * - All other values are reserved and shall be ignored.
 * Type: Integer
 */
#define NV_CTRL_DDCCI_PRESET_SETTINGS                           126/* -WD */
#define NV_CTRL_DDCCI_PRESET_SETTINGS_STORE_CURRENT             1
#define NV_CTRL_DDCCI_PRESET_SETTINGS_RESTORE_FACTORY_DEFAULTS  2


/***************************************
 * DDC/CI VCP CODES: IMAGE ADJUSTMENTS *
 ***************************************/

/* 
 * Increasing (decreasing) this value will increase (decrease) the
 * Brightness of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_BRIGHTNESS                          127/* RWD */

/* 
 * Increasing (decreasing) this value will increase (decrease) the
 * Contrast of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_CONTRAST                            128/* RWD */  

/* 
 * Turn on / off an auto setup function (periodic or event driven)
 * 0 and  3 : Shall be ignored
 * NV_CTRL_DDCCI_IMAGE_AUTO_SETUP_TOGGLE_OFF : Turn auto setup off
 * NV_CTRL_DDCCI_IMAGE_AUTO_SETUP_TOGGLE_ON : Turn auto setup on
 * Type: Integer
 */         
#define NV_CTRL_DDCCI_IMAGE_AUTO_SETUP_TOGGLE                   129/* -WD */
#define NV_CTRL_DDCCI_IMAGE_AUTO_SETUP_TOGGLE_OFF               1
#define NV_CTRL_DDCCI_IMAGE_AUTO_SETUP_TOGGLE_ON                2

/* 
 * Perform autosetup function (H/V position, clock, clock phase, A/D
 * converter, etc)
 * NV_CTRL_OFF : Indicates that auto-setup is not active
 * NV_CTRL_ON : Perform autosetup
 * >=2 : Shall be ignored
 * Type: Integer
 */         
#define NV_CTRL_DDCCI_IMAGE_AUTO_SETUP                          130/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * sampling clock frequency 
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_CLOCK                               131/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * phase shift of the sampling clock
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_CLOCK_PHASE                         132/* RWD */

/* Allows the display to specify the minimum increment in which it
 * can adjust the color temperature.
 * This will be used in conjunction with 
 * NV_CTRL_DDCCI_IMAGE_COLOR_TEMPERATURE_REQUEST, Color temperature request.
 * Values of 0 and > 5000 are invalid and shall be ignored.
 * Type: Integer
 */
#define NV_CTRL_DDCCI_IMAGE_COLOR_TEMPERATURE_INCREMENT         133/* R-D */

/* Allows a specified color temperature (in °K) to be requested. If
 * display is unable to achieve requested color temperature, then it
 * should move to the closest possible temperature.
 * A value of 0 shall be treated as a request for a color temperature
 * of 3000°K. Values greater than 0 shall be used as a multiplier of
 * the color temperature increment (read using 
 * NV_CTRL_DDCCI_IMAGE_COLOR_TEMPERATURE_INCREMENT) and the
 * result added to the base value of 3000°K
 * Example:
 * If NV_CTRL_DDCCI_IMAGE_COLOR_TEMPERATURE_INCREMENT returns a value of 
 * 50°K and NV_CTRL_DDCCI_IMAGE_COLOR_TEMPERATURE_INCREMENT sends a
 * value of 50 (decimal) then the display shall interpret this as a
 * request to adjust the color temperature to 5500°K
 * (3000 + (50 * 50))°K = 5500°K
 * Notes:
 * 1) Applications using this function are recommended to read the
 * actual color temperature after using this command and taking
 * appropriate action.
 * 2) This control is only recommended if the display can produce a
 * continuously (at defined increment, see VCP code 0Bh) variable
 * color temperature.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_COLOR_TEMPERATURE_REQUEST           134/* RWD */

/* Select a specified color temperature.
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_SRGB : sRGB
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_NATIVE : Display native
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_4000K :  4000 K
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_5000K :  5000 K
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_6500K :  6500 K
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_7500K :  7500 K
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_8200K :  8200 K
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_9300K :  9300 K
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_10000K : 10000 K
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_USER1 :  11500 K
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_USER1 :  User 1
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_USER2 :  User 2
 * NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_USER3 :  User 3
 * 00 and >=0E shall be ignored
 * Type: Integer
 */
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET                 135/* RWD */
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_SRGB            0x01
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_NATIVE          0x02
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_4000K           0x03
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_5000K           0x04
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_6500K           0x05
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_7500K           0x06
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_8200K           0x07
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_9300K           0x08
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_10000K          0x09
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_11500K          0x0A
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_USER1           0x0B
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_USER2           0x0C
#define NV_CTRL_DDCCI_IMAGE_SELECT_COLOR_PRESET_USER3           0x0D

/*
 * Increasing (decreasing) this value will increase (decrease) the
 * luminance of red pixels.
 * The value returned shall be an indication of the actual red gain at
 * the current color temperature and not be normalized.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_RED_VIDEO_GAIN                      136/* RWD */

/*
 * Increasing (decreasing) this value will increase (decrease) the
 * luminance of green pixels.
 * The value returned shall be an indication of the actual green gain at
 * the current color temperature and not be normalized.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_GREEN_VIDEO_GAIN                    137/* RWD */

/*
 * Increasing (decreasing) this value will increase (decrease) the
 * luminance of blue pixels.
 * The value returned shall be an indication of the actual blue gain at
 * the current color temperature and not be normalized.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_BLUE_VIDEO_GAIN                     138/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * black level of red pixels.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_RED_VIDEO_BLACK_LEVEL               139/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * black level of green pixels.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_GREEN_VIDEO_BLACK_LEVEL             140/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * black level of blue pixels.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_BLUE_VIDEO_BLACK_LEVEL              141/* RWD */

/* Increasing (decreasing) this value will adjust the focus of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_FOCUS                               142/* RWD */

/* Increasing (decreasing) this value controls the horizontal picture
 * moiré cancellation.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_HORIZONTAL_MOIRE                    143/* RWD */

/* Increasing (decreasing) this value controls the vertical picture
 * moiré cancellation.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_VERTICAL_MOIRE                      144/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * distance to the focal plane of the image.
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_ADJUST_FOCAL_PLANE                  145/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * zoom function of the optics
 * Type: Range
 */
#define NV_CTRL_DDCCI_IMAGE_ADJUST_ZOOM                         146/* RWD */

/* This value will cause the picture to flip horizontally.
 * NV_CTRL_DDCCI_ON : flip horizontally
 * NV_CTRL_DDCCI_OFF : Shall be ignored
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_IMAGE_HOR_FLIP                            147/* -WD */

/* This value will cause the picture to flip vertically.
 * NV_CTRL_DDCCI_ON : flip vertically
 * NV_CTRL_DDCCI_OFF : Shall be ignored
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_IMAGE_VER_FLIP                            148/* -WD */

/* Changing this value will affect the scaling (input versus output)
 * function of the display.
 * NV_CTRL_DDCCI_IMAGE_DISPLAY_SCALING_NO_SCALING : No scaling, 1:1 relationship
 * NV_CTRL_DDCCI_IMAGE_DISPLAY_SCALING_MAX_NO_DISTORT2 : Scale to maximum 
 *   without AR distortion
 * NV_CTRL_DDCCI_IMAGE_DISPLAY_SCALING_MAX_VERTICAL : Scale to maximum vertical 
     size
 * NV_CTRL_DDCCI_IMAGE_DISPLAY_SCALING_MAX_HORIZONTAL ; Scale to maximum 
     horizontal size
 * 0 and >=5 : Shall be ignored
 * Type: Integer
 */
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_SCALING                     149/* RWD */
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_SCALING_NO_SCALING          1
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_SCALING_MAX_NO_DISTORT      2
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_SCALING_MAX_VERTICAL        3
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_SCALING_MAX_HORIZONTAL      4

/* Allows one of several image expansion algorithms to be selecte
 * to suit different types of image and/or different personal
 * preference.
 * NV_CTRL_DDCCI_IMAGE_SHARPNESS_FILTER_FUNC(1) : Filter function 1
 * NV_CTRL_DDCCI_IMAGE_SHARPNESS_FILTER_FUNC(2) : Filter function 2
 * NV_CTRL_DDCCI_IMAGE_SHARPNESS_FILTER_FUNC(3) : Filter function 3
 * NV_CTRL_DDCCI_IMAGE_SHARPNESS_FILTER_FUNC(4) : Filter function 4
 * 0 and  5 shall be ignored
 * Type: Integer
 */
#define NV_CTRL_DDCCI_IMAGE_SHARPNESS                           150/* RWD */
#define NV_CTRL_DDCCI_IMAGE_SHARPNESS_FILTER_FUNC(n)            n

/* Indicates the orientation of the screen.
 * NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_0        
 *      The normal landscape mode.
 * NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_90   
 *      Portrait mode achieved by clockwise rotation of the display 90 degrees.
 * NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_180
 *      Landscape mode achieved by rotation of the display 180 degrees.
 * NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_270
 *      Portrait mode achieved by clockwise rotation of the display 270 degrees.
 * NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_NA 
 *      Indicates that the display cannot supply the current orientation
 * Type: Integer
 */
#define NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION                  151/* R-D */
#define NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_0                0x01
#define NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_90               0x02
#define NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_180              0x03
#define NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_270              0x04
#define NV_CTRL_DDCCI_IMAGE_SCREEN_ORIENTATION_NA               0xFF

/* Selects a scan format.
 * NV_CTRL_DDCCI_IMAGE_SCAN_FORMAT_UNDERSCAN: Underscan
 * NV_CTRL_DDCCI_IMAGE_SCAN_FORMAT_OVERSCAN: Overscan
 * NV_CTRL_DDCCI_IMAGE_SCAN_FORMAT_WIDESCREEN: Widescreen
 * Type: Integer
 */
#define NV_CTRL_DDCCI_IMAGE_SCAN_FORMAT                         152/* RWD */
#define NV_CTRL_DDCCI_IMAGE_SCAN_FORMAT_UNDERSCAN               1
#define NV_CTRL_DDCCI_IMAGE_SCAN_FORMAT_OVERSCAN                2
#define NV_CTRL_DDCCI_IMAGE_SCAN_FORMAT_WIDESCREEN              3
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_MODE                        153/* RWD */

/* Display mode:
 * NV_CTRL_DDCCI_IMAGE_DISPLAY_MODE_PRODUCTIVITY : 
 *      Productivity (e.g. office applications)
 * NV_CTRL_DDCCI_IMAGE_DISPLAY_MODE_MIXED : Mixed (e.g. internet)
 * NV_CTRL_DDCCI_IMAGE_DISPLAY_MODE_ENTERTAINMENT : 
        Entertainment (e.g. TV / movie)
 * NV_CTRL_DDCCI_IMAGE_DISPLAY_MODE_USERDEFINED : User defined
 * Note:
 * The condition(s) associated with options 1, 2 and 3 are defined
 * by the display manufacturer
 * Type: Integer
 */
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_MODE_PRODUCTIVITY           1
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_MODE_MIXED                  2
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_MODE_ENTERTAINMENT          3
#define NV_CTRL_DDCCI_IMAGE_DISPLAY_MODE_USERDEFINED            4

/***********************************************
 * DDC/CI VCP CODES: WINDOWED IMAGE OPERATIONS *
 ***********************************************/

/* Defines the top left X pixel of an area of the image. Specified in
 * co-ordinates of incoming image before any scaling etc in the display.
 * Type: Range
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_POSITION_TOP_LEFT_X         154/* RWD */ 

/* Defines the top left Y pixel of an area of the image. Specified in
 * co-ordinates of incoming image before any scaling etc in the display.
 * Type: Range
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_POSITION_TOP_LEFT_Y         155/* RWD */

/* Defines the bottom right X pixel of an area of the image.
 * Specified in co-ordinates of incoming image before any scaling
 * in the display.
 * Type: Range
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_POSITION_BOTTOM_RIGHT_X     156/* RWD */

/* Defines the bottom right Y pixel of an area of the image.
 * Specified in co-ordinates of incoming image before any scaling
 * in the display.
 * Type: Range
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_POSITION_BOTTOM_LEFT_X      157/* RWD */

/* Window control, enables the brightness and color within a
 * window to be different from the desktop.
 * NV_CTRL_DDCCI_WINDOWEDIMAGE_OLD_CONTROL_TOGGLE_OFF : Off
 * NV_CTRL_DDCCI_WINDOWEDIMAGE_OLD_CONTROL_TOGGLE_ON: On
 * All other values are reserved.
 * Notes:
 * 1. This control should be used in conjunction with
 *      NV_CTRL_DDCCI_WINDOWEDIMAGE_BACKGROUND
 * 2. This command structure is not recommended for new designs,
 * see NV_CTRL_DDCCI_WINDOWEDIMAGE_CONTROL_TOGGLE for alternate.
 * Type: Integer
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_OLD_CONTROL_TOGGLE          158/* RWD */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_OLD_CONTROL_TOGGLE_OFF      1
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_OLD_CONTROL_TOGGLE_ON       2

/* Changes the contrast ratio between the area of the window and
 * the rest of the desktop
 * Lower (higher) values will cause the desktop brightness to
 * decrease (increase)
 * Notes:
 * 1. This contropl should be used in conjunction with 
 *  NV_CTRL_DDCCI_WINDOWEDIMAGE_OLD_CONTROL_TOGGLE
 * 2. This command structure is not recommended for new designs,
 * see NV_CTRL_DDCCI_WINDOWEDIMAGE_WINDOW_SELECT for alternate.
 * Type: Range
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_BACKGROUND                  159/* RWD */

/* A value of 127 shall have no effect.
 * A value > 127 shall cause the color to shift towards yellow
 * A value < 127 shall cause the color to shift towards magenta
 * Type: Range 
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_6AXIS_COLOR_CONTROL_RED     160/* RWD */

/* A value of 127 shall have no effect.
 * A value > 127 shall cause the color to shift towards green
 * A value < 127 shall cause the color to shift towards red
 * Type: Range 
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_6AXIS_COLOR_CONTROL_YELLOW  161/* RWD */

/* A value of 127 shall have no effect.
 * A value > 127 shall cause the color to shift towards cyan
 * A value < 127 shall cause the color to shift towards yellow
 * Type: Range 
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_6AXIS_COLOR_CONTROL_GREEN   162/* RWD */

/* A value of 127 shall have no effect.
 * A value > 127 shall cause the color to shift towards blue
 * A value < 127 shall cause the color to shift towards green
 * Type: Range 
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_6AXIS_COLOR_CONTROL_CYAN    163/* RWD */

/* A value of 127 shall have no effect.
 * A value > 127 shall cause the color to shift towards magenta
 * A value < 127 shall cause the color to shift towards cyan
 * Type: Range 
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_6AXIS_COLOR_CONTROL_BLUE    164/* RWD */

/* A value of 127 shall have no effect.
 * A value > 127 shall cause the color to shift towards red
 * A value < 127 shall cause the color to shift towards blue
 * Type: Range 
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_6AXIS_COLOR_CONTROL_MAGENTA 165/* RWD */

/* Turn the selected window operation on / off.
 * Bit: NV_CTRL_DDCCI_WINDOWEDIMAGE_CONTROL_TOGGLE_DPY_IMAGE
 *         0      Window controls have no effect on the
 *                displayed image
 *         1      Window controls effect the displayed image
 *                (full image area)
 *  Bit: NV_CTRL_DDCCI_WINDOWEDIMAGE_CONTROL_TOGGLE_WINDOW(n)
 *         0      Window controls have no effect on the
 *                displayed image (window n)
 *         1      Window controls effect the displayed image
 *                (window n)
 *  n is between 1 and 7
 * Note: This command structure is recommend, in conjunction with
 * NV_CTRL_DDCCI_WINDOWEDIMAGE_WINDOW_SELECT, for all new designs.
 * Type: Bitmask
 */ 
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_CONTROL_TOGGLE              166/* RWD */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_CONTROL_TOGGLE_DPY_IMAGE    1
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_CONTROL_TOGGLE_WINDOW(n)    (1<<n)

/* Change the selected window as defined with 
 * NV_CTRL_DDCCI_WINDOWEDIMAGE_CONTROL_TOGGLE
 * When a window is selected then all commands that the display
 * controller supports for window operations are valid, this may
 * include but is not limited to: brightness, contrast, R/G/B gain, 6-
 * axis color, sharpness, etc.
 * meaning of value NV_CTRL_DDCCI_WINDOWEDIMAGE_WINDOW_SELECT_DPY_IMAGE:
 *           Full display image area is selected except for
 *           area(s) of active windows
 * meaning of value NV_CTRL_DDCCI_WINDOWEDIMAGE_WINDOW_SELECT_WINDOW(n)
 *           Window n is selected
 * If this command is not supported then the `full image area' shall
 * be the default.
 * This command structure is recommend, in conjunction with 
 * NV_CTRL_DDCCI_WINDOWEDIMAGE_CONTROL_TOGGLE, for all new designs.
 * Type: Integer
 */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_WINDOW_SELECT               167/* RWD */
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_WINDOW_SELECT_DPY_IMAGE     0
#define NV_CTRL_DDCCI_WINDOWEDIMAGE_WINDOW_SELECT_WINDOW(n)     (1<<n)


/*************************************/
/* DDC/CI VCP CODES: AUDIO FUNCTIONS */
/*************************************/

/* Increasing (decreasing) this value will increase (decrease) the
 * audio volume.
 * Type: Range
 */
#define NV_CTRL_DDCCI_AUDIO_SPEAKER_VOLUME                      168/* RWD */

/* Increasing (decreasing) this value will increase (decrease) the
 * microphone gain.
 * Type: Range
 */
#define NV_CTRL_DDCCI_AUDIO_MICROPHONE_VOLUME                   169/* RWD */

/* Increasing (decreasing) this control will cause high frequency
 * audio to be emphasized (de-emphasised).
 * Type: Range
 */
#define NV_CTRL_DDCCI_AUDIO_TV_AUDIO_TREBLE                     170/* RWD */

/* Increasing (decreasing) this control will cause low frequency
 * audio to be emphasized (de-emphasised).
 * Type: Range
 */
#define NV_CTRL_DDCCI_AUDIO_TV_AUDIO_BASS                       171/* RWD */

/* This control affects the left ­ right balance of audio output.
 * Increasing (decreasing) the value will cause the balance to move
 * to the right (left).
 * Type: Range
 */
#define NV_CTRL_DDCCI_AUDIO_TV_AUDIO_BALANCE                    172/* RWD */

/* This control allows one of several audio modes to be selected.
 * NV_CTRL_DDCCI_AUDIO_TV_STEREO_MODE_SPEAKER_OFF: Display audio is disabled,
 * NV_CTRL_DDCCI_AUDIO_TV_STEREO_MODE_SPEAKER_MONO: Both display audio channels,
 *  use the left audio channel
 * NV_CTRL_DDCCI_AUDIO_TV_STEREO_MODE_SPEAKER_STEREO: Incoming left and right 
 *  audio. Channels feed separate display output audio channels.
 * NV_CTRL_DDCCI_AUDIO_TV_STEREO_MODE_SPEAKER_STEREO_EXPANDED: As defined 
 *  by the manufacturer.
 * Type: Integer
 */
#define NV_CTRL_DDCCI_AUDIO_TV_STEREO_MODE                      173/* RWD */
#define NV_CTRL_DDCCI_AUDIO_TV_STEREO_MODE_SPEAKER_OFF          0
#define NV_CTRL_DDCCI_AUDIO_TV_STEREO_MODE_SPEAKER_MONO         1
#define NV_CTRL_DDCCI_AUDIO_TV_STEREO_MODE_SPEAKER_STEREO       2
#define NV_CTRL_DDCCI_AUDIO_TV_STEREO_MODE_SPEAKER_STEREO_EXPANDED 3


/************************************/
/* DDC/CI VCP CODES: DPVL Functions */
/************************************/

/* Video mode and status of a DPVL capable monitor
 * Bit   Value                          Meaning
 *  0      0      Raster scan mode
 *         1      DPVL mode
 *  1      0      Monitor is able to receive the next packet
 *         1      Monitor is unable to accept another packet
 *  2      0      No error detected in the last header received
 *         1      Error detected in the last header received
 * Other bits are reserved and shall be set to '0'.
 * Type: Bitmask
 */
#define NV_CTRL_DDCCI_DPVL_MONITOR_STATUS                       174/* R-D */ 
#define NV_CTRL_DDCCI_DPVL_MONITOR_STATUS_DPVL_MODE                     (1<<0)
#define NV_CTRL_DDCCI_DPVL_MONITOR_STATUS_UNABLE_TO_RECEIVE_NEXT_PACKET (1<<1)
#define NV_CTRL_DDCCI_DPVL_MONITOR_STATUS_ERROR_DETECTED_IN_LAST_HEADER (1<<2)

/* Counter for the DPVL packets received (valid and invalid ones).
 * This value counts from 0000h to FFFFh and then rolls over to 0000h.
 * The host can reset the value to 0000h.
 * Type: Range
 */
#define NV_CTRL_DDCCI_DPVL_PACKET_COUNT                         175/* RWD */

/* The X origin of the monitor in the virtual screen. The support of
 * this command indicates the multi-display support of the display. If
 * a display supports this command, the monitor must also support
 * Monitor Y Origin command.
 * Type: Range; "0000h" to "FFFFh" or 0 to 65535
 */
#define NV_CTRL_DDCCI_DPVL_MONITOR_X_ORIGIN                     176/* RWD */

/* The Y origin of the display in the virtual screen. The support of
 * this command indicates the multi-display support of the display. If
 * a display supports this command, the monitor must also support
 * Monitor X Origin command.
 * Type: Range; "0000h" to "FFFFh" or 0 to 65535
 */
#define NV_CTRL_DDCCI_DPVL_MONITOR_Y_ORIGIN                     177/* RWD */

/* Error Counter for the DPVL header. The counter value saturates
 * at FFFFh. Host can reset to 0000h.
 * Type: Range
 */
#define NV_CTRL_DDCCI_DPVL_HEADER_ERROR_COUNT                   178/* RWD */

/* CRC error Counter for the DPVL body (containing video data).
 * The counter value saturates at FFFFh. The Host can reset to 0000h
 * Type: Range
 */
#define NV_CTRL_DDCCI_DPVL_BODY_CRC_ERROR_COUNT                 179/* RWD */

/* Assigned identification number for the monitor.
 * Type: Valid range is 0000h to FFFEh, FFFFh is reserved for broadcast.
 */
#define NV_CTRL_DDCCI_DPVL_CLIENT_ID                            180/* RWD */

/* Indicates the status of the DVI link
 * NV_CTRL_DDCCI_DPVL_LINK_CONTROL_DISABLED: Link shutdown is disabled
 * NV_CTRL_DDCCI_DPVL_LINK_CONTROL_ENABLED: Link shutdown is enabled
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_DPVL_LINK_CONTROL                         181/* RWD */
#define NV_CTRL_DDCCI_DPVL_LINK_CONTROL_DISABLED                0
#define NV_CTRL_DDCCI_DPVL_LINK_CONTROL_ENABLED                 1

/**********************************************/
/* DDC/CI VCP CODES:  Miscellaneous Functions */
/**********************************************/

/* Causes a CRT display to perform a degauss cycle with
 * NV_CTRL_DDCCI_ON
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_MISC_DEGAUSS                              182/* -WD */

/* Used to indicate that a display user control (excluding power
 * controls) has been used to change and save (or autosave) a new value.
 * NV_CTRL_DDCCI_MISC_NEW_CONTROL_VALUE_NO_NEW : No new control value
 * NV_CTRL_DDCCI_MISC_NEW_CONTROL_VALUE_NO_USER_CONTROLS : A new control value 
 *  has been saved
 * NV_CTRL_DDCCI_MISC_NEW_CONTROL_VALUE_NO_USER_CONTROLS : No user controls are present
 * Note:
 * A value = NV_CTRL_DDCCI_MISC_NEW_CONTROL_VALUE_NEW should only be reset to 
 * a value = NV_CTRL_DDCCI_MISC_NEW_CONTROL_VALUE_NO_NEW by a host write
 * operation and not by the display
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_NEW_CONTROL_VALUE                    183/* RWD */
#define NV_CTRL_DDCCI_MISC_NEW_CONTROL_VALUE_NO_NEW             0x01
#define NV_CTRL_DDCCI_MISC_NEW_CONTROL_VALUE_NEW                0x02
#define NV_CTRL_DDCCI_MISC_NEW_CONTROL_VALUE_NO_USER_CONTROLS   0xFF

/* Allows display controls to be used as soft keys
 * NV_CTRL_DDCCI_MISC_SOFT_CONTROLS_NO_BUTTON_ACTIVE : No button active
 * NV_CTRL_DDCCI_MISC_SOFT_CONTROLS_BUTTON(n): Button n active
 * n is between 1 and 7.
 * NV_CTRL_DDCCI_MISC_SOFT_CONTROLS_NONE : No controls present
 * Note:
 * A `button active' value should only be reset to a value = 0 by
 * host write operation and not by the display
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_SOFT_CONTROLS                        184/* RWD */
#define NV_CTRL_DDCCI_MISC_SOFT_CONTROLS_NO_BUTTON_ACTIVE       0
#define NV_CTRL_DDCCI_MISC_SOFT_CONTROLS_BUTTON(n)              n
#define NV_CTRL_DDCCI_MISC_SOFT_CONTROLS_NONE                   0xFF

/* When read this control returns the VCP code associated with
 * the new control value reported using 02H
 * Examples:
 * If brightness has been changed then return value of 10h
 * If red gain has been changed then return value of 16h.
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_ACTIVE_CONTROL                       185/* R-D */

/* Used to select the active video source
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_ANALOG_1: Analog video (R/G/B) 1
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_ANALOG_2: Analog video (R/G/B) 2
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_DIGITAL_1: Digital video (TMDS) 1
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_DIGITAL_2: Digital video (TMDS) 2
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPOSITE_1: Composite video 1
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPOSITE_2: Composite video 2
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_SVIDEO_1: S-video 1
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_SVIDEO_2: S-video 2
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_TUNER_1: Tuner 1
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_TUNER_2: Tuner 2
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_TUNER_3: Tuner 3
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPONENT_1: Component video (YPrPb/YCrCb) 1
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPONENT_2: Component video (YPrPb/YCrCb) 2
 * NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPONENT_3: Component video (YPrPb/YCrCb) 3
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE                         186/* RWD */
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_ANALOG_1                1
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_ANALOG_2                2
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_DIGITAL_1               3
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_DIGITAL_2               4
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPOSITE_1             5
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPOSITE_2             6
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_SVIDEO_1                7
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_SVIDEO_2                8
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_TUNER_1                 9
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_TUNER_2                 10
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_TUNER_3                 11
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPONENT_1             12
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPONENT_2             13
#define NV_CTRL_DDCCI_MISC_INPUT_SOURCE_COMPONENT_3             14

/* ncreasing (decreasing) this value will increase (decrease) the
 * velocity modulation of the horizontal scan as a function of a
 * change in the luminance level.
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_VELOCITY_SCAN_MODULATION             187/* RWD */

/* Increasing this control increases the amplitude of the color
 * difference components of the video signal.
 * The result is an increase in the amount of pure color relative to
 * white in the video. This control does not affect the RGB input,
 * only the TV video inputs.
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_TV_COLOR_SATURATION                  188/* RWD */

/* Used to increment / decrement between TV-channels, the exact
 * behavior is implementation specific (e.g. increment / decrement
 * to next numeric channel or increment / decrement to next
 * channel with a signal)
 * NV_CTRL_DDCCI_MISC_TV_CHANNEL_UP_DOWN_INCREMENT: Increment channel
 * NV_CTRL_DDCCI_MISC_TV_CHANNEL_UP_DOWN_DECREMENT: Decrement channel
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_TV_CHANNEL_UP_DOWN                   189/* -WD */
#define NV_CTRL_DDCCI_MISC_TV_CHANNEL_UP_DOWN_INCREMENT         1
#define NV_CTRL_DDCCI_MISC_TV_CHANNEL_UP_DOWN_DECREMENT         2

/* Increasing this control increases the amplitude of the high
 * frequency components of the video signal.
 * This allows fine details to be accentuated. This control does not
 * affect the RGB input, only the TV video inputs.
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_TV_SHARPNESS                         190/* RWD */

/* Provides for the TV-audio to be muted or unmated.
 * NV_CTRL_DDCCI_MISC_TV_AUDIO_MUTE_ON: Mute the audio
 * NV_CTRL_DDCCI_MISC_TV_AUDIO_MUTE_OFF: Unmute the audio
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_TV_AUDIO_MUTE                        191/* RWD */
#define NV_CTRL_DDCCI_MISC_TV_AUDIO_MUTE_ON                     1
#define NV_CTRL_DDCCI_MISC_TV_AUDIO_MUTE_OFF                    2

/* Increasing (decreasing) this control increases (decreases) the
 * ratio between whites and blacks in the video.
 * This control does not affect the RGB input, only the TV video
 * inputs.
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_TV_CONTRAST                          192/* RWD */

/* Also known as `tint'
 * Increasing (decreasing) this control increases (decreases) the
 * wavelength of the color component of the video signal.
 * The result is a shift towards red (blue) in the hue of all colors.
 * This control does not affect the RGB input, only the TV video
 * inputs.
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_TV_HUE                               193/* RWD */

/* Increasing this control increases the black level of the video,
 * resulting in an increase of the luminance level of the video.
 * A value of zero represents the darkest level possible.
 * This control does not affect the RGB input, only the TV video
 * inputs.
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_TV_BLACK_LEVEL_SHARPNESS             194/* RWD */

/* Horizontal synchronization signal frequency in Hz as determined
 * by the display.
 * FFh: Indicates that the display cannot supply this information
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_HORIZONTAL_FREQUENCY                 195/* R-D */

/* Vertical synchronization signal frequency in 0.01Hz as
 * determined by the display.
 * FFh: Indicates that the display cannot supply this information
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_VERTICAL_FREQUENCY                   196/* R-D */

/* Select a type of LCD sub-pixel structure
 * NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_UNDEFINED:
 *      Sub-pixel layout is not defined
 * NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_RGB_V:
 *      Red / Green / Blue vertical stripe
 * NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_RGB_H:
 *      Red / Green / Blue horizontal stripe
 * NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_BGR_V:
 *      Blue / Green / Red vertical stripe
 * NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_BGR_H:
 *      Blue/ Green / Red horizontal stripe
 * NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_QUAD_1:
 *      Quad-pixel, a 2 x 2 sub-pixel structure with red at top
 *      left, blue at bottom right and green at top right and
 *      bottom left
 * NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_QUAD_2:
 *      Quad-pixel, a 2 x 2 sub-pixel structure with red at
 *      bottom left, blue at top right and green at top left and
 *      bottom right
 * NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_DELTA:
 *      Delta (triad)
 * NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_MOSAIC:
 *      Mosaic with red, green and blue fields overlayed 
 *      (field sequential color)
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT            197/* R-D */
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_UNDEFINED  0
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_RGB_V      1
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_RGB_H      2
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_BGR_V      3
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_BGR_H      4
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_QUAD_1     5
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_QUAD_2     6
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_DELTA      7
#define NV_CTRL_DDCCI_MISC_FLATPANEL_SUBPIXEL_LAYOUT_MOSAIC     8

/* Select the base technology type
 * NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_CRT_SHADOWMASK: CRT (shadowmask)
 * NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_CRT_APERTURE_GRILL: CRT (aperture
 *      grill)
 * NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_TFT: TFT
 * NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_LCOS: LCoS
 * NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_PLASMA: Plasma
 * NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_OLED: OLED
 * NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_EL: EL
 * NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_MEM: MEM
 * Type: integer
 */
#define NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE              198/* R-D */
#define NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_CRT_SHADOWMASK       1
#define NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_CRT_APERTURE_GRILL   2
#define NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_TFT                  3
#define NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_LCOS                 4
#define NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_PLASMA               5
#define NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_OLED                 6
#define NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_EL                   7
#define NV_CTRL_DDCCI_MISC_DISPLAY_TECHNOLOGY_TYPE_MEM                  8

/* Returns the current value (in hours) of `active power on' time
 * accumulated by the display ­ a 2 byte value.
 * "Active power on" time is defined as the period when the
 * emissive elements(s) of the display ­ cathodes for a CRT,
 * fluorescent lamps for a LCD, etc ­ are active.
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_DISPLAY_USAGE_TIME                   199/* R-D */

/* Returns the length (in bytes) of non-volatile storage in the display
 * available for writing a display descriptor ­ the maximum
 * descriptor length is 256 bytes
 * See 
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_DISPLAY_DESCRIPTOR_LENGTH            200/* R-D */

/* If enabled (NV_CTRL_DDCCI_ON), the display descriptor written to the 
 * display using XNVCTRLSetDDCCIDisplayDescriptor() shall be displayed when 
 * no video is being received.
 * The duration for which it is displayed is left to individual manufacturers.
 * NV_CTRL_DDCCI_ON: Display is enabled
 * NV_CTRL_DDCCI_OFF: The display descriptor shall not be displayed.
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_MISC_ENABLE_DISPLAY_OF_DISPLAY_DESCRIPTOR 201/* RWD */

/* A 2 byte value used to allow an application to only operate with
 * known products. The display manufacturer and application author
 * agree to a code such that application will only run when a valid
 * code is present in the display.
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_APPLICATION_ENABLE_KEY               202/* R-D */

/* This VCP code provides allows a 2 byte value identifying the
 * firmware level installed in the display to be determined.
 * Type: Range
 */
#define NV_CTRL_DDCCI_MISC_DISPLAY_FIRMWARE_LEVEL               203/* R-D */

/* Indicates the current state of the display OSD
 * NV_CTRL_DDCCI_MISC_OSD_DISABLED: OSD is disabled
 * NV_CTRL_DDCCI_MISC_OSD_ENABLED: OSD is enabled
 * NV_CTRL_DDCCI_MISC_OSD_CANNOT_SUPPLY: Indicates that the display cannot 
 *  supply this information.
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_OSD                                  204/* RWD */
#define NV_CTRL_DDCCI_MISC_OSD_DISABLED                         1
#define NV_CTRL_DDCCI_MISC_OSD_ENABLED                          2
#define NV_CTRL_DDCCI_MISC_OSD_CANNOT_SUPPLY                    0xFF

/* Allows the displayed OSD language to be selected.
 * Type: Integer */
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE                         205/* RWD */
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_CHINESE                 1
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_ENGLISH                 2
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_FRENCH                  3
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_GERMAN                  4
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_ITALIAN                 5
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_JAPANESE                6
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_KOREAN                  7
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_PORTUGESE               8
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_RUSSIAN                 9
#define NV_CTRL_DDCCI_MISC_OSD_LANGUAGE_SPANISH                 10

/* An `auxiliary display' is a small alpha-numeric display associated
 * with the primary display and able to be accessed via the primary
 * display.
 * This command returns a 1 byte value that defines the number of
 * characters and the number of rows available. The format is:
 * Bits 0 to 5 : The number of characters / row
 * Bits 6 to 7 : The number of rows
 * I.e. The maximum auxiliary display size is 4 rows each with 64 characters
 * Type: Bitmask
 */
#define NV_CTRL_DDCCI_MISC_AUXILIARY_DISPLAY_SIZE               206/* R-D */
#define NV_CTRL_DDCCI_MISC_AUXILIARY_DISPLAY_SIZE_COLUMNS_MASK  0x1F
#define NV_CTRL_DDCCI_MISC_AUXILIARY_DISPLAY_SIZE_ROWS_MASK     0x60

#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT                        207/* RWD */

/* Used to selects the active output.
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_ANALOG_1: Analog video (R/G/B) 1
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_ANALOG_2: Analog video (R/G/B) 2
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_DIGITAL_1: Digital video (TMDS) 1
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_DIGITAL_2: Digital video (TMDS) 2
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPOSITE_1: Composite video 1
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPOSITE_2: Composite video 2
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_SVIDEO_1: S-video 1
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_SVIDEO_2: S-video 2
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_TUNER_1: Tuner 1
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_TUNER_2: Tuner 2
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_TUNER_3: Tuner 3
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPONENT_1: Component video (YPrPb/YCrCb) 1
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPONENT_2: Component video (YPrPb/YCrCb) 2
 * NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPONENT_3: Component video (YPrPb/YCrCb) 3
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_ANALOG_1               1
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_ANALOG_2               2
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_DIGITAL_1              3
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_DIGITAL_2              4
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPOSITE_1            5
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPOSITE_2            6
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_SVIDEO_1               7
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_SVIDEO_2               8
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_TUNER_1                9
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_TUNER_2                10
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_TUNER_3                11
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPONENT_1            12
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPONENT_2            13
#define NV_CTRL_DDCCI_MISC_OUTPUT_SELECT_COMPONENT_3            14

/* Used to select the video mode with respect to 2D or 3D video.
 * Here is the meaning of the bitmask:
 * NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_FIELD_SEQUENTIAL_RIGHT_FIRST:
 *  Enable Field-Sequential Right Eye First
 * NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_FIELD_SEQUENTIAL_LEFT_FIRST:
 *  Enable Field-Sequential Left Eye First
 * NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_2WAY_INTERLEAVED_RIGHT_FIRST:
 *  Enable 2-Way Interleaved Right Eye First
 * NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_2WAY_INTERLEAVED_LEFT_FIRST:
 *  Enable 2-Way Interleaved Left Eye First
 * NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_4WAY_INTERLEAVED_ODD_LINES:
 *  Enable 4-Way Interleaved, Display Stereo Buffer 0 (even scan lines)
 * NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_4WAY_INTERLEAVED_ODD_LINES:
 *  Enable 4-Way Interleaved, Display Stereo Buffer 1 (odd scan lines)
 * NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_SIDE_BY_SIDE_INTERLEAVED:
 * Enable Side-by-Side Interleaved
 * Note: It is permissible, during a read operation, for a display to indicate
 * support for 2 or more stereo modes
 * Type: Bitmask
 */
#define NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE                    208/* RWD */
#define NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_FIELD_SEQUENTIAL_RIGHT_FIRST (1<<6)
#define NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_FIELD_SEQUENTIAL_LEFT_FIRST  (1<<5)
#define NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_2WAY_INTERLEAVED_RIGHT_FIRST (1<<4)
#define NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_2WAY_INTERLEAVED_LEFT_FIRST  (1<<3)
#define NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_4WAY_INTERLEAVED_EVEN_LINES  (1<<2)
#define NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_4WAY_INTERLEAVED_ODD_LINES   (1<<1)
#define NV_CTRL_DDCCI_MISC_STEREO_VIDEO_MODE_SIDE_BY_SIDE_INTERLEAVED     (1<<0)

/* Power Mode ­ DPMS and DPM standards are supported.
 * Value                                    DPMS        DPM
 * NV_CTRL_DDCCI_MISC_POWER_MODE_ON         On          On
 * NV_CTRL_DDCCI_MISC_POWER_MODE_STANDBY    Standby     Off
 * NV_CTRL_DDCCI_MISC_POWER_MODE_SUSPEND    Suspend     Off
 * NV_CTRL_DDCCI_MISC_POWER_MODE_OFF        Off         Off
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_POWER_MODE                           209/* RWD */
#define NV_CTRL_DDCCI_MISC_POWER_MODE_ON                        1
#define NV_CTRL_DDCCI_MISC_POWER_MODE_STANDBY                   2
#define NV_CTRL_DDCCI_MISC_POWER_MODE_SUSPEND                   3
#define NV_CTRL_DDCCI_MISC_POWER_MODE_OFF                       4

/* Controls output of an auxiliary power output from a display to a host device.
 * NV_CTRL_DDCCI_MISC_AUXILIARY_POWER_OUTPUT_DISABLE: 
 *  Disable auxiliary output power
 * NV_CTRL_DDCCI_MISC_AUXILIARY_POWER_OUTPUT_ENABLE: 
 * Enable auxiliary output power
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_AUXILIARY_POWER_OUTPUT               210/* RWD */
#define NV_CTRL_DDCCI_MISC_AUXILIARY_POWER_OUTPUT_DISABLE       1
#define NV_CTRL_DDCCI_MISC_AUXILIARY_POWER_OUTPUT_ENABLE        2

/* Operation mode
 * NV_CTRL_DDCCI_MISC_OPERATION_MODE_STANDALONE: Stand alone
 * NV_CTRL_DDCCI_MISC_OPERATION_MODE_SLAVE: Slave (full PC control)
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_OPERATION_MODE                       211/* -WD */
#define NV_CTRL_DDCCI_MISC_OPERATION_MODE_STANDALONE            1
#define NV_CTRL_DDCCI_MISC_OPERATION_MODE_SLAVE                 2

/* Define the version number of VCP list recognized by the display.
 * This is a 2 byte value, byte 1 defines the version number and
 * byte 2 defines the revision number
 * e.g 02 00 (hex) defines Version 2, Revision 0
 * Type: Integer
 */
#define NV_CTRL_DDCCI_MISC_VCP_VERSION                          212/* R-D */

/* Save the current adjustment data to EEPROM or other 
 * non-volatile storage inside the display
 * Returns TRUE on success
 * Type: Boolean
 */
#define NV_CTRL_DDCCI_SAVE_CURRENT_SETTINGS                     213/* -WD */

/* Detects if the display is DDC/CI capable
 * Returns TRUE on success
 * Type: Boolean
 */ 
#define NV_CTRL_DDCCI_CAPABLE                                   214/* R-D*/

#define NV_CTRL_DDCCI_FIRST_VCP NV_CTRL_DDCCI_GEOMETRY_HORIZONTAL_POSITION
#define NV_CTRL_DDCCI_LAST_VCP NV_CTRL_DDCCI_CAPABLE
/**************************************************************************/

/*
 * NV_CTRL_FLATPANEL_CHIP_LOCATION - for the specified display device,
 * report whether the flatpanel is driven by the on-chip controller,
 * or a separate controller chip elsewhere on the graphics board.
 * This attribute is only available for flatpanels.
 */

#define NV_CTRL_FLATPANEL_CHIP_LOCATION                         215/* R-D */
#define NV_CTRL_FLATPANEL_CHIP_LOCATION_INTERNAL                  0
#define NV_CTRL_FLATPANEL_CHIP_LOCATION_EXTERNAL                  1

/*
 * NV_CTRL_FLATPANEL_LINK - report whether the specified display
 * device is driven by a single link or dual link DVI connection.
 * This attribute is only available for flatpanels.
 */

#define NV_CTRL_FLATPANEL_LINK                                  216/* R-D */
#define NV_CTRL_FLATPANEL_LINK_SINGLE                             0
#define NV_CTRL_FLATPANEL_LINK_DUAL                               1

/*
 * NV_CTRL_FLATPANEL_SIGNAL - for the specified display device, report
 * whether the flatpanel is driven by an LVDS or TMDS signal.  This
 * attribute is only available for flatpanels.
 */

#define NV_CTRL_FLATPANEL_SIGNAL                                217/* R-D */
#define NV_CTRL_FLATPANEL_SIGNAL_LVDS                             0
#define NV_CTRL_FLATPANEL_SIGNAL_TMDS                             1


/*
 * NV_CTRL_USE_HOUSE_SYNC - when TRUE, framelock will sync to the house
 * sync
 *
 */

#define NV_CTRL_USE_HOUSE_SYNC                                  218/* RW- */
#define NV_CTRL_USE_HOUSE_SYNC_FALSE                            0
#define NV_CTRL_USE_HOUSE_SYNC_TRUE                             1

/**************************************************************************/
#define NV_CTRL_LAST_ATTRIBUTE NV_CTRL_USE_HOUSE_SYNC

/**************************************************************************/

/*
 * String Attributes:
 */

#define QUERY_STRING_ERR 0
#define QUERY_STRING_OK 1
#define QUERY_STRING_STATIC 2

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


/* NV_CTRL_STRING_DDCCI_MISC_TRANSMIT_DISPLAY_DESCRIPTOR -
 * Allows a display descriptor (up to maximum length defined by the
 * display (see NV_CTRL_DDCCI_MISC_DISPLAY_DESCRIPTOR_LENGTHC2h) to be written 
 * (read) to (from) non-volatile storage in the display.
 * If an attempt is made to write beyond the maximum storage
 * length, the descriptor shall be truncated with the excess bytes
 * being discarded.
 */
#define NV_CTRL_STRING_DDCCI_MISC_TRANSMIT_DISPLAY_DESCRIPTOR   6  /* RWD */


/* NV_CTRL_STRING_DDCCI_MISC_AUXILIARY_DISPLAY_DATA -
 * An `auxiliary display' is a small alpha-numeric display associated
 * with the primary display and able to be accessed via the primary
 * display.
 * This command transmits a number of bytes of alpha-numeric
 * data to be displayed on the auxiliary display. The data shall
 * conform to ISO 8859-2 (Latin 1) code set.
 * The auxiliary display will be written from the top left position,
 * moving to right along each line and then starting at left end of the
 * next line.
 */
#define NV_CTRL_STRING_DDCCI_MISC_AUXILIARY_DISPLAY_DATA        7  /* -WD */

#define NV_CTRL_STRING_LAST_ATTRIBUTE NV_CTRL_STRING_DDCCI_MISC_AUXILIARY_DISPLAY_DATA

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
