/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2024 NVIDIA Corporation.
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

#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"

#include "NVCtrlLib.h"

#include "common-utils.h"
#include "msg.h"

#include "parse.h"

#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xvlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <sys/utsname.h>

#include <dlfcn.h>  /* To dynamically load libvulkan.so */
#include <vulkan/vulkan.h>


typedef struct __libVkInfoRec {

    /* libvulkan.so library handle */
    void *handle;
    int   ref_count; /* # users of the library */

    /* Vulkan functions used */

    VkResult (*vkCreateInstance)(const VkInstanceCreateInfo *,
                                 const VkAllocationCallbacks *, VkInstance *);

    void     (*vkDestroyInstance)(VkInstance, const VkAllocationCallbacks *);

    PFN_vkVoidFunction (*vkGetInstanceProcAddr)(VkInstance, const char *);

    VkResult (*vkEnumerateInstanceLayerProperties)(uint32_t *,
                                                   VkLayerProperties *);
    VkResult (*vkEnumerateInstanceExtensionProperties)(const char *,
                                                       uint32_t *,
                                                       VkExtensionProperties *);

    VkResult (*vkEnumerateDeviceExtensionProperties)(VkPhysicalDevice,
                                                     const char *,
                                                     uint32_t *,
                                                     VkExtensionProperties *);

    VkResult (*vkEnumeratePhysicalDevices)(VkInstance instance, uint32_t *,
                                           VkPhysicalDevice *);

    VkResult (*vkGetPhysicalDeviceProperties)(VkPhysicalDevice,
                                              VkPhysicalDeviceProperties *);

#if defined(VK_KHR_get_physical_device_properties2)
    void (*vkGetPhysicalDeviceProperties2)(VkPhysicalDevice,
                                           VkPhysicalDeviceProperties2 *);
#endif

    VkResult (*vkGetPhysicalDeviceFeatures)(VkPhysicalDevice,
                                            VkPhysicalDeviceFeatures *);

    VkResult (*vkGetPhysicalDeviceQueueFamilyProperties)(
                 VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties *);
    VkResult (*vkGetPhysicalDeviceFormatProperties)(VkPhysicalDevice,
                                                    VkFormat,
                                                    VkFormatProperties *);
    VkResult (*vkGetPhysicalDeviceMemoryProperties)(
                 VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *);

    VkResult (*vkEnumerateInstanceVersion)(uint32_t *);


} __libVkInfo;

static __libVkInfo *__libVk = NULL;




/******************************************************************************
 *
 * Opens libvulkan for usage
 *
 ****/

static Bool open_libvk(void)
{
    const char *error_str = NULL;


    /* Initialize bookkeeping structure */
    if ( !__libVk ) {
        __libVk = nvalloc(sizeof(__libVkInfo));
    }


    /* Library was already opened */
    if ( __libVk->handle ) {
        __libVk->ref_count++;
        return True;
    }


    /* We are the first to open the library */
    __libVk->handle = dlopen("libvulkan.so.1", RTLD_LAZY);
    if ( !__libVk->handle ) {
        error_str = dlerror();
        goto fail;
    }

    /* Resolve Vulkan functions */
    __libVk->vkEnumerateInstanceLayerProperties =
        NV_DLSYM(__libVk->handle, "vkEnumerateInstanceLayerProperties");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkEnumerateInstanceExtensionProperties =
        NV_DLSYM(__libVk->handle, "vkEnumerateInstanceExtensionProperties");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkCreateInstance = NV_DLSYM(__libVk->handle, "vkCreateInstance");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkGetInstanceProcAddr =
        NV_DLSYM(__libVk->handle, "vkGetInstanceProcAddr");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkEnumeratePhysicalDevices =
        NV_DLSYM(__libVk->handle, "vkEnumeratePhysicalDevices");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkGetPhysicalDeviceProperties =
        NV_DLSYM(__libVk->handle, "vkGetPhysicalDeviceProperties");
    if ((error_str = dlerror()) != NULL) goto fail;

#if defined(VK_KHR_get_physical_device_properties2)
    __libVk->vkGetPhysicalDeviceProperties2 =
        NV_DLSYM(__libVk->handle, "vkGetPhysicalDeviceProperties2");
#endif

    __libVk->vkGetPhysicalDeviceQueueFamilyProperties =
        NV_DLSYM(__libVk->handle, "vkGetPhysicalDeviceQueueFamilyProperties");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkDestroyInstance =
        NV_DLSYM(__libVk->handle, "vkDestroyInstance");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkEnumerateDeviceExtensionProperties =
        NV_DLSYM(__libVk->handle, "vkEnumerateDeviceExtensionProperties");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkGetPhysicalDeviceFormatProperties =
        NV_DLSYM(__libVk->handle, "vkGetPhysicalDeviceFormatProperties");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkGetPhysicalDeviceMemoryProperties =
        NV_DLSYM(__libVk->handle, "vkGetPhysicalDeviceMemoryProperties");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkGetPhysicalDeviceFeatures =
        NV_DLSYM(__libVk->handle, "vkGetPhysicalDeviceFeatures");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libVk->vkEnumerateInstanceVersion =
        NV_DLSYM(__libVk->handle, "vkEnumerateInstanceVersion");


    /* Up the ref count */
    __libVk->ref_count++;

    return True;


    /* Handle failures */
 fail:
    if ( error_str ) {
        nv_warning_msg("libVulkan setup error : %s\n", error_str);
    }
    if ( __libVk ) {
        assert(__libVk->ref_count == 0);
        if ( __libVk->handle ) {
            dlclose(__libVk->handle);
            __libVk->handle = NULL;
        }
        free(__libVk);
        __libVk = NULL;
    }
    return False;

} /* open_libvk() */



/******************************************************************************
 *
 * Closes libVulkan -
 *
 ****/

static void close_libvk(void)
{
    if ( __libVk && __libVk->handle && __libVk->ref_count ) {
        __libVk->ref_count--;
        if ( __libVk->ref_count == 0 ) {
            dlclose(__libVk->handle);
            __libVk->handle = NULL;
            free(__libVk);
            __libVk = NULL;
        }
    }
} /* close_libvk() */



/******************************************************************************
 *
 * NvCtrlInitVkAttributes()
 *
 * Initializes the NvCtrlVkAttributes Extension by linking the libvulkan.so.1
 * and resolving functions used to retrieve Vulkan information.
 *
 ****/

Bool NvCtrlInitVkAttributes (NvCtrlAttributePrivateHandle *h)
{
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "nvidia-setings",
        .applicationVersion = 1,
        .pEngineName = "nvidia-settings",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 1, 0),
    };

    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = 0,      // Extension Count
        .ppEnabledExtensionNames = NULL, // Extension Names
    };
    VkResult res;

    /* Check parameters */
    if ( !h || !h->dpy || h->target_type != GPU_TARGET ) {
        return False;
    }


    /* Open libvulkan.so.1 */
    if ( !open_libvk() ) {
        return False;
    }

    /* Create Instance */
    if (!h->vk_instance) {
        /* The Vulkan library only enumerates physical devices associated with
         * the DISPLAY environment variable used. To avoid this, we clear that
         * variable and we will get all the devices.
         */
        char *orig_display_value = getenv("DISPLAY");
        setenv("DISPLAY", "", 1);

        h->vk_instance = (VkInstance*) nvalloc(sizeof(VkInstance));
        res = __libVk->vkCreateInstance(&inst_info, NULL, h->vk_instance);
        if (res) {
            nvfree(h->vk_instance);
            h->vk_instance = NULL;
            return False;
        }

        /* Load remaining function pointers */
#if defined(VK_KHR_get_physical_device_properties2)
        if (!__libVk->vkGetPhysicalDeviceProperties2) {
            __libVk->vkGetPhysicalDeviceProperties2 =
                (PFN_vkGetPhysicalDeviceProperties2)
                    __libVk->vkGetInstanceProcAddr(
                        *h->vk_instance, "vkGetPhysicalDeviceProperties2KHR");
        }

        if (!__libVk->vkGetPhysicalDeviceProperties2) {
            __libVk->vkGetPhysicalDeviceProperties2 =
                (PFN_vkGetPhysicalDeviceProperties2)
                    __libVk->vkGetInstanceProcAddr(
                        *h->vk_instance, "vkGetPhysicalDeviceProperties2");
        }
#endif

#if defined(PFN_vkEnumerateInstanceVersion)
        if (!__libVk->vkEnumerateInstanceVersion) {
            __libVk->vkEnumerateInstanceVersion =
                (PFN_vkEnumerateInstanceVersion)
                    __libVk->vkGetInstanceProcAddr(
                        VK_NULL_HANDLE, "vkEnumerateInstanceVersion");
        }
#else
        __libVk->vkEnumerateInstanceVersion = NULL;
#endif

        setenv("DISPLAY", orig_display_value, 1);
    }


    return True;

} /* NvCtrlInitVkAttributes() */



/******************************************************************************
 *
 * NvCtrlVkAttributesClose()
 *
 * Frees and relinquishes any resource used by the NvCtrlVkAttributes
 * extension.
 *
 ****/

void
NvCtrlVkAttributesClose (NvCtrlAttributePrivateHandle *h)
{
    if ( !h || !h->vulkan ) {
        return;
    }

    if (h->vk_instance) {
        __libVk->vkDestroyInstance(*h->vk_instance, NULL);
        nvfree(h->vk_instance);
        h->vk_instance = NULL;
    }

    close_libvk();

    h->vulkan = False;

} /* NvCtrlVkAttributesClose() */




/******************************************************************************
 *
 * NvCtrlVkGetVoidAttribute()
 *
 * Retrieves various Vulkan attributes (other than strings and ints)
 *
 ****/

ReturnStatus NvCtrlVkGetVoidAttribute(const NvCtrlAttributePrivateHandle *h,
                                      unsigned int display_mask, int attr,
                                      void **ptr)
{
    uint32_t count;
    uint32_t version;
    VkResult res;
    VkLayerAttr *vklp;
    VkDeviceAttr *vkdp;
    VkPhysicalDevice *phy_devices;

    char *vstr;
    int i, j;


    /* Validate */
    if (!h || !h->dpy || h->target_type != GPU_TARGET) {
        return NvCtrlBadHandle;
    }
    if (!h->vulkan || !__libVk) {
        return NvCtrlMissingExtension;
    }
    if (!ptr) {
        return NvCtrlBadArgument;
    }


    /* Fetch the right attribute */
    switch (attr) {

    case NV_CTRL_ATTR_VK_LAYER_INFO:

        vklp = (VkLayerAttr *)(ptr);

        res = __libVk->vkEnumerateInstanceLayerProperties(&count, NULL);
        if (res) return NvCtrlError;
        if (count == 0) return NvCtrlSuccess;

        if (__libVk->vkEnumerateInstanceVersion) {
            __libVk->vkEnumerateInstanceVersion(&version);
            vstr = nvasprintf("%d.%d.%d", VK_VERSION_MAJOR(version),
                                          VK_VERSION_MINOR(version),
                                          VK_VERSION_PATCH(version));
            vklp->instance_version = vstr;
        } else {
            vklp->instance_version = NULL;
        }

        vklp->inst_layer_properties = nvalloc(count *
                                              sizeof(VkLayerProperties));
        vklp->layer_extensions = nvalloc(count * sizeof(VkLayerProperties*));
        vklp->layer_extensions_count = nvalloc(count * sizeof(uint32_t));

        res = __libVk->vkEnumerateInstanceLayerProperties(
                  &count,
                  vklp->inst_layer_properties);
        if (res) return NvCtrlError;

        vklp->inst_layer_properties_count = count;

        for (i = 0; i < vklp->inst_layer_properties_count; i++) {

            // per Layer Extension Properties
            res = __libVk->vkEnumerateInstanceExtensionProperties(
                      vklp->inst_layer_properties[i].layerName,
                      &count,
                      NULL);
            if (res) return NvCtrlError;
            if (count == 0) continue;

            vklp->layer_extensions[i] =
                nvalloc(count * sizeof(VkExtensionProperties));

            res = __libVk->vkEnumerateInstanceExtensionProperties(
                      vklp->inst_layer_properties[i].layerName,
                      &count,
                      vklp->layer_extensions[i]);
            if (res) return NvCtrlError;

            vklp->layer_extensions_count[i] = count;
        }


        // Global Instance Extension Properties
        res = __libVk->vkEnumerateInstanceExtensionProperties(NULL,
                                                              &count,
                                                              NULL);
        if (res) return NvCtrlError;

        vklp->inst_extensions =
            nvalloc(count * sizeof(VkExtensionProperties));
        vklp->inst_extensions_count = count;

        res = __libVk->vkEnumerateInstanceExtensionProperties(
                  NULL,
                  &count,
                  vklp->inst_extensions);
        if (res) return NvCtrlError;


        // Device-Layer Extension Properties
        res = __libVk->vkEnumeratePhysicalDevices(*h->vk_instance,
                                                  &count, NULL);
        if (res) return NvCtrlError;

        phy_devices = nvalloc(count * sizeof(VkPhysicalDevice));
        vklp->phy_devices_count = count;

        vklp->layer_device_extensions_count =
            nvalloc(count * sizeof(uint32_t**));
        vklp->layer_device_extensions =
            nvalloc(count * sizeof(VkExtensionProperties**));

        res = __libVk->vkEnumeratePhysicalDevices(*h->vk_instance, &count,
                                                  phy_devices);
        if (res) return NvCtrlError;

        for (i = 0; i < vklp->phy_devices_count; i++) {

            if (vklp->inst_layer_properties_count == 0) {
                continue;
            }

            vklp->layer_device_extensions[i] =
                nvalloc(vklp->inst_layer_properties_count *
                        sizeof(VkExtensionProperties*));
            vklp->layer_device_extensions_count[i] =
                nvalloc(vklp->inst_layer_properties_count * sizeof(uint32_t*));


            for (j = 0; j < vklp->inst_layer_properties_count; j++) {

                res = __libVk->vkEnumerateDeviceExtensionProperties(
                          phy_devices[i],
                          vklp->inst_layer_properties[j].layerName,
                          &count, NULL);
                if (res) return NvCtrlError;

                vklp->layer_device_extensions_count[i][j] = count;
                if (count == 0) {
                    vklp->layer_device_extensions[i][j] = NULL;
                    continue;
                }

                vklp->layer_device_extensions[i][j] =
                    nvalloc(count * sizeof(VkExtensionProperties));

                res = __libVk->vkEnumerateDeviceExtensionProperties(
                          phy_devices[i],
                          vklp->inst_layer_properties[j].layerName,
                          &count,
                          vklp->layer_device_extensions[i][j]);
                if (res) return NvCtrlError;

            }
        }

        nvfree(phy_devices);


        break;

    case NV_CTRL_ATTR_VK_DEVICE_INFO:

        vkdp = (VkDeviceAttr *)(ptr);

        res = __libVk->vkEnumeratePhysicalDevices(*h->vk_instance, &count,
                                                  NULL);
        if (res) return NvCtrlError;

        phy_devices = nvalloc(count * sizeof(VkPhysicalDevice));
        vkdp->phy_devices_count = count;

        vkdp->phy_device_properties =
            nvalloc(count * sizeof(VkPhysicalDeviceProperties));
        vkdp->features = nvalloc(count * sizeof(VkPhysicalDeviceFeatures));
        vkdp->memory_properties =
            nvalloc(count * sizeof(VkPhysicalDeviceMemoryProperties));

        vkdp->formats_count = nvalloc(count * sizeof(uint32_t));
        vkdp->formats = nvalloc(count * sizeof(VkFormatProperties*));

        vkdp->queue_properties_count = nvalloc(count * sizeof(uint32_t));
        vkdp->queue_properties =
            nvalloc(count * sizeof(VkQueueFamilyProperties*));

        vkdp->device_extensions =
            nvalloc(vkdp->phy_devices_count * sizeof(VkExtensionProperties*));
        vkdp->device_extensions_count =
            nvalloc(vkdp->phy_devices_count * sizeof(uint32_t));


        res = __libVk->vkEnumeratePhysicalDevices(*h->vk_instance, &count,
                                                  phy_devices);
        if (res) return NvCtrlError;

        for (i = 0; i < vkdp->phy_devices_count; i++) {
            int c;

#if defined(VK_KHR_get_physical_device_properties2)
            VkPhysicalDeviceProperties2 *pdp2 =
                nvalloc(sizeof(VkPhysicalDeviceProperties2));
            VkPhysicalDeviceIDProperties *pdidp =
                nvalloc(sizeof(VkPhysicalDeviceIDProperties));

            pdp2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            pdp2->pNext = pdidp;
            pdidp->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

            if (__libVk->vkGetPhysicalDeviceProperties2) {

                if (vkdp->phy_device_uuid == NULL) {
                    vkdp->phy_device_uuid = nvalloc(count * sizeof(char*));
                }

                __libVk->vkGetPhysicalDeviceProperties2(phy_devices[i], pdp2);

                vkdp->phy_device_uuid[i] =
                    nvasprintf("GPU-%02x%02x%02x%02x-"
                               "%02x%02x-%02x%02x-%02x%02x-"
                               "%02x%02x%02x%02x%02x%02x",
                               pdidp->deviceUUID[0],
                               pdidp->deviceUUID[1],
                               pdidp->deviceUUID[2],
                               pdidp->deviceUUID[3],
                               pdidp->deviceUUID[4],
                               pdidp->deviceUUID[5],
                               pdidp->deviceUUID[6],
                               pdidp->deviceUUID[7],
                               pdidp->deviceUUID[8],
                               pdidp->deviceUUID[9],
                               pdidp->deviceUUID[10],
                               pdidp->deviceUUID[11],
                               pdidp->deviceUUID[12],
                               pdidp->deviceUUID[13],
                               pdidp->deviceUUID[14],
                               pdidp->deviceUUID[15]);
            }
#endif

            // Device Extensions //
            res = __libVk->vkEnumerateDeviceExtensionProperties(
                      phy_devices[i], NULL, &count, NULL);
            if (res) return NvCtrlError;

            vkdp->device_extensions_count[i] = count;
            if (count == 0) {
                vkdp->device_extensions[i] = NULL;
                continue;
            }

            vkdp->device_extensions[i] =
                nvalloc(count * sizeof(VkExtensionProperties));

            res = __libVk->vkEnumerateDeviceExtensionProperties(
                      phy_devices[i], NULL, &count,
                      vkdp->device_extensions[i]);
            if (res) return NvCtrlError;

            __libVk->vkGetPhysicalDeviceProperties(
                phy_devices[i], &(vkdp->phy_device_properties[i]));

            __libVk->vkGetPhysicalDeviceFeatures(phy_devices[i],
                                                 &(vkdp->features[i]));
            __libVk->vkGetPhysicalDeviceMemoryProperties(
                phy_devices[i], &(vkdp->memory_properties[i]));

            // Format Properties //

            vkdp->formats_count[i] = (VK_FORMAT_ASTC_12x12_SRGB_BLOCK -
                                      VK_FORMAT_UNDEFINED) + 1;
            vkdp->formats_count[i] += (VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM -
                                       VK_FORMAT_G8B8G8R8_422_UNORM) + 1;

            vkdp->formats[i] =
                nvalloc(vkdp->formats_count[i] * sizeof(VkFormatProperties));

            for (j = 0, c = VK_FORMAT_UNDEFINED;
                 c <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK &&
                     j < vkdp->formats_count[i];
                 j++, c++) {
                __libVk->vkGetPhysicalDeviceFormatProperties(
                    phy_devices[i], (VkFormat)c, &(vkdp->formats[i][j]));
            }

            for (c = VK_FORMAT_G8B8G8R8_422_UNORM;
                 c <= VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
                 j++, c++) {
                __libVk->vkGetPhysicalDeviceFormatProperties(
                    phy_devices[i], (VkFormat)c, &(vkdp->formats[i][j]));
            }

            // Queue Family Properties //

            __libVk->vkGetPhysicalDeviceQueueFamilyProperties(phy_devices[i],
                                                              &count,
                                                              NULL);
            vkdp->queue_properties_count[i] = count;
            vkdp->queue_properties[i] =
                nvalloc(count * sizeof(VkQueueFamilyProperties));
            __libVk->vkGetPhysicalDeviceQueueFamilyProperties(
                phy_devices[i], &count, vkdp->queue_properties[i]);

        }

        nvfree(phy_devices);

        break;

    default:
        return NvCtrlNoAttribute;
        break;
    } /* Done fetching attribute */


    return NvCtrlSuccess;

} /* NvCtrlVkGetVoidAttribute */



void NvCtrlFreeVkLayerAttr(VkLayerAttr *vklp)
{
    int i, j;

    nvfree(vklp->inst_layer_properties);

    for (i = 0; i < vklp->inst_layer_properties_count; i++) {
        nvfree(vklp->layer_extensions[i]);
    }

    nvfree(vklp->instance_version);

    nvfree(vklp->layer_extensions);
    nvfree(vklp->layer_extensions_count);

    nvfree(vklp->inst_extensions);


    for (i = 0; i < vklp->phy_devices_count; i++) {
        for (j = 0;
             j < vklp->inst_layer_properties_count &&
                 vklp->layer_device_extensions[i] != NULL;
             j++) {
            nvfree(vklp->layer_device_extensions[i][j]);
        }
        nvfree(vklp->layer_device_extensions[i]);
        nvfree(vklp->layer_device_extensions_count[i]);
    }

    nvfree(vklp->layer_device_extensions_count);
    nvfree(vklp->layer_device_extensions);
}



void NvCtrlFreeVkDeviceAttr(VkDeviceAttr *vkdp)
{
    int device;

    nvfree(vkdp->phy_device_properties);
    nvfree(vkdp->features);
    nvfree(vkdp->memory_properties);
    nvfree(vkdp->queue_properties_count);

    for (device = 0; device < vkdp->phy_devices_count; device++) {
        nvfree(vkdp->device_extensions[device]);
        nvfree(vkdp->formats[device]);
        nvfree(vkdp->queue_properties[device]);
    }

    nvfree(vkdp->device_extensions_count);
    nvfree(vkdp->device_extensions);
    nvfree(vkdp->formats);
    nvfree(vkdp->queue_properties);
}


/******************************************************************************
 *
 * NvCtrlVkGetStringAttribute()
 *
 *
 * Retrieves a particular GLX information string by calling the appropriate
 * OpenGL/GLX function.
 *
 *
 * But first, the following helper function may be used to set up a rendering
 * context such that valid information may be retrieved. (Having a context is
 * required for getting OpenGL and 'Direct rendering' information.)
 *
 * NOTE: A separate display connection is used to avoid the dependence on
 *       libVulkan when an XCloseDisplay is issued.   If we did not, calling
 *       XCloseDisplay AFTER the libVulkan library has been dlclose'ed (after
 *       having made at least one GLX call) would cause a segfault.
 *
 ****/

/*
 * Helper function for NvCtrlVkGetStringAttribute for queries that require a
 * current context.  If getDirect is true, then check if we can create a direct
 * GLX context and return "Yes" or "No".  Otherwise, create a context and query
 * the GLX implementation for the string specified in phy_device_properties.
 */


#define APP_SHORT_NAME "nvidia-settings"
ReturnStatus NvCtrlVkGetStringAttribute(const NvCtrlAttributePrivateHandle *h,
                                         unsigned int display_mask, int attr,
                                         char **ptr)
{
    char s[16];

    /* Validate */
    if ( !h || h->target_type != GPU_TARGET ) {
        return NvCtrlBadHandle;
    }
    if ( !h->vulkan || !__libVk ) {
        return NvCtrlMissingExtension;
    }
    if ( !ptr ) {
        return NvCtrlBadArgument;
    }


    /* Get the right string */
    switch (attr) {
    case NV_CTRL_STRING_VK_API_VERSION:
        snprintf(s, 16, "%d.%d.%d", VK_VERSION_MAJOR(VK_API_VERSION_1_0),
                                    VK_VERSION_MINOR(VK_API_VERSION_1_0),
                                    VK_VERSION_PATCH(VK_HEADER_VERSION));
        *ptr = strdup(s);

        break;

    default:
        return NvCtrlNoAttribute;

    }


    /* Copy the string and return it */
    if ((*ptr) == NULL) {
        return NvCtrlError;
    }

    return NvCtrlSuccess;

} /* NvCtrlVkGetStringAttribute() */
