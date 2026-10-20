#include "ESP32SignalGuard.h"

ESP32SignalGuard::ESP32SignalGuard(const Config& config) : cfg_(config) {
  if (cfg_.medianWindow < 1) cfg_.medianWindow = 1;
  if (cfg_.medianWindow > 9) cfg_.medianWindow = 9;
  if ((cfg_.medianWindow % 2) == 0) cfg_.medianWindow--; // odd window
  if (cfg_.soundSpeedMps < 250.0f || cfg_.soundSpeedMps > 400.0f)
    cfg_.soundSpeedMps = 343.0f;
}

bool ESP32SignalGuard::begin() {
  pinMode(cfg_.trigPin, OUTPUT);
  pinMode(cfg_.echoPin, INPUT);
  digitalWrite(cfg_.trigPin, LOW);
  delayMicroseconds(3);
  return true;
}

bool ESP32SignalGuard::waitForTriggerSlot() {
  if (!triggeredOnce_) return true;
  uint32_t elapsed = millis() - lastTriggerMs_;
  if (elapsed < cfg_.triggerIntervalMs) {
    delay(cfg_.triggerIntervalMs - elapsed);
  }
  return true;
}

ESP32SignalGuard::Reading ESP32SignalGuard::measure() {
  Reading r;
  r.timestampMs = millis();
  stats_.attempts++;

  waitForTriggerSlot();

  // Clear trigger low, then issue the HC-SR04 trigger pulse.
  digitalWrite(cfg_.trigPin, LOW);
  delayMicroseconds(3);
  digitalWrite(cfg_.trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(cfg_.trigPin, LOW);
  lastTriggerMs_ = millis();
  triggeredOnce_ = true;

  // pulseIn returns 0 when no complete HIGH pulse arrives before timeout.
  uint32_t width = pulseIn(cfg_.echoPin, HIGH, cfg_.echoTimeoutUs);

  r.timestampMs = millis();

  if (width == 0) {
    r.status = TIMEOUT;
    stats_.timeouts++;
    return r;
  }

  r.pulseUs = width;

  if (width < cfg_.minPulseUs) {
    r.status = PULSE_TOO_SHORT;
    stats_.tooShort++;
    return r;
  }

  if (width > cfg_.maxPulseUs) {
    r.status = PULSE_TOO_LONG;
    stats_.tooLong++;
    return r;
  }

  // HC-SR04 pulse is round-trip time, so divide by 2.
  r.rawDistanceCm = (width * cfg_.soundSpeedMps) / 20000.0f;

  if (cfg_.enableJumpCheck && haveBaseline_ &&
      fabsf(r.rawDistanceCm - lastAcceptedCm_) > cfg_.maxJumpCm) {
    r.status = OUTLIER_REJECTED;
    stats_.outliers++;
    return r;
  }

  r.filteredDistanceCm = median(r.rawDistanceCm);
  lastAcceptedCm_ = r.filteredDistanceCm;
  haveBaseline_ = true;

  r.status = OK;
  stats_.accepted++;
  return r;
}

float ESP32SignalGuard::median(float value) {
  history_[historyNext_] = value;
  historyNext_ = (historyNext_ + 1) % cfg_.medianWindow;
  if (historyCount_ < cfg_.medianWindow) historyCount_++;

  float sorted[9];
  for (uint8_t i = 0; i < historyCount_; i++) sorted[i] = history_[i];

  for (uint8_t i = 1; i < historyCount_; i++) {
    float key = sorted[i];
    int j = (int)i - 1;
    while (j >= 0 && sorted[j] > key) {
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = key;
  }

  return sorted[historyCount_ / 2];
}

ESP32SignalGuard::Diagnostics ESP32SignalGuard::diagnostics() const {
  return stats_;
}

void ESP32SignalGuard::resetDiagnostics() {
  stats_ = Diagnostics{};
}

void ESP32SignalGuard::resetFilter() {
  historyCount_ = 0;
  historyNext_ = 0;
  haveBaseline_ = false;
  lastAcceptedCm_ = NAN;
}

const char* ESP32SignalGuard::statusName(Status status) {
  switch (status) {
    case OK: return "OK";
    case TIMEOUT: return "TIMEOUT";
    case PULSE_TOO_SHORT: return "PULSE_TOO_SHORT";
    case PULSE_TOO_LONG: return "PULSE_TOO_LONG";
    case OUTLIER_REJECTED: return "OUTLIER_REJECTED";
    default: return "NOT_READY";
  }
}
