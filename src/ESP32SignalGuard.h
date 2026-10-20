#pragma once

#include <Arduino.h>

class ESP32SignalGuard {
public:
  enum Status : uint8_t {
    OK,
    TIMEOUT,
    PULSE_TOO_SHORT,
    PULSE_TOO_LONG,
    OUTLIER_REJECTED,
    NOT_READY
  };

  struct Config {
    uint8_t trigPin = 5;
    uint8_t echoPin = 18;

    // Adjust these for your sensor and measurement range.
    uint32_t minPulseUs = 100;
    uint32_t maxPulseUs = 25000;
    uint32_t echoTimeoutUs = 30000;
    uint32_t triggerIntervalMs = 60;

    // Odd number recommended; valid range is 1.. nine.
    uint8_t medianWindow = 5;

    // Optional jump rejection. Disabled by default because real objects
    // can move quickly. When enabled, a large jump is rejected once;
    // the next valid measurement can establish a new baseline.
    bool enableJumpCheck = false;
    float maxJumpCm = 80.0f;

    // Sound speed near room temperature; tune if temperature is known.
    float soundSpeedMps = 343.0f;
  };

  struct Reading {
    Status status = NOT_READY;
    uint32_t pulseUs = 0;
    float rawDistanceCm = NAN;
    float filteredDistanceCm = NAN;
    uint32_t timestampMs = 0;

    bool valid() const { return status == OK; }
  };

  struct Diagnostics {
    uint32_t attempts = 0;
    uint32_t accepted = 0;
    uint32_t timeouts = 0;
    uint32_t tooShort = 0;
    uint32_t tooLong = 0;
    uint32_t outliers = 0;
  };

  explicit ESP32SignalGuard(const Config& config);
  bool begin();
  Reading measure();
  Diagnostics diagnostics() const;
  void resetDiagnostics();
  void resetFilter();
  static const char* statusName(Status status);

private:
  Config cfg_;
  float history_[9];
  uint8_t historyCount_ = 0;
  uint8_t historyNext_ = 0;
  bool haveBaseline_ = false;
  float lastAcceptedCm_ = NAN;
  uint32_t lastTriggerMs_ = 0;
  bool triggeredOnce_ = false;
  Diagnostics stats_;

  float median(float value);
  bool waitForTriggerSlot();
};
