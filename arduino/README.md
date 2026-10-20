# Arduino IDE: BlackPill STM32F411 + ST7735S 80x160

Otwórz plik `BlackPill_ST7735_BMS/BlackPill_ST7735_BMS.ino` w Arduino IDE.

## Wymagane środowisko

1. W Menedżerze płytek zainstaluj pakiet **STM32 MCU based boards**
   (STM32duino).
2. Wybierz płytkę odpowiadającą **BlackPill STM32F411CEU6**.
3. Wgraj przez ST-Link albo przez używany przez siebie bootloader.

Program nie potrzebuje bibliotek Adafruit ani UTFT. Ma własną, programową
obsługę SPI i tę samą sekwencję startową LCD co działająca wersja bare-metal.

## Wybór panelu

Na początku pliku `.ino` jest jedna linia:

```cpp
#define LCD_PANEL LCD_PANEL_QSFX0961301
```

| Panel podłączony do BlackPill | Ustawienie |
|---|---|
| QSFX0961301 | `LCD_PANEL_QSFX0961301` |
| FXD096QQ08B-F | `LCD_PANEL_FXD096QQ08B_F` |

Piny są stałe: PA2=/CS, PA3=/RST, PA4=RS/DC, PA5=SCLK, PA6=SDIN/MOSI,
PA7=podświetlenie. Szczegóły i 13-pinowy pinout są w
[`../docs/wiring.md`](../docs/wiring.md).

Po wgraniu dioda PC13 miga raz na sekundę, a ekran pokazuje przykładowy
widok BMS 13S. Później zamiast tablicy `cells[]` podstawimy prawdziwe odczyty
z BMS.
