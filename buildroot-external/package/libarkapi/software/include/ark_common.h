#ifndef __ARK_COMMON_H__
#define __ARK_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ark_api.h"


#define	SUCCESS                         0


//#define ARK_DBG
#ifdef ARK_DBG
#define ark_dbg(...)  printf(__VA_ARGS__)
#else
#define ark_dbg(...)
#endif

#define SCREEN_WIDTH                    1024
#define SCREEN_HEIGHT                   600



#ifdef __cplusplus
}
#endif

#endif
