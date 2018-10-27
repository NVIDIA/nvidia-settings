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
#include <libintl.h>

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

#define _(STRING) gettext(STRING)
#define N_(STRING) STRING

#define DEFAULT_UPDATE_GRID_LICENSE_STATUS_INFO_TIME_INTERVAL 1000
#define GRID_CONFIG_FILE            "/etc/nvidia/gridd.conf"
#define GRID_CONFIG_FILE_TEMPLATE   "/etc/nvidia/gridd.conf.template"

static const char * __manage_grid_licenses_help =
N_("Use the Manage GRID License page to obtain licenses "
"for GRID vGPU or Quadro Virtual Datacenter Workstation on supported Tesla products.");
static const char * __grid_virtual_workstation_help =
N_("Select this option to enable Quadro Virtual Datacenter Workstation license.");
static const char * __grid_vapp_help =
N_("Select this option to disable the Quadro Virtual Datacenter Workstation license.");
static const char * __license_edition_help =
N_("This section indicates the status of GRID licensing for the system.");
static const char * __license_server_help =
N_("Shows the configured GRID license server details.");
static const char * __primary_server_address_help =
N_("Enter the address of your local GRID license server. "
"The address can be a fully-qualified domain name such as gridlicense.example.com, "
"or an IP address such as 10.31.20.45.");
static const char * __primary_server_port_help =
N_("This field can be left empty, and will default to 7070, "
"which is the default port number used by the NVIDIA GRID license server.");
static const char * __secondary_server_help =
N_("This field is optional. Enter the address of your backup GRID license server. "
"The address can be a fully-qualified domain name such as backuplicense.example.com, "
"or an IP address such as 10.31.20.46.");
static const char * __secondary_server_port_help =
N_("This field can be left empty, and will default to 7070, "
"which is the default port number used by the NVIDIA GRID license server.");
static const char * __apply_button_help =
N_("Clicking the Apply button updates license settings in the gridd.conf file and "
"sends update license request to the NVIDIA GRID licensing daemon.");
static const char * __cancel_button_help =
N_("Clicking the Cancel button sets the text in all textboxes from the gridd.conf file. "
"Any changes you have done will be lost.");

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
static void ctk_manage_grid_license_class_init(CtkManageGridLicenseClass *);
static void dbusClose(DbusData *dbusData);
static gboolean checkConfigfile(gboolean *writable);
static gboolean disallow_whitespace(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static gboolean allow_digits(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static gboolean enable_disable_ui_controls(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void update_gui_from_griddconfig(gpointer user_data);
static gboolean licenseStateQueryFailed = FALSE;
static void get_licensed_feature_code(gpointer user_data);
static gboolean is_restart_required(gpointer user_data);
static gboolean queryLicensedFeatureCode = TRUE;
int64_t licensedFeatureCode = NV_GRID_LICENSE_FEATURE_TYPE_VAPP;

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

    griddConfig->str[NV_GRIDD_SERVER_ADDRESS]        = nvstrdup("");
    griddConfig->str[NV_GRIDD_SERVER_PORT]           = nvstrdup("");
    griddConfig->str[NV_GRIDD_FEATURE_TYPE]          = nvstrdup("0");
    griddConfig->str[NV_GRIDD_ENABLE_UI]             = nvstrdup("FALSE");
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

    /* serverAddress */

    tmp = gtk_entry_get_text(GTK_ENTRY(
                              ctk_manage_grid_license->txt_server_address));
    if (strcmp(tmp, griddConfig->str[NV_GRIDD_SERVER_ADDRESS]) != 0) {
        nvfree(griddConfig->str[NV_GRIDD_SERVER_ADDRESS]);
        griddConfig->str[NV_GRIDD_SERVER_ADDRESS] = nvstrdup(tmp ? tmp : "");
    }

    /* serverPort */

    tmp = gtk_entry_get_text(GTK_ENTRY(
                              ctk_manage_grid_license->txt_server_port));
    if (strcmp(tmp, griddConfig->str[NV_GRIDD_SERVER_PORT]) != 0) {
        nvfree(griddConfig->str[NV_GRIDD_SERVER_PORT]);
        griddConfig->str[NV_GRIDD_SERVER_PORT] =
            nvstrdup((strcmp(tmp, "") != 0) ? tmp : "7070");
    }

    /* featureType */

    switch (ctk_manage_grid_license->feature_type) {
        case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
            tmp = "0";
            break;
        case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
            tmp = "1";
            break;
        case NV_GRID_LICENSE_FEATURE_TYPE_QDWS:
            tmp = "2";
            break;
        default:
            tmp = "0";
    }
    if (strcmp(tmp, griddConfig->str[NV_GRIDD_FEATURE_TYPE]) != 0) {
        nvfree(griddConfig->str[NV_GRIDD_FEATURE_TYPE]);
        griddConfig->str[NV_GRIDD_FEATURE_TYPE] = nvstrdup(tmp);
    }

    /* note: nothing in the UI will alter enableUI */

    /* backupServerAddress */

    tmp = gtk_entry_get_text(GTK_ENTRY(
                              ctk_manage_grid_license->txt_secondary_server_address));
    if (strcmp(tmp, griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS]) != 0) {
        nvfree(griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS]);
        griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS] = nvstrdup(tmp ? tmp : "");
    }

    /* backupServerPort */

    tmp = gtk_entry_get_text(GTK_ENTRY(
                              ctk_manage_grid_license->txt_secondary_server_port));
    if (strcmp(tmp, griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT]) != 0) {
        nvfree(griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT]);
        griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT] = nvstrdup(tmp ? tmp : "");
    }
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
 * Update the nvidia-gridd config file with the current GUI state.
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
    gchar *licenseStatusMessage = "";
    gboolean ret = TRUE;

    int licenseStatus = NV_GRID_UNLICENSED_VGPU;
    int licenseState = NV_GRID_UNLICENSED;
    int griddFeatureType = ctk_manage_grid_license->feature_type;

    /* Send license state and feature type request */
    if ((!(send_message_to_gridd(ctk_manage_grid_license, LICENSE_STATE_REQUEST,
                                 &licenseState))) ||
        (!(send_message_to_gridd(ctk_manage_grid_license, LICENSE_FEATURE_TYPE_REQUEST,
                                 &griddFeatureType)))) {
        licenseStatusMessage = _("Unable to query license state information "
                               "from the NVIDIA GRID "
                               "licensing daemon.\n"
                               "Please make sure nvidia-gridd and "
                               "dbus-daemon are running.\n");
        gtk_label_set_text(GTK_LABEL(ctk_manage_grid_license->label_license_state),
                           licenseStatusMessage);
        /* Disable text controls on UI. */
        gtk_widget_set_sensitive(ctk_manage_grid_license->txt_server_address, FALSE);
        gtk_widget_set_sensitive(ctk_manage_grid_license->txt_server_port, FALSE);
        gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_address, FALSE);
        gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_port, FALSE);
        /* Disable Apply/Cancel button. */
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
        gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);
        /* Disable toggle buttons */
        if (ctk_manage_grid_license->radio_btn_qdws && ctk_manage_grid_license->radio_btn_vapp) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_qdws, FALSE);
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_vapp, FALSE);
        }

        licenseStateQueryFailed = TRUE;
        return ret;
    }

    if (licenseStateQueryFailed == TRUE) {
        /*Failure to communicate with nvidia-gridd in previous attempt.
         Refresh UI with parameters from the griddConfig.*/
        update_gui_from_griddconfig(ctk_manage_grid_license);
        licenseStateQueryFailed = FALSE;
    }

    /* Set the license feature type fetched from nvidia-gridd.*/
    ctk_manage_grid_license->gridd_feature_type = griddFeatureType;

    if (licenseState == NV_GRID_UNLICENSED) {
        queryLicensedFeatureCode = TRUE;
        switch (ctk_manage_grid_license->feature_type) {
            case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                licenseStatus = NV_GRID_UNLICENSED_VAPP;
                if (is_restart_required(ctk_manage_grid_license)) {
                    licenseStatus = NV_GRID_LICENSE_RESTART_REQUIRED_VAPP;
                }
                break;
            case NV_GRID_LICENSE_FEATURE_TYPE_QDWS:
                licenseStatus = NV_GRID_UNLICENSED_QDWS_SELECTED;
                if (licensedFeatureCode == NV_GRID_LICENSE_FEATURE_TYPE_QDWS) {
                    licenseStatus = NV_GRID_UNLICENSED_REQUEST_DETAILS;
                }
                break;
            case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                licenseStatus = NV_GRID_UNLICENSED_VGPU;
                break;
            default:
                break;
        }
    }
    else {
        if ((licenseState == NV_GRID_LICENSED) ||
            (licenseState == NV_GRID_LICENSE_RENEWING) ||
            (licenseState == NV_GRID_LICENSE_RENEW_FAILED)) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                    licenseStatus = NV_GRID_LICENSE_ACQUIRED_QDWS;  // Default status in licensed state on non-vGPU case
                    if (ctk_manage_grid_license->feature_type == ctk_manage_grid_license->gridd_feature_type) {
                        licenseStatus = NV_GRID_LICENSE_RESTART_REQUIRED;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_QDWS:
                    licenseStatus = NV_GRID_LICENSE_ACQUIRED_QDWS;
                    if (ctk_manage_grid_license->feature_type != ctk_manage_grid_license->gridd_feature_type) {
                        /* On licensed non-vGPU setup, mismatch in feature type fetched from nvidia-gridd service and
                            the feature type read from gridd.conf/UI controls indicates user has edited the feature type
                            directly in the gridd.conf file and hence those changes will not be reflected until the system
                            is rebooted. Set appropriate license status to indicate the same. */
                        licenseStatus = NV_GRID_LICENSE_RESTART_REQUIRED;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    licenseStatus = NV_GRID_LICENSE_ACQUIRED_VGPU;
                    break;
                default:
                    break;
            }
            if (queryLicensedFeatureCode == TRUE) {
                get_licensed_feature_code(ctk_manage_grid_license);
                queryLicensedFeatureCode = FALSE;
            }
        }
        else if (licenseState == NV_GRID_LICENSE_REQUESTING) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                    licenseStatus = NV_GRID_LICENSE_REQUESTING_QDWS;    // Default status in license requesting state on non-vGPU case
                    if (ctk_manage_grid_license->feature_type == ctk_manage_grid_license->gridd_feature_type) {
                        licenseStatus = NV_GRID_UNLICENSED_VAPP;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_QDWS:
                    licenseStatus = NV_GRID_LICENSE_REQUESTING_QDWS;
                    if (ctk_manage_grid_license->feature_type != ctk_manage_grid_license->gridd_feature_type) {
                        /* On non-vGPU setup with license requesting state, mismatch in feature type fetched from nvidia-gridd service and
                            the feature type read from gridd.conf/UI controls indicates user has edited the feature type
                            directly in the gridd.conf file. So set appropriate license status message. */
                        licenseStatus = NV_GRID_UNLICENSED_QDWS_SELECTED;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    licenseStatus = NV_GRID_LICENSE_REQUESTING_VGPU;
                    break;
                default:
                    break;
            }
        }
        else if (licenseState == NV_GRID_LICENSE_FAILED) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                    licenseStatus = NV_GRID_LICENSE_FAILED_QDWS;    // Default status in license failed state on non-vGPU case
                    if (ctk_manage_grid_license->feature_type == ctk_manage_grid_license->gridd_feature_type) {
                        licenseStatus = NV_GRID_UNLICENSED_VAPP;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_QDWS:
                    licenseStatus = NV_GRID_LICENSE_FAILED_QDWS;
                    if (ctk_manage_grid_license->feature_type != ctk_manage_grid_license->gridd_feature_type) {
                        /* On non-vGPU setup with license failed state, mismatch in feature type fetched from nvidia-gridd service and
                            the feature type read from gridd.conf/UI controls indicates user has edited the feature type
                            directly in the gridd.conf file. So set appropriate license status message. */
                        licenseStatus = NV_GRID_UNLICENSED_QDWS_SELECTED;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    licenseStatus = NV_GRID_LICENSE_FAILED_VGPU;
                    break;
                default:
                    break;
            }
        }
        else if (licenseState == NV_GRID_LICENSE_EXPIRED) {
            switch (ctk_manage_grid_license->feature_type) {
                case NV_GRID_LICENSE_FEATURE_TYPE_VAPP:
                    licenseStatus = NV_GRID_LICENSE_EXPIRED_QDWS;    // Default status in license expired state on non-vGPU case
                    if (ctk_manage_grid_license->feature_type == ctk_manage_grid_license->gridd_feature_type) {
                        licenseStatus = NV_GRID_LICENSE_RESTART_REQUIRED;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_QDWS:
                    licenseStatus = NV_GRID_LICENSE_EXPIRED_QDWS;
                    if (ctk_manage_grid_license->feature_type != ctk_manage_grid_license->gridd_feature_type) {
                        /* On non-vGPU setup with expired license, mismatch in feature type fetched from nvidia-gridd service and
                            the feature type read from gridd.conf/UI controls indicates user has edited the feature type
                            directly in the gridd.conf file and hence those changes will not be reflected until the system
                            is rebooted. Set appropriate license status to indicate the same. */
                        licenseStatus = NV_GRID_LICENSE_RESTART_REQUIRED;
                    }
                    break;
                case NV_GRID_LICENSE_FEATURE_TYPE_VGPU:
                    licenseStatus = NV_GRID_LICENSE_EXPIRED_VGPU;
                    break;
                default:
                    break;
            }
        }
    }

    switch (licenseStatus) {
    case NV_GRID_UNLICENSED_VGPU:
          licenseStatusMessage = _("Your system does not have a valid GRID vGPU license.\n"
              "Enter license server details and apply.");
          break;
    case NV_GRID_UNLICENSED_VAPP:
          licenseStatusMessage = _("Your system is currently configured for "
              "GRID Virtual Apps.");
          break;
    case NV_GRID_UNLICENSED_QDWS_SELECTED:
          licenseStatusMessage = _("Your system is currently configured for GRID Virtual Apps.\n"
             "Enter license server details and apply.");
          break;
    case NV_GRID_LICENSE_ACQUIRED_VGPU:
          licenseStatusMessage = _("Your system is licensed for GRID vGPU.");
          break;
    case NV_GRID_LICENSE_ACQUIRED_QDWS:
          licenseStatusMessage = _("Your system is licensed for Quadro Virtual Datacenter "
              "Workstation.");
          break;
    case NV_GRID_LICENSE_REQUESTING_VGPU:
          licenseStatusMessage = _("Acquiring license for GRID vGPU.\n"
              "Your system does not have a valid GRID vGPU license.");
          break;
    case NV_GRID_LICENSE_REQUESTING_QDWS:
          licenseStatusMessage = _("Acquiring license for Quadro Virtual Datacenter "
              "Workstation.\n"
              " Your system is currently configured for GRID Virtual Apps.");
          break;
    case NV_GRID_LICENSE_FAILED_VGPU:
          licenseStatusMessage = _("Failed to acquire GRID vGPU license.");
          break;
    case NV_GRID_LICENSE_FAILED_QDWS:
          licenseStatusMessage = _("Failed to acquire Quadro Virtual Datacenter "
              "Worstation license.\n"
              " Your system is currently configured for GRID Virtual Apps.");
          break;
    case NV_GRID_LICENSE_EXPIRED_VGPU:
          licenseStatusMessage = _("GRID vGPU license has expired.\n"
              "Your system does not have a valid GRID vGPU license.");
          break;
    case NV_GRID_LICENSE_EXPIRED_QDWS:
          licenseStatusMessage = _("License for Quadro Virtual Datacenter Workstation "
              "has expired.\n"
              "Your system does not have a valid Quadro Virtual Datacenter "
              "Workstation license.");
          break;
    case NV_GRID_LICENSE_RESTART_REQUIRED:
          licenseStatusMessage = _("Restart your system for GRID Virtual Apps.\n"
              "Your system is currently licensed for Quadro Virtual Datacenter "
              "Workstation.");
          break;
    case NV_GRID_LICENSE_RESTART_REQUIRED_VAPP:
          licenseStatusMessage = _("Restart your system for GRID Virtual Apps.");
          break;
    case NV_GRID_UNLICENSED_REQUEST_DETAILS:
    default:
          licenseStatusMessage = _("Your system does not have a valid GRID license.\n"
              "Enter license server details and apply.");
          break;
    }

    gtk_label_set_text(GTK_LABEL(ctk_manage_grid_license->label_license_state),
                       licenseStatusMessage);
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
    if ((licensedFeatureCode) &&
        (licensedFeatureCode != ctk_manage_grid_license->feature_type)) {
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
                                             _("Unable to send license information "
                                             "update request to the NVIDIA GRID "
                                             "licensing daemon.\n"
                                             "Please make sure nvidia-gridd and "
                                             "dbus-daemon are running and retry applying the "
                                             "license settings.\n"));
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
                                         _("Unable to update GRID license configuration "
                                         "file (%s): %s"), GRID_CONFIG_FILE, strerror(err));
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
                                     _("You do not have enough "
                                     "permissions to edit '%s' file."), GRID_CONFIG_FILE);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
    } else {
        GtkWidget *dlg, *parent;

        parent = ctk_get_parent_window(GTK_WIDGET(ctk_manage_grid_license));
        dlg = gtk_message_dialog_new(GTK_WINDOW(parent),
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     _("'%s' file does not exist.\n You do not have "
                                     "permissions to create this file."), GRID_CONFIG_FILE);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
    }

    /* Disable Apply/Cancel button. */
    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);
}

/*
 * get_licensed_feature_code() - Get the feature code of the feature that is licensed on this system.
 */
static void get_licensed_feature_code(gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    nvmlGridLicensableFeatures_t *gridLicensableFeatures;
    gint ret, i;

    ret = NvCtrlNvmlGetGridLicenseAttributes(ctk_manage_grid_license->target,
                                             NV_CTRL_ATTR_NVML_GPU_GRID_LICENSABLE_FEATURES,
                                             &gridLicensableFeatures);
    if (ret != NvCtrlSuccess) {
        return;
    }

    for (i = 0; i < gridLicensableFeatures->licensableFeaturesCount; i++)
    {
        if (gridLicensableFeatures->gridLicensableFeatures[i].featureState != 0)
        {
            licensedFeatureCode = gridLicensableFeatures->gridLicensableFeatures[i].featureCode;
            break;
        }
    }
    nvfree(gridLicensableFeatures);
}

/*
 * cancel_clicked() - Called when the user clicks on the "Cancel" button.
 */

static void cancel_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);

    /* Discard user input and revert back to configuration from the nvidia-gridd config file */
    update_gui_from_griddconfig(ctk_manage_grid_license);

}

static void update_gui_from_griddconfig(gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    NvGriddConfigParams *griddConfig = NULL;
    const char *textBoxServerStr;

    griddConfig = GetNvGriddConfigParams();
    if (!griddConfig) {
        nv_error_msg("Null griddConfig. \n");
        /* If griddConfig is Null, clear out all the textboxes. */
        gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_address), "");
        gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_port), "");
        gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_address), "");
        gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_port), "");
    } else {
        /* Set the text in all the textboxes from the griddconfig. */
        gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_address),
                           griddConfig->str[NV_GRIDD_SERVER_ADDRESS]);
        gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_port),
                           griddConfig->str[NV_GRIDD_SERVER_PORT]);
        gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_address),
                           griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS]);
        gtk_entry_set_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_port),
                           griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT]);
        /* set default value for feature type based on the user configured parameter or virtualization mode */
        /* Check Feature type "2" for Quadro Virtual Datacenter Workstation. */
        if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "2") == 0) {
            ctk_manage_grid_license->feature_type = NV_GRID_LICENSE_FEATURE_TYPE_QDWS;
        }
        else {
            ctk_manage_grid_license->feature_type = NV_GRID_LICENSE_FEATURE_TYPE_VAPP;
        }

        /* Override feature type when on vGPU */
        if (ctk_manage_grid_license->license_edition_state == NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_VGPU)
        {
            ctk_manage_grid_license->feature_type = NV_GRID_LICENSE_FEATURE_TYPE_VGPU;
        }

        /* Set license edition toggle button active */
        if (ctk_manage_grid_license->radio_btn_qdws && ctk_manage_grid_license->radio_btn_vapp) {
            if (ctk_manage_grid_license->feature_type == NV_GRID_LICENSE_FEATURE_TYPE_QDWS) {
                /* Set 'Quadro Virtual Datacenter Workstation' toggle button active */
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_manage_grid_license->radio_btn_qdws), TRUE);
            }
            else {
                /* Set 'GRID Virtual Apps' toggle button active */
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_manage_grid_license->radio_btn_vapp), TRUE);
            }
        }

        /* Enable Primary server address/port textboxes. */
        gtk_widget_set_sensitive(ctk_manage_grid_license->txt_server_address, TRUE);
        gtk_widget_set_sensitive(ctk_manage_grid_license->txt_server_port, TRUE);
        /* Enable toggle buttons. */
        if (ctk_manage_grid_license->radio_btn_qdws && ctk_manage_grid_license->radio_btn_vapp) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_qdws, TRUE);
            gtk_widget_set_sensitive(ctk_manage_grid_license->radio_btn_vapp, TRUE);
        }

        textBoxServerStr = gtk_entry_get_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_address));
        /* Enable/Disable Secondary server address/port textboxes if Primary server address textbox string is empty. */
        if (strcmp(textBoxServerStr, "") == 0) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_address, FALSE);
            gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_port, FALSE);
        } else {
            gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_address, TRUE);
            gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_port, TRUE);
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
    const char *textBoxServerStr, *textBoxServerPortStr, *textBoxSecondaryServerStr, *textBoxSecondaryServerPortStr;

    griddConfig = GetNvGriddConfigParams();
    if (!griddConfig) {
        return;
    }

    textBoxServerStr = gtk_entry_get_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_address));
    textBoxServerPortStr = gtk_entry_get_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_port));
    textBoxSecondaryServerStr = gtk_entry_get_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_address));
    textBoxSecondaryServerPortStr = gtk_entry_get_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_port));

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    if (!enabled) {
        /* Ignore 'disable' events. */
        return;
    }
    user_data = g_object_get_data(G_OBJECT(widget), "button_id");

    if (GPOINTER_TO_INT(user_data) == NV_GRID_LICENSE_FEATURE_TYPE_QDWS) {
        gtk_widget_set_sensitive(ctk_manage_grid_license->box_server_info, TRUE);
        statusBarMsg = _("You selected Quadro Virtual Datacenter Workstation Edition.");
        ctk_manage_grid_license->feature_type =
            NV_GRID_LICENSE_FEATURE_TYPE_QDWS;
        /* Enable Apply/Cancel button if the feature type selection has changed*/
        if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "2") != 0) {
            /* Disable Apply/Cancel button if Primary server address textbox string is empty. */
            if (strcmp(textBoxServerStr, "") == 0) {
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);
            } else {
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, TRUE);
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, TRUE);
            }
        } else {
            /* feature type selection has not changed. But Enable Apply/Cancel button when user make changes
                in any of the textbox entries to retain those changes. */
            if ((strcmp(griddConfig->str[NV_GRIDD_SERVER_ADDRESS], textBoxServerStr) != 0) ||
                ((strcmp(griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS], textBoxSecondaryServerStr) != 0) ||
                (strcmp(griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT], textBoxSecondaryServerPortStr) != 0) ||
                (strcmp(griddConfig->str[NV_GRIDD_SERVER_PORT], textBoxServerPortStr) != 0))) {
                    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, TRUE);
                    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, TRUE);
            } else {
                    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
                    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);
            }
        }
    }  else if (GPOINTER_TO_INT(user_data) == NV_GRID_LICENSE_FEATURE_TYPE_VAPP) {
        gtk_widget_set_sensitive(ctk_manage_grid_license->box_server_info, FALSE);
        ctk_manage_grid_license->feature_type = 
            NV_GRID_LICENSE_FEATURE_TYPE_VAPP;
        statusBarMsg = _("You selected GRID Virtual Apps Edition.");
        /* Enable Apply/Cancel button if the feature type selection has changed*/
        if (strcmp(griddConfig->str[NV_GRIDD_FEATURE_TYPE], "0") != 0) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, TRUE);
            gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, TRUE);
        } else {
            /* feature type selection has not changed. But Enable Apply/Cancel button when user make changes
                in any of the textbox entries to retain those changes. */
            if ((strcmp(griddConfig->str[NV_GRIDD_SERVER_ADDRESS], textBoxServerStr) != 0) ||
                ((strcmp(griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS], textBoxSecondaryServerStr) != 0) ||
                (strcmp(griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT], textBoxSecondaryServerPortStr) != 0) ||
                (strcmp(griddConfig->str[NV_GRIDD_SERVER_PORT], textBoxServerPortStr) != 0))) {
                    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, TRUE);
                    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, TRUE);
            } else {
                    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
                    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);
            }
        }
    }
    /* update status bar message */
    ctk_config_statusbar_message(ctk_manage_grid_license->ctk_config,
                                 "%s", statusBarMsg);
    update_manage_grid_license_state_info(ctk_manage_grid_license);
    FreeNvGriddConfigParams(griddConfig);
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

static gboolean enable_disable_ui_controls(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    CtkManageGridLicense *ctk_manage_grid_license = CTK_MANAGE_GRID_LICENSE(user_data);
    NvGriddConfigParams *griddConfig;
    const char *textBoxServerStr, *textBoxServerPortStr, *textBoxSecondaryServerStr, *textBoxSecondaryServerPortStr;

    griddConfig = GetNvGriddConfigParams();
    if (!griddConfig)
        return TRUE;

    if (event->type == GDK_KEY_RELEASE) {

        // Read license strings from textboxes.
        textBoxServerStr = gtk_entry_get_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_address));
        textBoxServerPortStr = gtk_entry_get_text(GTK_ENTRY(ctk_manage_grid_license->txt_server_port));
        textBoxSecondaryServerStr = gtk_entry_get_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_address));
        textBoxSecondaryServerPortStr = gtk_entry_get_text(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_port));

        /* Enable apply/cancel button if either:
            Primary server address/port textbox string doesn't match with the Primary server string from the nvidia-gridd config file or
            Secondary server address/port textbox string doesn't match with the Secondary server string from the nvidia-gridd config file. */
        if ((strcmp(griddConfig->str[NV_GRIDD_SERVER_ADDRESS], textBoxServerStr) != 0) ||
            ((strcmp(griddConfig->str[NV_GRIDD_BACKUP_SERVER_ADDRESS], textBoxSecondaryServerStr) != 0) ||
            (strcmp(griddConfig->str[NV_GRIDD_BACKUP_SERVER_PORT], textBoxSecondaryServerPortStr) != 0) ||
            (strcmp(griddConfig->str[NV_GRIDD_SERVER_PORT], textBoxServerPortStr) != 0))) {
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, TRUE);
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, TRUE);
        } else {
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
                gtk_widget_set_sensitive(ctk_manage_grid_license->btn_cancel, FALSE);
        }

        /* Disable Secondary server address/port textboxes if Primary server address text box string is empty
            to notify user that Primary server address is mandatory. */
        if (strcmp(textBoxServerStr, "") == 0) {
            gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_address, FALSE);
            gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_port, FALSE);
        } else {
            gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_address, TRUE);
            gtk_widget_set_sensitive(ctk_manage_grid_license->txt_secondary_server_port, TRUE);
        }
    }

    FreeNvGriddConfigParams(griddConfig);

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
        case GDK_Tab:
        case GDK_KP_Tab:
        case GDK_ISO_Left_Tab:
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
     * Quadro Virtual Datacenter Workstation and GRID Virtual Apps on baremetal setup.
     * When virtualization mode is NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_NONE
     * treat it same way like NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH.
     * So that it will show the Quadro Virtual Datacenter Workstation interface in case of
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

    if (mode == NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH) {
        label = gtk_label_new(_("License Edition:"));
        hbox = gtk_hbox_new(FALSE, 0);
        eventbox = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(eventbox), label);
        ctk_config_set_tooltip(ctk_config, eventbox, _(__license_edition_help));

        gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 5);
        vbox3 = gtk_vbox_new(FALSE, 5);
        gtk_container_add(GTK_CONTAINER(vbox1), vbox3);
        gtk_container_set_border_width(GTK_CONTAINER(vbox3), 5);

        ctk_manage_grid_license->radio_btn_qdws = gtk_radio_button_new_with_label(NULL,
                                                  _("Quadro Virtual Datacenter Workstation"));
        slist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(ctk_manage_grid_license->radio_btn_qdws));
        gtk_box_pack_start(GTK_BOX(vbox3), ctk_manage_grid_license->radio_btn_qdws, FALSE, FALSE, 0);
        g_object_set_data(G_OBJECT(ctk_manage_grid_license->radio_btn_qdws), "button_id",
                GINT_TO_POINTER(NV_GRID_LICENSE_FEATURE_TYPE_QDWS));
        g_signal_connect(G_OBJECT(ctk_manage_grid_license->radio_btn_qdws), "toggled",
                         G_CALLBACK(license_edition_toggled),
                         (gpointer) ctk_manage_grid_license);

        ctk_manage_grid_license->radio_btn_vapp = gtk_radio_button_new_with_label(slist, _("GRID Virtual Apps"));
        gtk_box_pack_start(GTK_BOX(vbox3), ctk_manage_grid_license->radio_btn_vapp, FALSE, FALSE, 0);
        g_object_set_data(G_OBJECT(ctk_manage_grid_license->radio_btn_vapp), "button_id",
                GINT_TO_POINTER(NV_GRID_LICENSE_FEATURE_TYPE_VAPP));

        g_signal_connect(G_OBJECT(ctk_manage_grid_license->radio_btn_vapp), "toggled",
                         G_CALLBACK(license_edition_toggled),
                         (gpointer) ctk_manage_grid_license);

    }

    /* Show current license status message */
    ctk_manage_grid_license->label_license_state = gtk_label_new(_("Unknown"));
    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox),
                      ctk_manage_grid_license->label_license_state);
    ctk_config_set_tooltip(ctk_config, eventbox, _(__license_edition_help));
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 5);
    
    frame = gtk_frame_new("");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox2);
    
    /* License Server */
    label = gtk_label_new(_("License Server:"));
    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, _(__license_server_help));
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

    label = gtk_label_new(_("Primary Server:"));
    ctk_manage_grid_license->txt_server_address = gtk_entry_new();
    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, _(__primary_server_address_help));
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_server_address), "key-press-event",
                     G_CALLBACK(disallow_whitespace),
                     (gpointer) ctk_manage_grid_license);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_server_address), "key-release-event",
                     G_CALLBACK(enable_disable_ui_controls),
                     (gpointer) ctk_manage_grid_license);

    /* value */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_server_address,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* Port Number */
    label = gtk_label_new(_("Port Number:"));

    hbox = gtk_hbox_new(FALSE, 5);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, _(__primary_server_port_help));
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

    /* value */
    ctk_manage_grid_license->txt_server_port = gtk_entry_new();
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_server_port,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_server_port), "key-press-event",
                     G_CALLBACK(allow_digits),
                     (gpointer) ctk_manage_grid_license);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_server_port), "key-release-event",
                     G_CALLBACK(enable_disable_ui_controls),
                     (gpointer) ctk_manage_grid_license);

    /* Backup Server Address */
    label = gtk_label_new(_("Secondary Server:"));
    ctk_manage_grid_license->txt_secondary_server_address = gtk_entry_new();

    hbox = gtk_hbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox,
                           _(__secondary_server_help));
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 5, 6,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_address), "key-press-event",
                     G_CALLBACK(disallow_whitespace),
                     (gpointer) ctk_manage_grid_license);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_address), "key-release-event",
                     G_CALLBACK(enable_disable_ui_controls),
                     (gpointer) ctk_manage_grid_license);

    /* value */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_secondary_server_address,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 5, 6,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* Port Number */
    label = gtk_label_new(_("Port Number:"));

    hbox = gtk_hbox_new(FALSE, 5);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, _(__secondary_server_port_help));
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 6, 7,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

    /* value */
    ctk_manage_grid_license->txt_secondary_server_port = gtk_entry_new();

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_manage_grid_license->txt_secondary_server_port,
                       FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 6, 7,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_port), "key-press-event",
                     G_CALLBACK(allow_digits),
                     (gpointer) ctk_manage_grid_license);
    g_signal_connect(GTK_ENTRY(ctk_manage_grid_license->txt_secondary_server_port), "key-release-event",
                     G_CALLBACK(enable_disable_ui_controls),
                     (gpointer) ctk_manage_grid_license);
    ctk_manage_grid_license->box_server_info = vbox2;

    /* Apply button */
    ctk_manage_grid_license->btn_apply = gtk_button_new_with_label
        (" Apply ");
    gtk_widget_set_sensitive(ctk_manage_grid_license->btn_apply, FALSE);
    gtk_widget_set_size_request(ctk_manage_grid_license->btn_apply, 100, -1);
    ctk_config_set_tooltip(ctk_config, ctk_manage_grid_license->btn_apply,
                           _(__apply_button_help));
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_manage_grid_license->btn_apply, FALSE, FALSE, 5);

    g_signal_connect(G_OBJECT(ctk_manage_grid_license->btn_apply), "clicked",
                     G_CALLBACK(apply_clicked),
                     (gpointer) ctk_manage_grid_license);

    /* Cancel button */
    ctk_manage_grid_license->btn_cancel = gtk_button_new_with_label
        (_(" Cancel "));
    gtk_widget_set_size_request(ctk_manage_grid_license->btn_cancel, 100, -1);
    ctk_config_set_tooltip(ctk_config, ctk_manage_grid_license->btn_cancel,
                           _(__cancel_button_help));
    gtk_box_pack_end(GTK_BOX(hbox), ctk_manage_grid_license->btn_cancel, FALSE, FALSE, 5);

    g_signal_connect(G_OBJECT(ctk_manage_grid_license->btn_cancel), "clicked",
                     G_CALLBACK(cancel_clicked),
                     (gpointer) ctk_manage_grid_license);

    /* Update GUI with information from the nvidia-gridd config file */
    update_gui_from_griddconfig(ctk_manage_grid_license);

    get_licensed_feature_code(ctk_manage_grid_license);

    /* Set the license feature type fetched from nvidia-gridd */
    ctk_manage_grid_license->gridd_feature_type = ctk_manage_grid_license->feature_type;

    /* Register a timer callback to update license status info */
    str = g_strdup_printf(_("Manage GRID License"));

    ctk_config_add_timer(ctk_manage_grid_license->ctk_config,
                         DEFAULT_UPDATE_GRID_LICENSE_STATUS_INFO_TIME_INTERVAL,
                         str,
                         (GSourceFunc) update_manage_grid_license_state_info,
                         (gpointer) ctk_manage_grid_license);
    update_manage_grid_license_state_info(ctk_manage_grid_license);

    /* Enable Apply/Cancel button if there is mismatch between feature type fetched from nvidia-gridd
        and feature type updated from UI/gridd.conf. */
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

    ctk_help_heading(b, &i, _("Manage GRID Licenses Help"));
    ctk_help_para(b, &i, "%s", _(__manage_grid_licenses_help));

    if (ctk_manage_grid_license->license_edition_state ==
        NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH) {
        ctk_help_heading(b, &i, _("Quadro Virtual Datacenter Workstation"));
        ctk_help_para(b, &i, "%s", _(__grid_virtual_workstation_help));

        ctk_help_heading(b, &i, _("GRID Virtual Apps"));
        ctk_help_para(b, &i, "%s", _(__grid_vapp_help));
    }

    ctk_help_heading(b, &i, _("License Server"));
    ctk_help_para(b, &i, "%s", _(__license_server_help));

    ctk_help_heading(b, &i, _("Primary Server"));
    ctk_help_para(b, &i, "%s", _(__primary_server_address_help));

    ctk_help_heading(b, &i, _("Port Number"));
    ctk_help_para(b, &i, "%s", _(__primary_server_port_help));

    ctk_help_heading(b, &i, _("Secondary Server"));
    ctk_help_para(b, &i, "%s", _(__secondary_server_help));

    ctk_help_heading(b, &i, _("Port Number"));
    ctk_help_para(b, &i, "%s", _(__secondary_server_port_help));

    ctk_help_heading(b, &i, _("Apply"));
    ctk_help_para(b, &i, "%s", _(__apply_button_help));

    ctk_help_heading(b, &i, _("Cancel"));
    ctk_help_para(b, &i, "%s", _(__cancel_button_help));

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
