#include "../include/sfx_system.h"
#include <stdio.h>
#include <stdlib.h>

SFXSystem* sfx_create(MIX_Mixer *mixer) {
    SFXSystem *sfx = calloc(1, sizeof(SFXSystem));
    if (!sfx) return NULL;
    sfx->mixer = mixer;
    
    if (mixer) {
        // --------------------------------------------------------------------
        // Load footstep sound (WAV)
        // --------------------------------------------------------------------
        sfx->footstep = MIX_LoadAudio(mixer, "game_assets/sfx/footstep2.wav", true);
        if (!sfx->footstep) {
            printf("SFX: footstep2.wav failed (%s)\n", SDL_GetError());
        } else {
            sfx->footstep_track = MIX_CreateTrack(mixer);
            if (!sfx->footstep_track) {
                printf("SFX: Failed to create footstep track\n");
            } else {
                MIX_SetTrackAudio(sfx->footstep_track, sfx->footstep);
            }
        }

        // --------------------------------------------------------------------
        // Load laugh sound (MP3)
        // --------------------------------------------------------------------
        sfx->laugh = MIX_LoadAudio(mixer, "game_assets/sfx/naberius_laugh.mp3", true);
        if (!sfx->laugh) {
            printf("SFX: laugh.mp3 failed (%s)\n", SDL_GetError());
        } else {
            sfx->laugh_track = MIX_CreateTrack(mixer);
            if (!sfx->laugh_track) {
                printf("SFX: Failed to create laugh track\n");
            } else {
                if (!MIX_SetTrackAudio(sfx->laugh_track, sfx->laugh)) {
                    printf("SFX: Failed to set laugh audio on track\n");
                }
                MIX_SetTrackGain(sfx->laugh_track, 0.8f);
                printf("SFX: Laugh sound loaded and track ready\n");
            }
        }

        // --------------------------------------------------------------------
        // Load glass break sound (MP3)
        // --------------------------------------------------------------------
        sfx->glass_break = MIX_LoadAudio(mixer, "game_assets/sfx/glass_break.mp3", true);
        if (!sfx->glass_break) {
            printf("SFX: glass_break.mp3 failed (%s)\n", SDL_GetError());
        } else {
            sfx->glass_break_track = MIX_CreateTrack(mixer);
            if (!sfx->glass_break_track) {
                printf("SFX: Failed to create glass break track\n");
            } else {
                MIX_SetTrackAudio(sfx->glass_break_track, sfx->glass_break);
                MIX_SetTrackGain(sfx->glass_break_track, 0.8f);
                printf("SFX: Glass break sound loaded and track ready\n");
            }
        }
    }
    
    // Final status report
    printf("SFX: footstep %s, glass break %s, laugh %s\n",
           sfx->footstep ? "loaded" : "MISSING",
           sfx->glass_break ? "loaded" : "MISSING",
           sfx->laugh ? "loaded" : "MISSING");
    return sfx;
}

void sfx_destroy(SFXSystem *sfx) {
    if (!sfx) return;
    if (sfx->footstep_track) MIX_DestroyTrack(sfx->footstep_track);
    if (sfx->footstep) MIX_DestroyAudio(sfx->footstep);
    if (sfx->laugh_track) MIX_DestroyTrack(sfx->laugh_track);
    if (sfx->laugh) MIX_DestroyAudio(sfx->laugh);
    if (sfx->glass_break_track) MIX_DestroyTrack(sfx->glass_break_track);
    if (sfx->glass_break) MIX_DestroyAudio(sfx->glass_break);
    free(sfx);
}

void sfx_play_footstep_loop(SFXSystem *sfx) {
    if (!sfx || !sfx->footstep_track) return;
    MIX_StopTrack(sfx->footstep_track, 0);
    MIX_PlayTrack(sfx->footstep_track, -1);
}

void sfx_stop_footstep(SFXSystem *sfx) {
    if (!sfx || !sfx->footstep_track) return;
    MIX_StopTrack(sfx->footstep_track, 0);
}

bool sfx_is_footstep_playing(SFXSystem *sfx) {
    if (!sfx || !sfx->footstep_track) return false;
    return MIX_TrackPlaying(sfx->footstep_track) != 0;
}

void sfx_play_laugh(SFXSystem *sfx) {
    if (!sfx || !sfx->laugh_track) {
        printf("sfx_play_laugh: laugh track not available\n");
        return;
    }
    MIX_StopTrack(sfx->laugh_track, 0);
    if (!MIX_PlayTrack(sfx->laugh_track, 0)) {
        printf("sfx_play_laugh: MIX_PlayTrack failed: %s\n", SDL_GetError());
    } else {
        printf("sfx_play_laugh: playing\n");
    }
}

void sfx_play_glass_break(SFXSystem *sfx) {
    if (!sfx || !sfx->glass_break_track) {
        printf("sfx_play_glass_break: glass break track not available\n");
        return;
    }
    MIX_StopTrack(sfx->glass_break_track, 0);
    MIX_PlayTrack(sfx->glass_break_track, 0);
}