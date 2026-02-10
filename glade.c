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

        if (GTK_IS_WIDGET(gobj))
            fprintf(stdout, "isenabled_%s=%s ", xml_obj_name[i],
                    gtk_widget_is_sensitive(GTK_WIDGET(gobj)) && gtk_widget_is_visible(GTK_WIDGET(gobj)) ? "true" : "false"
            );

        if (GTK_IS_FILE_CHOOSER(gobj)) {
            GtkFileChooser *file_chooser = GTK_FILE_CHOOSER(gobj);
            if (gtk_file_chooser_get_preview_widget_active(file_chooser))
                fprintf(stdout, "%s_preview=\"%s\" ", xml_obj_name[i],
                        gtk_file_chooser_get_preview_filename(file_chooser));
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
            "',@class)][not(starts-with(@id,'_'))]/@id", glade_xml);
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
        g_signal_connect_swapped(
                gtk_builder_get_object( builder, objname),
                signame, G_CALLBACK(signal_handler), sighandler);
    }

    xmlXPathFreeObject(xpath_obj);
}

/* ----------------------------------------------
 * auto_set_timecode
 * =======================
 * Parse glade file to get the list of GtkTextBuffers representing a Timecode
 * And initialize them.
 * Note: they are identified by their parent's style class being 'timecode'.
 */
static void auto_set_timecode(xmlXPathContextPtr glade_xml, int doVerbose) {
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodes;
    int i, count;

    xpath_obj = xmlXPathEvalExpression((const xmlChar *)
            "//object[@class='GtkTextView'][@id][style/class[@name='timecode']]/property[@name='buffer']"
            , glade_xml);
    assert(xpath_obj != NULL);

    nodes = xpath_obj->nodesetval;
    count = (nodes) ? nodes->nodeNr : 0;
    for(i = 0; i < count; i++) {
        xmlNodePtr node_property = nodes->nodeTab[i];
        xmlNodePtr node_object = nodes->nodeTab[i]->parent;
        char *textbuff_id = (char *)xmlNodeGetContent(node_property);
        char *textview_id = (char *)xmlGetProp(node_object, (const xmlChar *)"id");
        assert(textbuff_id != NULL && textview_id != NULL);
        if (doVerbose)
            fprintf(stderr, "Set signals for timecode handling in \"%s/%s\"\n", textview_id, textbuff_id);
        GtkTextBuffer *textbuff = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, textbuff_id));
        GtkTextView *textview = GTK_TEXT_VIEW(gtk_builder_get_object(builder, textview_id));
        g_signal_connect_swapped(
                textbuff,
                "changed", G_CALLBACK(signal_handler_timecode_changed), NULL);
        g_signal_connect_swapped(
                textview,
                "move-cursor", G_CALLBACK(signal_handler_timecode_move_cursor), textbuff_id);
        gtk_timecode_valid(textbuff, 0);
        gtk_timecode_tag_field_separator(textbuff);
    }

    xmlXPathFreeObject(xpath_obj);
}


/* ----------------------------------------------
 * auto_set_filechooser
 * =======================
 * When the action is 'save' (ie. GTK_FILE_CHOOSER_ACTION_SAVE),
 * add two buttons to close the dialog: Cancel/Save.
 * NOTE: The glade UI allows only 'open', so 'save' has to be written by hand.
 */
static void auto_set_filechooser(xmlXPathContextPtr glade_xml, int doVerbose) {
    xmlXPathObjectPtr xpath_obj;
    xmlNodeSetPtr nodes;
    int i, count;

    xpath_obj = xmlXPathEvalExpression((const xmlChar *)
            "//object[@class='GtkFileChooserDialog'][property[@name='action']='save']/@id"
            , glade_xml);
    assert(xpath_obj != NULL);

    nodes = xpath_obj->nodesetval;
    count = (nodes) ? nodes->nodeNr : 0;
    for(i = 0; i < count; i++) {
        char *filechooser_id = (char *)xmlNodeGetContent(nodes->nodeTab[i]);
        GObject *gobj = gtk_builder_get_object(builder, filechooser_id);

        /*
         * GTK-Warning 'Failed to measure available space' can arise but can be ignored.
         * The warning disappear once the dialog has been properly resized.
         * See https://gitlab.gnome.org/GNOME/gtk/-/issues/2509
         * forwarded to https://gitlab.gnome.org/GNOME/gtk/-/issues/4891 (still open)
         */
        if (doVerbose)
            fprintf(stderr, "Found filechooser/filesave \"%s\"\n", filechooser_id);
        gtk_dialog_add_buttons(GTK_DIALOG(gobj),
                "_Cancel",  GTK_RESPONSE_CANCEL,
                "_Save",    GTK_RESPONSE_ACCEPT,
                NULL);
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
    auto_set_timecode(glade_xml, doVerbose);
    auto_set_filechooser(glade_xml, doVerbose);

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
