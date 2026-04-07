#include "../include/club_notification.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Global texture for the club image (loaded once, reused)
static SDL_Texture *club_texture = NULL;

void club_notification_init(ClubNotification *notify) {
    memset(notify, 0, sizeof(ClubNotification));
    notify->state = CLUB_NOTIFY_STATE_INACTIVE;
}

void club_notification_start(ClubNotification *notify, SDL_Renderer *renderer, 
                             TTF_Font *font_bold, TTF_Font *font, TTF_Font *font_small) {
    if (!notify || !renderer) return;
    
    // Clean up old textures
    if (notify->name_texture) SDL_DestroyTexture(notify->name_texture);
    if (notify->desc_texture) SDL_DestroyTexture(notify->desc_texture);
    if (notify->prompt_texture) SDL_DestroyTexture(notify->prompt_texture);
    
    // Load club image if not already loaded
    if (!club_texture) {
        SDL_Surface *club_surf = IMG_Load("game_assets/club.png");
        if (club_surf) {
            club_texture = SDL_CreateTextureFromSurface(renderer, club_surf);
            SDL_SetTextureScaleMode(club_texture, SDL_SCALEMODE_NEAREST);
            SDL_DestroySurface(club_surf);
            printf("Club image loaded successfully\n");
        } else {
            printf("Failed to load club.png: %s\n", SDL_GetError());
        }
    }
    
    notify->state = CLUB_NOTIFY_STATE_SHOWING;
    notify->timer = 0.0f;
    notify->alpha = 0.0f;
    notify->overlay_alpha = 0.0f;
    notify->pulse = 0.0f;
    
    // Create "CLUB OBTAINED!" text - BIG and GOLD
    SDL_Color gold_color = {255, 215, 0, 255};
    SDL_Surface *name_surf = TTF_RenderText_Blended(font_bold, "CLUB OBTAINED!", 0, gold_color);
    if (name_surf) {
        notify->name_texture = SDL_CreateTextureFromSurface(renderer, name_surf);
        SDL_DestroySurface(name_surf);
    }
    
    // Create description
    SDL_Color white_color = {255, 255, 255, 255};
    SDL_Surface *desc_surf = TTF_RenderText_Blended(font, "A sturdy wooden club for self-defense", 0, white_color);
    if (desc_surf) {
        notify->desc_texture = SDL_CreateTextureFromSurface(renderer, desc_surf);
        SDL_DestroySurface(desc_surf);
    }
    
    // Create prompt text
    SDL_Color prompt_color = {200, 200, 200, 255};
    SDL_Surface *prompt_surf = TTF_RenderText_Blended(font_small, "Press E or Click to continue", 0, prompt_color);
    if (prompt_surf) {
        notify->prompt_texture = SDL_CreateTextureFromSurface(renderer, prompt_surf);
        SDL_DestroySurface(prompt_surf);
    }
    
    printf("Club notification started\n");
}

void club_notification_update(ClubNotification *notify, float delta_time) {
    if (!notify || notify->state == CLUB_NOTIFY_STATE_INACTIVE) return;
    
    notify->timer += delta_time;
    notify->pulse += delta_time * 3.0f;
    
    switch (notify->state) {
        case CLUB_NOTIFY_STATE_SHOWING:
            // Fade in
            if (notify->overlay_alpha < 0.7f) {
                notify->overlay_alpha += delta_time * 2.0f;
                if (notify->overlay_alpha > 0.7f) notify->overlay_alpha = 0.7f;
            }
            if (notify->alpha < 1.0f) {
                notify->alpha += delta_time * 3.0f;
                if (notify->alpha > 1.0f) notify->alpha = 1.0f;
            }
            break;
            
        case CLUB_NOTIFY_STATE_FADING_OUT:
            notify->alpha -= delta_time * 4.0f;
            notify->overlay_alpha -= delta_time * 4.0f;
            if (notify->alpha <= 0.0f) {
                notify->alpha = 0.0f;
                notify->overlay_alpha = 0.0f;
                notify->state = CLUB_NOTIFY_STATE_INACTIVE;
            }
            break;
            
        default:
            break;
    }
}

void club_notification_render(SDL_Renderer *renderer, ClubNotification *notify, int window_w, int window_h) {
    if (!notify || !renderer || notify->state == CLUB_NOTIFY_STATE_INACTIVE) return;
    
    // Dark overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(180 * notify->overlay_alpha));
    SDL_FRect overlay = {0, 0, window_w, window_h};
    SDL_RenderFillRect(renderer, &overlay);
    
    // Pulsing scale
    float pulse_scale = 1.0f + 0.05f * sinf(notify->pulse);
    int item_size = (int)(200 * pulse_scale);
    int item_x = (window_w - item_size) / 2;
    int item_y = (window_h - item_size) / 2 - 50;
    
    // Glow behind the item
    SDL_SetRenderDrawColor(renderer, 100, 80, 40, (Uint8)(150 * notify->alpha));
    SDL_FRect glow_rect = {item_x - 10, item_y - 10, item_size + 20, item_size + 20};
    SDL_RenderFillRect(renderer, &glow_rect);
    
    // Item background (wooden panel)
    SDL_SetRenderDrawColor(renderer, 139, 90, 43, (Uint8)(255 * notify->alpha));
    SDL_FRect item_rect = {item_x, item_y, item_size, item_size};
    SDL_RenderFillRect(renderer, &item_rect);
    
    // Gold border
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, (Uint8)(255 * notify->alpha));
    for (int i = 0; i < 4; i++) {
        SDL_FRect border = {item_x - i, item_y - i, item_size + i*2, item_size + i*2};
        SDL_RenderRect(renderer, &border);
    }
    
    // Draw club image if loaded, otherwise fallback to placeholder
    if (club_texture) {
        // Get original image dimensions to maintain aspect ratio
        float img_w, img_h;
        SDL_GetTextureSize(club_texture, &img_w, &img_h);
        
        // Calculate display size (fit within item with padding)
        float max_display = item_size * 0.75f;  // 75% of box size
        float scale = max_display / fmaxf(img_w, img_h);
        float display_w = img_w * scale;
        float display_h = img_h * scale;
        
        // Center in the box
        float display_x = item_x + (item_size - display_w) / 2;
        float display_y = item_y + (item_size - display_h) / 2;
        
        // Set alpha for fade effect
        SDL_SetTextureAlphaMod(club_texture, (Uint8)(255 * notify->alpha));
        
        SDL_FRect club_rect = {
            display_x,
            display_y,
            display_w,
            display_h
        };
        SDL_RenderTexture(renderer, club_texture, NULL, &club_rect);
    } else {
        // Fallback: draw simple placeholder text
        SDL_Color placeholder_color = {200, 150, 100, (Uint8)(255 * notify->alpha)};
        // You could render "CLUB" text here as fallback
    }
    
    // Title
    if (notify->name_texture) {
        float name_w, name_h;
        SDL_GetTextureSize(notify->name_texture, &name_w, &name_h);
        float title_scale = 1.5f;
        SDL_SetTextureAlphaMod(notify->name_texture, (Uint8)(255 * notify->alpha));
        SDL_FRect name_rect = {
            (window_w - name_w * title_scale) / 2,
            item_y - name_h * title_scale - 40,
            name_w * title_scale,
            name_h * title_scale
        };
        SDL_RenderTexture(renderer, notify->name_texture, NULL, &name_rect);
    }
    
    // Description
    if (notify->desc_texture) {
        float desc_w, desc_h;
        SDL_GetTextureSize(notify->desc_texture, &desc_w, &desc_h);
        SDL_SetTextureAlphaMod(notify->desc_texture, (Uint8)(255 * notify->alpha));
        SDL_FRect desc_rect = {
            (window_w - desc_w) / 2,
            item_y + item_size + 30,
            desc_w,
            desc_h
        };
        SDL_RenderTexture(renderer, notify->desc_texture, NULL, &desc_rect);
    }
    
    // Prompt with pulsing
    if (notify->prompt_texture) {
        float prompt_w, prompt_h;
        SDL_GetTextureSize(notify->prompt_texture, &prompt_w, &prompt_h);
        float prompt_alpha = notify->alpha * (0.5f + 0.5f * sinf(notify->pulse * 2));
        SDL_SetTextureAlphaMod(notify->prompt_texture, (Uint8)(255 * prompt_alpha));
        SDL_FRect prompt_rect = {
            (window_w - prompt_w) / 2,
            window_h - prompt_h - 100,
            prompt_w,
            prompt_h
        };
        SDL_RenderTexture(renderer, notify->prompt_texture, NULL, &prompt_rect);
    }
}

void club_notification_handle_input(ClubNotification *notify) {
    if (!notify) return;
    if (notify->state == CLUB_NOTIFY_STATE_SHOWING) {
        notify->state = CLUB_NOTIFY_STATE_FADING_OUT;
    }
}

bool club_notification_is_active(ClubNotification *notify) {
    return notify && notify->state != CLUB_NOTIFY_STATE_INACTIVE;
}

void club_notification_destroy(ClubNotification *notify) {
    if (!notify) return;
    if (notify->name_texture) SDL_DestroyTexture(notify->name_texture);
    if (notify->desc_texture) SDL_DestroyTexture(notify->desc_texture);
    if (notify->prompt_texture) SDL_DestroyTexture(notify->prompt_texture);
    // Note: club_texture is NOT destroyed here - it's kept for reuse
    // Destroy it at game shutdown if needed
    memset(notify, 0, sizeof(ClubNotification));
}

// Optional: function to cleanup club texture at game exit
void club_notification_cleanup(void) {
    if (club_texture) {
        SDL_DestroyTexture(club_texture);
        club_texture = NULL;
    }
}