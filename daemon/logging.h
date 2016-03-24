#ifndef __LOGGING_H__
#define __LOGGING_H__

void print_to_console (char* fmt, ...);
#define ALOGD(fmt, args...) print_to_console(fmt, ##args)
#define ALOGE(fmt, args...) print_to_console(fmt, ##args)
#define ALOGV(fmt, args...) print_to_console(fmt, ##args)

#endif
