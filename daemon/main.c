#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <libstreamripper.h>

#include "mchar.h"
#include "filelib.h"
#include "debug.h"
#include "socket.h"
#include "prefs1.h"
#include "logging.h"

static void catch_sig (int code);
static void rip_callback (RIP_MANAGER_INFO* rmi, int message, void *data);
static void print_status (RIP_MANAGER_INFO *rmi);

static char m_buffer_chars[] = {'\\', '|', '/', '-', '*'}; /* for formating */
static BOOL m_started = FALSE;
static BOOL m_alldone = FALSE;
static BOOL m_got_the_signal = FALSE;
static BOOL m_dont_print = FALSE;
pthread_mutex_t signal_mutex;
static pthread_t evt_mon_thread;

int main()
{
    int ret = 0;
    RIP_MANAGER_INFO *rmi = 0;

    sr_set_locale();

    signal (SIGINT, catch_sig);
    signal (SIGTERM, catch_sig);

    print_to_console("Connecting...\n");
    rip_manager_init ();

    init_config();

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
        if ((!get_pref_started() || do_restart()) && m_started) {
            rip_manager_stop(rmi);
            m_started = FALSE;
        }
        if (!m_started && get_pref_started()) {
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
            print_to_console("\nerror %d [%s]\n", err->error_code, err->error_str);
            m_alldone = TRUE;
            break;
        case RM_DONE:
            print_to_console("\nbye..\n");
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
