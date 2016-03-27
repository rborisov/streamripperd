#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "config.h"
#include "gtk_utils.h"
#include "mpd_utils.h"
#include "db_utils.h"
#include "utils.h"

GtkBuilder  *xml = NULL;

static void ui_song_rating_update(int rating)
{
    GtkWidget *label = NULL;
    gchar *str;
    label = GTK_WIDGET (gtk_builder_get_object (xml, "lbl_rating"));
    str = g_strdup_printf ("%d", rating);
    gtk_label_set (GTK_LABEL (label), str);
    g_free(str);
}

static void ui_queue_update(int songpos, int queue_length)
{
    GtkWidget *label = NULL;
    gchar *str;
    label = GTK_WIDGET (gtk_builder_get_object (xml, "lbl_queue_length"));
    str = g_strdup_printf ("%d(%d)", songpos, queue_length);
    gtk_label_set (GTK_LABEL (label), str);
    g_free(str);
}

static void cb_prev_button_clicked (G_GNUC_UNUSED GtkWidget *widget,
        G_GNUC_UNUSED gpointer   data)
{
    printf("%s\n", __func__);
    mpd_prev();
    return;
}

static void cb_next_button_clicked (G_GNUC_UNUSED GtkWidget *widget,
        G_GNUC_UNUSED gpointer  data)
{
    printf("%s\n", __func__);
    mpd_db_update_current_song_rating(-1);
    mpd_next();
    return;
}

static void cb_play_button_clicked (G_GNUC_UNUSED GtkWidget *widget,
        G_GNUC_UNUSED gpointer   data)
{
    printf("%s\n", __func__);
    mpd_toggle_play();
    return;
}

static void cb_vol_inc_button_clicked (G_GNUC_UNUSED GtkWidget *widget,
                G_GNUC_UNUSED gpointer   data)
{
    printf("%s\n", __func__);
    mpd_change_volume(5);
    return;
}
static void cb_vol_dec_button_clicked (G_GNUC_UNUSED GtkWidget *widget,
                G_GNUC_UNUSED gpointer   data)
{
    printf("%s\n", __func__);
    mpd_change_volume(-5);
    return;
}
static void cb_like_button_clicked (G_GNUC_UNUSED GtkWidget *widget,
                G_GNUC_UNUSED gpointer   data)
{
    printf("%s\n", __func__);
    ui_song_rating_update(mpd_db_update_current_song_rating(5));
    return;
}
static void cb_radio_button_clicked (G_GNUC_UNUSED GtkWidget *widget,
                G_GNUC_UNUSED gpointer   data)
{
    printf("%s\n", __func__);
    return;
}
static void cb_crop_button_clicked (G_GNUC_UNUSED GtkWidget *widget,
                G_GNUC_UNUSED gpointer   data)
{
    printf("%s %i\n", __func__, mpd_get_queue_length());
    mpd_crop();
    printf("%s %i\n", __func__, mpd_get_queue_length());
    return;
}
static void cb_dislike_button_clicked (G_GNUC_UNUSED GtkWidget *widget,
                G_GNUC_UNUSED gpointer   data)
{
    printf("%s\n", __func__);
    mpd_db_update_current_song_rating(-5);
    mpd_next();
    return;
}

static gboolean cb_key_press (G_GNUC_UNUSED GtkWidget *widget,
        GdkEventKey *event,
        G_GNUC_UNUSED gpointer userdata)
{
    gboolean result = FALSE;

    if (event->type == GDK_KEY_PRESS) {
        switch (event->keyval) {
            case GDK_KEY_b: /* NEXT */
                cb_next_button_clicked (NULL, NULL);
                result = TRUE;
                break;
            case GDK_KEY_c: /* PLAY/PAUSE */
                cb_play_button_clicked (NULL, NULL);
                result = TRUE;
                break;
            case GDK_KEY_z: /* PREV */
                cb_prev_button_clicked (NULL, NULL);
                result = TRUE;
                break;
        }
    }
    return result;
}

void gtk_win_bg(char* file)
{
    GdkPixmap *background;
    GdkPixbuf *pixbuf, *pixbuf1;
    GtkStyle *style;
    gint width, height;
    gchar *path;
    GError* error = NULL;

    if (file) {
        path = g_strdup_printf ("%s/%s", IMAGEPATH, file);
    } else {
        path = g_strdup_printf ("%s/bkbg.jpg", PREFIX);
    }
    gtk_window_get_size(gtk.main_window, &width, &height);
    printf("bg_image: %s; window size: %ix%i\n", path, width, height);
    pixbuf = gdk_pixbuf_new_from_file (path,&error);
    if (error != NULL) {
        if (error->domain == GDK_PIXBUF_ERROR) {
            g_print ("Pixbuf Related Error:\n");
        }
        if (error->domain == G_FILE_ERROR) {
            g_print ("File Error: Check file permissions and state:\n");
        }

        g_printerr ("%s\n", error[0].message);
    } else {
        pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, width, GDK_INTERP_BILINEAR); //GDK_INTERP_TILES); //GDK_INTERP_NEAREST);
        gdk_pixbuf_render_pixmap_and_mask (pixbuf, &background, NULL, 0);
        style = gtk_style_new ();
        style->bg_pixmap[0] = background;
        gtk_widget_set_style (GTK_WIDGET(gtk.main_window), GTK_STYLE(style));
    }
    g_free(path);
}

void gtk_app_init(void)
{
    GdkColor color;
    GError* error = NULL;
    gchar *path;

    init_utils();

    gdk_color_parse("black", &color);

    path = g_strdup_printf ("%s/%s", PREFIX, UI_FILE);
    xml = gtk_builder_new ();
    if (!gtk_builder_add_from_file (xml, path, &error)) {
        g_error (_("Failed to initialize interface: %s"), error->message);
        g_error_free(error);
        goto cleanup;
    }
    g_free (path);

    gtk_builder_connect_signals (xml, NULL);

    gtk.main_window = GTK_WIDGET (gtk_builder_get_object (xml, "window1"));
    gtk_widget_modify_bg(GTK_WIDGET(gtk.main_window), GTK_STATE_NORMAL, &color);
    
    g_signal_connect (G_OBJECT(gtk.main_window), "key-press-event", G_CALLBACK(cb_key_press), NULL);

    GtkWidget *button;
    button = GTK_WIDGET (gtk_builder_get_object (xml, "btn_prev"));
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_PRELIGHT, &color);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_prev_button_clicked), NULL);
    button = GTK_WIDGET (gtk_builder_get_object (xml, "btn_play"));
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_PRELIGHT, &color);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_play_button_clicked), NULL);
    button = GTK_WIDGET (gtk_builder_get_object (xml, "btn_next"));
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_PRELIGHT, &color);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_next_button_clicked), NULL);
    button = GTK_WIDGET (gtk_builder_get_object (xml, "btn_vol_inc"));
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_PRELIGHT, &color);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_vol_inc_button_clicked), NULL);
    button = GTK_WIDGET (gtk_builder_get_object (xml, "btn_vol_dec"));
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_PRELIGHT, &color);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_vol_dec_button_clicked), NULL);
    button = GTK_WIDGET (gtk_builder_get_object (xml, "btn_like"));
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_PRELIGHT, &color);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_like_button_clicked), NULL);
    button = GTK_WIDGET (gtk_builder_get_object (xml, "btn_radio"));
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_PRELIGHT, &color);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_radio_button_clicked), NULL);
    button = GTK_WIDGET (gtk_builder_get_object (xml, "btn_crop"));
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_PRELIGHT, &color);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_crop_button_clicked), NULL);
    button = GTK_WIDGET (gtk_builder_get_object (xml, "btn_dislike"));
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(button), GTK_STATE_PRELIGHT, &color);
    g_signal_connect(button, "clicked", G_CALLBACK(cb_dislike_button_clicked), NULL);

    gtk_win_bg(NULL);

    gtk.song_id = -555; //magic number to show song at beginning
    gtk.queue_version = 0;
cleanup:
    return;
}

void gtk_poll(void)
{
    GtkWidget *label = NULL, *label1 = NULL, *label2 = NULL, *label3 = NULL,
              *label4 = NULL, *image0;
    gchar *title = NULL, *artist = NULL, *album = NULL;
    gchar time[11] = "00:00/00:00";
    int minutes_elapsed, minutes_total;
    gchar *str, *artist_art, *album_art = NULL;
   
    ui_queue_update(mpd.song_pos+1, mpd.queue_len);

    if (mpd.song_id != gtk.song_id) {
        /*
         * track artist album
         */
        title = mpd_get_current_title();
        if (title) {
            printf("%s %d %d\n", title, mpd.song_id, gtk.song_id);
            label = GTK_WIDGET (gtk_builder_get_object (xml, "lbl_track"));
            gtk_label_set (GTK_LABEL (label), title);
        }
        artist = mpd_get_current_artist();
        if (artist) {
            printf("%s\n", artist);
            label1 = GTK_WIDGET (gtk_builder_get_object (xml, "lbl_artist"));
            gtk_label_set (GTK_LABEL (label1), artist);
            artist_art = db_get_artist_art(artist);
       
            //impossible to have album without artist
            album = get_current_album();
            label2 = GTK_WIDGET (gtk_builder_get_object (xml, "lbl_album"));
            if (album) {
                printf("%s\n", album);
                gtk_label_set (GTK_LABEL (label2), album);
                album_art = db_get_album_art(artist, album);
            } else {
                gtk_label_set (GTK_LABEL (label2), "");
            }
            /*
             *          * artist art
             *                   */
            image0 = GTK_WIDGET (gtk_builder_get_object (xml, "img_artist"));
            if (artist_art) {
                printf("art: %s\n", artist_art);
                artist_art = g_strdup_printf("%s/%s", IMAGEPATH, artist_art);
            } else {
                artist_art = g_strdup_printf("%s/art.png", IMAGEPATH);
            }
            gtk_image_set_from_file(GTK_IMAGE (image0), artist_art);

            /*
             * album art
             */
            gtk_win_bg(album_art);
        }

        ui_song_rating_update(mpd_db_get_current_song_rating());

        /*
         * notification
         */
        if (title && artist) {
            /*
             * update DB with increased num played
             */
            int np, rating;
            np = db_listen_song(title, artist, album);
            rating = mpd_db_get_current_song_rating();
            notify_init (title);
            if (album)
                artist= g_strdup_printf("%i %i\n%s\n%s", rating, np, artist, album);
            else
                artist= g_strdup_printf("%i %i\n%s", rating, np, artist);
            gtk.TrackNotify = notify_notification_new (title, artist, artist_art);
            notify_notification_show (gtk.TrackNotify, NULL);
            g_object_unref(G_OBJECT(gtk.TrackNotify));
            notify_uninit();
        }

        gtk.song_id = mpd.song_id;
    }

    /*
     * time elapsed / total
     * */
/*    minutes_elapsed = mpd.elapsed_time/60;
    minutes_total = mpd.total_time/60;
    sprintf(time, "%02d:%02d/%02d:%02d", minutes_elapsed, mpd.elapsed_time - minutes_elapsed*60,
            minutes_total, mpd.total_time - minutes_total*60);*/
    str = g_strdup_printf ("%02i:%02i/%02i:%02i", mpd.elapsed_time / 60, mpd.elapsed_time % 60,
            mpd.total_time / 60, mpd.total_time % 60);
    label3 = GTK_WIDGET (gtk_builder_get_object (xml, "lbl_time"));
    gtk_label_set (GTK_LABEL (label3), str);
    g_free(str);

    /*
     * volume
     * */
    label4 = GTK_WIDGET (gtk_builder_get_object (xml, "lbl_volume"));
    str = g_strdup_printf ("%d", mpd.volume);
    gtk_label_set (GTK_LABEL (label4), str);
    g_free(str);

}
