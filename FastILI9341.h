#pragma once
#include <Arduino.h>

extern "C" {
  #include <zephyr/device.h>
  #include <zephyr/drivers/spi.h>
}

class FastILI9341 {
public:
  // piny: CS/DC/RST/LED (LED może być -1)
  FastILI9341(int cs, int dc, int rst, int led);

  bool begin(uint32_t spi_hz, uint8_t madctl);
  void setSPIFrequency(uint32_t spi_hz);

  int width()  const { return W; }
  int height() const { return H; }

  void fillScreen565(uint16_t color565); // color w normalnym RGB565 (nie-swapped)

  // Blit: wysyła bufor RGB565 (normalny endian) do prostokąta
  // bufor ma w*h pixeli, row-major
  void blit565(int x0, int y0, int w, int h, const uint16_t* pix);

  // Pomocnicze: konwersja RGB->565 + swap (do trzymania w buforze już “ready”)
  static inline uint16_t rgb565(uint8_t r,uint8_t g,uint8_t b){
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
  }
  static inline uint16_t bswap16(uint16_t v){ return (uint16_t)((v<<8)|(v>>8)); }

private:
  int PIN_CS, PIN_DC, PIN_RST, PIN_LED;
  static constexpr int W = 320;
  static constexpr int H = 240;

  const struct device* spi_dev = nullptr;
  struct spi_config spi_cfg{};

  void hwReset();
  void cmd(uint8_t c);
  void data(const uint8_t* d, size_t n);
  void setWindow(int x0,int y0,int x1,int y1);

  void streamBegin();
  void streamEnd();
};
