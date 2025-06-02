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


#include <sys/stat.h>
#include <pthread.h>
#include <libxml/parser.h>

#include "gtk-wrap.h"

short VERBOSE = 0;
short RUNNING = 1;

char *appname;
char *fpipeout = NULL;
char *fpipein = NULL;


/*
 * Set gtk controls from shell environment variables
 */
static void reader_getenv() {
    int i;

    for(i = 0; i < xml_obj_count; i++) {
        GObject *gobj = gtk_builder_get_object(builder, xml_obj_name[i]);
        char *obj_value = getenv((const char *)xml_obj_name[i]);

        if (obj_value != NULL)
            gtk_set_text(gobj, obj_value);
    }
}


static void *reader_loop(void* wojd){
    FILE *filein, *fileout;
    char input[2*STRING_SIZE] = "";
    struct gtkwrap_command_args *args;

    if (fpipeout) {
        mkfifo(fpipeout, S_IRWXU);
        fileout = fopen(fpipeout, "a+");
        if(!fileout){
            fprintf(stderr, "Error opening pipe %s !\n", fpipeout);
            pthread_exit(NULL);
        }
    } else fileout = stdout;

    mkfifo(fpipein, S_IRWXU);
    filein = fopen(fpipein, "r+");
    if(!filein){
        fprintf(stderr, "Error opening pipe %s !\n", fpipein);
        pthread_exit(NULL);
    }

    if(VERBOSE)
        fprintf(stderr, "Using pipes out:%s in:%s\n", fpipeout ? fpipeout : "-", fpipein);

    reader_getenv();

    while(RUNNING){
        if (!fgets(input, sizeof(input), filein))
            break;
        if (input[strlen(input)-1] != '\n') {
            fprintf(stderr, "Overly long command, exiting...\n");
            break;
        }

        if(!RUNNING)
            break;

        args = g_slice_alloc(sizeof(*args));
        args->fileout = fileout;
        args->VERBOSE = VERBOSE;
        args->input = g_malloc(strlen(input) + 1);
        strcpy(args->input, input);
        g_idle_add_once(gtkwrap_command, args);
    }

    fclose(filein);
    if (fpipeout) {
        fflush(fileout);
        fclose(fileout);
    }
    if (RUNNING)
        gtk_main_quit();
    pthread_exit(NULL);
}

static void usage(){
    fprintf(stderr,
        "Usage:\n"
        "%s \n"
        "\n"
        "Options:\n"
        "-f project.glade\n"
        "-m OBJECTNAME \t\t Set object as a main window. Default \"window1\".\n"
        "-s project.css \t\t Set a global style sheet.\n"
        "-v \t\t\t Be more verbose.\n"
        "-i INPIPENAME \t\t Use pipe for commands instead of standard input.\n"
        "-o OUTPIPE \t\t Use pipe for commands output.\n"
        "\n"
        "Commands:\n"
        "  exit\n"
        "  <id>|all reset: restore state from glade source file\n"
        "  <id> show true|false: show/hide a widget\n"
        "  <id> enable true|false: set widget sensitive yes/no\n"
        "  <id> name <name>: set widget name (usefull with css)\n"
        "  <id> set|get <text>: set/get value\n"
        "    For booleans (Buttons etc.), use 'true' 'false'.\n"
        "    For floats (GtkAdjustment), use '%%.2f'.\n"
        "    Supported for:" GTK_GETSET_TEXT_IMPLEMENTED
        "  For multiline support in GtkTextView, '\\n' is escaped to '|'.\n"
        "  Moreover, in case of a 'terminal' input-purpose, content is uuencoded.\n"
        "  Sample code: <id> set $(uuencode -m <<< \"$value\" -|tr '\\n' '|')\n"

    , appname);

    exit(1);
}

void on_window_destroy(GObject *object, gpointer user_data)
{
    if(VERBOSE)
        fprintf(stderr, "Caught destroy signal from main widget!\n"
                        "Quitting...\n");

    RUNNING = 0;
    gtk_main_quit();
}


int main(int argc, char *argv[])
{
    char *filename = NULL;
    char *cssfilename = NULL;
    char *main_object = (char*)"window1";
    int argn;
    int ret = 0;
    pthread_t thread = 0;
    GObject *window;
    GError *error = NULL;

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    appname = argv[0];

    for (argn = 1; argn < argc; argn++)
    {
        if(strlen(argv[argn]) < 2 )
            continue;

        if(argv[argn][0] == '-'){
            switch(argv[argn][1]){

                //load object as a main widget, default window1
                case 'm' :
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        main_object = argv[++argn];
                    continue;

                //verbose
                case 'v':
                    VERBOSE = 1;
                    continue;

                //command output pipe
                case 'o':
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        fpipeout = argv[++argn];
                    continue;

                //command input pipe
                case 'i':
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        fpipein = argv[++argn];
                    continue;

                //stylesheet (css) to apply globally
                case 's':
                    if(cssfilename != NULL)
                        usage();

                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        cssfilename = argv[++argn];
                    continue;

                //read ui from GtkBuilder(Glade) file
                case 'f':
                    if(filename != NULL)
                        usage();

                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        filename = argv[++argn];
                    continue;

                default:
                    usage();
                    break;
            }
        }
        break;
    }

    if(!filename)
        usage();
    g_set_prgname(g_path_get_basename(filename));

    if(VERBOSE)
        fprintf(stderr, "Loading widget \"%s\" as a main window.\n", main_object);

    argv = &argv[argn];
    argc -= argn;

    gtk_init(&argc, &argv);

    if (cssfilename) {
        GtkCssProvider *GlobalCssProvider = gtk_css_provider_new ();
        gtk_css_provider_load_from_path(GlobalCssProvider, cssfilename, &error);
        if (error) {
            fprintf(stderr, "Error occured while loading CSS!\n");
            fprintf(stderr, "Message: %s\n", error->message);
            g_free(error);
            return 1;
        }
        gtk_style_context_add_provider_for_screen(
                gdk_screen_get_default (),
                (GtkStyleProvider *)GlobalCssProvider,
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    builder = gtk_builder_new();

    if (!gtk_builder_add_from_file(builder, filename, &error))
    {
        fprintf(stderr, "Error occured while loading UI!\n");
        fprintf(stderr, "Message: %s\n", error->message);
        g_free(error);
        return 1;
    }

    window = gtk_builder_get_object(builder, main_object);
    if (!window || !GTK_IS_WINDOW(window)) {
        fprintf(stderr, "window '%s' not found\n", main_object);
        g_free(error);
        return 1;
    }


    //Adding default closing signal
    g_signal_connect_swapped(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    //Adding other signals
    if ((ret = parse_glade(filename, VERBOSE)) != 0)
        goto end;

    gtk_widget_show(GTK_WIDGET(window));

    //starting command reader
    if (fpipein)
        pthread_create(&thread, NULL, reader_loop, NULL);

    gtk_main();

end:
    RUNNING = 0;

    if (fpipein && thread)
        pthread_cancel(thread);

    g_object_unref(G_OBJECT(builder));

    if(VERBOSE)
        fprintf(stderr, "Cleaning...\n");

    if (fpipeout)
        unlink(fpipeout);
    unlink(fpipein);
    xml_obj_free();

    return ret;
}
