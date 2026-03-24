#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "game_screen.h"

// PC settings
#define PC_FRAME_WIDTH 38
#define PC_FRAME_HEIGHT 49
#define PC_FRAME_COUNT 7
#define ANIMATION_SPEED 4.5
#define MOVE_SPEED 1.2f
#define ZOOM_LEVEL 5.0f
#define CAMERA_SMOOTH 0.1f

// NPC settings
#define NPC_FRAME_WIDTH 65
#define NPC_FRAME_HEIGHT 60
#define NPC_FRAME_COUNT_ANIM 2
#define NPC_FRAME_DURATION 0.3f

// Dialogue settings
#define DIALOGUE_BOX_HEIGHT 200
#define DIALOGUE_BOX_MARGIN 50
#define DIALOGUE_TEXT_SIZE 26
#define DIALOGUE_NAME_SIZE 30
#define DIALOGUE_CHAR_DELAY 0.03f
#define DIALOGUE_CONTINUE_DELAY 0.5f

// Portrait settings
#define PORTRAIT_WIDTH 500
#define PORTRAIT_HEIGHT 550 
#define PORTRAIT_OFFSET_X 50
#define PORTRAIT_OFFSET_Y 170

// Player portrait (right side)
#define PLAYER_PORTRAIT_WIDTH 300
#define PLAYER_PORTRAIT_HEIGHT 350
#define PLAYER_PORTRAIT_OFFSET_X 50
#define PLAYER_PORTRAIT_OFFSET_Y 175

static float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return value;
    return value;
}

static SDL_Texture* create_dialogue_box(SDL_Renderer *renderer, int width, int height) {
    SDL_Surface *surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) return NULL;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float alpha = 0.92f;
            Uint8 r = 15, g = 15, b = 35;
            
            // Border
            if (x < 5 || x >= width - 5 || y < 5 || y >= height - 5) {
                r = 120; g = 100; b = 80;  // Golden border
            }
            // Inner gradient
            else if (x < 20 || y < 20 || x >= width - 20 || y >= height - 20) {
                r = 25; g = 25; b = 50;
            }
            
            Uint32 pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), NULL, r, g, b, (Uint8)(255 * alpha));
            ((Uint32*)surface->pixels)[y * surface->w + x] = pixel;
        }
    }
    
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_DestroySurface(surface);
    return texture;
}

GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height) {
    GameScreen *gs = calloc(1, sizeof(GameScreen));
    if (!gs) {
        fprintf(stderr, "Failed to allocate game screen\n");
        return NULL;
    }

    // Load background/map
    SDL_Surface *surface = IMG_Load("game_assets/map_1.jpg");
    if (!surface) {
        fprintf(stderr, "Failed to load map: %s\n", SDL_GetError());
        surface = SDL_CreateSurface(window_width, window_height, SDL_PIXELFORMAT_RGBA8888);
        SDL_FillSurfaceRect(surface, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), NULL, 20, 40, 60, 255));
        gs->map_width = window_width;
        gs->map_height = window_height;
    } else {
        gs->map_width = surface->w;
        gs->map_height = surface->h;
    }
    
    gs->bg_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureScaleMode(gs->bg_texture, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(surface);

    if (!gs->bg_texture) {
        fprintf(stderr, "Failed to create game bg texture: %s\n", SDL_GetError());
        free(gs);
        return NULL;
    }

    printf("Map size: %dx%d\n", gs->map_width, gs->map_height);

    // Load PC sprites
    SDL_Surface *idle_surface = IMG_Load("game_assets/pc_idle.png");
    if (!idle_surface) {
        fprintf(stderr, "Failed to load idle sprite: %s\n", SDL_GetError());
    } else {
        gs->idle_texture = SDL_CreateTextureFromSurface(renderer, idle_surface);
        SDL_SetTextureScaleMode(gs->idle_texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(idle_surface);
    }

    SDL_Surface *run_surface = IMG_Load("game_assets/pc_run.png");
    if (!run_surface) {
        fprintf(stderr, "Failed to load run sprite: %s\n", SDL_GetError());
    } else {
        gs->run_texture = SDL_CreateTextureFromSurface(renderer, run_surface);
        SDL_SetTextureScaleMode(gs->run_texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(run_surface);
    }

    // Load fonts
    gs->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 36);
    if (!gs->font) {
        fprintf(stderr, "Game font load error: %s\n", SDL_GetError());
        SDL_DestroyTexture(gs->bg_texture);
        free(gs);
        return NULL;
    }

    gs->font_bold = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 36);
    if (gs->font_bold) {
        TTF_SetFontStyle(gs->font_bold, TTF_STYLE_BOLD);
    }

    // Dialogue font
    gs->font_dialogue = TTF_OpenFont("game_assets/Roboto_Medium.ttf", DIALOGUE_TEXT_SIZE);
    if (!gs->font_dialogue) {
        gs->font_dialogue = gs->font;
    }

    // Load NPC
    SDL_Surface *npc_surface = IMG_Load("game_assets/npc1_idle.png");
    if (!npc_surface) {
        fprintf(stderr, "Failed to load NPC sprite: %s\n", SDL_GetError());
    } else {
        gs->npc_idle_texture = SDL_CreateTextureFromSurface(renderer, npc_surface);
        SDL_SetTextureScaleMode(gs->npc_idle_texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(npc_surface);
    }

    // Try to load NPC portrait
    SDL_Surface *portrait_surface = IMG_Load("game_assets/npc1_portrait.png");
    if (!portrait_surface) {
        printf("No portrait found, will use scaled idle sprite\n");
        gs->npc_portrait = NULL;
    } else {
        gs->npc_portrait = SDL_CreateTextureFromSurface(renderer, portrait_surface);
        SDL_SetTextureScaleMode(gs->npc_portrait, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(portrait_surface);
    }

    // // Create dialogue box - now narrower to make room for portrait
    // int box_width = window_width - (DIALOGUE_BOX_MARGIN * 2) - PORTRAIT_SIZE + 100;
    // gs->dialogue_box_texture = create_dialogue_box(renderer, box_width, DIALOGUE_BOX_HEIGHT);

    // Animation state
    gs->frame_counter = 0;
    gs->current_frame = 0;
    gs->facing_right = true;
    gs->is_moving = false;
    
    gs->moving_left = false;
    gs->moving_right = false;
    gs->moving_up = false;
    gs->moving_down = false;

    // Camera
    gs->camera.zoom = ZOOM_LEVEL;
    gs->camera.viewport_w = window_width;
    gs->camera.viewport_h = window_height;
    gs->camera.x = 0;
    gs->camera.y = 0;

    // Title
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *title_surf = TTF_RenderText_Blended(gs->font, "GAME RUNNING - Press ESC for Menu", 0, white);
    gs->title_text = SDL_CreateTextureFromSurface(renderer, title_surf);
    SDL_DestroySurface(title_surf);

    // Player position
    gs->player_x = gs->map_width / 2.0f;
    gs->player_y = gs->map_height / 2.0f + 17;
    gs->score = 0;
    gs->level = 1;

    // NPC position
    gs->npc_x = gs->player_x + 35;
    gs->npc_y = gs->player_y - 10;

    // NPC animation
    gs->npc_frame = 0;
    gs->npc_frame_timer = 0.0f;
    gs->npc_frame_duration = NPC_FRAME_DURATION;
    gs->npc_facing_right = false;

    // Dialogue system init
    gs->in_dialogue = false;
    gs->dialogue_char_delay = DIALOGUE_CHAR_DELAY;
    gs->dialogue_timer = 0.0f;
    gs->dialogue_area_hovered = false;

    // Initialize dialogue box rect
    gs->dialogue_box_rect = (SDL_FRect){0, 0, 0, 0};

    printf("Game screen created\n");
    return gs;
}

void update_camera(GameScreen *gs) {
    Camera *cam = &gs->camera;
    
    float target_x = gs->player_x * cam->zoom - cam->viewport_w / 2.0f;
    float target_y = gs->player_y * cam->zoom - cam->viewport_h / 2.0f;
    
    cam->x += (target_x - cam->x) * CAMERA_SMOOTH;
    cam->y += (target_y - cam->y) * CAMERA_SMOOTH;
    
    float max_x = gs->map_width * cam->zoom - cam->viewport_w;
    float max_y = gs->map_height * cam->zoom - cam->viewport_h;
    
    cam->x = clamp(cam->x, 0, max_x > 0 ? max_x : 0);
    cam->y = clamp(cam->y, 0, max_y > 0 ? max_y : 0);
}

void game_screen_update(GameScreen *gs, float delta_time) {
    if (!gs) return;
    
    if (gs->in_dialogue) {
        dialogue_update(gs, delta_time, NULL);
        return;
    }
    
    gs->is_moving = gs->moving_left || gs->moving_right || 
                    gs->moving_up || gs->moving_down;
    
    if (gs->is_moving) {
        gs->frame_counter++;
        if (gs->frame_counter >= ANIMATION_SPEED) {
            gs->frame_counter = 0;
            gs->current_frame = (gs->current_frame + 1) % PC_FRAME_COUNT;
        }
    } else {
        gs->current_frame = 0;
        gs->frame_counter = 0;
    }
    
    if (gs->moving_right) gs->player_x += MOVE_SPEED;
    if (gs->moving_left) gs->player_x -= MOVE_SPEED;
    if (gs->moving_up) gs->player_y -= MOVE_SPEED;
    if (gs->moving_down) gs->player_y += MOVE_SPEED;
    
    gs->player_x = clamp(gs->player_x, 0, gs->map_width);
    gs->player_y = clamp(gs->player_y, 0, gs->map_height);

    gs->npc_frame_timer += delta_time;
    if (gs->npc_frame_timer >= gs->npc_frame_duration) {
        gs->npc_frame_timer = 0.0f;
        gs->npc_frame = (gs->npc_frame + 1) % NPC_FRAME_COUNT_ANIM;
    }

    if (gs->player_x > gs->npc_x) {
        gs->npc_facing_right = true;
    } else {
        gs->npc_facing_right = false;
    }
    
    float dx = gs->player_x - gs->npc_x;
    float dy = gs->player_y - gs->npc_y;
    float distance_sq = dx*dx + dy*dy;
    gs->player_near_npc = (distance_sq < 40 * 20);

    update_camera(gs);
}

// === DIALOGUE SYSTEM FUNCTIONS ===

void dialogue_start(GameScreen *gs, SDL_Renderer *renderer) {
    if (!gs || gs->in_dialogue) return;
    
    gs->in_dialogue = true;
    gs->dialogue_index = 0;
    gs->dialogue_char_index = 0;
    gs->dialogue_timer = 0.0f;
    gs->dialogue_text_complete = false;
    gs->dialogue_skip_requested = false;
    gs->waiting_for_player_choice = false;  // NEW
    gs->player_speaking = false;            // NEW
    
    // Initialize dialogue NPC animation
    gs->dialogue_npc_frame = 0;
    gs->dialogue_npc_timer = 0.0f;
    gs->dialogue_npc_frame_duration = 0.4f;

    // Naberius lines (0, 1, 2) -> Player choice -> Naberius final
    gs->dialogue_count = 3;
    
    strcpy(gs->dialogue_lines[0].speaker, "???");
    strcpy(gs->dialogue_lines[0].text, "...");
    
    strcpy(gs->dialogue_lines[1].speaker, "Naberius");
    strcpy(gs->dialogue_lines[1].text, "I am Naberius, the Gate Keeper. I am the voice that whispers in the silence before the fall. Some call me a demon, a warden, a crow perched upon the edge of eternity.");
    
    strcpy(gs->dialogue_lines[2].speaker, "Naberius");
    strcpy(gs->dialogue_lines[2].text, "Vermin Soul, You are not supposed to be here..., Retrace your steps, NOW!.");
    
    // Player response (stored separately)
    strcpy(gs->player_dialogue_text, "I'm not taking any step behind my back!");
    
    // Clear cached textures
    if (gs->dialogue_name_texture) {
        SDL_DestroyTexture(gs->dialogue_name_texture);
        gs->dialogue_name_texture = NULL;
    }
    
    printf("Dialogue started - Line 1/%d\n", gs->dialogue_count);
}

void dialogue_advance(GameScreen *gs, SDL_Renderer *renderer) {
    if (!gs || !gs->in_dialogue) return;
    
    // If waiting for player choice, don't advance normally
    if (gs->waiting_for_player_choice) {
        return;
    }
    
    if (!gs->dialogue_text_complete) {
        gs->dialogue_skip_requested = true;
        printf("Skipping to end of line\n");
    } else {
        gs->dialogue_index++;
        printf("Advancing to line %d/%d\n", gs->dialogue_index + 1, gs->dialogue_count);
        
        // Check if we need to show player choice after line 2
        if (gs->dialogue_index == 3) {
            // Finished Naberius's 3 lines, now wait for player
            gs->waiting_for_player_choice = true;
            gs->dialogue_index = 2; // Stay on last line for context
            printf("Waiting for player choice...\n");
            return;
        }
        
        // Check if player just spoke, now show Naberius response
        if (gs->player_speaking) {
            gs->player_speaking = false;
            // Naberius final response
            strcpy(gs->dialogue_lines[0].speaker, "Naberius");
            strcpy(gs->dialogue_lines[0].text, "Kekekekek, you misunderstood it human. That was not a request. That was the only mercy I offer and you just threw it back. So now… we do this the HARD WAY. Don't disappoint me.");
            gs->dialogue_index = 0;
            gs->dialogue_count = 1;
            gs->dialogue_char_index = 0;
            gs->dialogue_timer = 0.0f;
            gs->dialogue_text_complete = false;
            
            // NEW: Mark battle to start after this dialogue WITH animation
            gs->battle_triggered = true;
            // The opening animation will play before battle
            
            if (gs->dialogue_name_texture) {
                SDL_DestroyTexture(gs->dialogue_name_texture);
                gs->dialogue_name_texture = NULL;
            }
            return;
        }
        
        if (gs->dialogue_index >= gs->dialogue_count) {
            dialogue_end(gs);
        } else {
            gs->dialogue_char_index = 0;
            gs->dialogue_timer = 0.0f;
            gs->dialogue_text_complete = false;
            gs->dialogue_skip_requested = false;
            
            if (gs->dialogue_name_texture) {
                SDL_DestroyTexture(gs->dialogue_name_texture);
                gs->dialogue_name_texture = NULL;
            }
        }
    }
}

void dialogue_end(GameScreen *gs) {
    if (!gs) return;
    gs->in_dialogue = false;
    gs->dialogue_index = 0;
    gs->dialogue_char_index = 0;
    
    if (gs->dialogue_name_texture) {
        SDL_DestroyTexture(gs->dialogue_name_texture);
        gs->dialogue_name_texture = NULL;
    }
    
    printf("Dialogue ended\n");
}

void dialogue_update(GameScreen *gs, float delta_time, SDL_Renderer *renderer) {
    if (!gs || !gs->in_dialogue) return;
    
    gs->dialogue_timer += delta_time;
    
    // Animate NPC during dialogue
    gs->dialogue_npc_timer += delta_time;
    if (gs->dialogue_npc_timer >= gs->dialogue_npc_frame_duration) {
        gs->dialogue_npc_timer = 0.0f;
        gs->dialogue_npc_frame = (gs->dialogue_npc_frame + 1) % NPC_FRAME_COUNT_ANIM;
    }
    
    if (!gs->dialogue_text_complete) {
        const char *current_text = gs->dialogue_lines[gs->dialogue_index].text;
        int text_len = strlen(current_text);
        
        if (gs->dialogue_skip_requested) {
            gs->dialogue_char_index = text_len;
            gs->dialogue_text_complete = true;
            gs->dialogue_skip_requested = false;
        } else if (gs->dialogue_timer >= gs->dialogue_char_delay) {
            gs->dialogue_timer = 0.0f;
            if (gs->dialogue_char_index < text_len) {
                gs->dialogue_char_index++;
            } else {
                gs->dialogue_text_complete = true;
            }
        }
    }
}

bool dialogue_handle_mouse(GameScreen *gs, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer) {
    if (!gs || !gs->in_dialogue) return false;
    
    if (mouse->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (mouse->button == SDL_BUTTON_LEFT) {
            // Check if clicking answer bar during choice
            if (gs->waiting_for_player_choice) {
                float mx = mouse->x;
                float my = mouse->y;
                if (mx >= gs->answer_bar_rect.x && 
                    mx <= gs->answer_bar_rect.x + gs->answer_bar_rect.w &&
                    my >= gs->answer_bar_rect.y && 
                    my <= gs->answer_bar_rect.y + gs->answer_bar_rect.h) {
                    dialogue_handle_player_choice(gs, renderer);
                    return true;
                }
            } else {
                dialogue_advance(gs, renderer);
                return true;
            }
        }
    }
    
    return false;
}

bool dialogue_handle_key(GameScreen *gs, SDL_KeyboardEvent *key, SDL_Renderer *renderer) {
    if (!gs || !gs->in_dialogue) return false;
    
    if (key->type == SDL_EVENT_KEY_DOWN) {
        switch (key->scancode) {
            case SDL_SCANCODE_E:
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_SPACE:
                dialogue_advance(gs, renderer);
                return true;
                
            case SDL_SCANCODE_ESCAPE:
                dialogue_end(gs);
                return true;
                
            default:
                break;
        }
    }
    
    return false;
}

void dialogue_render(SDL_Renderer *renderer, GameScreen *gs) {
    if (!gs || !gs->in_dialogue || !renderer) return;
    
    int window_w = gs->camera.viewport_w;
    int window_h = gs->camera.viewport_h;
    
    // Create dialogue box texture if needed
    if (!gs->dialogue_box_texture) {
        int box_w = window_w - (DIALOGUE_BOX_MARGIN * 2);
        gs->dialogue_box_texture = create_dialogue_box(renderer, box_w, DIALOGUE_BOX_HEIGHT);
    }
    
    // === DARKEN BACKGROUND MORE when waiting for choice ===
    if (gs->waiting_for_player_choice) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_FRect darken_rect = {0, 0, window_w, window_h};
        SDL_RenderFillRect(renderer, &darken_rect);
    } else {
        // Normal darken during regular dialogue
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);
        SDL_FRect darken_rect = {0, 0, window_w, window_h};
        SDL_RenderFillRect(renderer, &darken_rect);
    }
    
    // === DRAW NPC PORTRAIT (LEFT SIDE) ===
    if (!gs->player_speaking) {
        int portrait_w = PORTRAIT_WIDTH;
        int portrait_h = PORTRAIT_HEIGHT;
        int portrait_x = PORTRAIT_OFFSET_X;
        int portrait_y = window_h - portrait_h - PORTRAIT_OFFSET_Y;
        
        if (gs->waiting_for_player_choice) {
            SDL_SetTextureAlphaMod(gs->npc_idle_texture, 150);
        } else {
            SDL_SetTextureAlphaMod(gs->npc_idle_texture, 255);
        }
        
        if (gs->npc_idle_texture) {
            SDL_FRect src = {
                (float)(gs->dialogue_npc_frame * NPC_FRAME_WIDTH),
                (float)(2 * (NPC_FRAME_HEIGHT + 4)),
                NPC_FRAME_WIDTH, 
                NPC_FRAME_HEIGHT
            };
            SDL_FRect dst = {portrait_x, portrait_y, portrait_w, portrait_h};
            SDL_RenderTexture(renderer, gs->npc_idle_texture, &src, &dst);
        }
        
        SDL_SetTextureAlphaMod(gs->npc_idle_texture, 255);
    }
    
    // === DRAW PLAYER PORTRAIT (RIGHT SIDE) when player speaking ===
    if (gs->player_speaking) {
        int portrait_w = PLAYER_PORTRAIT_WIDTH;
        int portrait_h = PLAYER_PORTRAIT_HEIGHT;
        int portrait_x = window_w - portrait_w - PLAYER_PORTRAIT_OFFSET_X;
        int portrait_y = window_h - portrait_h - PLAYER_PORTRAIT_OFFSET_Y;
        
        if (gs->idle_texture) {
            SDL_FRect src = {
                2.0f,
                207,
                PC_FRAME_WIDTH,
                PC_FRAME_HEIGHT
            };
            SDL_FRect dst = {portrait_x, portrait_y, portrait_w, portrait_h};
            SDL_RenderTextureRotated(renderer, gs->idle_texture, &src, &dst, 0.0, NULL, SDL_FLIP_HORIZONTAL);
        }
    }
    
    // === DRAW DIALOGUE BOX (for normal dialogue) ===
    if (!gs->waiting_for_player_choice) {
        int box_h = DIALOGUE_BOX_HEIGHT;
        int box_w = window_w - (DIALOGUE_BOX_MARGIN * 2);
        int box_x = DIALOGUE_BOX_MARGIN;
        int box_y = window_h - box_h - DIALOGUE_BOX_MARGIN;
        
        gs->dialogue_box_rect = (SDL_FRect){box_x, box_y, box_w, box_h};
        
        if (gs->dialogue_box_texture) {
            SDL_FRect box_rect = {box_x, box_y, box_w, box_h};
            SDL_RenderTexture(renderer, gs->dialogue_box_texture, NULL, &box_rect);
        }
        
        // Text area
        int text_x = box_x + 40;
        int text_y = box_y + 25;
        int max_text_w = box_w - 80;
        
        // Speaker name
        const char *speaker = gs->dialogue_lines[gs->dialogue_index].speaker;
        SDL_Color name_color = gs->player_speaking ? (SDL_Color){100, 200, 255, 255} : (SDL_Color){255, 215, 100, 255};
        
        if (!gs->dialogue_name_texture) {
            SDL_Surface *name_surf = TTF_RenderText_Blended(gs->font_bold ? gs->font_bold : gs->font, 
                                                            speaker, 0, name_color);
            if (name_surf) {
                gs->dialogue_name_texture = SDL_CreateTextureFromSurface(renderer, name_surf);
                SDL_DestroySurface(name_surf);
            }
        }
        
        if (gs->dialogue_name_texture) {
            float name_w, name_h;
            SDL_GetTextureSize(gs->dialogue_name_texture, &name_w, &name_h);
            SDL_FRect name_rect = {text_x, text_y, name_w, name_h};
            SDL_RenderTexture(renderer, gs->dialogue_name_texture, NULL, &name_rect);
            text_y += name_h + 15;
        }
        
        // Dialogue text
        const char *full_text = gs->dialogue_lines[gs->dialogue_index].text;
        char display_text[MAX_LINE_LENGTH];
        strncpy(display_text, full_text, gs->dialogue_char_index);
        display_text[gs->dialogue_char_index] = '\0';
        
        SDL_Color text_color = {255, 255, 255, 255};
        SDL_Surface *text_surf = TTF_RenderText_Blended_Wrapped(gs->font_dialogue, 
                                                                display_text, 0, 
                                                                text_color, max_text_w);
        if (text_surf) {
            SDL_Texture *text_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
            float text_w, text_h;
            SDL_GetTextureSize(text_tex, &text_w, &text_h);
            
            SDL_FRect text_rect = {text_x, text_y, text_w, text_h};
            SDL_RenderTexture(renderer, text_tex, NULL, &text_rect);
            SDL_DestroyTexture(text_tex);
            SDL_DestroySurface(text_surf);
        }
        
        // Continue indicator
        if (gs->dialogue_text_complete) {
            float blink = (SDL_GetTicks() / 500) % 2;
            if (blink) {
                SDL_Color indicator_color = {255, 255, 255, 200};
                SDL_Surface *ind_surf = TTF_RenderText_Blended(gs->font, "▼", 0, indicator_color);
                if (ind_surf) {
                    SDL_Texture *ind_tex = SDL_CreateTextureFromSurface(renderer, ind_surf);
                    float ind_w, ind_h;
                    SDL_GetTextureSize(ind_tex, &ind_w, &ind_h);
                    
                    SDL_FRect ind_rect = {
                        box_x + box_w - ind_w - 30, 
                        box_y + box_h - ind_h - 20, 
                        ind_w, 
                        ind_h
                    };
                    SDL_RenderTexture(renderer, ind_tex, NULL, &ind_rect);
                    SDL_DestroyTexture(ind_tex);
                    SDL_DestroySurface(ind_surf);
                }
            }
        }
        
        // Hint text
        SDL_Color hint_color = {180, 180, 180, 180};
        const char *hint_text = gs->dialogue_text_complete ? "Click, E, Enter, or Space to continue" : "Click, E, Enter, or Space to skip";
        SDL_Surface *hint_surf = TTF_RenderText_Blended(gs->font_dialogue, hint_text, 0, hint_color);
        if (hint_surf) {
            SDL_Texture *hint_tex = SDL_CreateTextureFromSurface(renderer, hint_surf);
            float hint_w, hint_h;
            SDL_GetTextureSize(hint_tex, &hint_w, &hint_h);
            
            SDL_FRect hint_rect = {
                (window_w - hint_w) / 2,
                window_h - 35,
                hint_w,
                hint_h
            };
            SDL_RenderTexture(renderer, hint_tex, NULL, &hint_rect);
            SDL_DestroyTexture(hint_tex);
            SDL_DestroySurface(hint_surf);
        }
    }
    
        // === DRAW ANSWER BAR LAST (IN FRONT OF EVERYTHING) ===
    if (gs->waiting_for_player_choice) {
        const char *answer_text = "I'm not retreating!";
        SDL_Color answer_color = {255, 80, 80, 255};
        
        float pulse = 0.8f + 0.2f * sinf(SDL_GetTicks() / 200.0f);
        SDL_Color glow_color = {
            (Uint8)(255 * pulse),
            (Uint8)(100 * pulse),
            (Uint8)(100 * pulse),
            255
        };
        
        int bar_padding = 40;
        int bar_w, bar_h;
        int bar_x, bar_y;
        int hint_y;
        
        // Draw FULL SCREEN dark background first (cover everything)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
        SDL_FRect full_screen_dark = {0, 0, window_w, window_h};
        SDL_RenderFillRect(renderer, &full_screen_dark);
        
        SDL_Surface *answer_surf = TTF_RenderText_Blended(gs->font_bold, answer_text, 0, glow_color);
        if (answer_surf) {
            SDL_Texture *answer_tex = SDL_CreateTextureFromSurface(renderer, answer_surf);
            float text_w, text_h;
            SDL_GetTextureSize(answer_tex, &text_w, &text_h);
            
            bar_w = text_w + (bar_padding * 2);
            bar_h = text_h + (bar_padding * 2);
            
            bar_x = (window_w - bar_w) / 2;
            bar_y = (window_h - bar_h) / 2;
            
            hint_y = bar_y + bar_h + 80;
            
            // Draw bar background
            SDL_SetRenderDrawColor(renderer, 80, 20, 20, 240);
            gs->answer_bar_rect = (SDL_FRect){bar_x, bar_y, bar_w, bar_h};
            SDL_RenderFillRect(renderer, &gs->answer_bar_rect);
            
            // Bright border
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
            SDL_RenderRect(renderer, &gs->answer_bar_rect);
            
            // Inner highlight border
            SDL_SetRenderDrawColor(renderer, 255, 150, 150, 200);
            SDL_FRect inner_rect = {bar_x + 3, bar_y + 3, bar_w - 6, bar_h - 6};
            SDL_RenderRect(renderer, &inner_rect);
            
            // Draw text
            SDL_FRect text_rect = {
                bar_x + bar_padding,
                bar_y + bar_padding,
                text_w,
                text_h
            };
            SDL_RenderTexture(renderer, answer_tex, NULL, &text_rect);
            SDL_DestroyTexture(answer_tex);
            SDL_DestroySurface(answer_surf);
        }
        
        // Draw hint BELOW the centered bar
        SDL_Color hint_color = {220, 220, 220, 200};
        const char *hint = ">> CLICK TO RESPOND <<";
        SDL_Surface *hint_surf = TTF_RenderText_Blended(gs->font_bold, hint, 0, hint_color);
        if (hint_surf) {
            SDL_Texture *hint_tex = SDL_CreateTextureFromSurface(renderer, hint_surf);
            float hint_w, hint_h;
            SDL_GetTextureSize(hint_tex, &hint_w, &hint_h);
            
            SDL_FRect hint_rect = {
                (window_w - hint_w) / 2,
                hint_y,
                hint_w,
                hint_h
            };
            SDL_RenderTexture(renderer, hint_tex, NULL, &hint_rect);
            SDL_DestroyTexture(hint_tex);
            SDL_DestroySurface(hint_surf);
        }
    }
}

void game_screen_render(SDL_Renderer *renderer, GameScreen *gs) {
    if (!gs || !renderer) return;

    Camera *cam = &gs->camera;
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_FRect src_rect = {
        .x = cam->x / cam->zoom,
        .y = cam->y / cam->zoom,
        .w = cam->viewport_w / cam->zoom,
        .h = cam->viewport_h / cam->zoom
    };
    
    if (src_rect.x + src_rect.w > gs->map_width) {
        src_rect.w = gs->map_width - src_rect.x;
    }
    if (src_rect.y + src_rect.h > gs->map_height) {
        src_rect.h = gs->map_height - src_rect.y;
    }
    
    SDL_FRect dst_rect = {
        .x = 0,
        .y = 0,
        .w = cam->viewport_w,
        .h = cam->viewport_h
    };
    
    SDL_RenderTexture(renderer, gs->bg_texture, &src_rect, &dst_rect);

    float screen_x = (gs->player_x * cam->zoom) - cam->x;
    float screen_y = (gs->player_y * cam->zoom) - cam->y;

    float npc_screen_x = (gs->npc_x * cam->zoom) - cam->x;
    float npc_screen_y = (gs->npc_y * cam->zoom) - cam->y;

    float sprite_size = 16 * cam->zoom;
    float npc_sprite_size = 35 * cam->zoom;

    if (gs->npc_idle_texture && !gs->in_dialogue) {  // Add && !gs->in_dialogue
        int row = gs->npc_facing_right ? 3 : 1;
        
        SDL_FRect src_sprite = {
            .x = (float)(gs->npc_frame * NPC_FRAME_WIDTH),
            .y = (float)(row * (NPC_FRAME_HEIGHT + 4)),
            .w = NPC_FRAME_WIDTH,
            .h = NPC_FRAME_HEIGHT
        };

        SDL_FRect dst_sprite = {
            .x = npc_screen_x - npc_sprite_size / 2,
            .y = npc_screen_y - npc_sprite_size / 2,
            .w = npc_sprite_size,
            .h = npc_sprite_size
        };

        SDL_RenderTexture(renderer, gs->npc_idle_texture, &src_sprite, &dst_sprite);
    }

    SDL_Texture *current_texture = gs->is_moving ? gs->run_texture : gs->idle_texture;
    
    if (current_texture && !gs->in_dialogue) {
        SDL_FRect src_sprite = {
            .x = 2.0f + (gs->current_frame * PC_FRAME_WIDTH),
            .y = 207,
            .w = PC_FRAME_WIDTH,
            .h = PC_FRAME_HEIGHT
        };
        
        SDL_FRect dst_sprite = {
            .x = screen_x - sprite_size / 2,
            .y = screen_y - sprite_size / 2,
            .w = sprite_size,
            .h = sprite_size
        };
        
        SDL_FlipMode flip = gs->facing_right ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        
        SDL_RenderTextureRotated(renderer, current_texture, &src_sprite, &dst_sprite, 0.0, NULL, flip);
    }

    if (!gs->in_dialogue) {
        if (gs->player_near_npc) {
            SDL_Surface *surf = TTF_RenderText_Blended(
                gs->font,
                "Press E to talk",
                0,
                (SDL_Color){255,255,0,255}
            );

            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_DestroySurface(surf);

            float w, h;
            SDL_GetTextureSize(tex, &w, &h);

            SDL_FRect rect = {
                .x = npc_screen_x - w / 2,
                .y = npc_screen_y - npc_sprite_size / 2 - 5,
                .w = w,
                .h = h
            };

            SDL_RenderTexture(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
        }
        else {
            TTF_Font *font_to_use = gs->font_bold ? gs->font_bold : gs->font;
            
            SDL_Surface *surf = TTF_RenderText_Blended(
                font_to_use,
                "Who are you? Where are you going?",
                0,
                (SDL_Color){255,0,0,125}
            );

            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_DestroySurface(surf);

            float w, h;
            SDL_GetTextureSize(tex, &w, &h);

            SDL_FRect rect = {
                .x = npc_screen_x - w / 2,
                .y = npc_screen_y - npc_sprite_size / 2 + 5,
                .w = w,
                .h = h
            };

            SDL_RenderTexture(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
        }
    }

    if (!gs->in_dialogue) {
        float w, h;
        SDL_GetTextureSize(gs->title_text, &w, &h);
        SDL_FRect title_rect = {
            .x = (cam->viewport_w - w) / 2,
            .y = 30,
            .w = w,
            .h = h
        };
        SDL_RenderTexture(renderer, gs->title_text, NULL, &title_rect);
    }

    if (gs->in_dialogue) {
        dialogue_render(renderer, gs);
    }
}

void game_screen_handle_input(GameScreen *gs, SDL_KeyboardEvent *key) {
    if (!gs) return;

    bool is_pressed = (key->type == SDL_EVENT_KEY_DOWN);

    if (gs->in_dialogue) {
        dialogue_handle_key(gs, key, NULL);
        return;
    }

    switch (key->scancode) {
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
            gs->moving_right = is_pressed;
            if (is_pressed) gs->facing_right = true;
            break;
            
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
            gs->moving_left = is_pressed;
            if (is_pressed) gs->facing_right = false;
            break;
            
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
            gs->moving_up = is_pressed;
            break;
            
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
            gs->moving_down = is_pressed;
            break;

        case SDL_SCANCODE_E:
            if (is_pressed && gs->player_near_npc && !gs->in_dialogue) {
                dialogue_start(gs, NULL);
            }
            break;
            
        default:
            break;
    }
}

void game_screen_handle_mouse(GameScreen *gs, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer) {
    if (!gs) return;
    
    if (gs->in_dialogue) {
        dialogue_handle_mouse(gs, mouse, renderer);
        return;
    }
}

void dialogue_handle_player_choice(GameScreen *gs, SDL_Renderer *renderer) {
    if (!gs || !gs->waiting_for_player_choice) return;
    
    // Player clicked the answer bar
    gs->waiting_for_player_choice = false;
    gs->player_speaking = true;
    
    // Set up player dialogue
    strcpy(gs->dialogue_lines[0].speaker, "You");
    strcpy(gs->dialogue_lines[0].text, gs->player_dialogue_text);
    gs->dialogue_index = 0;
    gs->dialogue_count = 1;
    gs->dialogue_char_index = 0;
    gs->dialogue_timer = 0.0f;
    gs->dialogue_text_complete = false;
    
    if (gs->dialogue_name_texture) {
        SDL_DestroyTexture(gs->dialogue_name_texture);
        gs->dialogue_name_texture = NULL;
    }
    
    printf("Player chose: %s\n", gs->player_dialogue_text);
}

void game_screen_destroy(GameScreen *gs) {
    if (gs) {
        if (gs->title_text) SDL_DestroyTexture(gs->title_text);
        if (gs->font) TTF_CloseFont(gs->font);
        if (gs->font_bold) TTF_CloseFont(gs->font_bold);
        if (gs->font_dialogue && gs->font_dialogue != gs->font) TTF_CloseFont(gs->font_dialogue);
        if (gs->bg_texture) SDL_DestroyTexture(gs->bg_texture);
        if (gs->idle_texture) SDL_DestroyTexture(gs->idle_texture);
        if (gs->run_texture) SDL_DestroyTexture(gs->run_texture);
        if (gs->npc_idle_texture) SDL_DestroyTexture(gs->npc_idle_texture);
        if (gs->npc_portrait) SDL_DestroyTexture(gs->npc_portrait);
        if (gs->dialogue_box_texture) SDL_DestroyTexture(gs->dialogue_box_texture);
        if (gs->dialogue_name_texture) SDL_DestroyTexture(gs->dialogue_name_texture);
        free(gs);
        printf("Game screen destroyed\n");
    }
}