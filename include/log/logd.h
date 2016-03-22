/*
 * Send a formatted string to the log, used like printf(fmt,...)
 */
int __android_log_print(int prio, const char *tag,  const char *fmt, ...)
#if defined(__GNUC__)
#ifdef __USE_MINGW_ANSI_STDIO
#if __USE_MINGW_ANSI_STDIO
__attribute__ ((format(gnu_printf, 3, 4)))
#else
__attribute__ ((format(printf, 3, 4)))
#endif
#else
__attribute__ ((format(printf, 3, 4)))
#endif
#endif
    ;

