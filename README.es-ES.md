

gtkwrap
=======

Interfaz gráfica de GTK en bash.

## Compilación / Instalación

### Requisitos previos

`sudo apt-get install libgtk-3-dev libxml2-dev`



### Compilar e instalar localmente

`make all strip`

Cuando no se utiliza el paquete de Debian para instalar (véase más abajo), una instalación local típica se puede lograr muy simplemente de la siguiente manera:

`sudo ln -fs $(pwd)/gtk-wrap /usr/local/bin/gtk-wrap`

### Compilar el paquete de Debian

`fpm` es necesario, consulte [Installation &mdash; fpm - packaging made simple 1.9.0 documentation](https://fpm.readthedocs.io/en/latest/installation.html)

`git` se utiliza para la gestión de versiones, `dpkg-dev` para el análisis de dependencias y se recomienda `lintian` para la verificación final.

`sudo apt-get install git dpkg-dev lintian`

`make deb`

## Uso

Crea tu interfaz gráfica en Glade (GtkBuilder) y úsala en tus scripts de shell.

EJEMPLO 1:

1) Crea tu interfaz gráfica en Glade, asigna un nombre al controlador de señal que vas a utilizar

2) Crea tu script de bash, asigna a las funciones el mismo nombre que a los controladores de señal en tu proyecto de Glade
   
   ```shell
      #!/bin/bash
   
      on_button1_clicked(){
        #haz algo cuando se active la señal del clic del botón
      }
   
      gtk_wrap -f a.glade | while read line
      do
        eval $line
      done
   ```

EJEMPLO 2:
Calculadora simple

1) Crea un proyecto de Glade con 3 widgets de tipo textview y un botón

2) asigna a la señal de clic del botón el nombre on_button1_clicked
   
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
   
     gtk_wrap -f b.glade -i inpipe -o outpipe | while read line
     do
        eval $line
     done
   ```

o, de forma simplificada, utilizando la función de asignación automática de variables:

```shell
  #!/bin/bash

  on_button1_clicked(){
     echo "textview3 set $((textview1+textview2))"
  } > inpipe

  gtk_wrap -f b.glade -i inpipe | while read line
  do
     eval $line
  done
```

Los ejemplos completos, incluidos los archivos glade, se encuentran en la carpeta [DEMO](DEMO).
