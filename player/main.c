#include <gtk/gtk.h>
#include <locale.h>
#include <getopt.h>
#include <libgen.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libintl.h>

#include "utils.h"
#include "db_utils.h"
#include "gtk_utils.h"

void mpd_idle(gpointer data)
{
    mpd_poll();
    g_main_context_wakeup(NULL);
}

static void player_idle(gpointer data)
{
    gtk_poll();
    g_main_context_wakeup(NULL);
}

//static void streamripper_idle(gpointer data)
//{
//	printf("*");
//}

int main (int argc, char *argv[])
{
    gchar *path;
    guint hndl_id0, hndl_id1;
//    char *lang;
    int opt;
    int longopt_index;
    setlocale (LC_ALL, "");//lang);
    static struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "h:v", long_options, &longopt_index)) > 0)
    {
        switch (opt)
        {
            case 'v':
                printf ("%s %s\n%s\n", g_ascii_strdown(APPNAME,strlen(APPNAME)),
                        VERSION,
                        "Copyright 2015 Roman Borisov");
                goto cleanup;
                break;
            case 'h':
            default:
                fprintf(stderr, "usage: %s [options]\n", basename(argv[0]));
                fprintf(stderr, "  -h, --help           display this help\n");
                fprintf(stderr, "  -v, --version            version information\n");
                return 1;
        }
    }

    gdk_threads_init ();

    gtk_init (&argc, &argv);
    gtk_app_init();
//    gtk_window_fullscreen(GDK_WINDOW(gtk.main_window));
    gtk_widget_show (gtk.main_window);

    db_init();

    gdk_threads_enter ();
	hndl_id0 = g_idle_add((GtkFunction)mpd_idle, NULL);
    hndl_id1 = g_idle_add((GtkFunction)player_idle, NULL);
    gtk_main ();
    gtk_idle_remove(hndl_id0);
    gtk_idle_remove(hndl_id1);
    gdk_threads_leave ();

    db_close();
cleanup:
    

    return 0;
}
