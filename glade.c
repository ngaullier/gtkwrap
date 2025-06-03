/******************************************************************************
 *                                                                            *
 *      gtk-wrap                                                              *
 *                                                                            *
 *      Author: Michal Jamry                                                  *
 *              Nicolas Gaullier                                              *
 *                                                                            *
 *      License: LGPL                                                         *
 *                                                                            *
 *                                                                            *
 *****************************************************************************/


#include <assert.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "gtk-wrap.h"

GtkBuilder *builder;

/*
 * List of GObjects detected in the glade file that are
 * supported by gtk_get_text()/gtk_set_text().
 */
int  xml_obj_count = 0;
char *xml_obj_name[XML_OBJ_MAXCOUNT];
char *xml_obj_default[XML_OBJ_MAXCOUNT];


/* ----------------------------------------------
 * Generic signal handler
 * Write a command line on stdout (to be bash-interpreted),
 * including setting variables reflecting the actual gui state.
 */
static void signal_handler(gpointer user_data, GObject *object)
{
    char *jo = (char *)user_data;
    int i;

    for (i = 0; i < xml_obj_count; i++) {
        int ret;
        GObject *gobj = gtk_builder_get_object(builder, (char *)xml_obj_name[i]);
        char *obj_value = gtk_get_text(gobj, &ret);

        if (obj_value != NULL) {
            fprintf(stdout, "%s=\"%s\" ", xml_obj_name[i], obj_value);
            if (ret == 0)
                g_free(obj_value);
        } else {
            fprintf(stdout, "%s= ", xml_obj_name[i]);
        }
    }

    fprintf(stdout, "%s\n", jo);
    fflush(stdout);
}


/*
 * Parse glade file to get the list of GObjects
 * that can be handled through gtk_get_text()/gtk_set_text() above.
 */
static void auto_get_objects(xmlXPathContextPtr glade_xml) {
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodes;
    int count;

    xpath_obj = xmlXPathEvalExpression((const xmlChar *)"//object[contains('"
            GTK_GETSET_TEXT_IMPLEMENTED
            "',@class)]/@id", glade_xml);
    assert(xpath_obj != NULL);

    nodes = xpath_obj->nodesetval;
    count = (nodes) ? nodes->nodeNr : 0;
    for(xml_obj_count = 0; xml_obj_count < count && xml_obj_count < XML_OBJ_MAXCOUNT; xml_obj_count++) {
        GObject *gobj;
        char *obj_value;
        int ret;

        xml_obj_name[xml_obj_count] = strndup((char *)xmlNodeGetContent(nodes->nodeTab[xml_obj_count]), STRING_SIZE);
        gobj = gtk_builder_get_object(builder, xml_obj_name[xml_obj_count]);
        obj_value = gtk_get_text(gobj, &ret);
        if (obj_value) {
            xml_obj_default[xml_obj_count] = strndup(obj_value, STRING_SIZE);
            if (ret == 0)
                g_free(obj_value);
        } else {
            xml_obj_default[xml_obj_count] = strdup("");
        }

    }

    xmlXPathFreeObject(xpath_obj);
}

/*
 * Parse glade file to get the list of all signals.
 * Connect them to the generic signal handler.
 */
static void auto_add_signals(xmlXPathContextPtr glade_xml, int doVerbose) {
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodes;
    int i, count;

    xpath_obj = xmlXPathEvalExpression((const xmlChar *)"//object[@id]/signal[@handler][@name]", glade_xml);
    assert(xpath_obj != NULL);

    nodes = xpath_obj->nodesetval;
    count = (nodes) ? nodes->nodeNr : 0;
    for(i = 0; i < count; i++) {
        xmlNodePtr node_signal = nodes->nodeTab[i];
        xmlNodePtr node_object = nodes->nodeTab[i]->parent;
        char *objname = (char *)xmlGetProp(node_object, (const xmlChar *)"id");
        char *signame = (char *)xmlGetProp(node_signal, (const xmlChar *)"name");
        char *sighandler = (char *)xmlGetProp(node_signal, (const xmlChar *)"handler");
        assert(objname != NULL && signame != NULL && sighandler != NULL);
        if (doVerbose)
            fprintf(stderr, "Found signal \"%s\", handled by \"%s\" in object \"%s\"\n", signame, sighandler, objname);
        GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object( builder, objname));
        g_signal_connect_swapped(widget, signame, G_CALLBACK(signal_handler), sighandler);
    }

    xmlXPathFreeObject(xpath_obj);
}


int parse_glade(const char *filename, int doVerbose) {
    xmlDocPtr glade_file;
    xmlXPathContextPtr glade_xml;

    glade_file = xmlReadFile(filename, NULL, 0);
    if (!glade_file) {
        fprintf(stderr, "Failed to parse %s\n", filename);
        return 1;
    }

    glade_xml = xmlXPathNewContext(glade_file);
    if (!glade_xml) {
        xmlFreeDoc(glade_file);
        return 1;
    }

    auto_get_objects(glade_xml);
    auto_add_signals(glade_xml, doVerbose);

    xmlXPathFreeContext(glade_xml);
    xmlFreeDoc(glade_file);
    return 0;
}

void xml_obj_free() {
    int i;

    for (i = 0; i < xml_obj_count; i++) {
        free(xml_obj_name[i]);
        free(xml_obj_default[i]);
    }
}
