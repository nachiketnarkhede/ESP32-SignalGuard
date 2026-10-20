#include <ESP32SignalGuard.h>

// Configure pins and validation behaviour for your own setup.
ESP32SignalGuard::Config sensorConfig;
ESP32SignalGuard sensor(sensorConfig);

void setup() {
  Serial.begin(115200);

  // Defaults: TRIG=GPIO 5, ECHO=GPIO 18.
  // ECHO MUST go through a suitable divider/level shifter if it can output 5 V.
  sensor.begin();

  Serial.println("ESP32SignalGuard demo");
  Serial.println("status,pulse_us,raw_cm,filtered_cm");
}

void loop() {
  ESP32SignalGuard::Reading r = sensor.measure();

  Serial.print(ESP32SignalGuard::statusName(r.status));
  Serial.print(",");
  Serial.print(r.pulseUs);
  Serial.print(",");

  if (r.valid()) {
    Serial.print(r.rawDistanceCm, 2);
    Serial.print(",");
    Serial.println(r.filteredDistanceCm, 2);
  } else {
    Serial.println(",,");
  }

  // Print diagnostics periodically in a real project if needed.
  static uint32_t lastReport = 0;
  if (millis() - lastReport >= 10000) {
    lastReport = millis();
    ESP32SignalGuard::Diagnostics d = sensor.diagnostics();

    Serial.println("---- diagnostics ----");
    Serial.print("attempts: "); Serial.println(d.attempts);
    Serial.print("accepted: "); Serial.println(d.accepted);
    Serial.print("timeouts: "); Serial.println(d.timeouts);
    Serial.print("too short: "); Serial.println(d.tooShort);
    Serial.print("too long: "); Serial.println(d.tooLong);
    Serial.print("outliers: "); Serial.println(d.outliers);
  }
}
