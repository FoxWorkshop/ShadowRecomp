
#define XAUDIO_SAMPLES_HZ     24000      
#define XAUDIO_NUM_CHANNELS   2          
#define XAUDIO_SAMPLE_BITS    16         
#define XAUDIO_NUM_SAMPLES    256        

// EmuIDirectSoundStream_GetStatus flags
#define X_DSSSTATUS_READY             0x00000001
#define X_DSSSTATUS_PLAYING           0x00010000
#define X_DSSSTATUS_PAUSED            0x00020000
#define X_DSSSTATUS_STARVED           0x00040000
#define X_DSSSTATUS_ENVELOPECOMPLETE  0x00080000


void initSDL2Driver();
void AudioQuit();