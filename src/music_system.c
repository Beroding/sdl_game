#include <stdio.h>
#include <stdlib.h>
#include "../include/music_system.h"

#define DEFAULT_VOLUME 0.7f

MusicSystem* music_system_create(void) {
    MusicSystem *music = calloc(1, sizeof(MusicSystem));
    if (!music) return NULL;
    
    // Initialize SDL_mixer
    if (!MIX_Init()) {
        fprintf(stderr, "SDL_mixer init failed: %s\n", SDL_GetError());
        free(music);
        return NULL;
    }
    
    // Create mixer device (default audio device, stereo, 44100Hz)
    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_S16,
        .channels = 2,
        .freq = 44100
    };
    
    music->mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!music->mixer) {
        fprintf(stderr, "Failed to create mixer: %s\n", SDL_GetError());
        MIX_Quit();
        free(music);
        return NULL;
    }
    
    // Create a track for music playback
    music->current_track = MIX_CreateTrack(music->mixer);
    if (!music->current_track) {
        fprintf(stderr, "Failed to create track: %s\n", SDL_GetError());
    }
    
    music->current_music = MUSIC_NONE;
    music->volume = DEFAULT_VOLUME;
    music->is_playing = false;
    music->is_muted = false;
    
    // Set initial volume
    if (music->current_track) {
        MIX_SetTrackGain(music->current_track, music->volume);
    }
    
    // Try to load music files - but don't fail if they don't load
    // The user can fix the files later, game should still run
    printf("Loading music files...\n");
    
    music->menu_music = MIX_LoadAudio(music->mixer, "game_assets/music/menu_theme.mp3", false);
    if (!music->menu_music) {
        printf("Warning: Could not load menu_theme.mp3: %s\n", SDL_GetError());
        printf("  -> Make sure the file exists and SDL3_mixer decoders are installed\n");
    }
    
    music->game_music = MIX_LoadAudio(music->mixer, "game_assets/music/exploration.ogg", false);
    if (!music->game_music) {
        printf("Warning: Could not load exploration.ogg: %s\n", SDL_GetError());
    }
    
    music->battle_music = MIX_LoadAudio(music->mixer, "game_assets/music/battle_theme.mp3", false);
    if (!music->battle_music) {
        printf("Warning: Could not load battle_theme.mp3: %s\n", SDL_GetError());
    }

    music->credit_music = MIX_LoadAudio(music->mixer, "game_assets/music/credit_theme.mp3", false);
    if (!music->credit_music){
        printf("Warning: Could not load credit_theme.mp3: %s\n", SDL_GetError());
    }
    
    printf("Music system initialized (some tracks may be unavailable)\n");
    return music;
}

void music_system_destroy(MusicSystem *music) {
    if (!music) return;
    
    // Stop playback
    if (music->current_track) {
        MIX_StopTrack(music->current_track, 0);
        MIX_DestroyTrack(music->current_track);
    }
    
    // Free audio data
    if (music->menu_music) MIX_DestroyAudio(music->menu_music);
    if (music->game_music) MIX_DestroyAudio(music->game_music);
    if (music->battle_music) MIX_DestroyAudio(music->battle_music);
    if(music->credit_music) MIX_DestroyAudio(music->credit_music);
    
    // Destroy mixer and quit
    if (music->mixer) MIX_DestroyMixer(music->mixer);
    MIX_Quit();
    
    free(music);
    printf("Music system destroyed\n");
}

static void music_play_audio(MusicSystem *music, MIX_Audio *audio) {
    if (!music || !audio || !music->current_track) return;
    
    MIX_StopTrack(music->current_track, 0);
    
    if (MIX_SetTrackAudio(music->current_track, audio)) {
        // Try simpler play without explicit properties
        if (MIX_PlayTrack(music->current_track, 0)) {  // NULL or 0 instead of props
            music->is_playing = true;
            printf("Started playing track\n");
        } else {
            printf("MIX_PlayTrack failed: %s\n", SDL_GetError());
        }
    } else {
        printf("MIX_SetTrackAudio failed: %s\n", SDL_GetError());
    }
}

void music_play_menu(MusicSystem *music) {
    if (!music) return;
    if (music->current_music == MUSIC_MENU) return;  // already playing
    
    if (!music->menu_music) {
        static bool warned = false;
        if (!warned) {
            printf("Menu music not available (file missing)\n");
            warned = true;
        }
        return;
    }
    music_play_audio(music, music->menu_music);
    music->current_music = MUSIC_MENU;
}

void music_play_game(MusicSystem *music) {
    if (!music) return;
    if (music->current_music == MUSIC_GAME) return;
    
    if (!music->game_music) {
        static bool warned = false;
        if (!warned) {
            printf("Game music not available (file missing)\n");
            warned = true;
        }
        return;
    }
    music_play_audio(music, music->game_music);
    music->current_music = MUSIC_GAME;
}

void music_play_battle(MusicSystem *music) {
    if (!music) return;
    if (music->current_music == MUSIC_BATTLE) return;
    
    if (!music->battle_music) {
        static bool warned = false;
        if (!warned) {
            printf("Battle music not available (file missing)\n");
            warned = true;
        }
        return;
    }
    music_play_audio(music, music->battle_music);
    music->current_music = MUSIC_BATTLE;
}

void music_play_credits(MusicSystem *music) {
    if (!music) return;
    if (music->current_music == MUSIC_CREDIT) return;  // already playing
    
    if (!music->credit_music) {
        static bool warned = false;
        if (!warned) {
            printf("Credit music not available (file missing)\n");
            warned = true;
        }
        return;
    }
    music_play_audio(music, music->credit_music);
    music->current_music = MUSIC_CREDIT;
}

void music_stop(MusicSystem *music) {
    if (!music || !music->current_track) return;
    MIX_StopTrack(music->current_track, 0);
    music->is_playing = false;
}

void music_pause(MusicSystem *music) {
    if (!music || !music->current_track) return;
    MIX_PauseTrack(music->current_track);
}

void music_resume(MusicSystem *music) {
    if (!music || !music->current_track) return;
    MIX_ResumeTrack(music->current_track);
}

void music_set_volume(MusicSystem *music, float volume) {
    if (!music) return;
    music->volume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    
    if (music->current_track) {
        MIX_SetTrackGain(music->current_track, music->is_muted ? 0.0f : music->volume);
    }
}

void music_toggle_mute(MusicSystem *music) {
    if (!music) return;
    music->is_muted = !music->is_muted;
    
    if (music->current_track) {
        MIX_SetTrackGain(music->current_track, music->is_muted ? 0.0f : music->volume);
    }
}