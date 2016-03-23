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

#define ALOGD(fmt, args...) fprintf(stderr, fmt, ##args)
#define ALOGE(fmt, args...) fprintf(stderr, fmt, ##args)
#define ALOGV(fmt, args...) fprintf(stderr, fmt, ##args)

#define EVT_SOCK "evt_sock"

#define EVT_CMD_PACKET 1

#ifndef SRD_UID
#define SRD_UID 1002
#endif
#ifndef SYSTEM_UID
#define SYSTEM_UID 1000
#endif

#ifndef ROOT_UID
#define ROOT_UID 0
#endif

pthread_mutex_t signal_mutex;

int remote_evt_fd;
static pthread_t evt_mon_thread;

static void handle_cleanup();

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
    ALOGV("%s: \n", __func__);
    unsigned char first_byte;
    int retval;

    retval = read (fd, &first_byte, 1);
    if (retval < 0) {
        ALOGE("%s:read returns err: %d\n", __func__,retval);
        return -1;
    }

    if (retval == 0) {
        ALOGE("%s: This indicates the close of other end\n", __func__);
        return -99;
    }

    ALOGV("%s: Protocol_byte: %x\n", __func__, first_byte);
    switch(first_byte) {
        default:
            ALOGE("%s: Unexpected data format!!\n",__func__);
            retval = -1;
    }

    ALOGV("%s: retval %d\n", __func__, 0);
    return 0;
}

static int evt_thread() {
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
                   if (retval == -99) {
                       ALOGV("%s:End of wait loop\n", __func__);
                   }
                   ALOGV("%s: handle_command_writes returns: %d: \n", __func__, retval);
                   handle_cleanup();
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

static int main_thread()
{
    int ret = 0;
    ALOGV("%s: Entry\n", __func__);
    while (1) {
        sleep(1);
    }
    ALOGV("%s: End\n", __func__);
    return ret;
}

int main() {
    int ret = 0;
    ALOGV("%s: Entry\n", __func__);
    signal(SIGPIPE, SIG_IGN);

    pthread_mutex_init(&signal_mutex, NULL);
    if (pthread_create(&evt_mon_thread, NULL, (void *)evt_thread, NULL) != 0) {
        perror("pthread_create for evt_monitor\n");
        pthread_mutex_destroy(&signal_mutex);
        return -1;
    }

    ret = main_thread();
    if (ret < 0) {
        ALOGE("%s: start_main_thread returns: %d", __func__, ret);
    }

    cleanup_thread(evt_mon_thread);
    pthread_mutex_destroy(&signal_mutex);

    ALOGV("%s: Exit: %d\n", __func__, ret);
    return ret;
}

static void handle_cleanup()
{
}

void report_soc_failure(void)
{
   char eve_buf[] = {0x04,0x10,0x01,0x0f};
   int ret_val;
   ALOGD("%s\n",__func__);
   ret_val = write(remote_evt_fd,eve_buf,4);
   if(ret_val < 0)
    ALOGE("%s: Failed to report\n",__func__);
}
