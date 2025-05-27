#!/bin/bash

script_name=$(basename "${0}")
script_id="${script_name%.*}"
script_dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

glade_file="${script_dir}/${script_id}.glade"
gtkwrap_bin="gtk-wrap"

main() {
    "${gtkwrap_bin}" -f "${glade_file}" |
    while read -r line
    do
        eval "$line"
    done
}

on_button1_clicked(){
    echo button1 clicked
}

on_togglebutton1_toggled(){
    echo toggled
}

on_window1_destroy(){
    echo window destroyed
}

main
