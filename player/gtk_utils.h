#ifndef __GTK_UTILS_H__
#define __GTK_UTILS_H__

#include <libintl.h>
#include <libnotify/notify.h>
#include "config.h"

#define UI_FILE "player.glade"
#define APPNAME "player"
/*#define PREFIX "../assets"*/
#define VERSION "0.1"

#define _(String) gettext (String)

struct t_gtk {
    GtkWidget    *main_window;
    NotifyNotification * TrackNotify;
    int song_id;
    unsigned queue_version;
} gtk;

void gtk_poll(void);
void gtk_app_init(void);

#endif
