/*
 * Copyright (C) 2021 NVIDIA Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "nvpci-utils.h"

/*
 * libpciaccess stores the device class in bits 16-23, subclass in 8-15, and
 * interface in bits 0-7 of dev->device_class.  We care only about the class
 * and subclass.
 */
const uint32_t PCI_CLASS_DISPLAY_VGA = 0x30000;
const uint32_t PCI_CLASS_SUBCLASS_MASK = 0xffff00;

/*
 * nvpci_find_gpu_by_vendor() - use libpciaccess to find all VGA and 3D PCI
 * devices matching the passed-in vendor_id (which may be set to PCI_MATCH_ANY).
 * The caller is responsible for calling pci_system_init() before using this
 * function, and pci_system_cleanup() when libpciaccess is no longer needed.
 */
struct pci_device_iterator *nvpci_find_gpu_by_vendor(uint32_t vendor_id)
{
    const struct pci_id_match match = {
        .vendor_id = vendor_id,
        .device_id = PCI_MATCH_ANY,
        .subvendor_id = PCI_MATCH_ANY,
        .subdevice_id = PCI_MATCH_ANY,
        .device_class = PCI_CLASS_DISPLAY_VGA,
        /*
         * Ignore bit 1 of the subclass, to allow both 0x30000 (VGA controller)
         * and 0x30200 (3D controller).
         */
        .device_class_mask = PCI_CLASS_SUBCLASS_MASK & ~0x200,
    };

    return pci_id_match_iterator_create(&match);
}

/*
 * nvpci_dev_is_vga() - test whether the passed-in struct pci_device* has the
 * VGA device class 0x0300 (and not 3D class 0x0302).
 */
int nvpci_dev_is_vga(struct pci_device *dev)
{
    return (dev->device_class & PCI_CLASS_SUBCLASS_MASK) ==
           PCI_CLASS_DISPLAY_VGA;
}
