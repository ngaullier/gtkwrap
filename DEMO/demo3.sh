#! /usr/bin/env bash
# shellcheck disable=SC2154

#{{{ Bash / Glade / Generic
set -o errexit
set -o nounset
set -o pipefail
script_name=$(basename "${0}")
script_id="${script_name%.*}"
script_dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

glade_file="${script_dir}/${script_id}.glade"
glade_env="${script_dir}/${script_id}.save"
glade_prefix="gui_"

gtkwrap_bin="${script_dir}/../gtk-wrap"
gtkwrap_cmd="/tmp/${0}.${$}.in"

main() {
    "${gtkwrap_bin}" -i "${gtkwrap_cmd}" -f "${glade_file}" |
    while read -r line
    do
        eval "$line"
    done
}

gui_cmd() {
    echo "$*"
} > "${gtkwrap_cmd}"

gui_state_save() {
    env|awk -F= '/^'${glade_prefix}'/ {                 \
            name=$1; sub(/[^=]*=/, "");             \
            printf("export %s=\"%s\"\n", name, $0); \
        }'                                          \
        > "${glade_env}"
}
gui_state_load() {
    # shellcheck disable=SC1090,SC2015
    [[ -f ${glade_env} ]] && . "${glade_env}" ||:
}
#}}}

refresh_button_play() {
    if [[ ${gui_stack1} == "page1" && -z ${gui_file_chooser:-}   \
       || ${gui_stack1} == "page2" && -z ${gui_folder_chooser:-} ]]
    then
        gui_cmd "button_play" enable false
    else
        gui_cmd "button_play" enable true
    fi
}

on_gui_switch1_state_set() {
    gui_cmd "gui_search1" show "${gui_switch1}"
}
on_button_reset_clicked() {
    gui_cmd "all" reset
}

on_button_play_clicked() {

    printf "\nFlags: "
    for gobj in "switch1" "toggle1" "check1" "radio1" "radio2"
    do
        eval "\$gui_$gobj" && printf "%s " "$gobj"
    done
    printf "\n"
    for gobj in "combo1" "spin1" "scale1" "label1" "search1"
    do
        eval "echo $gobj=\${gui_$gobj:-undefined}"
    done

    gui_textview1+="|appended-line"
    gui_cmd "gui_textview1" set "${gui_textview1}"


    if [[ ${gui_stack1} == "page1" ]];
    then
        gui_cmd "window1" set "file=${gui_file_chooser}"
    else
        gui_cmd "window1" set "folder=${gui_folder_chooser}"
    fi

    gui_state_save
}

gui_state_load
main
