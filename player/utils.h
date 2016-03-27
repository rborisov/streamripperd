#ifndef __UTILS_H__
#define __UTILS_H__

#include "mpd_utils.h"

void init_utils();
void clean_utils();
void get_random_song(char *, char *);
void mpd_poll();
char* get_current_album();
int mpd_db_update_current_song_rating(int);
int mpd_db_get_current_song_rating();

#endif
