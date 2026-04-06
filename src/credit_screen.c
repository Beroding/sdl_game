// ============================================================================
// credit_screen.c - Scrolling/animated credits display
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/credit_screen.h"

// Timing constants (in seconds)
#define FADE_DURATION 0.5f
#define DISPLAY_DURATION 4.0f  // Time to read each scene

// ----------------------------------------------------------------------------
// Helper: Read entire file into allocated string
// ----------------------------------------------------------------------------
static char* read_file_to_string(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open credit scene: %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(file);
        return strdup("");
    }
    
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    
    return buffer;
}

// ----------------------------------------------------------------------------
// Helper: Split text into lines and create textures
// ----------------------------------------------------------------------------
static void prepare_scene_textures(CreditScreen *credits, SDL_Renderer *renderer, const char *text) {
    // Clean up old textures
    if (credits->line_textures) {
        for (int i = 0; i < credits->line_count; i++) {
            if (credits->line_textures[i]) {
                SDL_DestroyTexture(credits->line_textures[i]);
            }
        }
        free(credits->line_textures);
        free(credits->line_y_positions);
    }
    credits->line_textures = NULL;
    credits->line_y_positions = NULL;
    credits->line_count = 0;
    
    if (!text || strlen(text) == 0) {
        return;
    }
    
    // Count lines
    int max_lines = 1;
    const char *p = text;
    while (*p) {
        if (*p == '\n') max_lines++;
        p++;
    }
    max_lines += 2;
    
    // Allocate arrays
    credits->line_textures = calloc(max_lines, sizeof(SDL_Texture*));
    credits->line_y_positions = calloc(max_lines, sizeof(float));
    if (!credits->line_textures || !credits->line_y_positions) {
        return;
    }
    
    // Create texture for each line
    char line_buffer[512];
    int line_idx = 0;
    const char *start = text;
    float current_y = 200;
    
    while (*start && line_idx < max_lines - 1) {
        // Find end of line (handle both \n and \r\n)
        const char *end = start;
        while (*end && *end != '\n' && *end != '\r') end++;
        
        // Calculate length with bounds checking
        int len = (int)(end - start);
        if (len < 0) len = 0;
        if (len > 500) len = 500;  // Leave room for null terminator
        
        // SAFELY copy: clear buffer first, then copy, then null terminate
        memset(line_buffer, 0, sizeof(line_buffer));
        if (len > 0) {
            memcpy(line_buffer, start, len);
        }
        line_buffer[len] = '\0';
        
        // Debug: show raw bytes before cleaning
        if (len > 0) {
            printf("Raw line [%d]: '", len);
            for (int i = 0; i < len && i < 20; i++) {
                unsigned char c = (unsigned char)line_buffer[i];
                if (c >= 32 && c < 127) printf("%c", c);
                else printf("\\x%02X", c);
            }
            printf("'\n");
        }
        
        // Trim trailing whitespace and weird characters
        while (len > 0) {
            unsigned char c = (unsigned char)line_buffer[len - 1];
            // Remove: spaces, tabs, carriage returns, newlines, control chars, DEL
            if (c <= 32 || c == 127) {
                line_buffer[--len] = '\0';
            } else {
                break;
            }
        }
        
        // Skip empty lines
        if (len == 0) {
            current_y += 40;
            // Advance past any newline chars
            while (*end == '\n' || *end == '\r') end++;
            start = end;
            continue;
        }
        
        printf("Clean line: '%s'\n", line_buffer);
        
        // Determine font and color
        TTF_Font *font = credits->font_body;
        SDL_Color color = {220, 220, 220, 255};
        bool is_header = false;
        bool is_title = false;
        
        // Check for header markers
        if (strstr(line_buffer, "----") || strstr(line_buffer, "===")) {
            is_header = true;
        }
        // Check for ALL CAPS
        else if (len >= 3) {
            bool all_caps = true;
            bool has_letters = false;
            for (int i = 0; i < len; i++) {
                if (isalpha((unsigned char)line_buffer[i])) {
                    has_letters = true;
                    if (islower((unsigned char)line_buffer[i])) {
                        all_caps = false;
                        break;
                    }
                }
            }
            if (has_letters && all_caps) {
                is_header = true;
            }
        }
        
        // Check for title
        if (strstr(line_buffer, "Game Created") || 
            strstr(line_buffer, "AfterLife") ||
            strstr(line_buffer, "Thank You") ||
            strstr(line_buffer, "Ghostrunner")) {
            is_title = true;
            is_header = false;
        }
        
        // Apply styling
        if (is_title) {
            font = credits->font_title;
            color = (SDL_Color){255, 255, 255, 255};
            current_y += 30;
        } else if (is_header) {
            font = credits->font_header;
            color = (SDL_Color){255, 200, 100, 255};
            current_y += 50;
        }
        
        // Create texture using UTF-8 version for safety
        SDL_Surface *surf = TTF_RenderText_Blended(font, line_buffer, 0, color);
        if (surf) {
            credits->line_textures[line_idx] = SDL_CreateTextureFromSurface(renderer, surf);
            float w, h;
            SDL_GetTextureSize(credits->line_textures[line_idx], &w, &h);
            
            credits->line_y_positions[line_idx] = current_y;
            current_y += h + 25;
            line_idx++;
            SDL_DestroySurface(surf);
        }
        
        // Advance past any newline chars
        while (*end == '\n' || *end == '\r') end++;
        start = end;
    }
    
    credits->line_count = line_idx;
    
    // Center vertically
    if (line_idx > 0) {
        float first_y = credits->line_y_positions[0];
        float last_y = credits->line_y_positions[line_idx - 1];
        float content_center = (first_y + last_y) / 2.0f;
        float screen_center = credits->window_h / 2.0f;
        float offset = screen_center - content_center;
        
        for (int i = 0; i < line_idx; i++) {
            credits->line_y_positions[i] += offset;
        }
    }
    
    printf("  -> Created %d lines\n", line_idx);
}

// ----------------------------------------------------------------------------
// Create credit screen
// ----------------------------------------------------------------------------
CreditScreen* credit_screen_create(SDL_Renderer *renderer, int window_w, int window_h) {
    CreditScreen *credits = calloc(1, sizeof(CreditScreen));
    if (!credits) return NULL;
    
    credits->window_w = window_w;
    credits->window_h = window_h;
    credits->current_scene = 0;
    credits->phase = CREDIT_PHASE_FADE_IN;
    credits->phase_timer = 0;
    credits->alpha = 0;
    credits->complete = false;
    credits->skip_requested = false;
    credits->line_count = 0;
    credits->line_textures = NULL;
    
    // Load fonts
    credits->font_title = TTF_OpenFont("game_assets/MedievalSharp-Regular.ttf", 72);
    if (!credits->font_title) {
        credits->font_title = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 72);
    }
    
    credits->font_header = TTF_OpenFont("game_assets/MedievalSharp-Regular.ttf", 48);
    if (!credits->font_header) {
        credits->font_header = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 48);
    }
    
    credits->font_body = TTF_OpenFont("game_assets/NotoSansSC-Medium.ttf", 32);
    
    // Create dark background
    SDL_Surface *bg = SDL_CreateSurface(window_w, window_h, SDL_PIXELFORMAT_RGBA8888);
    if (bg) {
        Uint32 *pixels = (Uint32*)bg->pixels;
        for (int y = 0; y < window_h; y++) {
            float t = y / (float)window_h;
            Uint8 r = (Uint8)(5 + 15 * t);
            Uint8 g = (Uint8)(5 + 10 * t);
            Uint8 b = (Uint8)(15 + 10 * t);
            Uint32 color = SDL_MapRGBA(SDL_GetPixelFormatDetails(bg->format), NULL, r, g, b, 255);
            for (int x = 0; x < window_w; x++) {
                pixels[y * window_w + x] = color;
            }
        }
        credits->bg_texture = SDL_CreateTextureFromSurface(renderer, bg);
        SDL_DestroySurface(bg);
    }
    
    printf("Credit screen created\n");
    return credits;
}

// ----------------------------------------------------------------------------
// Load scene files
// ----------------------------------------------------------------------------
bool credit_screen_load_scenes(CreditScreen *credits, const char *scene_files[], int count) {
    if (!credits || count > MAX_CREDIT_SCENES) return false;
    
    credits->scene_count = 0;
    for (int i = 0; i < count; i++) {
        printf("Loading scene file: %s\n", scene_files[i]);
        credits->scene_texts[i] = read_file_to_string(scene_files[i]);
        if (credits->scene_texts[i]) {
            printf("  -> Loaded successfully (%zu bytes)\n", strlen(credits->scene_texts[i]));
            credits->scene_count++;
        } else {
            printf("  -> FAILED to load!\n");
        }
    }
    
    printf("Total scenes loaded: %d\n", credits->scene_count);
    return credits->scene_count > 0;
}

// ----------------------------------------------------------------------------
// Destroy credit screen
// ----------------------------------------------------------------------------
void credit_screen_destroy(CreditScreen *credits) {
    if (!credits) return;
    
    for (int i = 0; i < credits->scene_count; i++) {
        free(credits->scene_texts[i]);
    }
    
    if (credits->line_textures) {
        for (int i = 0; i < credits->line_count; i++) {
            if (credits->line_textures[i]) {
                SDL_DestroyTexture(credits->line_textures[i]);
            }
        }
        free(credits->line_textures);
        free(credits->line_y_positions);
    }
    
    if (credits->bg_texture) SDL_DestroyTexture(credits->bg_texture);
    if (credits->font_title) TTF_CloseFont(credits->font_title);
    if (credits->font_header) TTF_CloseFont(credits->font_header);
    if (credits->font_body) TTF_CloseFont(credits->font_body);
    
    free(credits);
    printf("Credit screen destroyed\n");
}

// ----------------------------------------------------------------------------
// Update animation - FIXED TIMING
// ----------------------------------------------------------------------------
void credit_screen_update(CreditScreen *credits, float delta_time) {
    if (!credits || credits->complete) return;
    
    credits->phase_timer += delta_time;
    
    switch (credits->phase) {
        case CREDIT_PHASE_FADE_IN:
            // Fade from 0 to 1 over FADE_DURATION seconds
            credits->alpha = credits->phase_timer / FADE_DURATION;
            if (credits->alpha >= 1.0f) {
                credits->alpha = 1.0f;
                credits->phase = CREDIT_PHASE_DISPLAY;
                credits->phase_timer = 0;
                printf("Scene %d: Now displaying\n", credits->current_scene);
            }
            break;
            
        case CREDIT_PHASE_DISPLAY:
            // Stay visible for DISPLAY_DURATION seconds
            if (credits->skip_requested) {
                printf("Scene %d: Skipped by user\n", credits->current_scene);
                credits->phase = CREDIT_PHASE_FADE_OUT;
                credits->phase_timer = 0;
                credits->skip_requested = false;
            } else if (credits->phase_timer >= DISPLAY_DURATION) {
                printf("Scene %d: Display time complete\n", credits->current_scene);
                credits->phase = CREDIT_PHASE_FADE_OUT;
                credits->phase_timer = 0;
            }
            break;
            
        case CREDIT_PHASE_FADE_OUT:
            // Fade from 1 to 0 over FADE_DURATION seconds
            credits->alpha = 1.0f - (credits->phase_timer / FADE_DURATION);
            if (credits->alpha <= 0.0f) {
                credits->alpha = 0.0f;
                credits->phase = CREDIT_PHASE_NEXT;
                credits->phase_timer = 0;
            }
            break;
            
        case CREDIT_PHASE_NEXT:
            // Move to next scene
            credits->current_scene++;
            printf("Advancing to scene %d\n", credits->current_scene);
            
            if (credits->current_scene >= credits->scene_count) {
                printf("All scenes complete\n");
                credits->phase = CREDIT_PHASE_COMPLETE;
                credits->complete = true;
            } else {
                // Reset for next scene
                credits->phase = CREDIT_PHASE_FADE_IN;
                // Clear old textures - new ones will be created in render
                if (credits->line_textures) {
                    for (int i = 0; i < credits->line_count; i++) {
                        if (credits->line_textures[i]) {
                            SDL_DestroyTexture(credits->line_textures[i]);
                        }
                    }
                    free(credits->line_textures);
                    free(credits->line_y_positions);
                    credits->line_textures = NULL;
                    credits->line_y_positions = NULL;
                    credits->line_count = 0;
                }
            }
            break;
            
        case CREDIT_PHASE_COMPLETE:
            credits->complete = true;
            break;
    }
}

// ----------------------------------------------------------------------------
// Render credits
// ----------------------------------------------------------------------------
void credit_screen_render(SDL_Renderer *renderer, CreditScreen *credits) {
    if (!credits || !renderer) return;
    
    // Draw background
    if (credits->bg_texture) {
        SDL_SetTextureAlphaMod(credits->bg_texture, 255);  // Background always full opacity
        SDL_FRect full = {0, 0, credits->window_w, credits->window_h};
        SDL_RenderTexture(renderer, credits->bg_texture, NULL, &full);
    } else {
        SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
        SDL_RenderClear(renderer);
    }
    
    // Prepare textures if needed
    if (credits->current_scene < credits->scene_count && 
        credits->scene_texts[credits->current_scene] &&
        credits->line_count == 0) {
        printf("Preparing scene %d textures...\n", credits->current_scene);
        prepare_scene_textures(credits, renderer, credits->scene_texts[credits->current_scene]);
    }
    
    // Draw text with current alpha
    if (credits->line_textures && credits->alpha > 0) {
        for (int i = 0; i < credits->line_count; i++) {
            if (!credits->line_textures[i]) continue;
            
            float w, h;
            SDL_GetTextureSize(credits->line_textures[i], &w, &h);
            
            SDL_SetTextureAlphaMod(credits->line_textures[i], (Uint8)(255 * credits->alpha));
            
            SDL_FRect rect = {
                (credits->window_w - w) / 2.0f,
                credits->line_y_positions[i],
                w, h
            };
            
            SDL_RenderTexture(renderer, credits->line_textures[i], NULL, &rect);
        }
    }
    
    // Draw hint at bottom
    if (credits->font_body && credits->alpha > 0.5f) {
        SDL_Color hint_color = {150, 150, 150, (Uint8)(150 * credits->alpha)};
        SDL_Surface *hint_surf = TTF_RenderText_Blended(credits->font_body, 
            "Press SPACE to skip scene, ESC to exit", 0, hint_color);
        if (hint_surf) {
            SDL_Texture *hint_tex = SDL_CreateTextureFromSurface(renderer, hint_surf);
            float hw, hh;
            SDL_GetTextureSize(hint_tex, &hw, &hh);
            SDL_SetTextureAlphaMod(hint_tex, (Uint8)(150 * credits->alpha));
            SDL_FRect hint_rect = {
                (credits->window_w - hw) / 2.0f,
                credits->window_h - 80,
                hw, hh
            };
            SDL_RenderTexture(renderer, hint_tex, NULL, &hint_rect);
            SDL_DestroyTexture(hint_tex);
            SDL_DestroySurface(hint_surf);
        }
    }
}

// ----------------------------------------------------------------------------
// Input handling
// ----------------------------------------------------------------------------
void credit_screen_handle_key(CreditScreen *credits, SDL_KeyboardEvent *key) {
    if (!credits || credits->complete) return;
    
    // Only process key down events
    if (key->type != SDL_EVENT_KEY_DOWN) return;
    
    switch (key->scancode) {
        case SDL_SCANCODE_SPACE:
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_RIGHT:
            // Skip to next scene (only if we're displaying or faded in)
            printf("SPACE pressed: requesting scene skip\n");
            if (credits->phase == CREDIT_PHASE_DISPLAY || 
                credits->phase == CREDIT_PHASE_FADE_IN) {
                credits->skip_requested = true;
            }
            break;
            
        case SDL_SCANCODE_ESCAPE:
            // Exit all credits immediately
            printf("ESC pressed: exiting credits\n");
            credits->complete = true;
            break;
            
        default:
            break;
    }
}

void credit_screen_skip(CreditScreen *credits) {
    if (!credits) return;
    credits->skip_requested = true;
}

bool credit_screen_is_complete(CreditScreen *credits) {
    return credits && credits->complete;
}