#ifndef DIALOGUE_LOADER_H
#define DIALOGUE_LOADER_H

#include <SDL3/SDL.h>           // For potential rendering, though not directly used here
#include <SDL3_ttf/SDL_ttf.h>   // For text rendering, again not directly used
#include <stdbool.h>            // For boolean flags

// ============================================================================
// CONSTANTS – Define limits of the dialogue system
// ============================================================================

// Maximum number of dialogue lines we can store in one script.
// This is a fixed size to keep memory management simple.
#define MAX_DIALOGUE_LINES 100

// Maximum length of any single text string (question, answer, etc.)
// Large enough to hold typical game dialogue without being wasteful.
#define MAX_TEXT_LEN 1024

// ============================================================================
// DIALOGUE ENTRY – One “line” of dialogue or a special command
// ============================================================================

typedef struct {
    // The character speaking, e.g. "Naberius", "SYSTEM", or left empty for narration
    char speaker[64];

    // The main text displayed to the player (question, statement, etc.)
    char text[MAX_TEXT_LEN];

    // If true, the player is given two choices instead of displaying text.
    // This overrides the normal display flow.
    bool is_player_choice;

    // Text of the first choice (only meaningful when is_player_choice is true)
    char choice_a[128];

    // Text of the second choice
    char choice_b[128];

    // For a normal line: where to go next after displaying it.
    // -1 means "advance to the next line in the file".
    // Can also be an explicit line index (0‑based) for branching.
    int next_line;

    // When a choice is presented, these indicate which dialogue line to jump to
    // depending on the player's selection.
    int next_line_a;        // Line index for choice A
    int next_line_b;        // Line index for choice B

    // If true, this entry triggers a battle scene instead of continuing dialogue.
    // The game engine should handle the battle and later resume dialogue.
    bool triggers_battle;

} DialogueEntry;

// ============================================================================
// DIALOGUE SCRIPT – Container for a whole dialogue file
// ============================================================================

typedef struct {
    // Path to the .txt file from which this script was loaded.
    // Stored mainly for debugging or reloading purposes.
    char file_path[256];

    // Array of all dialogue entries parsed from the file.
    // The order follows the file, except that empty/comment lines are skipped.
    DialogueEntry lines[MAX_DIALOGUE_LINES];

    // Number of valid entries in the lines array (always <= MAX_DIALOGUE_LINES).
    int line_count;

} DialogueScript;

// ============================================================================
// PUBLIC FUNCTIONS – How to use this module
// ============================================================================

/**
 * Load a dialogue file from disk and parse it into a DialogueScript.
 *
 * @param filename Path to the text file (e.g. "data/dialogue.txt").
 * @return Pointer to newly allocated DialogueScript, or NULL if file cannot be opened
 *         or memory allocation fails. The caller must free it with dialogue_destroy().
 *
 * @assumption The file uses UTF‑8 or plain ASCII (no special encoding).
 * @assumption Line endings are Unix (\n) or Windows (\r\n) – both handled by fgets.
 */
DialogueScript* dialogue_load(const char *filename);

/**
 * Free all memory associated with a dialogue script.
 *
 * @param script The script to destroy. After this call, the pointer becomes invalid.
 */
void dialogue_destroy(DialogueScript *script);

/**
 * Retrieve a specific dialogue entry by its index (0‑based).
 *
 * @param script The loaded script.
 * @param index The line number to fetch (must be < script->line_count).
 * @return Pointer to the entry, or NULL if index is out of range.
 *
 * @note The returned pointer is owned by the script; do not free it separately.
 */
DialogueEntry* dialogue_get_line(DialogueScript *script, int index);

#endif // DIALOGUE_LOADER_H