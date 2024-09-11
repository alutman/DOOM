
#include <SDL.h>

#include "i_system.h"
#include "i_sound.h"
#include "w_wad.h"

extern int	numChannels;
#define SND_MAX_VOL 15

#define SND_FREQ 11025
#define SND_SAMPLES  32
#define SND_CHANNELS 2

typedef struct
{
  char		header[4];
  int		num_samples;
  byte		data;

} pcm_data;

typedef struct {
  int		vol;
  int		sep;
  int		pitch;
  int   priority;
  int   sfxid;
  int   data_idx;

  sfxinfo_t* sfx;
} audio_channel;


audio_channel* audio_channels;
SDL_AudioDeviceID audio_device;

// process volume and separation for a sample
void processSample(uint8_t sample, const int vol, const int sep, uint8_t* left, uint8_t* right) {
  // 128 is the midpoint for unsigned 8 bit PCM audio
  // sample = sample - ((128 - sample) * factor);
  float v_factor = (float)vol/SND_MAX_VOL;

  // middle is 128
  // 128-255 right full
  // 0-128 left full
  // right scales 0 -> full from 0 - 128
  // left scales 0 -> full from 128 - 256
  float rightFactor = sep / 256.0 * 2;
  if (rightFactor > 1.0)
    rightFactor = 1.0;
  float leftFactor = 256.0 / sep / 2;
  if (leftFactor > 1.0)
    leftFactor = 1.0;

  uint8_t l_sample = 128 - ((128 - sample) * v_factor * leftFactor);
  uint8_t r_sample = 128 - ((128 - sample) * v_factor * rightFactor);

  *left = l_sample;
  *right = r_sample;
}

// cap the value of a mixed sample and convert back to unsigned 8 bit
uint8_t capMix(int16_t mix) {
  if (mix > 128) {
    mix = (float)mix * ((float)128/mix);
  }

  if (mix < -128) {
    mix = (float)mix * ((float)-128/mix);
  }

  mix += 128;

  return mix > 255 ? 255 : mix < 0 ? 0 : mix;
}


void audio_callback(void* userdata, Uint8* stream, int len) {
  for(int l = 0; l < len; l += 2) {
    int16_t mixed_l = 0;
    int16_t mixed_r = 0;

    // read in all active channel data
    for(int i = 0; i < numChannels; i++) {
      audio_channel* ac = &audio_channels[i+1];
      if (ac->sfxid == 0) {
        // sfxid 0 is a dummy, channel not active
        continue;
      }

      pcm_data* sfxData = ac->sfx->data;
      if (ac->data_idx < sfxData->num_samples) {
        uint8_t sample = (&sfxData->data)[ac->data_idx];
        uint8_t left;
        uint8_t right;
        ac->data_idx += 1;
        processSample(sample, ac->vol, ac->sep, &left, &right);

        // move to signed and add to the mix
        mixed_l += left - 128;
        mixed_r += right - 128;

      } else {
        ac->sfxid = 0;
      }
    }


    stream[l] = capMix(mixed_l);
    stream[l + 1] = capMix(mixed_r);
  }
}


// Init at program start...
void I_InitSound() {
  SDL_AudioSpec wanted_spec;
  SDL_AudioSpec obtained_spec;


  if(SDL_Init(SDL_INIT_AUDIO) < 0) {
    I_Error("init sound fail: %s\n", SDL_GetError());
  }

  // don't use the zero index as its the ID
  audio_channels = malloc(sizeof(audio_channel) * (numChannels+1));\
  if (audio_channels == NULL) {
    I_Error("failed to allocate audio channels");
  }
  memset(audio_channels, 0, sizeof(audio_channel) * (numChannels+1));

  SDL_zero(wanted_spec);

  wanted_spec.freq = SND_FREQ;
  wanted_spec.format = AUDIO_U8; // 8-bit unsigned PCM
  wanted_spec.channels = SND_CHANNELS;
  wanted_spec.samples = SND_SAMPLES; // Buffer size
  wanted_spec.callback = audio_callback;

  audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &obtained_spec, 0);
  if (audio_device == 0) {
    I_Error("SDL_OpenAudioDevice fail: %s\n", SDL_GetError());
  }

  if (obtained_spec.freq != wanted_spec.freq) {
    I_Error("SDL_OpenAudioDevice fail freq: %i\n", obtained_spec.freq);
  }
  if (obtained_spec.format != wanted_spec.format) {
    I_Error("SDL_OpenAudioDevice fail format: %i\n", obtained_spec.format);
  }
  if (obtained_spec.channels != wanted_spec.channels) {
    I_Error("SDL_OpenAudioDevice fail channels: %i\n", obtained_spec.channels);
  }
  if (obtained_spec.samples != wanted_spec.samples) {
    I_Error("SDL_OpenAudioDevice fail samples: %i\n", obtained_spec.samples);
  }

  SDL_PauseAudioDevice(audio_device, 0);
}

// ... update sound buffer and audio device at runtime...
void I_UpdateSound(void) {}
void I_SubmitSound(void){}

// ... shut down and relase at program termination.
void I_ShutdownSound(void) {
  SDL_CloseAudioDevice(audio_device);
  free(audio_channels);
}


//
//  SFX I/O
//

// Initialize channels?
void I_SetChannels(){}

// Get raw data lump index for sound descriptor.
int I_GetSfxLumpNum (sfxinfo_t* sfx) {
  char namebuf[9];
  sprintf(namebuf, "ds%s", sfx->name);
  return W_GetNumForName(namebuf);
}


int
I_StartSound
( int		sfxid,
  int		vol,
  int		sep,
  int		pitch,
  int		priority ) {

  // limit sound effects to one for mostly the chainsaw
  if ( sfxid == sfx_sawup
    || sfxid == sfx_sawidl
    || sfxid == sfx_sawful
    || sfxid == sfx_sawhit
    || sfxid == sfx_stnmov
    ) {
    for (int i=0; i < numChannels; i++)
    {
      audio_channel* ac = &audio_channels[i+1];
      if (ac->sfxid == sfxid) {
        ac->sfxid = 0;
        break;
      }
    }
  }


  // look for empty channel
  for(int i = 0; i < numChannels; i++) {
    audio_channel* ac = &audio_channels[i+1];
    if (ac->sfxid == 0) {
      ac->vol = vol;
      ac->sep = sep;
      ac->sfxid = sfxid;
      ac->pitch = pitch;
      ac->priority = priority;
      ac->data_idx = 0;
      ac->sfx  = &S_sfx[sfxid];
      return i+1;
    }
  }

  return -1;
}

// Stops a sound channel.
// Sounds bugged when this does something
void I_StopSound(int handle) {
  if (handle <= 0) return;
  audio_channel* ac = &audio_channels[handle];
  if (ac->sfxid != 0) {
    ac->sfxid = 0;
  }
}

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle) {
  if (handle <= 0) return 0;
  audio_channel* ac = &audio_channels[handle];
  return ac->sfxid != 0;
}

// Updates the volume, separation,
//  and pitch of a sound channel.
void
I_UpdateSoundParams
( int		handle,
  int		vol,
  int		sep,
  int		pitch ) {
  if (handle <= 0) return;

  audio_channel* ac = &audio_channels[handle];
  ac->vol = vol;
  ac->sep = sep;
  ac->pitch = pitch;
}


//
//  MUSIC I/O
//
void I_InitMusic(void){}
void I_ShutdownMusic(void){}
// Volume.
void I_SetMusicVolume(int volume){}
// PAUSE game handling.
void I_PauseSong(int handle){}
void I_ResumeSong(int handle){}
// Registers a song handle to song data.
int I_RegisterSong(void *data) {
  return 0;
}
// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void
I_PlaySong
( int		handle,
  int		looping ){}
// Stops a song over 3 seconds.
void I_StopSong(int handle){}
// See above (register), then think backwards
void I_UnRegisterSong(int handle){}
