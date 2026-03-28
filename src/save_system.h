#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <time.h>

#define MAX_SAVE_SLOTS 4        /* Number of save slots available */
#define SAVE_FILE_VERSION 1     /* File format version */
#define SAVE_FILENAME "saves/savegame.dat"  /* Where saves are stored */

/* All data that gets saved to disk */
typedef struct {
    int version;           /* File format version */
    time_t timestamp;      /* When the save was created */
    char player_name[32];  /* Player name */
    
    /* Progress */
    int current_level;
    int player_score;
    float player_x;        /* Position on map */
    float player_y;
    
    /* Dialogue state */
    char current_dialogue_file[256];
    int current_dialogue_line;
    
    /* Player stats */
    int player_max_hp;
    int player_current_hp;
    int player_max_chakra;
    int player_current_chakra;
    
    /* Story flags */
    bool has_met_naberius;
    bool defeated_naberius;
    bool tutorial_completed;
    
    /* Total play time in seconds */
    float play_time;
    
    /* Checksum for file corruption detection */
    unsigned int checksum;
    
} SaveSlotData;

/* Manages all save slots */
typedef struct {
    SaveSlotData slots[MAX_SAVE_SLOTS];      /* The 4 save slots */
    bool slot_occupied[MAX_SAVE_SLOTS];      /* true if slot has a save */
    int selected_slot;                       /* Currently selected (-1 = none) */
} SaveSystem;

/* Create and destroy */
SaveSystem* save_system_create(void);
void save_system_destroy(SaveSystem *sys);

/* Save operations */
bool save_game(SaveSystem *sys, int slot, const char *player_name, 
               int level, int score, float player_x, float player_y,
               const char *dialogue_file, int dialogue_line,
               int max_hp, int current_hp, int max_chakra, int current_chakra,
               bool met_naberius, bool defeated_naberius, bool tutorial_done,
               float play_time);
bool load_game(SaveSystem *sys, int slot, SaveSlotData *out_data);
bool delete_save(SaveSystem *sys, int slot);
bool has_save_in_slot(SaveSystem *sys, int slot);

/* File I/O */
bool save_system_write_to_disk(SaveSystem *sys);
bool save_system_read_from_disk(SaveSystem *sys);

/* Get formatted info for display */
void save_system_get_slot_info(SaveSystem *sys, int slot, 
                                char *date_str, size_t date_len,
                                char *preview_str, size_t preview_len);

#endif