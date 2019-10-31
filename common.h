#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "types.h"

#ifndef ZeroMemory
#define ZeroMemory(a, b)      memset( a, 0, b )
#endif

#define PRINTF                    printf
#define SECTOR                    DWORD

#define STRINGIFY(x)            #x
#define TOSTRING(x)                STRINGIFY(x)

#ifndef __FUNCTION__
#define __LOCATION__			__FILE__ "(" TOSTRING(__LINE__) ") "
#else
#define __LOCATION__            __FILE__ "(" TOSTRING(__LINE__) ", " __FUNCTION__ ") "
#endif

#define WARNING(...)            PRINTF( __VA_ARGS__ )

#ifndef _DEBUG
#define TRACE
#else
#define TRACE                   PRINTF
#endif

#define STEP(a)                { PRINTF( "%s(%d): %s;\n", __FILE__, __LINE__, # a ); a; }

#define EXT2_ERROR                -1
#define EXT2_SUCCESS                0

#endif
