/*
 * Makerfabs ESP32-S3 Parallel TFT 3.16" ST7701S
 * Demo: colored gradient image + short beep via MAX98357A I2S amp
 *
 * Pin definitions verified against hardware schematic v1.0
 *
 * Display (RGB parallel, SPI init):
 *   SPI CS=45, SCK=39, MOSI=40
 *   DE=7, VSYNC=4, HSYNC=5, PCLK=6
 *   R: 12,11,8,16,15  G: 0,14,10,9,3,13  B: 48,47,1,21,41
 *   Backlight: GPIO 46
 *
 * Audio MAX98357A (I2S):
 *   BCLK=44, LRCLK=43, DOUT=19
 */

#include <Arduino_GFX_Library.h>
#include "driver/i2s.h"

// GFX 1.6+ renamed color constants; restore short aliases
#define BLACK  RGB565_BLACK
#define WHITE  RGB565_WHITE
#define RED    RGB565_RED
#define GREEN  RGB565_GREEN
#define BLUE   RGB565_BLUE

// ── Display ──────────────────────────────────────────────────────────────────

#define GFX_BL 46

// Correct ST7701S init sequence for Makerfabs 3.16" panel.
// The library's built-in st7701_type9 targets a different panel; we override it here.
static const uint8_t mf_st7701_init[] = {
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77,0x01,0x00,0x00,0x13,
    WRITE_C8_D8, 0xEF, 0x08,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77,0x01,0x00,0x00,0x10,
    WRITE_C8_D16, 0xC0, 0xE5,0x02,
    WRITE_C8_D16, 0xC1, 0x15,0x0A,
    WRITE_C8_D16, 0xC2, 0x07,0x02,
    WRITE_C8_D8,  0xCC, 0x10,
    WRITE_COMMAND_8, 0xB0, WRITE_BYTES, 16,
        0x00,0x08,0x51,0x0D, 0xCE,0x06,0x00,0x08,
        0x08,0x24,0x05,0xD0, 0x0F,0x6F,0x36,0x1F,
    WRITE_COMMAND_8, 0xB1, WRITE_BYTES, 16,
        0x00,0x10,0x4F,0x0C, 0x11,0x05,0x00,0x07,
        0x07,0x18,0x02,0xD3, 0x11,0x6E,0x34,0x1F,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77,0x01,0x00,0x00,0x11,
    WRITE_C8_D8,  0xB0, 0x4D,
    WRITE_C8_D8,  0xB1, 0x37,
    WRITE_C8_D8,  0xB2, 0x87,
    WRITE_C8_D8,  0xB3, 0x80,
    WRITE_C8_D8,  0xB5, 0x4A,
    WRITE_C8_D8,  0xB7, 0x85,
    WRITE_C8_D8,  0xB8, 0x21,
    WRITE_C8_D16, 0xB9, 0x00,0x13,
    WRITE_C8_D8,  0xC0, 0x09,
    WRITE_C8_D8,  0xC1, 0x78,
    WRITE_C8_D8,  0xC2, 0x78,
    WRITE_C8_D8,  0xD0, 0x88,
    WRITE_COMMAND_8, 0xE0, WRITE_BYTES, 3, 0x80,0x00,0x02,
    WRITE_COMMAND_8, 0xE1, WRITE_BYTES, 11,
        0x0F,0xA0,0x00,0x00, 0x10,0xA0,0x00,0x00, 0x00,0x60,0x60,
    WRITE_COMMAND_8, 0xE2, WRITE_BYTES, 13,
        0x30,0x30,0x60,0x60, 0x45,0xA0,0x00,0x00,
        0x46,0xA0,0x00,0x00, 0x00,
    WRITE_COMMAND_8, 0xE3, WRITE_BYTES, 4, 0x00,0x00,0x33,0x33,
    WRITE_C8_D16, 0xE4, 0x44,0x44,
    WRITE_COMMAND_8, 0xE5, WRITE_BYTES, 16,
        0x0F,0x4A,0xA0,0xA0, 0x11,0x4A,0xA0,0xA0,
        0x13,0x4A,0xA0,0xA0, 0x15,0x4A,0xA0,0xA0,
    WRITE_COMMAND_8, 0xE6, WRITE_BYTES, 4, 0x00,0x00,0x33,0x33,
    WRITE_C8_D16, 0xE7, 0x44,0x44,
    WRITE_COMMAND_8, 0xE8, WRITE_BYTES, 16,
        0x10,0x4A,0xA0,0xA0, 0x12,0x4A,0xA0,0xA0,
        0x14,0x4A,0xA0,0xA0, 0x16,0x4A,0xA0,0xA0,
    WRITE_COMMAND_8, 0xEB, WRITE_BYTES, 7, 0x02,0x00,0x4E,0x4E,0xEE,0x44,0x00,
    WRITE_COMMAND_8, 0xED, WRITE_BYTES, 16,
        0xFF,0xFF,0x04,0x56, 0x72,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0x27, 0x65,0x40,0xFF,0xFF,
    WRITE_COMMAND_8, 0xEF, WRITE_BYTES, 6, 0x08,0x08,0x08,0x40,0x3F,0x64,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77,0x01,0x00,0x00,0x13,
    WRITE_C8_D16, 0xE8, 0x00,0x0E,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77,0x01,0x00,0x00,0x00,
    WRITE_COMMAND_8, 0x11,
    END_WRITE,
    DELAY, 120,
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77,0x01,0x00,0x00,0x13,
    WRITE_C8_D16, 0xE8, 0x00,0x0C,
    END_WRITE,
    DELAY, 10,
    BEGIN_WRITE,
    WRITE_C8_D16, 0xE8, 0x00,0x00,
    WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77,0x01,0x00,0x00,0x00,
    WRITE_C8_D8,  0x3A, 0x55,  // RGB565
    WRITE_C8_D8,  0x36, 0x00,
    WRITE_C8_D8,  0x35, 0x00,
    WRITE_COMMAND_8, 0x29,
    END_WRITE,
    DELAY, 20
};

Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC */, 45 /* CS */,
    39 /* SCK */, 40 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    7 /* DE */, 4 /* VSYNC */, 5 /* HSYNC */, 6 /* PCLK */,
    12 /* R0 */, 11 /* R1 */, 8  /* R2 */, 16 /* R3 */, 15 /* R4 */,
     0 /* G0 */, 14 /* G1 */, 10 /* G2 */,  9 /* G3 */,  3 /* G4 */, 13 /* G5 */,
    48 /* B0 */, 47 /* B1 */,  1 /* B2 */, 21 /* B3 */, 41 /* B4 */,
    1 /* hsync_polarity */,
    10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */,
    10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */,
    0 /* pclk_active_neg */, GFX_NOT_DEFINED /* prefer_speed */,
    false /* useBigEndian */, 0 /* de_idle_high */, 0 /* pclk_idle_high */,
    320 * 10 /* bounce_buffer_size_px: 10 lines in SRAM */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    320 /* width */, 820 /* height */, rgbpanel, 0 /* rotation */,
    true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */,
    mf_st7701_init, sizeof(mf_st7701_init));

// ── I2S / Audio ──────────────────────────────────────────────────────────────

#define I2S_BCLK   44
#define I2S_LRCLK  43
#define I2S_DOUT   19

#define SAMPLE_RATE   44100
#define BEEP_HZ       880      // A5 tone
#define BEEP_DURATION 300      // ms

static void i2s_init()
{
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
    };
    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);

    i2s_pin_config_t pins = {
        .bck_io_num   = I2S_BCLK,
        .ws_io_num    = I2S_LRCLK,
        .data_out_num = I2S_DOUT,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };
    i2s_set_pin(I2S_NUM_0, &pins);
}

static void beep(int freq_hz, int duration_ms)
{
    const int samples = SAMPLE_RATE * duration_ms / 1000;
    const float twopi_f_over_sr = 2.0f * 3.14159265f * freq_hz / SAMPLE_RATE;
    const int16_t amplitude = 16000;

    int16_t buf[256];
    int written = 0;
    size_t bytes_written;

    for (int i = 0; i < samples; i += 256) {
        int chunk = min(256, samples - i);
        for (int j = 0; j < chunk; j++) {
            int16_t s = (int16_t)(amplitude * sinf(twopi_f_over_sr * (i + j)));
            buf[j] = s; // left  channel
        }
        // Interleave L+R into one flat buffer for I2S stereo
        // (channel_format RIGHT_LEFT sends L in first slot)
        static int16_t stereo[512];
        for (int j = 0; j < chunk; j++) {
            stereo[2*j]   = buf[j]; // L
            stereo[2*j+1] = buf[j]; // R
        }
        i2s_write(I2S_NUM_0, stereo, chunk * 4, &bytes_written, portMAX_DELAY);
    }
    i2s_zero_dma_buffer(I2S_NUM_0);
}

// ── Helpers ──────────────────────────────────────────────────────────────────

// Quick HSV→RGB565 helper (S=1, V=1)
static uint16_t hue_to_565(float hue, float brightness)
{
    float h6 = hue / 60.0f;
    int   hi = (int)h6 % 6;
    float f  = h6 - (int)h6;
    float q  = 1.0f - f;
    float t  = f;

    uint8_t r, g, b;
    switch (hi) {
        case 0: r=255; g=(uint8_t)(t*255); b=0;          break;
        case 1: r=(uint8_t)(q*255); g=255; b=0;          break;
        case 2: r=0; g=255; b=(uint8_t)(t*255);          break;
        case 3: r=0; g=(uint8_t)(q*255); b=255;          break;
        case 4: r=(uint8_t)(t*255); g=0; b=255;          break;
        default: r=255; g=0; b=(uint8_t)(q*255);         break;
    }
    return gfx->color565(
        (uint8_t)(r * brightness),
        (uint8_t)(g * brightness),
        (uint8_t)(b * brightness));
}

// Draw rainbow gradient using fillRect horizontal bands.
// Each band is BAND_H pixels tall and a single solid hue,
// brightness varies in 8 vertical columns across the width.
static void draw_gradient()
{
    const int W      = gfx->width();   // 320
    const int H      = gfx->height();  // 820
    const int COLS   = 8;              // brightness steps left→right
    const int BAND_H = 4;             // rows per hue band

    for (int y = 0; y < H; y += BAND_H) {
        float hue = 360.0f * y / H;
        int band  = min(BAND_H, H - y);
        int col_w = W / COLS;

        for (int c = 0; c < COLS; c++) {
            float br    = (float)(c + 1) / COLS;
            uint16_t col = hue_to_565(hue, br);
            gfx->fillRect(c * col_w, y, col_w, band, col);
        }
    }
}

// Draw a centred text banner
static void draw_banner()
{
    const int W = gfx->width();
    const int H = gfx->height();

    // Semi-transparent black box
    gfx->fillRect(10, H/2 - 60, W - 20, 120, BLACK);

    gfx->setTextColor(WHITE);
    gfx->setTextSize(3);

    // Line 1
    const char *line1 = "Makerfabs";
    int16_t x1 = (W - strlen(line1) * 6 * 3) / 2;
    gfx->setCursor(x1, H/2 - 45);
    gfx->print(line1);

    // Line 2
    gfx->setTextSize(2);
    const char *line2 = "ESP32-S3 3.16\"";
    int16_t x2 = (W - strlen(line2) * 6 * 2) / 2;
    gfx->setCursor(x2, H/2 - 5);
    gfx->print(line2);

    // Line 3
    const char *line3 = "ST7701S RGB";
    int16_t x3 = (W - strlen(line3) * 6 * 2) / 2;
    gfx->setCursor(x3, H/2 + 25);
    gfx->print(line3);
}

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);
    Serial.println("Booting demo...");

    // Backlight: LOW = ON (NPN transistor, active-low enable)
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, LOW);

    if (!gfx->begin()) {
        Serial.println("ERROR: display init failed");
        while (1) delay(100);
    }
    Serial.println("Display OK");

    gfx->fillScreen(BLACK);

    // Draw gradient + banner
    Serial.println("Drawing gradient...");
    draw_gradient();
    draw_banner();
    Serial.println("Draw done");

    // Init I2S and beep
    i2s_init();
    Serial.println("Beep!");
    beep(BEEP_HZ, BEEP_DURATION);
    Serial.println("Setup complete");
}

void loop()
{
    // Nothing – image stays on screen
    delay(1000);
}
