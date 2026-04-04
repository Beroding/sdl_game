#ifndef MUSIC_SYSTEM_H
#define MUSIC_SYSTEM_H

#include <SDL3_mixer/SDL_mixer.h>
#include <stdbool.h>

typedef enum {
    MUSIC_NONE,
    MUSIC_MENU,
    MUSIC_GAME,
    MUSIC_BATTLE
} MusicType;

typedef struct {
    MIX_Mixer *mixer;           // The mixer device
    MIX_Audio *menu_music;      // Loaded audio data
    MIX_Audio *game_music;
    MIX_Audio *battle_music;
    
    MIX_Track *current_track;   // Currently playing track
    
    float volume;               // 0.0 to 1.0
    bool is_playing;
    bool is_muted;

    MusicType current_music;   // what is currently playing
    bool missing_warning_shown[3]; 
} MusicSystem;

// Initialize SDL_mixer and load music files
MusicSystem* music_system_create(void);

// Destroy music system and free resources
void music_system_destroy(MusicSystem *music);

// Play specific tracks
void music_play_menu(MusicSystem *music);
void music_play_game(MusicSystem *music);
void music_play_battle(MusicSystem *music);

// Control playback
void music_stop(MusicSystem *music);
void music_pause(MusicSystem *music);
void music_resume(MusicSystem *music);
void music_set_volume(MusicSystem *music, float volume);  // 0.0 to 1.0
void music_toggle_mute(MusicSystem *music);

#endif