/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
#define PLOG_BUFSIZE (4096)

#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */


/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

//TODO:add somekid of support for thread safe prints, to prevet interleaving

#ifndef NDEBUG
    #define LOG_ERR(...) \
        do { \
            printf(ANSI_COLOR_CYAN "[%s][%s:%d]:\n", __FUNCTION__, __FILE__, __LINE__);   \
            printf(ANSI_COLOR_RED);    \
            printf(__VA_ARGS__); \
            printf(ANSI_COLOR_RESET "\n"); \
        } while (0)
#else
    #define LOG_ERR(...) do {} while (0)
#endif

#ifndef NDEBUG
    #define LOG(...) \
        do { \
            printf(ANSI_COLOR_CYAN "[%s][%s:%d]:\n", __FUNCTION__, __FILE__, __LINE__);   \
            printf(ANSI_COLOR_WHITE);    \
            printf(__VA_ARGS__); \
            printf(ANSI_COLOR_RESET "\n"); \
        } while (0)
#else
    #define LOG(...) do {} while (0)
#endif

#ifndef NDEBUG
#define pLOG(fmt, ...) do {                                                        \
    char logBuf[PLOG_BUFSIZE];                                                     \
    snprintf(logBuf, sizeof(logBuf),                                               \
             ANSI_COLOR_CYAN "[%s:%d:%s] \n" ANSI_COLOR_WHITE fmt ANSI_COLOR_RESET "\n", \
             __FILE__, __LINE__, __func__, ##__VA_ARGS__);                         \
    fprintf(stderr, "%s", logBuf);                                                 \
    fflush(stderr);                                                                \
} while (0)
#else
    #define pLOG(...) do {} while (0)
#endif // DEBUG

