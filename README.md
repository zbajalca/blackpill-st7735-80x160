# ST7735 80x160 — BlackPill STM32F411

Prosty, bare-metalowy przykład dla BlackPill STM32F411CEU6 i paneli 0,96 cala
80x160 podłączanych przez programowe SPI.

Obsługiwane są dwa panele na tych samych przewodach:

| `LCD_PANEL` | Panel | `MADCTL` | Inwersja |
|---:|---|---:|---|
| `1` | `QSFX0961301` | `0x40` | `INVON` (`0x21`) |
| `0` | `FXD096QQ08B-F` | `0xC0` | `INVOFF` (`0x20`) |

## Połączenie

| LCD | BlackPill STM32F411 |
|---|---|
| CS | PA2 |
| RST | PA3 |
| DC / RS | PA4 |
| SCLK | PA5 |
| SDIN / MOSI | PA6 |
| Backlight | PA7 |
| VDD | 3,3 V |
| GND | GND |

Wbudowana dioda BlackPill jest na PC13 i jest aktywna stanem niskim.

## Budowanie

Wymagany jest `arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, `make` oraz do
wgrywania OpenOCD ze ST-Linkiem.

QSFX0961301 (domyślnie):

```sh
make clean
make LCD_PANEL=1
make flash LCD_PANEL=1
```

FXD096QQ08B-F:

```sh
make clean
make LCD_PANEL=0
make flash LCD_PANEL=0
```

Po uruchomieniu program pokazuje przykładowy ekran BMS 13S. W `main.c` są
również jasno opisane ustawienia obu paneli.

## Arduino IDE

Wersja gotowa do wgrania z Arduino IDE jest w katalogu
[`arduino/BlackPill_ST7735_BMS`](arduino/BlackPill_ST7735_BMS). Nie wymaga
bibliotek Adafruit: używa dokładnie tej samej sekwencji ST7735S i tych samych
profili paneli, które zostały sprawdzone w wersji bare-metal.

Pełny schemat połączeń 13-pinowego FPC jest w
[`docs/wiring.md`](docs/wiring.md), a jako rysunek w
[`docs/blackpill-st7735-wiring.svg`](docs/blackpill-st7735-wiring.svg).

## Ważne

Oba panele używają tego samego przesunięcia obszaru widzialnego: X=26, Y=1.
QSFX0961301 wymaga `INVON`; bez niego wszystkie kolory są negatywem
(biały staje się czarny, czerwony — turkusowy).
