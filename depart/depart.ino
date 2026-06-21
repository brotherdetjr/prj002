#include <Arduino_GFX_Library.h>
#include <sys/time.h>
#include <time.h>

// All available GFX fonts (copied from Arduino_GFX HelloWorldGfxfont example;
// more at https://github.com/moononournation/ArduinoFreeFontFile)
#include "font1.h"

static void log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static void log(const char *fmt, ...)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm t;
    localtime_r(&tv.tv_sec, &t);
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d.%03ld %s\n", t.tm_year + 1900,
                  t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                  tv.tv_usec / 1000, msg);
}

#define GFX_BL 46

// Display Pin config: Makerfabs/ESP32-S3_Parallel_TFT_3.16_ST7701S repo,
// PDQgraphicstest/Arduino_GFX_dev_device.h, #define ESP32_S3_RGB section.
// Init sequence: tl032fwv01_init_operations from Arduino_RGB_Display.h
// (library built-in for the 320x820 bar display), verbatim except
// register 0x3A changed 0x66→0x55: our parallel interface is wired
// 5+6+5 pins (RGB565, 16-bit); 0x66 would set 18-bit mode and drop
// the blue channel, making white appear yellow.
// clang-format off
static const uint8_t init_ops[] = {
    BEGIN_WRITE, WRITE_COMMAND_8, 0x11, END_WRITE, DELAY, 100,

    BEGIN_WRITE,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
    WRITE_C8_D8, 0xEF, 0x08,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x10,
    WRITE_C8_D16, 0xC0, 0xE5, 0x02,
    WRITE_C8_D16, 0xC1, 0x0C, 0x0A,
    WRITE_C8_D16, 0xC2, 0x07, 0x0F,
    WRITE_C8_D8, 0xC2, 0x02,
    WRITE_C8_D8, 0xCC, 0x10,
    WRITE_C8_D8, 0xCD, 0x08,
    WRITE_COMMAND_8, 0xB0, WRITE_BYTES, 16,
        0x00, 0x08, 0x51, 0x0D, 0xCE, 0x06, 0x00, 0x08, 0x08, 0x1D, 0x02, 0xD0, 0x0F, 0x6F, 0x36, 0x3F,
    WRITE_COMMAND_8, 0xB1, WRITE_BYTES, 16,
        0x00, 0x10, 0x4F, 0x0C, 0x11, 0x05, 0x00, 0x07, 0x07, 0x1F, 0x05, 0xD3, 0x11, 0x6E, 0x34, 0x3F,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x11,
    WRITE_C8_D8, 0xB0, 0x4D,
    WRITE_C8_D8, 0xB1, 0x1C,
    WRITE_C8_D8, 0xB2, 0x87,
    WRITE_C8_D8, 0xB3, 0x80,
    WRITE_C8_D8, 0xB5, 0x47,
    WRITE_C8_D8, 0xB7, 0x85,
    WRITE_C8_D8, 0xB8, 0x21,
    WRITE_C8_D8, 0xB9, 0x10,
    WRITE_C8_D8, 0xC1, 0x78,
    WRITE_C8_D8, 0xC2, 0x78,
    WRITE_C8_D8, 0xD0, 0x88,
    END_WRITE, DELAY, 100,

    BEGIN_WRITE,
    WRITE_COMMAND_8, 0xE0, WRITE_BYTES, 3, 0x80, 0x00, 0x02,
    WRITE_COMMAND_8, 0xE1, WRITE_BYTES, 11, 0x04, 0xA0, 0x00, 0x00, 0x05, 0xA0, 0x00, 0x00, 0x00, 0x60, 0x60,
    WRITE_COMMAND_8, 0xE2, WRITE_BYTES, 13, 0x30, 0x30, 0x60, 0x60, 0x3C, 0xA0, 0x00, 0x00, 0x3D, 0xA0, 0x00, 0x00, 0x00,
    WRITE_COMMAND_8, 0xE3, WRITE_BYTES, 4, 0x00, 0x00, 0x33, 0x33,
    WRITE_C8_D16, 0xE4, 0x44, 0x44,
    WRITE_COMMAND_8, 0xE5, WRITE_BYTES, 16, 0x06, 0x3E, 0xA0, 0xA0, 0x08, 0x40, 0xA0, 0xA0, 0x0A, 0x42, 0xA0, 0xA0, 0x0C, 0x44, 0xA0, 0xA0,
    WRITE_COMMAND_8, 0xE6, WRITE_BYTES, 4, 0x00, 0x00, 0x33, 0x33,
    WRITE_C8_D16, 0xE7, 0x44, 0x44,
    WRITE_COMMAND_8, 0xE8, WRITE_BYTES, 16, 0x07, 0x3F, 0xA0, 0xA0, 0x09, 0x41, 0xA0, 0xA0, 0x0B, 0x43, 0xA0, 0xA0, 0x0D, 0x45, 0xA0, 0xA0,
    WRITE_COMMAND_8, 0xEB, WRITE_BYTES, 7, 0x00, 0x01, 0x4E, 0x4E, 0xEE, 0x44, 0x00,
    WRITE_COMMAND_8, 0xED, WRITE_BYTES, 16, 0xFF, 0xFF, 0x04, 0x56, 0x72, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x27, 0x65, 0x40, 0xFF, 0xFF,
    WRITE_COMMAND_8, 0xEF, WRITE_BYTES, 6, 0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
    WRITE_C8_D16, 0xE8, 0x00, 0x0E,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x00,
    WRITE_COMMAND_8, 0x11,
    END_WRITE, DELAY, 120,

    BEGIN_WRITE,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
    WRITE_C8_D16, 0xE8, 0x00, 0x0C,
    END_WRITE, DELAY, 10,

    BEGIN_WRITE,
    WRITE_C8_D16, 0xE8, 0x00, 0x00,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x00,
    WRITE_C8_D8, 0x36, 0x00,
    WRITE_C8_D8, 0x3A, 0x55, /* 0x66 in library = 18-bit; 0x55 = 16-bit RGB565 */
    WRITE_COMMAND_8, 0x11,
    END_WRITE, DELAY, 120,

    BEGIN_WRITE, WRITE_COMMAND_8, 0x29, END_WRITE, DELAY, 120};
// clang-format on

Arduino_DataBus *bus =
    new Arduino_SWSPI(GFX_NOT_DEFINED /* DC */, 45 /* CS */, 39 /* SCK */,
                      40 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    7 /* DE */, 4 /* VSYNC */, 5 /* HSYNC */, 6 /* PCLK */, 12 /* R0 */,
    11 /* R1 */, 8 /* R2 */, 16 /* R3 */, 15 /* R4 */, 0 /* G0 */, 14 /* G1 */,
    10 /* G2 */, 9 /* G3 */, 3 /* G4 */, 13 /* G5 */, 48 /* B0 */, 47 /* B1 */,
    1 /* B2 */, 21 /* B3 */, 41 /* B4 */, 1 /* hsync_polarity */,
    10 /* hsync_front_porch */, 8 /* hsync_pulse_width */,
    50 /* hsync_back_porch */, 1 /* vsync_polarity */,
    10 /* vsync_front_porch */, 8 /* vsync_pulse_width */,
    20 /* vsync_back_porch */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    320 /* width */, 820 /* height */, rgbpanel, 1 /* rotation: landscape */,
    true /* auto_flush */, bus, GFX_NOT_DEFINED /* RST */, init_ops,
    sizeof(init_ops));

// Entry points

void setup()
{
    log("Starting application...");
    Serial.begin(115200);

    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, LOW); // NPN transistor: LOW = backlight on
    gfx->begin();
    gfx->fillScreen(RGB565_BLACK);

    for (int row = 0; row < 9; row++)
        for (int col = 0; col < 135; col++)
            gfx->fillRect(10 + col * 6, 62 * 0 + 21 + row * 6, 4, 4, RGB565(100, 33, 30));

    for (int row = 0; row < 9; row++)
        for (int col = 0; col < 135; col++)
            gfx->fillRect(10 + col * 6, 62 * 1 + 21 + row * 6, 4, 4, RGB565(100, 33, 30));

    for (int row = 0; row < 9; row++)
        for (int col = 0; col < 135; col++)
            gfx->fillRect(10 + col * 6, 62 * 2 + 21 + row * 6, 4, 4, RGB565(100, 33, 30));

    for (int row = 0; row < 9; row++)
        for (int col = 0; col < 135; col++)
            gfx->fillRect(10 + col * 6, 62 * 3 + 21 + row * 6, 4, 4, RGB565(100, 33, 30));

    for (int row = 0; row < 9; row++)
        for (int col = 0; col < 135; col++)
            gfx->fillRect(10 + col * 6, 62 * 4 + 21 + row * 6, 4, 4, RGB565(100, 33, 30));

    gfx->setFont(&Font1);
    gfx->setTextSize(1);
    gfx->setTextColor(RGB565_DARKORANGE);
    // y = baseline; cap tops land at y - 26
    gfx->setCursor(10, 62 * 1);
    gfx->print("14:28 London St Pan Exp 14:44");
    gfx->setCursor(10, 62 * 2);
    gfx->print("Calling at od Junction, East Cr");
    gfx->setCursor(10, 62 * 3);
    gfx->print("3rd 14:11 Moorgate     On time");
}

void loop()
{
    delay(1000);
    log("heartbeat");
}
