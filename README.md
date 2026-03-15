# UnoQ-Arkanoid

Arduino sketch build/upload workflow without Arduino IDE v2.

Edit sources in `vim` and use `make` for compile, flash, and serial monitor.

## Requirements

- `arduino-cli` installed, or the bundled CLI from Arduino IDE available at:
  `/opt/arduino-ide/resources/app/lib/backend/resources/arduino-cli`
- Arduino libraries installed in:
  `$(HOME)/Arduino/libraries`
- Board platforms installed in your Arduino CLI data dir

This repo currently supports:

- `board=unoq`
- `board=esp32`

## Commands

Build:

```bash
make build board=unoq
make build board=esp32
```

Flash:

```bash
make flash board=unoq
make flash board=esp32
```

Monitor:

```bash
make monitor board=unoq
make monitor board=esp32
```

Default ports:

- `board=unoq` -> `/dev/ttyACM0`
- `board=esp32` -> `/dev/ttyUSB0`

Override the port when needed:

```bash
make flash board=esp32 port=/dev/ttyUSB1
make monitor board=unoq port=/dev/ttyACM1
```

Default monitor config is `115200`. Override it if needed:

```bash
make monitor board=esp32 port=/dev/ttyUSB0 monitor_config=baudrate=115200
```

Other useful targets:

```bash
make boards
make info board=unoq
make clean board=unoq
```

## Board Selection

Board choice controls both:

- Arduino FQBN / board options
- `SGF_HW_PRESET` passed to the build

Current mapping:

- `board=unoq` -> `arduino:zephyr:unoq` + `SGF_HW_PRESET_UNOQ_ILI9341_320X240`
- `board=esp32` -> `esp32:esp32:esp32` + `SGF_HW_PRESET_ESP32_ST7789_240X240`

`SGF_HW_PRESET` is intentionally selected by the build system. The sketch does not silently choose a hardware target.

## Notes

The Makefile compiles from a temporary staging directory to avoid symlink loops in local `vendor/` links.

Arkanoid currently targets a `320x240` layout. `board=esp32` builds successfully, but the game layout is not yet adapted for `240x240`.
