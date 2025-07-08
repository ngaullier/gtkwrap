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

#define STRING_SIZE (64*1024)

#define ERR_NO_MEM -2
#define ERR_NOT_IMPLEMENTED -3

#define XML_OBJ_MAXCOUNT 100

#define GTK_GETSET_TEXT_IMPLEMENTED \
        " GtkTextView GtkAdjustment GtkFileChooserButton GtkFileChooserDialog \n" \
        " GtkWindow GtkEntry GtkSearchEntry GtkLabel GtkStack \n" \
        " GtkComboBox GtkComboBoxText \n" \
        " GtkToggleButton GtkCheckButton GtkRadioButton GtkSwitch GtkButton \n" \
        " GtkCheckMenuItem \n" \
        ""

#define BOOL2STR(X) ((X) ? "true" : "false")
#define STR2BOOL(X) (!strcmp(X, "true") ? 1 : 0)

/*
 * For use by g_idle_add
 *
 * https://stackoverflow.com/questions/20695068/how-to-pass-multiple-arguments-through-g-idle-add#20696225
 */
struct gtkwrap_command_args {
    char *input;
    FILE *fileout;
    int VERBOSE;
};


/* from glade.c */
extern GtkBuilder *builder;
extern int  xml_obj_count;
extern char *xml_obj_name[XML_OBJ_MAXCOUNT];
extern char *xml_obj_default[XML_OBJ_MAXCOUNT];
extern void xml_obj_free();
extern int parse_glade(const char *filename, int doVerbose);

/* from gtk-timecode */
extern void gtk_timecode_tag_field_separator(GtkTextBuffer *text_buffer);
extern void gtk_timecode_valid(GtkTextBuffer *text_buffer, int cursor_move);
extern void signal_handler_timecode_changed(gpointer data, GObject *text_buffer);
extern void signal_handler_timecode_move_cursor(gpointer user_data, GObject *text_view, int cursor_move);

/* from gtk-wrap.c */
extern char *gtk_get_text(GObject *gobj, int *ret_code);
extern int gtk_set_text(GObject *gobj, char *text);
extern int gtk_set_label(GObject *gobj, char *text);

/* from commands.c */
extern void gtkwrap_command(gpointer data);
