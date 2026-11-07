#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include <M5Unified.h>

// ============================================================================
// SoundManager — M5Stack CoreS3 SE I2S Speaker (AW88298) Melody & SFX Engine
// ============================================================================
// Hardware Note:
//   The M5Stack CoreS3 SE features a built-in 1W dynamic speaker driven by an
//   AW88298 16-bit I2S audio amplifier (not a passive piezo buzzer).
//   M5.Speaker synthesizes clean waveforms and frequency sweeps in hardware.
//
// Memory Footprint:
//   All melodies are stored as compact {freqHz, durationMs, gapMs} structs in
//   Flash (const static arrays), consuming < 2.5 KB total (< 0.02% of the
//   CoreS3 SE's 16 MB Flash) and 0 bytes of dynamic heap RAM.
//
// Non-Blocking Playback:
//   Calling playStartupTune(), playFinishTune(), or playAlertTune() queues the
//   melody state machine and returns immediately so touch UI and stepper control
//   remain 100% responsive. Calling update() in loop() advances the notes.
// ============================================================================

// Single musical note or sound-effect step
struct MelodyNote {
  uint16_t freqHz;     // Frequency in Hz (0 = rest / silence)
  uint16_t durationMs; // Tone duration in milliseconds
  uint16_t gapMs;      // Total step time before next note (>= durationMs)
};

// --- Startup Tune Options (8 choices, 0 = Off) ---
enum StartupTuneId {
  STARTUP_TUNE_OFF = 0,        // 0: Off (Silent)
  STARTUP_TUNE_STANDARD,       // 1: Standard Warm Chime
  STARTUP_TUNE_DOTA2,          // 2: Dota 2 "Game Ready" Horn
  STARTUP_TUNE_SPONGEBOB,      // 3: SpongeBob Sea Shanty Riff
  STARTUP_TUNE_ADVENTURE_TIME, // 4: Adventure Time Theme
  STARTUP_TUNE_CLASSICAL,      // 5: Classical (Beethoven Ode to Joy)
  STARTUP_TUNE_HEAVY_METAL,    // 6: Heavy Metal Power Riff
  STARTUP_TUNE_MARIO_1UP,      // 7: Mario 1-Up Chime
  STARTUP_TUNE_COUNT = 8
};

// --- Batch Finish / Process Complete Tune Options (10 choices, 0 = Off) ---
enum FinishTuneId {
  FINISH_TUNE_OFF = 0,         // 0: Off (Silent)
  FINISH_TUNE_STANDARD,        // 1: Standard Victory Fanfare
  FINISH_TUNE_DOTA2,           // 2: Dota 2 "Game Ready" Horn
  FINISH_TUNE_SPONGEBOB,       // 3: SpongeBob Bikini Bottom Riff
  FINISH_TUNE_ADVENTURE_TIME,  // 4: Adventure Time Jingle
  FINISH_TUNE_CLASSICAL,       // 5: Classical (Für Elise / Beethoven)
  FINISH_TUNE_HEAVY_METAL,     // 6: Heavy Metal Gallop Riff
  FINISH_TUNE_KITCHEN_TIMER,   // 7: Kitchen Timer Double-Bell
  FINISH_TUNE_IPHONE_DING,     // 8: iPhone Tri-Tone Crystal Ding
  FINISH_TUNE_BIRD_SONG,       // 9: Bird Song (Robin Chirp)
  FINISH_TUNE_COUNT = 10
};

// --- Error / Collision / Low-Wax Alert Tune Options (9 choices, 0 = Off) ---
enum AlertTuneId {
  ALERT_TUNE_OFF = 0,          // 0: Off (Silent)
  ALERT_TUNE_STANDARD,         // 1: Standard Industrial Dual-Beep
  ALERT_TUNE_MARIO_DEATH,      // 2: Mario Classic "Lost a Life"
  ALERT_TUNE_DOG_BARK,         // 3: Dog Bark ("Woof! Woof!")
  ALERT_TUNE_BIRD_ALARM,       // 4: Startled Bird Chirp
  ALERT_TUNE_KITCHEN_TIMER,    // 5: Kitchen Timer Rapid Ring
  ALERT_TUNE_IPHONE_DING,      // 6: iPhone Two-Note Ding
  ALERT_TUNE_HEAVY_METAL,      // 7: Heavy Metal Tritone Doom
  ALERT_TUNE_RETRO_FAIL,       // 8: Retro Arcade Defeat
  ALERT_TUNE_COUNT = 9
};

class SoundManager {
public:
  SoundManager();

  // Initializes M5.Speaker (AW88298 I2S amplifier) and applies initial volume (0..100%)
  void begin(int volumePercent = 70);

  // Sets speaker volume from 0% (muted) to 100% (max 255 on AW88298 I2S amp)
  void setVolume(int volumePercent);
  void setVolumePercent(int volumePercent) { setVolume(volumePercent); }
  int getVolume() const { return _volumePercent; }

  // Non-blocking melody players (calling any of these immediately interrupts any playing tune)
  void playStartupTune(int tuneId, bool blocking = false);
  void playFinishTune(int tuneId, bool blocking = false);
  void playAlertTune(int tuneId, bool blocking = false);

  // Short tactile UI / feedback clicks & beeps (respects volume setting)
  void playBeep(uint16_t freqHz, uint16_t durationMs);

  // Stops any currently playing melody immediately
  void stop();

  // Must be called once per loop() iteration to advance non-blocking melodies
  void update();

  // Human-readable display labels for the UI cards
  static const char* getStartupTuneName(int tuneId);
  static const char* getFinishTuneName(int tuneId);
  static const char* getAlertTuneName(int tuneId);

private:
  int _volumePercent = 70;
  const MelodyNote* _activeNotes = nullptr;
  int _activeLength = 0;
  int _currentIndex = 0;
  unsigned long _noteStartMs = 0;
  bool _isPlaying = false;

  void startSequence(const MelodyNote* notes, int length, bool blocking);
};

#endif // SOUND_MANAGER_H
