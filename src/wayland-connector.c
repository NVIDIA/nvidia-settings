#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-client.h>

#include "wayland-connector.h"


static const char *get_transform_name(int32_t transform)
{
    const char *transform_name = NULL;

    switch (transform) {
        case WL_OUTPUT_TRANSFORM_NORMAL:
            transform_name = "Normal";
            break;
        case WL_OUTPUT_TRANSFORM_90:
            transform_name = "90";
            break;
        case WL_OUTPUT_TRANSFORM_180:
            transform_name = "180";
            break;
        case WL_OUTPUT_TRANSFORM_270:
            transform_name = "270";
            break;
        case WL_OUTPUT_TRANSFORM_FLIPPED:
            transform_name = "Flipped";
            break;
        case WL_OUTPUT_TRANSFORM_FLIPPED_90:
            transform_name = "Flipped 90";
            break;
        case WL_OUTPUT_TRANSFORM_FLIPPED_180:
            transform_name = "Flipped 180";
            break;
        case WL_OUTPUT_TRANSFORM_FLIPPED_270:
            transform_name = "Flipped 270";
            break;
        default:
            transform_name = "Unknown";
    }

    return transform_name;
}

static void output_geometry(void *data, struct wl_output *wl_output,
                            int32_t x, int32_t y,
                            int32_t pw, int32_t ph, int32_t subpx,
                            const char *make, const char *model,
                            int32_t transform)
{
    wayland_output_info *output = (wayland_output_info *) data;

    output->x = x;
    output->y = y;
    output->pw = pw;
    output->ph = ph;
    output->subpx = subpx;
    output->make = strdup(make);
    output->model = strdup(model);

    output->transform_name = get_transform_name(transform);
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
                        int32_t width, int32_t height, int32_t refresh)
{
    wayland_output_info *output = (wayland_output_info *) data;

    output->mode_width   = width;
    output->mode_height  = height;
    output->mode_refresh = refresh;
    output->mode_flags   = flags;

    output->is_current_mode = (flags & WL_OUTPUT_MODE_CURRENT) != 0;
}

static void output_done(void *data, struct wl_output *wl_output)
{
    // No-op
}

static void output_scale(void *data, struct wl_output *wl_output,
                         int32_t factor)
{
    wayland_output_info *output = (wayland_output_info *) data;

    output->scale = factor;
}

static const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode     = output_mode,
    .done     = output_done,
    .scale    = output_scale,
};

static void registry_handle_global(void *data_in, struct wl_registry *registry,
                                   uint32_t name, const char *interface,
                                   uint32_t version)
{
    wayland_data *data = (wayland_data*)data_in;

    if (strcmp(interface, "wl_output") == 0) {
        wayland_output_info *output;
        output = calloc(1, sizeof(wayland_output_info));
        output->name = name;
        output->version = 2;
        output->output = wl_registry_bind(data->registry, name,
                                          &wl_output_interface,
                                          output->version);
        wl_output_add_listener(output->output,
                                          &output_listener,
                                          output);

        if (data->output) {
            wayland_output_info *i = data->output;
            while (i->next != NULL) {
                i = i->next;
            }
            i->next = output;
        } else {
            data->output = output;
        }
    }
}

static void registry_handle_global_remove(void *data,
                                          struct wl_registry *registry,
                                          uint32_t name)
{
    // No-op.
}

static const struct wl_registry_listener registry_listener = {
                                .global = registry_handle_global,
                                .global_remove = registry_handle_global_remove,
};


wayland_output_info *get_wayland_output_info(void)
{
    wayland_data data;
    struct wl_display *display;
    struct wl_registry *registry;

    display = wl_display_connect(NULL);
    if (!display) {
        return NULL;
    }
    registry = wl_display_get_registry(display);

    data.registry = registry;
    data.output = 0;
    wl_registry_add_listener(registry, &registry_listener, &data);
    wl_display_roundtrip(display);
    wl_display_roundtrip(display); // Second call for added output listener
    wl_display_disconnect(display);

    return data.output;
}

void *get_wayland_display(void)
{
    return (void*) wl_display_connect(NULL);
}
