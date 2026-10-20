/*
 * BlackPill STM32F411CEU6 + ST7735S 80x160 (FXD096QQ08B-F / QSFX0961301)
 *
 * Arduino IDE + STM32duino.  No display libraries are required.
 *
 * Wiring:
 *   PA2  -> LCD /CS
 *   PA3  -> LCD /RST
 *   PA4  -> LCD RS / DC
 *   PA5  -> LCD SCLK
 *   PA6  -> LCD SDIN / MOSI
 *   PA7  -> LCD LEDA / BL+ (see docs/wiring.md)
 *   3V3  -> LCD VDD
 *   GND  -> LCD GND and LEDK / BL-
 *   PC13 -> BlackPill built-in LED (active LOW)
 */

#define LCD_PANEL_FXD096QQ08B_F 0
#define LCD_PANEL_QSFX0961301   1

/* Change only this line when the other display is connected. */
#define LCD_PANEL LCD_PANEL_QSFX0961301

#if LCD_PANEL == LCD_PANEL_QSFX0961301
  #define LCD_MADCTL        0x40
  #define LCD_INVERSION_CMD 0x21  // INVON: fixes QSFX colour inversion
#elif LCD_PANEL == LCD_PANEL_FXD096QQ08B_F
  #define LCD_MADCTL        0xC0
  #define LCD_INVERSION_CMD 0x20  // INVOFF
#else
  #error "Unknown LCD_PANEL"
#endif

static const uint8_t LCD_X_OFFSET = 26;
static const uint8_t LCD_Y_OFFSET = 1;

static const uint8_t LCD_CS   = PA2;
static const uint8_t LCD_RST  = PA3;
static const uint8_t LCD_DC   = PA4;
static const uint8_t LCD_SCLK = PA5;
static const uint8_t LCD_MOSI = PA6;
static const uint8_t LCD_BL   = PA7;
static const uint8_t LED      = PC13;

static void spiByte(uint8_t value) {
  for (int8_t bit = 7; bit >= 0; --bit) {
    digitalWrite(LCD_MOSI, (value & (1U << bit)) ? HIGH : LOW);
    digitalWrite(LCD_SCLK, HIGH);
    digitalWrite(LCD_SCLK, LOW);
  }
}

/* Keep CS low while sending one command and all its data bytes. */
static void lcdBegin(uint8_t command) {
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_DC, LOW);
  spiByte(command);
  digitalWrite(LCD_DC, HIGH);
}

static void lcdData(uint8_t value) { spiByte(value); }
static void lcdEnd(void) { digitalWrite(LCD_CS, HIGH); }

static void lcdCommand(uint8_t command) {
  lcdBegin(command);
  lcdEnd();
}

static void lcdInit(void) {
  digitalWrite(LCD_RST, LOW);
  delay(30);
  digitalWrite(LCD_RST, HIGH);
  delay(150);

  lcdCommand(0x01);  // SWRESET
  delay(150);
  lcdCommand(0x11);  // SLPOUT
  delay(150);

  lcdBegin(0xB1); lcdData(0x05); lcdData(0x3C); lcdData(0x3C); lcdEnd();
  lcdBegin(0xB2); lcdData(0x05); lcdData(0x3C); lcdData(0x3C); lcdEnd();
  lcdBegin(0xB3);
  lcdData(0x05); lcdData(0x3C); lcdData(0x3C);
  lcdData(0x05); lcdData(0x3C); lcdData(0x3C);
  lcdEnd();
  lcdBegin(0xB4); lcdData(0x03); lcdEnd();
  lcdBegin(0xC0); lcdData(0x28); lcdData(0x08); lcdData(0x04); lcdEnd();
  lcdBegin(0xC1); lcdData(0xC0); lcdEnd();
  lcdBegin(0xC2); lcdData(0x0D); lcdData(0x00); lcdEnd();
  lcdBegin(0xC3); lcdData(0x8D); lcdData(0x2A); lcdEnd();
  lcdBegin(0xC4); lcdData(0x8D); lcdData(0xEE); lcdEnd();
  lcdBegin(0xC5); lcdData(0x1A); lcdEnd();

  lcdBegin(0x3A); lcdData(0x05); lcdEnd();       // RGB565
  lcdBegin(0x36); lcdData(LCD_MADCTL); lcdEnd();
  lcdCommand(LCD_INVERSION_CMD);
  lcdCommand(0x13);                               // NORON
  lcdCommand(0x29);                               // DISPON
  delay(100);
}

static void lcdWindow(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  const uint8_t x0 = x + LCD_X_OFFSET;
  const uint8_t y0 = y + LCD_Y_OFFSET;
  const uint8_t x1 = x0 + width - 1;
  const uint8_t y1 = y0 + height - 1;

  lcdBegin(0x2A);
  lcdData(0); lcdData(x0); lcdData(0); lcdData(x1);
  lcdEnd();

  lcdBegin(0x2B);
  lcdData(0); lcdData(y0); lcdData(0); lcdData(y1);
  lcdEnd();
}

static void lcdColor(uint16_t color) {
  spiByte(color >> 8);
  spiByte(color & 0xFF);
}

static void lcdFill(uint16_t color) {
  lcdWindow(0, 0, 80, 160);
  lcdBegin(0x2C);                                 // RAMWR
  for (uint32_t i = 0; i < 80UL * 160UL; ++i) lcdColor(color);
  lcdEnd();
}

/* 5x7 font. One bit is one pixel; sixth column is spacing. */
static uint8_t fontColumn(char c, uint8_t column) {
  static const uint8_t digits[10][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}
  };
  static const uint8_t a[5] = {0x7E,0x11,0x11,0x11,0x7E};
  static const uint8_t b[5] = {0x7F,0x49,0x49,0x49,0x36};
  static const uint8_t c1[5] = {0x3E,0x41,0x41,0x41,0x22};
  static const uint8_t d[5] = {0x7F,0x41,0x41,0x22,0x1C};
  static const uint8_t e[5] = {0x7F,0x49,0x49,0x49,0x41};
  static const uint8_t g[5] = {0x3E,0x41,0x49,0x49,0x7A};
  static const uint8_t i[5] = {0x00,0x41,0x7F,0x41,0x00};
  static const uint8_t k[5] = {0x7F,0x08,0x14,0x22,0x41};
  static const uint8_t l[5] = {0x7F,0x40,0x40,0x40,0x40};
  static const uint8_t m[5] = {0x7F,0x02,0x0C,0x02,0x7F};
  static const uint8_t n[5] = {0x7F,0x04,0x08,0x10,0x7F};
  static const uint8_t o[5] = {0x3E,0x41,0x41,0x41,0x3E};
  static const uint8_t p[5] = {0x7F,0x09,0x09,0x09,0x06};
  static const uint8_t s[5] = {0x46,0x49,0x49,0x49,0x31};
  static const uint8_t v[5] = {0x1F,0x20,0x40,0x20,0x1F};
  const uint8_t *glyph = nullptr;

  if (column >= 5) return 0;
  if (c >= '0' && c <= '9') return digits[c - '0'][column];
  switch (c) {
    case 'A': glyph = a; break; case 'B': glyph = b; break;
    case 'C': glyph = c1; break; case 'D': glyph = d; break;
    case 'E': glyph = e; break; case 'G': glyph = g; break;
    case 'I': glyph = i; break; case 'K': glyph = k; break;
    case 'L': glyph = l; break; case 'M': glyph = m; break;
    case 'N': glyph = n; break; case 'O': glyph = o; break;
    case 'P': glyph = p; break; case 'S': glyph = s; break;
    case 'V': glyph = v; break;
    case ':': return (column == 2) ? 0x36 : 0;
    case '.': return (column == 2) ? 0x40 : 0;
    case '-': return (column == 2) ? 0x08 : 0;
    default: return 0;
  }
  return glyph[column];
}

static void drawChar(uint8_t x, uint8_t y, char c,
                     uint16_t foreground, uint16_t background, uint8_t scale) {
  lcdWindow(x, y, 6 * scale, 8 * scale);
  lcdBegin(0x2C);
  for (uint8_t row = 0; row < 8; ++row) {
    for (uint8_t sy = 0; sy < scale; ++sy) {
      for (uint8_t col = 0; col < 6; ++col) {
        const uint16_t color = (fontColumn(c, col) & (1U << row)) ? foreground : background;
        for (uint8_t sx = 0; sx < scale; ++sx) lcdColor(color);
      }
    }
  }
  lcdEnd();
}

static void drawText(uint8_t x, uint8_t y, const char *text,
                     uint16_t foreground, uint16_t background, uint8_t scale) {
  while (*text) {
    drawChar(x, y, *text++, foreground, background, scale);
    x += 6 * scale;
  }
}

static void drawCellLine(uint8_t y, uint8_t cell, uint16_t millivolts) {
  const uint16_t whole = millivolts / 1000;
  const uint16_t hundredths = (millivolts % 1000) / 10;
  char line[10] = {
    'C', (char)('0' + cell / 10), (char)('0' + cell % 10), ':',
    (char)('0' + whole), '.',
    (char)('0' + hundredths / 10), (char)('0' + hundredths % 10), 'V', 0
  };
  drawText(3, y, line, 0xFFFF, 0x0000, 1);
}

static void drawDashboard(void) {
  static const uint16_t cells[13] = {
    4110, 4100, 4150, 4110, 4100, 4120, 4080,
    4130, 4120, 4070, 4100, 4090, 4180
  };

  lcdFill(0x0000);
  drawText(2, 2, "BMS", 0x07FF, 0x0000, 2);
  drawText(42, 2, "13S", 0xFFE0, 0x0000, 2);
  for (uint8_t i = 0; i < 13; ++i) drawCellLine(22 + i * 8, i + 1, cells[i]);
  drawText(2, 132, "PACK:53.51V", 0x07E0, 0x0000, 1);
  drawText(2, 145, "I:0.54A", 0xFFFF, 0x0000, 1);
  drawText(55, 145, "ON", 0xF800, 0x0000, 1);
}

void setup(void) {
  pinMode(LCD_CS, OUTPUT);   pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_DC, OUTPUT);   pinMode(LCD_SCLK, OUTPUT);
  pinMode(LCD_MOSI, OUTPUT); pinMode(LCD_BL, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(LED, HIGH);       // BlackPill LED off
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_RST, HIGH);
  digitalWrite(LCD_SCLK, LOW);
  digitalWrite(LCD_MOSI, LOW);
  digitalWrite(LCD_BL, HIGH);    // backlight on

  digitalWrite(LED, LOW); delay(100); digitalWrite(LED, HIGH);
  lcdInit();
  drawDashboard();
}

void loop(void) {
  digitalWrite(LED, LOW);
  delay(100);
  digitalWrite(LED, HIGH);
  delay(900);
}
