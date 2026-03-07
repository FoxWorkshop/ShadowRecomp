
const int32_t ppDSAddr = 0x550310;
const int32_t ppDSEventAddr = 0x5611C0;

//#define XAUDIO_SAMPLES_HZ     24000      
#define XAUDIO_SAMPLES_HZ     48000 
#define XAUDIO_NUM_CHANNELS   2          
#define XAUDIO_SAMPLE_BITS    16         
#define XAUDIO_NUM_SAMPLES    256        

// EmuIDirectSoundStream_GetStatus flags
#define X_DSSSTATUS_READY             0x00000001
#define X_DSSSTATUS_PLAYING           0x00010000
#define X_DSSSTATUS_PAUSED            0x00020000
#define X_DSSSTATUS_STARVED           0x00040000
#define X_DSSSTATUS_ENVELOPECOMPLETE  0x00080000

// EmuIDirectSoundBuffer_GetStatus flags
#define X_DSBSTATUS_PLAYING           0x00000001
#define X_DSBSTATUS_PAUSED            0x00000002
#define X_DSBSTATUS_LOOPING           0x00000004

#define X_DSBPLAY_LOOPING       0x00000001
#define X_DSBPLAY_FROMSTART     0x00000002

#define DSERR_INVALIDPARAM              E_INVALIDARG
#define DSBLOCK_FROMWRITECURSOR     0x00000001
#define DSBLOCK_ENTIREBUFFER        0x00000002


#define _FACDS  0x878   /* DirectSound's facility code */
#define MAKE_DSHRESULT(code)  MAKE_HRESULT(1, _FACDS, code)
#define XTL_DSBCAPS_CTRL3D               0x00000010
#define DSERR_UNINITIALIZED             MAKE_DSHRESULT(170)
#define DSE_FLAG_BUFFER_EXTERNAL        (1 << 31)
#define DSE_FLAG_PAUSE                  (1 << 11)
#define DSBSTATUS_LOOPING           0x00000004

#define DSBSIZE_MIN                 4

#define DSE_FLAG_PCM                    (1 << 0)
#define DSE_FLAG_XADPCM                 (1 << 1)
#define DSE_FLAG_PCM_UNKNOWN            (1 << 2)
#define DSBCAPS_CTRL3D              0x00000010

#define WAVE_FORMAT_ADPCM 0x0002
#define WAVE_FORMAT_XBOX_ADPCM 0x0069

// IDirectSoundBuffer_StopEx flags
#define X_DSBSTOPEX_IMMEDIATE         0x00000000
#define X_DSBSTOPEX_ENVELOPE          0x00000001
#define X_DSBSTOPEX_RELEASEWAVEFORM   0x00000002
#define X_DSBSTOPEX_ALL               (X_DSBSTOPEX_ENVELOPE | X_DSBSTOPEX_RELEASEWAVEFORM)

typedef struct _XDSMIXBINVOLUMEPAIR {
    uint32_t       dwMixBin;
    int32_t        lVolume;
} X_DSMIXBINVOLUMEPAIR, * X_LPDSMIXBINVOLUMEPAIR;

typedef struct _XDSMIXBINS {
    uint32_t                       dwCount;
    uint32_t     lpMixBinVolumePairs; //pointer X_DSMIXBINVOLUMEPAIR
} X_DSMIXBINS, * X_LPDSMIXBINS;

union X_DSMIXBINBUNION {
    uint32_t      dwMixBinMask;
    uint32_t pMixBins; //pointer X_LPDSMIXBINS
};

struct X_DSBUFFERDESC
{
 
    uint32_t           dwSize;
    uint32_t           dwFlags;
    uint32_t           dwBufferBytes;
    //uint32_t  lpwfxFormat; //pointer LPWAVEFORMATEX
    xpointer<WAVEFORMATEX>  lpwfxFormat; //pointer LPWAVEFORMATEX
    xpointer<X_DSMIXBINBUNION> mixBinsOutput; //pointer X_DSMIXBINBUNION
    uint32_t           dwInputMixBin;
};

