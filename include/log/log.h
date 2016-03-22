#ifndef __LOG_H
#define __LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

    /*
     *  * This is the local tag used for the following simplified
     *   * logging macros.  You can change this preprocessor definition
     *    * before using the other macros to change the tag.
     *     */
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif
#define LOG_BUF_SIZE 1024
/*
 * Android log priority values, in ascending priority order.
 */
    typedef enum LogPriority {
        DM_LOG_UNKNOWN = 0,
        DM_LOG_DEFAULT,    /* only for SetMinPriority() */
        DM_LOG_VERBOSE,
        DM_LOG_DEBUG,
        DM_LOG_INFO,
        DM_LOG_WARN,
        DM_LOG_ERROR,
        DM_LOG_FATAL,
        DM_LOG_SILENT,     /* only for SetMinPriority(); must be last */
    } LogPriority;

/*
 * ===========================================================================
 *
 * The stuff in the rest of this file should not be used directly.
 */

#define android_printLog(prio, tag, fmt...) \
    {    va_list ap; \
    char buf[LOG_BUF_SIZE]; \
    va_start(ap, fmt); \
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap); \
    va_end(ap); \
        printf("%s", buf);


/*
 * Log macro that allows you to specify a number for the priority.
 */
#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) \
    android_printLog(priority, tag, __VA_ARGS__)
#endif

/*
 * Basic log message macro.
 *
 * Example:
 * ALOG(LOG_WARN, NULL, "Failed with error %d", errno);
 *
 * The second argument may be NULL or "" to indicate the "global" tag.
 */
#ifndef ALOG
#define ALOG(priority, tag, ...) \
    LOG_PRI(DM_##priority, tag, __VA_ARGS__)
#endif

/*
 *  * Simplified macro to send a debug log message using the current LOG_TAG.
 *   */
#ifndef ALOGD
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

/*
 *  * Simplified macro to send a verbose log message using the current LOG_TAG.
 *   */
#ifndef ALOGV
#define __ALOGV(...) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#if LOG_NDEBUG
#define ALOGV(...) do { if (0) { __ALOGV(__VA_ARGS__); } } while (0)
#else
#define ALOGV(...) __ALOGV(__VA_ARGS__)
#endif
#endif

/*
 *  * Simplified macro to send an error log message using the current LOG_TAG.
 *   */
#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif



#ifdef __cplusplus
}
#endif

#endif /* _LIBS_LOG_LOG_H */
