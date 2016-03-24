#ifndef __INTERFACES_H__
#define __INTERFACES_H__

#include <libstreamripper.h>

#define CLIENT_NAME_MAX (20)
#define BUF_MAX MAX_URL_LEN

enum MsgType {
    MSG_SET_URL,
    MSG_SET_OUTPUT_PATH,
    MSG_SET_INCOMPLETE_PATH,
};

struct sr_msg_data {
    enum MsgType msg_type;
    char client_name[CLIENT_NAME_MAX];
    char data[BUF_MAX];
};

#endif
