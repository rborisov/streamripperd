#ifndef __DB_UTILS_H__
#define __DB_UTILS_H__

int db_init();
void db_close();
int db_listen_song(char*, char*, char*);
int db_get_song_numplayed(char*, char*);
int db_get_album_id(char*, char*);
char* db_get_song_album(char*, char*);
char* db_get_album_art(char*, char*);
char* db_get_artist_art(char*);
int db_update_song_album(char*, char*, char*);
int db_update_album_art(char*, char*, char*);
int db_update_artist_art(char*, char*);
void db_result_free(char*);
int db_update_song_rating(char*, char*, int);
int db_get_song_rating(char*, char*);
int db_get_prior_song_by_rating_first();
int db_get_song_by_rating_next();
char *db_get_song_name(int id);
char *db_get_song_artist(int id);

#endif
