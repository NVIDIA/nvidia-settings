#ifndef __WAYLAND_CONNECTOR__
#define __WAYLAND_CONNECTOR__

typedef struct _wayland_output_info {
    struct wl_output *output;
    struct _wayland_output_info *next;

    int name, version;
    int x, y, pw, ph, subpx, scale;
    const char *make, *model, *transform_name;
    int mode_width, mode_height, mode_refresh;
    unsigned int mode_flags;
    int is_current_mode;
} wayland_output_info;

typedef struct _wayland_data {
    struct wl_registry *registry;
    wayland_output_info *output;
    struct _wayland_lib *wayland_lib_data;
} wayland_data;

typedef struct _wayland_lib
{
    void *lib_handle;
    char *error_msg;

    wayland_output_info *(*fn_get_wayland_output_info)(void);
} wayland_lib;

wayland_output_info *get_wayland_output_info(void);

#endif
