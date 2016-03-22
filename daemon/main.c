#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pwd.h>
#include <libstreamripper.h>

#include "config1.h"
#include "mchar.h"
#include "filelib.h"
#include "debug.h"
#include "socket.h"

#ifndef VERSION
#define VERSION "dev"
#endif
#define DEFAULT_CFG_FILENAME "streamripperd.cfg"
#define DEFAULT_OUTPUT_DIR "Music"
#define DEFAULT_INCOMPLETE_DIR "Incomplete"
#define DEFAULT_URL "http://svr1.msmn.co:8136"

static void catch_sig (int code);
static void rip_callback (RIP_MANAGER_INFO* rmi, int message, void *data);
static void verify_splitpoint_rules (STREAM_PREFS *prefs);
static void print_status (RIP_MANAGER_INFO *rmi);
static void print_to_console (char* fmt, ...);

static char m_buffer_chars[] = {'\\', '|', '/', '-', '*'}; /* for formating */
config_obj *m_config = NULL;
static char cfg_file_name[128] = "";
static BOOL m_started = FALSE;
static BOOL m_alldone = FALSE;
static BOOL m_got_the_signal = FALSE;
static BOOL m_dont_print = FALSE;
static BOOL m_print_stderr = FALSE;
time_t      m_stop_time = 0;
STREAM_PREFS prefs;
pthread_mutex_t signal_mutex;
static pthread_t evt_mon_thread;

enum PrefsValue {
    URL,
    OUTPUT_DIR,
    INCOMPLETE_DIR
};

static void set_pref(enum PrefsValue val, char * data)
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
                    strncpy(prefs.output_directory, data, MAX_URL_LEN);
                }
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
                    strncpy(prefs.incomplete_directory, data, MAX_URL_LEN);
                }
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

int main()
{
    int ret = 0;
    char *cfg_string = NULL;
    struct passwd *pw = getpwuid(getuid());

    RIP_MANAGER_INFO *rmi = 0;

    sr_set_locale();

    signal (SIGINT, catch_sig);
    signal (SIGTERM, catch_sig);

    print_to_console("Connecting...\n");
    rip_manager_init ();

    sprintf(cfg_file_name, "%s/%s", pw->pw_dir, DEFAULT_CFG_FILENAME);
    m_config = cfg_open(cfg_file_name);
    cfg_string = cfg_get_single_value_as_string(m_config, "Default", "version");
    if (cfg_string == NULL || strcmp(cfg_string, VERSION))
    {
        cfg_set_single_value_as_string(m_config, "Default", "version", VERSION);
    }
    free(cfg_string);
    cfg_close(m_config);

    set_pref(URL, NULL);
    set_pref(OUTPUT_DIR, NULL);
    set_pref(INCOMPLETE_DIR, NULL);

    pthread_mutex_init(&signal_mutex, NULL);
    if (pthread_create(&evt_mon_thread, NULL, (void *)evt_thread, NULL) != 0) {
        perror("pthread_create for evt_monitor\n");
        pthread_mutex_destroy(&signal_mutex);
        return -1;
    }

    while (!m_got_the_signal) {
        if (!m_started) {
            if ((ret = rip_manager_start (&rmi, &prefs, rip_callback)) != SR_SUCCESS) {
                print_to_console("error: rip_manager_start %d\n", ret);
            }
        }
        sleep(1);
    }

    print_to_console("shutting down\n");

    rip_manager_stop (rmi);
    rip_manager_cleanup ();

    cleanup_thread(evt_mon_thread);
    pthread_mutex_destroy(&signal_mutex);

    return ret;
}

void rip_callback (RIP_MANAGER_INFO* rmi, int message, void *data)
{
    ERROR_INFO *err;
    switch(message)
    {
        case RM_UPDATE:
            print_status (rmi);
            break;
        case RM_ERROR:
            err = (ERROR_INFO*)data;
            print_to_console("error %d [%s]\n", err->error_code, err->error_str);
            m_alldone = TRUE;
            break;
        case RM_DONE:
            print_to_console("bye..\n");
            m_alldone = TRUE;
            break;
        case RM_NEW_TRACK:
            print_to_console("\n");
            break;
        case RM_STARTED:
            m_started = TRUE;
            break;
    }
}

void catch_sig(int code)
{
    print_to_console("\n");
    if (!m_started)
        exit(2);
    m_got_the_signal = TRUE;
}

static void verify_splitpoint_rules (STREAM_PREFS *prefs)
{
#if defined (commentout)
    /* This is still not complete, but the warning causes people to 
     *        wonder what is going on. */
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

void print_status (RIP_MANAGER_INFO *rmi)
{
    STREAM_PREFS *prefs = rmi->prefs;
    char status_str[128];
    char filesize_str[64];
    static int buffering_tick = 0;
    BOOL static printed_fullinfo = FALSE;

    if (m_dont_print)
        return;

    if (printed_fullinfo && rmi->filename[0]) {

        switch(rmi->status)
        {
            case RM_STATUS_BUFFERING:
                buffering_tick++;
                if (buffering_tick == 5)
                    buffering_tick = 0;

                sprintf(status_str,"buffering - %c ",
                        m_buffer_chars[buffering_tick]);

                print_to_console("[%14s] %.50s\r",
                        status_str,
                        rmi->filename);
                break;

            case RM_STATUS_RIPPING:
                if (rmi->track_count < prefs->dropcount) {
                    strcpy(status_str, "skipping...   ");
                } else {
                    strcpy(status_str, "ripping...    ");
                                                                                                                                                    }
                format_byte_size(filesize_str, rmi->filesize);
                print_to_console("[%14s] %.50s [%7s]\r",
                        status_str,
                        rmi->filename,
                        filesize_str);
                break;
            case RM_STATUS_RECONNECTING:
                strcpy(status_str, "re-connecting..");
                print_to_console("[%14s]\r", status_str);
                break;
        }

    }
    if (!printed_fullinfo) {
        print_to_console("stream: %s\n"
                "server name: %s\n",
                rmi->streamname,
                rmi->server_name);
        if (rmi->http_bitrate > 0) {
            print_to_console("declared bitrate: %d\n",
                    rmi->http_bitrate);
        }
        if (rmi->meta_interval != NO_META_INTERVAL) {
            print_to_console("meta interval: %d\n",
                    rmi->meta_interval);
        }
        if (GET_MAKE_RELAY(prefs->flags)) {
            print_to_console("relay port: %d\n"
                    "[%14s]\r",
                    prefs->relay_port,
                    "getting track name... ");
        }

        printed_fullinfo = TRUE;
    }
}

static void print_to_console (char* fmt, ...)
{
    va_list argptr;

    if (!m_dont_print) {
        va_start (argptr, fmt);

        if (m_print_stderr) {
            vfprintf (stderr, fmt, argptr);
            fflush (stderr);
        } else {
            vfprintf (stdout, fmt, argptr);
            fflush (stdout);
        }

        va_end (argptr);
    }
}

