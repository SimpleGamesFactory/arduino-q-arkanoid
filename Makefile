SKETCH_NAME := UnoQ-Arkanoid

DEFAULT_PORT_unoq := /dev/ttyACM0
DEFAULT_PORT_esp32 := /dev/ttyUSB0

SUPPORTED_BOARDS := unoq esp32

FQBN_unoq := arduino:zephyr:unoq
BOARD_OPTIONS_unoq := link_mode=dynamic,flash_mode=flash,wait_linux_boot=yes
SGF_HW_PRESET_unoq := SGF_HW_PRESET_UNOQ_ILI9341_320X240

FQBN_esp32 := esp32:esp32:esp32
BOARD_OPTIONS_esp32 := UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default
SGF_HW_PRESET_esp32 := SGF_HW_PRESET_ESP32_ST7789_240X240

STAGE_LINKS := vendor/sgf-hardware-presets

include vendor/SGF/tools/sgf-arduino.mk
