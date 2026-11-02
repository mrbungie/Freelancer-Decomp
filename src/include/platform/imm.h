// Shim for <imm.h>. There is no Windows IME here: the context is always null and every query
// reports "no IME", so imeui falls back to plain character input. A real IME bridge
// (SDL_TEXTINPUT / SDL_TEXTEDITING) belongs behind an interface in src/platform.
#ifndef FL_PLATFORM_IMM_H
#define FL_PLATFORM_IMM_H
#include "win_compat.h"
typedef void* HIMC;
typedef void* HIMCC;
typedef void* HKL;
#define GCS_COMPREADSTR 0x0001
#define GCS_COMPREADATTR 0x0002
#define GCS_COMPREADCLAUSE 0x0004
#define GCS_COMPSTR 0x0008
#define GCS_COMPATTR 0x0010
#define GCS_COMPCLAUSE 0x0020
#define GCS_CURSORPOS 0x0080
#define GCS_DELTASTART 0x0100
#define GCS_RESULTREADSTR 0x0200
#define GCS_RESULTSTR 0x0800
#define WM_IME_STARTCOMPOSITION 0x010D
#define WM_IME_ENDCOMPOSITION 0x010E
#define WM_IME_COMPOSITION 0x010F
#define WM_IME_SETCONTEXT 0x0281
#define WM_IME_NOTIFY 0x0282
#define WM_IME_CONTROL 0x0283
#define IMN_CLOSECANDIDATE 0x0004
#define IMN_OPENCANDIDATE 0x0005
#define IMN_CHANGECANDIDATE 0x0003
#define IMN_SETCONVERSIONMODE 0x0006
#define IMN_SETOPENSTATUS 0x0008
#define IMN_PRIVATE 0x000E
#define NI_COMPOSITIONSTR 0x0015
#define CPS_COMPLETE 0x0001
#define CPS_CANCEL 0x0004
#define IME_CMODE_NATIVE 0x0001
#define IME_CMODE_FULLSHAPE 0x0008
#define IME_SMODE_NONE 0x0000
typedef struct tagCANDIDATELIST { DWORD dwSize, dwStyle, dwCount, dwSelection, dwPageStart, dwPageSize, dwOffset[1]; } CANDIDATELIST, *LPCANDIDATELIST;
typedef struct tagCOMPOSITIONFORM { DWORD dwStyle; POINT ptCurrentPos; RECT rcArea; } COMPOSITIONFORM;
typedef struct tagCANDIDATEFORM { DWORD dwIndex, dwStyle; POINT ptCurrentPos; RECT rcArea; } CANDIDATEFORM;
inline HIMC ImmGetContext(HWND) { return nullptr; }
inline BOOL ImmReleaseContext(HWND, HIMC) { return TRUE; }
inline HIMC ImmAssociateContext(HWND, HIMC) { return nullptr; }
inline BOOL ImmIsIME(HKL) { return FALSE; }
inline BOOL ImmNotifyIME(HIMC, DWORD, DWORD, DWORD) { return FALSE; }
inline BOOL ImmGetOpenStatus(HIMC) { return FALSE; }
inline BOOL ImmGetConversionStatus(HIMC, DWORD* c, DWORD* s) { if (c) *c = 0; if (s) *s = 0; return FALSE; }
inline LONG ImmGetCompositionStringA(HIMC, DWORD, void*, DWORD) { return 0; }
inline LONG ImmGetCompositionStringW(HIMC, DWORD, void*, DWORD) { return 0; }
inline DWORD ImmGetCandidateListA(HIMC, DWORD, CANDIDATELIST*, DWORD) { return 0; }
inline DWORD ImmGetCandidateListW(HIMC, DWORD, CANDIDATELIST*, DWORD) { return 0; }
inline UINT ImmGetIMEFileNameA(HKL, char* b, UINT) { if (b) b[0] = 0; return 0; }
inline HWND ImmGetDefaultIMEWnd(HWND) { return nullptr; }
inline UINT ImmGetVirtualKey(HWND) { return 0; }
inline BOOL ImmDisableTextFrameService(DWORD) { return TRUE; }
inline void* ImmLockIMC(HIMC) { return nullptr; }
inline BOOL ImmUnlockIMC(HIMC) { return TRUE; }
inline void* ImmLockIMCC(HIMCC) { return nullptr; }
inline BOOL ImmUnlockIMCC(HIMCC) { return TRUE; }
#endif
