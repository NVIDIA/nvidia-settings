/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See Version 2
 * of the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *           Free Software Foundation, Inc.
 *           59 Temple Place - Suite 330
 *           Boston, MA 02111-1307, USA
 *
 */


/* nv-ddcci-client: A tool for exercising all the display-supported DDC/CI VCP codes
 *
 * This test program does the following:
 * - Check for the available displays
 * - For each available display:
 *   + Get the timing report
 *   + Get the monitor capabilities (list of vcp supported codes)
 *   + Write and reads each supported vcp code
 *   + Try to excercise the special vcp codes. These are vcp codes 
 *      that cannot be implemented with the XNVCTRLSetAttribute...()
 *      interface. 
 *
 * Usage:
 *  Just run the client on a display
 *  
 * Options:
 *  -defaults: Tries to restore the display defaults only. Does not run the test
 *
 * If the monitor does not support the factory defaults reset through DDC/CI,
 * it is strongly recommended to do so through the OSD.
 * After the test all monitor settings are changed, and the client tries to 
 * restore the factory defaults.
 */


#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <string.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"

static void Do_Capable(Display *dpy, int screen, unsigned int *ddcci_devices);
static void Do_Timing_Report(Display *dpy, int screen, unsigned int ddcci_devices);
static void Do_VCP_List(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask,
                        unsigned int *nvctrl_vcp_supported, unsigned int *possible_values_offset,
                        unsigned int *possible_values_size, unsigned int *nvctrl_vcp_possible_values);
static void Do_String_VCP(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask,
                        unsigned int *nvctrl_string_vcp_supported);
static void Do_Special_VCP(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask);
static Bool Do_Capabilities(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask,
                            unsigned int **nvctrl_vcp_supported, unsigned int **possible_values_offset,
                            unsigned int **possible_values_size, unsigned int **nvctrl_vcp_possible_values,
                            unsigned int **nvctrl_string_vcp_supported);
static void RestoreDefaults(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask,
                            unsigned int *nvctrl_vcp_supported);
                            
#define DISPLAY_DEVICES 0xFF
#define TRUE    1
#define FALSE   0

#define RETRIES 1

#define VERSION "1.0"

int main(int argc, char **argv) {
    Display *dpy;
    int screen;
    int bit;
    unsigned int display_mask;
    unsigned int ddcci_devices;
    unsigned int *nvctrl_vcp_supported;
    unsigned int *possible_values_offset;
    unsigned int *possible_values_size;
    unsigned int *nvctrl_vcp_possible_values;
    unsigned int *nvctrl_string_vcp_supported;
    int defaults=0;
    Bool ret;
    
    printf("nv-ddcci-client v%s\n\n", VERSION);
    
    
    if(argc==2  && !strcmp(argv[1], "-defaults") ) {   
        defaults=1;
    }
    else if(argc!=1) {
         printf("Usage: nv-ddcci-client [-defaults]\n");
         printf("Option -defaults: Tries to restore the display defaults only\n");
         return 1;
    }
    
    dpy = XOpenDisplay(NULL);
    if(dpy==NULL) {
        printf("Cannot open display\n");
        return 1;
    }
    screen=DefaultScreen(dpy);
    
    Do_Capable(dpy, screen, &ddcci_devices);
    if(ddcci_devices==0) {
        printf("No capable devices found.\n");
        printf("Please make sure you allowed DDC/CI with the X config option\n");
        printf("\"AllowDDCCI\" set to \"1\" in the device section of your X config file, i.e.:\n");
        printf("Option \"AllowDDCCI\" \"1\"\n");
        return 1;
    }
    
    if(defaults==0) 
        Do_Timing_Report(dpy, screen, ddcci_devices);
        
    for (bit = 0; bit < 24; bit++) { 
        display_mask = (1 << bit);
        ret=Do_Capabilities(dpy, screen, ddcci_devices, display_mask,
                            &nvctrl_vcp_supported, &possible_values_offset,
                            &possible_values_size, &nvctrl_vcp_possible_values,
                            &nvctrl_string_vcp_supported);
        if(ret==FALSE) 
            continue;
        
        if(defaults==0) {    
            Do_VCP_List(dpy, screen, ddcci_devices, display_mask,
                    nvctrl_vcp_supported, possible_values_offset,
                    possible_values_size, nvctrl_vcp_possible_values);
            Do_String_VCP(dpy, screen, ddcci_devices, display_mask,
                    nvctrl_string_vcp_supported);
            Do_Special_VCP(dpy, screen, ddcci_devices, display_mask);
        }     
        RestoreDefaults(dpy, screen, ddcci_devices, display_mask, nvctrl_vcp_supported);
    }  
    return 0 ;
}

/*
 *  - RestoreDefaults
 * Restore display defaults
 * If the display does not support this command, the display has to be reset manually
 */
static void RestoreDefaults(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask,
                            unsigned int *nvctrl_vcp_supported) {
    int display_devices=DISPLAY_DEVICES;
        
    if ((display_mask & display_devices) == 0x0) return;  
    if ((display_mask & ddcci_devices) == 0x0) return;  
    
    printf("Restoring defaults\n");
    printf("------------------\n");
    
    printf("  Display Mask 0x%x\n", display_mask);
    if(nvctrl_vcp_supported[NV_CTRL_DDCCI_PRESET_SETTINGS_RESTORE_FACTORY_DEFAULTS]) {
        printf("      Sending request\n");
        XNVCTRLSetAttribute (dpy, screen, display_mask, NV_CTRL_DDCCI_PRESET_SETTINGS_RESTORE_FACTORY_DEFAULTS, 1);          
    }
    else 
        printf("      Code not supported on display. PLEASE RESET YOUR DISPLAY MANUALLY !\n");
    printf("================================================================================\n\n");
   
}

/*
 *  - Do_Capable
 * Returns in ddcci_devices a  mask of DDC/CI capable displays
 */
static void Do_Capable(Display *dpy, int screen, unsigned int *ddcci_devices) {
    int status;
    unsigned int display_mask;
    int bit;
    unsigned int display_devices=DISPLAY_DEVICES;
    int val;
    
    *ddcci_devices=0;            
    printf("DDC/CI Capable\n");
    printf("--------------\n");
    
    for (bit = 0; bit < 24; bit++) { 
        display_mask = (1 << bit);
        
        if ((display_mask & display_devices) == 0x0) continue;  
        
        printf("  Display Mask 0x%x\n", display_mask);
        
        status = XNVCTRLQueryAttribute (dpy, screen, display_mask, NV_CTRL_DDCCI_CAPABLE, &val);
        if (!status) {
            printf("    Error when querying attribute\n");
        }
        else if(val==1) {
            printf("    Display is DDC/CI capable\n");
            *ddcci_devices |= display_mask;
        }
        else 
            printf("    Display is not DDC/CI capable\n");
    }
    printf("================================================================================\n\n");
}


/*
 * - Do_Timing_Report
 * Prints the timing report
 */
static void Do_Timing_Report(Display *dpy, int screen, unsigned int ddcci_devices) {
    int status;
    unsigned int display_mask;
    int bit;
    int display_devices=DISPLAY_DEVICES;
    unsigned int sync_freq_out_range, unstable_count, positive_h_sync, positive_v_sync,
                 h_freq,  v_freq;
                 
    printf("Timing Report\n");
    printf("-------------\n");
    
    for (bit = 0; bit < 24; bit++) { 
        display_mask = (1 << bit);
        
        if ((display_mask & display_devices) == 0x0) continue;  
        if ((display_mask & ddcci_devices) == 0x0) continue;  
        
        printf("  Display Mask 0x%x\n", display_mask);
        
        status = XNVCTRLQueryDDCCITimingReport (dpy, screen, display_mask,
                &sync_freq_out_range, &unstable_count, &positive_h_sync, &positive_v_sync,
                &h_freq, &v_freq); 
        if (!status) {
            printf("    Error when querying Timing Report\n");
            continue;
        }
        
        if(sync_freq_out_range) printf("    - Sync out of range\n");
        if(unstable_count)      printf("    - Unstable count\n");
        if(positive_h_sync)     printf("    - Positive H sync\n"); else printf("    - Negative H sync\n"); 
        if(positive_v_sync)     printf("    - Positive V sync\n"); else printf("    - Negative V sync\n"); 
        printf("    - H Freq = %d\n", h_freq);
        printf("    - V Freq = %d\n", v_freq);
    }
    printf("================================================================================\n\n");
}

/* - Do_Capabilities
 * Gets the supported NV-CONTROL DDC/CI attributes 
 */
static Bool Do_Capabilities(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask,
                            unsigned int **nvctrl_vcp_supported, unsigned int **possible_values_offset,
                            unsigned int **possible_values_size, unsigned int **nvctrl_vcp_possible_values,
                            unsigned int **nvctrl_string_vcp_supported) {
    int status;
    int display_devices=DISPLAY_DEVICES;
    int i;
                
    if ((display_mask & display_devices) == 0x0) return FALSE;  
    if ((display_mask & ddcci_devices) == 0x0) return FALSE;  
    
    printf("Capabilities Query\n");
    printf("------------------\n");

    printf("  Display Mask 0x%x\n", display_mask);

    status = NVCTRLQueryDDCCICapabilities (dpy, screen, display_mask,
        nvctrl_vcp_supported, possible_values_offset,
        possible_values_size, nvctrl_vcp_possible_values,
        nvctrl_string_vcp_supported);

    if (!status) {
        printf("    Error when querying Capabilities\n");
        return FALSE;
    }
    
    printf("    Supported NV_CTRL:");
    for(i=0;i<NV_CTRL_DDCCI_LAST_VCP+1;i++) {
        if((*nvctrl_vcp_supported)[i]) {
            printf(" %d", i);
            if((*possible_values_offset)[i]!=-1) {
                unsigned int of=(*possible_values_offset)[i];
                unsigned int sz=(*possible_values_size)[i];
                int p;
                printf(" (");
                for(p=0;p<sz;p++) 
                    printf(" %d", (*nvctrl_vcp_possible_values)[of+p]);
                printf(")");
            }
        }
    }
    printf("\n");
    printf("    Supported String NV_CTRL:");
    for(i=0;i<NV_CTRL_STRING_LAST_ATTRIBUTE+1;i++) {
        if((*nvctrl_string_vcp_supported)[i]) {
            printf(" %d", i);
            
        }
    }
    printf("\n");
    printf("================================================================================\n\n");  
    return TRUE;  
}

/* - Do_VCP_List
 * Excercies the simple NV-CONTROL DDC/CI attributes. Setting and Querying
  */
static void Do_VCP_List(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask,
                            unsigned int *nvctrl_vcp_supported, unsigned int *possible_values_offset,
                            unsigned int *possible_values_size, unsigned int *nvctrl_vcp_possible_values) {
    int status;
    int attr, val;
    NVCTRLAttributeValidValuesRec valid;
    int boolean, integer, range, bitmask, writable, readable;
    int display_devices=DISPLAY_DEVICES;
    int retry;
                
    if ((display_mask & display_devices) == 0x0) return;  
    if ((display_mask & ddcci_devices) == 0x0) return; 
     
    printf("NVCTRL_ Attributes\n");
    printf("------------------\n");
    
    printf("  Display Mask 0x%x\n", display_mask);
    
    for(attr=NV_CTRL_DDCCI_FIRST_VCP; 
        attr<=NV_CTRL_DDCCI_LAST_VCP;attr++) {
        if(!nvctrl_vcp_supported[attr]) continue;
        printf("    Testing attribute %d\n", attr);
        
        retry=RETRIES;        
        do {
            status = XNVCTRLQueryValidAttributeValues (dpy, screen, display_mask, attr, &valid); 
            if (!status) {
                printf("      Error when querying valid values for attribute");
                if(retry)
                    printf(" ... retrying\n");
                else 
                    printf("\n");
            }
            else 
                retry=0;
        } while(retry--);
        
        boolean=integer=range=writable=readable=0;
            
        if(status) switch(valid.type) {
           case ATTRIBUTE_TYPE_INTEGER:
               printf("      Type is integer\n");
               break;
           case ATTRIBUTE_TYPE_BITMASK:
               printf("      Type is bitmask\n");
               bitmask=1;
               break;
           case ATTRIBUTE_TYPE_BOOL:
               printf("      Type is boolean\n");
               boolean=1;
               break;
           case ATTRIBUTE_TYPE_RANGE:
               printf("      Type is range: %d-%d\n", valid.u.range.min, valid.u.range.max);
               range=1;
               break;
           case ATTRIBUTE_TYPE_INT_BITS:
               printf("      Type is integer bits\n");
               break;
           default:
               printf("       Error: Unknown type\n");
               break;
        }
    
        if(status && (valid.permissions & ATTRIBUTE_TYPE_WRITE)) {
            printf("      Writable\n");
            writable=1;
        }
        if(status && (valid.permissions & ATTRIBUTE_TYPE_READ)) {
            printf("      Readable\n");
            readable=1;
        }
            
        if(range) {
            int set_val;
            int error=0;
            retry=RETRIES;;
            if(readable && writable) {
                for(set_val=valid.u.range.min;  set_val<=valid.u.range.max; set_val++) {
                    printf("      Setting attribute to %d - ", set_val);fflush(stdout); 
                    status=XNVCTRLSetAttributeAndGetStatus (dpy, screen, display_mask, attr, set_val);
                    if(!status) 
                        printf("failed - ");
                    status = XNVCTRLQueryAttribute (dpy, screen, display_mask, attr, &val);
                    if (!status) {
                        printf("Error when querying attribute                               \n");
                        error=1;
                    }
                    else {
                        if(val==set_val) {
                            printf("Reading back correct value %d\r", val);fflush(stdout); 
                            retry=RETRIES;
                        }
                        else {
                            error=1;
                            if(retry--) {
                                printf("Reading back value: %d - retrying                   \n", val);
                                set_val--;
                            }
                            else {
                                printf("Error when reading back value: %d                   \n", val);
                                retry=1;
                            }
                        }
                    }
                }
            }
            else if(readable) {
                status = XNVCTRLQueryAttribute (dpy, screen, display_mask, attr, &val);
                if (!status) {
                    printf("      Error when querying attribute                             \n");
                    error=1;
                }
                else {
                    printf("      Reading back current value is %d\r", val);fflush(stdout); 
                }
            }
            // There is no WO range
            if(error) 
                printf("      Errors occured                                                \n");
            else
                printf("      Attribute setting and reading back complete                   \n");
            
        }
        
        else if(boolean) {
            int set_val=NV_CTRL_DDCCI_OFF;
            retry=RETRIES+1;
            while(1) {
                if(writable) {
                    printf("      Setting attribute to %d - ", set_val);fflush(stdout); 
                    status=XNVCTRLSetAttributeAndGetStatus (dpy, screen, display_mask, attr, set_val);
                    if(!status)
                        printf("failed.");
                }
                if(readable) {
                    status = XNVCTRLQueryAttribute (dpy, screen, display_mask, attr, &val);
                    if (!status) 
                        printf("      Error when querying attribute");
                    else if(writable) {
                        if(val==set_val) {
                            printf("  Reading back correct value %d", val);
                            retry=0;
                        }
                        else {
                            printf(" Error when reading back value: %d", val);
                            retry--;
                        }
                    }
                    else 
                       printf("      Current value is %d", val); 
                }
                else {
                    printf("OK");
                    retry=0;
                }
                printf("\n");
                
                if(!writable)
                    break;
                    
                if(retry==0) {
                    if(set_val==NV_CTRL_DDCCI_ON)
                        break;
                    set_val=NV_CTRL_DDCCI_ON;
                    retry=RETRIES;
                }
            }
            
        } 
        
        else if(integer||bitmask) {
            int set_val;
            int offset=possible_values_offset[attr];
            int size=possible_values_size[attr];
            int index;
            retry=RETRIES;
            
            
            for(index=offset;offset!=-1 && index<offset+size;index++) {
                set_val=nvctrl_vcp_possible_values[index];
                if(writable) {                    
                    printf("      Setting attribute to %d - ", set_val);fflush(stdout); 
                    status=XNVCTRLSetAttributeAndGetStatus (dpy, screen, display_mask, attr, set_val);
                    if(!status)
                        printf("failed - ");
                }
                if(readable) {
                    status = XNVCTRLQueryAttribute (dpy, screen, display_mask, attr, &val);
                    if (!status) 
                        printf("      Error when querying attribute");
                    else if(writable) {
                        if(val==set_val) {
                            printf(" Reading back correct value %d", val);
                            retry=RETRIES;
                        }
                        else {
                            printf(" Error when reading back value: %d", val);
                            if(retry--)
                                index--;
                            else
                                retry=RETRIES;
                        }
                    }
                    else 
                        printf("      Current value is %d", val); 
                }
                printf("\n");
            }
            if(offset==-1 && readable) {
                status = XNVCTRLQueryAttribute (dpy, screen, display_mask, attr, &val);
                if (!status) 
                    printf("      Error when querying attribute");
                printf("      Current value is %d\n", val); 
            }
        }                  
    }
    printf("================================================================================\n\n");    
}


/* - Do_String_VCP
 * Excercises the String NV-CONTROL DDC/CI atytributes
 */
static void Do_String_VCP(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask,
                          unsigned int *nvctrl_string_vcp_supported) {
    int display_devices=DISPLAY_DEVICES;
    int status;
    int attr;
    int retry;
    
    if ((display_mask & display_devices) == 0x0) return;  
    if ((display_mask & ddcci_devices) == 0x0) return; 
     
    printf("NVCTRL_ String Attributes\n");
    printf("-------------------------\n");
    
    printf("  Display Mask 0x%x\n", display_mask);
    if(nvctrl_string_vcp_supported[NV_CTRL_STRING_DDCCI_MISC_TRANSMIT_DISPLAY_DESCRIPTOR]) 
    {
        char *ptr=NULL;
        char *str="New String for NV_CTRL_STRING_DDCCI_MISC_TRANSMIT_DISPLAY_DESCRIPTOR";
        printf("    Testing attribute NV_CTRL_STRING_DDCCI_MISC_TRANSMIT_DISPLAY_DESCRIPTOR\n");   
    
        attr=NV_CTRL_STRING_DDCCI_MISC_TRANSMIT_DISPLAY_DESCRIPTOR;
        retry=RETRIES;
        do {
            printf("      Setting attribute... ");  fflush(stdout); 
            XNVCTRLSetStringAttribute(dpy, screen, display_mask, attr, str);
            if(status)
                printf("success\n");
            else {
                printf("failed.\n");
            }
        
            printf("      Querying attribute... ");  fflush(stdout); 
            status=XNVCTRLQueryStringAttribute(dpy, screen, display_mask, attr, &ptr);
            if (!status) {
                printf("Error when querying attribute");
                if(retry) {
                    retry--;
                    printf(". Retrying ...");
                }
                printf("\n");;
            }
            else if(strcmp(ptr, str)) {
                printf("Error when reading back string: %s", ptr);
                if(retry) {
                    retry--;
                    printf(". Retrying ...");
                }
                printf("\n");
            }
            else {
                printf("Reading back correct string: %s\n", ptr); 
                retry=0;
            }
            free(ptr);
        } while (retry--);
    }
    
    if(nvctrl_string_vcp_supported[NV_CTRL_STRING_DDCCI_MISC_AUXILIARY_DISPLAY_DATA]) 
    {
        char *str="New String for NV_CTRL_STRING_DDCCI_MISC_AUXILIARY_DISPLAY_DATA";
        printf("    Testing attribute NV_CTRL_STRING_DDCCI_MISC_AUXILIARY_DISPLAY_DATA\n");   
    
        attr=NV_CTRL_STRING_DDCCI_MISC_AUXILIARY_DISPLAY_DATA;
        printf("      Setting attribute... ");  fflush(stdout); 
        status=XNVCTRLSetStringAttribute(dpy, screen, display_mask, attr, str);
        if(status)
            printf("success\n");
        else
            printf("failed\n");
    }       
    printf("================================================================================\n\n");        
}

/* - Do_Special_VCP
 * Excercises the scpecial VCP codes, that are neither simple or string NV-CONTROL attributes
 * These correspond to VCP code that handle multiple values at once
 */
static void Do_Special_VCP(Display *dpy, int screen, unsigned int ddcci_devices, unsigned int display_mask) {
    int display_devices=DISPLAY_DEVICES;
    int status;
    int retry, error;
    
    if ((display_mask & display_devices) == 0x0) return;  
    if ((display_mask & ddcci_devices) == 0x0) return; 
     
    printf("NVCTRL_ String Attributes\n");
    printf("-------------------------\n");
    
    printf("  Display Mask 0x%x\n", display_mask);
       
    printf("    Testing function XNVCTRLQueryDDCCILutSize\n");        
    {
        unsigned int red_entries;
        unsigned int green_entries;
        unsigned int blue_entries;
        unsigned int red_bits_per_entries;
        unsigned int green_bits_per_entries;
        unsigned int blue_bits_per_entries;
        
        printf("      Querying attribute... ");  fflush(stdout); 
        status=XNVCTRLQueryDDCCILutSize (dpy, screen, display_mask,
                                         &red_entries, &green_entries, &blue_entries,
                                         &red_bits_per_entries, &green_bits_per_entries, &blue_bits_per_entries);
        if (!status) {
            printf("Error when querying attribute.\n");
            printf("      (normal if this attribute is not supported by the display)\n");
        }
        else {
            printf("red_entries=%d; green_entries=%d; blue_entries=%d\n", red_entries, green_entries, blue_entries);
            printf("red_bits_per_entries=%d; green_bits_per_entries=%d; blue_bits_per_entries=%d\n", 
                red_bits_per_entries, green_bits_per_entries, blue_bits_per_entries);
        }
    }
    
    printf("    Testing function XNVCTRLSetDDCCISinglePointLutOperation\n");  
    printf("                     XNVCTRLQueryDDCCISinglePointLutOperation\n");  
    {
        unsigned int offset;
        unsigned int red_value, ret_red_value;
        unsigned int green_value, ret_green_value;
        unsigned int blue_value, ret_blue_value;
        
        offset=0;
        red_value=1;
        green_value=2;
        blue_value=3;
        
        retry=RETRIES;
        do {
            error=0;
            printf("      Setting attribute... ");  fflush(stdout); 
            status=XNVCTRLSetDDCCISinglePointLutOperation(dpy, screen, display_mask, offset, red_value, green_value, blue_value);
            if(status)
                printf("success\n");
            else {
                printf("failed.\n");
                printf("      (normal if this attribute is not supported by the display)\n"); 
            }    
            printf("      Querying attribute... ");  fflush(stdout); 
        
            status=XNVCTRLQueryDDCCISinglePointLutOperation(dpy, screen, display_mask, offset, 
                                            &ret_red_value, &ret_green_value, &ret_blue_value);
            if (!status) {
                printf("Error when querying attribute.\n");
                printf("      (normal if this attribute is not supported by the display)\n");
                error=1;
            }
            else {
                if(red_value!=ret_red_value) {
                    printf("      Error when reading back red_value: %d\n", ret_red_value);
                    error=1;
                }
                if(red_value!=ret_red_value) {
                    printf("      Error when reading back green_value: %d\n", ret_green_value);
                    error=1;
                }
                if(red_value!=ret_red_value) {
                    printf("      Error when reading back blue_value: %d\n", ret_blue_value);
                    error=1;
                }
            }
            if(error) {
                if(retry) {
                    printf("      Retrying...\n");
                }
            }
        } while(retry--);
    }
    
    printf("    Testing function XNVCTRLSetDDCCIBlockLutOperation\n");  
    printf("                     XNVCTRLQueryDDCCIBlockLutOperation\n");  
    {
        unsigned int color; // NV_CTRL_DDCCI_RED_LUT, NV_CTRL_DDCCI_GREEN_LUT, NV_CTRL_DDCCI_BLUE_LUT
        unsigned int offset;
        unsigned int size=2;
        unsigned int value[2], *ret_value;
        int i;
        
        color=NV_CTRL_DDCCI_RED_LUT;
        offset=0;
        value[0]=1;
        value[1]=2;
        
        retry=RETRIES;
        do {
            printf("      Setting attribute... ");  fflush(stdout); 
            status=XNVCTRLSetDDCCIBlockLutOperation(dpy, screen, display_mask, color, offset, size, value);
            if(status)
                printf("success\n");
            else {
                printf("failed.\n");
                printf("      (normal if this attribute is not supported by the display)\n");
            }
        
            printf("      Querying attribute... ");  fflush(stdout); 
            status=XNVCTRLQueryDDCCIBlockLutOperation(dpy, screen, display_mask, color, offset, size, &ret_value);
            if (!status) {
                printf("Error when querying attribute.\n");
                printf("      (normal if this attribute is not supported by the display)\n");
                error=1;
            }
            else {
                for(i=0;i<size;i++)
                    if(value[i]!=ret_value[i]) {
                        printf("Error when reading back value[%d]: %d", i, ret_value[i]);
                        error=1;
                    }
                free(ret_value);
            }
            if(error) {
                if(retry) {
                    printf("      Retrying...\n");
                }
            }
        } while(retry--);
    }
    
    printf("    Testing function XNVCTRLSetDDCCIRemoteProcedureCall\n");  
    {
        unsigned int offset;
        unsigned int size=2;
        unsigned int red_lut[2];
        unsigned int green_lut[2];
        unsigned int blue_lut[2];
        unsigned int increment[2];
    
        offset=0;
        red_lut[0]=1;red_lut[1]=2;
        green_lut[0]=3;green_lut[1]=4;
        blue_lut[0]=5;blue_lut[1]=6;
        increment[0]=7;increment[1]=8;
        
        printf("      Setting attribute... ");  fflush(stdout); 
        status=XNVCTRLSetDDCCIRemoteProcedureCall(dpy, screen, display_mask, offset, size, 
                                           red_lut, green_lut, blue_lut, increment);
        if(status)
            printf("success\n");
        else
            printf("failed\n");
    }
    
    printf("    Testing function XNVCTRLQueryDDCCIDisplayControllerType\n");
    {
       unsigned char *controller_manufacturer;
       unsigned int controller_type;
       int status;
       
       printf("      Querying attribute... ");  fflush(stdout); 
       status=XNVCTRLQueryDDCCIDisplayControllerType(dpy, screen, display_mask, &controller_manufacturer, &controller_type);
       if (!status) {
           printf("Error when querying attribute.\n");
           printf("      (normal if this attribute is not supported by the display)\n");
       }
       else {
           printf("Controller manufacturer =%s\n", controller_manufacturer);
           printf("Controller type =%d\n", controller_type);
           free(controller_manufacturer);
       }
    }
    printf("================================================================================\n\n");  
}
