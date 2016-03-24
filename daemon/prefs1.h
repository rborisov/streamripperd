#ifndef __PREFS1_H__
#define __PREFS1_H__

#include <libstreamripper.h>

STREAM_PREFS prefs;

enum PrefsValue {
    URL,
    OUTPUT_DIR,
    INCOMPLETE_DIR
};

void init_config();
void set_pref(enum PrefsValue val, char * data);
BOOL do_restart();

#endif
