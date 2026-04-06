#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../include/save_menu.h"

#define PANEL_WIDTH 800
#define PANEL_HEIGHT 700
#define SLOT_HEIGHT 120
#define SLOT_SPACING 15
#define ANIM_SPEED 10.0f

/*
 * Creates a textured panel with a gradient background and gold border.
 * We pre‑render this once at creation so we don't waste CPU cycles
 * redrawing the same background every frame.
 * The gradient gives a subtle depth, and the border helps the menu
 * stand out against the game world.
 */
static SDL_Texture* create_save_panel(SDL_Renderer *renderer, int w, int h) {
    SDL_Surface *surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return NULL;
    
    Uint32 *pixels = (Uint32*)surf->pixels;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float gradient = 1.0f - (y / (float)h) * 0.2f;
            Uint8 r = (Uint8)(20 * gradient);
            Uint8 g = (Uint8)(25 * gradient);
            Uint8 b = (Uint8)(35 * gradient);
            Uint8 a = 250;
            
            // Outer border – use a distinct color to frame the panel.
            if (x < 4 || x >= w-4 || y < 4 || y >= h-4) {
                r = 200; g = 170; b = 100;
            }
            // Inner bevel – adds a pseudo‑3D edge to make the panel feel solid.
            else if (x < 8 || x >= w-8 || y < 8 || y >= h-8) {
                r = 40; g = 45; b = 60;
            }
            
            pixels[y * w + x] = SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), NULL, r, g, b, a);
        }
    }
    
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_DestroySurface(surf);
    return tex;
}

/*
 * Creates the save menu structure, precomputes all rectangle positions,
 * loads fonts, and builds the static textures.
 * Assumption: the window size will not change while the menu is open,
 * so we only compute positions once.
 */
SaveMenu* save_menu_create(SDL_Renderer *renderer, int window_w, int window_h,
                           SaveSystem *save_sys) {
    SaveMenu *menu = calloc(1, sizeof(SaveMenu));
    if (!menu) return NULL;
    
    menu->is_open = false;
    menu->mode = SAVE_MENU_MODE_LOAD;
    menu->selected_slot = -1;
    menu->hovered_slot = -1;
    menu->save_system = save_sys;
    menu->anim_alpha = 0.0f;
    menu->target_alpha = 0.0f;
    menu->show_confirm_dialog = false;
    menu->confirm_delete = false;
    
    // Center the panel on the screen.
    menu->panel_w = PANEL_WIDTH;
    menu->panel_h = PANEL_HEIGHT;
    menu->panel_x = (window_w - PANEL_WIDTH) / 2.0f;
    menu->panel_y = (window_h - PANEL_HEIGHT) / 2.0f;
    
    // Load fonts – fallback to system font if these are missing would be ideal,
    // but we assume the assets exist.
    menu->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 32);
    menu->font_title = TTF_OpenFont("game_assets/MedievalSharp-Regular.ttf", 48);
    menu->font_small = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 20);
    
    // Build the decorative panel texture once.
    menu->panel_texture = create_save_panel(renderer, PANEL_WIDTH, PANEL_HEIGHT);
    
    // Create a semi‑transparent overlay that darkens everything behind the menu.
    // This focuses the player's attention on the UI.
    SDL_Surface *overlay = SDL_CreateSurface(window_w, window_h, SDL_PIXELFORMAT_RGBA8888);
    if (overlay) {
        SDL_FillSurfaceRect(overlay, NULL,
            SDL_MapRGBA(SDL_GetPixelFormatDetails(overlay->format), NULL, 0, 0, 0, 200));
        menu->bg_overlay = SDL_CreateTextureFromSurface(renderer, overlay);
        SDL_SetTextureBlendMode(menu->bg_overlay, SDL_BLENDMODE_BLEND);
        SDL_DestroySurface(overlay);
    }
    
    // Pre‑compute the rectangle for each save slot.
    // Layout is vertical, starting 100 pixels from the top of the panel.
    float slot_start_y = menu->panel_y + 100;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        menu->slot_rects[i].x = menu->panel_x + 50;
        menu->slot_rects[i].y = slot_start_y + i * (SLOT_HEIGHT + SLOT_SPACING);
        menu->slot_rects[i].w = PANEL_WIDTH - 100;
        menu->slot_rects[i].h = SLOT_HEIGHT;
    }
    
    // Place the two main buttons at the bottom of the panel.
    float btn_y = menu->panel_y + PANEL_HEIGHT - 80;
    menu->back_button.x = menu->panel_x + 50;
    menu->back_button.y = btn_y;
    menu->back_button.w = 200;
    menu->back_button.h = 50;
    
    menu->confirm_button.x = menu->panel_x + PANEL_WIDTH - 250;
    menu->confirm_button.y = btn_y;
    menu->confirm_button.w = 200;
    menu->confirm_button.h = 50;
    
    printf("Save menu created\n");
    return menu;
}

void save_menu_destroy(SaveMenu *menu) {
    if (!menu) return;
    
    if (menu->panel_texture) SDL_DestroyTexture(menu->panel_texture);
    if (menu->bg_overlay) SDL_DestroyTexture(menu->bg_overlay);
    if (menu->font) TTF_CloseFont(menu->font);
    if (menu->font_title) TTF_CloseFont(menu->font_title);
    if (menu->font_small) TTF_CloseFont(menu->font_small);
    
    free(menu);
}

/*
 * Opens the menu with a specific mode.
 * We reset the selection and start the fade‑in animation.
 * The caller provides the mode because the same UI is reused for
 * save, load, and delete – only the confirm action changes.
 */
void save_menu_open(SaveMenu *menu, SaveMenuMode mode) {
    if (!menu) return;
    menu->is_open = true;
    menu->mode = mode;
    menu->selected_slot = -1;
    menu->target_alpha = 1.0f;
    menu->show_confirm_dialog = false;
    printf("Save menu opened in mode %d\n", mode);
}

/*
 * Request to close the menu.
 * We don't hide it instantly – the fade‑out animation will set is_open = false
 * when alpha reaches zero.
 */
void save_menu_close(SaveMenu *menu) {
    if (!menu) return;
    menu->target_alpha = 0.0f;
    menu->show_confirm_dialog = false;
}

/*
 * Returns true only if the menu is both open and fully visible.
 * This is used by the main loop to know when the menu should receive input.
 */
bool save_menu_is_open(SaveMenu *menu) {
    return menu && menu->is_open && menu->anim_alpha > 0.01f;
}

/*
 * Sets the callback functions that will be invoked when the user confirms an action.
 * The menu itself does not know how to save/load – it only fires these callbacks.
 * This separation keeps the UI independent of the game logic.
 */
void save_menu_set_callbacks(SaveMenu *menu,
                             void (*on_save)(int),
                             void (*on_load)(int, SaveSlotData*),
                             void (*on_delete)(int),
                             void (*on_cancel)(void)) {
    if (!menu) return;
    menu->on_save_completed = on_save;
    menu->on_load_selected = on_load;
    menu->on_delete_completed = on_delete;
    menu->on_cancelled = on_cancel;
}

/*
 * Updates the fade animation based on delta time.
 * We move anim_alpha toward target_alpha linearly – no easing,
 * which gives a simple, predictable fade.
 * When the fade‑out finishes, we set is_open = false so the menu can be hidden.
 */
void save_menu_update(SaveMenu *menu, float delta_time) {
    if (!menu) return;
    
    if (menu->anim_alpha < menu->target_alpha) {
        menu->anim_alpha += delta_time * ANIM_SPEED;
        if (menu->anim_alpha > menu->target_alpha) menu->anim_alpha = menu->target_alpha;
    } else if (menu->anim_alpha > menu->target_alpha) {
        menu->anim_alpha -= delta_time * ANIM_SPEED;
        if (menu->anim_alpha < menu->target_alpha) {
            menu->anim_alpha = menu->target_alpha;
            if (menu->anim_alpha <= 0.01f) menu->is_open = false;
        }
    }
}

/*
 * Renders the entire save menu.
 * We draw everything with the current alpha value (anim_alpha) so
 * the whole menu fades in and out uniformly.
 * The breathing effect on the selected slot is a subtle cue for the player.
 */
void save_menu_render(SDL_Renderer *renderer, SaveMenu *menu) {
    if (!menu || !menu->is_open || menu->anim_alpha <= 0.01f) return;
    
    Uint8 alpha = (Uint8)(255 * menu->anim_alpha);
    // The breathing intensity cycles with time – used to highlight the selected slot.
    float breathe = 0.5f + 0.5f * sinf(SDL_GetTicks() / 500.0f);
    
    // Dark overlay to dim the background.
    if (menu->bg_overlay) {
        SDL_SetTextureAlphaMod(menu->bg_overlay, (Uint8)(200 * menu->anim_alpha));
        SDL_FRect full = {0, 0, 0, 0};
        SDL_RenderTexture(renderer, menu->bg_overlay, NULL, &full);
    }
    
    // Main panel background.
    if (menu->panel_texture) {
        SDL_SetTextureAlphaMod(menu->panel_texture, alpha);
        SDL_FRect panel = {menu->panel_x, menu->panel_y, menu->panel_w, menu->panel_h};
        SDL_RenderTexture(renderer, menu->panel_texture, NULL, &panel);
    }
    
    // Title changes based on current mode.
    if (menu->font_title) {
        const char *title = menu->mode == SAVE_MENU_MODE_SAVE ? "SAVE GAME" :
                           menu->mode == SAVE_MENU_MODE_LOAD ? "LOAD GAME" : "DELETE SAVE";
        SDL_Color title_color = {255, 215, 100, alpha};
        SDL_Surface *surf = TTF_RenderText_Blended(menu->font_title, title, 0, title_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            float tw, th;
            SDL_GetTextureSize(tex, &tw, &th);
            SDL_SetTextureAlphaMod(tex, alpha);
            SDL_FRect rect = {
                menu->panel_x + (menu->panel_w - tw) / 2,
                menu->panel_y + 30,
                tw, th
            };
            SDL_RenderTexture(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }
    }
    
    // Render each save slot.
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        SDL_FRect *slot = &menu->slot_rects[i];
        bool occupied = has_save_in_slot(menu->save_system, i);
        bool is_selected = (menu->selected_slot == i);
        bool is_hovered = (menu->hovered_slot == i);
        
        // Slot background color: selected > hovered > occupied > empty.
        SDL_Color bg_color;
        if (is_selected) {
            bg_color = (SDL_Color){60, 80, 120, alpha};
        } else if (is_hovered) {
            bg_color = (SDL_Color){50, 60, 80, alpha};
        } else {
            bg_color = occupied ? 
                (SDL_Color){40, 45, 55, alpha} : 
                (SDL_Color){30, 35, 45, alpha};
        }
        
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderFillRect(renderer, slot);
        
        // Border: selected gets a breathing gold color, otherwise a muted tone.
        SDL_Color border_color;
        if (is_selected) {
            border_color = (SDL_Color){
                (Uint8)(255 * breathe), 
                (Uint8)(200 * breathe), 
                100, alpha
            };
        } else if (is_hovered) {
            border_color = (SDL_Color){150, 150, 150, alpha};
        } else {
            border_color = occupied ? 
                (SDL_Color){100, 150, 200, alpha} : 
                (SDL_Color){80, 80, 80, alpha};
        }
        
        SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, 
                              border_color.b, border_color.a);
        SDL_RenderRect(renderer, slot);
        
        // Slot number (1‑based) on the left side.
        if (menu->font) {
            char num_str[8];
            snprintf(num_str, sizeof(num_str), "%d", i + 1);
            SDL_Color num_color = is_selected ? 
                (SDL_Color){255, 215, 100, alpha} : 
                (SDL_Color){150, 150, 150, alpha};
            SDL_Surface *surf = TTF_RenderText_Blended(menu->font, num_str, 0, num_color);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                float tw, th;
                SDL_GetTextureSize(tex, &tw, &th);
                SDL_SetTextureAlphaMod(tex, alpha);
                SDL_FRect rect = {
                    slot->x + 20,
                    slot->y + (slot->h - th) / 2,
                    tw, th
                };
                SDL_RenderTexture(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(surf);
            }
        }
        
        // Show save metadata (date and preview) if the slot is occupied.
        if (menu->font_small) {
            char date_str[64] = "EMPTY";
            char preview_str[128] = "No save data";
            
            if (occupied) {
                save_system_get_slot_info(menu->save_system, i,
                                          date_str, sizeof(date_str),
                                          preview_str, sizeof(preview_str));
            }
            
            SDL_Color info_color = occupied ? 
                (SDL_Color){200, 200, 200, alpha} : 
                (SDL_Color){100, 100, 100, alpha};
            
            // Date/time of save.
            SDL_Surface *date_surf = TTF_RenderText_Blended(menu->font_small, date_str, 0, info_color);
            if (date_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, date_surf);
                float tw, th;
                SDL_GetTextureSize(tex, &tw, &th);
                SDL_SetTextureAlphaMod(tex, alpha);
                SDL_FRect rect = {
                    slot->x + 80,
                    slot->y + 20,
                    tw, th
                };
                SDL_RenderTexture(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(date_surf);
            }
            
            // Short preview (e.g., location, level).
            SDL_Surface *prev_surf = TTF_RenderText_Blended(menu->font_small, preview_str, 0, info_color);
            if (prev_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, prev_surf);
                float tw, th;
                SDL_GetTextureSize(tex, &tw, &th);
                SDL_SetTextureAlphaMod(tex, alpha);
                SDL_FRect rect = {
                    slot->x + 80,
                    slot->y + 60,
                    tw, th
                };
                SDL_RenderTexture(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(prev_surf);
            }
        }
        
        // In delete mode, show an "X" on occupied slots as a visual hint.
        if (menu->mode == SAVE_MENU_MODE_DELETE && occupied) {
            SDL_Color x_color = {255, 80, 80, alpha};
            SDL_SetRenderDrawColor(renderer, x_color.r, x_color.g, x_color.b, x_color.a);
            float cx = slot->x + slot->w - 40;
            float cy = slot->y + slot->h / 2;
            SDL_RenderLine(renderer, cx - 10, cy - 10, cx + 10, cy + 10);
            SDL_RenderLine(renderer, cx + 10, cy - 10, cx - 10, cy + 10);
        }
    }
    
    // Back button – always present to cancel.
    SDL_Color back_bg = menu->hovered_slot == -2 ? 
        (SDL_Color){80, 60, 60, alpha} : (SDL_Color){60, 40, 40, alpha};
    SDL_SetRenderDrawColor(renderer, back_bg.r, back_bg.g, back_bg.b, back_bg.a);
    SDL_RenderFillRect(renderer, &menu->back_button);
    
    SDL_SetRenderDrawColor(renderer, 150, 100, 100, alpha);
    SDL_RenderRect(renderer, &menu->back_button);
    
    if (menu->font) {
        SDL_Color text_color = {200, 150, 150, alpha};
        SDL_Surface *surf = TTF_RenderText_Blended(menu->font, "BACK", 0, text_color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            float tw, th;
            SDL_GetTextureSize(tex, &tw, &th);
            SDL_SetTextureAlphaMod(tex, alpha);
            SDL_FRect rect = {
                menu->back_button.x + (menu->back_button.w - tw) / 2,
                menu->back_button.y + (menu->back_button.h - th) / 2,
                tw, th
            };
            SDL_RenderTexture(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }
    }
    
    // Confirm button – only shown if a slot is selected and the action is valid.
    if (menu->selected_slot >= 0) {
        bool can_confirm = (menu->mode != SAVE_MENU_MODE_LOAD) || 
                         has_save_in_slot(menu->save_system, menu->selected_slot);
        
        if (can_confirm) {
            SDL_Color confirm_bg = menu->hovered_slot == -3 ?
                (SDL_Color){60, 100, 60, alpha} : (SDL_Color){40, 80, 40, alpha};
            SDL_SetRenderDrawColor(renderer, confirm_bg.r, confirm_bg.g, confirm_bg.b, confirm_bg.a);
            SDL_RenderFillRect(renderer, &menu->confirm_button);
            
            SDL_SetRenderDrawColor(renderer, 100, 200, 100, alpha);
            SDL_RenderRect(renderer, &menu->confirm_button);
            
            if (menu->font) {
                const char *btn_text = menu->mode == SAVE_MENU_MODE_SAVE ? "SAVE" :
                                      menu->mode == SAVE_MENU_MODE_LOAD ? "LOAD" : "DELETE";
                SDL_Color text_color = {150, 255, 150, alpha};
                SDL_Surface *surf = TTF_RenderText_Blended(menu->font, btn_text, 0, text_color);
                if (surf) {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                    float tw, th;
                    SDL_GetTextureSize(tex, &tw, &th);
                    SDL_SetTextureAlphaMod(tex, alpha);
                    SDL_FRect rect = {
                        menu->confirm_button.x + (menu->confirm_button.w - tw) / 2,
                        menu->confirm_button.y + (menu->confirm_button.h - th) / 2,
                        tw, th
                    };
                    SDL_RenderTexture(renderer, tex, NULL, &rect);
                    SDL_DestroyTexture(tex);
                    SDL_DestroySurface(surf);
                }
            }
        }
    }
    
    // Confirm delete dialog – blocks all other input until dismissed.
    if (menu->show_confirm_dialog) {
        // Darken the whole panel to focus on the dialog.
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(200 * menu->anim_alpha));
        SDL_FRect full = {menu->panel_x, menu->panel_y, menu->panel_w, menu->panel_h};
        SDL_RenderFillRect(renderer, &full);
        
        // Dialog box.
        SDL_FRect dialog = {
            menu->panel_x + 150,
            menu->panel_y + 250,
            500, 200
        };
        SDL_SetRenderDrawColor(renderer, 40, 40, 50, alpha);
        SDL_RenderFillRect(renderer, &dialog);
        SDL_SetRenderDrawColor(renderer, 200, 100, 100, alpha);
        SDL_RenderRect(renderer, &dialog);
        
        if (menu->font) {
            SDL_Color warn_color = {255, 100, 100, alpha};
            SDL_Surface *surf = TTF_RenderText_Blended(menu->font, 
                "Delete this save? This cannot be undone.", 0, warn_color);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                float tw, th;
                SDL_GetTextureSize(tex, &tw, &th);
                SDL_SetTextureAlphaMod(tex, alpha);
                SDL_FRect rect = {
                    dialog.x + (dialog.w - tw) / 2,
                    dialog.y + 40,
                    tw, th
                };
                SDL_RenderTexture(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(surf);
            }
        }
        
        // Yes/No buttons.
        SDL_FRect yes_btn = {dialog.x + 100, dialog.y + 120, 120, 40};
        SDL_FRect no_btn = {dialog.x + 280, dialog.y + 120, 120, 40};
        
        SDL_SetRenderDrawColor(renderer, 100, 40, 40, alpha);
        SDL_RenderFillRect(renderer, &yes_btn);
        SDL_SetRenderDrawColor(renderer, 200, 80, 80, alpha);
        SDL_RenderRect(renderer, &yes_btn);
        
        SDL_SetRenderDrawColor(renderer, 40, 80, 40, alpha);
        SDL_RenderFillRect(renderer, &no_btn);
        SDL_SetRenderDrawColor(renderer, 80, 200, 80, alpha);
        SDL_RenderRect(renderer, &no_btn);
        
        if (menu->font_small) {
            SDL_Surface *yes_surf = TTF_RenderText_Blended(menu->font_small, "YES", 0, 
                (SDL_Color){255, 150, 150, alpha});
            SDL_Surface *no_surf = TTF_RenderText_Blended(menu->font_small, "NO", 0,
                (SDL_Color){150, 255, 150, alpha});
            
            if (yes_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, yes_surf);
                float tw, th;
                SDL_GetTextureSize(tex, &tw, &th);
                SDL_FRect rect = {
                    yes_btn.x + (yes_btn.w - tw) / 2,
                    yes_btn.y + (yes_btn.h - th) / 2,
                    tw, th
                };
                SDL_RenderTexture(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(yes_surf);
            }
            
            if (no_surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, no_surf);
                float tw, th;
                SDL_GetTextureSize(tex, &tw, &th);
                SDL_FRect rect = {
                    no_btn.x + (no_btn.w - tw) / 2,
                    no_btn.y + (no_btn.h - th) / 2,
                    tw, th
                };
                SDL_RenderTexture(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(no_surf);
            }
        }
    }
}

/*
 * Handles mouse events. Returns true if the event was consumed.
 * We check the confirm dialog first because it blocks all other input.
 * Otherwise, we update hover states and process clicks.
 */
bool save_menu_handle_mouse(SaveMenu *menu, SDL_MouseButtonEvent *mouse) {
    if (!menu || !menu->is_open) return false;
    
    float mx = mouse->x;
    float my = mouse->y;
    
    // If the delete confirmation dialog is up, only the Yes/No buttons are active.
    if (menu->show_confirm_dialog) {
        SDL_FRect yes_btn = {
            menu->panel_x + 250,
            menu->panel_y + 370,
            120, 40
        };
        SDL_FRect no_btn = {
            menu->panel_x + 430,
            menu->panel_y + 370,
            120, 40
        };
        
        if (mouse->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (mx >= yes_btn.x && mx <= yes_btn.x + yes_btn.w &&
                my >= yes_btn.y && my <= yes_btn.y + yes_btn.h) {
                // User confirmed deletion.
                if (menu->selected_slot >= 0) {
                    delete_save(menu->save_system, menu->selected_slot);
                    if (menu->on_delete_completed) {
                        menu->on_delete_completed(menu->selected_slot);
                    }
                }
                menu->show_confirm_dialog = false;
                menu->selected_slot = -1;
                return true;
            }
            
            if (mx >= no_btn.x && mx <= no_btn.x + no_btn.w &&
                my >= no_btn.y && my <= no_btn.y + no_btn.h) {
                menu->show_confirm_dialog = false;
                return true;
            }
        }
        return true; // Block everything else while dialog is open.
    }
    
    // Update which slot (or button) the mouse is hovering over.
    menu->hovered_slot = -1;
    
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        SDL_FRect *slot = &menu->slot_rects[i];
        if (mx >= slot->x && mx <= slot->x + slot->w &&
            my >= slot->y && my <= slot->y + slot->h) {
            menu->hovered_slot = i;
            break;
        }
    }
    
    if (mx >= menu->back_button.x && mx <= menu->back_button.x + menu->back_button.w &&
        my >= menu->back_button.y && my <= menu->back_button.y + menu->back_button.h) {
        menu->hovered_slot = -2;
    }
    
    if (menu->selected_slot >= 0 &&
        mx >= menu->confirm_button.x && mx <= menu->confirm_button.x + menu->confirm_button.w &&
        my >= menu->confirm_button.y && my <= menu->confirm_button.y + menu->confirm_button.h) {
        menu->hovered_slot = -3;
    }
    
    if (mouse->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        // Click on a slot: select it. In load mode we immediately load if the slot is occupied.
        for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
            SDL_FRect *slot = &menu->slot_rects[i];
            if (mx >= slot->x && mx <= slot->x + slot->w &&
                my >= slot->y && my <= slot->y + slot->h) {
                menu->selected_slot = i;
                
                if (menu->mode == SAVE_MENU_MODE_LOAD && 
                    has_save_in_slot(menu->save_system, i)) {
                    SaveSlotData data;
                    if (load_game(menu->save_system, i, &data)) {
                        if (menu->on_load_selected) {
                            menu->on_load_selected(i, &data);
                        }
                        save_menu_close(menu);
                    }
                }
                return true;
            }
        }
        
        // Back button: cancel and close.
        if (mx >= menu->back_button.x && mx <= menu->back_button.x + menu->back_button.w &&
            my >= menu->back_button.y && my <= menu->back_button.y + menu->back_button.h) {
            if (menu->on_cancelled) menu->on_cancelled();
            save_menu_close(menu);
            return true;
        }
        
        // Confirm button: perform the current mode's action (or show delete dialog).
        if (menu->selected_slot >= 0 &&
            mx >= menu->confirm_button.x && mx <= menu->confirm_button.x + menu->confirm_button.w &&
            my >= menu->confirm_button.y && my <= menu->confirm_button.y + menu->confirm_button.h) {
            
            bool can_confirm = (menu->mode != SAVE_MENU_MODE_LOAD) || 
                             has_save_in_slot(menu->save_system, menu->selected_slot);
            
            if (can_confirm) {
                switch (menu->mode) {
                    case SAVE_MENU_MODE_SAVE:
                        if (menu->on_save_completed) {
                            menu->on_save_completed(menu->selected_slot);
                        }
                        save_menu_close(menu);
                        break;
                        
                    case SAVE_MENU_MODE_LOAD:
                        {
                            SaveSlotData data;
                            if (load_game(menu->save_system, menu->selected_slot, &data)) {
                                if (menu->on_load_selected) {
                                    menu->on_load_selected(menu->selected_slot, &data);
                                }
                                save_menu_close(menu);
                            }
                        }
                        break;
                        
                    case SAVE_MENU_MODE_DELETE:
                        menu->show_confirm_dialog = true;
                        break;
                }
            }
            return true;
        }
    }
    
    return menu->hovered_slot >= -3;
}

/*
 * Handles keyboard navigation.
 * Arrow keys move selection, Enter confirms, Escape cancels.
 * While the delete confirmation dialog is active, Y/N keys work.
 */
bool save_menu_handle_key(SaveMenu *menu, SDL_KeyboardEvent *key) {
    if (!menu || !menu->is_open) return false;
    
    if (key->type == SDL_EVENT_KEY_DOWN) {
        if (menu->show_confirm_dialog) {
            switch (key->scancode) {
                case SDL_SCANCODE_Y:
                    if (menu->selected_slot >= 0) {
                        delete_save(menu->save_system, menu->selected_slot);
                        if (menu->on_delete_completed) {
                            menu->on_delete_completed(menu->selected_slot);
                        }
                    }
                    menu->show_confirm_dialog = false;
                    return true;
                    
                case SDL_SCANCODE_N:
                case SDL_SCANCODE_ESCAPE:
                    menu->show_confirm_dialog = false;
                    return true;
                    
                default:
                    return true; // Block other keys while dialog is up.
            }
        }
        
        switch (key->scancode) {
            case SDL_SCANCODE_ESCAPE:
                if (menu->on_cancelled) menu->on_cancelled();
                save_menu_close(menu);
                return true;
                
            case SDL_SCANCODE_UP:
                if (menu->selected_slot > 0) {
                    menu->selected_slot--;
                } else {
                    menu->selected_slot = MAX_SAVE_SLOTS - 1; // wrap around
                }
                return true;
                
            case SDL_SCANCODE_DOWN:
                if (menu->selected_slot < MAX_SAVE_SLOTS - 1) {
                    menu->selected_slot++;
                } else {
                    menu->selected_slot = 0;
                }
                return true;
                
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_KP_ENTER:
                if (menu->selected_slot >= 0) {
                    // Simulate a click on the confirm button.
                    SDL_MouseButtonEvent fake = {0};
                    fake.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                    fake.button = SDL_BUTTON_LEFT;
                    fake.x = menu->confirm_button.x + 10;
                    fake.y = menu->confirm_button.y + 10;
                    return save_menu_handle_mouse(menu, &fake);
                }
                return true;
                
            default:
                break;
        }
    }
    
    return false;
}