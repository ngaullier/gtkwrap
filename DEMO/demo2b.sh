#! /usr/bin/env bash
# shellcheck disable=SC2154

# same as demo2.sh but more easy/compact

GLADE_FILE="demo2.glade"

IN="/tmp/${0}.${$}.in"

GTK_WRAP="../gtk-wrap -v -f $GLADE_FILE -i $IN"


on_button1_clicked(){
    echo "textview3 set_textview_text $((textview1+textview2))"
} > "$IN"


$GTK_WRAP | while read -r line
do
    eval "$line"
done
