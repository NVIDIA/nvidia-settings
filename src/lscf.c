#include <stdio.h>
#if defined(NV_SUNOS)

/* Interface to the Solaris Service Management Facility.
 * This facility is responsible for running programs and services
 * and store their configuration informations (named properties)
 * The configuration informations for the X server are managed by 
 * this facility. The functions in this source file use the library 
 * libscf (Service Configuration Facility) to access and modify 
 * the properties for the X server, more specifically the default depth. 
 * On Solaris, changing the default depth in the xorg.conf file is not
 * enough. The session manager overrides the xorg.conf default depth:
 * it passes the option -defdepth  to the X server with the value 
 * retrieved from the Service Management Facility.
 *
 * For more information refer to the manpages of fmf(5), libsfc(3LIB),
 * and to the source code of svccfg(1M) available on cvs.opensolaris.org.
 */


#include <libscf.h>

static int lscf_init_handle(scf_handle_t **scf_handle, 
                            scf_scope_t **scf_scope);
static int lscf_select(scf_handle_t *scf_handle, 
                       scf_scope_t *scf_scope,
                       const char *selection,
                       scf_service_t **current_svc);
static int lscf_setprop_int(scf_handle_t *scf_handle, 
                            scf_scope_t *scf_scope,
                            scf_service_t *current_svc,
                            const char *group, 
                            const char *name, int value);

/* UPDATE THE DEFAULT DEPTH PROPERTY IN SMF WITH THE LIBSCF FUNCTIONS */
int update_scf_depth(int depth) 
{  
    static scf_handle_t *scf_handle = NULL;
    static scf_scope_t *scf_scope  = NULL;
    scf_service_t       *curren_svc = NULL;
    int status = 1;
    
    // Initialization of the handles
    lscf_init_handle(&scf_handle, &scf_scope);
    if (scf_handle == NULL) {
        status =0;
        goto done;
    }
    
    // Set the current selection
    if(!lscf_select(scf_handle, scf_scope, "application/x11/x11-server",
                    &curren_svc)) {
        status =0;
        goto done;
    }
    
    // Set the depth property of the current selection
    if(!lscf_setprop_int(scf_handle, scf_scope, curren_svc,
                         "options", "default_depth", depth)) {
        status =0;
        goto done;
    }

done:
    if(curren_svc)  scf_service_destroy(curren_svc);
    if(scf_scope)       scf_scope_destroy(scf_scope);    
    if(scf_handle) {
                    scf_handle_unbind(scf_handle);
                    scf_handle_destroy(scf_handle);
    }
    if (!status) {
        fmterr("Unable to set X server default depth through "
               "Solaris Service Management Facility");
    }
    return status;
}

/* INITIALIZATION OF THE HANDLES */
static int lscf_init_handle(scf_handle_t **scf_handle, 
                            scf_scope_t **scf_scope) 
{
    scf_handle_t *handle = NULL;
    scf_scope_t *scope = NULL;;
    
    *scf_handle = NULL;
    *scf_scope = NULL;
    
    // Create a new Service Configuration Facility
    // handle, needed for the communication with the
    // configuration repository.
    handle = scf_handle_create(SCF_VERSION);
    if (handle == NULL) {
        return 0;
    }
    
    // Bind the handle to the running svc.config daemon
    if (scf_handle_bind(handle) != 0) {
        scf_handle_destroy(handle);
        return 0;
    }
    
    
    // Allocate a new scope. A scope is a top level of the 
    // SCF repository tree.  
    scope = scf_scope_create(handle);
    if (scope == NULL) {
        scf_handle_unbind(handle);
        scf_handle_destroy(handle);
        return 0;
    }
    
    // Set the scope to the root of the local SCF repository tree.
    if (scf_handle_get_scope(handle, SCF_SCOPE_LOCAL, scope) !=0) {
        scf_scope_destroy(scope);
        scf_handle_unbind(handle);
        scf_handle_destroy(handle);
        return 0;
    }
    
    *scf_handle = handle;
    *scf_scope = scope;
    
    return 1;
}


/* EQUIVALENT TO THE SVCCFG SELECT COMMAND */
static int lscf_select(scf_handle_t *scf_handle, 
                       scf_scope_t *scf_scope,
                       const char *selection,
                       scf_service_t **current_svc) 
{
    scf_service_t *svc;
    
    // Services are childrens of a  scope, and 
    // contain configuration informations for 
    // the service. 
    svc = scf_service_create(scf_handle);
    if (svc == NULL) {
        return 0;
    }

    // Set the service 'svc' to the service specified
    // by 'selection', in the scope 'scf_scope'.
    if (scf_scope_get_service(scf_scope, selection, svc) == SCF_SUCCESS) {
        *current_svc = svc;
        return 1;
    }
    
    scf_service_destroy(svc);
    return 0;
}

/* EQUIVALENT TO THE SVCCFG SETPROP COMMAND FOR AN INTEGER TYPED VALUE */
static int lscf_setprop_int(scf_handle_t *scf_handle, 
                            scf_scope_t *scf_scope,
                            scf_service_t *current_svc,
                            const char *group, 
                            const char *name, int value) 
{
    scf_transaction_entry_t *entry=NULL;
    scf_propertygroup_t *pg = NULL;
    scf_property_t *prop = NULL;
    scf_transaction_t *transax = NULL;
    scf_value_t *v = NULL;
    int status = 1;
    
    // Allocate a new transaction entry handle
    entry = scf_entry_create(scf_handle);
    if (entry == NULL) {
        status=0;
        goto done;
    }
    
    // Allocate a property group. 
    pg = scf_pg_create(scf_handle);
    if (pg == NULL) {
        status=0;
        goto done;
    }
    
    // Allocate a property. A property is a named set
    // of values.
    prop = scf_property_create(scf_handle);
    if (prop == NULL) {
        status=0;
        goto done;
    }
    
    // Allocate a transaction, used to change 
    // property groups.
    transax = scf_transaction_create(scf_handle);
    if (transax == NULL) {
        status=0;
        goto done;
    }
    
    // Allocate a value.
    v = scf_value_create(scf_handle);
    if (v == NULL) {
        status=0;
        goto done;
    }
    
    // Set the the property group 'pg' to the 
    // groups specified by 'group' in the service
    // specified by 'current_svc'
    if (scf_service_get_pg(current_svc, group, pg) != SCF_SUCCESS) {
        status=0;
        goto done;
    }
    
    // Update the property group.
    if (scf_pg_update(pg) == -1) {
        status=0;
        goto done;
    }
    
    // Set up the transaction to modify the property group.
    if (scf_transaction_start(transax, pg) != SCF_SUCCESS) {
        status=0;
        goto done;
    }
    
    // Set the property 'prop' to the property 
    // specified ny 'name' in the property group 'pg'
    if (scf_pg_get_property(pg, name, prop) == SCF_SUCCESS) {
        // Found 
        // It should be already of integer type.
        // To be secure, reset the property type to integer.
        if (scf_transaction_property_change_type(transax, entry,
            name, SCF_TYPE_INTEGER) == -1) {
            status=0;
            goto done;
        }      
    } else {
        // Not found
        // Add a new property to the property group.
        if (scf_transaction_property_new(transax, entry, 
                                         name, SCF_TYPE_INTEGER) 
            == -1) {
            status=0;
            goto done;
        }      
    }
    
    // Set the integer value
    scf_value_set_integer(v, value);
    
    // Set up the value to the property.
    if (scf_entry_add_value(entry, v) != SCF_SUCCESS) {
        status=0;
        goto done;
    }   
    
    // Commit the transaction
    if (scf_transaction_commit(transax) < 0) {
        status=0;
    }   
    
done:
    if (entry)      scf_entry_destroy(entry);
    if (pg)         scf_pg_destroy(pg);
    if (prop)       scf_property_destroy(prop);
    if (transax)    scf_transaction_destroy(transax);
    if (v)          scf_value_destroy(v);
    return status;
}

#else // NOT SOLARIS
int update_scf_depth(int depth) 
{
    return 1;
}
#endif
