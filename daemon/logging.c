#include <stdio.h>
#include <stdarg.h>
#include "logging.h"

#undef PRINT_STDERR
//#define PRINT_STDERR

void print_to_console (char* fmt, ...)
{
    va_list argptr;

    va_start (argptr, fmt);
#ifdef PRINT_STDERR
        vfprintf (stderr, fmt, argptr);
        fflush (stderr);
#else
        vfprintf (stdout, fmt, argptr);
        fflush (stdout);
#endif

    va_end (argptr);
}
