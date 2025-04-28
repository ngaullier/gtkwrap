gtkwrap
=======

GTK gui in bash.

## Build / Install

### prerequisites

`sudo apt-get install libgtk-3-dev libxml2-dev`

### make

`make all strip`

### build debian package

`fpm` is required, see [Installation &mdash; fpm - packaging made simple 1.9.0 documentation](https://fpm.readthedocs.io/en/latest/installation.html)

`git` is used for the versionning, `dpkg-dev` for dependencies analysis, and `lintian` is recommanded for final checking.

`sudo apt-get install git dpkg-dev lintian`

`make deb`

### install package

`sudo apt-get install ./gtk-wrap-*`  

## Usage

Create Your gui in Glade(GtkBuilder) and use it in your shell scripts.

EXAMPLE 1:

1) create Your GUI in Glade, name signal handler You're going to use

2) create Your bash script, name functions the same as signal handlers in Your Glade project
   
   ```shell
      #!/bin/bash
   
      on_button1_clicked(){
        #do something on button clicked signal
      }
   
      ./gtk_wrap -f a.glade | while read line
      do
        eval $line
      done
   ```

EXAMPLE 2:
Simple calculator

1) create Glade project with 3 textview widgets and one button

2) name button clicked signal as on_button1_clicked
   
   ```shell
     #!/bin/bash
   
     on_button1_clicked(){
        echo "textview1 get"
        echo "textview2 get"
        read a
        read b
        c=$((a + b))
        echo "textview3 set $c"
     } > inpipe < outpipe
   
     ./gtk_wrap -f b.glade -i inpipe -o outpipe | while read line
     do
        eval $line
     done
   ```

or, simplified using automatic variable assignment feature:

```shell
  #!/bin/bash

  on_button1_clicked(){
     echo "textview3 set $((textview1+textview2))"
  } > inpipe

  ./gtk_wrap -f b.glade -i inpipe | while read line
  do
     eval $line
  done
```

Full examples including glade files included in the [DEMO](DEMO) folder.
