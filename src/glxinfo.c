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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "NvCtrlAttributes.h"
#include "query-assign.h"
#include "msg.h"
#include "glxinfo.h"

#include <GL/glx.h> /* GLX #defines */
#include <EGL/egl.h>


/*
 * print_extension_list() - Formats OpenGL/GLX extension strings
 * to contain commas and returns a pointer to the formatted string.
 * The user is responsible for freeing this buffer.
 *
 * If there is an error or there is not enough memory to create
 * the buffer, NULL is returned.
 *
 */

static char *
format_extension_list(const char *ext)
{
   int    i;
   char * extTmp = NULL; /* Actual string to print */
   const char *extCountTmp;


   if ( !ext || !ext[0] )
      return NULL;

   /* Count number of extensions (to get number of commas needed) */
   i = 0;
   extCountTmp = ext;
   while ( *extCountTmp != '\0' ) {
       if ( *extCountTmp == ' ' ) {
           i++;
       }
       extCountTmp++;
   }

   /*
    * Allocate buffer that will hold the extension string with
    * commas in it 
    */
   extTmp = nvalloc( (strlen(ext) +i +1) *sizeof(char) );

   /* Copy extension string to buffer, adding commas */
   i = 0;
   while ( *ext != '\0' && *ext != '\n' ) {
       if ( *ext == ' ' ) {
           extTmp[i++] = ',';
       }
       extTmp[i++] = *ext;
       ext++;
   }
   extTmp[i] = '\0';

   /* Remove any trailing whitespace and commas */
   while ( extTmp[ strlen(extTmp)-1 ] == ' ' || 
           extTmp[ strlen(extTmp)-1 ] == ',' ) {
       extTmp[ strlen(extTmp)-1 ] = '\0';
   }

   return extTmp;
} /* format_extension_list() */


/*
 * print_fbconfig_attribs() & helper functions -
 *   Prints a table of fbconfig attributes
 *
 * NOTE: Only support FBconfig for GLX v1.3+
 *
 */

#ifdef GLX_VERSION_1_3

const char *
render_type_abbrev(int rend_type)
{
    switch (rend_type) {
       case GLX_RGBA_BIT:
           return "rgb";
       case GLX_COLOR_INDEX_BIT:
	   return "ci";
       case (GLX_RGBA_BIT | GLX_COLOR_INDEX_BIT):
	   return "any";
       default:
	   return ".";
    }
}


const char *
transparent_type_abbrev(int trans_type)
{
    switch (trans_type) {
       case GLX_NONE:
	   return ".";
       case GLX_TRANSPARENT_RGB:
	   return "rg";
       case GLX_TRANSPARENT_INDEX:
	   return "ci";
       default:
	   return ".";
    }
}


const char *
x_visual_type_abbrev(int x_visual_type)
{
    switch (x_visual_type) {
       case GLX_TRUE_COLOR:
           return "tc";
       case GLX_DIRECT_COLOR:
           return "dc";
       case GLX_PSEUDO_COLOR:
           return "pc";
       case GLX_STATIC_COLOR:
           return "sc";
       case GLX_GRAY_SCALE:
           return "gs";
       case GLX_STATIC_GRAY:
           return "sg";
       default:
           return ".";
    }
}


const char *
caveat_abbrev(int caveat)
{
   if (caveat == GLX_NONE_EXT || caveat == 0)
      return(".");
   else if (caveat == GLX_SLOW_VISUAL_EXT)
      return("slo");
   else if (caveat == GLX_NON_CONFORMANT_VISUAL_EXT)
      return("NoC");
   else
       return(".");
}


static void
print_fbconfig_attribs(const GLXFBConfigAttr *fbca)
{
    int i; /* Iterator */


    if ( fbca == NULL ) {
        return;
    }

    printf("--fc- --vi- vt buf lv rgb d s colorbuffer ax dp st "
           "accumbuffer ---ms---- cav -----pbuffer----- ---transparent----\n");
    printf("  id    id     siz l  ci  b t  r  g  b  a bf th en "
           " r  g  b  a mvs mcs b eat widt hght max-pxs typ  r  g  b  a  i\n");
    printf("---------------------------------------------------"
           "--------------------------------------------------------------\n");

    i = 0;
    while ( fbca[i].fbconfig_id != 0 ) {
        
        printf("0x%03x ", fbca[i].fbconfig_id);
        if ( fbca[i].visual_id ) {
            printf("0x%03x ", fbca[i].visual_id);
        } else {
            printf("   .  ");
        }
        printf("%2.2s %3d %2d %3.3s %1c %1c ",
               x_visual_type_abbrev(fbca[i].x_visual_type),
               fbca[i].buffer_size,
               fbca[i].level,
               render_type_abbrev(fbca[i].render_type),
               fbca[i].doublebuffer ? 'y' : '.',
               fbca[i].stereo ? 'y' : '.'
               );
        printf("%2d %2d %2d %2d %2d %2d %2d ",
               fbca[i].red_size,
               fbca[i].green_size,
               fbca[i].blue_size,
               fbca[i].alpha_size,
               fbca[i].aux_buffers,
               fbca[i].depth_size,
               fbca[i].stencil_size
               );
        printf("%2d %2d %2d %2d ",
               fbca[i].accum_red_size,
               fbca[i].accum_green_size,
               fbca[i].accum_blue_size,
               fbca[i].accum_alpha_size
               );
        if ( fbca[i].multi_sample_valid == 1 ) {
            printf("%3d ",
                   fbca[i].multi_samples
                   );

            if ( fbca[i].multi_sample_coverage_valid == 1 ) {
                printf("%3d ",
                       fbca[i].multi_samples_color
                       );
            } else {
                printf("%3d ",
                       fbca[i].multi_samples
                       );
            }
            printf("%1d ",
                   fbca[i].multi_sample_buffers
                   );

        } else {
            printf("  .   . . ");
        }
        printf("%3.3s %4x %4x %7x %3.3s %2d %2d %2d %2d %2d\n",
               caveat_abbrev(fbca[i].config_caveat),
               fbca[i].pbuffer_width,
               fbca[i].pbuffer_height,
               fbca[i].pbuffer_max,
               transparent_type_abbrev(fbca[i].transparent_type),
               fbca[i].transparent_red_value,
               fbca[i].transparent_green_value,
               fbca[i].transparent_blue_value,
               fbca[i].transparent_alpha_value,
               fbca[i].transparent_index_value
               );
        
        i++;
    } /* Done printing FBConfig attributes for FBConfig */

} /* print_fbconfig_attribs() */

#endif /* GLX_VERSION_1_3 */


/*
 * print_glxinfo() - prints information about glx
 *
 */

#define TAB "  "

#define SAFE_FREE(m) \
if ( (m) != NULL ) { \
    free( m ); \
    m = NULL; \
}

#define NULL_TO_EMPTY(s) \
((s)!=NULL)?(s):""

void print_glxinfo(const char *display_name, CtrlSystemList *systems)
{
    CtrlSystem       *system;
    CtrlTargetNode  *node;
    ReturnStatus     status = NvCtrlSuccess;

    char            *direct_rendering  = NULL;
    char            *glx_extensions    = NULL;
    char            *server_vendor     = NULL;
    char            *server_version    = NULL;
    char            *server_extensions = NULL;
    char            *client_vendor     = NULL;
    char            *client_version    = NULL;
    char            *client_extensions = NULL;
    char            *opengl_vendor     = NULL;
    char            *opengl_renderer   = NULL;
    char            *opengl_version    = NULL;
    char            *opengl_extensions = NULL;

    GLXFBConfigAttr *fbconfig_attribs  = NULL;

    char            *formatted_ext_str = NULL;

    system = NvCtrlConnectToSystem(display_name, systems);
    if (system == NULL) {
        return;
    }

    /* Print information for each screen */
    for (node = system->targets[X_SCREEN_TARGET]; node; node = node->next) {

        CtrlTarget *t = node->t;

        /* No screen, move on */
        if ( !t->h ) continue;

        nv_msg(NULL, "GLX Information for %s:", t->name);

        /* Get GLX information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_DIRECT_RENDERING,
                                          &direct_rendering);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_GLX_EXTENSIONS,
                                          &glx_extensions);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        if ( glx_extensions != NULL ) {
            formatted_ext_str = format_extension_list(glx_extensions);
            if ( formatted_ext_str != NULL ) {
                free(glx_extensions);
                glx_extensions = formatted_ext_str;
            }
        }
        /* Get server GLX information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_SERVER_VENDOR,
                                          &server_vendor);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_SERVER_VERSION,
                                          &server_version);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_SERVER_EXTENSIONS,
                                          &server_extensions);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        if ( server_extensions != NULL ) {
            formatted_ext_str = format_extension_list(server_extensions);
            if ( formatted_ext_str != NULL ) {
                free(server_extensions);
                server_extensions = formatted_ext_str;
            }
        }
        /* Get client GLX information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_CLIENT_VENDOR,
                                          &client_vendor);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_CLIENT_VERSION,
                                          &client_version);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_CLIENT_EXTENSIONS,
                                          &client_extensions);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        if ( client_extensions != NULL ) {
            formatted_ext_str = format_extension_list(client_extensions);
            if ( formatted_ext_str != NULL ) {
                free(client_extensions);
                client_extensions = formatted_ext_str;
            }
        }
        /* Get OpenGL information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_OPENGL_VENDOR,
                                          &opengl_vendor);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_OPENGL_RENDERER,
                                          &opengl_renderer);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_OPENGL_VERSION,
                                          &opengl_version);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_OPENGL_EXTENSIONS,
                                          &opengl_extensions);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        if ( opengl_extensions != NULL ) {
            formatted_ext_str = format_extension_list(opengl_extensions);
            if ( formatted_ext_str != NULL ) {
                free(opengl_extensions);
                opengl_extensions = formatted_ext_str;
            }
        }

        /* Get FBConfig information */
        status = NvCtrlGetVoidAttribute(t,
                                        NV_CTRL_ATTR_GLX_FBCONFIG_ATTRIBS,
                                        (void *)(&fbconfig_attribs));
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }

        /* Print results */
        nv_msg(TAB, "direct rendering: %s", NULL_TO_EMPTY(direct_rendering));
        nv_msg(TAB, "GLX extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(glx_extensions));
        nv_msg(" ", "\n");
        nv_msg(TAB, "server glx vendor string: %s",
               NULL_TO_EMPTY(server_vendor));
        nv_msg(TAB, "server glx version string: %s",
               NULL_TO_EMPTY(server_version));
        nv_msg(TAB, "server glx extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(server_extensions));
        nv_msg(" ", "\n");
        nv_msg(TAB, "client glx vendor string: %s",
               NULL_TO_EMPTY(client_vendor));
        nv_msg(TAB, "client glx version string: %s",
               NULL_TO_EMPTY(client_version));
        nv_msg(TAB, "client glx extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(client_extensions));
        nv_msg(" ", "\n");
        nv_msg(TAB, "OpenGL vendor string: %s",
               NULL_TO_EMPTY(opengl_vendor));
        nv_msg(TAB, "OpenGL renderer string: %s",
               NULL_TO_EMPTY(opengl_renderer));
        nv_msg(TAB, "OpenGL version string: %s",
               NULL_TO_EMPTY(opengl_version));
        nv_msg(TAB, "OpenGL extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(opengl_extensions));
#ifdef GLX_VERSION_1_3        
        if ( fbconfig_attribs != NULL ) {
            nv_msg(" ", "\n");
            print_fbconfig_attribs(fbconfig_attribs);
        }
#endif
        fflush(stdout);

        /* Free memory used */
        SAFE_FREE(server_vendor);
        SAFE_FREE(server_version);
        SAFE_FREE(server_extensions);
        SAFE_FREE(client_vendor);
        SAFE_FREE(client_version);
        SAFE_FREE(client_extensions);
        SAFE_FREE(direct_rendering);
        SAFE_FREE(glx_extensions);
        SAFE_FREE(opengl_vendor);
        SAFE_FREE(opengl_renderer);
        SAFE_FREE(opengl_version);
        SAFE_FREE(opengl_extensions);
        SAFE_FREE(fbconfig_attribs);

    } /* Done looking at all screens */


    /* Fall through */
 finish:
    if ( status == NvCtrlError ) {
        nv_error_msg("Error fetching GLX Information: %s",
                     NvCtrlAttributesStrError(status) );
    }

    /* Free any leftover memory used */
    SAFE_FREE(server_vendor);
    SAFE_FREE(server_version);
    SAFE_FREE(server_extensions);
    SAFE_FREE(client_vendor);
    SAFE_FREE(client_version);
    SAFE_FREE(client_extensions);
    SAFE_FREE(direct_rendering);
    SAFE_FREE(glx_extensions);
    SAFE_FREE(opengl_vendor);
    SAFE_FREE(opengl_renderer);
    SAFE_FREE(opengl_version);
    SAFE_FREE(opengl_extensions);
    SAFE_FREE(fbconfig_attribs);

    NvCtrlFreeAllSystems(systems);

} /* print_glxinfo() */



/*
 * EGL information
 *
 */

const char *egl_color_buffer_type_abbrev(int type)
{
    switch (type) {
    case EGL_RGB_BUFFER:
        return "rgb";
    case EGL_LUMINANCE_BUFFER:
        return "lum";
    default:
        return ".";
    }
}

const char *egl_config_caveat_abbrev(int type)
{
    switch (type) {
    case EGL_SLOW_CONFIG:
        return "slo";
    case EGL_NON_CONFORMANT_CONFIG:
        return "NoC";
    case EGL_NONE:
    default:
        return ".";
    }
}

const char *egl_transparent_type_abbrev(int type)
{
    switch (type) {
    case EGL_TRANSPARENT_RGB:
        return "rgb";
    case EGL_NONE:
    default:
        return ".";
    }
}

static void print_egl_config_attribs(const EGLConfigAttr *fbca)
{
    int i;

    if (fbca == NULL) {
        return;
    }

    printf("--fc- --vi- --vt-- buf lv rgb colorbuffer am lm dp st "
           "-bind cfrm sb sm cav -----pbuffer----- swapin nv   rn   su "
           "-transparent--\n");
    printf("  id    id         siz l  lum  r  g  b  a sz sz th en "
           " -  a            eat widt hght max-pxs  mx mn rd   ty   ty "
           "typ  r  g  b  \n");
    printf("------------------------------------------------------"
           "-----------------------------------------------------------"
           "--------------\n");

    for (i = 0; fbca[i].config_id != 0; i++) {

        printf("0x%03x ", fbca[i].config_id);
        if (fbca[i].native_visual_id) {
            printf("0x%03x ", fbca[i].native_visual_id);
        } else {
            printf("   .  ");
        }
        printf("0x%X %3d %2d %3s ",
               fbca[i].native_visual_type,
               fbca[i].buffer_size,
               fbca[i].level,
               egl_color_buffer_type_abbrev(fbca[i].color_buffer_type));
        printf("%2d %2d %2d %2d %2d %2d %2d %2d ",
               fbca[i].red_size,
               fbca[i].green_size,
               fbca[i].blue_size,
               fbca[i].alpha_size,
               fbca[i].alpha_mask_size,
               fbca[i].luminance_size,
               fbca[i].depth_size,
               fbca[i].stencil_size);
        printf("%2c %2c ",
               fbca[i].bind_to_texture_rgb ? 'y' : '.',
               fbca[i].bind_to_texture_rgba ? 'y' : '.');
        printf("0x%02X %2d %2d ",
               fbca[i].conformant,
               fbca[i].sample_buffers,
               fbca[i].samples);
        printf("%3.3s %4x %4x %7x %2d %2d ",
               egl_config_caveat_abbrev(fbca[i].config_caveat),
               fbca[i].max_pbuffer_width,
               fbca[i].max_pbuffer_height,
               fbca[i].max_pbuffer_pixels,
               fbca[i].max_swap_interval,
               fbca[i].min_swap_interval);
        printf("%2c %4x %4x ",
               fbca[i].native_renderable ? 'y' : '.',
               fbca[i].renderable_type,
               fbca[i].surface_type);
        printf("%3s %2d %2d %2d\n",
               egl_transparent_type_abbrev(fbca[i].transparent_type),
               fbca[i].transparent_red_value,
               fbca[i].transparent_green_value,
               fbca[i].transparent_blue_value);
    }

} /* print_egl_config_attribs() */


/*
 * print_eglinfo() - prints information about egl
 *
 */

void print_eglinfo(const char *display_name, CtrlSystemList *systems)
{
    CtrlSystem     *system;
    CtrlTargetNode *node;
    ReturnStatus   status = NvCtrlSuccess;

    char *egl_vendor        = NULL;
    char *egl_version       = NULL;
    char *egl_extensions    = NULL;
    char *formatted_ext_str = NULL;

    EGLConfigAttr *egl_config_attribs = NULL;


    system = NvCtrlConnectToSystem(display_name, systems);
    if (system == NULL) {
        return;
    }

    /* Print information for each screen */
    for (node = system->targets[X_SCREEN_TARGET]; node; node = node->next) {

        CtrlTarget *t = node->t;

        /* No screen, move on */
        if (!t->h) continue;

        nv_msg(NULL, "EGL Information for %s:", t->name);

        /* Get EGL information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_EGL_VENDOR,
                                          &egl_vendor);
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }

        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_EGL_VERSION,
                                          &egl_version);
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }

        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_EGL_EXTENSIONS,
                                          &egl_extensions);
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }

        if (egl_extensions != NULL) {
            formatted_ext_str = format_extension_list(egl_extensions);
            if (formatted_ext_str != NULL) {
                free(egl_extensions);
                egl_extensions = formatted_ext_str;
            }
        }

        /* Get FBConfig information */
        status = NvCtrlGetVoidAttribute(t,
                                        NV_CTRL_ATTR_EGL_CONFIG_ATTRIBS,
                                        (void *)(&egl_config_attribs));
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }


        /* Print results */
        nv_msg(TAB, "EGL vendor string: %s", NULL_TO_EMPTY(egl_vendor));
        nv_msg(TAB, "EGL version string: %s", NULL_TO_EMPTY(egl_version));
        nv_msg(TAB, "EGL extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(egl_extensions));
        nv_msg(" ", "\n");

        if (egl_config_attribs != NULL) {
            nv_msg(" ", "\n");
            print_egl_config_attribs(egl_config_attribs);
        }

        fflush(stdout);

        /* Free memory used */
        SAFE_FREE(egl_vendor);
        SAFE_FREE(egl_version);
        SAFE_FREE(egl_extensions);
        SAFE_FREE(egl_config_attribs);

    } /* Done looking at all screens */


    /* Fall through */
 finish:
    if (status == NvCtrlError) {
        nv_error_msg("Error fetching EGL Information: %s",
                     NvCtrlAttributesStrError(status) );
    }

    /* Free any leftover memory used */
    SAFE_FREE(egl_vendor);
    SAFE_FREE(egl_version);
    SAFE_FREE(egl_extensions);
    SAFE_FREE(egl_config_attribs);

    NvCtrlFreeAllSystems(systems);

} /* print_eglinfo() */



/*
 * Vulkan information
 *
 */


const char *vulkan_get_physical_device_type(VkPhysicalDeviceType type)
{
    switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        return "Other";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "Integrated";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "Discrete";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "Virtual";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "CPU";
    default:
        return "Unknown";
    }
}


char *vulkan_get_queue_family_flags(VkQueueFlags flags)
{
    const char *gb = flags & VK_QUEUE_GRAPHICS_BIT ?       " Graphics" : "";
    const char *cb = flags & VK_QUEUE_COMPUTE_BIT  ?       " Compute"  : "";
    const char *tb = flags & VK_QUEUE_TRANSFER_BIT ?       " Transfer" : "";
    const char *sb = flags & VK_QUEUE_SPARSE_BINDING_BIT ? " Sparse"   : "";

    if (!flags) {
        return nvasprintf(" None");
    }

    return nvasprintf("%s%s%s%s", gb, cb, tb, sb);
}


char *vulkan_get_memory_property_flags(VkMemoryPropertyFlagBits flags)
{
    const char *dl  =
        flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  ? " DeviceLocal" : "";
    const char *hv  =
        flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  ? " HostVisible" : "";
    const char *hco =
        flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? " HostCoherent" : "";
    const char *hca =
        flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT   ? " HostCached" : "";
    const char *la  = flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ?
                          " LazilyAllocated" : "";

    if (!flags) {
        return nvasprintf(" None");
    }

    return nvasprintf("%s%s%s%s%s", dl, hv, hco, hca, la);
}


char *vulkan_get_memory_heap_flags(VkMemoryHeapFlags flags)
{
    const char *dl =
        flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? " DeviceLocal" : "";

    if (!flags) {
        return nvasprintf(" None");
    }

    return nvasprintf("%s", dl);
}


#define PRINT_MATCH_FLAGS(var) flags & var ? " " #var : ""

char *vulkan_get_format_feature_flags(VkFormatFeatureFlags flags)
{

    const char *smi = PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    const char *sti = PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
    const char *sia =
        PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT);
    const char *utb =
        PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT);
    const char *stb =
        PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT);
    const char *sta =
        PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT);
    const char *vb  = PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT);
    const char *ca  = PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
    const char *cab =
        PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT);
    const char *dsa =
        PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    const char *bs  = PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_BLIT_SRC_BIT);
    const char *bd  = PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_BLIT_DST_BIT);
    const char *sfl =
        PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    const char *sfc =
        PRINT_MATCH_FLAGS(VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG);

    if (!flags) {
        return nvasprintf(" None");
    }

    return nvasprintf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                      smi, sti, sia, utb, stb, sta, vb,
                      ca, cab, dsa, bs, bd, sfl, sfc);


}
#undef PRINT_MATCH_FLAGS


static void print_vulkan_format_feature_flags(VkFormatFeatureFlags flags)
{
    char *s = vulkan_get_format_feature_flags(flags);
    char *tok = strtok(s, " ");
    while (tok) {
        if (strlen(tok) > 0) {
            nv_msg("        ", "%s", tok);
        }
        tok = strtok(NULL, " ");
    }
    nvfree(s);
}


static void print_extension_property(VkExtensionProperties *ext, const char *s)
{
    const char *prefix = s ? s : "";
    nv_msg(prefix, "%s - Version: %d",
           ext->extensionName, ext->specVersion);
}


char *vulkan_get_version_string(uint32_t version) {
    return nvasprintf("%d.%d.%d", VK_VERSION_MAJOR(version),
                                  VK_VERSION_MINOR(version),
                                  VK_VERSION_PATCH(version));
}


/*
 * print_vulkaninfo() - prints information about Vulkan
 */

void print_vulkaninfo(const char *display_name, CtrlSystemList *systems)
{
    CtrlSystem     *system;
    CtrlTargetNode *node;
    ReturnStatus   status = NvCtrlSuccess;

    char *vk_api_version = NULL;

    int i, j;

    system = NvCtrlConnectToSystem(display_name, systems);
    if (system == NULL) {
        return;
    }

    /* Print information - We only need to address the first GPU target as it
     * give us all available devices since we clear the Display envvar when
     * creating an instance. */
    node = system->targets[GPU_TARGET];

    if (node) {

        VkLayerAttr *vklp = nvalloc(sizeof(VkLayerAttr));
        VkDeviceAttr *vkdp = nvalloc(sizeof(VkDeviceAttr));
        CtrlTarget *t = node->t;

        /* No handle, move on */
        if (!t->h) return;

        nv_msg(NULL, "Vulkan Information for %s:", t->name);

        /* Get Vulkan information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_VK_API_VERSION,
                                          &vk_api_version);
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }

        nv_msg(NULL, "Vulkan API version string: %s",
               NULL_TO_EMPTY(vk_api_version));
        nv_msg("", "");

        /* Query Layers and Extensions */

        status = NvCtrlGetVoidAttribute(t,
                                        NV_CTRL_ATTR_VK_LAYER_INFO,
                                        (void *)(vklp));
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }


        /* Instance Extensions */
        nv_msg("", "### Instance Extensions - %d ###",
               vklp->inst_extensions_count);

        for (i = 0; i < vklp->inst_extensions_count; i++) {
            print_extension_property(&vklp->inst_extensions[i],"  ");
        }
        nv_msg("","");

        /* Layers and Layer Extensions */
        nv_msg("", "### Layers - %d ###\n", vklp->inst_layer_properties_count);

        for (i = 0; i < vklp->inst_layer_properties_count; i++) {
            int d;
            char *vstr = vulkan_get_version_string(
                             vklp->inst_layer_properties[i].specVersion);

            nv_msg("  ", "Name: %s",
                   vklp->inst_layer_properties[i].layerName);
            nv_msg("    ", "Description: %s",
                   vklp->inst_layer_properties[i].description);
            nv_msg("    ", "Version: %s - Implementation: %d",
                   vstr, vklp->inst_layer_properties[i].implementationVersion);
            nvfree(vstr);

            nv_msg("    ", "Layer Extensions: %d",
                   vklp->layer_extensions_count[i]);

            if (vklp->layer_extensions_count[i] > 0) {
                for (j = 0; j < vklp->layer_extensions_count[i]; j++) {
                    print_extension_property(&vklp->layer_extensions[i][j],
                                             "      ");
                }
            }

            /* Layer-Device Extensions */

            for (d = 0; d < vklp->phy_devices_count; d++) {

                nv_msg("    ", "Device %d", d);

                nv_msg("      ", "Layer-Device Extensions: %d",
                       vklp->layer_device_extensions_count[d][i]);

                for (j = 0;
                     j < vklp->layer_device_extensions_count[d][i];
                     j++) {
                    print_extension_property(
                        &vklp->layer_device_extensions[d][i][j], "      ");
                }
            }
            nv_msg("", "");
        }
        nv_msg("", "");

        /* Query Device */

        status = NvCtrlGetVoidAttribute(t,
                                        NV_CTRL_ATTR_VK_DEVICE_INFO,
                                        (void *)(vkdp));
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }

        nv_msg("", "### Physical Devices - %d ###", vkdp->phy_devices_count);
        for (i = 0; i < vkdp->phy_devices_count; i++) {
            int j;
            char *str;

            /* Device Properties */
            str = vulkan_get_version_string(
                      vkdp->phy_device_properties[i].apiVersion);
            nv_msg("  ", "Device Name:    %s",
                   vkdp->phy_device_properties[i].deviceName);
            nv_msg("  ", "Device Type:    %s",
                   vulkan_get_physical_device_type(
                       vkdp->phy_device_properties[i].deviceType));
            nv_msg("  ", "API Version:    %s", str);
            nv_msg("  ", "Driver Version: %#x",
                   vkdp->phy_device_properties[i].driverVersion);
            nv_msg("  ", "Vendor ID:      %#x",
                   vkdp->phy_device_properties[i].vendorID);
            nv_msg("  ", "Device ID:      %#x",
                   vkdp->phy_device_properties[i].deviceID);
            if (vkdp->phy_device_uuid && vkdp->phy_device_uuid[i]) {
                nv_msg("  ", "Device UUID:    %s",
                       vkdp->phy_device_uuid[i]);
            }
            nvfree(str);

            nv_msg("  ", "Device Extensions:");
            if (!vkdp->device_extensions_count[i]) {
                nv_msg("    ", "None");
            } else {
                for (j = 0; j < vkdp->device_extensions_count[i]; j++) {
                    print_extension_property(
                        &vkdp->device_extensions[i][j], "    ");
                }
            }


            nv_msg("  ", "Sparse Properties:");
#define PRINT_SPARSE_FEATURES(var)                                        \
            nv_msg("    ", "%-41s: %s", #var,                             \
                   vkdp->phy_device_properties[i].sparseProperties.var ?  \
                       "yes" : "no");
            PRINT_SPARSE_FEATURES(residencyStandard2DBlockShape)
            PRINT_SPARSE_FEATURES(residencyStandard2DMultisampleBlockShape)
            PRINT_SPARSE_FEATURES(residencyStandard3DBlockShape)
            PRINT_SPARSE_FEATURES(residencyAlignedMipSize)
            PRINT_SPARSE_FEATURES(residencyNonResidentStrict)
#undef PRINT_SPARSE_FEATURES

            nv_msg("  ", "Limits:");
#define PRINT_LIMITS_UINT(var)                                  \
            nv_msg("    ", "%-48s: %u", #var,                   \
                   vkdp->phy_device_properties[i].limits.var);
#define PRINT_LIMITS_ULONG(var)                                 \
            nv_msg("    ", "%-48s: %lu", #var,                  \
                   (unsigned long)vkdp->phy_device_properties[i].limits.var);
#define PRINT_LIMITS_FLOAT(var)                                 \
            nv_msg("    ", "%-48s: %f", #var,                   \
                   vkdp->phy_device_properties[i].limits.var);
#define PRINT_LIMITS_SIZE(var)                                  \
            nv_msg("    ", "%-48s: %zu", #var,                  \
                   vkdp->phy_device_properties[i].limits.var);
            PRINT_LIMITS_UINT (maxImageDimension1D)
            PRINT_LIMITS_UINT (maxImageDimension2D)
            PRINT_LIMITS_UINT (maxImageDimension3D)
            PRINT_LIMITS_UINT (maxImageDimensionCube)
            PRINT_LIMITS_UINT (maxImageArrayLayers)
            PRINT_LIMITS_UINT (maxTexelBufferElements)
            PRINT_LIMITS_UINT (maxUniformBufferRange)
            PRINT_LIMITS_UINT (maxStorageBufferRange)
            PRINT_LIMITS_UINT (maxPushConstantsSize)
            PRINT_LIMITS_UINT (maxMemoryAllocationCount)
            PRINT_LIMITS_UINT (maxSamplerAllocationCount)
            PRINT_LIMITS_ULONG(bufferImageGranularity)
            PRINT_LIMITS_ULONG(sparseAddressSpaceSize)
            PRINT_LIMITS_UINT (maxBoundDescriptorSets)
            PRINT_LIMITS_UINT (maxPerStageDescriptorSamplers)
            PRINT_LIMITS_UINT (maxPerStageDescriptorUniformBuffers)
            PRINT_LIMITS_UINT (maxPerStageDescriptorStorageBuffers)
            PRINT_LIMITS_UINT (maxPerStageDescriptorSampledImages)
            PRINT_LIMITS_UINT (maxPerStageDescriptorStorageImages)
            PRINT_LIMITS_UINT (maxPerStageDescriptorInputAttachments)
            PRINT_LIMITS_UINT (maxPerStageResources)
            PRINT_LIMITS_UINT (maxDescriptorSetSamplers)
            PRINT_LIMITS_UINT (maxDescriptorSetUniformBuffers)
            PRINT_LIMITS_UINT (maxDescriptorSetUniformBuffersDynamic)
            PRINT_LIMITS_UINT (maxDescriptorSetStorageBuffers)
            PRINT_LIMITS_UINT (maxDescriptorSetStorageBuffersDynamic)
            PRINT_LIMITS_UINT (maxDescriptorSetSampledImages)
            PRINT_LIMITS_UINT (maxDescriptorSetStorageImages)
            PRINT_LIMITS_UINT (maxDescriptorSetInputAttachments)
            PRINT_LIMITS_UINT (maxVertexInputAttributes)
            PRINT_LIMITS_UINT (maxVertexInputBindings)
            PRINT_LIMITS_UINT (maxVertexInputAttributeOffset)
            PRINT_LIMITS_UINT (maxVertexInputBindingStride)
            PRINT_LIMITS_UINT (maxVertexOutputComponents)
            PRINT_LIMITS_UINT (maxTessellationGenerationLevel)
            PRINT_LIMITS_UINT (maxTessellationPatchSize)
            PRINT_LIMITS_UINT (maxTessellationControlPerVertexInputComponents)
            PRINT_LIMITS_UINT (maxTessellationControlPerVertexOutputComponents)
            PRINT_LIMITS_UINT (maxTessellationControlPerPatchOutputComponents)
            PRINT_LIMITS_UINT (maxTessellationControlTotalOutputComponents)
            PRINT_LIMITS_UINT (maxTessellationEvaluationInputComponents)
            PRINT_LIMITS_UINT (maxTessellationEvaluationOutputComponents)
            PRINT_LIMITS_UINT (maxGeometryShaderInvocations)
            PRINT_LIMITS_UINT (maxGeometryInputComponents)
            PRINT_LIMITS_UINT (maxGeometryOutputComponents)
            PRINT_LIMITS_UINT (maxGeometryOutputVertices)
            PRINT_LIMITS_UINT (maxGeometryTotalOutputComponents)
            PRINT_LIMITS_UINT (maxFragmentInputComponents)
            PRINT_LIMITS_UINT (maxFragmentOutputAttachments)
            PRINT_LIMITS_UINT (maxFragmentDualSrcAttachments)
            PRINT_LIMITS_UINT (maxFragmentCombinedOutputResources)
            PRINT_LIMITS_UINT (maxComputeSharedMemorySize)
            PRINT_LIMITS_UINT (maxComputeWorkGroupCount[0])
            PRINT_LIMITS_UINT (maxComputeWorkGroupCount[1])
            PRINT_LIMITS_UINT (maxComputeWorkGroupCount[2])
            PRINT_LIMITS_UINT (maxComputeWorkGroupInvocations)
            PRINT_LIMITS_UINT (maxComputeWorkGroupSize[0])
            PRINT_LIMITS_UINT (maxComputeWorkGroupSize[1])
            PRINT_LIMITS_UINT (maxComputeWorkGroupSize[2])
            PRINT_LIMITS_UINT (subPixelPrecisionBits)
            PRINT_LIMITS_UINT (subTexelPrecisionBits)
            PRINT_LIMITS_UINT (mipmapPrecisionBits)
            PRINT_LIMITS_UINT (maxDrawIndexedIndexValue)
            PRINT_LIMITS_UINT (maxDrawIndirectCount)
            PRINT_LIMITS_FLOAT(maxSamplerLodBias)
            PRINT_LIMITS_FLOAT(maxSamplerAnisotropy)
            PRINT_LIMITS_UINT (maxViewports)
            PRINT_LIMITS_UINT (maxViewportDimensions[0])
            PRINT_LIMITS_UINT (maxViewportDimensions[1])
            PRINT_LIMITS_FLOAT(viewportBoundsRange[0])
            PRINT_LIMITS_FLOAT(viewportBoundsRange[1])
            PRINT_LIMITS_UINT (viewportSubPixelBits)
            PRINT_LIMITS_SIZE (minMemoryMapAlignment)
            PRINT_LIMITS_ULONG(minTexelBufferOffsetAlignment)
            PRINT_LIMITS_ULONG(minUniformBufferOffsetAlignment)
            PRINT_LIMITS_ULONG(minStorageBufferOffsetAlignment)
            PRINT_LIMITS_UINT (minTexelOffset)
            PRINT_LIMITS_UINT (maxTexelOffset)
            PRINT_LIMITS_UINT (minTexelGatherOffset)
            PRINT_LIMITS_UINT (maxTexelGatherOffset)
            PRINT_LIMITS_FLOAT(minInterpolationOffset)
            PRINT_LIMITS_FLOAT(maxInterpolationOffset)
            PRINT_LIMITS_UINT (subPixelInterpolationOffsetBits)
            PRINT_LIMITS_UINT (maxFramebufferWidth)
            PRINT_LIMITS_UINT (maxFramebufferHeight)
            PRINT_LIMITS_UINT (maxFramebufferLayers)
            PRINT_LIMITS_UINT (framebufferColorSampleCounts)
            PRINT_LIMITS_UINT (framebufferDepthSampleCounts)
            PRINT_LIMITS_UINT (framebufferStencilSampleCounts)
            PRINT_LIMITS_UINT (framebufferNoAttachmentsSampleCounts)
            PRINT_LIMITS_UINT (maxColorAttachments)
            PRINT_LIMITS_UINT (sampledImageColorSampleCounts)
            PRINT_LIMITS_UINT (sampledImageIntegerSampleCounts)
            PRINT_LIMITS_UINT (sampledImageDepthSampleCounts)
            PRINT_LIMITS_UINT (sampledImageStencilSampleCounts)
            PRINT_LIMITS_UINT (storageImageSampleCounts)
            PRINT_LIMITS_UINT (maxSampleMaskWords)
            PRINT_LIMITS_UINT (timestampComputeAndGraphics)
            PRINT_LIMITS_FLOAT(timestampPeriod)
            PRINT_LIMITS_UINT (maxClipDistances)
            PRINT_LIMITS_UINT (maxCullDistances)
            PRINT_LIMITS_UINT (maxCombinedClipAndCullDistances)
            PRINT_LIMITS_UINT (discreteQueuePriorities)
            PRINT_LIMITS_FLOAT(pointSizeRange[0])
            PRINT_LIMITS_FLOAT(pointSizeRange[1])
            PRINT_LIMITS_FLOAT(lineWidthRange[0])
            PRINT_LIMITS_FLOAT(lineWidthRange[1])
            PRINT_LIMITS_FLOAT(pointSizeGranularity)
            PRINT_LIMITS_FLOAT(lineWidthGranularity)
            PRINT_LIMITS_UINT(strictLines)
            PRINT_LIMITS_UINT(standardSampleLocations)
            PRINT_LIMITS_ULONG(optimalBufferCopyOffsetAlignment)
            PRINT_LIMITS_ULONG(optimalBufferCopyRowPitchAlignment)
            PRINT_LIMITS_ULONG(nonCoherentAtomSize)
#undef PRINT_LIMITS_UINT
#undef PRINT_LIMITS_ULONG
#undef PRINT_LIMITS_FLOAT
#undef PRINT_LIMITS_SIZE

            /* Device Features */
            nv_msg("  ", "Features:");
#define PRINT_DEVICE_FEATURE(var) nv_msg("    ", "%-39s: %s", #var,            \
                                         (vkdp->features[i].var? "yes" : "no"));
            PRINT_DEVICE_FEATURE(robustBufferAccess)
            PRINT_DEVICE_FEATURE(fullDrawIndexUint32)
            PRINT_DEVICE_FEATURE(imageCubeArray)
            PRINT_DEVICE_FEATURE(independentBlend)
            PRINT_DEVICE_FEATURE(geometryShader)
            PRINT_DEVICE_FEATURE(tessellationShader)
            PRINT_DEVICE_FEATURE(sampleRateShading)
            PRINT_DEVICE_FEATURE(dualSrcBlend)
            PRINT_DEVICE_FEATURE(logicOp)
            PRINT_DEVICE_FEATURE(multiDrawIndirect)
            PRINT_DEVICE_FEATURE(drawIndirectFirstInstance)
            PRINT_DEVICE_FEATURE(depthClamp)
            PRINT_DEVICE_FEATURE(depthBiasClamp)
            PRINT_DEVICE_FEATURE(fillModeNonSolid)
            PRINT_DEVICE_FEATURE(depthBounds)
            PRINT_DEVICE_FEATURE(wideLines)
            PRINT_DEVICE_FEATURE(largePoints)
            PRINT_DEVICE_FEATURE(alphaToOne)
            PRINT_DEVICE_FEATURE(multiViewport)
            PRINT_DEVICE_FEATURE(samplerAnisotropy)
            PRINT_DEVICE_FEATURE(textureCompressionETC2)
            PRINT_DEVICE_FEATURE(textureCompressionASTC_LDR)
            PRINT_DEVICE_FEATURE(textureCompressionBC)
            PRINT_DEVICE_FEATURE(occlusionQueryPrecise)
            PRINT_DEVICE_FEATURE(pipelineStatisticsQuery)
            PRINT_DEVICE_FEATURE(vertexPipelineStoresAndAtomics)
            PRINT_DEVICE_FEATURE(fragmentStoresAndAtomics)
            PRINT_DEVICE_FEATURE(shaderTessellationAndGeometryPointSize)
            PRINT_DEVICE_FEATURE(shaderImageGatherExtended)
            PRINT_DEVICE_FEATURE(shaderStorageImageExtendedFormats)
            PRINT_DEVICE_FEATURE(shaderStorageImageMultisample)
            PRINT_DEVICE_FEATURE(shaderStorageImageReadWithoutFormat)
            PRINT_DEVICE_FEATURE(shaderStorageImageWriteWithoutFormat)
            PRINT_DEVICE_FEATURE(shaderUniformBufferArrayDynamicIndexing)
            PRINT_DEVICE_FEATURE(shaderSampledImageArrayDynamicIndexing)
            PRINT_DEVICE_FEATURE(shaderStorageBufferArrayDynamicIndexing)
            PRINT_DEVICE_FEATURE(shaderStorageImageArrayDynamicIndexing)
            PRINT_DEVICE_FEATURE(shaderClipDistance)
            PRINT_DEVICE_FEATURE(shaderCullDistance)
            PRINT_DEVICE_FEATURE(shaderFloat64)
            PRINT_DEVICE_FEATURE(shaderInt64)
            PRINT_DEVICE_FEATURE(shaderInt16)
            PRINT_DEVICE_FEATURE(shaderResourceResidency)
            PRINT_DEVICE_FEATURE(shaderResourceMinLod)
            PRINT_DEVICE_FEATURE(sparseBinding)
            PRINT_DEVICE_FEATURE(sparseResidencyBuffer)
            PRINT_DEVICE_FEATURE(sparseResidencyImage2D)
            PRINT_DEVICE_FEATURE(sparseResidencyImage3D)
            PRINT_DEVICE_FEATURE(sparseResidency2Samples)
            PRINT_DEVICE_FEATURE(sparseResidency4Samples)
            PRINT_DEVICE_FEATURE(sparseResidency8Samples)
            PRINT_DEVICE_FEATURE(sparseResidency16Samples)
            PRINT_DEVICE_FEATURE(sparseResidencyAliased)
            PRINT_DEVICE_FEATURE(variableMultisampleRate)
            PRINT_DEVICE_FEATURE(inheritedQueries)
#undef PRINT_DEVICE_FEATURE
            nv_msg("", "");

            /* Memory Properties */
            nv_msg("", "### Memory Type Properties - %d ###",
                   vkdp->memory_properties[i].memoryTypeCount);
            for (j = 0; j < vkdp->memory_properties[i].memoryTypeCount; j++) {
                char *mstr = vulkan_get_memory_property_flags(
                    vkdp->memory_properties[i].memoryTypes[j].propertyFlags);
                nv_msg("  ", "Memory Type [%d]", j);
                nv_msg("    ", "Heap Index: %d",
                       vkdp->memory_properties[i].memoryTypes[j].heapIndex);
                nv_msg("    ", "Flags     :%s", mstr);
                nvfree(mstr);
            }
            nv_msg("", "");

            nv_msg("", "### Memory Heap Properties - %d ###",
                   vkdp->memory_properties[i].memoryHeapCount);
            for (j = 0; j < vkdp->memory_properties[i].memoryHeapCount; j++) {
                char *mstr = vulkan_get_memory_heap_flags(
                    vkdp->memory_properties[i].memoryHeaps[j].flags);
                nv_msg("  ", "Memory Heap [%d]", j);

                nv_msg("    ", "Size : %" PRIu64 "",
                        vkdp->memory_properties[i].memoryHeaps[j].size);
                nv_msg("    ", "Flags:%s", mstr);
                nvfree(mstr);
            }
            nv_msg("", "");

            /* Queue Properties */
            nv_msg("", "### Queue Properties - %d ###",
                   vkdp->queue_properties_count[i]);
            for (j = 0; j < vkdp->queue_properties_count[i]; j++) {
                VkExtent3D e =
                    vkdp->queue_properties[i][j].minImageTransferGranularity;
                char *qstr =
                    vulkan_get_queue_family_flags(
                        vkdp->queue_properties[i][j].queueFlags);
                nv_msg("  ", "Queue [%d]", j);

                nv_msg("    ", "Flags:%s", qstr);
                nv_msg("    ", "Count: %d",
                       vkdp->queue_properties[i][j].queueCount);
                nv_msg("    ",
                       "Min Image Transfer Granularity (WxHxD): %dx%dx%d",
                       e.width, e.height, e.depth);
                nvfree(qstr);
            }
            nv_msg("", "");

            nv_msg("", "### Formats ###");
            for (j = 0; j < vkdp->formats_count[i]; j++) {
                nv_msg("    ", "Format [%d] - Linear : 0x%x",
                       j, vkdp->formats[i][j].linearTilingFeatures);
                print_vulkan_format_feature_flags(
                    vkdp->formats[i][j].linearTilingFeatures);
                nv_msg("    ", "Format [%d] - Optimal: 0x%x",
                       j, vkdp->formats[i][j].optimalTilingFeatures);
                print_vulkan_format_feature_flags(
                    vkdp->formats[i][j].optimalTilingFeatures);
                nv_msg("    ", "Format [%d] - Buffer : 0x%x",
                       j, vkdp->formats[i][j].bufferFeatures);
                print_vulkan_format_feature_flags(
                    vkdp->formats[i][j].bufferFeatures);
                nv_msg("", "");
            }
            nv_msg("", "");

        }

        NvCtrlFreeVkLayerAttr(vklp);
        nvfree(vklp);
        NvCtrlFreeVkDeviceAttr(vkdp);
        nvfree(vkdp);

    } /* Done looking at all screens */


    /* Fall through */
 finish:
    if (status == NvCtrlError) {
        nv_error_msg("Error fetching Vulcan Information: %s",
                     NvCtrlAttributesStrError(status) );
    }

    /* Free any leftover memory used */
    SAFE_FREE(vk_api_version);

    NvCtrlFreeAllSystems(systems);

} /* print_vulkaninfo() */

