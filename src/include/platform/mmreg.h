// Portable shim for <mmreg.h> (macOS build).
#ifndef FL_PLATFORM_MMREG_H
#define FL_PLATFORM_MMREG_H
#include "win_compat.h"
#include "mmsystem.h"

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif
#define WAVE_FORMAT_ADPCM 2
#define WAVE_FORMAT_IEEE_FLOAT 3
#define WAVE_FORMAT_MPEGLAYER3 0x55
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE

#pragma pack(push, 1)
typedef struct mpeglayer3waveformat_tag {
    WAVEFORMATEX wfx;
    WORD  wID;
    DWORD fdwFlags;
    WORD  nBlockSize;
    WORD  nFramesPerBlock;
    WORD  nCodecDelay;
} MPEGLAYER3WAVEFORMAT, *PMPEGLAYER3WAVEFORMAT, *LPMPEGLAYER3WAVEFORMAT;
#pragma pack(pop)
#define MPEGLAYER3_ID_MPEG 1
#define MPEGLAYER3_FLAG_PADDING_ISO 0
#endif
