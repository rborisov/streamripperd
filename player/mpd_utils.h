#ifndef __MPD_UTILS_H__
#define __MPD_UTILS_H__

#include <mpd/client.h>

enum mpd_conn_states {
    MPD_DISCONNECTED,
    MPD_FAILURE,
    MPD_CONNECTED,
    MPD_RECONNECT,
    MPD_DISCONNECT
};

struct t_mpd {
    struct mpd_connection *conn;
    enum mpd_conn_states conn_state;

    int song_pos;
    int next_song_pos;
    int song_id;
    unsigned queue_version;
    unsigned queue_len;
    int volume;
    enum mpd_state state;
    bool repeat;
    bool single;
    bool consume;
    bool random;
    unsigned elapsed_time;
    unsigned total_time;

} mpd;

//void mpd_poll();

unsigned mpd_get_queue_length();
int mpd_insert ( char * );
int mpd_crop();
//void get_random_song( char *, char *);
int get_current_song_rating();
char* mpd_get_title(struct mpd_song const *song);
int mpd_list_artists();
char* mpd_get_artist(struct mpd_song const *song);
void get_song_to_delete(char *str);
char* mpd_get_album(struct mpd_song const *song);

void mpd_toggle_play(void);
void mpd_next(void);
void mpd_prev(void);
void mpd_change_volume(int val);

void mpd_put_state(void);

int mpd_delete_current_song();

char *mpd_get_current_title(void);
char *mpd_get_current_artist(void);
char *mpd_get_current_album(void);

#endif
