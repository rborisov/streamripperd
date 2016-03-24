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

#define ALOGD(fmt, args...) fprintf(stderr, fmt, ##args)
#define ALOGE(fmt, args...) fprintf(stderr, fmt, ##args)
#define ALOGV(fmt, args...) fprintf(stderr, fmt, ##args)

static struct sr_msg_data client_msg;

void send_smth_to(int fd)
{
    int rc = 0;
    ALOGD("%s: sending %d bytes to fd %d\n", __func__, sizeof(struct sr_msg_data), fd);
    rc = send(fd, &client_msg, sizeof(struct sr_msg_data), MSG_NOSIGNAL);
    if (rc <= 0) {
        ALOGE("%s: %s - unable to send data to fd %d\n", __func__, strerror(errno), fd);
    }
}

int connect_to_local_socket(char* name) 
{
    socklen_t len; int sk = -1;

    ALOGE("%s: ACCEPT \n", __func__);
    sk  = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sk < 0) {
        ALOGE("Socket creation failure\n");
        return -1;
    }

    if(socket_local_client_connect(sk, name,
                ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM) < 0)
    {
        ALOGE("failed to connect (%s)\n", strerror(errno));
        close(sk);
        sk = -1;
    } else {
        ALOGE("%s: Connection succeeded\n", __func__);
    }
    return sk;
}

int main() 
{
    int ret = 0;
    int fd, fd1;
    ALOGV("%s: Entry\n", __func__);

    fd1 = connect_to_local_socket("evt_sock");
    if (fd1 != -1) {
        ALOGV("%s: received the socket fd: %d\n",
                __func__, fd1);
        client_msg.msg_type = MSG_SET_URL;
        strcpy(client_msg.data, "http://svr1.msmn.co:8136");
        send_smth_to(fd1);
        sleep(10);
        client_msg.msg_type = MSG_SET_OUTPUT_PATH;
        strcpy(client_msg.data, "/storage/Music");
        send_smth_to(fd1);
    }

    close(fd1);

    return ret;
}
