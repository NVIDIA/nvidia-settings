/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2017 NVIDIA Corporation.
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


#define DEFAULT_UPDATE_GRID_LICENSE_STATUS_INFO_TIME_INTERVAL 1000
#define GRID_CONFIG_FILE            "/etc/nvidia/gridd.conf"
#define GRID_CONFIG_FILE_TEMPLATE   "/etc/nvidia/gridd.conf.template"

static const char * __manage_grid_licenses_help =
"Use the Manage GRID Licenses page to obtain a license for GRID vGPU or GRID "
"Virtual Workstation on supported Tesla products.";
static const char * __grid_virtual_workstation_help =
"Allows to enter license server details like server address and port number.";
static const char * __tesla_unlicensed_help =
"Allows to run system in unlicensed mode.";
static const char * __license_edition_help =
"The License Edition section shows if your system has a valid GRID vGPU "
"license.";
static const char * __license_server_help =
"Shows the GRID license server details configured.";
static const char * __primary_server_address_help =
"Primary license server address.";
static const char * __primary_server_port_help =
"License server port number. The default value is 7070.";
static const char * __secondary_server_help =
"Backup license server address to fall back to "
"if the primary license server is not reachable. "
"This setting is optional.";
static const char * __secondary_server_port_help =
"Backup license server port number. The default value is 7070.";
static const char * __apply_button_help =
"Clicking the Apply button updates values in the gridd.conf file and "
"sends update license request to the NVIDIA GRID licensing daemon.";

typedef struct 
{
    void                                        *dbusHandle;
    typeof(dbus_bus_get)                        *getDbus;
    typeof(dbus_error_init)                     *dbusErrorInit;
    typeof(dbus_error_is_set)                   *dbusErrorIsSet;
    typeof(dbus_error_free)                     *dbusErrorFree;
    typeof(dbus_bus_request_name)               *dbusRequestName;
    typeof(dbus_message_new_method_call)        *dbusMessageNewMethodCall;
    typeof(dbus_message_iter_init)              *dbusMessageIterInit;
    typeof(dbus_message_iter_init_append)       *dbusMessageIterInitAppend;
    typeof(dbus_message_iter_append_basic)      *dbusMessageIterAppendBasic;
    typeof(dbus_message_iter_get_arg_type)      *dbusMessageIterGetArgType;
    typeof(dbus_message_iter_get_basic)         *dbusMessageIterGetBasic;
    typeof(dbus_message_unref)                  *dbusMessageUnref;
    typeof(dbus_connection_close)               *dbusConnectionClose;
    typeof(dbus_connection_flush)               *dbusConnectionFlush;
    typeof(dbus_connection_send_with_reply)     *dbusConnectionSendWithReply;
    typeof(dbus_pending_call_block)             *dbusPendingCallBlock;
    typeof(dbus_pending_call_steal_reply)       *dbusPendingCallStealReply;
} GridDbus;

struct _DbusData
{
    DBusConnection *conn;
    GridDbus       dbus;
};

static void apply_clicked(GtkWidget *widget, gpointer user_data);
static void ctk_manage_grid_license_finalize(GObject *object);
static void ctk_manage_grid_license_class_init(CtkManageGridLicenseClass *);
static void dbusClose(DbusData *dbusData);
static gboolean checkConfigfile(gboolean *writable);
static gboolean disallow_whitespace(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static gboolean allow_digits(GtkWidget *widget, GdkEvent *event, gpointer user_data);

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
    NV_GRIDD_SERVER_ADDRESS = 0,
    NV_GRIDD_SERVER_PORT,
    NV_GRIDD_FEATURE_TYPE,
    NV_GRIDD_ENABLE_UI,
    NV_GRIDD_BACKUP_SERVER_ADDRESS,
    NV_GRIDD_BACKUP_SERVER_PORT,
    NV_GRIDD_MAX_TOKENS
} CfgParams;

static const char *configParamsList[] = {
    [NV_GRIDD_SERVER_ADDRESS]        = "ServerAddress",
    [NV_GRIDD_SERVER_PORT]           = "ServerPort",
    [NV_GRIDD_FEATURE_TYPE]          = "FeatureType",
    [NV_GRIDD_ENABLE_UI]             = "EnableUI",
    [NV_GRIDD_BACKUP_SERVER_ADDRESS] = "BackupServerAddress",
    [NV_GRIDD_BACKUP_SERVER_PORT]    = "BackupServerPort",
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

static void FreeConfigFileLines(ConfigFileLines *pLines)
{
    int i;

    for (i = 0; i < pLines->nLines; i++) {
        nvfree(pLines->lines[i]);
    }

    nvfree(pLines->lines);
    nvfree(pLines);
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

    for (i = 0; i < ARRAY_LEN(griddConfig->str); i++) {
        nvfree(griddConfig->str[i]);
    }

    nvfree(griddConfig);
}

static NvGriddConfigParams *AllocNvGriddConfigParams(void)
{
    NvGriddConfigParams *griddConfig = nvalloc(sizeof(*griddConfig));

    griddConfig->str[NV_GRIDD_SERVER_ADDRESS]        = nvstrdup("");
    griddConfig->str[NV_GRIDD_SERVER_PORT]           = nvstrdup("");
    griddConfig->str[NV_GRIDD_FEATURE_TYPE]          = nvstrdup("0");
    griddConfig->str[NV_GRIDD_ENABLE_UI]             = nvstrdup("TRUE");
    griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS] = nvstrdup("");
    griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT]    = nvstrdup("");

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
 * Update NvGriddConfigParams if the line from gridd.conf describes any
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

    /* serverAddress */

    nvfree(griddConfig->str[NV_GRIDD_SERVER_ADDRESS]);
    tmp = gtk_entry_get_text(GTK_ENTRY(
                              ctk_manage_grid_license->txt_server_address));
    griddConfig->str[NV_GRIDD_SERVER_ADDRESS] = nvstrdup(tmp ? tmp : "");

    /* serverPort */

    nvfree(griddConfig->str[NV_GRIDD_SERVER_PORT]);
    tmp = gtk_entry_get_text(GTK_ENTRY(
                              ctk_manage_grid_license->txt_server_port));
    griddConfig->str[NV_GRIDD_SERVER_PORT] =
        nvstrdup((strcmp(tmp, "") != 0) ? tmp : "7070");

    /* featureType */

    nvfree(griddConfig->str[NV_GRIDD_FEATURE_TYPE]);
    switch (ctk_manage_grid_license->feature_type) {
    case GRID_LICENSED_FEATURE_TYPE_TESLA:
        tmp = "0";
        break;
    case GRID_LICENSED_FEATURE_TYPE_VGPU:
        tmp = "1";
        break;
    case GRID_LICENSED_FEATURE_TYPE_GVW:
        tmp = "2";
        break;
    default:
        tmp = "0";
    }
    griddConfig->str[NV_GRIDD_FEATURE_TYPE] = nvstrdup(tmp);

    /* note: nothing in the UI will alter enableUI */

    /* backupServerAddress */

    nvfree(griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS]);
    tmp = gtk_entry_get_text(GTK_ENTRY(
                              ctk_manage_grid_license->txt_secondary_server_address));
    griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS] = nvstrdup(tmp ? tmp : "");

    /* backupServerPort */

    nvfree(griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT]);
    tmp = gtk_entry_get_text(GTK_ENTRY(
                              ctk_manage_grid_license->txt_secondary_server_port));
    griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT] = nvstrdup(tmp ? tmp : "");
}

/*
 * Read the nvidia-gridd config file specified by configFile, returning the
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

static ConfigFileLines *ReadConfigFile(void)
{
    ConfigFileLines *pLines;
    FILE *configFile = fopen(GRID_CONFIG_FILE, "r");

    if (configFile == NULL) {
        return NULL;
    }

    pLines = ReadConfigFileStream(configFile);

    fclose(configFile);

    return pLines;
}

static void WriteConfigFileStream(FILE *configFile, const ConfigFileLines *pLines)
{
    int i;

    for (i = 0; i < pLines->nLines; i++) {
        fprintf(configFile, "%s\n", pLines->lines[i]);
    }
}

static gboolean WriteConfigFile(const ConfigFileLines *pLines)
{
    FILE *configFile;

    configFile = fopen(GRID_CONFIG_FILE, "w");

    if (configFile == NULL) {
        return FALSE;
    }

    WriteConfigFileStream(configFile, pLines);

    fclose(configFile);

    return TRUE;
}


/*
 * Update the nvidia-gridd config file with the current GUI state.
 *
 */
static gboolean UpdateConfigFile(CtkManageGridLicense *ctk_manage_grid_license)
{
    ConfigFileLines *pLines;
    NvGriddConfigParams *griddConfig;
    gboolean ret;

    /* Read gridd.conf */
    pLines = ReadConfigFile();

    if (pLines == NULL) {
        return FALSE;
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
    ret = WriteConfigFile(pLines);

    FreeConfigFileLines(pLines);
    FreeNvGriddConfigParams(griddConfig);

    return ret;
}

/*
 * Create an NvGriddConfigParams by parsing the configuration file and
 * populating NvGriddConfigParams.  The caller should call
 * FreeNvGriddConfigParams() when done with it.
 *
 */
static NvGriddConfigParams *GetNvGriddConfigParams(void)
{
    ConfigFileLines *pLines;
    NvGriddConfigParams *griddConfig;

    /* Read gridd.conf */
    pLines = ReadConfigFile();

    if (pLines == NULL) {
        return NULL;
    }

    /* Create a griddConfig */
    griddConfig = AllocNvGriddConfigParams();

    /* Update the griddConfig with the lines from gridd.conf */
    UpdateGriddConfigFromConfigFileLines(griddConfig, pLines);

    FreeConfigFileLines(pLines);

    return griddConfig;
}


static void ctk_manage_grid_license_class_init(CtkManageGridLicenseClass *ctk_object_class)
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
 * nvidia-gridd and nvidia-settings using DBus.
 * Nvidia-settings sends different command messages to either query info
 * such as license state or to notify nvidia-gridd to update license
 * info changes.
 */
static gboolean
send_message_to_gridd(CtkManageGridLicense *ctk_manage_grid_license,
                      gint param,
                      gint *pStatus)
{
    gboolean ret = FALSE;

    DBusMessage* msg;
    DBusMessageIter args;
    DBusPendingCall* pending;
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

    /* send message and get a handle for a reply */
    if (!dbusData->dbus.dbusConnectionSendWithReply(conn,
                                      msg, &pending, -1)) { // -1 is default timeout
        goto done;
    }
    if (NULL == pending) {
        goto done;
    }
    dbusData->dbus.dbusConnectionFlush(conn);

    /* free message */
    dbusData->dbus.dbusMessageUnref(msg);

    /* block until we receive a reply */
    dbusData->dbus.dbusPendingCallBlock(pending);

    /* get the reply */
    msg = dbusData->dbus.dbusPendingCallStealReply(pending);
    if (NULL == msg) {
        goto done;
    }
    
    /* read the parameters */
    if (!dbusData->dbus.dbusMessageIterInit(msg, &args)) {
        nv_error_msg("GRID License dbus communication: Message has no arguments!\n");
    } else if (DBUS_TYPE_INT32 !=
               dbusData->dbus.dbusMessageIterGetArgType(&args)) {
        nv_error_msg("GRID License dbus communication: Argument is not int!\n");
    } else {
        dbusData->dbus.dbusMessageIterGetBasic(&args, pStatus);
    }

    ret = TRUE;
done:
    /* free reply */
    dbusData->dbus.dbusMessageUnref(msg);

    return ret;
}

    
/*
 * update_manage_grid_license_state_info() - update manage_grid_license state
 */

static gboolean update_manage_grid_license_state_info(gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    gchar *licenseState = "";
    gboolean ret = TRUE;

    int licenseStatus = NV_GRID_UNLICENSED;

    /* Send license state request */
    if (!(send_message_to_gridd(ctk_manage_grid_license, LICENSE_STATE_REQUEST,
                                &licenseStatus))) {
        ret = FALSE;
    }

    /* Set default unlicensed state string if could not communicate with nvidia-gridd */
    if (licenseStatus == NV_GRID_UNLICENSED)
    {
        switch (ctk_manage_grid_license->feature_type) {
        case GRID_LICENSED_FEATURE_TYPE_VGPU:
            licenseStatus = NV_GRID_UNLICENSED_VGPU;
            break;
        case GRID_LICENSED_FEATURE_TYPE_TESLA:
        case GRID_LICENSED_FEATURE_TYPE_GVW:
            licenseStatus = NV_GRID_UNLICENSED_TESLA;
            break;
        default:
            break;
        }
    }

    /* Set correct status message when Grid Virtual Workstation
     * checkbox selected */ 
    if ((licenseStatus == NV_GRID_UNLICENSED_TESLA) &&
        (ctk_manage_grid_license->license_edition_gvw_selected == TRUE)) {
        licenseStatus = NV_GRID_UNLICENSED_GVW_SELECTED;
    }

    /* Show the message received */
    switch (licenseStatus) {
    case NV_GRID_UNLICENSED_VGPU:
          licenseState = "Your system does not have a valid GRID vGPU license.\n"
              "Enter license server details and apply.";
          break;
    case NV_GRID_UNLICENSED_TESLA:
          licenseState = "Your system is currently running on "
              "Tesla (unlicensed).";
          break;
    case NV_GRID_UNLICENSED_GVW_SELECTED:
          licenseState = "Your system is currently running on Tesla (unlicensed).\n"
             "Enter license server details and apply.";
          break;
    case NV_GRID_LICENSE_ACQUIRED_VGPU:
          licenseState = "Your system is licensed for GRID vGPU.";
          break;
    case NV_GRID_LICENSE_ACQUIRED_GVW:
          licenseState = "Your system is licensed for GRID Virtual "
              "Workstation Edition.";
          break;
    case NV_GRID_LICENSE_REQUESTING_VGPU:
          licenseState = "Acquiring license for GRID vGPU Edition.\n"
              "Your system does not have a valid GRID vGPU license.";
          break;
    case NV_GRID_LICENSE_REQUESTING_GVW:
          licenseState = "Acquiring license for GRID Virtual Workstation "
              "Edition.\n"
              "Your system does not have a valid GRID Virtual "
              "Workstation license.";
          break;
    case NV_GRID_LICENSE_FAILED_VGPU:
          licenseState = "Failed to acquire NVIDIA vGPU license.";
          break;
    case NV_GRID_LICENSE_FAILED_GVW:
          licenseState = "Failed to acquire NVIDIA GRID Virtual "
              "Workstation license.";
          break;
    case NV_GRID_LICENSE_EXPIRED_VGPU:
          licenseState = "Failed to renew license for GRID vGPU Edition.\n"
              "Your system does not have a valid GRID vGPU license.";
          break;
    case NV_GRID_LICENSE_EXPIRED_GVW:
          licenseState = "Failed to renew license for GRID Virtual "
              "Workstation Edition.\n"
              "Your system is currently running GRID Virtual "
              "Workstation (unlicensed).";
          break;
    case NV_GRID_LICENSE_RESTART_REQUIRED:
          licenseState = "Restart your system for Tesla Edition.\n"
              "Your system is currently running GRID Virtual "
              "Workstation Edition.";
          break;
    default:
          licenseState = "Your system does not have a valid GPU license.\n"
              "Enter license server details and apply.";
          break;
    }

    gtk_label_set_text(GTK_LABEL(ctk_manage_grid_license->label_license_state),
                       licenseState);
    return ret;
}



/*
 * apply_clicked() - Called when the user clicks on the "Apply" button.
 */

static void apply_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    gboolean ret;
    gint status = 0;

    /* Add information to gridd.conf file */
    ret = UpdateConfigFile(ctk_manage_grid_license);
    
    if (ret) {
        /* Send update request to nvidia-gridd */
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
                                         "update request to the NVIDIA GRID "
                                         "licensing daemon.\n"
                                         "Please make sure nvidia-gridd "
                                         "is running and retry applying the "
                                         "license settings.\n");
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        }
    }
}

static void license_edition_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    gboolean enabled;
    gchar *licenseState = "";

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    if (!enabled) {
        /* Ignore 'disable' events. */
        return;
    }
    user_data = g_object_get_data(G_OBJECT(widget), "button_id");

    if (GPOINTER_TO_INT(user_data) == GRID_LICENSED_FEATURE_TYPE_GVW) {
        gtk_widget_set_sensitive(ctk_manage_grid_license->box_server_info, TRUE);
        licenseState = "You selected GRID Virtual Workstation Edition.";
        ctk_manage_grid_license->feature_type = 
            GRID_LICENSED_FEATURE_TYPE_GVW;
        ctk_manage_grid_license->license_edition_gvw_selected = TRUE;
    }  else if (GPOINTER_TO_INT(user_data) == GRID_LICENSED_FEATURE_TYPE_TESLA) {
        gtk_widget_set_sensitive(ctk_manage_grid_license->box_server_info, FALSE);
        /* force unlicensed mode */
        ctk_manage_grid_license->feature_type = 
            GRID_LICENSED_FEATURE_TYPE_TESLA;
        licenseState = "You selected Tesla (Unlicensed) mode.";
        ctk_manage_grid_license->license_edition_gvw_selected = FALSE;
    }
    /* update status bar message */
    ctk_config_statusbar_message(ctk_manage_grid_license->ctk_config,
                                 "%s", licenseState);
}

static gboolean disallow_whitespace(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    GdkEventKey *key_event;

    if (event->type == GDK_KEY_PRESS) {
        key_event = (GdkEventKey *) event;

        if (isspace(key_event->keyval)) {
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean allow_digits(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    GdkEventKey *key_event;

    if (event->type == GDK_KEY_PRESS) {
        key_event = (GdkEventKey *) event;
        switch (key_event->keyval) {
        case GDK_Left:
        case GDK_KP_Left:
        case GDK_Down:
        case GDK_KP_Down:
        case GDK_Right:
        case GDK_KP_Right:
        case GDK_Up:
        case GDK_KP_Up:
        case GDK_Page_Down:
        case GDK_KP_Page_Down:
        case GDK_Page_Up:
        case GDK_KP_Page_Up:
        case GDK_BackSpace:
        case GDK_Delete:
            // Allow all control keys
            return FALSE;
        default:
            // Allow digits
            if (isdigit(key_event->keyval)) {
                return FALSE;
            }
        }
    }

    return TRUE;
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
                goto done;
            }

            pLines = ReadConfigFileStream(templateFile);

            WriteConfigFileStream(configFile, pLines);

            FreeConfigFileLines(pLines);
        } else {
            nv_error_msg("Config file '%s' had size zero.",
                         GRID_CONFIG_FILE);
            goto done;
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
    GtkWidget *button1 = NULL, *button2 = NULL;
    GSList *slist = NULL;
    gint ret;
    gboolean configFileAvailable;
    DBusError err;
    DBusConnection* conn;
    DbusData *dbusData;
    gboolean writable = FALSE;
    int64_t gridLicenseSupported;
    NvGriddConfigParams *griddConfig;

    /* make sure we have a handle */

    g_return_val_if_fail((target != NULL) &&
                         (target->h != NULL), NULL);

    /*
     * check if Manage GRID license page available.
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

    /* GRID M6 is licensable gpu so we want to allow users to choose
     * GRID virtual workstation and Unlicensed Tesla mode on baremetal setup.
     * When virtualization mode is NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_NONE
     * treat it same way like NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH.
     * So that it will show the GRID Virtual Workstation interface in case of
     * baremetal setup.
     */
    if (mode == NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_NONE) {
        mode = NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH;
    }

    /* DBUS calls are used for quering the current license status  */
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
    ret = dbusData->dbus.dbusRequestName(conn, NV_GRID_DBUS_CLIENT,
                                         DBUS_NAME_FLAG_REPLACE_EXISTING,
                                         &err);
    if (dbusData->dbus.dbusErrorIsSet(&err)) {
        dbusData->dbus.dbusErrorFree(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        return NULL;
    }

    /* Check available config file */
    configFileAvailable = checkConfigfile(&writable);
    
    /* Initialize config parameters */
    griddConfig = GetNvGriddConfigParams();
        
    if (griddConfig &&
        (strcmp(griddConfig->str[NV_GRIDD_ENABLE_UI], "FALSE") == 0)) {
        return NULL;
    }
    

    /* create the CtkManageGridLicense object */

    object = g_object_new(CTK_TYPE_MANAGE_GRID_LICENSE, NULL);
    
    ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(object);
    ctk_manage_grid_license->ctk_config = ctk_config;
    ctk_manage_grid_license->license_edition_state = mode;
    ctk_manage_grid_license->dbusData = dbusData;
    ctk_manage_grid_license->license_edition_gvw_selected = FALSE;
    ctk_manage_grid_license->feature_type = 0;

    /* set default value for feature type based on the user configured parameter or virtualization mode */
    if (griddConfig) {
        if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "0"))
        {
            ctk_manage_grid_license->feature_type = GRID_LICENSED_FEATURE_TYPE_TESLA;
        }
        else if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "1"))
        {
            ctk_manage_grid_license->feature_type = GRID_LICENSED_FEATURE_TYPE_VGPU;
        }
        else if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "2"))
        {
            ctk_manage_grid_license->feature_type = GRID_LICENSED_FEATURE_TYPE_GVW;
        }
        else
        {
            ctk_manage_grid_license->feature_type = GRID_LICENSED_FEATURE_TYPE_TESLA;
        }
    }

    /* Overwrite feature type in vGPU case */
    if (ctk_manage_grid_license->license_edition_state == NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_VGPU)
    {
        ctk_manage_grid_license->feature_type = GRID_LICENSED_FEATURE_TYPE_VGPU;
    }

    /* set container properties for the CtkManageGridLicense widget */

    gtk_box_set_spacing(GTK_BOX(ctk_manage_grid_license), 5);

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

    /* Show message if config file not available and user do not have
     * permissions to create new file.
     */
    if (!configFileAvailable && !griddConfig) {
        str = g_strdup_printf("'%s' file does not exist.\n You do not have "
                              "permissions to create this file.", GRID_CONFIG_FILE);
        label = gtk_label_new(str);
        g_free(str);
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 5);

        goto done;
    }

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
        
        button1 = gtk_radio_button_new_with_label(NULL,
                                                  "GRID Virtual Workstation");
        slist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button1));
        gtk_box_pack_start(GTK_BOX(vbox3), button1, FALSE, FALSE, 0);
        g_object_set_data(G_OBJECT(button1), "button_id",
                GINT_TO_POINTER(GRID_LICENSED_FEATURE_TYPE_GVW));
        g_signal_connect(G_OBJECT(button1), "toggled",
                         G_CALLBACK(license_edition_toggled),
                         (gpointer) ctk_manage_grid_license);

        button2 = gtk_radio_button_new_with_label(slist, "Tesla (Unlicensed)");
        gtk_box_pack_start(GTK_BOX(vbox3), button2, FALSE, FALSE, 0);
        g_object_set_data(G_OBJECT(button2), "button_id",
                GINT_TO_POINTER(GRID_LICENSED_FEATURE_TYPE_TESLA));

        g_signal_connect(G_OBJECT(button2), "toggled",
                         G_CALLBACK(license_edition_toggled),
                         (gpointer) ctk_manage_grid_license);

    }

    /* Only users with sufficient privileges can update server address
     * and port number.
     */
    if (!writable) {
        gtk_widget_set_sensitive(GTK_WIDGET(vbox2), FALSE);
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
    label = gtk_label_new("License Server:");
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

    label = gtk_label_new("Primary Server:");
    ctk_manage_grid_license->txt_server_address = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_address),
                       griddConfig->str[NV_GRIDD_SERVER_ADDRESS]);
    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __primary_server_address_help);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_server_address), "key-press-event",
                     G_CALLBACK(disallow_whitespace),
                     (gpointer) ctk_manage_grid_license);

    /* value */
    hbox = gtk_hbox_new(FALSE, 0);
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
    gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_port),
                       griddConfig->str[NV_GRIDD_SERVER_PORT]);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_server_port,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_server_port), "key-press-event",
                     G_CALLBACK(allow_digits),
                     (gpointer) ctk_manage_grid_license);
    
    /* Backup Server Address */
    label = gtk_label_new("Secondary Server:");
    ctk_manage_grid_license->txt_secondary_server_address = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_address),
                       griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS]);
    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox,
                           __secondary_server_help);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 5, 6,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_address), "key-press-event",
                     G_CALLBACK(disallow_whitespace),
                     (gpointer) ctk_manage_grid_license);

    /* value */
    hbox = gtk_hbox_new(FALSE, 0);
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
    gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_port),
                       griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT]);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_secondary_server_port,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 6, 7,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_port), "key-press-event",
                     G_CALLBACK(allow_digits),
                     (gpointer) ctk_manage_grid_license);
    ctk_manage_grid_license->box_server_info = vbox2;
    
    /* Apply button */
    ctk_manage_grid_license->btn_apply = gtk_button_new_with_label
        (" Apply ");
    gtk_widget_set_size_request(ctk_manage_grid_license->btn_apply, 100, -1);
    ctk_config_set_tooltip(ctk_config, ctk_manage_grid_license->btn_apply,
                           __apply_button_help);
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_manage_grid_license->btn_apply, FALSE, FALSE, 5);

    g_signal_connect(G_OBJECT(ctk_manage_grid_license->btn_apply), "clicked",
                     G_CALLBACK(apply_clicked),
                     (gpointer) ctk_manage_grid_license);
    
    /* Set license edition toggle button active */
    if (button2 && button1) {
        if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "0") == 0) {
            /* Tesla (Unlicensed) button is active */
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button2), TRUE);
        } else if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "2") == 0) {
            /* GRID Virtual Workstation button is active */
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button1), TRUE);
        }
    }

    FreeNvGriddConfigParams(griddConfig);

    /* Register a timer callback to update license status info */
    str = g_strdup_printf("Manage GRID License");

    ctk_config_add_timer(ctk_manage_grid_license->ctk_config,
                         DEFAULT_UPDATE_GRID_LICENSE_STATUS_INFO_TIME_INTERVAL,
                         str,
                         (GSourceFunc) update_manage_grid_license_state_info,
                         (gpointer) ctk_manage_grid_license);
    g_free(str);
    update_manage_grid_license_state_info(ctk_manage_grid_license);

done:
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

    ctk_help_heading(b, &i, "Manage GRID Licenses Help");
    ctk_help_para(b, &i, "%s", __manage_grid_licenses_help);

    if (ctk_manage_grid_license->license_edition_state ==
        NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH) { 
        ctk_help_heading(b, &i, "GRID Virtual Workstation");
        ctk_help_para(b, &i, "%s", __grid_virtual_workstation_help);

        ctk_help_heading(b, &i, "Tesla (Unlicensed)");
        ctk_help_para(b, &i, "%s", __tesla_unlicensed_help);
    }

    ctk_help_heading(b, &i, "License Server");
    ctk_help_para(b, &i, "%s", __license_server_help);

    ctk_help_heading(b, &i, "Primary Server");
    ctk_help_para(b, &i, "%s", __primary_server_address_help);

    ctk_help_heading(b, &i, "Port Number");
    ctk_help_para(b, &i, "%s", __primary_server_port_help);

    ctk_help_heading(b, &i, "Secondary Server");
    ctk_help_para(b, &i, "%s", __secondary_server_help);

    ctk_help_heading(b, &i, "Port Number");
    ctk_help_para(b, &i, "%s", __secondary_server_port_help);

    ctk_help_heading(b, &i, "Apply");
    ctk_help_para(b, &i, "%s", __apply_button_help);

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
}

void ctk_manage_grid_license_stop_timer(GtkWidget *widget)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(widget);

    /* Stop the manage_grid_license timer */

    ctk_config_stop_timer(ctk_manage_grid_license->ctk_config,
                          (GSourceFunc) update_manage_grid_license_state_info,
                          (gpointer) ctk_manage_grid_license);
}
