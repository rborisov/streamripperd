#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "db_utils.h"
#include "mpd_utils.h"

void get_random_song(char *str, char *path)
{
    struct mpd_entity *entity;
    int listened0 = 65000,
        skipnum, numberofsongs = 0;

    struct mpd_stats *stats = mpd_run_stats(mpd.conn);
    if (stats == NULL)
        return;
    numberofsongs = mpd_stats_get_number_of_songs(stats);
    mpd_stats_free(stats);
    skipnum = rand() % numberofsongs;

    syslog(LOG_DEBUG, "%s: path %s; number of songs: %i skip: %i\n",
            __func__, path, numberofsongs, skipnum);
    if (!mpd_send_list_all_meta(mpd.conn, ""))//path))
    {
        syslog(LOG_ERR, "%s: error: mpd_send_list_meta %s\n", __func__, path);
        return;
    }

    while((entity = mpd_recv_entity(mpd.conn)) != NULL)
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
//                syslog(LOG_DEBUG, "listened: %i ", listened);
                int probability = 50 - listened +
                    db_get_song_rating(mpd_get_title(song),
                            mpd_get_artist(song));
//                syslog(LOG_DEBUG, "probability: %i ", probability);
                bool Yes = (rand() % 100) < probability;
                if (Yes) {
                    sprintf(str, "%s", mpd_song_get_uri(song));
                    syslog(LOG_DEBUG, "probability: %i; uri: %s ", probability, str);
//                    syslog(LOG_DEBUG, "title: %s ", mpd_get_title(song));
//                    syslog(LOG_DEBUG, "artist: %s", mpd_get_artist(song));
                }
            }
        }
        mpd_entity_free(entity);
    }
}

struct memstruct {
    char *memory;
    size_t size;
};
struct memstruct albumstr;
static size_t WriteMemory(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct memstruct *mem = (struct MemoryStruct *)userp;

    printf("%i %i\n", size, nmemb);

    mem->memory = realloc(mem->memory, realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(mem->memory, contents, realsize);
    mem->size = realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}
void init_utils()
{
    albumstr.memory = malloc(1);
    albumstr.size = 0;
}
void clean_utils()
{
    free(albumstr.memory);
}

char* get_current_album()
{
    const struct mpd_song *song;
    char *str = NULL;
    if (mpd.conn_state == MPD_CONNECTED) {
        song = mpd_run_current_song(mpd.conn);
        if(song == NULL) {
            printf("song == NULL\n");
            return NULL;
        }
        str = (char *)mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
        if (str == NULL) {
            str = db_get_song_album(mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
                    mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
            printf("%s: no MPD_TAG_ALBUM\n", __func__);
        }
        if (str) {
            WriteMemory(str, 1, strlen(str), (void *)&albumstr);
            str = albumstr.memory;
            printf("%s: %s\n", __func__, albumstr.memory);
        }

        mpd_song_free(song);
    }

    return str;
}

int mpd_db_get_current_song_rating()
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

int mpd_db_update_current_song_rating(int increase)
{
    int rating = 0;
    struct mpd_song *song;

    song = mpd_run_current_song(mpd.conn);
    if(song == NULL)
        return 0;

    rating = db_update_song_rating(mpd_get_title(song),
            mpd_song_get_tag(song, MPD_TAG_ARTIST, 0), increase);

    mpd_song_free(song);

    return rating;
}

void mpd_poll()
{
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
            if (mpd.song_pos+1 >= mpd.queue_len)
            {
                char str[128] = "";
                syslog(LOG_DEBUG, "%s: queue is empty %i(%i)\n", __func__, mpd.song_pos, mpd.queue_len);
                get_random_song(str, "");
                if (strlen(str) > 5)
                if (!mpd_run_add(mpd.conn, str)) {
                    syslog(LOG_ERR, "%s: %s", __func__, mpd_connection_get_error_message(mpd.conn));
                }
            }

            break;
        default:
            syslog(LOG_INFO, "%s - mpd.conn_state %i\n", __func__, mpd.conn_state);
    }
}

