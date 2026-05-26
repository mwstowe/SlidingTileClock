# Sliding Tile Clock

An ESP32-based sliding tile clock that displays the time using stepper motor-driven tiles, synchronized via NTP.

Based on this Instructables project: https://www.instructables.com/Sliding-Tile-Clock/

## Features

- NTP time synchronization with automatic DST handling (Pacific Time)
- 12-hour or 24-hour display mode
- Web interface at http://st.local for setting dial positions and nudging alignment
- Serial command interface for manual motor control
- Randomized dial update order

## Setup

1. Copy `SlidingTileClock/credentials.example.h` to `SlidingTileClock/credentials.h`
2. Edit `credentials.h` with your WiFi SSID and password
3. Open `SlidingTileClock/SlidingTileClock.ino` in the Arduino IDE
4. Upload to your ESP32
