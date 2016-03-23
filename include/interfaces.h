#ifndef __INTERFACES_H__
#define __INTERFACES_H__

#define CLIENT_NAME_MAX (20)
#define BUF_MAX (128)

struct sr_msg_data {
    int msg_type;
    char client_name[CLIENT_NAME_MAX];
    union {
        int req_data;
        char data[BUF_MAX];
    };
};

#endif
