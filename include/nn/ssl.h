#ifndef NN_SSL_MIN_H
#define NN_SSL_MIN_H

#include <cstdint>
#include <cstddef>
#include "nn/result.h"

namespace nn { namespace ssl {

typedef uint64_t SslContextId;
typedef uint64_t SslConnectionId;
typedef uint64_t CertStoreId;

enum CertificateFormat {
    CertificateFormat_Pem = 0x01,
    CertificateFormat_Der = 0x02,
};

nn::Result Initialize();
nn::Result Finalize();

class Context {
protected:
    SslContextId m_ContextId;

public:
    enum ContextOption {
        ContextOption_None                     = 0x00,
        ContextOption_CrlImportDateCheckEnable = 0x01,
    };

    static const uint32_t ApiBitOffset   = 24;
    static const uint32_t ApiVersion     = 3;
    static const uint32_t ApiVersionFlag = ApiVersion << ApiBitOffset;

    enum class SslVersion : uint32_t {
        Auto   = 0x01 | ApiVersionFlag,
        TlsV10 = 0x08 | ApiVersionFlag,
        TlsV11 = 0x10 | ApiVersionFlag,
        TlsV12 = 0x20 | ApiVersionFlag,
        TlsV13 = 0x40 | ApiVersionFlag,
    };

    enum InternalPki {
        InternalPki_None                    = 0x00,
        InternalPki_DeviceClientCertDefault = 0x01,
    };

    nn::Result Create(SslVersion version);
    nn::Result Destroy();
    nn::Result ImportServerPki(CertStoreId* pOutCertId, const char* pInCertData,
                               uint32_t certDataSize, CertificateFormat certFormat);
    nn::Result ImportClientPki(CertStoreId* pOutCertId, const char* pInP12Data,
                               const char* pInPwData, uint32_t p12DataSize,
                               uint32_t pwDataSize);
    nn::Result ImportCrl(CertStoreId* pOutCrlId, const char* pInCrlDerData,
                         uint32_t crlDataSize);
    nn::Result RemovePki(CertStoreId certId);
    nn::Result GetContextId(SslContextId* pOutValue);
    nn::Result SetOption(ContextOption optionName, int optionValue);
    nn::Result GetOption(int* pOutValue, ContextOption optionName);
};

class Connection {
protected:
    SslConnectionId m_ConnectionId;
    SslContextId    m_ContextId;
    int             m_SockToClose;

private:
    uint32_t   m_certSize;
    uint32_t   m_certCount;
    char*      m_CertBuf;
    uint32_t   m_CertBufLen;
    nn::Result m_IoLastError;

public:
    enum class PollEvent : uint32_t {
        PollEvent_None   = 0x00,
        PollEvent_Read   = 0x01,
        PollEvent_Write  = 0x02,
        PollEvent_Except = 0x04,
    };

    enum class VerifyOption : uint32_t {
        VerifyOption_None      = 0x00,
        VerifyOption_PeerCa    = 0x01,
        VerifyOption_HostName  = 0x02,
        VerifyOption_DateCheck = 0x04,
        VerifyOption_Default   = (VerifyOption_PeerCa | VerifyOption_HostName),
        VerifyOption_All       = (VerifyOption_PeerCa | VerifyOption_HostName | VerifyOption_DateCheck),
    };

    enum IoMode {
        IoMode_Blocking    = 1,
        IoMode_NonBlocking = 2,
    };

    enum SessionCacheMode {
        SessionCacheMode_None             = 0,
        SessionCacheMode_SessionId        = 1,
        SessionCacheMode_SessionTicket    = 2,
    };

    enum OptionType {
        OptionType_DoNotCloseSocket   = 0,
        OptionType_GetServerCertChain = 1,
        OptionType_SkipDefaultVerify  = 2,
        OptionType_EnableAlpn         = 3,
    };

    enum AlpnProtoState {
        AlpnProtoState_NoSupport  = 0,
        AlpnProtoState_Negotiated = 1,
        AlpnProtoState_NoOverlap  = 2,
        AlpnProtoState_Selected   = 3,
        AlpnProtoState_EarlyValue = 4,
    };

    nn::Result Create(Context* pInSslContext);
    nn::Result Destroy();
    nn::Result SetSocketDescriptor(int socketDescriptor);
    nn::Result SetHostName(const char* pInHostName, uint32_t hostNameLength);
    nn::Result SetVerifyOption(VerifyOption optionValue);
    nn::Result SetIoMode(IoMode mode);
    nn::Result SetSessionCacheMode(SessionCacheMode mode);
    nn::Result GetSocketDescriptor(int* pOutValue);
    nn::Result DoHandshake();
    nn::Result DoHandshake(uint32_t* pOutServerCertSize, uint32_t* pOutServerCertCount);
    int        Read(char* pOutBuffer, uint32_t bufferLength);
    nn::Result Read(char* pOutBuffer, int* pOutReadSizeCourier, uint32_t bufferLength);
    int        Write(const char* pInBuffer, uint32_t bufferLength);
    nn::Result Write(const char* pInBuffer, int* pOutWrittenSizeCourier, uint32_t bufferLength);
    int        Pending();
    nn::Result Pending(int* pOutValue);
    nn::Result Peek(char* pOutBuffer, int* pOutReadSizeCourier, uint32_t bufferLength);
    nn::Result Poll(PollEvent* pOutEvent, PollEvent* pInEvent, uint32_t msecTimeout);
    nn::Result GetLastError(nn::Result* pOutValue);
    nn::Result GetVerifyCertError(nn::Result* pOutValue);
    nn::Result GetContextId(SslContextId* pOutValue);
    nn::Result GetConnectionId(SslConnectionId* pOutValue);
    nn::Result SetOption(OptionType optionType, bool enable);
    nn::Result GetOption(bool* pOutIsEnabled, OptionType optionType);
    nn::Result SetNextAlpnProto(const uint8_t* pInProtocols, uint32_t protocolBufLen);
    nn::Result GetNextAlpnProto(AlpnProtoState* pOutProtoState, uint8_t* pOutChosenProto,
                                uint32_t* pOutProtoLen, uint32_t inProtoBufLen);
};

}}

#endif /* NN_SSL_MIN_H */
