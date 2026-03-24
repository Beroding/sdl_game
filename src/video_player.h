#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <SDL3/SDL.h>
#include <stdbool.h>

typedef struct {
    // Video file path
    char *filename;
    
    // SDL Video playback
    SDL_IOStream *io;
    SDL_Surface *frame_surface;
    SDL_Texture *frame_texture;
    
    // Animation state
    bool playing;
    bool finished;
    float duration;
    float current_time;
    
    // For software decoding fallback
    int frame_width;
    int frame_height;
    int fps;
    
    // Simulated video (if no actual video file)
    bool use_fallback;
    float fallback_timer;
    
} VideoPlayer;

VideoPlayer* video_player_create(SDL_Renderer *renderer, const char *filename);
void video_player_destroy(VideoPlayer *player);
void video_player_update(VideoPlayer *player, float delta_time);
void video_player_render(SDL_Renderer *renderer, VideoPlayer *player);
bool video_player_is_finished(VideoPlayer *player);
void video_player_skip(VideoPlayer *player);

#endif