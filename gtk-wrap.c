/******************************************************************************
 *                                                                            *
 *      gtk-wrap                                                              *
 *                                                                            *
 *      Author: Michal Jamry                                                  *
 *                                                                            *
 *      License: LGPL                                                         *
 *                                                                            *
 *                                                                            *
 *****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <assert.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define STRING_SIZE (64*1024)

short VERBOSE = 0;
short RUNNING = 1;
char *appname;
GtkBuilder *builder;
char *fpipeout = NULL;
char *fpipein = NULL;


/*
 * List of GObjects detected in the glade file that are
 * supported by gtk_get_text()/gtk_set_text() below.
 */
#define XML_OBJ_MAXCOUNT 100
int  xml_obj_count = 0;
char *xml_obj_name[XML_OBJ_MAXCOUNT];

/*
 * Multiline escaping
 * for easy handling in shell variables
 */
#define EOL_ESCAPE_CHAR '|'
void eol_escape(char *str)
{
    while((str = strchr(str, '\n')) != NULL)
        *(str++) = EOL_ESCAPE_CHAR;
}
void eol_unescape(char *str)
{
    while((str = strchr(str, EOL_ESCAPE_CHAR)) != NULL)
        *(str++) = '\n';
}

/*
 * Generic get_text/set_text for easy management of gtk controls/displays
 */
#define GTK_GETSET_TEXT_IMPLEMENTED \
        " GtkTextView GtkAdjustment GtkFileChooserButton" \
        " GtkWindow GtkEntry GtkSearchEntry GtkLabel GtkStack" \
        " GtkComboBox GtkComboBoxText" \
        " GtkToggleButton GtkCheckButton GtkRadioButton GtkSwitch GtkButton" \
        ""
#define ERR_NO_MEM -2
#define ERR_NOT_IMPLEMENTED -3
// If returning NULL, ret is an error ret_code < 0.
// Otherwise, if ret_code == 0, returned string must be g_freed.
// And if ret_code == 1, returned string is a const char *.
char *gtk_get_text(GObject *gobj, int *ret_code) {
    if (GTK_IS_TEXT_VIEW(gobj)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gobj));
        GtkTextIter start, end;
        char *ret;

        gtk_text_buffer_get_bounds(buffer, &start, &end);
        ret = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        eol_escape(ret);
        *ret_code = 0;
        return ret;
    }
    else if (GTK_IS_ADJUSTMENT(gobj)) /* SpinButton/Scale */ {
        char *ret = g_malloc(16);

        if (!ret) {
            *ret_code = ERR_NO_MEM;
            return NULL;
        }
        snprintf(ret, 16, "%.2f", gtk_adjustment_get_value(GTK_ADJUSTMENT(gobj)));
        *ret_code = 0;
        return ret;
    }
    else if (GTK_IS_FILE_CHOOSER(gobj)) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(gobj);
        GtkFileChooserAction chooser_type = gtk_file_chooser_get_action(chooser);
        *ret_code = 0;
        if (chooser_type == GTK_FILE_CHOOSER_ACTION_SAVE
        ||  chooser_type == GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
            return gtk_file_chooser_get_current_name(chooser);
        else
            return gtk_file_chooser_get_filename(chooser);
    }

    *ret_code = 1;
    if (GTK_IS_WINDOW(gobj)) {
        return (char *)gtk_window_get_title(GTK_WINDOW(gobj));
    }
    else if (GTK_IS_ENTRY(gobj)) /* GtkSearchEntry */ {
        return (char *)gtk_entry_get_text(GTK_ENTRY(gobj));
    }
    else if (GTK_IS_LABEL(gobj)) {
        return (char *)gtk_label_get_text(GTK_LABEL(gobj));
    }
    else if (GTK_IS_STACK(gobj)) {
        return (char *)gtk_stack_get_visible_child_name(GTK_STACK(gobj));
    }
    else if (GTK_IS_COMBO_BOX(gobj)) /* ComboBox/ComboBoxText */ {
        return (char *)gtk_combo_box_get_active_id(GTK_COMBO_BOX(gobj));
    }
    else if (GTK_IS_TOGGLE_BUTTON(gobj)) /* Toggle/Check/Radio-Button */ {
        return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gobj)) ? "true" : "false";
    }
    else if (GTK_IS_SWITCH(gobj)) {
        return gtk_switch_get_active(GTK_SWITCH(gobj)) ? "true" : "false";
    }

    *ret_code = ERR_NOT_IMPLEMENTED;
    return NULL;
}

int gtk_set_text(GObject *gobj, char *text) {
    if (GTK_IS_TEXT_VIEW(gobj)) {
        eol_unescape(text);
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gobj)), text, -1);
    }
    else if (GTK_IS_ADJUSTMENT(gobj)) /* SpinButton/Scale */ {
        gtk_adjustment_set_value(GTK_ADJUSTMENT(gobj), atof(text));
    }
    else if (GTK_IS_FILE_CHOOSER(gobj)) {
        if (!strcmp(text, "")) {
            gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(gobj));
        } else {
            GtkFileChooser *chooser = GTK_FILE_CHOOSER(gobj);
            GtkFileChooserAction chooser_type = gtk_file_chooser_get_action(chooser);
            if (chooser_type == GTK_FILE_CHOOSER_ACTION_SAVE ||
                chooser_type == GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
                gtk_file_chooser_set_current_name(chooser, text);
            else
                gtk_file_chooser_set_filename(chooser, text);
        }
    }
    else if (GTK_IS_WINDOW(gobj)) {
        gtk_window_set_title(GTK_WINDOW(gobj), text);
    }
    else if (GTK_IS_ENTRY(gobj)) {
        gtk_entry_set_text(GTK_ENTRY(gobj), text);
    }
    else if (GTK_IS_LABEL(gobj)) {
        gtk_label_set_text(GTK_LABEL(gobj), text);
    }
    else if (GTK_IS_STACK(gobj)) {
        gtk_stack_set_visible_child_name(GTK_STACK(gobj), text);
    }
    else if (GTK_IS_COMBO_BOX(gobj)) /* ComboBox/ComboBoxText */ {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(gobj), text);
    }
    else if (GTK_IS_TOGGLE_BUTTON(gobj)) /* Toggle/Check/Radio-Button */ {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gobj), strcmp(text, "true") ? 0 : 1);
    }
    else if (GTK_IS_SWITCH(gobj)) {
        gtk_switch_set_active(GTK_SWITCH(gobj), strcmp(text, "true") ? 0 : 1);
    }
    else
        return ERR_NOT_IMPLEMENTED;
    return 0;
}


/* ----------------------------------------------
 * Signal handlers
 */
void on_window_destroy(GObject *object, gpointer user_data){
    if(VERBOSE)
        fprintf(stderr, "Caught destroy signal from main widget!\n"
                        "Quitting...\n");

    RUNNING = 0;
    gtk_main_quit();
}


void signal_handler(gpointer user_data, GObject *object){
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


void *reader_loop(void* wojd){
    FILE *filein, *fileout;
    char input[2*STRING_SIZE];
    char *delims_field = " \n";
    char *delims_eol = "\n";
    char *next_token;
    char *undefined = "";
    char *object;
    char *command;
    char *operanda;

    if (fpipeout) {
        mkfifo(fpipeout, S_IRWXU);
        fileout = fopen(fpipeout, "a+");
        if(!fileout){
            fprintf(stderr, "Error opening pipe %s !\n", fpipeout);
            pthread_exit(NULL);
        }
    } else fileout = stdout;

    mkfifo(fpipein, S_IRWXU);
    filein = fopen(fpipein, "r+");
    if(!filein){
        fprintf(stderr, "Error opening pipe %s !\n", fpipein);
        pthread_exit(NULL);
    }

    if(VERBOSE)
        fprintf(stderr, "Using pipes out:%s in:%s\n", fpipeout ? fpipeout : "-", fpipein);

    while(RUNNING){
        GtkWidget *widget;

        if (!fgets(input, sizeof(input), filein))
            break;

        if(!RUNNING)
            break;

        if (!(object = strtok_r(input, delims_field, &next_token))) {
            object = command = operanda = undefined;
        } else {
            if (!(command = strtok_r(NULL, delims_field, &next_token))) {
                command = operanda = undefined;
            } else {
                if (!(operanda = strtok_r(NULL, delims_eol, &next_token)))
                    operanda = undefined;
            }
        }
        if(VERBOSE)
            fprintf(stderr, "Command:> %s %s %s\n", object, command, operanda);

        widget = GTK_WIDGET(gtk_builder_get_object(builder, object));


        //window set title
        if(!strcmp(command, "set_window_title")){
            gtk_window_set_title(GTK_WINDOW(widget), operanda);
        } else

        //window show
        if(!strcmp(command, "show")){
            gtk_widget_show(widget);
        } else

        //window hide
        if(!strcmp(command, "hide")){
            gtk_widget_hide(widget);
        } else


        //textview set text
        if(!strcmp(command, "set_textview_text")){
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)), operanda, -1);
        } else

        //textview get text
        if(!strcmp(command, "get_textview_text")){
            GtkTextIter a, b;
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
            gtk_text_buffer_get_iter_at_offset(buffer, &a, 0);
            gtk_text_buffer_get_iter_at_offset(buffer, &b, -1);
            gchar* mtext = gtk_text_buffer_get_text(buffer, &a, &b, FALSE);
            fprintf(fileout, "%s\n", mtext);
            fflush(fileout);
        } else

        //spinner activate/deactivate
        if(!strcmp(command, "spinner_start")){
            gtk_spinner_start(GTK_SPINNER(widget));
        } else

        if(!strcmp(command, "spinner_stop")){
            gtk_spinner_stop(GTK_SPINNER(widget));
        } else

        //label set/get
        if(!strcmp(command, "set_label_text")){
            gtk_label_set_text(GTK_LABEL(widget), operanda);
        } else

        //set button label
        if(!strcmp(command, "set_button_label")){
            gtk_button_set_label(GTK_BUTTON(widget), operanda);
        } else

        //entrytext set/get
        if(!strcmp(command, "get_entry_text")){
            gchar* mtext = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
            fprintf(fileout, "%s\n", mtext);
            fflush(fileout);
        } else

        if(!strcmp(command, "set_entry_text")){
            gtk_entry_set_text(GTK_ENTRY(widget), operanda);
        } else


        //combobox add options, get/set selected
        if(!strcmp(command, "set_combobox_items")){
            //GtkTreeModel *tree_model;
            //gtk_combo_box_model_set(GTK_COMBO_BOX(widget), tree_model);
        } else

        if(!strcmp(command, "get_selected_combobox_item")){
            fprintf(fileout, "%d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));
            fflush(fileout);
        } else

        //image set image TODO doesn't work
        if(!strcmp(command, "set_image")){
            gtk_image_set_from_file(GTK_IMAGE(widget), operanda);
            gtk_widget_show(widget);
        } else

        //progressbar set, show/hide
        if(!strcmp(command, "set_progressbar")){
        } else

        //togglebutton istoggled //toggle, check, radio button
        if(!strcmp(command, "get_button_state")){
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
                fprintf(fileout, "1\n");
            else
                fprintf(fileout, "0\n");
            fflush(fileout);
        }

    }

    fclose(filein);
    if (fpipeout) {
        fflush(fileout);
        fclose(fileout);
    }
    pthread_exit(NULL);
}

/*
 * Parse glade file to get the list of GObjects
 * that can be handled through gtk_get_text()/gtk_set_text() above.
 */
void auto_get_objects(xmlXPathContextPtr glade_xml) {
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
        xml_obj_name[xml_obj_count] = strndup((char *)xmlNodeGetContent(nodes->nodeTab[xml_obj_count]), STRING_SIZE);
    }

    xmlXPathFreeObject(xpath_obj);
}

//Adding signals handled in glade file
void auto_add_signals(xmlXPathContextPtr glade_xml) {
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
        if (VERBOSE)
            fprintf(stderr, "Found signal \"%s\", handled by \"%s\" in object \"%s\"\n", signame, sighandler, objname);
        GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object( builder, objname));
        g_signal_connect_swapped(widget, signame, G_CALLBACK(signal_handler), sighandler);
    }

    xmlXPathFreeObject(xpath_obj);
}


int parse_glade(const char *filename, GtkBuilder *builder) {
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
    auto_add_signals(glade_xml);

    xmlXPathFreeContext(glade_xml);
    xmlFreeDoc(glade_file);
    return 0;
}

void usage(){
    fprintf(stderr,
        "Usage:\n"
        "%s \n"
        "\n"
        "Options:\n"
        "-f project.glade\n"
        "-m OBJECTNAME \t\t Set object as a main window. Default \"window1\".\n"
        "-v \t\t\t Be more verbose.\n"
        "-i INPIPENAME \t\t Use pipe for commands instead of standard input.\n"
        "-o OUTPIPE \t\t Use pipe for commands output.\n"
    , appname);

    exit(1);
}

void xml_obj_free() {
    int i;

    for (i = 0; i < xml_obj_count; i++) {
        free(xml_obj_name[i]);
    }
}

int main(int argc, char *argv[])
{
    char *filename = NULL;
    char *main_object = (char*)"window1";
    int argn;
    int ret = 0;
    pthread_t thread = 0;

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    appname = argv[0];

    for (argn = 1; argn < argc; argn++)
    {
        if(strlen(argv[argn]) < 2 )
            continue;

        if(argv[argn][0] == '-'){
            switch(argv[argn][1]){

                //load object as a main widget, default window1
                case 'm' :
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        main_object = argv[++argn];
                    continue;

                //verbose
                case 'v':
                    VERBOSE = 1;
                    continue;

                //command output pipe
                case 'o':
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        fpipeout = argv[++argn];
                    continue;

                //command input pipe
                case 'i':
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        fpipein = argv[++argn];
                    continue;

                //read ui from GtkBuilder(Glade) file
                case 'f':
                    if(filename != NULL)
                        usage();

                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        filename = argv[++argn];
                    continue;

                default:
                    usage();
                    break;
            }
        }
        break;
    }

    if(!filename)
        usage();

    if(VERBOSE)
        fprintf(stderr, "Loading widget \"%s\" as a main window.\n", main_object);

    GtkWidget *window;
    GError *error = NULL;

    argv = &argv[argn];
    argc -= argn;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();

    if (!gtk_builder_add_from_file(builder, filename, &error))
    {
        fprintf(stderr, "Error occured while loading UI!\n");
        fprintf(stderr, "Message: %s\n", error->message);
        g_free(error);
        return 1;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, main_object));

    //Adding default closing signal
    g_signal_connect_swapped(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    //Adding other signals
    if ((ret = parse_glade(filename, builder)) != 0)
        goto end;

    gtk_widget_show(window);

    //starting command reader
    if (fpipein)
        pthread_create(&thread, NULL, reader_loop, NULL);

    gtk_main();

end:
    RUNNING = 0;

    if (fpipein && thread)
        pthread_cancel(thread);

    g_object_unref(G_OBJECT(builder));

    if(VERBOSE)
        fprintf(stderr, "Cleaning...\n");

    unlink(fpipeout);
    unlink(fpipein);
    xml_obj_free();

    return ret;
}
