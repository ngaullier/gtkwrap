#!/bin/bash

script_name=$(basename "${0}")
script_id="${script_name%.*}"
script_dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

glade_file="${script_dir}/${script_id}.glade"
gtkwrap_bin="gtk-wrap"
gtkwrap_cmd="/tmp/${script_name}.${$}.in"
gtkwrap_out="/tmp/${script_name}.${$}.out"

main() {
    "${gtkwrap_bin}" -v -i "${gtkwrap_cmd}" -o "${gtkwrap_out}" -f "${glade_file}" |
    while read -r line
    do
        eval "$line"
    done
}

on_button1_clicked(){

    echo "textview1 get"
    echo "textview2 get"
    read -r a
    read -r b
    c=$((a + b))
    echo "textview3 set $c"

} > "${gtkwrap_cmd}" < "${gtkwrap_out}"

main
