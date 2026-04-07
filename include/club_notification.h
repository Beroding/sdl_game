#ifndef CLUB_NOTIFICATION_H
#define CLUB_NOTIFICATION_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include <SDL3_image/SDL_image.h>

typedef enum {
    CLUB_NOTIFY_STATE_INACTIVE,
    CLUB_NOTIFY_STATE_SHOWING,
    CLUB_NOTIFY_STATE_FADING_OUT
} ClubNotifyState;

typedef struct {
    ClubNotifyState state;
    float timer;
    float alpha;
    float overlay_alpha;
    SDL_Texture *name_texture;
    SDL_Texture *desc_texture;
    SDL_Texture *prompt_texture;
    float pulse;
} ClubNotification;

// Functions
void club_notification_init(ClubNotification *notify);
void club_notification_start(ClubNotification *notify, SDL_Renderer *renderer, TTF_Font *font_bold, TTF_Font *font, TTF_Font *font_small);
void club_notification_update(ClubNotification *notify, float delta_time);
void club_notification_render(SDL_Renderer *renderer, ClubNotification *notify, int window_w, int window_h);
void club_notification_handle_input(ClubNotification *notify);
bool club_notification_is_active(ClubNotification *notify);
void club_notification_destroy(ClubNotification *notify);
void club_notification_cleanup(void);

#endif