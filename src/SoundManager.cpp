#include "SoundManager.h"

// ============================================================================
// Musical Note Frequency Constants (Hz)
// ============================================================================
#define NOTE_REST 0
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_A6  1760

// ============================================================================
// 1. STARTUP & SHARED MELODY TABLES (Stored in Flash)
// ============================================================================

// Standard Warm Ascending Chime
static const MelodyNote MELODY_STARTUP_STD[] = {
  { NOTE_C5, 90, 110 },
  { NOTE_E5, 90, 110 },
  { NOTE_G5, 90, 110 },
  { NOTE_C6, 240, 260 }
};

// Dota 2 "Game Ready" — Loud deep warhorn + brass impact + metallic gong
static const MelodyNote MELODY_DOTA2_READY[] = {
  { NOTE_D3,  70,  80 },  // Deep horn attack
  { NOTE_A3,  90, 100 },  // Brass swell
  { NOTE_D4, 280, 310 },  // Main "BWAAAH!" horn blast
  { NOTE_A3, 140, 160 },  // Low horn echo
  { NOTE_F4, 120, 135 },  // Metallic anvil chime 1
  { NOTE_E4, 120, 135 },  // Metallic anvil chime 2
  { NOTE_D4, 380, 400 }   // Final resonant horn sustain
};

// SpongeBob Sea-Shanty Nose-Flute Riff
static const MelodyNote MELODY_SPONGEBOB[] = {
  { NOTE_G5, 130, 150 },
  { NOTE_A5, 130, 150 },
  { NOTE_G5, 130, 150 },
  { NOTE_E5, 130, 150 },
  { NOTE_C5, 110, 130 },
  { NOTE_E5, 110, 130 },
  { NOTE_G5, 220, 250 },
  { NOTE_D5, 120, 140 },
  { NOTE_E5, 120, 140 },
  { NOTE_D5, 120, 140 },
  { NOTE_B4, 120, 140 },
  { NOTE_G4, 110, 130 },
  { NOTE_B4, 110, 130 },
  { NOTE_D5, 200, 230 },
  { NOTE_C5, 110, 125 },
  { NOTE_E5, 110, 125 },
  { NOTE_G5, 130, 150 },
  { NOTE_A5, 130, 150 },
  { NOTE_G5, 140, 160 },
  { NOTE_C6, 300, 320 }
};

// Adventure Time Theme ("Adventure Time, c'mon grab your friends...")
static const MelodyNote MELODY_ADVENTURE_TIME[] = {
  { NOTE_D5, 120, 140 },  // Ad-
  { NOTE_G5, 180, 200 },  // ven-
  { NOTE_G5, 120, 140 },  // ture
  { NOTE_G5, 220, 250 },  // Time,
  { NOTE_G5, 110, 130 },  // c'mon
  { NOTE_FS5,140, 160 },  // grab
  { NOTE_G5, 140, 160 },  // your
  { NOTE_A5, 200, 220 },  // friends,
  { NOTE_G5, 130, 150 },  // we'll
  { NOTE_FS5,130, 150 },  // go
  { NOTE_E5, 130, 150 },  // to
  { NOTE_D5, 160, 180 },  // ve-ry
  { NOTE_E5, 160, 180 },  // dis-tant
  { NOTE_D5, 140, 160 },  // lands!
  { NOTE_G5, 280, 300 }
};

// Classical 1: Beethoven's "Ode to Joy" (9th Symphony)
static const MelodyNote MELODY_CLASSICAL_ODE[] = {
  { NOTE_E5, 150, 170 },
  { NOTE_E5, 150, 170 },
  { NOTE_F5, 150, 170 },
  { NOTE_G5, 150, 170 },
  { NOTE_G5, 150, 170 },
  { NOTE_F5, 150, 170 },
  { NOTE_E5, 150, 170 },
  { NOTE_D5, 150, 170 },
  { NOTE_C5, 150, 170 },
  { NOTE_C5, 150, 170 },
  { NOTE_D5, 150, 170 },
  { NOTE_E5, 150, 170 },
  { NOTE_E5, 220, 240 },
  { NOTE_D5,  90, 110 },
  { NOTE_D5, 280, 300 }
};

// Classical 2: Beethoven's "Für Elise" (used on Finish Tune)
static const MelodyNote MELODY_CLASSICAL_ELISE[] = {
  { NOTE_E5, 120, 135 },
  { NOTE_DS5,120, 135 },
  { NOTE_E5, 120, 135 },
  { NOTE_DS5,120, 135 },
  { NOTE_E5, 120, 135 },
  { NOTE_B4, 120, 135 },
  { NOTE_D5, 120, 135 },
  { NOTE_C5, 120, 135 },
  { NOTE_A4, 240, 270 },
  { NOTE_C4, 120, 135 },
  { NOTE_E4, 120, 135 },
  { NOTE_A4, 120, 135 },
  { NOTE_B4, 240, 270 },
  { NOTE_E4, 120, 135 },
  { NOTE_GS4,120, 135 },
  { NOTE_B4, 120, 135 },
  { NOTE_C5, 280, 300 }
};

// Heavy Metal Gallop & Power Riff (E-minor palm-mute gallop + power chords)
static const MelodyNote MELODY_HEAVY_METAL[] = {
  { NOTE_E3,  70,  85 },  // Palm-mute gallop 1
  { NOTE_E3,  70,  85 },
  { NOTE_E4, 130, 150 },  // Octave accent
  { NOTE_E3,  70,  85 },  // Gallop 2
  { NOTE_E3,  70,  85 },
  { NOTE_G4, 130, 150 },  // Minor 3rd accent
  { NOTE_E3,  70,  85 },  // Gallop 3
  { NOTE_E3,  70,  85 },
  { NOTE_A4, 130, 150 },  // 4th accent
  { NOTE_AS4,110, 125 },  // Tritone ("Devil's interval")
  { NOTE_A4, 110, 125 },
  { NOTE_G4, 110, 125 },
  { NOTE_E4, 300, 320 }   // Power chord sustain
};

// Mario Classic 1-Up Arpeggio
static const MelodyNote MELODY_MARIO_1UP[] = {
  { NOTE_E5, 100, 120 },
  { NOTE_G5, 100, 120 },
  { NOTE_E6, 100, 120 },
  { NOTE_C6, 100, 120 },
  { NOTE_D6, 100, 120 },
  { NOTE_G6, 220, 240 }
};

// ============================================================================
// 2. BATCH FINISH / COMPLETION TUNES
// ============================================================================

// Standard Workshop Victory Fanfare
static const MelodyNote MELODY_FINISH_STD[] = {
  { NOTE_G4, 110, 130 },
  { NOTE_C5, 110, 130 },
  { NOTE_E5, 110, 130 },
  { NOTE_G5, 220, 250 },
  { NOTE_E5, 100, 120 },
  { NOTE_G5, 380, 400 }
};

// Kitchen Timer Double-Bell Ring ("Ding-Ding! Ding-Ding! Ding-Ding!")
static const MelodyNote MELODY_KITCHEN_TIMER[] = {
  { 2650,  55,  70 },
  { 2950,  55,  70 },
  { 2650,  55,  70 },
  { 2950, 140, 240 },
  { 2650,  55,  70 },
  { 2950,  55,  70 },
  { 2650,  55,  70 },
  { 2950, 140, 240 },
  { 2650,  55,  70 },
  { 2950,  55,  70 },
  { 2650,  55,  70 },
  { 2950, 260, 280 }
};

// iPhone Tri-Tone Crystal Notification Ding
static const MelodyNote MELODY_IPHONE_TRITONE[] = {
  { NOTE_B5,  130, 150 },
  { NOTE_DS6, 130, 150 },
  { NOTE_FS6, 320, 340 }
};

// Bird Song (Cheerful Robin / Canary Chirp & Warble)
static const MelodyNote MELODY_BIRD_SONG[] = {
  { 2300,  45,  55 },
  { 2850,  45,  55 },
  { 2500,  45,  55 },
  { 3200,  90, 150 },
  { 2600,  40,  50 },
  { 3400,  40,  50 },
  { 2900,  40,  50 },
  { 3600,  40,  50 },
  { 3100,  40,  50 },
  { 3800, 160, 180 }
};

// ============================================================================
// 3. ERROR / COLLISION / LOW-WAX ALERT TUNES (Short & Distinct)
// ============================================================================

// Standard Industrial Dual-Tone Alert
static const MelodyNote MELODY_ALERT_STD[] = {
  { 440, 160, 190 },
  { 330, 260, 290 },
  { 440, 160, 190 },
  { 330, 320, 340 }
};

// Super Mario Bros. Classic "Lost a Life / Death" Jingle
static const MelodyNote MELODY_MARIO_DEATH[] = {
  { NOTE_B4,  90, 110 },
  { NOTE_F5, 160, 200 },
  { NOTE_F5,  90, 115 },
  { NOTE_F5, 110, 135 },
  { NOTE_E5, 110, 135 },
  { NOTE_D5, 110, 135 },
  { NOTE_C5, 140, 165 },
  { NOTE_E4, 140, 165 },
  { NOTE_C4, 280, 300 }
};

// Dog Bark ("Woof! Woof!") — Two punchy pitch-swept barks
static const MelodyNote MELODY_DOG_BARK[] = {
  // First "WOOF!"
  { 170,  35,  35 },
  { 320,  55,  55 },
  { 260,  45,  45 },
  { 140,  50, 160 },
  // Second "WOOF!"
  { 185,  35,  35 },
  { 360,  60,  60 },
  { 280,  45,  45 },
  { 150,  60,  80 }
};

// Startled Bird Chirp Alert
static const MelodyNote MELODY_BIRD_ALARM[] = {
  { 2800,  45,  55 },
  { 3600,  55,  70 },
  { 2600,  45,  55 },
  { 3800,  85, 100 },
  { 2900,  45,  55 },
  { 4000, 120, 140 }
};

// Short Kitchen Timer Alert Ring
static const MelodyNote MELODY_ALERT_TIMER[] = {
  { 2700,  55,  70 },
  { 3000,  55,  70 },
  { 2700,  55,  70 },
  { 3000, 180, 200 }
};

// Short iPhone Two-Note Ding
static const MelodyNote MELODY_ALERT_DING[] = {
  { NOTE_GS5, 110, 130 },
  { NOTE_DS6, 260, 280 }
};

// Heavy Metal Tritone Doom Alert
static const MelodyNote MELODY_ALERT_METAL[] = {
  { NOTE_E3,  180, 200 },
  { NOTE_E4,  180, 200 },
  { NOTE_AS3, 380, 400 }
};

// Retro Arcade Defeat (Descending Warble)
static const MelodyNote MELODY_RETRO_FAIL[] = {
  { NOTE_A4, 140, 155 },
  { NOTE_GS4,140, 155 },
  { NOTE_G4, 140, 155 },
  { NOTE_FS4,360, 380 }
};

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

// ============================================================================
// SoundManager Implementation
// ============================================================================

SoundManager::SoundManager() {}

void SoundManager::begin(int volumePercent) {
  auto spkCfg = M5.Speaker.config();
  M5.Speaker.config(spkCfg);
  M5.Speaker.begin();
  setVolume(volumePercent);
}

void SoundManager::setVolume(int volumePercent) {
  _volumePercent = constrain(volumePercent, 0, 100);
  // Map 0..100% linearly to 0..255 on the CoreS3 SE AW88298 I2S amplifier
  uint8_t hwVol = (uint8_t)map(_volumePercent, 0, 100, 0, 255);
  M5.Speaker.setVolume(hwVol);
}

void SoundManager::stop() {
  _isPlaying = false;
  _activeNotes = nullptr;
  _activeLength = 0;
  _currentIndex = 0;
  M5.Speaker.stop();
}

void SoundManager::playBeep(uint16_t freqHz, uint16_t durationMs) {
  if (_volumePercent <= 0) return;
  // Don't overwrite an active multi-note melody preview with a button click
  if (_isPlaying) return;
  M5.Speaker.tone(freqHz, durationMs);
}

void SoundManager::startSequence(const MelodyNote* notes, int length, bool blocking) {
  stop();
  if (_volumePercent <= 0 || notes == nullptr || length <= 0) return;

  if (blocking) {
    for (int i = 0; i < length; i++) {
      if (notes[i].freqHz > 0 && notes[i].durationMs > 0) {
        M5.Speaker.tone(notes[i].freqHz, notes[i].durationMs);
      }
      uint16_t waitMs = max(notes[i].durationMs, notes[i].gapMs);
      delay(waitMs);
    }
    return;
  }

  _activeNotes = notes;
  _activeLength = length;
  _currentIndex = 0;
  _isPlaying = true;
  _noteStartMs = millis();

  if (_activeNotes[0].freqHz > 0 && _activeNotes[0].durationMs > 0) {
    M5.Speaker.tone(_activeNotes[0].freqHz, _activeNotes[0].durationMs);
  }
}

void SoundManager::update() {
  if (!_isPlaying || _activeNotes == nullptr) return;

  unsigned long now = millis();
  uint16_t stepWait = max(_activeNotes[_currentIndex].durationMs, _activeNotes[_currentIndex].gapMs);

  if (now - _noteStartMs >= stepWait) {
    _currentIndex++;
    if (_currentIndex >= _activeLength) {
      _isPlaying = false;
      _activeNotes = nullptr;
      return;
    }
    _noteStartMs = now;
    if (_activeNotes[_currentIndex].freqHz > 0 && _activeNotes[_currentIndex].durationMs > 0) {
      M5.Speaker.tone(_activeNotes[_currentIndex].freqHz, _activeNotes[_currentIndex].durationMs);
    }
  }
}

void SoundManager::playStartupTune(int tuneId, bool blocking) {
  switch (tuneId) {
    case STARTUP_TUNE_OFF:            stop(); break;
    case STARTUP_TUNE_STANDARD:       startSequence(MELODY_STARTUP_STD,    ARRAY_LEN(MELODY_STARTUP_STD),    blocking); break;
    case STARTUP_TUNE_DOTA2:          startSequence(MELODY_DOTA2_READY,    ARRAY_LEN(MELODY_DOTA2_READY),    blocking); break;
    case STARTUP_TUNE_SPONGEBOB:      startSequence(MELODY_SPONGEBOB,      ARRAY_LEN(MELODY_SPONGEBOB),      blocking); break;
    case STARTUP_TUNE_ADVENTURE_TIME: startSequence(MELODY_ADVENTURE_TIME, ARRAY_LEN(MELODY_ADVENTURE_TIME), blocking); break;
    case STARTUP_TUNE_CLASSICAL:      startSequence(MELODY_CLASSICAL_ODE,  ARRAY_LEN(MELODY_CLASSICAL_ODE),  blocking); break;
    case STARTUP_TUNE_HEAVY_METAL:    startSequence(MELODY_HEAVY_METAL,    ARRAY_LEN(MELODY_HEAVY_METAL),    blocking); break;
    case STARTUP_TUNE_MARIO_1UP:      startSequence(MELODY_MARIO_1UP,      ARRAY_LEN(MELODY_MARIO_1UP),      blocking); break;
    default:                          startSequence(MELODY_STARTUP_STD,    ARRAY_LEN(MELODY_STARTUP_STD),    blocking); break;
  }
}

void SoundManager::playFinishTune(int tuneId, bool blocking) {
  switch (tuneId) {
    case FINISH_TUNE_OFF:            stop(); break;
    case FINISH_TUNE_STANDARD:       startSequence(MELODY_FINISH_STD,      ARRAY_LEN(MELODY_FINISH_STD),      blocking); break;
    case FINISH_TUNE_DOTA2:          startSequence(MELODY_DOTA2_READY,     ARRAY_LEN(MELODY_DOTA2_READY),     blocking); break;
    case FINISH_TUNE_SPONGEBOB:      startSequence(MELODY_SPONGEBOB,       ARRAY_LEN(MELODY_SPONGEBOB),       blocking); break;
    case FINISH_TUNE_ADVENTURE_TIME: startSequence(MELODY_ADVENTURE_TIME,  ARRAY_LEN(MELODY_ADVENTURE_TIME),  blocking); break;
    case FINISH_TUNE_CLASSICAL:      startSequence(MELODY_CLASSICAL_ELISE, ARRAY_LEN(MELODY_CLASSICAL_ELISE), blocking); break;
    case FINISH_TUNE_HEAVY_METAL:    startSequence(MELODY_HEAVY_METAL,     ARRAY_LEN(MELODY_HEAVY_METAL),     blocking); break;
    case FINISH_TUNE_KITCHEN_TIMER:  startSequence(MELODY_KITCHEN_TIMER,   ARRAY_LEN(MELODY_KITCHEN_TIMER),   blocking); break;
    case FINISH_TUNE_IPHONE_DING:    startSequence(MELODY_IPHONE_TRITONE,  ARRAY_LEN(MELODY_IPHONE_TRITONE),  blocking); break;
    case FINISH_TUNE_BIRD_SONG:      startSequence(MELODY_BIRD_SONG,       ARRAY_LEN(MELODY_BIRD_SONG),       blocking); break;
    default:                         startSequence(MELODY_FINISH_STD,      ARRAY_LEN(MELODY_FINISH_STD),      blocking); break;
  }
}

void SoundManager::playAlertTune(int tuneId, bool blocking) {
  switch (tuneId) {
    case ALERT_TUNE_OFF:           stop(); break;
    case ALERT_TUNE_STANDARD:      startSequence(MELODY_ALERT_STD,    ARRAY_LEN(MELODY_ALERT_STD),    blocking); break;
    case ALERT_TUNE_MARIO_DEATH:   startSequence(MELODY_MARIO_DEATH,  ARRAY_LEN(MELODY_MARIO_DEATH),  blocking); break;
    case ALERT_TUNE_DOG_BARK:      startSequence(MELODY_DOG_BARK,     ARRAY_LEN(MELODY_DOG_BARK),     blocking); break;
    case ALERT_TUNE_BIRD_ALARM:    startSequence(MELODY_BIRD_ALARM,   ARRAY_LEN(MELODY_BIRD_ALARM),   blocking); break;
    case ALERT_TUNE_KITCHEN_TIMER: startSequence(MELODY_ALERT_TIMER,  ARRAY_LEN(MELODY_ALERT_TIMER),  blocking); break;
    case ALERT_TUNE_IPHONE_DING:   startSequence(MELODY_ALERT_DING,   ARRAY_LEN(MELODY_ALERT_DING),   blocking); break;
    case ALERT_TUNE_HEAVY_METAL:   startSequence(MELODY_ALERT_METAL,  ARRAY_LEN(MELODY_ALERT_METAL),  blocking); break;
    case ALERT_TUNE_RETRO_FAIL:    startSequence(MELODY_RETRO_FAIL,   ARRAY_LEN(MELODY_RETRO_FAIL),   blocking); break;
    default:                       startSequence(MELODY_ALERT_STD,    ARRAY_LEN(MELODY_ALERT_STD),    blocking); break;
  }
}

const char* SoundManager::getStartupTuneName(int tuneId) {
  static const char* names[STARTUP_TUNE_COUNT] = {
    "Off (Silent)",
    "Standard",
    "Dota 2 Ready",
    "SpongeBob",
    "Adv. Time",
    "Classical",
    "Heavy Metal",
    "Mario 1-Up"
  };
  return names[constrain(tuneId, 0, STARTUP_TUNE_COUNT - 1)];
}

const char* SoundManager::getFinishTuneName(int tuneId) {
  static const char* names[FINISH_TUNE_COUNT] = {
    "Off (Silent)",
    "Standard",
    "Dota 2 Ready",
    "SpongeBob",
    "Adv. Time",
    "Classical",
    "Heavy Metal",
    "Kitchen Timer",
    "iPhone Ding",
    "Bird Song"
  };
  return names[constrain(tuneId, 0, FINISH_TUNE_COUNT - 1)];
}

const char* SoundManager::getAlertTuneName(int tuneId) {
  static const char* names[ALERT_TUNE_COUNT] = {
    "Off (Silent)",
    "Standard",
    "Mario Death",
    "Dog Bark",
    "Bird Chirp",
    "Timer Ring",
    "iPhone Ding",
    "Metal Doom",
    "Retro Fail"
  };
  return names[constrain(tuneId, 0, ALERT_TUNE_COUNT - 1)];
}
