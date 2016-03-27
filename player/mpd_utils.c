#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <syslog.h>

#include "mpd_utils.h"

char nullstr[1] = "";
char titlebuf[128] = "";
char artistbuf[128] = "";
char albumbuf[128] = "";

int mpd_crop()
{
    struct mpd_status *status = mpd_run_status(mpd.conn);
    if (status == 0)
        return 0;
    int length = mpd_status_get_queue_length(status) - 1;

    if (length < 0) {
        mpd_status_free(status);
        syslog(LOG_INFO, "%s: A playlist longer than 1 song in length is required to crop.\n", __func__);
    } else if (mpd_status_get_state(status) == MPD_STATE_PLAY ||
            mpd_status_get_state(status) == MPD_STATE_PAUSE) {
        if (!mpd_command_list_begin(mpd.conn, false)) {
            syslog(LOG_ERR, "%s: mpd_command_list_begin failed\n", __func__);
            return 0;
        }

        for (; length >= 0; --length)
            if (length != mpd_status_get_song_pos(status))
                mpd_send_delete(mpd.conn, length);

        mpd_status_free(status);

        if (!mpd_command_list_end(mpd.conn) || !mpd_response_finish(mpd.conn)) {
            syslog(LOG_ERR, "%s: mpd_command_list_end || mpd_response_finish failed\n", __func__);
            return 0;
        }

        return 0;
    } else {
        mpd_status_free(status);
        syslog(LOG_INFO, "%s: You need to be playing to crop the playlist\n", __func__);
        return 0;
    }
    return 1;
}

int mpd_list_artists()
{
    int num = 0;

    mpd_search_db_tags(mpd.conn, MPD_TAG_ARTIST);
    if (!mpd_search_commit(mpd.conn)) {
        syslog(LOG_DEBUG, "%s: search_commit error\n", __func__);
        return 0;
    }

    struct mpd_pair *pair;
    while ((pair = mpd_recv_pair_tag(mpd.conn, MPD_TAG_ARTIST)) != NULL) {
        syslog(LOG_DEBUG, "%s: %s\n", __func__, pair->value);
        mpd_return_pair(mpd.conn, pair);
        num++;
    }

    if (!mpd_response_finish(mpd.conn)) {
        syslog(LOG_DEBUG, "%s: error\n", __func__);
        return 0;
    }

    syslog(LOG_DEBUG, "%s: found %i\n", __func__, num);
    return num;
}

char* mpd_get_current_title()
{
    struct mpd_song *song;

    if (mpd.conn_state == MPD_CONNECTED) {
        song = mpd_run_current_song(mpd.conn);
        if(song == NULL) {
            printf("song == NULL\n");
            return NULL;
        }
        sprintf(titlebuf, "%s", mpd_get_title(song));
        mpd_song_free(song);
    }
    
    return titlebuf;
}

char* mpd_get_current_artist()
{   
    struct mpd_song *song;

    if (mpd.conn_state == MPD_CONNECTED) {
        song = mpd_run_current_song(mpd.conn);
        if(song == NULL) {
            printf("song == NULL\n");
            return NULL;
        }
        sprintf(artistbuf, "%s", mpd_get_artist(song));
        mpd_song_free(song);
    }

    return artistbuf;
}

char* mpd_get_current_album()
{
    struct mpd_song *song;
    
    if (mpd.conn_state == MPD_CONNECTED) {
        song = mpd_run_current_song(mpd.conn);
        if(song == NULL) {
            printf("song == NULL\n");
            return NULL;
        }
        sprintf(albumbuf, "%s", mpd_get_album(song));
        mpd_song_free(song);
    }

    return albumbuf;
}

char* mpd_get_artist(struct mpd_song const *song)
{
    char *str;
    str = (char *)mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if(str == NULL) {
        str = basename((char *)mpd_song_get_uri(song));
        if  (str == NULL)
            str = nullstr;
    }

    return str;
}

char* mpd_get_album(struct mpd_song const *song)
{
    char *str;
    str = (char *)mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    if(str == NULL)
        str = nullstr; //TODO: get album from db

    return str;
}

char* mpd_get_title(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if(str == NULL){
        str = basename((char *)mpd_song_get_uri(song));
        if (str == NULL)
            str = nullstr;
    }

    return str;
}

unsigned mpd_get_queue_length()
{
    struct mpd_status *status = mpd_run_status(mpd.conn);
    if (status == NULL)
        return 0;
    const unsigned length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    return length;
}

int mpd_insert (char *song_path )
{
    struct mpd_status *status = mpd_run_status(mpd.conn);
    if (status == NULL)
        return 0;
    const unsigned from = mpd_status_get_queue_length(status);
    const int cur_pos = mpd_status_get_song_pos(status);
    mpd_status_free(status);

    if (mpd_run_add(mpd.conn, song_path) != true)
        return 0;

    /* check the new queue length to find out how many songs were
     *        appended  */
    const unsigned end = mpd_get_queue_length();
    if (end == from)
        return 0;

    /* move those songs to right after the current one */
    return mpd_run_move_range(mpd.conn, from, end, cur_pos + 1);
}
#if 0
void get_random_song(struct mpd_connection *conn, char *str, char *path)
{
    struct mpd_entity *entity;
    int listened0 = 65000,
        skipnum, numberofsongs = 0;

    struct mpd_stats *stats = mpd_run_stats(conn);
    if (stats == NULL)
        return;
    numberofsongs = mpd_stats_get_number_of_songs(stats);
    mpd_stats_free(stats);
    skipnum = rand() % numberofsongs;

    syslog(LOG_DEBUG, "%s: path %s; number of songs: %i skip: %i\n",
            __func__, path, numberofsongs, skipnum);
    if (!mpd_send_list_all_meta(conn, ""))//path))
    {
        syslog(LOG_ERR, "%s: error: mpd_send_list_meta %s\n", __func__, path);
        return;
    }

    while((entity = mpd_recv_entity(conn)) != NULL)
    {
        const struct mpd_song *song;
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG)
        {
            if (skipnum-- > 0)
                continue;

            int listened;
            song = mpd_entity_get_song(entity);
            listened = db_get_song_numplayed(mpd_get_title(song),
                    mpd_get_artist(song));
            if (listened < listened0) {
                listened0 = listened;
                syslog(LOG_DEBUG, "listened: %i ", listened);
                int probability = 50 +
                    db_get_song_rating(mpd_get_title(song),
                            mpd_get_artist(song));
                syslog(LOG_DEBUG, "probability: %i ", probability);
                bool Yes = (rand() % 100) < probability;
                if (Yes) {
                    sprintf(str, "%s", mpd_song_get_uri(song));
                    syslog(LOG_DEBUG, "uri: %s ", str);
                    syslog(LOG_DEBUG, "title: %s ", mpd_get_title(song));
                    syslog(LOG_DEBUG, "artist: %s", mpd_get_artist(song));
                }
            }
        }
        mpd_entity_free(entity);
    }
}

void get_song_to_delete(char *str)
{
    struct mpd_entity *entity;
    int rating0 = 65000;
    if (!mpd_send_list_meta(mpd.conn, "/")) {
        syslog(LOG_DEBUG, "error: mpd_send_list_meta\n");
        return;
    }
    while((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            const struct mpd_song *song = mpd_entity_get_song(entity);
            int rating = db_get_song_rating(mpd_get_title(song),
                    mpd_get_artist(song));
            if (rating < rating0) {
                rating0 = rating;
                sprintf(str, "%s", mpd_song_get_uri(song));
            }
        }
        mpd_entity_free(entity);
    }
}

int get_current_song_rating()
{
    int rating;
    struct mpd_song *song;

    song = mpd_run_current_song(mpd.conn);
    if(song == NULL)
        return 0;

    rating = db_get_song_rating(mpd_get_title(song), mpd_get_artist(song));

    mpd_song_free(song);
    return rating;
}

int mpd_delete_current_song(struct mpd_connection *conn)
{
    struct mpd_song *song;
    char *currentsonguri = NULL;

    song = mpd_run_current_song(conn);
    if(song == NULL)
        return 0;

    db_update_song_rating(mpd_get_title(song),
            mpd_song_get_tag(song, MPD_TAG_ARTIST, 0), -5);

    currentsonguri = mpd_song_get_uri(song);
    printf("%s: let's delete song: %s\n", __func__, currentsonguri);

    delete_file_forever(currentsonguri);

    mpd_song_free(song);

    return 1;
}
#endif
void mpd_toggle_play(void)
{
    if (mpd.state == MPD_STATE_STOP)
        mpd_run_play(mpd.conn);
    else
        mpd_run_toggle_pause(mpd.conn);
}

void mpd_next(void)
{
    mpd_run_next(mpd.conn);
}

void mpd_prev(void)
{
    mpd_run_previous(mpd.conn);
}

void mpd_change_volume(int val)
{
    int volume = mpd.volume + val;
    if (volume > 100)
        volume = 100;
    if (volume < 0)
        volume = 0;
    mpd_run_set_volume(mpd.conn, volume);
}

void mpd_put_state(void)
{
    struct mpd_status *status;
    int len;
    unsigned queue_len;
    int song_pos, next_song_pos;

    status = mpd_run_status(mpd.conn);
    
    if (!status) {
        syslog(LOG_ERR, "%s mpd_run_status: %s\n", __func__, mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return;
    }

    mpd.song_pos = mpd_status_get_song_pos(status);
    mpd.next_song_pos = song_pos+1; //TODO: mpd_status_get_next_song_pos(status);
    mpd.queue_len = mpd_status_get_queue_length(status);
    mpd.volume = mpd_status_get_volume(status);
    mpd.state = mpd_status_get_state(status);
    mpd.repeat = mpd_status_get_repeat(status);
    mpd.single = mpd_status_get_single(status);
    mpd.consume = mpd_status_get_consume(status);
    mpd.random = mpd_status_get_random(status);
    mpd.elapsed_time = mpd_status_get_elapsed_time(status);
    mpd.total_time = mpd_status_get_total_time(status);
    mpd.song_id = mpd_status_get_song_id(status);

//    printf("%d\n", mpd.song_id);

    mpd_status_free(status);
}
#if 0
void mpd_poll()
{
//    printf("%d\n", mpd.conn_state);
    switch (mpd.conn_state) {
        case MPD_DISCONNECTED:
            syslog(LOG_INFO, "%s - MPD Connecting...\n", __func__);
            mpd.conn = mpd_connection_new(NULL, NULL, 3000);
            if (mpd.conn == NULL) {
                syslog(LOG_ERR, "%s - Out of memory.", __func__);
                mpd.conn_state = MPD_FAILURE;
                return;
            }
            if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS) {
                syslog(LOG_ERR, "%s - MPD connection: %s\n", __func__,
                        mpd_connection_get_error_message(mpd.conn));
                mpd.conn_state = MPD_FAILURE;
                return;
            }
            syslog(LOG_INFO, "%s - MPD connected.\n", __func__);
            mpd_connection_set_timeout(mpd.conn, 10000);
            mpd.conn_state = MPD_CONNECTED;
            break;
        case MPD_FAILURE:
        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            syslog(LOG_ERR, "%s - MPD (dis)reconnect or failure\n", __func__);
            if(mpd.conn != NULL)
                mpd_connection_free(mpd.conn);
            mpd.conn = NULL;
            mpd.conn_state = MPD_DISCONNECTED;
            break;
        case MPD_CONNECTED:
            mpd_put_state();
            //TODO: display status
/*            if (queue_is_empty) {
                queue_is_empty = 0;
                get_random_song(mpd.conn, str, rcm.file_path);
                if (strcmp(str, "") != 0) {
                    syslog(LOG_DEBUG, "%s: add random song %s\n", __func__, str);
                    mpd_run_add(mpd.conn, str);
                }
            }*/
            break;
        default:
            syslog(LOG_INFO, "%s - mpd.conn_state %i\n", __func__, mpd.conn_state);
    }
}
#endif
