/*
 * Makerfabs ESP32-S3 Parallel TFT 3.16" ST7701S
 * Railway departure board — live via national-rail-api.davwheat.dev
 *
 * Libraries required (Arduino Library Manager):
 *   - Arduino_GFX_Library  (moononournation)
 *   - ArduinoJson          (bblanchon) v7+
 *   - QMI8658              (Makerfabs or compatible)
 */

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <WebServer.h>
#include <time.h>
#include <sys/time.h>

#define BLACK   RGB565_BLACK
#define WHITE   RGB565_WHITE
#define YELLOW  RGB565_YELLOW
#define GREEN   RGB565_GREEN
#define RED     RGB565_RED
#define NAVY    RGB565_NAVY

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
    820 * 10 /* bounce_buffer_size_px */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    320 /* width */, 820 /* height */, rgbpanel, 1 /* rotation: landscape */,
    false /* auto_flush: we call flush(true) manually after each screen update */,
    bus, GFX_NOT_DEFINED /* RST */,
    mf_st7701_init, sizeof(mf_st7701_init));

// ── Screen log ───────────────────────────────────────────────────────────────

#define X_TIME 10  // left margin (px); shared with departure board layout

static int slog_y = 0;

static void slog(const char *msg, uint16_t color = WHITE)
{
    gfx->setTextSize(3);
    gfx->setTextColor(color);
    gfx->setCursor(X_TIME, slog_y);
    gfx->print(msg);
    slog_y += 25;
}

static bool serial_dbg_anchored = false;
static void serial_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// ── Config (persisted to NVS via Preferences) ────────────────────────────────

#define REFRESH_MS 60000UL

static char cfg_ssid[64]   = "";
static char cfg_pass[64]   = "";
static char cfg_station[8] = "MZH";
static char api_url[72]    = "";
static unsigned long imu_ready_at_ms = 0;

// RTC_NOINIT_ATTR: startup code never re-initialises this section, so values
// survive esp_restart() (unlike RTC_DATA_ATTR which is re-copied from flash
// on every non-deep-sleep boot).  Magic number guards against stale data on
// a true power-on reset.
#define RTC_SAVE_MAGIC 0xCAFED00D
RTC_NOINIT_ATTR static uint32_t rtc_magic;
RTC_NOINIT_ATTR static char     rtc_ssid[64];
RTC_NOINIT_ATTR static char     rtc_pass[64];
RTC_NOINIT_ATTR static char     rtc_station[8];

static void load_config()
{
    Preferences prefs;
    bool ok = prefs.begin("depart", true);
    Serial.printf("Prefs load: begin=%d\n", ok);
    if (ok) {
        prefs.getString("ssid",    cfg_ssid,    sizeof(cfg_ssid));
        prefs.getString("pass",    cfg_pass,    sizeof(cfg_pass));
        prefs.getString("station", cfg_station, sizeof(cfg_station));
        prefs.end();
    }
    Serial.printf("Prefs load: ssid='%s' station='%s'\n", cfg_ssid, cfg_station);
    snprintf(api_url, sizeof(api_url),
        "https://national-rail-api.davwheat.dev/departures/%s", cfg_station);
}

static bool nvs_open_rw(Preferences &prefs)
{
    if (prefs.begin("depart", false)) return true;
    // NVS is corrupted (likely from a crash-interrupted write).  Erase the
    // whole partition and reinitialise before retrying.
    Serial.println("NVS open failed — erasing partition and retrying");
    nvs_flash_erase();
    nvs_flash_init();
    return prefs.begin("depart", false);
}

static void save_config()
{
    Preferences prefs;
    if (!nvs_open_rw(prefs)) {
        Serial.println("Prefs save: failed to open namespace");
        return;
    }
    prefs.putString("ssid",    cfg_ssid);
    prefs.putString("pass",    cfg_pass);
    prefs.putString("station", cfg_station);
    prefs.end();
    Serial.printf("Prefs save: ssid='%s' station='%s'\n", cfg_ssid, cfg_station);
}

// ── Departure data ────────────────────────────────────────────────────────────

// Landscape: 820 px wide × 320 px tall.
// textSize(3): 18 px/char wide, 24 px/char tall.
// TIME and DEST are left-aligned; PLAT and STATUS are right-aligned to R_PLAT / R_STATUS.
//   TIME    left  x=10   (5 ch × 18 =  90 px)
//   DEST    left  x=116  (up to ~37 ch before PLAT, truncated in struct)
//   PLAT    right r=620
//   STATUS  right r=810  (820 - 10 margin)
#define X_DEST   116
#define R_PLAT   620   // right edge of PLAT column
#define R_STATUS 810   // right edge of STATUS column

#define CHAR_W  18  // textSize(3): 6 px × 3

#define HDR_H  52   // header section height (px)
#define ROW_H  38   // height of each table row (px)

#define MAX_DEPARTURES 5

struct Departure {
    char time[6];         // "HH:MM"
    char destination[28]; // station name, truncated to fit
    char platform[5];     // "1", "2", "-", "BUS"
    char status[12];      // "On time", "Delayed", "Cancelled", "Exp HH:MM"
};

static Departure board[MAX_DEPARTURES];
static int N = 0;
static char station_name[32] = "";

// ── State machine ─────────────────────────────────────────────────────────────

enum State { MANDATORY_CONFIG, WORKING, OPTIONAL_CONFIG };
static State state = MANDATORY_CONFIG;

// ── IMU (QMI8658) ─────────────────────────────────────────────────────────────

#define IMU_SDA  17
#define IMU_SCL  18
#define IMU_ADDR 0x6B

#define SHAKE_ENERGY_ALPHA    0.80f   // EMA alpha for motion magnitude smoothing
#define SHAKE_ENERGY_ENTER    8.0f   // enter shaking when smoothed energy exceeds this
#define SHAKE_ENERGY_EXIT     3.5f   // exit shaking when energy drops below this
#define SHAKE_REVERSAL_THRESH 2.0f   // dominant-axis threshold for sign reversal
#define SHAKE_COUNT_ALPHA     0.92f  // per-sample decay factor for reversal counter
#define SHAKE_COUNT_ENTER     2.5f   // min reversal count to confirm shaking
#define SHAKE_COOLDOWN_MS     3000UL
#define OPTIONAL_CONFIG_TIMEOUT_MS (5 * 60 * 1000UL)
#define WIFI_AP_SID "DepartBoard"
#define WIFI_AP_PASS "t123"

static bool          imu_ok                    = false;
static bool          imu_gravity_initialized   = false;
static unsigned long optional_config_enter_ms  = 0;

static float         gravityX = 0.0f, gravityY = 0.0f, gravityZ = 0.0f;
static const float   kAlpha   = 0.95f;
static unsigned long shake_cooldown_ms = 0;
static float         dbg_energy        = 0.0f;
static float         dbg_count         = 0.0f;
static float         dbg_ax = 0, dbg_ay = 0, dbg_az = 0, dbg_dx = 0, dbg_dy = 0, dbg_dz = 0;
static float         shakeEnergy       = 0.0f;
static float         shakeCount        = 0.0f;
static float         prevDominant      = 0.0f;
static bool          shaking           = false;

static bool imu_write_reg(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

// IRAM_ATTR: called from loop() while WiFi may have flash cache disabled
static bool IRAM_ATTR imu_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
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
    delay(50);  // let IMU power rail stabilise
    uint8_t who = 0;
    imu_read_regs(0x00, &who, 1);
    if (who != 0x05) {
        Serial.printf("IMU: WHO_AM_I=0x%02X (expected 0x05)\n", who);
        return;
    }
    imu_write_reg(0x03, 0x26);  // CTRL2: ACC ±4 g, 125 Hz (bits[6:4]=001)
    imu_write_reg(0x08, 0x01);  // CTRL7: ACC enable
    imu_ok = true;
    Serial.println("IMU OK");
}

static bool IRAM_ATTR check_shake()
{
    if (!imu_ok) return false;
    if (millis() < imu_ready_at_ms) return false;

    uint8_t buf[6];
    if (!imu_read_regs(0x35, buf, 6)) return false;

    int16_t rx = (int16_t)((uint16_t)buf[1] << 8 | buf[0]);
    int16_t ry = (int16_t)((uint16_t)buf[3] << 8 | buf[2]);
    int16_t rz = (int16_t)((uint16_t)buf[5] << 8 | buf[4]);
    const float scale = 4.0f / 32768.0f;
    float ax = rx * scale, ay = ry * scale, az = rz * scale;

    if (!imu_gravity_initialized) {
        gravityX = ax; gravityY = ay; gravityZ = az;
        imu_gravity_initialized = true;
        return false;
    }

    // Slower LPF keeps shake energy in dx/dy/dz rather than absorbing it into gravity
    gravityX = kAlpha * gravityX + (1.0f - kAlpha) * ax;
    gravityY = kAlpha * gravityY + (1.0f - kAlpha) * ay;
    gravityZ = kAlpha * gravityZ + (1.0f - kAlpha) * az;

    float dx = ax - gravityX;
    float dy = ay - gravityY;
    float dz = az - gravityZ;
    dbg_ax = ax; dbg_ay = ay; dbg_az = az; dbg_dx = dx; dbg_dy = dy; dbg_dz = dz;

    // Smoothed L1 motion magnitude
    float motion = fabsf(dx) + fabsf(dy) + fabsf(dz);
    shakeEnergy = shakeEnergy * SHAKE_ENERGY_ALPHA + motion * (1.0f - SHAKE_ENERGY_ALPHA);

    // Dominant axis: whichever of dx/dy/dz has the largest absolute value
    float axAbs = fabsf(dx), ayAbs = fabsf(dy), azAbs = fabsf(dz);
    float dominant;
    if (axAbs > ayAbs && axAbs > azAbs)  dominant = dx;
    else if (ayAbs > azAbs)               dominant = dy;
    else                                   dominant = dz;

    // Increment counter on sign reversal; decay every sample
    if ((dominant >  SHAKE_REVERSAL_THRESH && prevDominant < -SHAKE_REVERSAL_THRESH) ||
        (dominant < -SHAKE_REVERSAL_THRESH && prevDominant >  SHAKE_REVERSAL_THRESH))
    {
        shakeCount += 1.0f;
    }
    prevDominant = dominant;
    shakeCount  *= SHAKE_COUNT_ALPHA;

    dbg_energy = shakeEnergy;
    dbg_count  = shakeCount;

    uint32_t now = millis();
    if (now < shake_cooldown_ms) return false;

    if (!shaking) {
        if (shakeEnergy > SHAKE_ENERGY_ENTER && shakeCount > SHAKE_COUNT_ENTER) {
            shaking           = true;
            shake_cooldown_ms = now + SHAKE_COOLDOWN_MS;
            return true;
        }
    } else {
        if (shakeEnergy < SHAKE_ENERGY_EXIT) {
            shaking = false;
        }
    }
    return false;
}

// ── Serial logging ────────────────────────────────────────────────────────────

static void serial_log(const char *fmt, ...)
{
    serial_dbg_anchored = false;
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm t;
    localtime_r(&tv.tv_sec, &t);
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d.%03ld ",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec / 1000);
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Serial.print(buf);
}

static void serial_imu_debug()
{
    if (serial_dbg_anchored) Serial.print("\033[2A\r");
    serial_dbg_anchored = true;
    unsigned long cdwn = (shake_cooldown_ms > millis()) ? (shake_cooldown_ms - millis()) / 1000 : 0;
    Serial.printf("ax=%6.2f  ay=%6.2f  az=%6.2f  dx=%6.2f  dy=%6.2f  dz=%6.2f\n",
                  dbg_ax, dbg_ay, dbg_az, dbg_dx, dbg_dy, dbg_dz);
    Serial.printf("energy=%5.2f  count=%4.2f  cd=%lus%s\n",
                  dbg_energy, dbg_count, cdwn, shaking ? " [SHAKING]" : "          ");
}

// ── Fetch ─────────────────────────────────────────────────────────────────────

static bool fetch_departures(char *err_buf, size_t err_len)
{
    serial_log("Fetch: %s\n", api_url);
    WiFiClientSecure client;
    // Skips certificate verification — fine for a local hobby device.
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, api_url)) {
        strlcpy(err_buf, "HTTP begin failed", err_len);
        return false;
    }

    int code = http.GET();
    serial_log("Fetch: HTTP %d\n", code);
    if (code != HTTP_CODE_OK) {
        snprintf(err_buf, err_len, "HTTP %d from %s", code, cfg_station);
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError jerr = deserializeJson(doc, body);
    if (jerr) {
        snprintf(err_buf, err_len, "JSON err: %s", jerr.c_str());
        return false;
    }

    strlcpy(station_name, doc["locationName"] | cfg_station, sizeof(station_name));

    // Prefer train services; fall back to bus replacements when trains are absent.
    JsonArray services = doc["trainServices"];
    if (services.isNull()) services = doc["busServices"];

    N = 0;
    if (services.isNull()) return true;

    for (JsonObject svc : services) {
        if (N >= MAX_DEPARTURES) break;
        Departure &d = board[N];

        strlcpy(d.time, svc["std"] | "--:--", sizeof(d.time));

        const char *loc = svc["destination"][0]["locationName"] | "Unknown";
        strlcpy(d.destination, loc, sizeof(d.destination));

        // Buses often have null platform; show "BUS" as a fallback.
        const char *plat = svc["platform"] | "BUS";
        strlcpy(d.platform, plat, sizeof(d.platform));

        bool cancelled = svc["isCancelled"] | false;
        const char *etd = svc["etd"] | "";
        if (cancelled || strcasecmp(etd, "cancelled") == 0) {
            strlcpy(d.status, "Cancelled", sizeof(d.status));
        } else if (strcasecmp(etd, "on time") == 0 || *etd == '\0') {
            strlcpy(d.status, "On time", sizeof(d.status));
        } else if (strcasecmp(etd, "delayed") == 0 || strcasecmp(etd, "no report") == 0) {
            strlcpy(d.status, "Delayed", sizeof(d.status));
        } else {
            snprintf(d.status, sizeof(d.status), "Exp %s", etd);
        }

        N++;
    }
    return true;
}

// ── Drawing ───────────────────────────────────────────────────────────────────

static void draw_board()
{
    serial_log("Screen: departure board\n");
    const int W = gfx->width();   // 820
    const int H = gfx->height();  // 320

    // Header: navy background, station name + fetch time
    gfx->fillRect(0, 0, W, HDR_H, NAVY);
    gfx->setTextSize(3);
    int hy = (HDR_H - 24) / 2;

    gfx->setTextColor(YELLOW);
    gfx->setCursor(X_TIME, hy);
    gfx->print(station_name);

    char ts[6];
    struct tm t;
    if (getLocalTime(&t, 2000)) {
        snprintf(ts, sizeof(ts), "%02d:%02d", t.tm_hour, t.tm_min);
    } else {
        strlcpy(ts, "--:--", sizeof(ts));
    }
    gfx->setTextColor(WHITE);
    gfx->setCursor(W - 5 * CHAR_W - X_TIME, hy);
    gfx->print(ts);

    // Yellow separator under header
    int y = HDR_H;
    gfx->fillRect(0, y, W, 3, YELLOW);
    y += 3;

    // Column-header row
    uint16_t hdr_bg = gfx->color565(20, 20, 60);
    gfx->fillRect(0, y, W, ROW_H, hdr_bg);
    gfx->setTextSize(3);
    gfx->setTextColor(WHITE);
    int ty = y + (ROW_H - 24) / 2;
    gfx->setCursor(X_TIME,                    ty); gfx->print("TIME");
    gfx->setCursor(X_DEST,                    ty); gfx->print("DESTINATION");
    gfx->setCursor(R_PLAT   - 4 * CHAR_W,    ty); gfx->print("PLAT");
    gfx->setCursor(R_STATUS - 6 * CHAR_W,    ty); gfx->print("STATUS");
    y += ROW_H;

    gfx->fillRect(0, y, W, 2, YELLOW);
    y += 2;

    if (N == 0) {
        gfx->fillRect(0, y, W, H - y, BLACK);
        gfx->setTextColor(WHITE);
        gfx->setCursor(X_DEST, y + (ROW_H - 24) / 2);
        gfx->print("No services");
        gfx->flush(true);
        return;
    }

    // Departure rows
    uint16_t divider = gfx->color565(50, 50, 50);
    for (int i = 0; i < N; i++) {
        gfx->fillRect(0, y, W, ROW_H, BLACK);
        gfx->setTextSize(3);
        int ry = y + (ROW_H - 24) / 2;

        gfx->setTextColor(YELLOW);
        gfx->setCursor(X_TIME, ry);
        gfx->print(board[i].time);

        gfx->setCursor(X_DEST, ry);
        gfx->print(board[i].destination);

        gfx->setCursor(R_PLAT - strlen(board[i].platform) * CHAR_W, ry);
        gfx->print(board[i].platform);

        bool on_time = (strcmp(board[i].status, "On time") == 0);
        gfx->setTextColor(on_time ? GREEN : RED);
        gfx->setCursor(R_STATUS - strlen(board[i].status) * CHAR_W, ry);
        gfx->print(board[i].status);

        gfx->drawFastHLine(0, y + ROW_H - 1, W, divider);
        y += ROW_H;
    }
    if (y < H) gfx->fillRect(0, y, W, H - y, BLACK);
    gfx->flush(true);
}

static void draw_mandatory_config(const char *reason)
{
    serial_log("Screen: mandatory config (%s)\n", reason);
    gfx->fillScreen(BLACK);
    slog_y = X_TIME;
    slog("CONFIG REQUIRED!", RED);
    slog(reason);
    slog("");
    slog("Connect to WiFi:");
    char creds[32] = "";
    snprintf(creds, sizeof(creds), "  %s / %s", WIFI_AP_SID, WIFI_AP_PASS);
    slog(creds);
    slog("Then open in browser:");
    slog("  http://192.168.4.1");
    gfx->flush(true);
}

static void draw_optional_config()
{
    serial_log("Screen: optional config\n");
    gfx->fillScreen(BLACK);
    slog_y = X_TIME;
    slog("CONFIG MODE", YELLOW);
    slog("Connect to WiFi:");
    char creds[32] = "";
    snprintf(creds, sizeof(creds), "  %s / %s", WIFI_AP_SID, WIFI_AP_PASS);
    slog(creds);
    slog("Then open in browser:");
    slog("  http://192.168.4.1");
    slog("Or shake to exit");
    slog("Auto-exits in 5 min");
    gfx->flush(true);
}



// ── Config portal ─────────────────────────────────────────────────────────────

static const char CONFIG_HTML[] =
    "<!DOCTYPE html><html><head>"
    "<meta charset=utf-8>"
    "<meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>Departure Board</title>"
    "<style>"
    "body{font-family:sans-serif;max-width:420px;margin:2em auto;padding:0 1em}"
    "h1{font-size:1.4em}"
    "label{display:block;margin-top:1em;font-weight:bold}"
    "input{width:100%;padding:.5em;font-size:1em;box-sizing:border-box}"
    "small{color:#666}"
    "button{margin-top:1.5em;width:100%;padding:.75em;font-size:1em;"
    "background:#00408a;color:#fff;border:none;border-radius:4px;cursor:pointer}"
    "</style></head><body>"
    "<h1>Departure Board</h1>"
    "<form method=POST action=/save>"
    "<label>WiFi network</label>"
    "<input name=ssid value='%SSID%'>"
    "<label>WiFi password</label>"
    "<input name=pass value='%PASS%'>"
    "<label>Station code</label>"
    "<input name=station value='%CODE%' maxlength=4 placeholder='e.g. MZH'>"
    "<small>3-letter CRS code &mdash; "
    "<a href=https://www.nationalrail.co.uk/stations_destinations/48541.aspx target=_blank>"
    "find yours here</a></small>"
    "<br><button>Save &amp; Reboot</button>"
    "</form></body></html>";

static WebServer     server(80);
static bool          portal_active   = false;
static unsigned long restart_at_ms   = 0;

static void portal_start()
{
    if (portal_active) return;
    server.on("/", HTTP_GET, []() {
        String html = CONFIG_HTML;
        html.replace("%SSID%", cfg_ssid);
        html.replace("%PASS%", cfg_pass);
        html.replace("%CODE%", cfg_station);
        server.send(200, "text/html; charset=utf-8", html);
    });
    server.on("/save", HTTP_POST, []() {
        if (server.hasArg("ssid"))    server.arg("ssid").toCharArray(rtc_ssid, sizeof(rtc_ssid));
        if (server.hasArg("pass"))    server.arg("pass").toCharArray(rtc_pass, sizeof(rtc_pass));
        if (server.hasArg("station")) {
            String s = server.arg("station");
            s.toUpperCase();
            s.toCharArray(rtc_station, sizeof(rtc_station));
        }
        rtc_magic     = RTC_SAVE_MAGIC;
        restart_at_ms = millis() + 1000;
        server.sendHeader("Cache-Control", "no-store");
        server.send(200, "text/html",
            "<html><body><h1>Saved!</h1><p>Restarting...</p></body></html>");
    });
    server.begin();
    portal_active = true;
}

static void portal_stop()
{
    if (!portal_active) return;
    server.stop();
    portal_active = false;
}

// ── State transitions ─────────────────────────────────────────────────────────

// Switches WiFi to pure AP mode, draws the error screen, and starts the portal.
// Safe to call from any state.
static void enter_mandatory_config(const char *reason)
{
    imu_ready_at_ms = millis() + 2000;  // WiFi mode change may write NVS → suppress Wire reads
    portal_stop();
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    WiFi.softAP(WIFI_AP_SID, WIFI_AP_PASS);
    draw_mandatory_config(reason);
    portal_start();
    state = MANDATORY_CONFIG;
}

// Adds AP alongside the existing STA connection so home WiFi stays active.
static void enter_optional_config()
{
    imu_ready_at_ms = millis() + 2000;  // WiFi mode change may write NVS → suppress Wire reads
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    WiFi.softAP(WIFI_AP_SID, WIFI_AP_PASS);
    draw_optional_config();
    portal_start();
    optional_config_enter_ms = millis();
    state = OPTIONAL_CONFIG;
}

// ── Arduino entry points ──────────────────────────────────────────────────────

static unsigned long last_fetch      = 0;
static unsigned long serial_dbg_ms   = 0;

void setup()
{
    Serial.begin(115200);

    // Deferred NVS write from the previous boot's config save.
    // Done here, before any peripheral is initialised, so no ISR can access
    // cached memory (PSRAM/DROM) while the flash cache is briefly disabled.
    bool save_pending = (rtc_magic == RTC_SAVE_MAGIC);
    Serial.printf("RTC save pending=%d station='%s'\n", save_pending, save_pending ? rtc_station : "");
    if (save_pending) {
        rtc_magic = 0;
        strlcpy(cfg_ssid,    rtc_ssid,    sizeof(cfg_ssid));
        strlcpy(cfg_pass,    rtc_pass,    sizeof(cfg_pass));
        strlcpy(cfg_station, rtc_station, sizeof(cfg_station));
        save_config();
    }

    load_config();
    imu_init();  // must be before WiFi — Wire.begin() after WiFi+LCD causes a cache panic

    // WiFi must come before display — RF calibration briefly disables the flash
    // cache; LCD DMA firing during that window causes a Cache panic.
    // Block IMU polling until RF calibration is well clear.
    imu_ready_at_ms = millis() + 2000;
    bool wifi_ok = false;
    char reason[64] = "";

    if (strlen(cfg_ssid) == 0) {
        strlcpy(reason, "No WiFi configured", sizeof(reason));
        WiFi.mode(WIFI_AP_STA);
        delay(100);
        WiFi.softAP(WIFI_AP_SID, WIFI_AP_PASS);
    } else {
        Serial.printf("WiFi: connecting to %s\n", cfg_ssid);
        WiFi.persistent(false);  // don't write credentials to NVS — avoids cache-disable window
        WiFi.begin(cfg_ssid, cfg_pass);
        unsigned long t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        if (WiFi.status() == WL_CONNECTED) {
            wifi_ok = true;
            Serial.printf("WiFi OK: %s\n", WiFi.localIP().toString().c_str());
            configTime(0, 0, "pool.ntp.org");
            setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1);
            tzset();
        } else {
            snprintf(reason, sizeof(reason), "No WiFi: %s pw:%d", cfg_ssid, (int)strlen(cfg_pass));
            WiFi.mode(WIFI_AP_STA);
            delay(100);
            WiFi.softAP(WIFI_AP_SID, WIFI_AP_PASS);
        }
    }

    // Display init (always after WiFi)
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, LOW);  // LOW = ON (NPN transistor)
    if (!gfx->begin()) {
        Serial.println("ERROR: display init failed");
        while (1) delay(100);
    }
    gfx->fillScreen(BLACK);

    if (!wifi_ok) {
        draw_mandatory_config(reason);
        portal_start();
        state = MANDATORY_CONFIG;
        return;
    }

    char err[64] = "";
    if (!fetch_departures(err, sizeof(err))) {
        // WiFi connected but fetch failed — switch to AP and show config screen.
        WiFi.mode(WIFI_AP_STA);
        delay(100);
        WiFi.softAP(WIFI_AP_SID, WIFI_AP_PASS);
        draw_mandatory_config(err);
        portal_start();
        state = MANDATORY_CONFIG;
        return;
    }

    last_fetch = millis();
    draw_board();
    state = WORKING;
}

void loop()
{
    if (portal_active) server.handleClient();

    if (restart_at_ms && millis() >= restart_at_ms) {
        portal_stop();
        ESP.restart();
    }

    bool shook = check_shake();

    switch (state) {
    case WORKING: {
        if (shook) {
            enter_optional_config();
            break;
        }
        unsigned long now = millis();
        if (now - last_fetch >= REFRESH_MS) {
            last_fetch = now;
            char err[64] = "";
            if (fetch_departures(err, sizeof(err))) {
                gfx->fillScreen(BLACK);
                draw_board();
            } else {
                enter_mandatory_config(err);
            }
        }
        break;
    }
    case OPTIONAL_CONFIG: {
        bool timed_out = (millis() - optional_config_enter_ms >= OPTIONAL_CONFIG_TIMEOUT_MS);
        if (shook || timed_out) {
            portal_stop();
            unsigned long now = millis();
            if (now - last_fetch >= REFRESH_MS) {
                char err[64] = "";
                if (!fetch_departures(err, sizeof(err))) {
                    enter_mandatory_config(err);
                    break;
                }
                last_fetch = now;
            }
            state = WORKING;
            gfx->fillScreen(BLACK);
            draw_board();
        }
        break;
    }
    case MANDATORY_CONFIG:
        // Portal handled above; nothing else to do here.
        break;
    }

    if (millis() >= serial_dbg_ms) {
        serial_dbg_ms = millis() + 200;
        serial_imu_debug();
    }

    delay(10);  // ~100 Hz — matches QMI8658 shake detection polling rate
}
