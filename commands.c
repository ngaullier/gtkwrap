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

#include "gtk-wrap.h"

/*
 * Set gtk controls from glade file
 * NB: support obj_name="all"
 */
static void command_reset(char *obj_name) {
    int i;

    for(i = 0; i < xml_obj_count; i++) {
        if (!strcmp(obj_name, "all") || !strcmp(obj_name, xml_obj_name[i])) {
            GObject *gobj = gtk_builder_get_object(builder, xml_obj_name[i]);

            gtk_set_text(gobj, xml_obj_default[i]);
        }
    }
}

/*
 * Parse and run a line of a command.
 * To be run by main GTK Thread (though g_idle_add_once)
 * since GTK is not thread safe.
 */
void gtkwrap_command(gpointer data) {
    char *delims_field = " \n";
    char *delims_eol = "\n";
    char *next_token;
    char *undefined = "";
    char *object;
    char *command;
    char *operanda;
    GObject *gobj;
    struct gtkwrap_command_args *args = data;
    char *input = args->input;
    FILE *fileout = args->fileout;
    int VERBOSE = args->VERBOSE;

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

    if (VERBOSE)
        fprintf(stderr, "Command:> %s %s %s\n", object, command, operanda);

    if (!strcmp(object, "exit")) {
        gtk_main_quit();
        goto exit;
    }

    if(!strcmp(command, "reset")) {
        command_reset(object);
        goto exit;
    }

    gobj = gtk_builder_get_object(builder, object);
    if (!gobj) {
        fprintf(stderr, "object '%s' not found\n", object);
        goto exit;
    }

    if (GTK_IS_WIDGET(gobj)) {
        GtkWidget *widget = GTK_WIDGET(gobj);

        if(!strcmp(command, "show")) {
            if (!strcmp(operanda, "true"))
                gtk_widget_show(widget);
            else
                gtk_widget_hide(widget);
            goto exit;
        }

        if (!strcmp(command, "enable")) {
            gtk_widget_set_sensitive(GTK_WIDGET(gobj),
                    strcmp(operanda, "true") ? 0 : 1);
            goto exit;
        }

        if (!strcmp(command, "name")) {
            gtk_widget_set_name(GTK_WIDGET(gobj), operanda);
            goto exit;
        }
    }

    if (!strcmp(command, "set")) {
        if (gtk_set_text(gobj, operanda))
            fprintf(stderr, "set: %s is not supported\n",
                    g_type_name(G_TYPE_FROM_INSTANCE(gobj)));
        goto exit;
    }

    if(!strcmp(command, "get")) {
        int ret;
        char *out = gtk_get_text(gobj, &ret);

        if (out != NULL) {
            fprintf(fileout, out);
        } else switch (ret) {
            case ERR_NOT_IMPLEMENTED:
                fprintf(stderr, "get: %s is not supported\n",
                        g_type_name(G_TYPE_FROM_INSTANCE(gobj)));
                goto exit;
            default:
                fprintf(stderr, "get: error\n");
                goto exit;
        }
        fprintf(fileout, "\n");
        fflush(fileout);
        goto exit;
    }

    fprintf(stderr, "unexpected command: '%s'\n", command);

exit:
    g_free(args->input);
    args->input = 0;
}
