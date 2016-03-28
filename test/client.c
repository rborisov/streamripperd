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

static void parse_arguments(int fd, int argc, char **argv)
{
    int i;
    char *c;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-')
            continue;

        c = strchr ("iou", argv[i][1]);
        if (c != NULL) {
            if ((i == (argc-1)) || (argv[i+1][0] == '-')) {
                fprintf(stderr, "option %s requires an argument\n", argv[i]);
                exit(1);
            }
        }
        switch (argv[i][1])
        {
            case 's':
                //stop
                client_msg.msg_type = MSG_STOP_STREAMRIPPER;
                break;
            case 'S':
                //start
                client_msg.msg_type = MSG_START_STREAMRIPPER;
                break;
            case 'u':
                //set url
                client_msg.msg_type = MSG_SET_URL;
                strcpy(client_msg.data, argv[i]);
                break;
            case 'o':
                //set out dir
                client_msg.msg_type = MSG_SET_OUTPUT_PATH;
                strcpy(client_msg.data, argv[i]);
                break;
            case 'i':
                //set incomplete dir
                client_msg.msg_type = MSG_SET_INCOMPLETE_PATH;
                strcpy(client_msg.data, argv[i]);
                break;
        }
        send_smth_to(fd);
    }

}

int main(int argc, char *argv[]) 
{
    int ret = 0;
    int fd1;
    ALOGV("%s: Entry\n", __func__);

//    fd1 = connect_to_local_socket("evt_sock");
    do {
        fd1 = connect_to_local_socket("evt_sock");
        sleep(1);
    } while (fd1 == -1);
    if (fd1 != -1) {
        ALOGV("%s: received the socket fd: %d\n",
                __func__, fd1);
        parse_arguments(fd1, argc, argv);
//        client_msg.msg_type = MSG_SET_URL;
//        strcpy(client_msg.data, "http://svr1.msmn.co:8136");
//        send_smth_to(fd1);
//        sleep(10);
//        client_msg.msg_type = MSG_SET_OUTPUT_PATH;
//        strcpy(client_msg.data, "/storage/Music");
//        send_smth_to(fd1);
    }

    close(fd1);

    return ret;
}
