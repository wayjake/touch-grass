#ifndef TG_T9_INPUT_H
#define TG_T9_INPUT_H

#include "../platform.h"
#include <string.h>
#include <stdio.h>

// Note frequencies (if not defined elsewhere)
#ifndef NOTE_C4
#define NOTE_C4   262
#define NOTE_E4   330
#define NOTE_G4   392
#define NOTE_C5   523
#define NOTE_E5   659
#define NOTE_G5   784
#endif

// Forward declaration for dictionary
uint8_t findPredictions(const char* prefix, const char** results, uint8_t maxResults);

// T9 key mappings (index 0-9)
// Layout:  1   2   3
//          4   5   6
//          7   8   9
//          DEL 0   OK
const char* const T9_KEYS[] = {
    " ",        // 0 - space
    ".,!?",     // 1 - punctuation
    "ABC",      // 2
    "DEF",      // 3
    "GHI",      // 4
    "JKL",      // 5
    "MNO",      // 6
    "PQRS",     // 7
    "TUV",      // 8
    "WXYZ"      // 9
};

// Grid position to key number mapping
// Grid is 3x4:  row 0: 1,2,3  row 1: 4,5,6  row 2: 7,8,9  row 3: DEL,0,OK
const int8_t T9_GRID[4][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
    {-1, 0, -2}  // -1 = DEL, -2 = OK
};

// T9 state variables
char t9Buffer[13];              // Input buffer (12 chars + null)
uint8_t t9CursorPos = 0;        // Cursor position in buffer
uint8_t t9GridX = 1;            // Grid cursor X (0-2), start at middle (2)
uint8_t t9GridY = 0;            // Grid cursor Y (0-3), start at top row
uint8_t t9LastKey = 255;        // Last pressed key number (for cycling), 255 = none
uint8_t t9LetterIndex = 0;      // Index within current key's letters
bool t9CharPending = false;     // Is there a pending character?
unsigned long t9LastKeyTime = 0;
const unsigned long T9_TIMEOUT = 2000;  // 2 seconds before letter auto-confirms

// Prediction state
const char* t9Predictions[3];   // Pointers to dictionary words
uint8_t t9PredictionCount = 0;
uint8_t t9PredictionIndex = 0;
bool t9InPredictionMode = false;

// Result flags
bool t9InputComplete = false;
bool t9InputCancelled = false;

// Initialize T9 input
void t9Init(const char* initialText) {
    memset(t9Buffer, 0, sizeof(t9Buffer));
    if (initialText && initialText[0]) {
        strncpy(t9Buffer, initialText, 12);
    }
    t9CursorPos = strlen(t9Buffer);
    t9GridX = 1;  // Start at center (key 2)
    t9GridY = 0;
    t9LastKey = 255;
    t9LetterIndex = 0;
    t9CharPending = false;
    t9LastKeyTime = 0;
    t9PredictionCount = findPredictions(t9Buffer, t9Predictions, 3);
    t9PredictionIndex = 0;
    t9InPredictionMode = false;
    t9InputComplete = false;
    t9InputCancelled = false;
}

// Get current text
const char* t9GetText() {
    return t9Buffer;
}

// Get currently selected key from grid (-1=DEL, -2=OK, 0-9=number)
int8_t t9GetSelectedKey() {
    return T9_GRID[t9GridY][t9GridX];
}

// Get current pending character (or 0 if none)
char t9GetPendingChar() {
    if (!t9CharPending || t9LastKey > 9) return 0;
    const char* letters = T9_KEYS[t9LastKey];
    uint8_t len = strlen(letters);
    if (len == 0 || t9LetterIndex >= len) return 0;
    return letters[t9LetterIndex];
}

// Confirm pending character
void t9ConfirmChar() {
    if (t9CharPending && t9CursorPos < 12) {
        char c = t9GetPendingChar();
        if (c) {
            t9Buffer[t9CursorPos] = c;
            t9CursorPos++;
            t9Buffer[t9CursorPos] = '\0';
        }
    }
    t9CharPending = false;
    t9LetterIndex = 0;
    t9LastKey = 255;

    // Update predictions
    t9PredictionCount = findPredictions(t9Buffer, t9Predictions, 3);
    t9PredictionIndex = 0;
}

// Delete character
void t9DeleteChar() {
    if (t9CharPending) {
        // Cancel pending char
        t9CharPending = false;
        t9LetterIndex = 0;
        t9LastKey = 255;
    } else if (t9CursorPos > 0) {
        t9CursorPos--;
        t9Buffer[t9CursorPos] = '\0';
        // Update predictions
        t9PredictionCount = findPredictions(t9Buffer, t9Predictions, 3);
        t9PredictionIndex = 0;
    }
}

// Press a key (0-9) - cycles through letters if same key, confirms if different
void t9PressKey(uint8_t key) {
    if (key > 9) return;

    const char* letters = T9_KEYS[key];
    uint8_t numLetters = strlen(letters);

    // If pressing a different key than last time, confirm pending char first
    if (t9CharPending && t9LastKey != key) {
        t9ConfirmChar();
    }

    if (numLetters == 0) return;

    if (t9LastKey == key && t9CharPending) {
        // Same key - cycle to next letter
        t9LetterIndex++;
        if (t9LetterIndex >= numLetters) {
            t9LetterIndex = 0;
        }
    } else {
        // New key - start with first letter
        t9CharPending = true;
        t9LetterIndex = 0;
        t9LastKey = key;
    }

    t9LastKeyTime = platform_millis();
}

// Add space (confirms pending char first)
void t9AddSpace() {
    if (t9CharPending) {
        t9ConfirmChar();
    }
    if (t9CursorPos < 12) {
        t9Buffer[t9CursorPos] = ' ';
        t9CursorPos++;
        t9Buffer[t9CursorPos] = '\0';
        // Update predictions
        t9PredictionCount = findPredictions(t9Buffer, t9Predictions, 3);
        t9PredictionIndex = 0;
    }
}

// Select current prediction
void t9SelectPrediction() {
    if (t9PredictionCount > 0 && t9Predictions[t9PredictionIndex]) {
        // Confirm any pending char first
        if (t9CharPending) {
            t9CharPending = false;
            t9LetterIndex = 0;
            t9LastKey = 255;
        }
        // Copy prediction to buffer
        char wordBuf[13];
        const char* ptr = t9Predictions[t9PredictionIndex];
        uint8_t i = 0;
        char c;
        while ((c = ptr[i]) != '\0' && i < 12) {
            wordBuf[i++] = c;
        }
        wordBuf[i] = '\0';

        strncpy(t9Buffer, wordBuf, 12);
        t9Buffer[12] = '\0';
        t9CursorPos = strlen(t9Buffer);
        t9InPredictionMode = false;
        // Update predictions for new text
        t9PredictionCount = findPredictions(t9Buffer, t9Predictions, 3);
        t9PredictionIndex = 0;
    }
}

// Navigate predictions
void t9NavigatePredictions(int8_t direction) {
    if (t9PredictionCount == 0) return;

    if (direction > 0) {
        t9PredictionIndex++;
        if (t9PredictionIndex >= t9PredictionCount) {
            t9PredictionIndex = 0;
        }
    } else {
        if (t9PredictionIndex == 0) {
            t9PredictionIndex = t9PredictionCount - 1;
        } else {
            t9PredictionIndex--;
        }
    }
}

// Handle T9 input, returns true when input is confirmed or cancelled
bool t9HandleInput() {
    unsigned long now = platform_millis();

    // Auto-confirm after 2 second timeout
    if (t9CharPending && (now - t9LastKeyTime >= T9_TIMEOUT)) {
        t9ConfirmChar();
        platform_beep(NOTE_C5, 30);
    }

    // In prediction mode, D-pad navigates predictions
    if (t9InPredictionMode) {
        if (platform_dpad_up_pressed()) {
            t9NavigatePredictions(-1);
            platform_beep(NOTE_G4, 30);
        }
        if (platform_dpad_down_pressed()) {
            t9NavigatePredictions(1);
            platform_beep(NOTE_E4, 30);
        }
        if (platform_button_pressed(BTN_A)) {
            t9SelectPrediction();
            t9InPredictionMode = false;
            platform_beep(NOTE_G5, 50);
        }
        if (platform_button_pressed(BTN_B) || platform_dpad_left_pressed() || platform_dpad_right_pressed()) {
            t9InPredictionMode = false;
            platform_beep(NOTE_C5, 30);
        }
        return false;
    }

    // D-pad navigation on grid
    if (platform_dpad_up_pressed()) {
        if (t9GridY > 0) {
            t9GridY--;
            platform_beep(NOTE_G4, 30);
        }
    }
    if (platform_dpad_down_pressed()) {
        if (t9GridY < 3) {
            t9GridY++;
            platform_beep(NOTE_E4, 30);
        }
    }
    if (platform_dpad_left_pressed()) {
        if (t9GridX > 0) {
            t9GridX--;
            platform_beep(NOTE_G4, 30);
        }
    }
    if (platform_dpad_right_pressed()) {
        if (t9GridX < 2) {
            t9GridX++;
            platform_beep(NOTE_E4, 30);
        }
    }

    // A button - press selected key or action
    if (platform_button_pressed(BTN_A)) {
        int8_t key = t9GetSelectedKey();
        if (key >= 0) {
            // Number key 0-9
            t9PressKey(key);
            platform_beep(NOTE_G4, 30);
        } else if (key == -1) {
            // DEL
            t9DeleteChar();
            platform_beep(NOTE_C4, 30);
        } else if (key == -2) {
            // OK - confirm input
            if (t9CharPending) {
                t9ConfirmChar();
            }
            if (strlen(t9Buffer) > 0) {
                t9InputComplete = true;
                platform_beep(NOTE_G5, 50);
                return true;
            } else {
                platform_beep(NOTE_C4, 100);  // Error - empty name
            }
        }
    }

    // B button - space (tap) or cancel (if buffer empty and no pending)
    if (platform_button_pressed(BTN_B)) {
        if (t9CharPending || t9CursorPos > 0) {
            // Add space
            t9AddSpace();
            platform_beep(NOTE_E5, 30);
        } else {
            // Cancel - empty buffer
            t9InputCancelled = true;
            platform_beep(NOTE_C4, 50);
            return true;
        }
    }

    // Long press B to cancel (check if held for 500ms)
    static unsigned long bPressStart = 0;
    if (platform_button_held(BTN_B)) {
        if (bPressStart == 0) {
            bPressStart = now;
        } else if (now - bPressStart >= 500) {
            t9InputCancelled = true;
            platform_beep(NOTE_C4, 100);
            bPressStart = 0;
            return true;
        }
    } else {
        bPressStart = 0;
    }

    return false;
}

#endif
