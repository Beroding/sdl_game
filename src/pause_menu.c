// ============================================================================
// pause_menu.c – In‑game pause menu overlay
// ============================================================================
//
// This file implements a modal pause menu that appears when the player presses ESC.
// It fades in/out smoothly, displays a semi‑transparent panel with buttons,
// and captures input while the game is paused. Buttons allow continuing, saving,
// opening settings (placeholder), or exiting to the main menu.
//
// Assumptions:
// - The game is rendered behind the menu, which is darkened by an overlay.
// - Fonts "Roboto_Medium.ttf" and "MedievalSharp-Regular.ttf" are available.
// - The window size is fixed and known at creation.
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../include/pause_menu.h"

// ----------------------------------------------------------------------------
// Layout constants
// ----------------------------------------------------------------------------
#define PANEL_WIDTH 500
#define PANEL_HEIGHT 600
#define BUTTON_WIDTH 400
#define BUTTON_HEIGHT 70
#define BUTTON_SPACING 20
#define ANIM_SPEED 8.0f          // Fade speed (alpha per second)

// ----------------------------------------------------------------------------
// Helper: create a stylised panel texture with gradient and gold border.
// Returns an SDL_Texture that can be drawn as the background of the pause menu.
// ----------------------------------------------------------------------------
static SDL_Texture* create_panel_texture(SDL_Renderer *renderer, int w, int h) {
    SDL_Surface *surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return NULL;
    
    Uint32 *pixels = (Uint32*)surf->pixels;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // Dark gradient: darker near the top, slightly lighter near bottom
            float gradient = 1.0f - (y / (float)h) * 0.3f;
            Uint8 r = (Uint8)(25 * gradient);
            Uint8 g = (Uint8)(25 * gradient);
            Uint8 b = (Uint8)(40 * gradient);
            Uint8 a = 240;        // Slight transparency
            
            // Gold border around the panel (outer 3 pixels)
            if (x < 3 || x >= w-3 || y < 3 || y >= h-3) {
                r = 180; g = 150; b = 100;
            }
            // Inner highlight band (next 3 pixels)
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

// ----------------------------------------------------------------------------
// Create the pause menu: allocate structure, load fonts, build textures.
// ----------------------------------------------------------------------------
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
    
    // Centre the panel on the screen
    menu->panel_w = PANEL_WIDTH;
    menu->panel_h = PANEL_HEIGHT;
    menu->panel_x = (window_w - PANEL_WIDTH) / 2.0f;
    menu->panel_y = (window_h - PANEL_HEIGHT) / 2.0f;
    
    // Load fonts (reuse game assets if available)
    menu->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 36);
    menu->font_title = TTF_OpenFont("game_assets/MedievalSharp-Regular.ttf", 48);
    menu->font_small = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 20);
    
    // Create the panel texture
    menu->panel_texture = create_panel_texture(renderer, PANEL_WIDTH, PANEL_HEIGHT);
    
    // Create a full‑screen dark overlay to dim the game behind the menu
    SDL_Surface *overlay_surf = SDL_CreateSurface(window_w, window_h, SDL_PIXELFORMAT_RGBA8888);
    if (overlay_surf) {
        SDL_FillSurfaceRect(overlay_surf, NULL, 
            SDL_MapRGBA(SDL_GetPixelFormatDetails(overlay_surf->format), NULL, 0, 0, 0, 180));
        menu->bg_overlay = SDL_CreateTextureFromSurface(renderer, overlay_surf);
        SDL_SetTextureBlendMode(menu->bg_overlay, SDL_BLENDMODE_BLEND);
        SDL_DestroySurface(overlay_surf);
    }
    
    // Setup buttons: positions and text textures
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
        
        // Button rectangle (centered horizontally in the panel)
        btn->rect.x = menu->panel_x + (PANEL_WIDTH - BUTTON_WIDTH) / 2;
        btn->rect.y = start_y + i * (BUTTON_HEIGHT + BUTTON_SPACING);
        btn->rect.w = BUTTON_WIDTH;
        btn->rect.h = BUTTON_HEIGHT;
        
        // Create a texture for the button's text (white)
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

// ----------------------------------------------------------------------------
// Free all resources allocated for the pause menu.
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// Open the menu (start fade‑in animation).
// ----------------------------------------------------------------------------
void pause_menu_open(PauseMenu *menu) {
    if (!menu) return;
    menu->is_open = true;
    menu->should_close = false;
    menu->target_alpha = 1.0f;
    menu->anim_alpha = 0.0f;      // Start fully transparent, then fade in
    printf("Pause menu opened\n");
}

// ----------------------------------------------------------------------------
// Request to close the menu (start fade‑out animation).
// ----------------------------------------------------------------------------
void pause_menu_close(PauseMenu *menu) {
    if (!menu) return;
    menu->should_close = true;
    menu->target_alpha = 0.0f;
    printf("Pause menu closing\n");
}

// ----------------------------------------------------------------------------
// Check if the menu is currently visible.
// ----------------------------------------------------------------------------
bool pause_menu_is_open(PauseMenu *menu) {
    return menu && menu->is_open;
}

// ----------------------------------------------------------------------------
// Update the fade animation. Called every frame while the menu is active.
// ----------------------------------------------------------------------------
void pause_menu_update(PauseMenu *menu, float delta_time) {
    if (!menu) return;
    
    // Smoothly approach target alpha (0 or 1)
    if (menu->anim_alpha < menu->target_alpha) {
        menu->anim_alpha += delta_time * ANIM_SPEED;
        if (menu->anim_alpha > menu->target_alpha) menu->anim_alpha = menu->target_alpha;
    } else if (menu->anim_alpha > menu->target_alpha) {
        menu->anim_alpha -= delta_time * ANIM_SPEED;
        if (menu->anim_alpha < menu->target_alpha) menu->anim_alpha = menu->target_alpha;
    }
    
    // Once fully faded out, mark the menu as closed
    if (menu->should_close && menu->anim_alpha <= 0.01f) {
        menu->is_open = false;
        menu->should_close = false;
        menu->anim_alpha = 0.0f;
    }
}

// ----------------------------------------------------------------------------
// Render the menu: dark overlay, panel, buttons, title, hint text.
// ----------------------------------------------------------------------------
void pause_menu_render(SDL_Renderer *renderer, PauseMenu *menu) {
    if (!menu || !menu->is_open || menu->anim_alpha <= 0.01f) return;
    
    Uint8 alpha = (Uint8)(255 * menu->anim_alpha);
    
    // Darken the game background
    if (menu->bg_overlay) {
        SDL_SetTextureAlphaMod(menu->bg_overlay, (Uint8)(180 * menu->anim_alpha));
        // Passing NULL for the source rectangle draws the whole texture stretched to the target.
        // We pass NULL for destination to use the whole screen (the texture is full screen).
        SDL_RenderTexture(renderer, menu->bg_overlay, NULL, NULL);
    }
    
    // Panel shadow (offset, dark rectangle)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(100 * menu->anim_alpha));
    SDL_FRect shadow = {
        menu->panel_x + 10, 
        menu->panel_y + 10, 
        menu->panel_w, 
        menu->panel_h
    };
    SDL_RenderFillRect(renderer, &shadow);
    
    // Main panel texture
    if (menu->panel_texture) {
        SDL_SetTextureAlphaMod(menu->panel_texture, alpha);
        SDL_FRect panel_rect = {menu->panel_x, menu->panel_y, menu->panel_w, menu->panel_h};
        SDL_RenderTexture(renderer, menu->panel_texture, NULL, &panel_rect);
    }
    
    // Title text ("PAUSED")
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
    
    // Draw buttons
    for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
        PauseButton *btn = &menu->buttons[i];
        
        // Breathing effect when hovered: scale the brightness slightly
        float breathe = 1.0f;
        if (btn->hovered) {
            breathe = 0.8f + 0.2f * sinf(SDL_GetTicks() / 200.0f);
        }
        
        // Button background
        SDL_Color bg_color;
        if (btn->hovered) {
            // Golden tint when hovered
            bg_color.r = (Uint8)(80 * breathe);
            bg_color.g = (Uint8)(70 * breathe);
            bg_color.b = (Uint8)(50 * breathe);
            bg_color.a = alpha;
        } else {
            // Dark grey when not hovered
            bg_color.r = 40;
            bg_color.g = 40;
            bg_color.b = 60;
            bg_color.a = alpha;
        }
        
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderFillRect(renderer, &btn->rect);
        
        // Button border (gold when hovered)
        SDL_Color border_color = btn->hovered ? 
            (SDL_Color){(Uint8)(255*breathe), (Uint8)(215*breathe), (Uint8)(100*breathe), alpha} :
            (SDL_Color){100, 100, 120, alpha};
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, border_color.a);
        SDL_RenderRect(renderer, &btn->rect);
        
        // Inner highlight for hovered button
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
    
    // Hint text at the bottom of the panel
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

    // SDL_Rect volume_rect = {x, y, 200, 20};
    // SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    // SDL_RenderFillRect(renderer, &volume_rect);
    
    // // Volume fill
    // int fill_w = (int)(200 * pm->music_volume);
    // SDL_Rect fill_rect = {x, y, fill_w, 20};
    // SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    // SDL_RenderFillRect(renderer, &fill_rect);
}

// ----------------------------------------------------------------------------
// Handle mouse clicks and hover updates on the menu.
// Returns true if the event was consumed (menu handled it).
// ----------------------------------------------------------------------------
bool pause_menu_handle_mouse(PauseMenu *menu, SDL_MouseButtonEvent *mouse) {
    if (!menu || !menu->is_open) return false;
    
    float mx = mouse->x;
    float my = mouse->y;
    
    // Update hover states for all buttons
    for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
        PauseButton *btn = &menu->buttons[i];
        btn->hovered = (mx >= btn->rect.x && mx <= btn->rect.x + btn->rect.w &&
                       my >= btn->rect.y && my <= btn->rect.y + btn->rect.h);
    }
    
    // On left click, check which button was clicked and set corresponding flags
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

// ----------------------------------------------------------------------------
// Handle keyboard input for menu navigation.
// Returns true if the event was consumed.
// ----------------------------------------------------------------------------
bool pause_menu_handle_key(PauseMenu *menu, SDL_KeyboardEvent *key) {
    if (!menu || !menu->is_open) return false;
    
    if (key->type == SDL_EVENT_KEY_DOWN) {
        switch (key->scancode) {
            case SDL_SCANCODE_ESCAPE:
                pause_menu_close(menu);
                return true;
            case SDL_SCANCODE_UP:
                // Move selection up (wrap around)
                for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
                    if (menu->buttons[i].hovered) {
                        menu->buttons[i].hovered = false;
                        int new_i = (i - 1 + PAUSE_BUTTON_COUNT) % PAUSE_BUTTON_COUNT;
                        menu->buttons[new_i].hovered = true;
                        return true;
                    }
                }
                // If nothing hovered, select the last button
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
                // If nothing hovered, select the first button
                menu->buttons[0].hovered = true;
                return true;
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_KP_ENTER:
                // Simulate a click on the currently hovered button
                for (int i = 0; i < PAUSE_BUTTON_COUNT; i++) {
                    if (menu->buttons[i].hovered) {
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