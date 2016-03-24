#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/un.h>
#include <cutils/sockets.h>
#include <interfaces.h>

#include "socket.h"
#include "logging.h"
#include "prefs1.h"

#define EVT_SOCK "evt_sock"

#define EVT_CMD_PACKET 1
#define UDM_CMD_PACKET 2

#ifndef SRD_UID
#define SRD_UID 1002
#endif
#ifndef SYSTEM_UID
#define SYSTEM_UID 1000
#endif

#ifndef ROOT_UID
#define ROOT_UID 0
#endif

int remote_evt_fd;

static int establish_remote_socket(char *name)
{
    int fd = -1;
    struct sockaddr_un client_address;
    socklen_t clen;
    int sock_id, ret;
    struct ucred creds;

    ALOGV("%s(%s) Entry  \n", __func__, name);

    sock_id = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock_id < 0) {
        ALOGE("%s: server Socket creation failure\n", __func__);
        return fd;
    }

    ALOGV("convert name to android abstract name:%s %d\n", name, sock_id);
    if (socket_local_server_bind(sock_id,
        name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0) {
        if (listen(sock_id, 5) == 0) {
            ALOGV("listen to local socket:%s, fd:%d\n", name, sock_id);
        } else {
            ALOGE("listen to local socket:failed\n");
            close(sock_id);
            return fd;
        }
    } else {
        close(sock_id);
        ALOGE("%s: server bind failed for socket : %s\n", __func__, name);
        return fd;
    }

    clen = sizeof(client_address);
    ALOGV("%s: before accept_server_socket\n", name);
    fd = accept(sock_id, (struct sockaddr *)&client_address, &clen);
    if (fd > 0) {
        ALOGV("%s accepted fd:%d for server fd:%d\n", name, fd, sock_id);
        close(sock_id);

        memset(&creds, 0, sizeof(creds));
        socklen_t szCreds = sizeof(creds);
        ret = getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &creds, &szCreds);
        if (ret < 0) {
            ALOGE("%s: error getting remote socket creds: %d\n\n", __func__, ret);
            close(fd);
            return -1;
        }
        if (creds.uid != SRD_UID && creds.uid != SYSTEM_UID
                && creds.uid != ROOT_UID) {
            ALOGE("%s: client doesn't have required credentials\n", __func__);
            ALOGE("<%s req> client uid: %d", name, creds.uid);
            close(fd);
            return -1;
        }

        ALOGV("%s: Remote socket credentials: %d\n", __func__, creds.uid);
        return fd;
    } else {
        ALOGE("accept failed fd:%d sock d:%d error %s\n", fd, sock_id, strerror(errno));
        close(sock_id);
        return fd;
    }

    close(sock_id);
    return fd;
}

int handle_command_writes(int fd) {
    int retval;
    struct sr_msg_data client_msg;

    memset(&client_msg, 0, sizeof(struct sr_msg_data));
    retval = recv(fd, &client_msg, sizeof(struct sr_msg_data), 0);    
    if (retval < 0) {
        ALOGE("%s:read returns err: %d\n", __func__,retval);
        return -1;
    }
    if (retval == 0)
        return -99; //no more data received

    if (retval > 0) {
        switch(client_msg.msg_type) {
            case MSG_SET_URL:
                ALOGV("%s: SET_URL msg received: %s\n", __func__, client_msg.data);
                set_pref(URL, client_msg.data);
                break;
            case MSG_SET_OUTPUT_PATH:
                ALOGV("%s: SET_OUTPUT_PATH msg received: %s\n", __func__, client_msg.data);
                break;
            case MSG_SET_INCOMPLETE_PATH:
                ALOGV("%s: SET_INCOMPLETE_PATH msg received: %s\n", __func__, client_msg.data);
                break;
            default:
                ALOGE("%s: Unexpected data format!!\n",__func__);
                retval = -1;
        }
    }

    return 0;
}

int evt_thread() {
    fd_set client_fds;
    int retval, n;

    ALOGV("%s: Entry \n", __func__);
    do {
        remote_evt_fd = establish_remote_socket(EVT_SOCK);

        if (remote_evt_fd < 0) {
            ALOGE("%s: invalid remote socket\n", __func__);
            return -1;
        }

        FD_ZERO(&client_fds);
        FD_SET(remote_evt_fd, &client_fds);

        do {
            ALOGV("%s: Back in Events select loop\n", __func__);
            n = select(remote_evt_fd+1, &client_fds, NULL, NULL, NULL);
            if(n < 0){
                ALOGE("Select: failed: %s\n", strerror(errno));
                break;
            }
            ALOGV("%s: select came out\n", __func__);
            if (FD_ISSET(remote_evt_fd, &client_fds)) {
                retval = handle_command_writes(remote_evt_fd);
                ALOGV("%s: handle_command_writes . %d\n", __func__, retval);
               if(retval < 0) {
                   ALOGV("%s:End of wait loop\n", __func__);
                   break;
                }
            }
        } while(1);

        ALOGV("%s: Events turned off\n", __func__);
        close(remote_evt_fd);
        remote_evt_fd = 0;
    } while(1);

    pthread_exit(NULL);
    return 0;
}

int cleanup_thread(pthread_t thread) {
    int status = 0;
    ALOGV("%s: Entry\n", __func__);

    if((status = pthread_kill(thread, SIGUSR1)) != 0) {
        ALOGE("Error cancelling thread %d, error = %d (%s)\n",
        (int)thread, status, strerror(status));
    }

    if((status = pthread_join(thread, NULL)) != 0) {
        ALOGE("Error joining thread %d, error = %d (%s)\n",
      (int)thread, status, strerror(status));
    }
    ALOGV("%s: End\n", __func__);
    return status;
}
