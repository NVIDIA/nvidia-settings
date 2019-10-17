/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
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
 
#ifndef __CTK_DISPLAYCONFIG_UTILS_H__
#define __CTK_DISPLAYCONFIG_UTILS_H__

#include <gtk/gtk.h>

#include "XF86Config-parser/xf86Parser.h"

#include "ctkdisplaylayout.h"




G_BEGIN_DECLS

/* Token parsing handlers */

void apply_modeline_token(char *token, char *value, void *data);
void apply_metamode_token(char *token, char *value, void *data);
void apply_monitor_token(char *token, char *value, void *data);
void apply_screen_info_token(char *token, char *value, void *data);


/* Mode functions */

void clamp_rect_to_viewportin(GdkRectangle *rect, const nvMode *mode);
void clamp_mode_panning(nvModePtr mode);
void get_viewportin_rect(const nvMode *mode, GdkRectangle *rect);
void mode_set_modeline(nvModePtr mode,
                       nvModeLinePtr modeline,
                       const nvSize *providedViewPortIn,
                       const GdkRectangle *providedViewPortOut);
Bool mode_set_rotation(nvModePtr mode, Rotation rotation);
nvModePtr mode_parse(nvDisplayPtr display, const char *mode_str);
void apply_underscan_to_viewportout(const nvSize raster_size,
                                    const int hpixel_value,
                                    GdkRectangle *viewPortOut);
void get_underscan_settings_from_viewportout(const nvSize raster_size,
                                             const GdkRectangle viewPortOut,
                                             gfloat *percent_value,
                                             gint *pixel_value);

/* ModeLine functions */

Bool modelines_match(nvModeLinePtr modeline1, nvModeLinePtr modeline2);
void modeline_free(nvModeLinePtr m);



/* ViewPort functions */

Bool viewports_in_match(const nvSize viewPortIn1,
                        const nvSize viewPortIn2);
Bool viewports_out_match(const GdkRectangle viewPortOut1,
                         const GdkRectangle viewPortOut2);



/* Display functions */

int display_find_closest_mode_matching_modeline(nvDisplayPtr display,
                                                nvModeLinePtr modeline);
Bool display_has_modeline(nvDisplayPtr display, nvModeLinePtr modeline);
Bool display_add_modelines_from_server(nvDisplayPtr display, nvGpuPtr gpu,
                                       gchar **err_str);
void display_remove_modes(nvDisplayPtr display);


/* Metamode functions */

void cleanup_metamode(nvMetaModePtr metamode);


/* Screen functions */

void clamp_screen_size_rect(GdkRectangle *rect);
int get_screen_max_displays(nvScreenPtr screen);
void renumber_xscreens(nvLayoutPtr layout);
void screen_unlink_display(nvDisplayPtr display);
void screen_link_display(nvScreenPtr screen, nvDisplayPtr display);
void screen_remove_display(nvDisplayPtr display);
void screen_unlink_prime_display(nvPrimeDisplayPtr prime);
void screen_link_prime_display(nvScreenPtr screen, nvPrimeDisplayPtr prime);
gchar * screen_get_metamode_str(nvScreenPtr screen, int metamode_idx,
                                int force_target_id_name);
void link_screen_to_gpu(nvScreenPtr screen, nvGpuPtr gpu);
Bool screen_has_gpu(nvScreenPtr screen, nvGpuPtr match_gpu);


/* GPU functions */

void gpu_remove_and_free_display(nvDisplayPtr display);

Bool gpu_add_screenless_modes_to_displays(nvGpuPtr gpu);


/* Layout functions */

void layout_free(nvLayoutPtr layout);
void layout_add_screen(nvLayoutPtr layout, nvScreenPtr screen);
nvLayoutPtr layout_load_from_server(CtrlTarget *ctrl_target,
                                    gchar **err_str);
nvScreenPtr layout_get_a_screen(nvLayoutPtr layout, nvGpuPtr preferred_gpu);
nvDisplayPtr layout_get_display(const nvLayoutPtr layout,
                                const unsigned int display_id);
void layout_remove_and_free_screen(nvScreenPtr screen);



/* Save X config dialog  */

typedef XConfigPtr (* generate_xconfig_callback) (XConfigPtr xconfCur,
                                                  Bool merge,
                                                  Bool *merged,
                                                  gpointer callback_data);

typedef struct _SaveXConfDlg {

    GtkWidget *parent;
    GtkWidget *top_window;

    /* Callback functions for generating the XConfig struct */
    generate_xconfig_callback xconf_gen_func;
    void *callback_data;

    Bool merge_toggleable; /* When possible, user able to toggle merge */

    GtkWidget *dlg_xconfig_save;     /* Save X config dialog */
    GtkWidget *scr_xconfig_save;     /* Scroll window */
    GtkWidget *txt_xconfig_save;     /* Text view of file contents */
    GtkTextBuffer *buf_xconfig_save; /* Text buffer (Actual) file contents */
    GtkWidget *btn_xconfig_merge;    /* Merge with existing X config */
    GtkWidget *btn_xconfig_preview;  /* Show/Hide button */
    GtkWidget *box_xconfig_save;     /* Show/Hide this box */

    GtkWidget *btn_xconfig_file;
    GtkWidget *txt_xconfig_file;

} SaveXConfDlg;



SaveXConfDlg *create_save_xconfig_dialog(GtkWidget *parent,
                                         Bool merge_toggleable,
                                         generate_xconfig_callback xconf_gen_func,
                                         gpointer callback_data);

int run_save_xconfig_dialog(SaveXConfDlg *dlg);

                   
G_END_DECLS

#endif /* __CTK_DISPLAYCONFIG_UTILS_H__ */
