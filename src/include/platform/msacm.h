// Portable shim for <msacm.h> (macOS build).
// The Windows Audio Compression Manager does not exist here. The API is declared
// with the original struct shapes and implemented as "no driver installed": driver
// enumeration finds nothing, so Freelancer's MP3 streaming path reports an ACM
// init failure. TODO: back this with a software MP3/ADPCM decoder (e.g. dr_mp3).
#ifndef FL_PLATFORM_MSACM_H
#define FL_PLATFORM_MSACM_H
#include "win_compat.h"
#include "mmsystem.h"
#include "mmreg.h"

typedef void* HACMDRIVERID;
typedef void* HACMDRIVER;
typedef void* HACMSTREAM;

#define ACM_STREAMOPENF_NONREALTIME 0x00000004
#define ACM_STREAMSIZEF_SOURCE 0x00000000
#define ACM_STREAMSIZEF_DESTINATION 0x00000001
#define ACM_STREAMCONVERTF_END 0x00000020
#define ACM_FORMATSUGGESTF_WFORMATTAG 0x00010000

#define ACMDRIVERDETAILS_SHORTNAME_CHARS 32
#define ACMDRIVERDETAILS_LONGNAME_CHARS 128
#define ACMDRIVERDETAILS_COPYRIGHT_CHARS 80
#define ACMDRIVERDETAILS_LICENSING_CHARS 128
#define ACMDRIVERDETAILS_FEATURES_CHARS 512

typedef struct tACMDRIVERDETAILSA {
    DWORD cbStruct;
    DWORD fccType;
    DWORD fccComp;
    WORD  wMid;
    WORD  wPid;
    DWORD vdwACM;
    DWORD vdwDriver;
    DWORD fdwSupport;
    DWORD cFormatTags;
    DWORD cFilterTags;
    DWORD hicon_;           // HICON is 32-bit here to keep the 0x398 layout
    char  szShortName[ACMDRIVERDETAILS_SHORTNAME_CHARS];
    char  szLongName[ACMDRIVERDETAILS_LONGNAME_CHARS];
    char  szCopyright[ACMDRIVERDETAILS_COPYRIGHT_CHARS];
    char  szLicensing[ACMDRIVERDETAILS_LICENSING_CHARS];
    char  szFeatures[ACMDRIVERDETAILS_FEATURES_CHARS];
} ACMDRIVERDETAILSA, *PACMDRIVERDETAILSA;
typedef ACMDRIVERDETAILSA ACMDRIVERDETAILS;

typedef struct tACMSTREAMHEADER {
    DWORD     cbStruct;
    DWORD     fdwStatus;
    DWORD_PTR dwUser;
    LPBYTE    pbSrc;
    DWORD     cbSrcLength;
    DWORD     cbSrcLengthUsed;
    DWORD_PTR dwSrcUser;
    LPBYTE    pbDst;
    DWORD     cbDstLength;
    DWORD     cbDstLengthUsed;
    DWORD_PTR dwDstUser;
    DWORD     dwReservedDriver[10];
} ACMSTREAMHEADER, *PACMSTREAMHEADER;

typedef BOOL (CALLBACK *ACMDRIVERENUMCB)(HACMDRIVERID, DWORD_PTR, DWORD);

inline MMRESULT acmDriverEnum(ACMDRIVERENUMCB, DWORD_PTR, DWORD) { return MMSYSERR_NOERROR; }
inline MMRESULT acmDriverDetailsA(HACMDRIVERID, ACMDRIVERDETAILSA*, DWORD) { return MMSYSERR_NODRIVER; }
inline MMRESULT acmDriverOpen(HACMDRIVER*, HACMDRIVERID, DWORD) { return MMSYSERR_NODRIVER; }
inline MMRESULT acmFormatSuggest(HACMDRIVER, WAVEFORMATEX*, WAVEFORMATEX*, DWORD, DWORD) { return MMSYSERR_NODRIVER; }
inline MMRESULT acmStreamOpen(HACMSTREAM*, HACMDRIVER, WAVEFORMATEX*, WAVEFORMATEX*, void*, DWORD_PTR, DWORD_PTR, DWORD) { return MMSYSERR_NODRIVER; }
inline MMRESULT acmStreamClose(HACMSTREAM, DWORD) { return MMSYSERR_NOERROR; }
inline MMRESULT acmStreamSize(HACMSTREAM, DWORD, DWORD*, DWORD) { return MMSYSERR_NODRIVER; }
inline MMRESULT acmStreamPrepareHeader(HACMSTREAM, ACMSTREAMHEADER*, DWORD) { return MMSYSERR_NODRIVER; }
inline MMRESULT acmStreamUnprepareHeader(HACMSTREAM, ACMSTREAMHEADER*, DWORD) { return MMSYSERR_NODRIVER; }
inline MMRESULT acmStreamConvert(HACMSTREAM, ACMSTREAMHEADER*, DWORD) { return MMSYSERR_NODRIVER; }
#endif
