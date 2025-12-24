#ifndef TG_DIALOG_H
#define TG_DIALOG_H

#include <stdint.h>
#include "../shared/platform.h"
#include "sprites.h"

// Dialog configuration
#define DIALOG_MAX_LINES 3
#define DIALOG_LINE_WIDTH 16    // Characters per line at size 1 (with portrait)
#define DIALOG_BOX_HEIGHT 26    // Pixels for dialog box
#define DIALOG_BOX_Y (PLATFORM_SCREEN_HEIGHT - DIALOG_BOX_HEIGHT)

// Dialog speaker types
enum DialogSpeaker {
    SPEAKER_NONE = 0,
    SPEAKER_SPIRIT,
    SPEAKER_PLAYER
};

// Dialog page structure - a single "screen" of dialog
struct DialogPage {
    DialogSpeaker speaker;
    const char* line1;
    const char* line2;
    const char* line3;
};

// Dialog state
static const DialogPage* currentDialog = nullptr;
static uint8_t dialogPageIndex = 0;
static uint8_t dialogPageCount = 0;

// Spirit dialog script - the sanctuary narrative
static const DialogPage spiritDialog[] = {
    { SPEAKER_SPIRIT, "Greetings,", "wanderer...", nullptr },
    { SPEAKER_SPIRIT, "A darkness", "spreads across", "these lands..." },
    { SPEAKER_SPIRIT, "The creatures", "flee but have", "nowhere safe." },
    { SPEAKER_SPIRIT, "You must build", "a sanctuary to", "shelter them." },
    { SPEAKER_SPIRIT, "Explore far,", "catch the lost,", "return them here." },
    { SPEAKER_SPIRIT, "But beware -", "stay fed, avoid", "corrupted beasts." },
    { SPEAKER_SPIRIT, "The world", "beyond awaits...", nullptr }
};
#define SPIRIT_DIALOG_LEN 7

// Start a dialog sequence
static void startDialog(const DialogPage* pages, uint8_t pageCount) {
    currentDialog = pages;
    dialogPageCount = pageCount;
    dialogPageIndex = 0;
}

// Advance to next dialog page, returns true if dialog is complete
static bool advanceDialog() {
    dialogPageIndex++;
    if (dialogPageIndex >= dialogPageCount) {
        currentDialog = nullptr;
        dialogPageIndex = 0;
        dialogPageCount = 0;
        return true;  // Dialog complete
    }
    return false;
}

// Check if dialog is active
static bool isDialogActive() {
    return currentDialog != nullptr && dialogPageIndex < dialogPageCount;
}

// Draw the current dialog page
static void drawDialog() {
    if (!isDialogActive()) return;

    const DialogPage* page = &currentDialog[dialogPageIndex];

    // Draw dialog box background (black with white border)
    platform_fill_rect(0, DIALOG_BOX_Y, PLATFORM_SCREEN_WIDTH, DIALOG_BOX_HEIGHT, false);
    platform_draw_rect(0, DIALOG_BOX_Y, PLATFORM_SCREEN_WIDTH, DIALOG_BOX_HEIGHT, true);

    // Draw portrait on left side (2x scaled)
    if (page->speaker == SPEAKER_SPIRIT) {
        const uint8_t* sprite = getTileSprite(TG_SPIRIT);
        if (sprite != nullptr) {
            platform_draw_sprite_scaled(4, DIALOG_BOX_Y + 5, sprite, 2);
        }
    }

    // Draw text lines (right of portrait)
    int textX = 24;
    int textY = DIALOG_BOX_Y + 4;
    platform_set_text_size(1);

    if (page->line1) {
        platform_set_cursor(textX, textY);
        platform_print(page->line1);
    }
    if (page->line2) {
        platform_set_cursor(textX, textY + 8);
        platform_print(page->line2);
    }
    if (page->line3) {
        platform_set_cursor(textX, textY + 16);
        platform_print(page->line3);
    }

    // Draw "A" prompt in bottom right corner
    platform_set_cursor(PLATFORM_SCREEN_WIDTH - 16, DIALOG_BOX_Y + DIALOG_BOX_HEIGHT - 9);
    platform_print("[A]");
}

// Start the spirit dialog
static void startSpiritDialog() {
    startDialog(spiritDialog, SPIRIT_DIALOG_LEN);
}

#endif
