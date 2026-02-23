#include "FastILI9341.h"

FastILI9341::FastILI9341(int cs, int dc, int rst, int led)
  : PIN_CS(cs), PIN_DC(dc), PIN_RST(rst), PIN_LED(led) {}

void FastILI9341::setSPIFrequency(uint32_t spi_hz) {
  spi_cfg.frequency = spi_hz;
}

static inline uint16_t be16(uint16_t v) {
  return (uint16_t)((v << 8) | (v >> 8));
}

void FastILI9341::cmd(uint8_t c) {
  digitalWrite(PIN_DC, LOW);
  digitalWrite(PIN_CS, LOW);
  spi_buf b{ .buf = (void*)&c, .len = 1 };
  spi_buf_set s{ .buffers = &b, .count = 1 };
  (void)spi_write(spi_dev, &spi_cfg, &s);
  digitalWrite(PIN_CS, HIGH);
}

void FastILI9341::data(const uint8_t* d, size_t n) {
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_CS, LOW);
  spi_buf b{ .buf = (void*)d, .len = (uint32_t)n };
  spi_buf_set s{ .buffers = &b, .count = 1 };
  (void)spi_write(spi_dev, &spi_cfg, &s);
  digitalWrite(PIN_CS, HIGH);
}

void FastILI9341::streamBegin() {
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_CS, LOW);
}
void FastILI9341::streamEnd() {
  digitalWrite(PIN_CS, HIGH);
}

void FastILI9341::setWindow(int x0, int y0, int x1, int y1) {
  cmd(0x2A);
  uint16_t xd[2] = { be16((uint16_t)x0), be16((uint16_t)x1) };
  data((uint8_t*)xd, 4);

  cmd(0x2B);
  uint16_t yd[2] = { be16((uint16_t)y0), be16((uint16_t)y1) };
  data((uint8_t*)yd, 4);

  cmd(0x2C);
}

void FastILI9341::hwReset() {
  if (PIN_RST < 0) return;
  digitalWrite(PIN_RST, HIGH);
  delay(5);
  digitalWrite(PIN_RST, LOW);
  delay(20);
  digitalWrite(PIN_RST, HIGH);
  delay(120);
}

bool FastILI9341::begin(uint32_t spi_hz, uint8_t madctl) {
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  if (PIN_RST >= 0) pinMode(PIN_RST, OUTPUT);
  if (PIN_LED >= 0) {
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);
  }

  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_DC, HIGH);
  if (PIN_RST >= 0) digitalWrite(PIN_RST, HIGH);

  // UNO Q (Zephyr core): bierzemy spi2 jak wcześniej
  spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi2));
  if (!spi_dev || !device_is_ready(spi_dev)) return false;

  spi_cfg.frequency = spi_hz;
  spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
  spi_cfg.slave = 0;
  spi_cfg.cs = spi_cs_control{};

  hwReset();

  cmd(0x01);
  delay(150);  // SWRESET
  cmd(0x11);
  delay(120);  // SLPOUT

  cmd(0x3A);  // COLMOD
  {
    uint8_t col = 0x55;
    data(&col, 1);
  }
  delay(10);

  cmd(0x36);  // MADCTL
  {
    uint8_t m = madctl;
    data(&m, 1);
  }
  delay(10);

  cmd(0x29);
  delay(20);  // DISPON
  return true;
}

void FastILI9341::fillScreen565(uint16_t color565) {
  // wysyłamy “swapped” w strumieniu
  uint16_t c = bswap16(color565);

  // pasek 320 * 10
  static constexpr int STRIP_H = 10;
  static uint16_t strip[W * STRIP_H];

  for (int i = 0; i < W * STRIP_H; i++) strip[i] = c;

  for (int y = 0; y < H; y += STRIP_H) {
    int h = (y + STRIP_H <= H) ? STRIP_H : (H - y);
    setWindow(0, y, W - 1, y + h - 1);
    streamBegin();
    spi_buf b{ .buf = strip, .len = (uint32_t)(W * h * 2) };
    spi_buf_set s{ .buffers = &b, .count = 1 };
    (void)spi_write(spi_dev, &spi_cfg, &s);
    streamEnd();
  }
}

void FastILI9341::blit565(int x0, int y0, int w, int h, const uint16_t* pix) {
  if (w <= 0 || h <= 0) return;

  // ILI9341 chce big-endian. Konwertujemy do tymczasowego bufora “swapped”.
  // Bufor max: narzuca caller (ty w grze) poprzez wielkość dirty rect.
  static uint16_t tmp[120 * 80];  // dopasuj do MAX_RW*MAX_RH w grze
  const int n = w * h;
  for (int i = 0; i < n; i++) tmp[i] = bswap16(pix[i]);

  setWindow(x0, y0, x0 + w - 1, y0 + h - 1);
  streamBegin();
  spi_buf b{ .buf = tmp, .len = (uint32_t)(n * 2) };
  spi_buf_set s{ .buffers = &b, .count = 1 };
  (void)spi_write(spi_dev, &spi_cfg, &s);
  streamEnd();
}
