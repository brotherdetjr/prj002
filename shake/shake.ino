/*
 * shake.ino — Accelerometer data logger for shake-pattern analysis.
 *
 * No WiFi, no departure board.  Cycles:
 *   CAL (5 s, hold still) → SHAKE (2 s) → REST (5 s) → SHAKE → REST → …
 *
 * Serial output at ~125 Hz:
 *   YYYY-MM-DD HH:mm:SS.sss DATA <phase> <ax> <ay> <az> <gx> <gy> <gz> <dx> <dy> <dz>
 *   YYYY-MM-DD HH:mm:SS.sss PROMPT <event> cycle=N
 *
 * Upload + capture:  ./upload_shake.sh
 */

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <time.h>
#include <sys/time.h>

#define BLACK   RGB565_BLACK
#define WHITE   RGB565_WHITE
#define YELLOW  RGB565_YELLOW
#define GREEN   RGB565_GREEN
#define RED     RGB565_RED

// ── Display ───────────────────────────────────────────────────────────────────

#define GFX_BL 46

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
    WRITE_C8_D8,  0x3A, 0x55,
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
    320 * 10 /* bounce_buffer_size_px */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    320 /* width */, 820 /* height */, rgbpanel, 1 /* rotation: landscape */,
    false /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */,
    mf_st7701_init, sizeof(mf_st7701_init));

// ── Phase state machine type (declared early so Arduino auto-prototypes compile)

enum Phase { CAL, SHAKE_P, REST_P };

// ── IMU (QMI8658) ─────────────────────────────────────────────────────────────

#define IMU_SDA  17
#define IMU_SCL  18
#define IMU_ADDR 0x6B

static bool  imu_ok                  = false;
static bool  imu_gravity_initialized = false;
static float gravityX = 0.0f, gravityY = 0.0f, gravityZ = 0.0f;
static const float kAlpha = 0.98f;

static bool imu_write_reg(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool imu_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom((uint8_t)IMU_ADDR, len);
    for (uint8_t i = 0; i < len; i++) {
        if (!Wire.available()) return false;
        buf[i] = Wire.read();
    }
    return true;
}

static void imu_init()
{
    Wire.begin(IMU_SDA, IMU_SCL);
    delay(50);
    uint8_t who = 0;
    imu_read_regs(0x00, &who, 1);
    if (who != 0x05) {
        Serial.printf("IMU: WHO_AM_I=0x%02X (expected 0x05)\n", who);
        return;
    }
    imu_write_reg(0x03, 0x26);  // CTRL2: ACC ±8 g, 125 Hz ODR
    imu_write_reg(0x08, 0x01);  // CTRL7: ACC enable
    imu_ok = true;
    Serial.println("IMU OK");
}

// ── Timestamp ─────────────────────────────────────────────────────────────────

static void clock_init()
{
    // Seed system clock from compile-time macros.  No NTP in this sketch;
    // timestamps will be correct to within the compile→upload gap (~seconds).
    struct tm t = {};
    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon[4] = {};
    int day, year;
    sscanf(__DATE__, "%3s %d %d", mon, &day, &year);
    for (int i = 0; i < 12; i++)
        if (strncmp(mon, months + i * 3, 3) == 0) { t.tm_mon = i; break; }
    t.tm_mday = day;
    t.tm_year = year - 1900;
    sscanf(__TIME__, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);
    t.tm_isdst = -1;
    time_t base = mktime(&t);
    struct timeval tv = { base, 0 };
    settimeofday(&tv, nullptr);
}

static void ts_fill(char *buf, size_t n)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm t;
    localtime_r(&tv.tv_sec, &t);
    snprintf(buf, n, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec / 1000);
}

// ── Phase state machine ───────────────────────────────────────────────────────

#define CAL_MS   5000UL   // hold still — lets gravity filter converge
#define SHAKE_MS 2000UL
#define REST_MS  5000UL

static Phase    phase        = CAL;
static uint32_t phase_end_ms = 0;
static uint32_t cycle_n      = 0;

static const char *phase_tag()
{
    switch (phase) {
    case CAL:     return "CAL";
    case SHAKE_P: return "SHAKE";
    case REST_P:  return "REST";
    }
    return "?";
}

// ── LCD ───────────────────────────────────────────────────────────────────────

// textSize N → char cell N*6 px wide × N*8 px tall.
// Display: 820 px wide × 320 px tall (landscape).

static uint32_t lcd_next_ms = 0;

static void lcd_draw(const char *label, uint16_t color, uint32_t rem_ms, uint32_t cn)
{
    const int W = gfx->width();   // 820
    const int H = gfx->height();  // 320
    gfx->fillScreen(BLACK);

    // Phase name — textSize 6 → 36 px/char wide, 48 px/char tall
    gfx->setTextSize(6);
    gfx->setTextColor(color);
    int tw = (int)strlen(label) * 36;
    gfx->setCursor((W - tw) / 2, 50);
    gfx->print(label);

    // Countdown — textSize 4 → 24 px/char wide, 32 px/char tall
    char cbuf[12];
    snprintf(cbuf, sizeof(cbuf), "%.1f s", rem_ms / 1000.0f);
    gfx->setTextSize(4);
    gfx->setTextColor(WHITE);
    int cw = (int)strlen(cbuf) * 24;
    gfx->setCursor((W - cw) / 2, 170);
    gfx->print(cbuf);

    // Cycle counter — textSize 3 → 18 px/char wide, 24 px/char tall
    char nbuf[20];
    snprintf(nbuf, sizeof(nbuf), "cycle %lu", cn);
    gfx->setTextSize(3);
    gfx->setTextColor(YELLOW);
    gfx->setCursor(10, 280);
    gfx->print(nbuf);

    gfx->flush(true);
}

static void lcd_update(bool force = false)
{
    uint32_t now = millis();
    if (!force && now < lcd_next_ms) return;
    lcd_next_ms = now + 500;

    uint32_t rem = (phase_end_ms > now) ? phase_end_ms - now : 0;
    switch (phase) {
    case CAL:     lcd_draw("CALIBRATING", YELLOW, rem, cycle_n); break;
    case SHAKE_P: lcd_draw("SHAKE!",     RED,    rem, cycle_n); break;
    case REST_P:  lcd_draw("REST/MOVE",  GREEN,  rem, cycle_n); break;
    }
}

// ── Phase transitions ─────────────────────────────────────────────────────────

static void enter_phase(Phase p)
{
    phase = p;
    uint32_t now = millis();
    char ts[32];
    ts_fill(ts, sizeof(ts));

    switch (p) {
    case CAL:
        phase_end_ms = now + CAL_MS;
        Serial.printf("%s PROMPT cal_start cycle=0\n", ts);
        break;
    case SHAKE_P:
        phase_end_ms = now + SHAKE_MS;
        Serial.printf("%s PROMPT shake_start cycle=%lu\n", ts, cycle_n);
        break;
    case REST_P:
        phase_end_ms = now + REST_MS;
        Serial.printf("%s PROMPT rest_start cycle=%lu\n", ts, cycle_n);
        break;
    }
    lcd_next_ms = 0;  // force immediate LCD update
    lcd_update(true);
}

// ── Setup & loop ──────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);
    clock_init();
    imu_init();

    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, LOW);  // LOW = ON (NPN transistor)
    if (!gfx->begin()) {
        Serial.println("ERROR: display init failed");
        while (1) delay(100);
    }
    gfx->fillScreen(BLACK);

    char ts[32];
    ts_fill(ts, sizeof(ts));
    Serial.printf("%s HEADER time type phase ax ay az gx gy gz dx dy dz\n", ts);

    enter_phase(CAL);
}

void loop()
{
    uint32_t now = millis();

    // Read IMU and emit one DATA line
    uint8_t buf[6];
    if (imu_ok && imu_read_regs(0x35, buf, 6)) {
        int16_t rx = (int16_t)((uint16_t)buf[1] << 8 | buf[0]);
        int16_t ry = (int16_t)((uint16_t)buf[3] << 8 | buf[2]);
        int16_t rz = (int16_t)((uint16_t)buf[5] << 8 | buf[4]);
        const float scale = 8.0f / 32768.0f;  // ±8 g, 16-bit
        float ax = rx * scale, ay = ry * scale, az = rz * scale;

        if (!imu_gravity_initialized) {
            gravityX = ax; gravityY = ay; gravityZ = az;
            imu_gravity_initialized = true;
        } else {
            gravityX = kAlpha * gravityX + (1.0f - kAlpha) * ax;
            gravityY = kAlpha * gravityY + (1.0f - kAlpha) * ay;
            gravityZ = kAlpha * gravityZ + (1.0f - kAlpha) * az;
        }

        float dx = ax - gravityX;
        float dy = ay - gravityY;
        float dz = az - gravityZ;

        char ts[32];
        ts_fill(ts, sizeof(ts));
        // columns: ax ay az  gx gy gz  dx dy dz
        Serial.printf("%s DATA %s %7.3f %7.3f %7.3f  %7.3f %7.3f %7.3f  %7.3f %7.3f %7.3f\n",
                      ts, phase_tag(), ax, ay, az, gravityX, gravityY, gravityZ, dx, dy, dz);
    }

    // Phase transitions
    if (now >= phase_end_ms) {
        switch (phase) {
        case CAL:
            cycle_n = 1;
            enter_phase(SHAKE_P);
            break;
        case SHAKE_P:
            enter_phase(REST_P);
            break;
        case REST_P:
            cycle_n++;
            enter_phase(SHAKE_P);
            break;
        }
    }

    lcd_update();
    delay(8);  // ~125 Hz, matching QMI8658 ODR
}
