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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#define ERR_NO_MEM -2
#define ERR_NOT_IMPLEMENTED -3

#define XML_OBJ_MAXCOUNT 100

/*
 * For use by g_idle_add
 *
 * https://stackoverflow.com/questions/20695068/how-to-pass-multiple-arguments-through-g-idle-add#20696225
 */
struct gtkwrap_command_args {
    //int *request;
    char *input;
    FILE *fileout;
    int VERBOSE;
};

/* from gtk-wrap.c */
extern short RUNNING;
extern int  xml_obj_count;
extern char *xml_obj_name[XML_OBJ_MAXCOUNT];
extern char *xml_obj_default[XML_OBJ_MAXCOUNT];
extern GtkBuilder *builder;

extern char *gtk_get_text(GObject *gobj, int *ret_code);
extern int gtk_set_text(GObject *gobj, char *text);


/* from commands.c */
extern void gtkwrap_command(gpointer data);
