SKETCH := $(CURDIR)
SKETCH_NAME := UnoQ-Arkanoid

board ?= $(if $(BOARD),$(BOARD),unoq)
DEFAULT_PORT_unoq := /dev/ttyACM0
DEFAULT_PORT_esp32 := /dev/ttyUSB0
port ?= $(if $(PORT),$(PORT),$(DEFAULT_PORT_$(board)))
monitor_config ?= $(if $(MONITOR_CONFIG),$(MONITOR_CONFIG),115200)

ARDUINO_SKETCHBOOK ?= $(HOME)/Arduino
ARDUINO_CLI ?= $(shell command -v arduino-cli 2>/dev/null)

ifeq ($(strip $(ARDUINO_CLI)),)
ARDUINO_CLI := /opt/arduino-ide/resources/app/lib/backend/resources/arduino-cli
endif

SUPPORTED_BOARDS := unoq esp32

FQBN_unoq := arduino:zephyr:unoq
BOARD_OPTIONS_unoq := link_mode=dynamic,flash_mode=flash,wait_linux_boot=yes
SGF_HW_PRESET_unoq := SGF_HW_PRESET_UNOQ_ILI9341_320X240

FQBN_esp32 := esp32:esp32:esp32
BOARD_OPTIONS_esp32 := UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default
SGF_HW_PRESET_esp32 := SGF_HW_PRESET_ESP32_ST7789_240X240

FQBN := $(FQBN_$(board))
BOARD_OPTIONS := $(BOARD_OPTIONS_$(board))
SGF_HW_PRESET := $(SGF_HW_PRESET_$(board))
BUILD_DIR := $(CURDIR)/build/$(board)
STAGE_ROOT := $(CURDIR)/build-stage/$(board)
STAGE_DIR := $(STAGE_ROOT)/$(SKETCH_NAME)
STAGE_VENDOR_DIR := $(STAGE_DIR)/vendor

LOCAL_SOURCES := $(wildcard *.ino) $(wildcard *.cpp) $(wildcard *.h)
LIBRARY_DIRS := $(ARDUINO_SKETCHBOOK)/libraries
LIBRARY_FLAGS := $(foreach dir,$(LIBRARY_DIRS),--libraries $(dir))

BOARD_OPTION_FLAG :=
ifneq ($(strip $(BOARD_OPTIONS)),)
BOARD_OPTION_FLAG := --board-options $(BOARD_OPTIONS)
endif

MONITOR_CONFIG_FLAG :=
ifneq ($(strip $(monitor_config)),)
MONITOR_CONFIG_FLAG := --config "$(monitor_config)"
endif

ifeq ($(strip $(FQBN)),)
$(error Unsupported board '$(board)'. Supported values: $(SUPPORTED_BOARDS))
endif

ifeq ($(strip $(SGF_HW_PRESET)),)
$(error Missing SGF_HW_PRESET mapping for board '$(board)')
endif

.PHONY: help boards info stage build upload flash monitor clean

help:
	@printf '%s\n' \
	  'Targets:' \
	  '  make build board=unoq' \
	  '  make upload board=unoq' \
	  '  make flash board=unoq' \
	  '  make monitor board=unoq' \
	  '  make clean board=unoq' \
	  '' \
	  'Supported BOARD values:' \
	  '  unoq   -> $(FQBN_unoq), $(SGF_HW_PRESET_unoq)' \
	  '  esp32  -> $(FQBN_esp32), $(SGF_HW_PRESET_esp32)' \
	  '' \
	  'Default ports:' \
	  '  unoq   -> $(DEFAULT_PORT_unoq)' \
	  '  esp32  -> $(DEFAULT_PORT_esp32)' \
	  '' \
	  'Override with: make flash board=esp32 port=/dev/ttyUSB1'

boards:
	@printf '%s\n' \
	  'unoq:  fqbn=$(FQBN_unoq)  preset=$(SGF_HW_PRESET_unoq)' \
	  'esp32: fqbn=$(FQBN_esp32)  preset=$(SGF_HW_PRESET_esp32)'

info:
	@printf '%s\n' \
	  'SKETCH=$(SKETCH_NAME)' \
	  'board=$(board)' \
	  'port=$(port)' \
	  'FQBN=$(FQBN)' \
	  'BOARD_OPTIONS=$(BOARD_OPTIONS)' \
	  'SGF_HW_PRESET=$(SGF_HW_PRESET)' \
	  'BUILD_DIR=$(BUILD_DIR)' \
	  'STAGE_DIR=$(STAGE_DIR)' \
	  'ARDUINO_CLI=$(ARDUINO_CLI)'

stage:
	rm -rf "$(STAGE_ROOT)"
	mkdir -p "$(STAGE_VENDOR_DIR)"
	for file in $(LOCAL_SOURCES); do \
	  ln -s "$(CURDIR)/$$file" "$(STAGE_DIR)/$$file"; \
	done
	ln -s "$(CURDIR)/vendor/sgf-hardware-presets" \
	  "$(STAGE_VENDOR_DIR)/sgf-hardware-presets"

build: stage
	"$(ARDUINO_CLI)" compile \
	  --clean \
	  --warnings all \
	  --fqbn "$(FQBN)" \
	  $(BOARD_OPTION_FLAG) \
	  --build-path "$(BUILD_DIR)" \
	  $(LIBRARY_FLAGS) \
	  --build-property "build.extra_flags=-DSGF_HW_PRESET=$(SGF_HW_PRESET)" \
	  "$(STAGE_DIR)"

upload: build
	@test -n "$(port)" || { \
	  echo "port is required, e.g. make board=$(board) port=/dev/ttyACM0 upload" >&2; \
	  exit 1; \
	}
	"$(ARDUINO_CLI)" upload \
	  --fqbn "$(FQBN)" \
	  $(BOARD_OPTION_FLAG) \
	  --build-path "$(BUILD_DIR)" \
	  --port "$(port)" \
	  "$(STAGE_DIR)"

flash: upload

monitor:
	@test -n "$(port)" || { \
	  echo "port is required, e.g. make board=$(board) port=/dev/ttyACM0 monitor" >&2; \
	  exit 1; \
	}
	"$(ARDUINO_CLI)" monitor \
	  --fqbn "$(FQBN)" \
	  $(BOARD_OPTION_FLAG) \
	  --port "$(port)" \
	  $(MONITOR_CONFIG_FLAG)

clean:
	rm -rf "$(BUILD_DIR)" "$(STAGE_ROOT)"
