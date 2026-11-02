// dplay8.h - DirectPlay 8 compatibility shim for non-Windows platforms
#pragma once

#include "win_compat.h"

typedef DWORD DPNHANDLE, *PDPNHANDLE;
typedef DWORD DPNID, *PDPNID;

typedef struct _DPN_APPLICATION_DESC {
    DWORD dwSize;
    DWORD dwFlags;
    GUID  guidInstance;
    GUID  guidApplication;
    DWORD dwMaxPlayers;
    DWORD dwCurrentPlayers;
    WCHAR* pwszSessionName;
    WCHAR* pwszPassword;
    void* pvReservedData;
    DWORD dwReservedDataSize;
    void* pvApplicationReservedData;
    DWORD dwApplicationReservedDataSize;
} DPN_APPLICATION_DESC, *PDPN_APPLICATION_DESC;

typedef struct _DPN_CONNECTION_INFO {
    DWORD dwSize;
    DWORD dwRoundTripLatencyMS;
    DWORD dwThroughputBPS;
    DWORD dwPeakThroughputBPS;
    DWORD dwBytesSentGuaranteed;
    DWORD dwPacketsSentGuaranteed;
    DWORD dwBytesSentNonGuaranteed;
    DWORD dwPacketsSentNonGuaranteed;
    DWORD dwBytesRetried;
    DWORD dwPacketsRetried;
    DWORD dwBytesDropped;
    DWORD dwPacketsDropped;
    DWORD dwMessagesSent;
    DWORD dwMessagesReceived;
} DPN_CONNECTION_INFO, *PDPN_CONNECTION_INFO;

typedef struct _DPNMSG_ENUM_HOSTS_RESPONSE {
    DWORD dwSize;
    void* pAddressSender;
    void* pAddressDevice;
    const DPN_APPLICATION_DESC* pApplicationDescription;
    void* pvResponseData;
    DWORD dwResponseDataSize;
    DWORD dwRoundTripLatencyMS;
} DPNMSG_ENUM_HOSTS_RESPONSE, *PDPNMSG_ENUM_HOSTS_RESPONSE;

struct IDirectPlay8Address : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE BuildFromURLA(CHAR* pszAddress) = 0;
};

struct IDirectPlay8Client : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Initialize(PVOID pUserContext, void* pfn, DWORD dwFlags) = 0;
};

struct IDirectPlay8Server : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Initialize(PVOID pUserContext, void* pfn, DWORD dwFlags) = 0;
};
