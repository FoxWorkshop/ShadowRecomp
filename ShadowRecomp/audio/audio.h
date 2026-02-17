
const int32_t ppDSAddr = 0x550310;
const int32_t ppDSEventAddr = 0x5611C0;

//#define XAUDIO_SAMPLES_HZ     24000      
#define XAUDIO_SAMPLES_HZ     44100      
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

#define _FACDS  0x878   /* DirectSound's facility code */
#define MAKE_DSHRESULT(code)  MAKE_HRESULT(1, _FACDS, code)
#define XTL_DSBCAPS_CTRL3D               0x00000010
#define DSERR_UNINITIALIZED             MAKE_DSHRESULT(170)

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


void initSDL2Driver();
void AudioQuit();