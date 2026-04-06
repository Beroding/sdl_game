#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

// ============================================================================
// Menu – Main menu state (title screen)
// ============================================================================
//
// This structure holds all resources and state for the game's main menu.
// It allows the player to choose between Continue, New Game, Credit, and Quit.
//
// Assumptions:
// - The window size is fixed (1500×??) – but we adapt text positions relative to window size.
// - The background image "menu_screen_bg.jpg" exists in game_assets.
// - Fonts "Roboto_Medium.ttf" and "MedievalSharp-Regular.ttf" are present.
// - There are exactly 4 menu options (hardcoded in the implementation).
// ============================================================================

typedef struct {
    // Background texture (the full-screen image)
    SDL_Texture *texture;

    // Destination rectangle for the background (usually the whole screen)
    SDL_FRect rect;

    // Font used for menu item text (normal size)
    TTF_Font *font;

    // Textures for each menu option label (rendered text)
    SDL_Texture *item_text[4];

    // Screen rectangles for each menu option (used for positioning and click detection)
    SDL_FRect option_rects[4];

    // Currently selected menu option index (0 = Continue, 1 = New Game, 2 = Credit, 3 = Quit)
    int selected;

    // Number of menu options (always 4 in this implementation)
    int option_count;

    // Texture for the game title ("AfterLife") – rendered with a fancy font
    SDL_Texture *game_title_text;

    // Fancy font used for the title
    TTF_Font *game_title_font;

} Menu;

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

/**
 * Create and initialise the main menu.
 *
 * @param renderer        SDL renderer for creating textures.
 * @param window_width    Width of the game window (used to centre items).
 * @param window_height   Height of the game window.
 * @return                Pointer to a new Menu structure, or NULL on failure.
 *
 * @note The menu assumes it can load background image and fonts from game_assets.
 *       The caller must later call menu_destroy() to free resources.
 */
Menu *menu_create(SDL_Renderer *renderer, int window_width, int window_height);

/**
 * Draw the menu on screen.
 *
 * @param renderer   SDL renderer to draw with.
 * @param menu       The menu to draw.
 *
 * @details This renders the background, then each option with a yellow highlight
 *          around the currently selected item.
 */
void menu_render(SDL_Renderer *renderer, Menu *menu);

/**
 * Handle mouse click on the menu.
 *
 * @param menu      The menu structure.
 * @param mouse_x   X-coordinate of the click (in screen coordinates).
 * @param mouse_y   Y-coordinate of the click.
 * @return          Index of the clicked option (0‑3) if a button was clicked,
 *                  or -1 if the click was outside any menu option.
 *
 * @note This function also updates menu->selected to the clicked option.
 */
int menu_handle_click(Menu *menu, float mouse_x, float mouse_y);

/**
 * Handle keyboard input for menu navigation.
 *
 * @param menu   The menu structure.
 * @param key    SDL keyboard event (must be an SDL_EVENT_KEY_DOWN).
 * @return       Index of the selected option (0‑3) if the Enter key was pressed,
 *               or -1 for any other key (including navigation keys).
 *
 * @details Up/Down arrows change the selected option (wrapping around).
 *          Enter key confirms the currently selected option.
 */
int menu_handle_key(Menu *menu, SDL_KeyboardEvent *key);

/**
 * Free all resources allocated by the menu.
 *
 * @param menu   The menu to destroy (can be NULL).
 */
void menu_destroy(Menu *menu);

#endif // MENU_SCREEN_H