#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>
#include "config.h"

// Note frequencies (Hz)
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_E5   659
#define NOTE_G5   784
#define NOTE_REST 0

// Non-blocking melody player state
static int _bgMelodyIndex = 0;
static unsigned long _bgNoteStartTime = 0;
static int _bgCurrentNoteDuration = 0;
static int _bgCurrentMelody = -1;

// Melody pointers (set by setBackgroundMelody)
static const int* _bgNotes = nullptr;
static const int* _bgDurations = nullptr;
static int _bgMelodyLen = 0;

// Play a melody (blocking)
void playMelody(const int *notes, const int *durations, int length) {
    for (int i = 0; i < length; i++) {
        int note = pgm_read_word(&notes[i]);
        int duration = pgm_read_word(&durations[i]);

        if (note == NOTE_REST) {
            noTone(BUZZER_PIN_1);
        } else {
            tone(BUZZER_PIN_1, note, duration);
        }
        delay(duration + 30);
    }
    noTone(BUZZER_PIN_1);
}

// Set background melody for non-blocking playback
// Pass notes/durations arrays and length, or nullptr to stop
void setBackgroundMelodyCustom(const int* notes, const int* durations, int length) {
    _bgNotes = notes;
    _bgDurations = durations;
    _bgMelodyLen = length;
    _bgMelodyIndex = 0;
    _bgNoteStartTime = millis();
    _bgCurrentNoteDuration = 0;
    noTone(BUZZER_PIN_1);
}

// Stop background melody
void stopBackgroundMelody() {
    _bgNotes = nullptr;
    _bgDurations = nullptr;
    _bgMelodyLen = 0;
    noTone(BUZZER_PIN_1);
}

// Update background melody (call every frame)
void updateBackgroundMelody() {
    if (_bgNotes == nullptr || _bgMelodyLen == 0) return;

    unsigned long currentTime = millis();

    if (currentTime - _bgNoteStartTime >= (unsigned long)(_bgCurrentNoteDuration + 20)) {
        if (_bgCurrentNoteDuration > 0) {
            _bgMelodyIndex++;
            if (_bgMelodyIndex >= _bgMelodyLen) {
                _bgMelodyIndex = 0;
            }
        }

        int note = pgm_read_word(&_bgNotes[_bgMelodyIndex]);
        _bgCurrentNoteDuration = pgm_read_word(&_bgDurations[_bgMelodyIndex]);
        _bgNoteStartTime = currentTime;

        if (note == NOTE_REST) {
            noTone(BUZZER_PIN_1);
        } else {
            tone(BUZZER_PIN_1, note, _bgCurrentNoteDuration);
        }
    }
}

// Play a simple beep
void playBeep(int freq, int durationMs) {
    tone(BUZZER_PIN_1, freq, durationMs);
}

#endif
