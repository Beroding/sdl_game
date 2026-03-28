#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "pause_menu.h"

#define PANEL_WIDTH 500
#define PANEL_HEIGHT 600
#define BUTTON_WIDTH 400
#define BUTTON_HEIGHT 70
#define BUTTON_SPACING 20
#define ANIM_SPEED 8.0f

static SDL_Texture* create_panel_texture(SDL_Renderer *renderer, int w, int h) {
    SDL_Surface *surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return NULL;
    
    Uint32 *pixels = (Uint32*)surf->pixels;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // Dark gradient panel
            float gradient = 1.0f - (y / (float)h) * 0.3f;
            Uint8 r = (Uint8)(25 * gradient);
            Uint8 g = (Uint8)(25 * gradient);
            Uint8 b = (Uint8)(40 * gradient);
            Uint8 a = 240;
            
            // Border
            if (x < 3 || x >= w-3 || y < 3 || y >= h-3) {
                r = 180; g = 150; b = 100; // Gold border
            }
            // Inner highlight
            else if (x < 6 || x >= w-6 || y < 6 || y >= h-6) {
                r = 60; g = 60; b = 80;
            }
            
            pixels[y * w + x] = SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), NULL, r, g, b, a);
        }
    }
    
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_DestroySurface(surf);
    return tex;
}

PauseMenu* pause_menu_create(SDL_Renderer *renderer, int window_w, int window_h) {
    PauseMenu *menu = calloc(1, sizeof(PauseMenu));
    if (!menu) return NULL;
    
    menu->is_open = false;
    menu->should_close = false;
    menu->exit_to_menu = false;
    menu->save_requested = false;
    menu->settings_requested = false;
    menu->anim_alpha = 0.0f;
    menu->target_alpha = 0.0f;
    
    // Center panel
    menu->panel_w = PANEL_WIDTH;
    menu->panel_h = PANEL_HEIGHT;
    menu->panel_x = (window_w - PANEL_WIDTH) / 2.0f;
    menu->panel_y = (window_h - PANEL_HEIGHT) / 2.0f;
    
    // Load fonts
    menu->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 36);
    menu->font_title = TTF_OpenFont("game_assets/MedievalSharp-Regular.ttf", 48);
    menu->font_small = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 20);
    
    // Create panel texture
    menu->panel_texture = create_panel_texture(renderer, PANEL_WIDTH, PANEL_HEIGHT);
    
    // Create dark overlay (full screen)
    SDL_Surface *overlay_surf = SDL_CreateSurface(window_w, window_h, SDL_PIXELFORMAT_RGBA8888);
    if (overlay_surf) {
        SDL_FillSurfaceRect(overlay_surf, NULL, 
            SDL_MapRGBA(SDL_GetPixelFormatDetails(overlay_surf->format), NULL, 0, 0, 0, 180));
        menu->bg_overlay = SDL_CreateTextureFromSurface(renderer, overlay_surf);
        SDL_SetTextureBlendMode(menu->bg_overlay, SDL_BLENDMODE_BLEND);
        SDL_DestroySurface(overlay_surf);
    }
    
    // Setup buttons
    const char *labels[PAUSE_BUTTON_COUNT] = {
        "CONTINUE",
        "SAVE GAME",
        "SETTINGS",
        "EXIT TO MENU"
    };
    
    float start_y = menu->panel_y + 120;
    
    for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
        PauseButton *btn = &menu->buttons[i];
        btn->type = i;
        btn->hovered = false;
        
        // Button rect centered in panel
        btn->rect.x = menu->panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
        btn->rect.y = start_y + i * (BUTTON_HEIGHT + BUTTON_SPACING);
        btn->rect.w = BUTTON_WIDTH;
        btn->rect.h = BUTTON_HEIGHT;
        
        // Create text texture
        SDL_Color text_color = {255, 255, 255, 255};
        SDL_Surface *text_surf = TTF_RenderText_Blended(menu->font, labels[i], 0, text_color);
        if (text_surf) {
            btn->text_texture = SDL_CreateTextureFromSurface(renderer, text_surf);
            SDL_DestroySurface(text_surf);
        }
    }
    
    printf("Pause menu created\n");
    return menu;
}

void pause_menu_destroy(PauseMenu *menu) {
    if (!menu) return;
    
    for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
        if (menu->buttons[i].text_texture) {
            SDL_DestroyTexture(menu->buttons[i].text_texture);
        }
    }
    
    if (menu->panel_texture) SDL_DestroyTexture(menu->panel_texture);
    if (menu->bg_overlay) SDL_DestroyTexture(menu->bg_overlay);
    if (menu->font) TTF_CloseFont(menu->font);
    if (menu->font_title) TTF_CloseFont(menu->font_title);
    if (menu->font_small) TTF_CloseFont(menu->font_small);
    
    free(menu);
    printf("Pause menu destroyed\n");
}

void pause_menu_open(PauseMenu *menu) {
    if (!menu) return;
    menu->is_open = true;
    menu->should_close = false;
    menu->target_alpha = 1.0f;
    menu->anim_alpha = 0.0f;
    printf("Pause menu opened\n");
}

void pause_menu_close(PauseMenu *menu) {
    if (!menu) return;
    menu->should_close = true;
    menu->target_alpha = 0.0f;
    printf("Pause menu closing\n");
}

bool pause_menu_is_open(PauseMenu *menu) {
    return menu && menu->is_open;
}

void pause_menu_update(PauseMenu *menu, float delta_time) {
    if (!menu) return;
    
    // Animate fade
    if (menu->anim_alpha < menu->target_alpha) {
        menu->anim_alpha += delta_time * ANIM_SPEED;
        if (menu->anim_alpha > menu->target_alpha) menu->anim_alpha = menu->target_alpha;
    } else if (menu->anim_alpha > menu->target_alpha) {
        menu->anim_alpha -= delta_time * ANIM_SPEED;
        if (menu->anim_alpha < menu->target_alpha) menu->anim_alpha = menu->target_alpha;
    }
    
    // Fully closed
    if (menu->should_close && menu->anim_alpha <= 0.01f) {
        menu->is_open = false;
        menu->should_close = false;
        menu->anim_alpha = 0.0f;
    }
}

void pause_menu_render(SDL_Renderer *renderer, PauseMenu *menu) {
    if (!menu || !menu->is_open || menu->anim_alpha <= 0.01f) return;
    
    Uint8 alpha = (Uint8)(255 * menu->anim_alpha);
    
    // Darken background
    if (menu->bg_overlay) {
        SDL_SetTextureAlphaMod(menu->bg_overlay, (Uint8)(180 * menu->anim_alpha));
        SDL_FRect full = {0, 0, 0, 0}; // Full screen
        SDL_RenderTexture(renderer, menu->bg_overlay, NULL, &full);
    }
    
    // Panel shadow (offset copy)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(100 * menu->anim_alpha));
    SDL_FRect shadow = {
        menu->panel_x + 10, 
        menu->panel_y + 10, 
        menu->panel_w, 
        menu->panel_h
    };
    SDL_RenderFillRect(renderer, &shadow);
    
    // Main panel
    if (menu->panel_texture) {
        SDL_SetTextureAlphaMod(menu->panel_texture, alpha);
        SDL_FRect panel_rect = {menu->panel_x, menu->panel_y, menu->panel_w, menu->panel_h};
        SDL_RenderTexture(renderer, menu->panel_texture, NULL, &panel_rect);
    }
    
    // Title
    if (menu->font_title) {
        const char *title = "PAUSED";
        SDL_Color title_color = {255, 215, 100, alpha};
        SDL_Surface *title_surf = TTF_RenderText_Blended(menu->font_title, title, 0, title_color);
        if (title_surf) {
            SDL_Texture *title_tex = SDL_CreateTextureFromSurface(renderer, title_surf);
            float tw, th;
            SDL_GetTextureSize(title_tex, &tw, &th);
            SDL_SetTextureAlphaMod(title_tex, alpha);
            SDL_FRect title_rect = {
                menu->panel_x + (menu->panel_w - tw) / 2,
                menu->panel_y + 40,
                tw, th
            };
            SDL_RenderTexture(renderer, title_tex, NULL, &title_rect);
            SDL_DestroyTexture(title_tex);
            SDL_DestroySurface(title_surf);
        }
    }
    
    // Buttons
    for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
        PauseButton *btn = &menu->buttons[i];
        
        // Breathing hover effect
        float breathe = 1.0f;
        if (btn->hovered) {
            breathe = 0.8f + 0.2f * sinf(SDL_GetTicks() / 200.0f);
        }
        
        // Button background
        SDL_Color bg_color;
        if (btn->hovered) {
            // Gold breathing when hovered
            bg_color.r = (Uint8)(80 * breathe);
            bg_color.g = (Uint8)(70 * breathe);
            bg_color.b = (Uint8)(50 * breathe);
            bg_color.a = alpha;
        } else {
            // Dark when not hovered
            bg_color.r = 40;
            bg_color.g = 40;
            bg_color.b = 60;
            bg_color.a = alpha;
        }
        
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderFillRect(renderer, &btn->rect);
        
        // Button border
        SDL_Color border_color = btn->hovered ? 
            (SDL_Color){(Uint8)(255*breathe), (Uint8)(215*breathe), (Uint8)(100*breathe), alpha} :
            (SDL_Color){100, 100, 120, alpha};
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, border_color.a);
        SDL_RenderRect(renderer, &btn->rect);
        
        // Inner highlight for hovered
        if (btn->hovered) {
            SDL_FRect inner = {
                btn->rect.x + 3, btn->rect.y + 3,
                btn->rect.w - 6, btn->rect.h - 6
            };
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(100*breathe), (Uint8)(90*breathe), (Uint8)(70*breathe), 
                (Uint8)(100 * menu->anim_alpha));
            SDL_RenderRect(renderer, &inner);
        }
        
        // Button text
        if (btn->text_texture) {
            SDL_SetTextureAlphaMod(btn->text_texture, alpha);
            float tw, th;
            SDL_GetTextureSize(btn->text_texture, &tw, &th);
            SDL_FRect text_rect = {
                btn->rect.x + (btn->rect.w - tw) / 2,
                btn->rect.y + (btn->rect.h - th) / 2,
                tw, th
            };
            SDL_RenderTexture(renderer, btn->text_texture, NULL, &text_rect);
        }
    }
    
    // Hint at bottom
    if (menu->font_small) {
        SDL_Color hint_color = {150, 150, 150, (Uint8)(200 * menu->anim_alpha)};
        SDL_Surface *hint_surf = TTF_RenderText_Blended(menu->font_small, 
            "ESC or Click Continue to Resume", 0, hint_color);
        if (hint_surf) {
            SDL_Texture *hint_tex = SDL_CreateTextureFromSurface(renderer, hint_surf);
            float hw, hh;
            SDL_GetTextureSize(hint_tex, &hw, &hh);
            SDL_SetTextureAlphaMod(hint_tex, alpha);
            SDL_FRect hint_rect = {
                menu->panel_x + (menu->panel_w - hw) / 2,
                menu->panel_y + menu->panel_h - 40,
                hw, hh
            };
            SDL_RenderTexture(renderer, hint_tex, NULL, &hint_rect);
            SDL_DestroyTexture(hint_tex);
            SDL_DestroySurface(hint_surf);
        }
    }
}

bool pause_menu_handle_mouse(PauseMenu *menu, SDL_MouseButtonEvent *mouse) {
    if (!menu || !menu->is_open) return false;
    
    float mx = mouse->x;
    float my = mouse->y;
    
    // Update hover states
    for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
        PauseButton *btn = &menu->buttons[i];
        btn->hovered = (mx >= btn->rect.x && mx <= btn->rect.x + btn->rect.w &&
                       my >= btn->rect.y && my <= btn->rect.y + btn->rect.h);
    }
    
    if (mouse->type == SDL_EVENT_MOUSE_BUTTON_DOWN && mouse->button == SDL_BUTTON_LEFT) {
        for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
            if (menu->buttons[i].hovered) {
                switch (menu->buttons[i].type) {
                    case PAUSE_BUTTON_CONTINUE:
                        pause_menu_close(menu);
                        return true;
                    case PAUSE_BUTTON_SAVE:
                        menu->save_requested = true;
                        printf("Save requested\n");
                        return true;
                    case PAUSE_BUTTON_SETTINGS:
                        menu->settings_requested = true;
                        printf("Settings requested\n");
                        return true;
                    case PAUSE_BUTTON_EXIT:
                        menu->exit_to_menu = true;
                        pause_menu_close(menu);
                        return true;
                }
            }
        }
    }
    
    return false;
}

bool pause_menu_handle_key(PauseMenu *menu, SDL_KeyboardEvent *key) {
    if (!menu || !menu->is_open) return false;
    
    if (key->type == SDL_EVENT_KEY_DOWN) {
        switch (key->scancode) {
            case SDL_SCANCODE_ESCAPE:
                pause_menu_close(menu);
                return true;
            case SDL_SCANCODE_UP:
                // Find current hover and move up
                for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
                    if (menu->buttons[i].hovered) {
                        menu->buttons[i].hovered = false;
                        int new_i = (i - 1 + PAUSE_BUTTON_COUNT) % PAUSE_BUTTON_COUNT;
                        menu->buttons[new_i].hovered = true;
                        return true;
                    }
                }
                // None hovered, select last
                menu->buttons[PAUSE_BUTTON_COUNT - 1].hovered = true;
                return true;
            case SDL_SCANCODE_DOWN:
                for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
                    if (menu->buttons[i].hovered) {
                        menu->buttons[i].hovered = false;
                        int new_i = (i + 1) % PAUSE_BUTTON_COUNT;
                        menu->buttons[new_i].hovered = true;
                        return true;
                    }
                }
                // None hovered, select first
                menu->buttons[0].hovered = true;
                return true;
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_KP_ENTER:
                for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
                    if (menu->buttons[i].hovered) {
                        // Simulate click
                        SDL_MouseButtonEvent fake_mouse = {0};
                        fake_mouse.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                        fake_mouse.button = SDL_BUTTON_LEFT;
                        return pause_menu_handle_mouse(menu, &fake_mouse);
                    }
                }
                return true;
            default:
                break;
        }
    }
    
    return false;
}