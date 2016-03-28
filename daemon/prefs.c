#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pwd.h>
#include "config1.h"
#include "prefs1.h"
#include "logging.h"

#ifndef VERSION
#define VERSION "dev"
#endif

#define DEFAULT_CFG_FILENAME "streamripperd.cfg"
#define DEFAULT_OUTPUT_DIR "Music"
#define DEFAULT_INCOMPLETE_DIR "Incomplete"
#define DEFAULT_URL "http://svr1.msmn.co:8136"

static BOOL m_prefs_updated = FALSE;
static int m_prefs_started = -1;

config_obj *m_config;
static char cfg_file_name[128] = "";
static void verify_splitpoint_rules (STREAM_PREFS *prefs);

void init_config() {
    char *cfg_string = NULL;
    struct passwd *pw = getpwuid(getuid());
    sprintf(cfg_file_name, "%s/%s", pw->pw_dir, DEFAULT_CFG_FILENAME);
    m_config = cfg_open(cfg_file_name);
    cfg_string = cfg_get_single_value_as_string(m_config, "Default", "version");
    if (cfg_string == NULL || strcmp(cfg_string, VERSION))
    {
        cfg_set_single_value_as_string(m_config, "Default", "version", VERSION);
    }
    m_prefs_started = cfg_get_single_value_as_int_with_default(m_config, "Streamripper", "started", 1);
    free(cfg_string);
    cfg_close(m_config);
}

BOOL do_restart()
{
    BOOL rc = m_prefs_updated;
    m_prefs_updated = FALSE;
    return rc;
}

BOOL get_pref_started()
{
    if (m_prefs_started == -1 && m_config != NULL) {
        m_config = cfg_open(cfg_file_name);
        //streamripper is started by default
        m_prefs_started = cfg_get_single_value_as_int_with_default(m_config, "Streamripper", "started", 1);
        cfg_close(m_config);
    }
    return TRUE ? (m_prefs_started == 1) : FALSE;
}

void set_pref_started(BOOL data)
{
    m_prefs_started = 1 ? data : 0;
    if (m_config != NULL) {
        m_config = cfg_open(cfg_file_name);
        cfg_set_single_value_as_int(m_config, "Streamripper",  "started", m_prefs_started);
        cfg_close(m_config);
    }
}

void set_pref(enum PrefsValue val, char * data)
{
    char * cfg_string = NULL;
    if (m_config != NULL) {
        prefs_load();
        m_config = cfg_open(cfg_file_name);
        switch (val) {
            case URL:
                if (data == NULL) {
                    cfg_string = cfg_get_single_value_as_string(m_config, "Streamripper", "url");
                    if (cfg_string == NULL) {
                        cfg_set_single_value_as_string(m_config, "Streamripper", "url", DEFAULT_URL);
                        strncpy(prefs.url, DEFAULT_URL, MAX_URL_LEN);
                    } else {
                        strncpy(prefs.url, cfg_string, MAX_URL_LEN);
                    }
                } else {
                    cfg_set_single_value_as_string(m_config, "Streamripper", "url", data);
                    strncpy(prefs.url, data, MAX_URL_LEN);
                }
                prefs_get_stream_prefs (&prefs, prefs.url);
                m_prefs_updated = TRUE;
                print_to_console("%s: set url to %s\n", __func__, prefs.url);
                break;
            case OUTPUT_DIR:
                if (data == NULL) {
                    cfg_string = cfg_get_single_value_as_string(m_config, "Streamripper", "dest_dir");
                    if (cfg_string == NULL) {
                        cfg_set_single_value_as_string(m_config, "Streamripper", "dest_dir", DEFAULT_OUTPUT_DIR);
                        strncpy(prefs.output_directory, DEFAULT_OUTPUT_DIR, SR_MAX_PATH);
                    } else {
                        strncpy(prefs.output_directory, cfg_string, SR_MAX_PATH);
                    }
                } else {
                    cfg_set_single_value_as_string(m_config, "Streamripper", "dest_dir", data);
                    strncpy(prefs.output_directory, data, SR_MAX_PATH);
                }
                m_prefs_updated = TRUE;
                print_to_console("%s: set output_directory to %s\n", __func__, prefs.output_directory);
                break;
            case INCOMPLETE_DIR:
                if (data == NULL) {
                    cfg_string = cfg_get_single_value_as_string(m_config, "Streamripper", "incomplete_dir");
                    if (cfg_string == NULL) {
                        cfg_set_single_value_as_string(m_config, "Streamripper", "incomplete_dir", DEFAULT_INCOMPLETE_DIR);
                        strncpy(prefs.incomplete_directory, DEFAULT_INCOMPLETE_DIR, SR_MAX_PATH);
                    } else {
                        strncpy(prefs.incomplete_directory, cfg_string, SR_MAX_PATH);
                    }
                } else {
                    cfg_set_single_value_as_string(m_config, "Streamripper", "incomplete_dir", data);
                    strncpy(prefs.incomplete_directory, data, SR_MAX_PATH);
                }
                m_prefs_updated = TRUE;
                print_to_console("%s: set incomplete_directory to %s\n", __func__, prefs.incomplete_directory);
                break;
            default:
                print_to_console("%s: what am I doing here?", __func__);
        }
        prefs_save();

        free(cfg_string);
        cfg_close(m_config);


        verify_splitpoint_rules(&prefs);
    } else
        print_to_console("%s: m_config must be NULL and cfg_file_name must be not NULL\n", __func__);
    return;
}

static void verify_splitpoint_rules (STREAM_PREFS *prefs)
{
#if defined (commentout)
    /* This is still not complete, but the warning causes people to 
     *      *        wonder what is going on. */
    fprintf (stderr, "Warning: splitpoint sanity check not yet complete.\n");
#endif

    /* xs_silence_length must be non-negative and divisible by two */
    if (prefs->sp_opt.xs_silence_length < 0) {
        prefs->sp_opt.xs_silence_length = 0;
    }
    if (prefs->sp_opt.xs_silence_length % 2) {
        prefs->sp_opt.xs_silence_length ++;
    }

    /* search_window values must be non-negative */
    if (prefs->sp_opt.xs_search_window_1 < 0) {
        prefs->sp_opt.xs_search_window_1 = 0;
    }
    if (prefs->sp_opt.xs_search_window_2 < 0) {
        prefs->sp_opt.xs_search_window_2 = 0;
    }

    /* if silence_length is 0, then search window should be zero */
    if (prefs->sp_opt.xs_silence_length == 0) {
        prefs->sp_opt.xs_search_window_1 = 0;
        prefs->sp_opt.xs_search_window_2 = 0;
    }

    /* search_window values must be longer than silence_length */
    if (prefs->sp_opt.xs_search_window_1 + prefs->sp_opt.xs_search_window_2
            < prefs->sp_opt.xs_silence_length) {
        /* if this happens, disable search */
        prefs->sp_opt.xs_search_window_1 = 0;
        prefs->sp_opt.xs_search_window_2 = 0;
        prefs->sp_opt.xs_silence_length = 0;
    }

    /* search window lengths must be at least 1/2 of silence_length */
    if (prefs->sp_opt.xs_search_window_1 < prefs->sp_opt.xs_silence_length) {
        prefs->sp_opt.xs_search_window_1 = prefs->sp_opt.xs_silence_length;
    }
    if (prefs->sp_opt.xs_search_window_2 < prefs->sp_opt.xs_silence_length) {
        prefs->sp_opt.xs_search_window_2 = prefs->sp_opt.xs_silence_length;
    }
}
