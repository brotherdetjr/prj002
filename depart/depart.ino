/*
 * Makerfabs ESP32-S3 Parallel TFT 3.16" ST7701S
 * Railway departure board — Maze Hill (live via national-rail-api.davwheat.dev)
 *
 * Libraries required (Arduino Library Manager):
 *   - Arduino_GFX_Library  (moononournation)
 *   - ArduinoJson          (bblanchon) v7+
 */

#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

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
    320 * 10 /* bounce_buffer_size_px */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    320 /* width */, 820 /* height */, rgbpanel, 1 /* rotation: landscape */,
    true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */,
    mf_st7701_init, sizeof(mf_st7701_init));

// ── Screen log ───────────────────────────────────────────────────────────────

static int slog_y = 0;

static void slog(const char *msg)
{
    Serial.println(msg);
    gfx->setTextSize(3);
    gfx->setTextColor(WHITE);
    gfx->setCursor(4, slog_y);
    gfx->print(msg);
    slog_y += 24;
}

// ── WiFi / API ────────────────────────────────────────────────────────────────

#define WIFI_SSID    "TALKTALKB4AEB9"
#define WIFI_PASS    "4CY8RKUE"
#define STATION_CODE "MZH"
#define API_URL      "https://national-rail-api.davwheat.dev/departures/" STATION_CODE
#define REFRESH_MS   60000UL

// ── Departure data ────────────────────────────────────────────────────────────

// Landscape: 820 px wide × 320 px tall.
// textSize(3): 18 px/char wide, 24 px/char tall.
// TIME and DEST are left-aligned; PLAT and STATUS are right-aligned to R_PLAT / R_STATUS.
//   TIME    left  x=10   (5 ch × 18 =  90 px)
//   DEST    left  x=116  (up to ~37 ch before PLAT, truncated in struct)
//   PLAT    right r=620
//   STATUS  right r=810  (820 - 10 margin)
#define X_TIME    10
#define X_DEST   116
#define R_PLAT   620   // right edge of PLAT column
#define R_STATUS 810   // right edge of STATUS column

#define CHAR_W  18  // textSize(3): 6 px × 3

#define HDR_H  52   // header section height (px)
#define ROW_H  42   // height of each table row (px)

#define MAX_DEPARTURES 5

struct Departure {
    char time[6];         // "HH:MM"
    char destination[28]; // station name, truncated to fit
    char platform[5];     // "1", "2", "-", "BUS"
    char status[12];      // "On time", "Delayed", "Cancelled", "Exp HH:MM"
};

static Departure board[MAX_DEPARTURES];
static int N = 0;
static char station_name[32] = STATION_CODE;  // overwritten by first successful fetch

// ── Fetch ─────────────────────────────────────────────────────────────────────

static bool fetch_departures()
{
    WiFiClientSecure client;
    // Skips certificate verification — fine for a local hobby device.
    // For stricter setups, call client.setCACert(ISRG_ROOT_X1_PEM) instead.
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, API_URL)) return false;

    int code = http.GET();
    char dbg[48];
    snprintf(dbg, sizeof(dbg), "HTTP: %d", code);
    slog(dbg);
    if (code != HTTP_CODE_OK) {
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);

    if (err) {
        snprintf(dbg, sizeof(dbg), "JSON err: %s", err.c_str());
        slog(dbg);
        return false;
    }

    strlcpy(station_name, doc["locationName"] | STATION_CODE, sizeof(station_name));

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
}

// ── Arduino entry points ──────────────────────────────────────────────────────

static unsigned long last_fetch = 0;  // global so setup() can prime it

void setup()
{
    Serial.begin(115200);
    Serial.println("\n--- setup start ---");

    // ── Display first so slog() works immediately ──────────────────────────
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, LOW); // LOW = ON (NPN transistor, active-low)

    // ── WiFi before display ────────────────────────────────────────────────
    // WiFi init briefly disables the flash cache for RF calibration.
    // Starting it before gfx->begin() ensures the LCD DMA never runs during
    // that window, avoiding the "Cache disabled but cached memory region
    // accessed" panic.
    char buf[48];
    Serial.printf("WiFi: connecting to %s\n", WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi OK: %s\n", WiFi.localIP().toString().c_str());
        // UK time: GMT in winter, BST (UTC+1) in summer
        configTime(0, 0, "pool.ntp.org");
        setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1);
        tzset();
        Serial.println("NTP sync done");
    } else {
        Serial.println("\nWiFi FAILED - no network");
    }

    // ── Display ────────────────────────────────────────────────────────────
    if (!gfx->begin()) {
        Serial.println("ERROR: display init failed");
        while (1) delay(100);
    }
    Serial.println("Display OK");
    gfx->fillScreen(BLACK);

    // ── First fetch ────────────────────────────────────────────────────────
    slog("Fetching...");
    bool ok = fetch_departures();
    snprintf(buf, sizeof(buf), "Fetch:%s N:%d %s", ok?"OK":"FAIL", N, station_name);
    slog(buf);
    last_fetch = millis();  // prevents loop() from re-fetching immediately

    delay(4000);  // pause so you can read the slog output

    // ── Draw ───────────────────────────────────────────────────────────────
    gfx->fillScreen(BLACK);
    slog_y = 0;
    draw_board();
    Serial.println("--- setup done ---");
}

void loop()
{
    unsigned long now = millis();
    if (now - last_fetch >= REFRESH_MS) {
        last_fetch = now;
        if (fetch_departures()) {
            gfx->fillScreen(BLACK);
            draw_board();
        }
    }
    delay(1000);
}
