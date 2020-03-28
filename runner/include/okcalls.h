#include <sys/syscall.h>
#define HOJ_MAX_LIMIT -1
#define CALL_ARRAY_SIZE 512
#ifdef __i386
   #include "okcalls32.h"
#endif
#ifdef __x86_64
   #include "okcalls64.h"
#endif
#ifdef __arm__
   #include "okcalls_arm.h"
#endif
#ifdef __aarch64__
   #include "okcalls_aarch64.h"
#endif
#ifdef __mips__
   #include "okcalls_mips.h"
#endif
