#ifndef NN_SOCKET_MIN_H
#define NN_SOCKET_MIN_H

#include <cstdint>
#include <cstddef>
#include "nn/result.h"

namespace nn { namespace socket {

enum class Level : int32_t {
    Sol_Socket  = 0xffff,
    Sol_Ip      = 0,
    Sol_Icmp    = 1,
    Sol_Tcp     = 6,
    Sol_Udp     = 17,
    Sol_UdpLite = 136,
};

enum class Option : uint32_t {
    So_Debug        = 0x0001,
    So_AcceptConn   = 0x0002,
    So_ReuseAddr    = 0x0004,
    So_KeepAlive    = 0x0008,
    So_DontRoute    = 0x0010,
    So_Broadcast    = 0x0020,
    So_UseLoopback  = 0x0040,
    So_Linger       = 0x0080,
    So_OobInline    = 0x0100,
    So_ReusePort    = 0x0200,

    So_SndBuf       = 0x1001,
    So_RcvBuf       = 0x1002,
    So_SndLoWat     = 0x1003,
    So_RcvLoWat     = 0x1004,
    So_SndTimeo     = 0x1005,
    So_RcvTimeo     = 0x1006,
    So_Error        = 0x1007,
    So_Type         = 0x1008,

    Ip_Tos          = 3,
    Ip_Ttl          = 4,

    Tcp_NoDelay     = 1,
    Tcp_MaxSeg      = 2,
    Tcp_NoPush      = 4,
    Tcp_KeepIdle    = 256,
    Tcp_KeepIntvl   = 512,
    Tcp_KeepCnt     = 1024,
};

int SetSockOpt(int socket, Level level, Option optionName,
               const void* optionValue, uint32_t optionLen);
int GetSockOpt(int socket, Level level, Option optionName,
               void* optionValue, uint32_t* optionLen);

nn::Result Initialize(void* pOutSystemMemory, unsigned long systemMemorySize,
                      unsigned long allocatorPoolSize, int concurrencyLimit);
nn::Result Finalize();
bool       IsInitialized();

}} 

#endif /* NN_SOCKET_MIN_H */
