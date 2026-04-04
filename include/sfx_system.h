#ifndef SFX_SYSTEM_H
#define SFX_SYSTEM_H

#include <SDL3_mixer/SDL_mixer.h>
#include <stdbool.h>

typedef struct {
    MIX_Mixer *mixer;
    MIX_Audio *footstep;
    MIX_Track *footstep_track;   // persistent track for footsteps
    MIX_Audio *laugh;
    MIX_Track *laugh_track;
    MIX_Audio *glass_break;
    MIX_Track *glass_break_track;
} SFXSystem;

SFXSystem* sfx_create(MIX_Mixer *mixer);
void sfx_destroy(SFXSystem *sfx);
void sfx_play_footstep_loop(SFXSystem *sfx);   // start looping
void sfx_stop_footstep(SFXSystem *sfx);        // stop immediately
bool sfx_is_footstep_playing(SFXSystem *sfx);  // check if track is active
void sfx_play_laugh(SFXSystem *sfx);
void sfx_play_glass_break(SFXSystem *sfx);

#endif