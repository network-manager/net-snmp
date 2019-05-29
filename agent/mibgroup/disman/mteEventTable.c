/*
 * Note: this file originally auto-generated by mib2c using
 *        : mib2c.create-dataset.conf,v 5.2 2002/07/17 14:41:53 dts12 Exp $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-features.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "mteTriggerTable.h"
#include "mteEventTable.h"
#include "mteEventNotificationTable.h"
#include "mteObjectsTable.h"

netsnmp_feature_require(table_dataset)
netsnmp_feature_require(table_set_multi_add_default_row)

static netsnmp_table_data_set *table_set = NULL;

/** Initialize the mteEventTable table by defining its contents and how it's structured */
void
initialize_table_mteEventTable(void)
{
    static oid      mteEventTable_oid[] =
        { 1, 3, 6, 1, 2, 1, 88, 1, 4, 2 };
    size_t          mteEventTable_oid_len = OID_LENGTH(mteEventTable_oid);

    /*
     * create the table structure itself 
     */
    table_set = netsnmp_create_table_data_set("mteEventTable");

    /*
     * comment this out or delete if you don't support creation of new rows 
     */
    table_set->allow_creation = 1;
    /* mark the row status column */
    table_set->rowstatus_column = COLUMN_MTEEVENTENTRYSTATUS;

    /***************************************************
     * Adding indexes
     */
    DEBUGMSGTL(("initialize_table_mteEventTable",
                "adding indexes to table mteEventTable\n"));
    netsnmp_table_set_add_indexes(table_set,
                                  ASN_OCTET_STR,   /* index: mteOwner */
                                  ASN_PRIV_IMPLIED_OCTET_STR, /* index: mteEventName */
                                  0);

    DEBUGMSGTL(("initialize_table_mteEventTable",
                "adding column types to table mteEventTable\n"));
    netsnmp_table_set_multi_add_default_row(table_set,
                                            COLUMN_MTEEVENTNAME,
                                            ASN_OCTET_STR, 0, NULL, 0,
                                            COLUMN_MTEEVENTCOMMENT,
                                            ASN_OCTET_STR, 1, NULL, 0,
                                            COLUMN_MTEEVENTACTIONS,
                                            ASN_OCTET_STR, 1, NULL, 0,
                                            COLUMN_MTEEVENTENABLED,
                                            ASN_INTEGER, 1, NULL, 0,
                                            COLUMN_MTEEVENTENTRYSTATUS,
                                            ASN_INTEGER, 1, NULL, 0, 0);

    /* keep index values around for comparisons later */
    table_set->table->store_indexes = 1;
    /*
     * registering the table with the master agent 
     */
    /*
     * note: if you don't need a subhandler to deal with any aspects
     * of the request, change mteEventTable_handler to "NULL" 
     */
    netsnmp_register_table_data_set(netsnmp_create_handler_registration
                                    ("mteEventTable",
                                     mteEventTable_handler,
                                     mteEventTable_oid,
                                     mteEventTable_oid_len,
                                     HANDLER_CAN_RWRITE), table_set, NULL);
}

/** Initializes the mteEventTable module */
void
init_mteEventTable(void)
{

    /*
     * here we initialize all the tables we're planning on supporting 
     */
    initialize_table_mteEventTable();

    snmpd_register_config_handler("notificationEvent", parse_notificationEvent,
                                  NULL,
                                  "notificationEvent NAME TRAP_OID [[-w] EXTRA_OID ...]");

    snmpd_register_config_handler("linkUpDownNotifications",
                                  parse_linkUpDownNotifications,
                                  NULL,
                                  "linkUpDownNotifications (yes|no)");
}

/** handles requests for the mteEventTable table, if anything else needs to be done */
int
mteEventTable_handler(netsnmp_mib_handler *handler,
                      netsnmp_handler_registration *reginfo,
                      netsnmp_agent_request_info *reqinfo,
                      netsnmp_request_info *requests)
{
    /*
     * perform anything here that you need to do.  The requests have
     * already been processed by the master table_dataset handler, but
     * this gives you chance to act on the request in some other way
     * if need be. 
     */

    /* XXX: on rowstatus = destroy, remove the corresponding rows from the
       other tables: snmpEventNotificationTable and the set table */
    return SNMP_ERR_NOERROR;
}

void
parse_linkUpDownNotifications(const char *token, char *line) {
    if (strncmp(line, "y", 1) == 0) {
        parse_notificationEvent("notificationEvent", "linkUpTrap   	 linkUp     ifIndex ifAdminStatus ifOperStatus");
        parse_notificationEvent("notificationEvent", "linkDownTrap 	 linkDown   ifIndex ifAdminStatus ifOperStatus");

        parse_simple_monitor("monitor", "-r 60 -e linkUpTrap \"Generate linkUp\" ifOperStatus != 2");
        parse_simple_monitor("monitor", "-r 60 -e linkDownTrap \"Generate linkDown\" ifOperStatus == 2");
    }
}

void
parse_notificationEvent(const char *token, char *line) {
    char            name_buf[64];
    char            oid_name_buf[SPRINT_MAX_LEN];
    oid             oid_buf[MAX_OID_LEN];
    size_t          oid_buf_len = MAX_OID_LEN;
    int             wild = 1;
    netsnmp_table_row *row;
    long tlong;
    char tc;

    /* get the owner */
    const char *owner = "snmpd.conf";

    /* get the name */
    char *cp = copy_nword(line, name_buf, SPRINT_MAX_LEN);

    if (!cp || name_buf[0] == '\0') {
        config_perror("syntax error.");
        return;
    }

    for(row = table_set->table->first_row; row; row = row->next) {
        if (strcmp(row->indexes->val.string, owner) == 0 &&
            strcmp(row->indexes->next_variable->val.string,
                   name_buf) == 0) {
            config_perror("An eventd by that name has already been defined.");
            return;
        }
    }

    /* now, get all the trap oid */
    cp = copy_nword(cp, oid_name_buf, SPRINT_MAX_LEN);

    if (oid_name_buf[0] == '\0') {
        config_perror("syntax error.");
        return;
    }
    if (!snmp_parse_oid(oid_name_buf, oid_buf, &oid_buf_len)) {
        snmp_log(LOG_ERR,"namebuf: %s\n",oid_name_buf);
        config_perror("unable to parse trap oid");
        return;
    }

    /*
     * add to the mteEventNotificationtable to point to the
     * notification and the objects.
     */
    row = netsnmp_create_table_data_row();

    /* indexes */
    netsnmp_table_row_add_index(row, ASN_OCTET_STR, owner, strlen(owner));
    netsnmp_table_row_add_index(row, ASN_PRIV_IMPLIED_OCTET_STR,
                                name_buf, strlen(name_buf));


    /* columns */
    netsnmp_set_row_column(row, COLUMN_MTEEVENTNOTIFICATION, ASN_OBJECT_ID,
                           (char *) oid_buf, oid_buf_len * sizeof(oid));
    netsnmp_set_row_column(row, COLUMN_MTEEVENTNOTIFICATIONOBJECTSOWNER,
                           ASN_OCTET_STR, owner, strlen(owner));
    netsnmp_set_row_column(row, COLUMN_MTEEVENTNOTIFICATIONOBJECTS,
                           ASN_OCTET_STR, name_buf, strlen(name_buf));

    netsnmp_table_data_add_row(mteEventNotif_table_set->table, row);

    /*
     * add to the mteEventTable to make it a notification to trigger
     * notification and the objects.
     */
    row = netsnmp_create_table_data_row();

    /* indexes */
    netsnmp_table_row_add_index(row, ASN_OCTET_STR, owner, strlen(owner));
    netsnmp_table_row_add_index(row, ASN_PRIV_IMPLIED_OCTET_STR,
                                name_buf, strlen(name_buf));


    /* columns */
    tc = (u_char)0x80;
    netsnmp_set_row_column(row, COLUMN_MTEEVENTACTIONS, ASN_OCTET_STR,
                           &tc, 1);
    tlong = MTETRIGGERENABLED_TRUE;
    netsnmp_set_row_column(row, COLUMN_MTEEVENTENABLED,
                           ASN_INTEGER, (char *) &tlong, sizeof(tlong));
    tlong = RS_ACTIVE;
    netsnmp_set_row_column(row, COLUMN_MTEEVENTENTRYSTATUS,
                           ASN_INTEGER, (char *) &tlong, sizeof(tlong));

    netsnmp_table_data_add_row(table_set->table, row);
    
    /*
     * now all the objects to put into the trap's object row
     */
    while(cp) {
        cp = copy_nword(cp, oid_name_buf, SPRINT_MAX_LEN);
        if (strcmp(oid_name_buf, "-w") == 0) {
            wild = 0;
            continue;
        }
        oid_buf_len = MAX_OID_LEN;
        if (!snmp_parse_oid(oid_name_buf, oid_buf, &oid_buf_len)) {
            config_perror("unable to parse an object oid");
            return;
        }
        mte_add_object_to_table("snmpd.conf", name_buf,
                                oid_buf, oid_buf_len, wild);
        wild = 1;
    }
}

/*
 * send trap 
 */
void
run_mte_events(struct mteTriggerTable_data *item,
               oid * name_oid, size_t name_oid_len,
               const char *eventobjowner, const char *eventobjname)
{
    static oid      objid_snmptrap[] = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 };     /* snmpTrapIOD.0 */

    netsnmp_variable_list *var_list = NULL;

    netsnmp_table_row *row, *notif_row;
    netsnmp_table_data_set_storage *col1, *tc, *no, *noo;

    for(row = table_set->table->first_row; row; row = row->next) {
        if (strcmp(row->indexes->val.string, eventobjowner) == 0 &&
            strcmp(row->indexes->next_variable->val.string,
                   eventobjname) == 0) {
            /* run this event */
            col1 = (netsnmp_table_data_set_storage *) row->data;
            
            tc = netsnmp_table_data_set_find_column(col1,
                                                    COLUMN_MTEEVENTACTIONS);
            if (!(tc->data.bitstring[0] & 0x80)) {
                /* not a notification.  next! (XXX: do sets) */
                continue;
            }

            tc = netsnmp_table_data_set_find_column(col1,
                                                    COLUMN_MTEEVENTENABLED);
            if (*(tc->data.integer) != 1) {
                /* not enabled.  next! */
                continue;
            }

            if (!mteEventNotif_table_set) {
                /* no notification info */
                continue;
            }

            /* send the notification */
            var_list = NULL;

            /* XXX: get notif information */
            for(notif_row = mteEventNotif_table_set->table->first_row;
                notif_row; notif_row = notif_row->next) {
                if (strcmp(notif_row->indexes->val.string,
                           eventobjowner) == 0 &&
                    strcmp(notif_row->indexes->next_variable->val.string,
                           eventobjname) == 0) {

                    /* run this event */
                    col1 = (netsnmp_table_data_set_storage *) notif_row->data;
            
                    tc = netsnmp_table_data_set_find_column(col1, COLUMN_MTEEVENTNOTIFICATION);
                    no = netsnmp_table_data_set_find_column(col1, COLUMN_MTEEVENTNOTIFICATIONOBJECTS);
                    noo = netsnmp_table_data_set_find_column(col1, COLUMN_MTEEVENTNOTIFICATIONOBJECTSOWNER);
                    if (!tc)
                        continue; /* no notification to be had. XXX: return? */
                    
                    /*
                     * snmpTrap oid 
                     */
                    snmp_varlist_add_variable(&var_list, objid_snmptrap,
                                              sizeof(objid_snmptrap) /
                                              sizeof(oid),
                                              ASN_OBJECT_ID,
                                              (u_char *) tc->data.objid,
                                              tc->data_len);

                    /* XXX: add objects from the mteObjectsTable */
                    DEBUGMSGTL(("mteEventTable:send_events", "no: %x, no->data: %s", no, no->data.string));
                    DEBUGMSGTL(("mteEventTable:send_events", "noo: %x, noo->data: %s", noo, noo->data.string));
                    DEBUGMSGTL(("mteEventTable:send_events", "name_oid: %x",name_oid));
                    if (no && no->data.string &&
                        noo && noo->data.string && name_oid) {
                        char *tmpowner =
                            netsnmp_strdup_and_null(noo->data.string,
                                                    noo->data_len);
                        char *tmpname =
                            netsnmp_strdup_and_null(no->data.string,
                                                    no->data_len);

                        DEBUGMSGTL(("mteEventTable:send_events", "Adding objects for owner=%s name=%s", tmpowner, tmpname));
                        mte_add_objects(var_list, item,
                                        tmpowner, tmpname, 
                                       name_oid + item->mteTriggerValueIDLen,
                                        name_oid_len - item->mteTriggerValueIDLen);
                        free(tmpowner);
                        free(tmpname);
                    }

                    DEBUGMSGTL(("mteEventTable:send_events", "sending an event "));
                    DEBUGMSGOID(("mteEventTable:send_events", tc->data.objid, tc->data_len / sizeof(oid)));
                    DEBUGMSG(("mteEventTable:send_events", "\n"));
                    
                    send_v2trap(var_list);
                    snmp_free_varbind(var_list);
                }
            }
        }
    }
}
