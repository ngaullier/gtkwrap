/******************************************************************************
 *                                                                            *
 *      gtk-timecode                                                          *
 *                                                                            *
 *      Author: Nicolas Gaullier                                              *
 *                                                                            *
 *                                                                            *
 *      License: LGPL                                                         *
 *                                                                            *
 *                                                                            *
 *****************************************************************************/

#include <assert.h>
#include "gtk-wrap.h"

/* ----------------------------------------------
 * gtk_timecode_tag_field_separator
 * ================================
 * Protect the timecode field separators: make them non-editable.
 */
void gtk_timecode_tag_field_separator(GtkTextBuffer *textbuff)
{
    GtkTextTag *tag = gtk_text_buffer_create_tag(textbuff, NULL, "editable", 0, NULL);
    int i;

    for (i = 0; i < 3 * 3; i += 3)
    {
        GtkTextIter start, end;

        gtk_text_buffer_get_iter_at_offset(textbuff, &start, 2 + i);
        gtk_text_buffer_get_iter_at_offset(textbuff, &end, 3 + i);
        gtk_text_buffer_apply_tag (textbuff, tag, &start, &end);
    }
}

/* ----------------------------------------------
 * gtk_timecode_valid
 * ==================
 * Check/fix the timecode fields formating and cursor position.
 *
 * Reset invalid and missing fields to '00'.
 * Note: all fields following an invalid field are considered invalids.
 * Make the cursor automatically skip the field separators
 * when typing or moving the cursor.
 *
 * Note that 'overwriting' mode is handled by GTK in two steps:
 * - first delete a digit
 * - then insert the new digit
 * Similarly, 'set_text' is also handled in two steps:
 * - first delete everything
 * - then insert all text
 * BUT, this function is meant to be used through g_idle_add()
 * which means the first temporary step will not be catched,
 * and the stable final step will be catched twice.
 * Moreover, the 'cursor_move' is interpreted as a 'cursor_moved',
 * since the cursor is assumed to have already moved.
 */
void gtk_timecode_valid(GtkTextBuffer *textbuff, int cursor_move)
{
    GtkTextIter start, end;
    char *tc_str;
    int tc_len;
    char *delims_field = ":\n";
    char *next_token;
    char *tokens[4];
    int i;

    gtk_text_buffer_get_bounds(textbuff, &start, &end);
    tc_str = gtk_text_buffer_get_text(textbuff, &start, &end, TRUE);
    tc_len = strlen(tc_str);

    for (i = 0; i < 4; i++) {
        tokens[i] = strtok_r(!i ? tc_str : NULL, delims_field, &next_token);
        if (!tokens[i] || strlen(tokens[i]) != 2
                || tokens[i][0] < '0' || tokens[i][0] > '5'
                || tokens[i][1] < '0' || tokens[i][1] > '9'
                )
            break;
    }
    if (i == 4 && tc_len == 11) {
        GtkTextMark *cursor_mark = gtk_text_buffer_get_insert(textbuff);
        GtkTextIter cursor;
        int cursor_pos;

        gtk_text_buffer_get_iter_at_mark(textbuff, &cursor, cursor_mark);
        cursor_pos = gtk_text_iter_get_offset(&cursor);

        if (!( (cursor_pos + 1) % 3)) {
            if (cursor_move < 0)
                gtk_text_iter_backward_cursor_position(&cursor);
            else
                gtk_text_iter_forward_cursor_position(&cursor);
            gtk_text_buffer_place_cursor(textbuff, &cursor);
        }
    } else {
        int nb_tokens = i;
        char tc_str_out[13];
        for (i = 0; i < 4; i++)
        {
            int field = i < nb_tokens ? atoi(tokens[i]) : 0;
            if (field < 0 || field > 59)
                field = 0;
            snprintf(tc_str_out + 3 * i, 4, "%02d:", field);
        }
        tc_str_out[11] = 0;
        gtk_text_buffer_set_text(textbuff, tc_str_out, -1);
        gtk_timecode_tag_field_separator(textbuff);
    }
    g_free(tc_str);
}

/* ----------------------------------------------
 * gtk_timecode_valid_delayed
 * ===============================
 * Helpers for using gtk_timecode_valid() through g_idle_add_once().
 */
struct gtk_timecode_valid_args {
    GtkTextBuffer *textbuff;
    int cursor_move;
};
void gtk_timecode_valid_gsource(gpointer data) {
    struct gtk_timecode_valid_args *args = data;

    gtk_timecode_valid(args->textbuff, args->cursor_move);
}
void gtk_timecode_valid_delayed(GtkTextBuffer *textbuff, int cursor_move)
{
    struct gtk_timecode_valid_args *args;

    args = g_slice_alloc(sizeof(*args));
    args->textbuff = textbuff;
    args->cursor_move = cursor_move;
    g_idle_add_once(gtk_timecode_valid_gsource, args);
}

/* ----------------------------------------------
 * signal_handler_timecode_changed
 * ===============================
 * Signal handler for GtkTextBuffer holding a Timecode value.
 */
void signal_handler_timecode_changed(gpointer user_data, GObject *gobj)
{
    GtkTextBuffer *textbuff = GTK_TEXT_BUFFER(gobj);

    gtk_timecode_valid_delayed(textbuff, 0);
}

/* ----------------------------------------------
 * signal_handler_timecode_move_cursor
 * ===================================
 * Signal handler for GtkTextView holding a GtkTextBuffer/Timecode value.
 */
void signal_handler_timecode_move_cursor(gpointer user_data, GObject *gobj, int cursor_move)
{
    char *textbuff_id = (char *)user_data;
    GtkTextBuffer *textbuff = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, textbuff_id));

    assert(textbuff != NULL);
    gtk_timecode_valid_delayed(textbuff, cursor_move);
}
