// ============================================================================
// game_screen.c - Game world rendering and dialogue system
// ============================================================================
//
// This file implements the main gameplay screen: player movement, NPC interaction,
// camera following, and a full dialogue system that supports branching conversations,
// choices, and battle triggers.
//
// Key assumptions:
// - Sprite sheets have fixed frame dimensions (PC: 38x49, 7 frames; NPC: 65x60, 2 frames)
// - Player animation frames are stored in a single row at Y=207 in the texture
// - NPC frames are arranged in rows: row 0 = facing left animation, row 3 = facing right
// - Dialogue files follow the format defined in dialogue_loader.h
// - Font "Roboto_Medium.ttf" is present in game_assets/
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "game_screen.h"

// ----------------------------------------------------------------------------
// Constants – Control how the game feels
// ----------------------------------------------------------------------------

// Player sprite dimensions (pixels in source texture)
#define PC_FRAME_WIDTH 38
#define PC_FRAME_HEIGHT 49
#define PC_FRAME_COUNT 7               // Number of frames in the walk/run cycle

// Animation speed: lower values = faster animation
#define ANIMATION_SPEED 4.5            // Frames of game loop per sprite change
#define MOVE_SPEED 1.2f                // World units per second
#define ZOOM_LEVEL 5.0f                // Camera zoom (5x = closer to player)
#define CAMERA_SMOOTH 0.1f             // How quickly camera catches up to player

// NPC sprite dimensions (pixels in source texture)
#define NPC_FRAME_WIDTH 65
#define NPC_FRAME_HEIGHT 60
#define NPC_FRAME_COUNT_ANIM 2         // Number of frames in NPC idle animation
#define NPC_FRAME_DURATION 0.3f        // Seconds per frame

// Dialogue box dimensions and positioning
#define DIALOGUE_BOX_HEIGHT 200
#define DIALOGUE_BOX_MARGIN 50
#define DIALOGUE_TEXT_SIZE 26          // Font size for dialogue text
#define DIALOGUE_NAME_SIZE 30          // Font size for speaker name
#define DIALOGUE_CHAR_DELAY 0.03f      // Seconds between letters in typewriter effect
#define DIALOGUE_CONTINUE_DELAY 0.5f   // (unused) Originally for auto‑advance

// NPC portrait size and position (left side of screen)
#define PORTRAIT_WIDTH 500
#define PORTRAIT_HEIGHT 550
#define PORTRAIT_OFFSET_X 50
#define PORTRAIT_OFFSET_Y 170

// Player portrait (right side) – shown when player speaks
#define PLAYER_PORTRAIT_WIDTH 300
#define PLAYER_PORTRAIT_HEIGHT 350
#define PLAYER_PORTRAIT_OFFSET_X 50
#define PLAYER_PORTRAIT_OFFSET_Y 175

// ----------------------------------------------------------------------------
// Helper: Clamp a float between min and max
// ----------------------------------------------------------------------------
static float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return value;
    return value;
}

// ----------------------------------------------------------------------------
// Create a textured dialogue box with a golden border and gradient interior
// ----------------------------------------------------------------------------
static SDL_Texture* create_dialogue_box(SDL_Renderer *renderer, int width, int height) {
    // Create a blank RGBA surface of the requested size
    SDL_Surface *surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) return NULL;
    
    // Draw each pixel manually – simple but effective for a small, static box
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float alpha = 0.92f;            // Mostly opaque
            Uint8 r = 15, g = 15, b = 35;   // Dark blue‑grey base

            // Border pixels (outer 5 pixels) get a golden colour
            if (x < 5 || x >= width - 5 || y < 5 || y >= height - 5) {
                r = 120; g = 100; b = 80;    // Golden border
            }
            // Inner decorative band (next 15 pixels) gets a slightly lighter blue
            else if (x < 20 || y < 20 || x >= width - 20 || y >= height - 20) {
                r = 25; g = 25; b = 50;
            }
            
            Uint32 pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), NULL,
                                        r, g, b, (Uint8)(255 * alpha));
            ((Uint32*)surface->pixels)[y * surface->w + x] = pixel;
        }
    }
    
    // Convert surface to a texture and enable alpha blending
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_DestroySurface(surface);
    return texture;
}

// ----------------------------------------------------------------------------
// GameScreen creation – load all assets, set up initial state
// ----------------------------------------------------------------------------
GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height) {
    GameScreen *gs = calloc(1, sizeof(GameScreen));
    if (!gs) {
        fprintf(stderr, "Failed to allocate game screen\n");
        return NULL;
    }

    // Load the background map (static image of the game world)
    SDL_Surface *surface = IMG_Load("game_assets/map_1.jpg");
    if (!surface) {
        fprintf(stderr, "Failed to load map: %s\n", SDL_GetError());
        // Fallback: create a blank dark surface the size of the window
        surface = SDL_CreateSurface(window_width, window_height, SDL_PIXELFORMAT_RGBA8888);
        SDL_FillSurfaceRect(surface, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format),
                            NULL, 20, 40, 60, 255));
        gs->map_width = window_width;
        gs->map_height = window_height;
    } else {
        // Store actual map dimensions (the image may be larger than the window)
        gs->map_width = surface->w;
        gs->map_height = surface->h;
    }
    
    gs->bg_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureScaleMode(gs->bg_texture, SDL_SCALEMODE_NEAREST); // Keep crisp pixels
    SDL_DestroySurface(surface);

    if (!gs->bg_texture) {
        fprintf(stderr, "Failed to create game bg texture: %s\n", SDL_GetError());
        free(gs);
        return NULL;
    }

    printf("Map size: %dx%d\n", gs->map_width, gs->map_height);

    // Load player sprites (idle and run animations)
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

    // Load fonts – regular and bold
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

    // Dialogue font – smaller size for text boxes
    gs->font_dialogue = TTF_OpenFont("game_assets/Roboto_Medium.ttf", DIALOGUE_TEXT_SIZE);
    if (!gs->font_dialogue) {
        gs->font_dialogue = gs->font;   // Fallback to the normal font
    }

    // Load NPC sprite (world idle animation)
    SDL_Surface *npc_surface = IMG_Load("game_assets/npc1_idle.png");
    if (!npc_surface) {
        fprintf(stderr, "Failed to load NPC sprite: %s\n", SDL_GetError());
    } else {
        gs->npc_idle_texture = SDL_CreateTextureFromSurface(renderer, npc_surface);
        SDL_SetTextureScaleMode(gs->npc_idle_texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(npc_surface);
    }

    // Try to load a separate portrait for the NPC (used in dialogue)
    SDL_Surface *portrait_surface = IMG_Load("game_assets/npc1_portrait.png");
    if (!portrait_surface) {
        printf("No portrait found, will use scaled idle sprite\n");
        gs->npc_portrait = NULL;
    } else {
        gs->npc_portrait = SDL_CreateTextureFromSurface(renderer, portrait_surface);
        SDL_SetTextureScaleMode(gs->npc_portrait, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(portrait_surface);
    }

    // Animation state initialisation
    gs->frame_counter = 0;
    gs->current_frame = 0;
    gs->facing_right = true;
    gs->is_moving = false;
    
    gs->moving_left = false;
    gs->moving_right = false;
    gs->moving_up = false;
    gs->moving_down = false;

    // Camera – starts centred on player
    gs->camera.zoom = ZOOM_LEVEL;
    gs->camera.viewport_w = window_width;
    gs->camera.viewport_h = window_height;
    gs->camera.x = 0;
    gs->camera.y = 0;

    // Create a title texture (just for info)
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *title_surf = TTF_RenderText_Blended(gs->font, "GAME RUNNING - Press ESC for Menu", 0, white);
    gs->title_text = SDL_CreateTextureFromSurface(renderer, title_surf);
    SDL_DestroySurface(title_surf);

    // Start player in the middle of the map (slightly offset vertically)
    gs->player_x = gs->map_width / 2.0f;
    gs->player_y = gs->map_height / 2.0f + 17;
    gs->score = 0;
    gs->level = 1;

    // Place NPC slightly offset from the player
    gs->npc_x = gs->player_x + 35;
    gs->npc_y = gs->player_y - 10;

    // NPC world animation
    gs->npc_frame = 0;
    gs->npc_frame_timer = 0.0f;
    gs->npc_frame_duration = NPC_FRAME_DURATION;
    gs->npc_facing_right = false;

    // Dialogue system initialisation
    gs->in_dialogue = false;
    gs->dialogue_char_delay = DIALOGUE_CHAR_DELAY;
    gs->dialogue_timer = 0.0f;
    gs->dialogue_area_hovered = false;
    gs->choice_a_hovered = false;
    gs->choice_b_hovered = false;

    // Dialogue box rectangle will be set later when rendering
    gs->dialogue_box_rect = (SDL_FRect){0, 0, 0, 0};

    printf("Game screen created\n");
    return gs;
}

// ----------------------------------------------------------------------------
// Update camera to smoothly follow the player
// ----------------------------------------------------------------------------
void update_camera(GameScreen *gs) {
    Camera *cam = &gs->camera;
    
    // Target camera position: centre the player in the viewport
    float target_x = gs->player_x * cam->zoom - cam->viewport_w / 2.0f;
    float target_y = gs->player_y * cam->zoom - cam->viewport_h / 2.0f;
    
    // Move camera towards target with smoothing
    cam->x += (target_x - cam->x) * CAMERA_SMOOTH;
    cam->y += (target_y - cam->y) * CAMERA_SMOOTH;
    
    // Clamp to world boundaries (prevent showing empty space)
    float max_x = gs->map_width * cam->zoom - cam->viewport_w;
    float max_y = gs->map_height * cam->zoom - cam->viewport_h;
    
    cam->x = clamp(cam->x, 0, max_x > 0 ? max_x : 0);
    cam->y = clamp(cam->y, 0, max_y > 0 ? max_y : 0);
}

// ----------------------------------------------------------------------------
// Main game update – movement, animation, and NPC interaction
// ----------------------------------------------------------------------------
void game_screen_update(GameScreen *gs, float delta_time) {
    if (!gs) return;
    
    // If in dialogue, only update the dialogue system (no movement)
    if (gs->in_dialogue) {
        dialogue_update(gs, delta_time, NULL);
        return;
    }
    
    // Determine if player is moving based on held keys
    gs->is_moving = gs->moving_left || gs->moving_right || 
                    gs->moving_up || gs->moving_down;
    
    // Update player walk animation
    if (gs->is_moving) {
        gs->frame_counter++;
        if (gs->frame_counter >= ANIMATION_SPEED) {
            gs->frame_counter = 0;
            gs->current_frame = (gs->current_frame + 1) % PC_FRAME_COUNT;
        }
    } else {
        // Reset to first frame when standing still
        gs->current_frame = 0;
        gs->frame_counter = 0;
    }
    
    // Apply movement
    if (gs->moving_right) gs->player_x += MOVE_SPEED;
    if (gs->moving_left)  gs->player_x -= MOVE_SPEED;
    if (gs->moving_up)    gs->player_y -= MOVE_SPEED;
    if (gs->moving_down)  gs->player_y += MOVE_SPEED;
    
    // Keep player inside map boundaries
    gs->player_x = clamp(gs->player_x, 0, gs->map_width);
    gs->player_y = clamp(gs->player_y, 0, gs->map_height);

    // Update NPC idle animation (independent of player)
    gs->npc_frame_timer += delta_time;
    if (gs->npc_frame_timer >= gs->npc_frame_duration) {
        gs->npc_frame_timer = 0.0f;
        gs->npc_frame = (gs->npc_frame + 1) % NPC_FRAME_COUNT_ANIM;
    }

    // Make NPC face the player (for world rendering)
    if (gs->player_x > gs->npc_x) {
        gs->npc_facing_right = true;
    } else {
        gs->npc_facing_right = false;
    }
    
    // Check if player is close enough to talk to the NPC
    float dx = gs->player_x - gs->npc_x;
    float dy = gs->player_y - gs->npc_y;
    float distance_sq = dx*dx + dy*dy;
    gs->player_near_npc = (distance_sq < 40 * 20);   // Magic threshold – feels right

    // Update camera after movement
    update_camera(gs);
}

// ----------------------------------------------------------------------------
// Dialogue system – script loading and management
// ----------------------------------------------------------------------------

// Begin a dialogue script from a file. Free any previous script.
void dialogue_start_script(GameScreen *gs, SDL_Renderer *renderer, const char *filename) {
    if (!gs || gs->in_dialogue) return;
    
    // Clean up old script to avoid memory leaks
    if (gs->current_script) {
        dialogue_destroy(gs->current_script);
        gs->current_script = NULL;
    }
    
    gs->current_script = dialogue_load(filename);
    if (!gs->current_script) {
        printf("Failed to load dialogue: %s\n", filename);
        return;
    }
    
    gs->in_dialogue = true;
    gs->current_line_index = 0;
    gs->dialogue_char_index = 0;          // No characters displayed yet
    gs->dialogue_timer = 0.0f;
    gs->dialogue_text_complete = false;
    gs->dialogue_skip_requested = false;
    gs->waiting_for_player_choice = false;
    gs->player_speaking = false;
    
    // Separate animation for NPC during dialogue (uses same sprites, different timing)
    gs->dialogue_npc_frame = 0;
    gs->dialogue_npc_timer = 0.0f;
    gs->dialogue_npc_frame_duration = 0.4f;   // Slightly faster than world idle
    
    // If the very first line is a player choice, enter waiting state immediately
    DialogueEntry *first = dialogue_get_line(gs->current_script, 0);
    if (first && first->is_player_choice) {
        gs->waiting_for_player_choice = true;
    }
    
    // Clear cached name texture so it will be regenerated for the new speaker
    if (gs->dialogue_name_texture) {
        SDL_DestroyTexture(gs->dialogue_name_texture);
        gs->dialogue_name_texture = NULL;
    }
    
    printf("Dialogue started: %s (%d lines)\n", filename, gs->current_script->line_count);
}

// Advance to the next line or end the script.
// Called when the player presses a key/click while text is complete.
void dialogue_advance_script(GameScreen *gs, SDL_Renderer *renderer) {
    if (!gs || !gs->in_dialogue || !gs->current_script) return;
    
    // If waiting for a choice, do nothing – choices are handled separately
    if (gs->waiting_for_player_choice) {
        return;
    }
    
    // If text is still being typed, just skip to the end on this press
    if (!gs->dialogue_text_complete) {
        gs->dialogue_skip_requested = true;
        printf("Skipping to end of line\n");
        return;
    }
    
    // Text is complete; get the current line and decide where to go
    DialogueEntry *current = dialogue_get_line(gs->current_script, gs->current_line_index);
    if (!current) {
        dialogue_end(gs);
        return;
    }
    
    // If this line triggers a battle, exit dialogue and set the flag
    if (current->triggers_battle) {
        gs->battle_triggered = true;
        dialogue_end(gs);
        return;
    }
    
    // Move to the next line (either specified by next_line or automatically +1)
    int next = current->next_line;
    
    if (next >= 0 && next < gs->current_script->line_count) {
        gs->current_line_index = next;
        gs->dialogue_char_index = 0;
        gs->dialogue_timer = 0.0f;
        gs->dialogue_text_complete = false;
        gs->dialogue_skip_requested = false;
        gs->player_speaking = false;   // Will be set based on the speaker field later
        
        // Check if the new line is a choice; if so, wait for player selection
        DialogueEntry *next_entry = dialogue_get_line(gs->current_script, next);
        if (next_entry && next_entry->is_player_choice) {
            gs->waiting_for_player_choice = true;
            printf("Waiting for player choice at line %d\n", next);
        }
        
        // Clear cached name texture so it will be re‑rendered for the new speaker
        if (gs->dialogue_name_texture) {
            SDL_DestroyTexture(gs->dialogue_name_texture);
            gs->dialogue_name_texture = NULL;
        }
        
        printf("Advanced to line %d\n", next);
    } else {
        // No valid next line – end dialogue
        printf("End of dialogue\n");
        dialogue_end(gs);
    }
}

// Player selected choice A (choice == 0) or B (choice == 1)
void dialogue_select_choice(GameScreen *gs, int choice) {
    if (!gs || !gs->waiting_for_player_choice || !gs->current_script) return;
    
    DialogueEntry *current = dialogue_get_line(gs->current_script, gs->current_line_index);
    if (!current || !current->is_player_choice) return;
    
    int next_line = (choice == 0) ? current->next_line_a : current->next_line_b;
    
    printf("Player selected choice %d -> jumping to line %d\n", choice, next_line);
    
    gs->waiting_for_player_choice = false;
    // Important: Do NOT reset player_speaking here. The next line's speaker field
    // determines who is talking.
    
    if (next_line >= 0 && next_line < gs->current_script->line_count) {
        gs->current_line_index = next_line;
        gs->dialogue_char_index = 0;
        gs->dialogue_timer = 0.0f;
        gs->dialogue_text_complete = false;
        gs->dialogue_skip_requested = false;
        
        // Check if the new line triggers a battle
        DialogueEntry *next_entry = dialogue_get_line(gs->current_script, next_line);
        if (next_entry && next_entry->triggers_battle) {
            gs->battle_triggered = true;
            dialogue_end(gs);
            return;
        }
        
        // If the next line is another choice, stay in waiting mode
        if (next_entry && next_entry->is_player_choice) {
            gs->waiting_for_player_choice = true;
        }
        
        // Clear cached name texture
        if (gs->dialogue_name_texture) {
            SDL_DestroyTexture(gs->dialogue_name_texture);
            gs->dialogue_name_texture = NULL;
        }
    } else {
        dialogue_end(gs);
    }
}

// End the current dialogue, clean up
void dialogue_end(GameScreen *gs) {
    if (!gs) return;
    gs->in_dialogue = false;
    
    if (gs->dialogue_name_texture) {
        SDL_DestroyTexture(gs->dialogue_name_texture);
        gs->dialogue_name_texture = NULL;
    }
    
    printf("Dialogue ended\n");
}

// Update the typewriter effect – called every frame while in dialogue
void dialogue_update(GameScreen *gs, float delta_time, SDL_Renderer *renderer) {
    if (!gs || !gs->in_dialogue || !gs->current_script) return;
    
    gs->dialogue_timer += delta_time;
    
    // Animate NPC portrait during dialogue (independent of world animation)
    gs->dialogue_npc_timer += delta_time;
    if (gs->dialogue_npc_timer >= gs->dialogue_npc_frame_duration) {
        gs->dialogue_npc_timer = 0.0f;
        gs->dialogue_npc_frame = (gs->dialogue_npc_frame + 1) % NPC_FRAME_COUNT_ANIM;
    }
    
    // Typewriter effect: add a new character when enough time has passed
    if (!gs->dialogue_text_complete) {
        DialogueEntry *entry = dialogue_get_line(gs->current_script, gs->current_line_index);
        if (!entry) return;
        
        const char *current_text = entry->text;
        int text_len = strlen(current_text);
        
        if (gs->dialogue_skip_requested) {
            // Skip to end immediately
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

// Handle mouse clicks during dialogue – for choice selection and advancing
bool dialogue_handle_mouse(GameScreen *gs, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer) {
    if (!gs || !gs->in_dialogue) return false;
    
    float mx = mouse->x;
    float my = mouse->y;
    
    // Update hover states for choice buttons
    gs->choice_a_hovered = (mx >= gs->choice_a_rect.x && 
                            mx <= gs->choice_a_rect.x + gs->choice_a_rect.w &&
                            my >= gs->choice_a_rect.y && 
                            my <= gs->choice_a_rect.y + gs->choice_a_rect.h);
                            
    gs->choice_b_hovered = (mx >= gs->choice_b_rect.x && 
                            mx <= gs->choice_b_rect.x + gs->choice_b_rect.w &&
                            my >= gs->choice_b_rect.y && 
                            my <= gs->choice_b_rect.y + gs->choice_b_rect.h);
    
    if (mouse->type == SDL_EVENT_MOUSE_BUTTON_DOWN && mouse->button == SDL_BUTTON_LEFT) {
        // If waiting for a choice, check if one of the buttons was clicked
        if (gs->waiting_for_player_choice) {
            if (gs->choice_a_hovered) {
                dialogue_select_choice(gs, 0);
                return true;
            } else if (gs->choice_b_hovered) {
                dialogue_select_choice(gs, 1);
                return true;
            }
        } else {
            // Otherwise, clicking anywhere advances the dialogue
            dialogue_advance_script(gs, renderer);
            return true;
        }
    }
    
    return false;
}

// Handle keyboard input during dialogue
bool dialogue_handle_key(GameScreen *gs, SDL_KeyboardEvent *key, SDL_Renderer *renderer) {
    if (!gs || !gs->in_dialogue) return false;
    
    if (key->type == SDL_EVENT_KEY_DOWN) {
        switch (key->scancode) {
            case SDL_SCANCODE_E:
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_SPACE:
                dialogue_advance_script(gs, renderer);
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

// Render all dialogue elements: dark background, portraits, text box, choices
void dialogue_render(SDL_Renderer *renderer, GameScreen *gs) {
    if (!gs || !gs->in_dialogue || !renderer || !gs->current_script) return;
    
    int window_w = gs->camera.viewport_w;
    int window_h = gs->camera.viewport_h;
    
    // Create dialogue box texture once (if not already created)
    if (!gs->dialogue_box_texture) {
        int box_w = window_w - (DIALOGUE_BOX_MARGIN * 2);
        gs->dialogue_box_texture = create_dialogue_box(renderer, box_w, DIALOGUE_BOX_HEIGHT);
    }
    
    // Semi‑transparent dark overlay to focus attention on the dialogue
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect darken_rect = {0, 0, window_w, window_h};
    SDL_RenderFillRect(renderer, &darken_rect);
    
    // Get the current dialogue entry
    DialogueEntry *entry = dialogue_get_line(gs->current_script, gs->current_line_index);
    if (!entry) return;
    
    // Determine who is speaking by checking the speaker field (exact string "You")
    bool is_player_speaking = (strcmp(entry->speaker, "You") == 0);
    
    // ------------------------------------------------------------------------
    // Player choice screen (buttons with breathing RGB effect)
    // ------------------------------------------------------------------------
    if (gs->waiting_for_player_choice) {
        // Breathing intensity: smoothly varies between 0.0 and 1.0 over 800ms
        float breathe = (sinf(SDL_GetTicks() / 800.0f) + 1.0f) / 2.0f;
        
        // Question text – displayed above the buttons
        int question_y = window_h / 4;
        SDL_Surface *q_surf = TTF_RenderText_Blended(gs->font_bold, entry->text, 0, 
                                                    (SDL_Color){255, 255, 255, 255});
        if (q_surf) {
            SDL_Texture *q_tex = SDL_CreateTextureFromSurface(renderer, q_surf);
            float q_w, q_h;
            SDL_GetTextureSize(q_tex, &q_w, &q_h);
            SDL_FRect q_rect = {(window_w - q_w) / 2, question_y, q_w, q_h};
            SDL_RenderTexture(renderer, q_tex, NULL, &q_rect);
            SDL_DestroyTexture(q_tex);
            SDL_DestroySurface(q_surf);
        }
        
        // Button dimensions – large and centered near the bottom
        int btn_w = 700;
        int btn_h = 90;
        int btn_x = (window_w - btn_w) / 2;
        int btn_spacing = 25;
        int btn_a_y = window_h - 280;   // Lower than default dialogue box
        int btn_b_y = btn_a_y + btn_h + btn_spacing;
        
        // --- Choice A: Reddish / aggressive theme ---------------------------------
        Uint8 a_r = (Uint8)(200 + 55 * breathe);
        Uint8 a_g = (Uint8)(50 + 100 * breathe);
        Uint8 a_b = (Uint8)(50 * (1.0f - breathe));
        
        SDL_Color a_bg_base = {a_r / 3, a_g / 3, a_b / 3, 220};
        SDL_Color a_bg_hover = {a_r / 2, a_g / 2, a_b / 2, 240};
        SDL_Color a_border = {a_r, a_g, a_b, 255};
        
        SDL_Color a_bg_color = gs->choice_a_hovered ? a_bg_hover : a_bg_base;
        
        gs->choice_a_rect = (SDL_FRect){btn_x, btn_a_y, btn_w, btn_h};
        SDL_SetRenderDrawColor(renderer, a_bg_color.r, a_bg_color.g, a_bg_color.b, a_bg_color.a);
        SDL_RenderFillRect(renderer, &gs->choice_a_rect);
        
        // Border (thicker when hovered)
        int border_thickness = gs->choice_a_hovered ? 4 : 2;
        SDL_SetRenderDrawColor(renderer, a_border.r, a_border.g, a_border.b, a_border.a);
        for (int i = 0; i < border_thickness; i++) {
            SDL_FRect border_rect = {
                btn_x - i, btn_a_y - i,
                btn_w + i * 2, btn_h + i * 2
            };
            SDL_RenderRect(renderer, &border_rect);
        }
        
        // Inner glow line
        SDL_SetRenderDrawColor(renderer, a_r, a_g, a_b, (Uint8)(100 + 100 * breathe));
        SDL_FRect inner_glow = {btn_x + 4, btn_a_y + 4, btn_w - 8, btn_h - 8};
        SDL_RenderRect(renderer, &inner_glow);
        
        // Choice A text
        SDL_Surface *a_surf = TTF_RenderText_Blended(gs->font_bold, entry->choice_a, 0,
                                                    (SDL_Color){255, 255, 255, 255});
        if (a_surf) {
            SDL_Texture *a_tex = SDL_CreateTextureFromSurface(renderer, a_surf);
            float a_w, a_h;
            SDL_GetTextureSize(a_tex, &a_w, &a_h);
            SDL_FRect a_text_rect = {
                btn_x + (btn_w - a_w) / 2,
                btn_a_y + (btn_h - a_h) / 2,
                a_w, a_h
            };
            SDL_RenderTexture(renderer, a_tex, NULL, &a_text_rect);
            SDL_DestroyTexture(a_tex);
            SDL_DestroySurface(a_surf);
        }
        
        // --- Choice B: Bluish / calm theme ---------------------------------------
        Uint8 b_r = (Uint8)(50 * (1.0f - breathe));
        Uint8 b_g = (Uint8)(100 + 100 * breathe);
        Uint8 b_b = (Uint8)(200 + 55 * breathe);
        
        SDL_Color b_bg_base = {b_r / 3, b_g / 3, b_b / 3, 220};
        SDL_Color b_bg_hover = {b_r / 2, b_g / 2, b_b / 2, 240};
        SDL_Color b_border = {b_r, b_g, b_b, 255};
        
        SDL_Color b_bg_color = gs->choice_b_hovered ? b_bg_hover : b_bg_base;
        
        gs->choice_b_rect = (SDL_FRect){btn_x, btn_b_y, btn_w, btn_h};
        SDL_SetRenderDrawColor(renderer, b_bg_color.r, b_bg_color.g, b_bg_color.b, b_bg_color.a);
        SDL_RenderFillRect(renderer, &gs->choice_b_rect);
        
        border_thickness = gs->choice_b_hovered ? 4 : 2;
        SDL_SetRenderDrawColor(renderer, b_border.r, b_border.g, b_border.b, b_border.a);
        for (int i = 0; i < border_thickness; i++) {
            SDL_FRect border_rect = {
                btn_x - i, btn_b_y - i,
                btn_w + i * 2, btn_h + i * 2
            };
            SDL_RenderRect(renderer, &border_rect);
        }
        
        SDL_SetRenderDrawColor(renderer, b_r, b_g, b_b, (Uint8)(100 + 100 * breathe));
        SDL_FRect inner_glow_b = {btn_x + 4, btn_b_y + 4, btn_w - 8, btn_h - 8};
        SDL_RenderRect(renderer, &inner_glow_b);
        
        // Choice B text
        SDL_Surface *b_surf = TTF_RenderText_Blended(gs->font_bold, entry->choice_b, 0,
                                                    (SDL_Color){255, 255, 255, 255});
        if (b_surf) {
            SDL_Texture *b_tex = SDL_CreateTextureFromSurface(renderer, b_surf);
            float b_w, b_h;
            SDL_GetTextureSize(b_tex, &b_w, &b_h);
            SDL_FRect b_text_rect = {
                btn_x + (btn_w - b_w) / 2,
                btn_b_y + (btn_h - b_h) / 2,
                b_w, b_h
            };
            SDL_RenderTexture(renderer, b_tex, NULL, &b_text_rect);
            SDL_DestroyTexture(b_tex);
            SDL_DestroySurface(b_surf);
        }
        
        // Hint text below the buttons (also breathing)
        SDL_Color hint_color = {
            (Uint8)(200 + 55 * breathe),
            (Uint8)(200 + 55 * breathe),
            (Uint8)(200 + 55 * breathe),
            220
        };
        const char *hint = "Click your choice";
        SDL_Surface *hint_surf = TTF_RenderText_Blended(gs->font_dialogue, hint, 0, hint_color);
        if (hint_surf) {
            SDL_Texture *hint_tex = SDL_CreateTextureFromSurface(renderer, hint_surf);
            float hint_w, hint_h;
            SDL_GetTextureSize(hint_tex, &hint_w, &hint_h);
            SDL_FRect hint_rect = {
                (window_w - hint_w) / 2,
                btn_b_y + btn_h + 30,
                hint_w, hint_h
            };
            SDL_RenderTexture(renderer, hint_tex, NULL, &hint_rect);
            SDL_DestroyTexture(hint_tex);
            SDL_DestroySurface(hint_surf);
        }
        
        return;   // No normal dialogue rendering when choices are shown
    }
    
    // ------------------------------------------------------------------------
    // Normal dialogue rendering (text box + portraits)
    // ------------------------------------------------------------------------
    
    // NPC portrait (left side) – only if speaker is NOT the player
    if (!is_player_speaking) {
        int portrait_w = PORTRAIT_WIDTH;
        int portrait_h = PORTRAIT_HEIGHT;
        int portrait_x = PORTRAIT_OFFSET_X;
        int portrait_y = window_h - portrait_h - PORTRAIT_OFFSET_Y;
        
        if (gs->npc_idle_texture) {
            // Source rectangle: NPC sprite sheet has 2 frames per row.
            // We use the first row for facing left? Actually we always show the
            // talking NPC, but we animate the frames.
            SDL_FRect src = {
                (float)(gs->dialogue_npc_frame * NPC_FRAME_WIDTH),
                (float)(2 * (NPC_FRAME_HEIGHT + 4)),   // Row index 2? This was a specific offset from original.
                NPC_FRAME_WIDTH, 
                NPC_FRAME_HEIGHT
            };
            SDL_FRect dst = {portrait_x, portrait_y, portrait_w, portrait_h};
            SDL_RenderTexture(renderer, gs->npc_idle_texture, &src, &dst);
        }
    }
    
    // Player portrait (right side) – only when the player is speaking
    if (is_player_speaking) {
        int portrait_w = PLAYER_PORTRAIT_WIDTH;
        int portrait_h = PLAYER_PORTRAIT_HEIGHT;
        int portrait_x = window_w - portrait_w - PLAYER_PORTRAIT_OFFSET_X;
        int portrait_y = window_h - portrait_h - PLAYER_PORTRAIT_OFFSET_Y;
        
        if (gs->idle_texture) {
            // Player portrait uses the idle sprite, frame 0, with a fixed offset
            SDL_FRect src = {
                2.0f,
                207,                         // Y position of the first frame in the texture
                PC_FRAME_WIDTH,
                PC_FRAME_HEIGHT
            };
            SDL_FRect dst = {portrait_x, portrait_y, portrait_w, portrait_h};
            SDL_RenderTextureRotated(renderer, gs->idle_texture, &src, &dst, 0.0, NULL, SDL_FLIP_HORIZONTAL);
        }
    }
    
    // Draw the main dialogue box
    int box_h = DIALOGUE_BOX_HEIGHT;
    int box_w = window_w - (DIALOGUE_BOX_MARGIN * 2);
    int box_x = DIALOGUE_BOX_MARGIN;
    int box_y = window_h - box_h - DIALOGUE_BOX_MARGIN;
    
    gs->dialogue_box_rect = (SDL_FRect){box_x, box_y, box_w, box_h};
    
    if (gs->dialogue_box_texture) {
        SDL_FRect box_rect = {box_x, box_y, box_w, box_h};
        SDL_RenderTexture(renderer, gs->dialogue_box_texture, NULL, &box_rect);
    }
    
    // Text area – margins inside the box
    int text_x = box_x + 40;
    int text_y = box_y + 25;
    int max_text_w = box_w - 80;
    
    // Speaker name – displayed above the text
    const char *speaker = entry->speaker;
    SDL_Color name_color = is_player_speaking ? 
        (SDL_Color){100, 200, 255, 255} :   // Player name in light blue
        (SDL_Color){255, 215, 100, 255};    // NPC name in gold
    
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
    
    // Dialogue text with typewriter effect
    const char *full_text = entry->text;
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
    
    // Continue indicator (blinking triangle) when text is fully shown
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
    
    // Hint text at the very bottom
    SDL_Color hint_color = {180, 180, 180, 180};
    const char *hint_text = gs->dialogue_text_complete ? 
        "Click, E, Enter, or Space to continue" : 
        "Click, E, Enter, or Space to skip";
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

// ----------------------------------------------------------------------------
// Main rendering – draws the world, player, NPC, and optionally dialogue
// ----------------------------------------------------------------------------
void game_screen_render(SDL_Renderer *renderer, GameScreen *gs) {
    if (!gs || !renderer) return;

    Camera *cam = &gs->camera;
    
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Determine which part of the world map to draw based on camera position
    SDL_FRect src_rect = {
        .x = cam->x / cam->zoom,
        .y = cam->y / cam->zoom,
        .w = cam->viewport_w / cam->zoom,
        .h = cam->viewport_h / cam->zoom
    };
    
    // Clip source rectangle to map boundaries
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

    // Convert world coordinates to screen coordinates
    float screen_x = (gs->player_x * cam->zoom) - cam->x;
    float screen_y = (gs->player_y * cam->zoom) - cam->y;

    float npc_screen_x = (gs->npc_x * cam->zoom) - cam->x;
    float npc_screen_y = (gs->npc_y * cam->zoom) - cam->y;

    float sprite_size = 16 * cam->zoom;
    float npc_sprite_size = 35 * cam->zoom;

    // Draw NPC (only when not in dialogue, to avoid overlapping with portrait)
    if (gs->npc_idle_texture && !gs->in_dialogue) {
        // Choose row based on facing direction (row 3 for right, row 1 for left)
        int row = gs->npc_facing_right ? 3 : 1;
        
        SDL_FRect src_sprite = {
            .x = (float)(gs->npc_frame * NPC_FRAME_WIDTH),
            .y = (float)(row * (NPC_FRAME_HEIGHT + 4)),   // +4 for spacing between rows
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

    // Draw player sprite
    SDL_Texture *current_texture = gs->is_moving ? gs->run_texture : gs->idle_texture;
    
    if (current_texture && !gs->in_dialogue) {
        SDL_FRect src_sprite = {
            .x = 2.0f + (gs->current_frame * PC_FRAME_WIDTH),
            .y = 207,                         // Row where player frames are stored
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

    // Interaction prompt above NPC (only when not in dialogue)
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
            // Default floating text when not near NPC (for atmosphere)
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

    // Draw title text (only when not in dialogue)
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

    // Finally, draw the dialogue UI if active (overwrites everything)
    if (gs->in_dialogue) {
        dialogue_render(renderer, gs);
    }
}

// ----------------------------------------------------------------------------
// Handle keyboard input – movement, talk, and dialogue keys
// ----------------------------------------------------------------------------
void game_screen_handle_input(GameScreen *gs, SDL_KeyboardEvent *key) {
    if (!gs) return;

    bool is_pressed = (key->type == SDL_EVENT_KEY_DOWN);

    // If in dialogue, route input to dialogue handler
    if (gs->in_dialogue) {
        dialogue_handle_key(gs, key, NULL);
        return;
    }

    // Movement keys – update flags for continuous movement
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
                // Start a specific dialogue script
                dialogue_start_script(gs, NULL, "game_assets/dialogues/naberius_encounter.txt");
            }
            break;
            
        default:
            break;
    }
}

// ----------------------------------------------------------------------------
// Handle mouse input – route to dialogue if needed
// ----------------------------------------------------------------------------
void game_screen_handle_mouse(GameScreen *gs, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer) {
    if (!gs) return;
    
    if (gs->in_dialogue) {
        dialogue_handle_mouse(gs, mouse, renderer);
        return;
    }
}

// ----------------------------------------------------------------------------
// Clean up all resources used by the game screen
// ----------------------------------------------------------------------------
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

        if (gs->current_script) {
            dialogue_destroy(gs->current_script);
        }
        free(gs);
        printf("Game screen destroyed\n");
    }
}