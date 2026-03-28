#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dialogue_loader.h"

// ============================================================================
// HELPER - Remove spaces from start/end of text
// ============================================================================

static char* trim_whitespace(char *str) {
    char *end;
    // Skip leading spaces
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;  // All spaces
    
    // Skip trailing spaces
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// ============================================================================
// PARSE LINE - Convert one line of text file into dialogue data
// ============================================================================

static void parse_line(char *line, DialogueEntry *entry) {
    // Clear the entry first (set all to zero)
    memset(entry, 0, sizeof(DialogueEntry));
    entry->next_line = -1;  // -1 means "use next line automatically"
    
    line = trim_whitespace(line);
    
    // Skip empty lines and comments
    if (strlen(line) == 0 || line[0] == '#' || line[0] == '/') return;
    
    // ============================================================================
    // SPECIAL COMMAND: [BATTLE] - Triggers combat
    // ============================================================================
    
    if (strncmp(line, "[BATTLE]", 8) == 0) {
        entry->triggers_battle = true;
        strcpy(entry->speaker, "SYSTEM");
        strcpy(entry->text, "BATTLE_START");
        return;
    }
    
    // ============================================================================
    // SPECIAL COMMAND: [CHOICE] - Player makes a decision
    // Format: [CHOICE] Speaker | Question | Option A | Option B | Next A | Next B
    // ============================================================================
    
    if (strncmp(line, "[CHOICE]", 8) == 0) {
        entry->is_player_choice = true;
        char *rest = line + 8;  // Skip "[CHOICE]"
        char *token = strtok(rest, "|");  // Split by | character
        
        if (token) {
            strcpy(entry->speaker, trim_whitespace(token));
            token = strtok(NULL, "|");
        }
        if (token) {
            strcpy(entry->text, trim_whitespace(token));  // The question
            token = strtok(NULL, "|");
        }
        if (token) {
            strcpy(entry->choice_a, trim_whitespace(token));
            token = strtok(NULL, "|");
        }
        if (token) {
            strcpy(entry->choice_b, trim_whitespace(token));
            token = strtok(NULL, "|");
        }
        if (token) {
            entry->next_line_a = atoi(trim_whitespace(token));  // Where choice A goes
            token = strtok(NULL, "|");
        }
        if (token) {
            entry->next_line_b = atoi(trim_whitespace(token));  // Where choice B goes
        }
        return;
    }
    
    // ============================================================================
    // SPECIAL COMMAND: [GOTO] - Jump to different line
    // ============================================================================
    
    if (strncmp(line, "[GOTO]", 6) == 0) {
        entry->next_line = atoi(line + 6);  // Number after [GOTO]
        strcpy(entry->speaker, "SYSTEM");
        strcpy(entry->text, "GOTO");
        return;
    }
    
    // ============================================================================
    // REGULAR DIALOGUE LINE
    // Format: Speaker: Text | next_line_number (optional)
    // Example: "Naberius: Who goes there? | 5"
    // ============================================================================
    
    char *colon = strchr(line, ':');  // Find the colon
    if (colon) {
        *colon = '\0';  // Split string at colon
        strcpy(entry->speaker, trim_whitespace(line));
        
        char *rest = colon + 1;
        char *pipe = strchr(rest, '|');  // Check for |next_line
        
        if (pipe) {
            *pipe = '\0';
            strcpy(entry->text, trim_whitespace(rest));
            entry->next_line = atoi(trim_whitespace(pipe + 1));
        } else {
            strcpy(entry->text, trim_whitespace(rest));
            entry->next_line = -1;  // Auto-increment
        }
    } else {
        // No speaker specified, just text
        strcpy(entry->text, line);
    }
}

// ============================================================================
// DIALOGUE LOAD - Read entire .txt file into memory
// ============================================================================

DialogueScript* dialogue_load(const char *filename) {
    // STEP 1: Open the file
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open dialogue file: %s\n", filename);
        return NULL;
    }
    
    // STEP 2: Allocate memory for script
    DialogueScript *script = calloc(1, sizeof(DialogueScript));
    if (!script) {
        fclose(file);
        return NULL;
    }
    
    strcpy(script->file_path, filename);
    
    // STEP 3: Read line by line
    char line[MAX_TEXT_LEN];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), file) && script->line_count < MAX_DIALOGUE_LINES) {
        // Remove newline character from end
        line[strcspn(line, "\n")] = 0;
        
        // Parse this line into the next slot
        parse_line(line, &script->lines[script->line_count]);
        
        // Only count non-empty lines
        if (strlen(script->lines[script->line_count].text) > 0 ||
            script->lines[script->line_count].triggers_battle) {
            
            // Auto-set next line if not specified
            if (script->lines[script->line_count].next_line == -1 && 
                !script->lines[script->line_count].is_player_choice) {
                script->lines[script->line_count].next_line = script->line_count + 1;
            }
            
            script->line_count++;
        }
    }
    
    fclose(file);
    
    printf("Loaded dialogue: %d lines from %s\n", script->line_count, filename);
    return script;
}

// ============================================================================
// DIALOGUE DESTROY - Free memory when done
// ============================================================================

void dialogue_destroy(DialogueScript *script) {
    free(script);
}

// ============================================================================
// GET LINE - Retrieve specific line by index
// ============================================================================

DialogueEntry* dialogue_get_line(DialogueScript *script, int index) {
    if (!script || index < 0 || index >= script->line_count) return NULL;
    return &script->lines[index];
}