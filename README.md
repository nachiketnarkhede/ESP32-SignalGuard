# ESP32SignalGuard

A small Arduino library for ESP32 projects using HC-SR04-style ultrasonic sensors. It provides pulse timeout handling, duration bounds, median filtering, optional jump rejection, status codes, and diagnostic counters.

## Install

1. Download and unzip `ESP32_SignalGuard.zip`.
2. In Arduino IDE, choose **Sketch → Include Library → Add .ZIP Library** and select the ZIP file. If Arduino does not accept the outer ZIP, copy the `ESP32SignalGuard` folder into your Arduino `libraries` folder.
3. Open **File → Examples → ESP32SignalGuard → SignalGuardDemo**.
4. Select your ESP32 board and port, then upload.

## Wiring

- HC-SR04 TRIG → ESP32 GPIO 5 (configurable)
- HC-SR04 ECHO → ESP32 GPIO 18 through a suitable voltage divider or level shifter
- Sensor GND and ESP32 GND → common ground
- VCC → sensor's specified supply

**Never connect a possible 5 V ECHO output directly to an original ESP32 GPIO.** Confirm the sensor board's voltage requirements.

## Basic use

```cpp
#include <ESP32SignalGuard.h>

ESP32SignalGuard::Config cfg;
ESP32SignalGuard sensor(cfg);

void setup() {
  Serial.begin(115200);
  sensor.begin();
}

void loop() {
  auto reading = sensor.measure();
  if (reading.valid()) {
    Serial.println(reading.filteredDistanceCm);
  } else {
    Serial.println(ESP32SignalGuard::statusName(reading.status));
  }
}
```

## Main controls

- `trigPin`, `echoPin`: GPIO selection
- `minPulseUs`, `maxPulseUs`: accepted pulse-width range
- `echoTimeoutUs`: maximum wait for a pulse
- `triggerIntervalMs`: minimum spacing between triggers
- `medianWindow`: odd number from 1 to 9
- `enableJumpCheck`: optional large-change rejection; disabled by default
- `maxJumpCm`: threshold used when jump checking is enabled
- `soundSpeedMps`: approximate speed of sound (default 343 m/s)

## What it can and cannot do

It can reject missing, too-short, too-long, and optionally implausible measurements; reduce the effect of occasional outliers; and count failures for diagnostics. The median filter adds latency because it reflects recent measurements. Optional jump rejection may reject genuine rapid movement, so enable it only when suitable for the application.

This implementation uses Arduino `pulseIn()` for broad compatibility. It is **not a hardware-timestamped RMT implementation**, and it cannot remove electrical noise, fix bad wiring, correct an unsafe voltage interface, or guarantee accurate readings. For timing-critical work, compare against an oscilloscope or logic analyzer and consider a version using ESP32 RMT hardware capture.

## Suggested validation

1. Compare readings with a measured target distance.
2. Log status counts over many acquisitions.
3. Change one condition at a time (wiring, workload, sensor position).
4. Compare the library with an unfiltered baseline.
5. Do not claim an improvement until repeated tests support it.
