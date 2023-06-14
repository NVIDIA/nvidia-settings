/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2022-2023 NVIDIA Corporation.
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
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dlfcn.h>

#include <gtk/gtk.h>
#include <NvCtrlAttributesPrivate.h>

#include "ctkutils.h"
#include "ctkhelp.h"
#include "ctkgridlicense.h"
#include "ctkbanner.h"
#include "nv_grid_dbus.h"
#include <gdk/gdkkeysyms.h>
#ifdef CTK_GTK3
#include <gdk/gdkkeysyms-compat.h>
#endif

#include <dbus/dbus.h>
#include <unistd.h>
#include <errno.h>

#define DEFAULT_UPDATE_GRID_LICENSE_STATUS_INFO_TIME_INTERVAL   1000
#define GRID_CONFIG_FILE                                        "/etc/nvidia/gridd.conf"
#define GRID_CONFIG_FILE_TEMPLATE                               "/etc/nvidia/gridd.conf.template"
#define BUF_LEN                                                 256

static const char * __manage_grid_licenses_help =
"Use the Manage License page to obtain licenses "
"for NVIDIA vGPU or NVIDIA RTX Virtual Workstation or NVIDIA Virtual Compute Server on supported Tesla products. "
"The current license status is displayed on the page. If licensed, the license expiry information in GMT time zone is shown.";
static const char * __manage_grid_licenses_vcompute_help =
"Use the Manage License page to obtain licenses "
"for NVIDIA vGPU or NVIDIA RTX Virtual Workstation or NVIDIA Virtual Compute Server on supported Tesla products. "
"The current license status is displayed on the page. If licensed, the license expiry information in GMT time zone is shown.";
static const char * __grid_virtual_workstation_help =
"Select this option to enable NVIDIA RTX Virtual Workstation license.";
static const char * __grid_virtual_compute_help =
"Select this option to enable NVIDIA Virtual Compute Server license.";
static const char * __grid_vapp_help =
"Select this option to disable the NVIDIA RTX Virtual Workstation license.";
static const char * __license_edition_help =
"This section indicates the status of vGPU Software licensing on the system.";
static const char * __license_server_help =
"Shows the configured license server details read from Client Configuration token.";
static const char * __primary_server_address_help =
"The address of Primary license server.";
static const char * __primary_server_port_help =
"Port number used by the Primary license server.";
static const char * __secondary_server_help =
"The address of Secondary license server.";
static const char * __secondary_server_port_help =
"Port number used by the Secondary license server";
static const char * __apply_button_help =
"Clicking the Apply button updates license feature type in the gridd.conf file and "
"sends update license request to the NVIDIA vGPU licensing daemon.";
static const char * __cancel_button_help =
"Clicking the Cancel button sets the license settings from the gridd.conf file. "
"Any changes you have done on this page will be lost.";

typedef struct
{
    void                                                *dbusHandle;
    typeof(dbus_bus_get)                                *getDbus;
    typeof(dbus_error_init)                             *dbusErrorInit;
    typeof(dbus_error_is_set)                           *dbusErrorIsSet;
    typeof(dbus_error_free)                             *dbusErrorFree;
    typeof(dbus_bus_request_name)                       *dbusRequestName;
    typeof(dbus_message_new_method_call)                *dbusMessageNewMethodCall;
    typeof(dbus_message_iter_init)                      *dbusMessageIterInit;
    typeof(dbus_message_iter_init_append)               *dbusMessageIterInitAppend;
    typeof(dbus_message_iter_append_basic)              *dbusMessageIterAppendBasic;
    typeof(dbus_message_iter_get_arg_type)              *dbusMessageIterGetArgType;
    typeof(dbus_message_iter_get_basic)                 *dbusMessageIterGetBasic;
    typeof(dbus_message_unref)                          *dbusMessageUnref;
    typeof(dbus_connection_close)                       *dbusConnectionClose;
    typeof(dbus_connection_flush)                       *dbusConnectionFlush;
    typeof(dbus_connection_send_with_reply)             *dbusConnectionSendWithReply;
    typeof(dbus_connection_send_with_reply_and_block)   *dbusConnectionSendWithReplyAndBlock;
    typeof(dbus_pending_call_block)                     *dbusPendingCallBlock;
    typeof(dbus_pending_call_steal_reply)               *dbusPendingCallStealReply;
} GridDbus;

struct _DbusData
{
    DBusConnection *conn;
    GridDbus       dbus;
};

static void apply_clicked(GtkWidget *widget, gpointer user_data);
static void cancel_clicked(GtkWidget *widget, gpointer user_data);
static void ctk_manage_grid_license_finalize(GObject *object);
static void ctk_manage_grid_license_class_init(CtkManageGridLicenseClass *, gpointer);
static void dbusClose(DbusData *dbusData);
static gboolean checkConfigfile(gboolean *writable);
static void update_gui_from_griddconfig(gpointer user_data);
static gboolean licenseStateQueryFailed = FALSE;
static void get_licensable_feature_information(gpointer user_data);
static gboolean is_feature_supported(gpointer user_data, int featureType);
static gboolean is_restart_required(gpointer user_data);
static gboolean bQueryGridLicenseInfo = TRUE;
int64_t enabledFeatureCode = NV_GRID_LICENSE_FEATURE_TYPE_VAPP;

GType ctk_manage_grid_license_get_type(void)
{
    static GType ctk_manage_grid_license_type = 0;

    if (!ctk_manage_grid_license_type) {
        static const GTypeInfo ctk_manage_grid_license_info = {
            sizeof (CtkManageGridLicenseClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_manage_grid_license_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkManageGridLicense),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_manage_grid_license_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkManageGridLicense",
                                   &ctk_manage_grid_license_info, 0);
    }

    return ctk_manage_grid_license_type;

} /* ctk_manage_grid_license_get_type() */

typedef enum
{
    NV_GRIDD_FEATURE_TYPE = 0,
    NV_GRIDD_ENABLE_UI,
    NV_GRIDD_MAX_TOKENS
} CfgParams;

static const char *configParamsList[] = {
    [NV_GRIDD_FEATURE_TYPE]          = "FeatureType",
    [NV_GRIDD_ENABLE_UI]             = "EnableUI",
};

typedef struct NvGriddConfigParamsRec
{
    char *str[NV_GRIDD_MAX_TOKENS];
} NvGriddConfigParams;

typedef struct ConfigFileLinesRec
{
    char **lines;
    int nLines;
} ConfigFileLines;

/*
 * updateFeatureTypeFromGriddConfig() - Update feature type from gridd.conf or virtualization mode
 *
 */
static void updateFeatureTypeFromGriddConfig(gpointer user_data, NvGriddConfigParams *griddConfig)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);

    if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "4") == 0)
    {
        ctk_manage_grid_license->feature_type = NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE;
    }
    else if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "2") == 0)
    {
        ctk_manage_grid_license->feature_type = NV_GRID_LICENSE_FEATURE_TYPE_VWS;
    }
    else
    {
        ctk_manage_grid_license->feature_type = NV_GRID_LICENSE_FEATURE_TYPE_VAPP;
    }

    // Override feature type when on vGPU
    if (ctk_manage_grid_license->license_edition_state == NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_VGPU)
    {
        ctk_manage_grid_license->feature_type = NV_GRID_LICENSE_FEATURE_TYPE_VGPU;
    }
}

static void FreeConfigFileLines(ConfigFileLines *pLines)
{
    int i;

    if (pLines != NULL) {
        for (i = 0; i < pLines->nLines; i++) {
            nvfree(pLines->lines[i]);
        }

        nvfree(pLines->lines);
        nvfree(pLines);
    }
}

static ConfigFileLines *AllocConfigFileLines(void)
{
    return nvalloc(sizeof(ConfigFileLines));
}

static void AddLineToConfigFileLines(ConfigFileLines *pLines,
                                     char *line)
{
    pLines->lines = nvrealloc(pLines->lines, (pLines->nLines + 1) * sizeof(*pLines->lines));

    pLines->lines[pLines->nLines] = line;

    pLines->nLines++;
}


static void FreeNvGriddConfigParams(NvGriddConfigParams *griddConfig)
{
    int i;

    if (griddConfig != NULL) {
        for (i = 0; i < ARRAY_LEN(griddConfig->str); i++) {
            nvfree(griddConfig->str[i]);
        }

        nvfree(griddConfig);
    }
}

static NvGriddConfigParams *AllocNvGriddConfigParams(void)
{
    NvGriddConfigParams *griddConfig = nvalloc(sizeof(*griddConfig));

    griddConfig->str[NV_GRIDD_FEATURE_TYPE]          = nvstrdup("0");
    griddConfig->str[NV_GRIDD_ENABLE_UI]             = nvstrdup("FALSE");

    return griddConfig;
}

/*
 * Return TRUE if 'line' begins with 'item='.
 */
static gboolean LineIsItem(const char *line, const char *item)
{
    const size_t itemLen = strlen(item);

    if (strlen(line) <= itemLen) {
        return FALSE;
    }
    if (strncmp(line, item, itemLen) != 0) {
        return FALSE;
    }
    if (line[itemLen] != '=') {
        return FALSE;
    }
    return TRUE;
}

/*
 * Update NvGriddConfigParams if the line from vGPU license config describes any
 * nvidia-gridd configuration parameters.
 */
static void UpdateGriddConfigFromLine(NvGriddConfigParams *griddConfig,
                                      const char *line)
{
    int item;
    char *tmpLine = remove_spaces(line);

    /* ignore comment lines */
    if (tmpLine[0] == '#') {
        goto done;
    }

    for (item = 0; item < ARRAY_LEN(configParamsList); item++) {

        char *p;
        const char *itemStr = configParamsList[item];

        /* continue if tmpLine does not start with "itemStr=" */
        if (!LineIsItem(tmpLine, itemStr)) {
            continue;
        }

        /* skip past "itemStr="; +1 is for '=' */
        p = tmpLine + strlen(itemStr) + 1;

        /* Empty string, skip parsing */
        if (*p == '\0') {
            continue;
        }

        nvfree(griddConfig->str[item]);
        griddConfig->str[item] = nvstrdup(p);
        goto done;
    }

    /*
     * The gridd.conf syntax supports tokens beyond those in
     * configParamsList[].  Just ignore lines we don't recognize.
     */

done:
    nvfree(tmpLine);
}

static void UpdateGriddConfigFromConfigFileLines(
                                   NvGriddConfigParams *griddConfig,
                                   const ConfigFileLines *pLines)
{
    int i;

    for (i = 0; i < pLines->nLines; i++) {
        UpdateGriddConfigFromLine(griddConfig, pLines->lines[i]);
    }
}

static char *AllocConfigLine(const NvGriddConfigParams *griddConfig,
                             CfgParams item)
{
    return nvstrcat(configParamsList[item], "=", griddConfig->str[item], NULL);
}

/*
 * Update the line from gridd.conf with information in NvGriddConfigParams.
 *
 * The old line is passed as argument, and the new line is returned.
 * If the line has to be reallocated, the old line is freed.  If the
 * old line does not need to be reallocated, then it is returned
 * as-is.  This allows callers to use it like this:
 *
 * pLines->lines[i] = UpdateLineWithGriddConfig(griddConfig, pLines->lines[i]);
 *
 * As a side effect, update the pItemIsPresent array, recording which items
 * are present when we find a match with line.
 */
static char *UpdateLineWithGriddConfig(const NvGriddConfigParams *griddConfig,
                                       char *line, gboolean *pItemIsPresent)
{
    int item;
    char *tmpLine = remove_spaces(line);

    /* ignore comment lines */
    if (tmpLine[0] == '#') {
        goto done;
    }

    for (item = 0; item < ARRAY_LEN(configParamsList); item++) {

        const char *itemStr = configParamsList[item];

        /* continue if tmpLine does not start with "itemStr=" */
        if (!LineIsItem(tmpLine, itemStr)) {
            continue;
        }

        pItemIsPresent[item] = TRUE;

        nvfree(tmpLine);
        nvfree(line);
        return AllocConfigLine(griddConfig, item);
    }

done:
    nvfree(tmpLine);
    return line;
}

static void UpdateConfigFileLinesFromGriddConfig(
                             const NvGriddConfigParams *griddConfig,
                             ConfigFileLines *pLines)
{
    gboolean itemIsPresent[NV_GRIDD_MAX_TOKENS] = { FALSE };
    int i;

    /* Update the lines in pLines */

    for (i = 0; i < pLines->nLines; i++) {
        /* TODO: "EnableUI" config parameter will never be
         * set by the user through UI, so skip updating the
         * pLines with "EnableUI" parameter */
        pLines->lines[i] = UpdateLineWithGriddConfig(griddConfig,
                                                     pLines->lines[i],
                                                     itemIsPresent);
    }

    /* Append any items not updated in the above loop. */
    for (i = 0; i < ARRAY_LEN(configParamsList); i++) {

        if (itemIsPresent[i]) {
            continue;
        }

        AddLineToConfigFileLines(pLines, AllocConfigLine(griddConfig, i));
    }
}

static void UpdateGriddConfigFromGui(
                      NvGriddConfigParams *griddConfig,
                      CtkManageGridLicense *ctk_manage_grid_license)
{
    const char *tmp;

    /* featureType */

    switch (ctk_manage_grid_license->feature_type) {
        case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
            tmp = "0";
            break;
        case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
            tmp = "1";
            break;
        case NV_GRID_LICENSE_FEATURE_TYPE_VWS:
            tmp = "2";
            break;
        case NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE:
            tmp = "4";
            break;
        default:
            tmp = "0";
    }
    if (strcmp(tmp, griddConfig->str[NV_GRIDD_FEATURE_TYPE]) != 0) {
        nvfree(griddConfig->str[NV_GRIDD_FEATURE_TYPE]);
        griddConfig->str[NV_GRIDD_FEATURE_TYPE] = nvstrdup(tmp);
    }

    /* note: nothing in the UI will alter enableUI */
}

/*
 * Read the vGPU license config file specified by configFile, returning the
 * lines in ConfigFileLines.
 */
static ConfigFileLines *ReadConfigFileStream(FILE *configFile)
{
    ConfigFileLines *pLines;
    int eof = FALSE;
    char *line = NULL;

    pLines = AllocConfigFileLines();

    while ((line = fget_next_line(configFile, &eof)) && !eof) {
        AddLineToConfigFileLines(pLines, line);
    }

    return pLines;
}

static int ReadConfigFile(ConfigFileLines **ppLines)
{
    FILE *configFile = fopen(GRID_CONFIG_FILE, "r");

    if (configFile == NULL) {
        return errno;
    }

    *ppLines = ReadConfigFileStream(configFile);

    fclose(configFile);

    return 0;
}

static void WriteConfigFileStream(FILE *configFile, const ConfigFileLines *pLines)
{
    int i;

    for (i = 0; i < pLines->nLines; i++) {
        fprintf(configFile, "%s\n", pLines->lines[i]);
    }
}

static int WriteConfigFile(const ConfigFileLines *pLines)
{
    FILE *configFile;

    configFile = fopen(GRID_CONFIG_FILE, "w");

    if (configFile == NULL) {
        return errno;
    }

    WriteConfigFileStream(configFile, pLines);

    fclose(configFile);

    return 0;
}


/*
 * Update the vGPU license config file with the current GUI state.
 *
 */
static int UpdateConfigFile(CtkManageGridLicense *ctk_manage_grid_license)
{
    ConfigFileLines *pLines = NULL;
    NvGriddConfigParams *griddConfig = NULL;
    int retErr;

    /* Read gridd.conf */
    retErr = ReadConfigFile(&pLines);
    if (retErr != 0) {
        goto done;
    }

    /* Create a griddConfig */
    griddConfig = AllocNvGriddConfigParams();

    /* Update the griddConfig with the lines from gridd.conf */
    UpdateGriddConfigFromConfigFileLines(griddConfig, pLines);

    /* Update the griddConfig with the state of the nvidia-settings GUI */
    UpdateGriddConfigFromGui(griddConfig, ctk_manage_grid_license);

    /* Update the lines of gridd.conf with griddConfig */
    UpdateConfigFileLinesFromGriddConfig(griddConfig, pLines);

    /* Write the lines of gridd.conf to file */
    retErr = WriteConfigFile(pLines);

done:
    FreeConfigFileLines(pLines);
    FreeNvGriddConfigParams(griddConfig);

    return retErr;
}

/*
 * Create an NvGriddConfigParams by parsing the configuration file and
 * populating NvGriddConfigParams.  The caller should call
 * FreeNvGriddConfigParams() when done with it.
 *
 */
static NvGriddConfigParams *GetNvGriddConfigParams(void)
{
    ConfigFileLines *pLines = NULL;
    NvGriddConfigParams *griddConfig = NULL;
    int retErr;

    /* Create a griddConfig */
    griddConfig = AllocNvGriddConfigParams();

    /* Read gridd.conf */
    retErr = ReadConfigFile(&pLines);
    if (retErr != 0) {
        goto done;
    }

    /* Update the griddConfig with the lines from gridd.conf */
    UpdateGriddConfigFromConfigFileLines(griddConfig, pLines);

done:
    FreeConfigFileLines(pLines);

    return griddConfig;
}


static void ctk_manage_grid_license_class_init(CtkManageGridLicenseClass *ctk_object_class,
                                               gpointer class_data)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_object_class;
    gobject_class->finalize = ctk_manage_grid_license_finalize;
}


static void ctk_manage_grid_license_finalize(GObject *object)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(object);

    dbusClose(ctk_manage_grid_license->dbusData);
}


/*
 * dlclose libdbus-1.so.3
 */
static void dbusClose(DbusData *dbusData)
{
    if (dbusData) {
        if (dbusData->dbus.dbusHandle) {
            dlclose(dbusData->dbus.dbusHandle);
        }
        nvfree(dbusData);
    }
}


/*
 * load D-Bus symbols from libdbus-1.so.3
 */
static gboolean dbusLoadSymbols(DbusData *dbusData)
{
    const char *missingSym = " ";
    
    dbusData->dbus.dbusHandle = dlopen("libdbus-1.so.3", RTLD_LAZY);
    
    if (!dbusData->dbus.dbusHandle)
    {
        nv_error_msg("Couldn't open libdbus-1.so.3");
        return FALSE;
    }

#define LOAD_SYM(name, sym)                                      \
    dbusData->dbus.name = dlsym(dbusData->dbus.dbusHandle, sym); \
    if (!dbusData->dbus.name) {                                  \
        missingSym = sym;                                        \
        goto dbus_end;                                           \
    }
    LOAD_SYM(getDbus, "dbus_bus_get");
    LOAD_SYM(dbusErrorInit, "dbus_error_init");
    LOAD_SYM(dbusErrorIsSet, "dbus_error_is_set");
    LOAD_SYM(dbusErrorFree, "dbus_error_free");
    LOAD_SYM(dbusRequestName, "dbus_bus_request_name");
    LOAD_SYM(dbusMessageNewMethodCall, "dbus_message_new_method_call");
    LOAD_SYM(dbusMessageIterInit, "dbus_message_iter_init");
    LOAD_SYM(dbusMessageIterInitAppend, "dbus_message_iter_init_append");
    LOAD_SYM(dbusMessageIterAppendBasic, "dbus_message_iter_append_basic");
    LOAD_SYM(dbusMessageIterGetArgType, "dbus_message_iter_get_arg_type");
    LOAD_SYM(dbusMessageIterGetBasic, "dbus_message_iter_get_basic");
    LOAD_SYM(dbusMessageUnref, "dbus_message_unref");
    LOAD_SYM(dbusConnectionClose, "dbus_connection_close");
    LOAD_SYM(dbusConnectionFlush, "dbus_connection_flush");
    LOAD_SYM(dbusConnectionSendWithReply, "dbus_connection_send_with_reply");
    LOAD_SYM(dbusConnectionSendWithReplyAndBlock, "dbus_connection_send_with_reply_and_block");
    LOAD_SYM(dbusPendingCallBlock, "dbus_pending_call_block");
    LOAD_SYM(dbusPendingCallStealReply, "dbus_pending_call_steal_reply");
#undef LOAD_SYM

    return TRUE;

dbus_end:
    nv_error_msg("Required libdbus symbol %s was not found", missingSym);
    dbusClose(dbusData);

    return FALSE;
}


/*
 * send_message_to_gridd() - This function is for communication between
 * vGPU licensing daemon and nvidia-settings using DBus.
 * Nvidia-settings sends different command messages to either query info
 * such as license state or to notify vGPU licensing daemon to update license
 * info changes.
 */
static gboolean
send_message_to_gridd(CtkManageGridLicense *ctk_manage_grid_license,
                      gint param,
                      gint *pStatus)
{
    gboolean ret = FALSE;

    DBusMessage *msg, *reply;
    DBusMessageIter args;
    DBusError err;

    DbusData *dbusData = ctk_manage_grid_license->dbusData;
    DBusConnection* conn = dbusData->conn;

    /* initialise the errors */
    dbusData->dbus.dbusErrorInit(&err);

    /* a new method call */
    msg = dbusData->dbus.dbusMessageNewMethodCall(NV_GRID_DBUS_TARGET, // target for the method call
                                       NV_GRID_DBUS_OBJECT, // object to call on
                                       NV_GRID_DBUS_INTERFACE, // interface to call on
                                       NV_GRID_DBUS_METHOD); // method name
    if (NULL == msg) {
        return FALSE;
    }
    /* append arguments */
    dbusData->dbus.dbusMessageIterInitAppend(msg, &args);

    if (!dbusData->dbus.dbusMessageIterAppendBasic(&args,
                                                   DBUS_TYPE_INT32, &param)) {
        goto done;
    }

    /* send a message and block for a default time period
       while waiting for a reply and returns NULL on failure with an error code.*/
    reply = dbusData->dbus.dbusConnectionSendWithReplyAndBlock(conn,
                                      msg, -1, &err);  // -1 is default timeout
    if ((reply == NULL) || (dbusData->dbus.dbusErrorIsSet(&err))) {
        goto done;
    }

    /* read the parameters */
    if (!dbusData->dbus.dbusMessageIterInit(reply, &args)) {
        nv_error_msg("vGPU License dbus communication: Message has no arguments!\n");
    } else if (DBUS_TYPE_INT32 !=
               dbusData->dbus.dbusMessageIterGetArgType(&args)) {
        nv_error_msg("vGPU License dbus communication: Argument is not int!\n");
    } else {
        dbusData->dbus.dbusMessageIterGetBasic(&args, pStatus);
    }

    ret = TRUE;
done:
    /* free reply */
    dbusData->dbus.dbusMessageUnref(msg);

    return ret;
}

static gboolean send_string_message_to_gridd(CtkManageGridLicense *ctk_manage_grid_license, char *param, char *value)
{
    gboolean ret = FALSE;
    DBusMessage *msg, *reply;
    DBusMessageIter args;
    DBusError err;
    char *receivedVal = NULL;

    DbusData *dbusData = ctk_manage_grid_license->dbusData;
    DBusConnection* conn = dbusData->conn;

    /* initialise the errors */
    dbusData->dbus.dbusErrorInit(&err);

    /* a new method call */
    msg = dbusData->dbus.dbusMessageNewMethodCall(NV_GRID_DBUS_TARGET, // target for the method call
                                                  NV_GRID_DBUS_OBJECT, // object to call on
                                                  NV_GRID_DBUS_INTERFACE, // interface to call on
                                                  NV_GRID_DBUS_METHOD); // method name
    if (NULL == msg)
    {
        return FALSE;
    }
    /* append arguments */
    dbusData->dbus.dbusMessageIterInitAppend(msg, &args);

    if (!dbusData->dbus.dbusMessageIterAppendBasic(&args, DBUS_TYPE_STRING, &param))
    {
        goto done;
    }

    /* send a message and block for a default time period
    while waiting for a reply and returns NULL on failure with an error code.*/
    reply = dbusData->dbus.dbusConnectionSendWithReplyAndBlock(conn, msg, -1, &err);  // -1 is default timeout
    if ((reply == NULL) || (dbusData->dbus.dbusErrorIsSet(&err)))
    {
        goto done;
    }

    /* read the parameters */
    if (!dbusData->dbus.dbusMessageIterInit(reply, &args))
    {
        nv_error_msg("vGPU License dbus communication: Message has no arguments!\n");
    }
    else if (DBUS_TYPE_STRING != dbusData->dbus.dbusMessageIterGetArgType(&args))
    {
        nv_error_msg("vGPU License dbus communication: Argument is not string!\n");
    }
    else
    {
        dbusData->dbus.dbusMessageIterGetBasic(&args, &receivedVal);
        if (receivedVal != NULL)
        {
            strncpy(value, receivedVal, BUF_LEN);
        }
    }

    ret = TRUE;
done:
    /* free message */
    dbusData->dbus.dbusMessageUnref(msg);
    return ret;
}

/*
 * update_manage_grid_license_state_info() - update manage_grid_license state
 */
static gboolean update_manage_grid_license_state_info(gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    gchar *licenseStatusMessage = "";
    gboolean ret = TRUE;
    char licenseStatusMsgTmp[GRID_MESSAGE_MAX_BUFFER_SIZE] = {0};

    int licenseState            = NV_GRID_UNLICENSED;
    int griddFeatureType        = ctk_manage_grid_license->feature_type;
    int portNum = 0;
    char licenseServerInfo[BUF_LEN] = { 0 };
    char port[BUF_LEN] = { 0 };

    /* Send license state and feature type request */
    if ((!(send_message_to_gridd(ctk_manage_grid_license, LICENSE_STATE_REQUEST,
                                 &licenseState))) ||
        (!(send_message_to_gridd(ctk_manage_grid_license, LICENSE_FEATURE_TYPE_REQUEST,
                                 &griddFeatureType)))) {
        licenseStatusMessage = "Unable to query license state information "
                               "from the NVIDIA vGPU "
                               "licensing daemon.\n"
                               "Please make sure nvidia-gridd and "
                               "dbus-daemon are running.\n";
        gtk_label_set_text(GTK_LABEL(ctk_manage_grid_license->label_license_state),
                           licenseStatusMessage);
        /* Disable Apply/Cancel button. */
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);
        /* Disable toggle buttons */
        if (ctk_manage_grid_license->radio_btn_vcompute) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_vcompute, FALSE);
        }
        if (ctk_manage_grid_license->radio_btn_vws) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_vws, FALSE);
        }
        if (ctk_manage_grid_license->radio_btn_vapp) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_vapp, FALSE);
        }

        licenseStateQueryFailed = TRUE;
        return ret;
    }

    if (licenseStateQueryFailed == TRUE) {
        /*Failure to communicate with vGPU licensing daemon in previous attempt.
         Refresh UI with parameters from the griddConfig.*/
        update_gui_from_griddconfig(ctk_manage_grid_license);
        licenseStateQueryFailed = FALSE;
    }

    /* Set the license feature type fetched from vGPU licensing daemon.*/
    ctk_manage_grid_license->gridd_feature_type = griddFeatureType;

    /* Validate GSP firmware support.
       As per current POR, GSP should be enabled only for vCS */
    switch (ctk_manage_grid_license->feature_type) {
        case NV_GRID_LICENSE_FEATURE_TYPE_VWS:
            if (ctk_manage_grid_license->gspDefaultMode && ctk_manage_grid_license->isGspEnabled) {
                ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_GSP_NOT_SUPPORTED_VWS;
            }
            break;
        case NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE:
            if (ctk_manage_grid_license->gspDefaultMode && !ctk_manage_grid_license->isGspEnabled) {
                ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_GSP_REQUIRED_VCS;
            }
            break;
    }

    if (licenseState == NV_GRID_UNLICENSED) {
        bQueryGridLicenseInfo = TRUE;
        switch (ctk_manage_grid_license->feature_type) {
            case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                ctk_manage_grid_license->licenseStatus = NV_GRID_UNLICENSED_VAPP;
                if (is_restart_required(ctk_manage_grid_license)) {
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_RESTART_REQUIRED_VAPP;
                }
                break;
            case NV_GRID_LICENSE_FEATURE_TYPE_VWS:
                if (ctk_manage_grid_license->licenseStatus == NV_GRID_LICENSE_GSP_NOT_SUPPORTED_VWS) {
                    break;
                }
                ctk_manage_grid_license->licenseStatus = NV_GRID_UNLICENSED_REQUEST_DETAILS_VWS;
                if (is_restart_required(ctk_manage_grid_license)) {
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_RESTART_REQUIRED_VWS;
                }
                break;
            case NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE:
                if (ctk_manage_grid_license->licenseStatus == NV_GRID_LICENSE_GSP_REQUIRED_VCS) {
                    break;
                }
                ctk_manage_grid_license->licenseStatus = NV_GRID_UNLICENSED_REQUEST_DETAILS_VCOMPUTE;
                if (is_restart_required(ctk_manage_grid_license)) {
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_RESTART_REQUIRED_VCOMPUTE;
                }
                break;
            case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                ctk_manage_grid_license->licenseStatus = NV_GRID_UNLICENSED_VGPU;
                break;
            default:
                break;
        }
    }
    else {
        if ((ctk_manage_grid_license->feature_type == ctk_manage_grid_license->gridd_feature_type) &&   // 'Apply' button is clicked
            ((licenseState == NV_GRID_LICENSED) ||
            (licenseState == NV_GRID_LICENSE_RENEWING) ||
            (licenseState == NV_GRID_LICENSE_RENEW_FAILED))) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_ACQUIRED;  // Default status in licensed state on non-vGPU case
                    if (is_restart_required(ctk_manage_grid_license)) {
                        ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSED_RESTART_REQUIRED_VAPP;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VWS:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_ACQUIRED;
                    if (is_restart_required(ctk_manage_grid_license)) {
                        ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSED_RESTART_REQUIRED_VWS;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_ACQUIRED;
                    if (is_restart_required(ctk_manage_grid_license)) {
                        ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSED_RESTART_REQUIRED_VCOMPUTE;
                    }

                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_ACQUIRED;
                    break;
                default:
                    break;
            }

            if (ctk_manage_grid_license->prevLicenseState == NV_GRID_LICENSE_RENEWING && licenseState == NV_GRID_LICENSED) {
                bQueryGridLicenseInfo = TRUE;
            }

            if (bQueryGridLicenseInfo == TRUE) {
                get_licensable_feature_information(ctk_manage_grid_license);
                bQueryGridLicenseInfo = FALSE;
            }
        }
        else if (licenseState == NV_GRID_LICENSE_REQUESTING) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                case NV_GRID_LICENSE_FEATURE_TYPE_VWS:
                case NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE:
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_REQUESTING;
                    break;
                default:
                    break;
            }
        }
        else if (licenseState == NV_GRID_LICENSE_FAILED) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                case NV_GRID_LICENSE_FEATURE_TYPE_VWS:
                case NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE:
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_FAILED;
                    break;
                default:
                    break;
            }
        }
        else if (licenseState == NV_GRID_LICENSE_RENEWING) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                case NV_GRID_LICENSE_FEATURE_TYPE_VWS:
                case NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE:
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_RENEWING;
                    break;
                default:
                    break;
            }
        }
        else if (licenseState == NV_GRID_LICENSE_RENEW_FAILED) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                case NV_GRID_LICENSE_FEATURE_TYPE_VWS:
                case NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE:
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_RENEW_FAILED;
                    break;
                default:
                    break;
            }
        }
        else if ((ctk_manage_grid_license->feature_type == ctk_manage_grid_license->gridd_feature_type) &&  // 'Apply' button is clicked
                 (licenseState == NV_GRID_LICENSE_EXPIRED)) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_EXPIRED;    // Default status in license expired state on non-vGPU case
                    if (is_restart_required(ctk_manage_grid_license)) {
                        ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSED_RESTART_REQUIRED_VAPP;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VWS:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_EXPIRED;
                    if (is_restart_required(ctk_manage_grid_license)) {
                        ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSED_RESTART_REQUIRED_VWS;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_EXPIRED;
                    if (is_restart_required(ctk_manage_grid_license)) {
                        ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSED_RESTART_REQUIRED_VCOMPUTE;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    ctk_manage_grid_license->licenseStatus = NV_GRID_LICENSE_STATUS_EXPIRED;
                    break;
                default:
                    break;
            }
        }
    }

    // Fetch License Server details from nvidia-gridd through dbus
    if (send_string_message_to_gridd(ctk_manage_grid_license, PRIMARY_SERVER_ADDRESS, licenseServerInfo))
    {
        // nvidia-gridd sends "Not Configured" if primary node is not available
        if ((strlen(licenseServerInfo) > 0) && (strcmp(licenseServerInfo, SERVER_DETAILS_NOT_CONFIGURED) != 0))
        {
            gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_address), licenseServerInfo);
            // Fetch primary server port number
            if (send_message_to_gridd(ctk_manage_grid_license, LICENSE_SERVER_PORT_REQUEST, &portNum))
            {
                snprintf(port, BUF_LEN, "%d", portNum);
            }
            if (strlen(port) > 0)
            {
                gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_port), port);
            }
        }

        memset(licenseServerInfo, '\0', BUF_LEN);
        // Fetch Secondary Server Address
        if (send_string_message_to_gridd(ctk_manage_grid_license, SECONDARY_SERVER_ADDRESS, licenseServerInfo))
        {
            // nvidia-gridd sends "Not Configured" if secondary node is not available
            if ((strlen(licenseServerInfo) > 0) && (strcmp(licenseServerInfo, SERVER_DETAILS_NOT_CONFIGURED) != 0))
            {
                gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_address), licenseServerInfo);
                portNum = 0;
                // Fetch secondary server port number
                if (send_message_to_gridd(ctk_manage_grid_license, LICENSE_SERVER_PORT_REQUEST, &portNum))
                {
                    memset(port, '\0', BUF_LEN);
                    snprintf(port, BUF_LEN, "%d", portNum);
                }
                if (strlen(port) > 0)
                {
                    gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_port), port);
                }
            }
        }
    }
    else
    {
        licenseStatusMessage = "Unable to query license server information "
                               "from the NVIDIA vGPU "
                               "licensing daemon.\n"
                               "Please make sure nvidia-gridd and "
                               "dbus-daemon are running.\n";
        gtk_label_set_text(GTK_LABEL(ctk_manage_grid_license->label_license_state), licenseStatusMessage);
        return ret;
    }

    switch (ctk_manage_grid_license->licenseStatus) {
    case NV_GRID_UNLICENSED_VGPU:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Your system does not have a valid %s license.\n"
              "Please configure client configuration token to acquire license.", ctk_manage_grid_license->productName);
          break;
    case NV_GRID_UNLICENSED_VAPP:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Your system is currently configured for %s.", NVIDIA_VIRTUAL_APPLICATIONS);
          break;
    case NV_GRID_LICENSE_STATUS_ACQUIRED:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Your system is licensed for %s, expiring at %s.",
                ctk_manage_grid_license->productName, ctk_manage_grid_license->licenseExpiry);
          break;
    case NV_GRID_LICENSE_STATUS_REQUESTING:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Acquiring license for %s.", ctk_manage_grid_license->productName);
          break;
    case NV_GRID_LICENSE_STATUS_FAILED:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Failed to acquire %s license.", ctk_manage_grid_license->productName);
          break;
    case NV_GRID_LICENSE_STATUS_RENEWING:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Renewing license for %s.", ctk_manage_grid_license->productName);
          break;
    case NV_GRID_LICENSE_STATUS_RENEW_FAILED:
         snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Failed to renew license for %s, expiring at %s.", ctk_manage_grid_license->productName, ctk_manage_grid_license->licenseExpiry);
         break;
    case NV_GRID_LICENSE_STATUS_EXPIRED:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "%s license has expired.", ctk_manage_grid_license->productName);
          break;
    case NV_GRID_LICENSED_RESTART_REQUIRED_VAPP:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Restart your system for %s.\n"
              "Your system is currently licensed for %s.", NVIDIA_VIRTUAL_APPLICATIONS, ctk_manage_grid_license->productName);
          break;
    case NV_GRID_LICENSED_RESTART_REQUIRED_VWS:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Restart your system for %s.\n"
              "Your system is currently licensed for %s.", ctk_manage_grid_license->productNamevWS, ctk_manage_grid_license->productName);
          break;
    case NV_GRID_LICENSED_RESTART_REQUIRED_VCOMPUTE:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Restart your system for %s.\n"
              "Your system is currently licensed for %s.", ctk_manage_grid_license->productNamevCompute, ctk_manage_grid_license->productName);
          break;
    case NV_GRID_LICENSE_RESTART_REQUIRED_VAPP:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Restart your system for %s.", NVIDIA_VIRTUAL_APPLICATIONS);
          break;
    case NV_GRID_LICENSE_RESTART_REQUIRED_VWS:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Restart your system for %s.", ctk_manage_grid_license->productNamevWS);
          break;
    case NV_GRID_LICENSE_RESTART_REQUIRED_VCOMPUTE:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Restart your system for %s.", ctk_manage_grid_license->productNamevCompute);
          break;
    case NV_GRID_UNLICENSED_REQUEST_DETAILS_VWS:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Your system does not have a valid %s license.\n"
              "Please configure client configuration token to acquire license.", ctk_manage_grid_license->productNamevWS);
          break;
    case NV_GRID_UNLICENSED_REQUEST_DETAILS_VCOMPUTE:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Your system does not have a valid %s license.\n"
              "Please configure client configuration token to acquire license.", ctk_manage_grid_license->productNamevCompute);
          break;
    case NV_GRID_LICENSE_GSP_REQUIRED_VCS:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Enable GSP firmware for %s.", ctk_manage_grid_license->productNamevCompute);
          break;
    case NV_GRID_LICENSE_GSP_NOT_SUPPORTED_VWS:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Disable GSP firmware for %s.", ctk_manage_grid_license->productNamevWS);
          break;
    default:
          snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "Your system does not have a valid GRID license.\n"
              "Please configure client configuration token to acquire license.");
          break;
    }

    gtk_label_set_text(GTK_LABEL(ctk_manage_grid_license->label_license_state),
                      licenseStatusMsgTmp);

    ctk_manage_grid_license->prevLicenseState = licenseState;

    return ret;
}

/*
 * is_restart_required() - Check if restart is required for new feature to be enabled.
 */
static gboolean is_restart_required(gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    gboolean ret = FALSE;

    /* Once licensed, system reboot required if there is mismatch between feature type
        updated from UI/gridd.conf and feature code of the feature that is licensed on this system. */
    if ((enabledFeatureCode) &&
        (enabledFeatureCode != ctk_manage_grid_license->feature_type)) {
        ret = TRUE;
    }
    return ret;
}

/*
 * apply_clicked() - Called when the user clicks on the "Apply" button.
 */
static void apply_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    gboolean ret;
    int err = 0;
    gint status = 0;
    gboolean configFileAvailable;
    gboolean writable = FALSE;

    /* Check available config file */
    configFileAvailable = checkConfigfile(&writable);
    if (configFileAvailable && writable) {
        /* Add information to gridd.conf file */
        err = UpdateConfigFile(ctk_manage_grid_license);
        if (err == 0) {
            /* Send update request to vGPU licensing daemon */
            ret = send_message_to_gridd(ctk_manage_grid_license,
                                        LICENSE_DETAILS_UPDATE_REQUEST,
                                        &status);
            if ((!ret) || (status != LICENSE_DETAILS_UPDATE_SUCCESS)) {
                GtkWidget *dlg, *parent;

                parent = ctk_get_parent_window(GTK_WIDGET(ctk_manage_grid_license));

                dlg = gtk_message_dialog_new(GTK_WINDOW(parent),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_WARNING,
                                             GTK_BUTTONS_OK,
                                             "Unable to send license information "
                                             "update request to the NVIDIA vGPU "
                                             "licensing daemon.\n"
                                             "Please make sure nvidia-gridd and "
                                             "dbus-daemon are running and retry applying the "
                                             "license settings.\n");
                gtk_dialog_run(GTK_DIALOG(dlg));
                gtk_widget_destroy(dlg);
            }
        } else {
            GtkWidget *dlg, *parent;

            parent = ctk_get_parent_window(GTK_WIDGET(ctk_manage_grid_license));
            dlg = gtk_message_dialog_new(GTK_WINDOW(parent),
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         "Unable to update license configuration "
                                         "file (%s): %s", GRID_CONFIG_FILE, strerror(err));
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        }
    } else if (configFileAvailable && !(writable)) {
        GtkWidget *dlg, *parent;

        parent = ctk_get_parent_window(GTK_WIDGET(ctk_manage_grid_license));
        dlg = gtk_message_dialog_new(GTK_WINDOW(parent),
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     "You do not have enough "
                                     "permissions to edit '%s' file.", GRID_CONFIG_FILE);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
    } else {
        GtkWidget *dlg, *parent;

        parent = ctk_get_parent_window(GTK_WIDGET(ctk_manage_grid_license));
        dlg = gtk_message_dialog_new(GTK_WINDOW(parent),
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     "'%s' file does not exist.\n You do not have "
                                     "permissions to create this file.", GRID_CONFIG_FILE);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
    }

    /* Disable Apply/Cancel button. */
    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);

    if (enabledFeatureCode == NV_GRID_LICENSE_FEATURE_TYPE_VAPP) {
        get_licensable_feature_information(ctk_manage_grid_license);
    }
}

/*
 * get_licensable_feature_information() - Get the details of the supported licensable features on the system.
 */
static void get_licensable_feature_information(gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    nvmlGridLicensableFeatures_t *gridLicensableFeatures;
    gint ret, i;

    // Add delay of 1 second to allow sometime for recently applied feature to
    // get enabled and set corresponding properties in RM before querying them
    sleep(1);

    ret = NvCtrlNvmlGetGridLicenseAttributes(ctk_manage_grid_license->target,
                                             NV_CTRL_ATTR_NVML_GPU_GRID_LICENSABLE_FEATURES,
                                             &gridLicensableFeatures);
    if (ret != NvCtrlSuccess) {
        return;
    }

    for (i = 0; i < gridLicensableFeatures->licensableFeaturesCount; i++)
    {
        if (gridLicensableFeatures->gridLicensableFeatures[i].featureEnabled != 0) {
            enabledFeatureCode = gridLicensableFeatures->gridLicensableFeatures[i].featureCode;
            // Save product name of enabled feature
            strncpy(ctk_manage_grid_license->productName,
                    gridLicensableFeatures->gridLicensableFeatures[i].productName,
                    sizeof(ctk_manage_grid_license->productName) - 1);
            ctk_manage_grid_license->productName[sizeof(ctk_manage_grid_license->productName) - 1] = '\0';
            //Save the license expiry information
            snprintf(ctk_manage_grid_license->licenseExpiry, sizeof(ctk_manage_grid_license->licenseExpiry), "%d-%d-%d %d:%d:%d GMT",
                     gridLicensableFeatures->gridLicensableFeatures[i].licenseExpiry.year,
                     gridLicensableFeatures->gridLicensableFeatures[i].licenseExpiry.month,
                     gridLicensableFeatures->gridLicensableFeatures[i].licenseExpiry.day,
                     gridLicensableFeatures->gridLicensableFeatures[i].licenseExpiry.hour,
                     gridLicensableFeatures->gridLicensableFeatures[i].licenseExpiry.min,
                     gridLicensableFeatures->gridLicensableFeatures[i].licenseExpiry.sec);
        }
        else if (gridLicensableFeatures->gridLicensableFeatures[i].featureEnabled == 0) {
            if ((ctk_manage_grid_license->feature_type == gridLicensableFeatures->gridLicensableFeatures[i].featureCode) &&
                !enabledFeatureCode) {
                // Save product name of feature applied from UI
                strncpy(ctk_manage_grid_license->productName,
                    gridLicensableFeatures->gridLicensableFeatures[i].productName,
                    sizeof(ctk_manage_grid_license->productName) - 1);
                ctk_manage_grid_license->productName[sizeof(ctk_manage_grid_license->productName) - 1] = '\0';
            }
        }

        if (strlen(ctk_manage_grid_license->productNamevWS) == 0 &&
            (int)gridLicensableFeatures->gridLicensableFeatures[i].featureCode == NV_GRID_LICENSE_FEATURE_TYPE_VWS) {
            // Save product name of feature corresponding to feature type '2'
            strncpy(ctk_manage_grid_license->productNamevWS,
                gridLicensableFeatures->gridLicensableFeatures[i].productName,
                sizeof(ctk_manage_grid_license->productNamevWS) - 1);
            ctk_manage_grid_license->productNamevWS[sizeof(ctk_manage_grid_license->productNamevWS) - 1] = '\0';
        }

        if (strlen(ctk_manage_grid_license->productNamevCompute) == 0 &&
            (int)gridLicensableFeatures->gridLicensableFeatures[i].featureCode == NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE) {
            // Save product name of feature corresponding to feature type '4'
            strncpy(ctk_manage_grid_license->productNamevCompute,
                gridLicensableFeatures->gridLicensableFeatures[i].productName,
                sizeof(ctk_manage_grid_license->productNamevCompute) - 1);
            ctk_manage_grid_license->productNamevCompute[sizeof(ctk_manage_grid_license->productNamevCompute) - 1] = '\0';
        }
    }

    nvfree(gridLicensableFeatures);
}

/*
 * is_feature_supported() - Check if the specified feature is supported
 */
static gboolean is_feature_supported(gpointer user_data, int featureType)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    nvmlGridLicensableFeatures_t *gridLicensableFeatures;
    gint ret, i;

    ret = NvCtrlNvmlGetGridLicenseAttributes(ctk_manage_grid_license->target,
                                             NV_CTRL_ATTR_NVML_GPU_GRID_LICENSABLE_FEATURES,
                                             &gridLicensableFeatures);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    for (i = 0; i < gridLicensableFeatures->licensableFeaturesCount; i++)
    {
        if (gridLicensableFeatures->gridLicensableFeatures[i].featureCode == featureType) {
            nvfree(gridLicensableFeatures);
            return TRUE;
        }
    }

    nvfree(gridLicensableFeatures);
    return FALSE;
}

/*
 * cancel_clicked() - Called when the user clicks on the "Cancel" button.
 */
static void cancel_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);

    /* Discard user input and revert back to configuration from the vGPU license config file */
    update_gui_from_griddconfig(ctk_manage_grid_license);

}

static void update_gui_from_griddconfig(gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    NvGriddConfigParams *griddConfig = NULL;

    griddConfig = GetNvGriddConfigParams();
    if (griddConfig) {
        /* set default value for feature type based on the user configured parameter or virtualization mode */
        updateFeatureTypeFromGriddConfig(ctk_manage_grid_license, griddConfig);

        /* Set license edition toggle button active */
        if (ctk_manage_grid_license->radio_btn_vcompute && ctk_manage_grid_license->feature_type == NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_manage_grid_license->radio_btn_vcompute), TRUE);
        }
        if (ctk_manage_grid_license->radio_btn_vws && ctk_manage_grid_license->feature_type == NV_GRID_LICENSE_FEATURE_TYPE_VWS) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_manage_grid_license->radio_btn_vws), TRUE);
        }
        if (ctk_manage_grid_license->radio_btn_vapp && ctk_manage_grid_license->feature_type == NV_GRID_LICENSE_FEATURE_TYPE_VAPP) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_manage_grid_license->radio_btn_vapp), TRUE);
        }

        /* Enable toggle buttons. */
        if (ctk_manage_grid_license->radio_btn_vcompute) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_vcompute, TRUE);
        }
        if (ctk_manage_grid_license->radio_btn_vws) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_vws, TRUE);
        }
        if (ctk_manage_grid_license->radio_btn_vapp) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_vapp, TRUE);
        }

        /* Disable Apply/Cancel button. */
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);
    }

    if(griddConfig)
        FreeNvGriddConfigParams(griddConfig);
}

static void license_edition_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    gboolean enabled;
    gchar *statusBarMsg = "";
    NvGriddConfigParams *griddConfig;
    char licenseStatusMsgTmp[GRID_MESSAGE_MAX_BUFFER_SIZE] = {0};

    griddConfig = GetNvGriddConfigParams();
    if (!griddConfig) {
        return;
    }

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    if (!enabled) {
        /* Ignore 'disable' events. */
        return;
    }
    user_data = g_object_get_data(G_OBJECT(widget), "button_id");

    if (GPOINTER_TO_INT(user_data) == NV_GRID_LICENSE_FEATURE_TYPE_VWS) {
        snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "You selected %s", ctk_manage_grid_license->productNamevWS);
        statusBarMsg = licenseStatusMsgTmp;
        ctk_manage_grid_license->feature_type =
            NV_GRID_LICENSE_FEATURE_TYPE_VWS;
        /* Enable Apply/Cancel button if the feature type selection has changed*/
        if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "2") != 0) {
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, TRUE);
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, TRUE);
        }
    }  else if (GPOINTER_TO_INT(user_data) == NV_GRID_LICENSE_FEATURE_TYPE_VAPP) {
        ctk_manage_grid_license->feature_type = 
            NV_GRID_LICENSE_FEATURE_TYPE_VAPP;
        snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "You selected %s", NVIDIA_VIRTUAL_APPLICATIONS);
        statusBarMsg = licenseStatusMsgTmp;
        /* Enable Apply/Cancel button if the feature type selection has changed*/
        if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "0") != 0) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, TRUE);
            gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, TRUE);
        }
    } else if (GPOINTER_TO_INT(user_data) == NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE) {
        snprintf(licenseStatusMsgTmp, sizeof(licenseStatusMsgTmp), "You selected %s", ctk_manage_grid_license->productNamevCompute);
        statusBarMsg = licenseStatusMsgTmp;
        ctk_manage_grid_license->feature_type =
            NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE;
        /* Enable Apply/Cancel button if the feature type selection has changed*/
        if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "4") != 0) {
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, TRUE);
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, TRUE);
        }
    }

    /* update status bar message */
    ctk_config_statusbar_message(ctk_manage_grid_license->ctk_config,
                                 "%s", statusBarMsg);
    update_manage_grid_license_state_info(ctk_manage_grid_license);
    FreeNvGriddConfigParams(griddConfig);
}

static gboolean checkConfigfile(gboolean *writable)
{
    struct stat st;
    gboolean ret = FALSE;
    FILE *configFile = NULL;
    FILE *templateFile = NULL;

    /* Check if user can create gridd.conf file */
    configFile = fopen(GRID_CONFIG_FILE, "r+");
    if (configFile) {
        *writable = TRUE;
    } else {
        /* check if file is readable */
        configFile = fopen(GRID_CONFIG_FILE, "r");
        if (!configFile) {
            /* file does not exist so create new file */
            configFile = fopen(GRID_CONFIG_FILE, "w");
            if (configFile) {
                *writable = TRUE;
            } else {
                goto done;
            }
        } else {
            *writable = FALSE;
        }
    }

    /* Check if config file available */
    if ((fstat(fileno(configFile), &st) == 0) && (st.st_size == 0)) {
        if (*writable) {
            ConfigFileLines *pLines;
            templateFile = fopen(GRID_CONFIG_FILE_TEMPLATE, "r");

            if (templateFile == NULL) {
                nv_error_msg("Config file '%s' had size zero.",
                             GRID_CONFIG_FILE);
            } else {
                pLines = ReadConfigFileStream(templateFile);

                WriteConfigFileStream(configFile, pLines);

                FreeConfigFileLines(pLines);
            }
        } else {
            nv_error_msg("Config file '%s' had size zero.",
                         GRID_CONFIG_FILE);
        }
    }
    ret = TRUE;
done:
    if (templateFile) {
        fclose(templateFile);
    }
    if (configFile) {
        fclose(configFile);
    }
    return ret;
}

GtkWidget* ctk_manage_grid_license_new(CtrlTarget *target,
                                       CtkConfig *ctk_config)
{
    GObject *object;
    CtkManageGridLicense *ctk_manage_grid_license = NULL;
    GtkWidget *hbox, *vbox, *vbox1, *vbox2 = NULL, *vbox3;
    GtkWidget *table;
    GtkWidget *eventbox;
    GtkWidget *banner, *label, *frame;
    gchar *str = NULL;
    int64_t mode;
    GSList *slist = NULL;
    gint ret;
    DBusError err;
    DBusConnection* conn;
    DbusData *dbusData;
    int64_t gridLicenseSupported;
    NvGriddConfigParams *griddConfig;
    gint isGspEnabled = 0;
    gint gspDefaultMode = 0;

    /* make sure we have a handle */

    g_return_val_if_fail((target != NULL) &&
                         (target->h != NULL), NULL);

    /*
     * check if Manage license page is available.
     */

    ret = NvCtrlNvmlGetAttribute(target,
                                 NV_CTRL_ATTR_NVML_GPU_GRID_LICENSE_SUPPORTED,
                                 &gridLicenseSupported);

    if ((ret != NvCtrlSuccess) ||
        ((ret == NvCtrlSuccess) &&
         gridLicenseSupported ==
              NV_CTRL_ATTR_NVML_GPU_GRID_LICENSE_SUPPORTED_FALSE)) {
        return NULL;
    }

    /* Query the virtualization mode */
    ret = NvCtrlNvmlGetAttribute(target,
                                 NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE,
                                 &mode);

    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    /* Query GSP firmware attributes */
    ret = NvCtrlNvmlDeviceGetGspAttributes(target,
                                           NV_CTRL_ATTR_NVML_GSP_FIRMWARE_MODE,
                                           (unsigned int*)&isGspEnabled, (unsigned int*)&gspDefaultMode);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    /* GRID M6 is licensable gpu so we want to allow users to choose
     * NVIDIA RTX Virtual Workstation and NVIDIA Virtual Applications on baremetal setup.
     * When virtualization mode is NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_NONE
     * treat it same way like NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH.
     * So that it will show the NVIDIA RTX Virtual Workstation interface in case of
     * baremetal setup.
     */
    if (mode == NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_NONE) {
        mode = NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH;
    }

    /* DBUS calls are used for querying the current license status  */
    dbusData = nvalloc(sizeof(*dbusData));

    ret = dbusLoadSymbols(dbusData);
    
    if (!ret) {
        return NULL;
    }
    dbusData->dbus.dbusErrorInit(&err);
    
    /* connect to the system bus */
    conn = dbusData->dbus.getDbus(DBUS_BUS_SYSTEM, &err);
    if (dbusData->dbus.dbusErrorIsSet(&err)) {
        dbusData->dbus.dbusErrorFree(&err);
    }
    if (NULL == conn) {
        return NULL;
    }
    dbusData->conn = conn;

    /* request the bus name */
    dbusData->dbus.dbusRequestName(conn, NV_GRID_DBUS_CLIENT,
                                   DBUS_NAME_FLAG_REPLACE_EXISTING,
                                   &err);
    if (dbusData->dbus.dbusErrorIsSet(&err)) {
        dbusData->dbus.dbusErrorFree(&err);
    }

    /* Initialize config parameters */
    griddConfig = GetNvGriddConfigParams();
    if (griddConfig &&
        (strcmp(griddConfig->str[NV_GRIDD_ENABLE_UI], "TRUE") != 0)) {
        return NULL;
    }

    /* create the CtkManageGridLicense object */
    object = g_object_new(CTK_TYPE_MANAGE_GRID_LICENSE, NULL);

    ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(object);
    ctk_manage_grid_license->ctk_config = ctk_config;
    ctk_manage_grid_license->license_edition_state = mode;
    ctk_manage_grid_license->dbusData = dbusData;
    ctk_manage_grid_license->feature_type = 0;
    ctk_manage_grid_license->target = target;
    ctk_manage_grid_license->licenseStatus = NV_GRID_UNLICENSED_VGPU;
    ctk_manage_grid_license->isGspEnabled = isGspEnabled;
    ctk_manage_grid_license->gspDefaultMode = gspDefaultMode;

    /* set container properties for the CtkManageGridLicense widget */
    gtk_box_set_spacing(GTK_BOX(ctk_manage_grid_license), 5);

    // Set feature type based on user configured parameter or virtualization mode
    if (griddConfig != NULL)
    {
        updateFeatureTypeFromGriddConfig(ctk_manage_grid_license, griddConfig);
    }

    get_licensable_feature_information(ctk_manage_grid_license);

    /* banner */
    banner = ctk_banner_image_new(BANNER_ARTWORK_SERVER_LICENSING);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), vbox, FALSE, FALSE, 0);

    vbox1 = gtk_vbox_new(FALSE, 5);
    vbox2 = gtk_vbox_new(FALSE, 0);
    frame = gtk_frame_new("");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox1);

    if (mode == NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH) {
        label = gtk_label_new("License Edition:");
        hbox = gtk_hbox_new(FALSE, 0);
        eventbox = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(eventbox), label);
        ctk_config_set_tooltip(ctk_config, eventbox, __license_edition_help);

        gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 5);
        vbox3 = gtk_vbox_new(FALSE, 5);
        gtk_container_add(GTK_CONTAINER(vbox1), vbox3);
        gtk_container_set_border_width(GTK_CONTAINER(vbox3), 5);

        ctk_manage_grid_license->isvComputeSupported = is_feature_supported(ctk_manage_grid_license, NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE);
        ctk_manage_grid_license->isvWSSupported = is_feature_supported(ctk_manage_grid_license, NV_GRID_LICENSE_FEATURE_TYPE_VWS);
        if (ctk_manage_grid_license->isvComputeSupported && ctk_manage_grid_license->isvWSSupported) {
            ctk_manage_grid_license->radio_btn_vcompute = gtk_radio_button_new_with_label(NULL, ctk_manage_grid_license->productNamevCompute);
            slist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(ctk_manage_grid_license->radio_btn_vcompute));

            gtk_box_pack_start(GTK_BOX(vbox3), ctk_manage_grid_license->radio_btn_vcompute, FALSE, FALSE, 0);
            g_object_set_data(G_OBJECT(ctk_manage_grid_license->radio_btn_vcompute), "button_id",
                              GINT_TO_POINTER(NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE));

            g_signal_connect(G_OBJECT(ctk_manage_grid_license->radio_btn_vcompute), "toggled",
                             G_CALLBACK(license_edition_toggled),
                             (gpointer) ctk_manage_grid_license);

            ctk_manage_grid_license->radio_btn_vws = gtk_radio_button_new_with_label(slist, ctk_manage_grid_license->productNamevWS);
            slist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(ctk_manage_grid_license->radio_btn_vws));

            gtk_box_pack_start(GTK_BOX(vbox3), ctk_manage_grid_license->radio_btn_vws, FALSE, FALSE, 0);
            g_object_set_data(G_OBJECT(ctk_manage_grid_license->radio_btn_vws), "button_id",
                              GINT_TO_POINTER(NV_GRID_LICENSE_FEATURE_TYPE_VWS));

            g_signal_connect(G_OBJECT(ctk_manage_grid_license->radio_btn_vws), "toggled",
                             G_CALLBACK(license_edition_toggled),
                             (gpointer) ctk_manage_grid_license);

        }
        else if (ctk_manage_grid_license->isvComputeSupported) {
            ctk_manage_grid_license->radio_btn_vcompute = gtk_radio_button_new_with_label(NULL, ctk_manage_grid_license->productNamevCompute);
            slist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(ctk_manage_grid_license->radio_btn_vcompute));

            gtk_box_pack_start(GTK_BOX(vbox3), ctk_manage_grid_license->radio_btn_vcompute, FALSE, FALSE, 0);
            g_object_set_data(G_OBJECT(ctk_manage_grid_license->radio_btn_vcompute), "button_id",
                              GINT_TO_POINTER(NV_GRID_LICENSE_FEATURE_TYPE_VCOMPUTE));

            g_signal_connect(G_OBJECT(ctk_manage_grid_license->radio_btn_vcompute), "toggled",
                             G_CALLBACK(license_edition_toggled),
                             (gpointer) ctk_manage_grid_license);
        }
        else if (ctk_manage_grid_license->isvWSSupported) {
            ctk_manage_grid_license->radio_btn_vws = gtk_radio_button_new_with_label(NULL, ctk_manage_grid_license->productNamevWS);
            slist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(ctk_manage_grid_license->radio_btn_vws));

            gtk_box_pack_start(GTK_BOX(vbox3), ctk_manage_grid_license->radio_btn_vws, FALSE, FALSE, 0);
            g_object_set_data(G_OBJECT(ctk_manage_grid_license->radio_btn_vws), "button_id",
                              GINT_TO_POINTER(NV_GRID_LICENSE_FEATURE_TYPE_VWS));

            g_signal_connect(G_OBJECT(ctk_manage_grid_license->radio_btn_vws), "toggled",
                             G_CALLBACK(license_edition_toggled),
                             (gpointer) ctk_manage_grid_license);
        }

        ctk_manage_grid_license->radio_btn_vapp = gtk_radio_button_new_with_label(slist, NVIDIA_VIRTUAL_APPLICATIONS);
        gtk_box_pack_start(GTK_BOX(vbox3), ctk_manage_grid_license->radio_btn_vapp, FALSE, FALSE, 0);
        g_object_set_data(G_OBJECT(ctk_manage_grid_license->radio_btn_vapp), "button_id",
                          GINT_TO_POINTER(NV_GRID_LICENSE_FEATURE_TYPE_VAPP));

        g_signal_connect(G_OBJECT(ctk_manage_grid_license->radio_btn_vapp), "toggled",
                         G_CALLBACK(license_edition_toggled),
                         (gpointer) ctk_manage_grid_license);

    }

    /* Show current license status message */
    ctk_manage_grid_license->label_license_state = gtk_label_new("Unknown");
    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox),
                      ctk_manage_grid_license->label_license_state);
    ctk_config_set_tooltip(ctk_config, eventbox, __license_edition_help);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 5);
    
    frame = gtk_frame_new("");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox2);
    
    /* License Server */
    label = gtk_label_new("License Server Details:");
    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __license_server_help);
    gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 5);
    vbox3 = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox3), 5);
    gtk_box_pack_start(GTK_BOX(vbox2), vbox3, FALSE, FALSE, 5);

    table = gtk_table_new(7, 5, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox3), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);


    /* Primary License Server Address */
    label = gtk_label_new("Primary Server Address:");
    ctk_manage_grid_license->txt_server_address = gtk_entry_new();
    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __primary_server_address_help);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

    /* value */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_set_size_request(ctk_manage_grid_license->txt_server_address, BUF_LEN, -1);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_server_address,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* Port Number */
    label = gtk_label_new("Port Number:");

    hbox = gtk_hbox_new(FALSE, 5);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __primary_server_port_help);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

    /* value */
    ctk_manage_grid_license->txt_server_port = gtk_entry_new();

    gtk_widget_set_size_request(ctk_manage_grid_license->txt_server_port, BUF_LEN, -1);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_server_port,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* Backup Server Address */
    label = gtk_label_new("Secondary Server Address:");
    ctk_manage_grid_license->txt_secondary_server_address = gtk_entry_new();

    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox,
                           __secondary_server_help);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 5, 6,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

    /* value */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_set_size_request(ctk_manage_grid_license->txt_secondary_server_address, BUF_LEN, -1);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_secondary_server_address,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 5, 6,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* Port Number */
    label = gtk_label_new("Port Number:");

    hbox = gtk_hbox_new(FALSE, 5);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __secondary_server_port_help);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 6, 7,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

    /* value */
    ctk_manage_grid_license->txt_secondary_server_port = gtk_entry_new();
    gtk_widget_set_size_request(ctk_manage_grid_license->txt_secondary_server_port, BUF_LEN, -1);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_secondary_server_port,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 6, 7,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    ctk_manage_grid_license->box_server_info = vbox2;

    /* Apply button */
    ctk_manage_grid_license->btn_apply = gtk_button_new_with_label
        (" Apply ");
    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
    gtk_widget_set_size_request(ctk_manage_grid_license->btn_apply, 100, -1);
    ctk_config_set_tooltip(ctk_config, ctk_manage_grid_license->btn_apply,
                           __apply_button_help);
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_manage_grid_license->btn_apply, FALSE, FALSE, 5);

    g_signal_connect(G_OBJECT(ctk_manage_grid_license->btn_apply), "clicked",
                     G_CALLBACK(apply_clicked),
                     (gpointer) ctk_manage_grid_license);

    /* Cancel button */
    ctk_manage_grid_license->btn_cancel = gtk_button_new_with_label
        (" Cancel ");
    gtk_widget_set_size_request(ctk_manage_grid_license->btn_cancel, 100, -1);
    ctk_config_set_tooltip(ctk_config, ctk_manage_grid_license->btn_cancel,
                           __cancel_button_help);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_manage_grid_license->btn_cancel, FALSE, FALSE, 5);

    g_signal_connect(G_OBJECT(ctk_manage_grid_license->btn_cancel), "clicked",
                     G_CALLBACK(cancel_clicked),
                     (gpointer) ctk_manage_grid_license);

    // Disable write action text boxes for license server information
    gtk_widget_set_sensitive(ctk_manage_grid_license->txt_server_address, FALSE);
    gtk_widget_set_sensitive(ctk_manage_grid_license->txt_server_port, FALSE);
    gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_address, FALSE);
    gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_port, FALSE);

    /* Update GUI with information from the vGPU license config file */
    update_gui_from_griddconfig(ctk_manage_grid_license);

    /* Set the value of feature type fetched from vGPU licensing daemon
       to feature type fetched from vGPU license config file */
    ctk_manage_grid_license->gridd_feature_type = ctk_manage_grid_license->feature_type;

    /* Register a timer callback to update license status info */
    str = g_strdup_printf("Manage vGPU Software License");

    ctk_config_add_timer(ctk_manage_grid_license->ctk_config,
                         DEFAULT_UPDATE_GRID_LICENSE_STATUS_INFO_TIME_INTERVAL,
                         str,
                         (GSourceFunc) update_manage_grid_license_state_info,
                         (gpointer) ctk_manage_grid_license);
    update_manage_grid_license_state_info(ctk_manage_grid_license);

    /* Enable Apply/Cancel button if there is mismatch between feature type fetched from vGPU licensing daemon
        and feature type updated from UI/vGPU license config file. */
    if (ctk_manage_grid_license->feature_type != ctk_manage_grid_license->gridd_feature_type) {
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, TRUE);
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, TRUE);
    }
    else {
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);
    }

    g_free(str);
    FreeNvGriddConfigParams(griddConfig);

    gtk_widget_show_all(GTK_WIDGET(ctk_manage_grid_license));
    
    return GTK_WIDGET(ctk_manage_grid_license);
}


GtkTextBuffer *ctk_manage_grid_license_create_help(GtkTextTagTable *table,
                                   CtkManageGridLicense *ctk_manage_grid_license)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_heading(b, &i, "Manage vGPU Software Licenses Help");

    if (ctk_manage_grid_license->isvComputeSupported) {
        ctk_help_para(b, &i, "%s", __manage_grid_licenses_vcompute_help);
    }
    else {
        ctk_help_para(b, &i, "%s", __manage_grid_licenses_help);
    }

    if (ctk_manage_grid_license->license_edition_state ==
        NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH) {
        if (ctk_manage_grid_license->isvComputeSupported) {
            ctk_help_heading(b, &i, "%s", ctk_manage_grid_license->productNamevCompute);
            ctk_help_para(b, &i, "%s", __grid_virtual_compute_help);
        }
        ctk_help_heading(b, &i, "%s", ctk_manage_grid_license->productNamevWS);
        ctk_help_para(b, &i, "%s", __grid_virtual_workstation_help);

        ctk_help_heading(b, &i, "%s", NVIDIA_VIRTUAL_APPLICATIONS);
        ctk_help_para(b, &i, "%s", __grid_vapp_help);
    }

    ctk_help_heading(b, &i, "License Server Details");
    ctk_help_para(b, &i, "%s", __license_server_help);

    ctk_help_heading(b, &i, "Primary Server Address");
    ctk_help_para(b, &i, "%s", __primary_server_address_help);

    ctk_help_heading(b, &i, "Port Number");
    ctk_help_para(b, &i, "%s", __primary_server_port_help);

    ctk_help_heading(b, &i, "Secondary Server Address");
    ctk_help_para(b, &i, "%s", __secondary_server_help);

    ctk_help_heading(b, &i, "Port Number");
    ctk_help_para(b, &i, "%s", __secondary_server_port_help);

    ctk_help_heading(b, &i, "Apply");
    ctk_help_para(b, &i, "%s", __apply_button_help);

    ctk_help_heading(b, &i, "Cancel");
    ctk_help_para(b, &i, "%s", __cancel_button_help);

    ctk_help_finish(b);

    return b;
}

void ctk_manage_grid_license_start_timer(GtkWidget *widget)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(widget);

    /* Start the manage_grid_license timer */

    ctk_config_start_timer(ctk_manage_grid_license->ctk_config,
                           (GSourceFunc) update_manage_grid_license_state_info,
                           (gpointer) ctk_manage_grid_license);

    /* Query licensable feature info if switched to 'Manage License' page
       from any other page to update license expiry information */
    if (ctk_manage_grid_license->licenseStatus == NV_GRID_LICENSE_STATUS_ACQUIRED) {
        get_licensable_feature_information(ctk_manage_grid_license);
    }
}

void ctk_manage_grid_license_stop_timer(GtkWidget *widget)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(widget);

    /* Stop the manage_grid_license timer */

    ctk_config_stop_timer(ctk_manage_grid_license->ctk_config,
                          (GSourceFunc) update_manage_grid_license_state_info,
                          (gpointer) ctk_manage_grid_license);
}
