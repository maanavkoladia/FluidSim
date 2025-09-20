#include "LOG.h"
#include <stdbool.h>
#include <stdio.h>

/* ================================================== */
/*                      DEFINES                       */
/* ================================================== */

//fprintf(stderr, ANSI_COLOR_RED "ASSERT FAILED: \n");
//fprintf(stderr, ANSI_COLOR_RESET);
//pLOG("%s", (errMsg));

//printf(ANSI_COLOR_RED "ASSERT FAILED: \n");   \
//printf(ANSI_COLOR_RESET);    \
//pLOG("%s", (errMsg)); \

#ifndef NDEBUG
    #define ASSERT_OUT(errMsg)  \
        do {    \
            fprintf(stderr, ANSI_COLOR_RED "ASSERT FAILED: \n");    \
            fprintf(stderr, ANSI_COLOR_RESET);  \
            pLOG("%s", (errMsg));   \
        }while(0)
#endif

#ifndef NDEBUG
    #define ASSERT_COMMON(cond, errMsg, errRet) \
        do { \
            if(!(cond)){ \
                ASSERT_OUT((errMsg));   \
                return (errRet); \
            } \
        }while(0) 
#else 
    #define ASSERT_COMMON(cond, errMsg, errRet) \
        do { if (!(cond)) return (errRet); } while(0)
#endif // DEBUG


#ifndef NDEBUG 
    #define ASSERT_COMMON_CB(cond, cb, data, errRet)      \
        do {                                              \
            if (!(cond)) {                                \
                if(cb != NULL){ \
                    cb(data); \
                } \
                return (errRet);                          \
            }                                             \
        } while (0)
#else
    #define ASSERT_COMMON_CB(cond, cb, data, errRet) \
        do { if (!(cond)) return (errRet); } while(0)
#endif

#ifndef NDEBUG
    #define ASSERT_COMMON_NO_RET(cond, errMsg) \
        do { \
            if (!(cond)) { \
                ASSERT_OUT((errMsg)); \
            } \
        } while (0)
#else
    #define ASSERT_COMMON_NO_RET(cond, errMsg) \
        do { (void)(cond); } while (0)
#endif
