#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <SDL3/SDL.h>
#include <stdbool.h>

/* Video player - plays opening cutscene before battle */
typedef struct {
    char *filename;              /* Path to video file */
    
    /* SDL video playback (currently uses fallback) */
    SDL_IOStream *io;
    SDL_Surface *frame_surface;
    SDL_Texture *frame_texture;
    
    /* Playback state */
    bool playing;
    bool finished;
    float duration;      /* Total length in seconds */
    float current_time;  /* Current playback position */
    
    /* Video properties */
    int frame_width;
    int frame_height;
    int fps;
    
    /* Fallback animation (used when no video file exists) */
    bool use_fallback;
    float fallback_timer;
    
} VideoPlayer;

/* Create and destroy */
VideoPlayer* video_player_create(SDL_Renderer *renderer, const char *filename);
void video_player_destroy(VideoPlayer *player);

/* Update and draw */
void video_player_update(VideoPlayer *player, float delta_time);
void video_player_render(SDL_Renderer *renderer, VideoPlayer *player);

/* Check status and skip */
bool video_player_is_finished(VideoPlayer *player);
void video_player_skip(VideoPlayer *player);

#endif