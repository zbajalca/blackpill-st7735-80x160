#include <stdint.h>

/* BlackPill STM32F411CEU6, internal HSI = 16 MHz.
 * LCD 80x160, software SPI:
 * PA06 SDIN, PA05 SCLK, PA04 DC, PA03 RST, PA02 CS, PA07 BACKLIGHT.
 * On-board LED: PC13, active-low.
 *
 * Two verified panels use the same wiring but different controller settings:
 *   FXD096QQ08B-F : MADCTL=0xC0, INVOFF
 *   QSFX0961301   : MADCTL=0x40, INVON
 */

#define LCD_PANEL_FXD096QQ08B_F 0
#define LCD_PANEL_QSFX0961301   1

/* Change to LCD_PANEL_FXD096QQ08B_F when the FXD panel is connected. */
#ifndef LCD_PANEL
#define LCD_PANEL LCD_PANEL_QSFX0961301
#endif

#if LCD_PANEL == LCD_PANEL_QSFX0961301
#define LCD_MADCTL        0x40U
#define LCD_INVERSION_CMD 0x21U  /* INVON: QSFX colours are otherwise inverted. */
#define LCD_X_OFFSET      26U
#define LCD_Y_OFFSET      1U
#elif LCD_PANEL == LCD_PANEL_FXD096QQ08B_F
#define LCD_MADCTL        0xC0U
#define LCD_INVERSION_CMD 0x20U  /* INVOFF */
#define LCD_X_OFFSET      26U
#define LCD_Y_OFFSET      1U
#else
#error "Unknown LCD_PANEL"
#endif

#define RCC_AHB1ENR (*(volatile uint32_t *)0x40023830UL)

#define GPIOA_MODER (*(volatile uint32_t *)0x40020000UL)
#define GPIOA_BSRR  (*(volatile uint32_t *)0x40020018UL)
#define GPIOC_MODER (*(volatile uint32_t *)0x40020800UL)
#define GPIOC_BSRR  (*(volatile uint32_t *)0x40020818UL)

#define PIN_CS  2U
#define PIN_RST 3U
#define PIN_DC  4U
#define PIN_SCK 5U
#define PIN_MOSI 6U
#define PIN_BL  7U
#define PIN_LED 13U

#define BIT(pin) (1UL << (pin))

static void delay(volatile uint32_t n)
{
    while (n--) __asm__ volatile ("nop");
}

static void gpio_output_a(uint8_t pin)
{
    GPIOA_MODER = (GPIOA_MODER & ~(3UL << (pin * 2U))) | (1UL << (pin * 2U));
}

static void gpio_output_c(uint8_t pin)
{
    GPIOC_MODER = (GPIOC_MODER & ~(3UL << (pin * 2U))) | (1UL << (pin * 2U));
}

static inline void a_high(uint8_t pin) { GPIOA_BSRR = BIT(pin); }
static inline void a_low(uint8_t pin)  { GPIOA_BSRR = BIT(pin + 16U); }
static inline void led_on(void)  { GPIOC_BSRR = BIT(PIN_LED + 16U); }
static inline void led_off(void) { GPIOC_BSRR = BIT(PIN_LED); }

static void spi_byte(uint8_t value)
{
    for (int8_t bit = 7; bit >= 0; bit--) {
        if (value & (1U << bit)) a_high(PIN_MOSI); else a_low(PIN_MOSI);
        a_high(PIN_SCK);
        a_low(PIN_SCK);
    }
}

/* CS must stay low through the command and all its parameter bytes. */
static void lcd_begin(uint8_t command)
{
    a_low(PIN_CS);
    a_low(PIN_DC);
    spi_byte(command);
    a_high(PIN_DC);
}

static void lcd_data(uint8_t value) { spi_byte(value); }
static void lcd_end(void) { a_high(PIN_CS); }

static void lcd_command(uint8_t command)
{
    lcd_begin(command);
    lcd_end();
}

static void lcd_init(void)
{
    a_low(PIN_RST);
    delay(400000);
    a_high(PIN_RST);
    delay(2000000);

    lcd_command(0x01);             /* SWRESET */
    delay(2000000);
    lcd_command(0x11);             /* SLPOUT */
    delay(2000000);

    lcd_begin(0xB1); lcd_data(0x05); lcd_data(0x3C); lcd_data(0x3C); lcd_end();
    lcd_begin(0xB2); lcd_data(0x05); lcd_data(0x3C); lcd_data(0x3C); lcd_end();
    lcd_begin(0xB3);
    lcd_data(0x05); lcd_data(0x3C); lcd_data(0x3C);
    lcd_data(0x05); lcd_data(0x3C); lcd_data(0x3C);
    lcd_end();
    lcd_begin(0xB4); lcd_data(0x03); lcd_end();
    lcd_begin(0xC0); lcd_data(0x28); lcd_data(0x08); lcd_data(0x04); lcd_end();
    lcd_begin(0xC1); lcd_data(0xC0); lcd_end();
    lcd_begin(0xC2); lcd_data(0x0D); lcd_data(0x00); lcd_end();
    lcd_begin(0xC3); lcd_data(0x8D); lcd_data(0x2A); lcd_end();
    lcd_begin(0xC4); lcd_data(0x8D); lcd_data(0xEE); lcd_end();
    lcd_begin(0xC5); lcd_data(0x1A); lcd_end();

    lcd_begin(0x3A); lcd_data(0x05); lcd_end();       /* RGB565 */
    lcd_begin(0x36); lcd_data(LCD_MADCTL); lcd_end();
    lcd_command(LCD_INVERSION_CMD);
    lcd_command(0x13);                                 /* NORON */
    lcd_command(0x29);                                 /* DISPON */
    delay(500000);
}

static void lcd_window(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    uint8_t x0 = (uint8_t)(x + LCD_X_OFFSET);          /* panel offset */
    uint8_t y0 = (uint8_t)(y + LCD_Y_OFFSET);
    uint8_t x1 = (uint8_t)(x0 + width - 1U);
    uint8_t y1 = (uint8_t)(y0 + height - 1U);

    lcd_begin(0x2A);
    lcd_data(0); lcd_data(x0); lcd_data(0); lcd_data(x1);
    lcd_end();

    lcd_begin(0x2B);
    lcd_data(0); lcd_data(y0); lcd_data(0); lcd_data(y1);
    lcd_end();
}

static void lcd_color(uint16_t color)
{
    spi_byte((uint8_t)(color >> 8));
    spi_byte((uint8_t)color);
}

static void lcd_fill(uint16_t color)
{
    lcd_window(0, 0, 80, 160);
    lcd_begin(0x2C);                                   /* RAMWR */
    for (uint32_t i = 0; i < 12800U; i++) {
        lcd_color(color);
    }
    lcd_end();
}

/* Small 5 x 7 font.  One bit is one pixel; column 5 is the letter spacing. */
static uint8_t font_column(char c, uint8_t column)
{
    static const uint8_t digits[10][5] = {
        {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}
    };
    static const uint8_t a[5] = {0x7E,0x11,0x11,0x11,0x7E};
    static const uint8_t b[5] = {0x7F,0x49,0x49,0x49,0x36};
    static const uint8_t glyph_c[5] = {0x3E,0x41,0x41,0x41,0x22};
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
    const uint8_t *glyph = 0;

    if (column >= 5U) return 0;
    if (c >= '0' && c <= '9') return digits[(uint8_t)(c - '0')][column];
    switch (c) {
    case 'A': glyph = a; break; case 'B': glyph = b; break;
    case 'C': glyph = glyph_c; break; case 'D': glyph = d; break;
    case 'E': glyph = e; break; case 'G': glyph = g; break;
    case 'I': glyph = i; break; case 'K': glyph = k; break;
    case 'L': glyph = l; break; case 'M': glyph = m; break;
    case 'N': glyph = n; break; case 'O': glyph = o; break;
    case 'P': glyph = p; break; case 'S': glyph = s; break;
    case 'V': glyph = v; break;
    case ':': return (column == 2U) ? 0x36 : 0;
    case '.': return (column == 2U) ? 0x40 : 0;
    case '-': return (column == 2U) ? 0x08 : 0;
    default: return 0;
    }
    return glyph[column];
}

static void lcd_char(uint8_t x, uint8_t y, char c,
                     uint16_t foreground, uint16_t background, uint8_t scale)
{
    uint8_t width = (uint8_t)(6U * scale);
    uint8_t height = (uint8_t)(8U * scale);
    lcd_window(x, y, width, height);
    lcd_begin(0x2C);
    for (uint8_t row = 0; row < 8U; row++) {
        for (uint8_t sy = 0; sy < scale; sy++) {
            for (uint8_t col = 0; col < 6U; col++) {
                uint16_t color = (font_column(c, col) & (1U << row)) ? foreground : background;
                for (uint8_t sx = 0; sx < scale; sx++) lcd_color(color);
            }
        }
    }
    lcd_end();
}

static void lcd_text(uint8_t x, uint8_t y, const char *text,
                     uint16_t foreground, uint16_t background, uint8_t scale)
{
    while (*text) {
        lcd_char(x, y, *text++, foreground, background, scale);
        x = (uint8_t)(x + 6U * scale);
    }
}

static void lcd_cell_line(uint8_t y, uint8_t number, uint16_t millivolts)
{
    char line[9] = {'C','0',':','0','.','0','0','V',0};
    uint16_t whole = (uint16_t)(millivolts / 1000U);
    uint16_t hundredths = (uint16_t)((millivolts % 1000U) / 10U);
    line[1] = (number >= 10U) ? '1' : (char)('0' + number);
    line[2] = (number >= 10U) ? (char)('0' + number - 10U) : ':';
    line[3] = (number >= 10U) ? ':' : (char)('0' + whole);
    if (number >= 10U) {
        line[4] = (char)('0' + whole);
        line[5] = '.';
        line[6] = (char)('0' + hundredths / 10U);
        line[7] = (char)('0' + hundredths % 10U);
        line[8] = 0;
    } else {
        line[4] = '.';
        line[5] = (char)('0' + hundredths / 10U);
        line[6] = (char)('0' + hundredths % 10U);
        line[7] = 'V';
    }
    lcd_text(3, y, line, 0xFFFF, 0x0000, 1);
}

static void lcd_dashboard(void)
{
    static const uint16_t cells[13] = {
        4110, 4100, 4150, 4110, 4100, 4120, 4080,
        4130, 4120, 4070, 4100, 4090, 4180
    };

    lcd_fill(0x0000);                                 /* black */
    lcd_text(2, 2, "BMS", 0x07FF, 0x0000, 2);          /* cyan */
    lcd_text(42, 2, "13S", 0xFFE0, 0x0000, 2);         /* yellow */
    for (uint8_t i = 0; i < 13U; i++) lcd_cell_line((uint8_t)(22U + i * 8U), (uint8_t)(i + 1U), cells[i]);
    lcd_text(2, 130, "PACK:53.51V", 0x07E0, 0x0000, 1);
    lcd_text(2, 142, "I:0.54A", 0xFFFF, 0x0000, 1);
    lcd_text(53, 142, "ON", 0xF800, 0x0000, 1);
}

int main(void)
{
    RCC_AHB1ENR |= BIT(0) | BIT(2);                     /* GPIOA + GPIOC */

    gpio_output_a(PIN_CS); gpio_output_a(PIN_RST); gpio_output_a(PIN_DC);
    gpio_output_a(PIN_SCK); gpio_output_a(PIN_MOSI); gpio_output_a(PIN_BL);
    gpio_output_c(PIN_LED);

    led_off();
    a_high(PIN_BL);                                     /* backlight ON */
    a_high(PIN_CS); a_high(PIN_DC); a_high(PIN_RST);
    a_low(PIN_SCK); a_low(PIN_MOSI);

    led_on(); delay(300000); led_off();                 /* immediate proof */
    lcd_init();
    lcd_dashboard();                                    /* example BMS values */

    while (1) {
        led_on();  delay(1200000);
        led_off(); delay(1200000);
    }
}
