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

/* ----------------------------------------------
 * Multiline escaping
 * for easy handling in shell variables
 */
#define EOL_ESCAPE_CHAR '|'
static void eol_escape(char *str)
{
    while((str = strchr(str, '\n')) != NULL)
        *(str++) = EOL_ESCAPE_CHAR;
}
static void eol_unescape(char *str)
{
    while((str = strchr(str, EOL_ESCAPE_CHAR)) != NULL)
        *(str++) = '\n';
}


/* ----------------------------------------------
 * Generic get_text/set_text for easy management
 * of gtk controls/displays
 */

/*
 * If returning NULL, ret is an error ret_code < 0.
 * Possible error codes: ERR_NO_MEM, ERR_NOT_IMPLEMENTED
 * Otherwise, if ret_code == 0, returned string must be g_freed.
 * And if ret_code == 1, returned string is a const char *.
 */
char *gtk_get_text(GObject *gobj, int *ret_code) {
    if (GTK_IS_TEXT_VIEW(gobj)) {
        if (gtk_text_view_get_input_purpose(GTK_TEXT_VIEW(gobj)) == GTK_INPUT_PURPOSE_TERMINAL)
        {
            *ret_code = 1;
            return "";
        }
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
    else if (GTK_IS_CHECK_MENU_ITEM(gobj)) {
        return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gobj)) ? "true" : "false";
    }

    *ret_code = ERR_NOT_IMPLEMENTED;
    return NULL;
}

/*
 * Read a text file entirely.
 * Ensures \0 termination.
 * Returned buffer must be freed.
 * Returns NULL on error.
 */
static char *read_entire_txt_file(const char *input)
{
    FILE *fp = fopen(input, "rb");
    long size;
    char *ret;

    if (!fp)
        return NULL;
    if (fseek(fp, 0, SEEK_END))
        return NULL;
    size = ftell(fp);
    if (size < 0)
        return NULL;
    if (fseek(fp, 0, SEEK_SET))
        return NULL;
    ret = malloc(size + 1);
    if (!ret)
        return NULL;
    if (fread(ret, 1, size, fp) != size) {
        free(ret);
        return NULL;
    }
    ret[size] = 0;
    return ret;
}

int gtk_set_text(GObject *gobj, char *text) {
    if (GTK_IS_TEXT_VIEW(gobj)) {
        if (gtk_text_view_get_input_purpose(GTK_TEXT_VIEW(gobj)) == GTK_INPUT_PURPOSE_TERMINAL
            && strlen(text)) {
            char *content = read_entire_txt_file(text);
            if (content) {
                gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gobj)), content, -1);
                free(content);
            } else {
                gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gobj)), "", -1);
            }
        }
        else {
            eol_unescape(text);
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gobj)), text, -1);
        }
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
    else if (GTK_IS_CHECK_MENU_ITEM(gobj)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gobj), strcmp(text, "true") ? 0 : 1);
    }
    else
        return ERR_NOT_IMPLEMENTED;
    return 0;
}
