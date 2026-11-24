/*
 * STM32DSP1466 UI bring-up.
 *
 * Uses the raw ST7789 init path that already worked on this PCB, while
 * reusing Adafruit GFX for FreeSans fonts and bitmap/text drawing.
 */

#include <Arduino.h>
#include <SPI.h>
#include <math.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

#include "Heges_Logo.h"
#include "SpotifyIcon.h"
#include "AirplayIcon.h"
#include "RadioIcon.h"
#include "MuteIcon.h"
#include "LoopIcon.h"
#include "WiFiSmallIcon.h"
#include "dsp_map.h"

static constexpr uint32_t DEBUG_UART_RX = PD6;
static constexpr uint32_t DEBUG_UART_TX = PD5;
static constexpr uint32_t DEBUG_BAUD = 115200;

static constexpr uint32_t LINKPLAY_UART_RX = PA10;
static constexpr uint32_t LINKPLAY_UART_TX = PA9;
static constexpr uint32_t LINKPLAY_BAUD = 57600;

static constexpr uint32_t BT_UART_RX = PC7;
static constexpr uint32_t BT_UART_TX = PC6;
static constexpr uint32_t BT_BAUD = 115200;
static constexpr uint8_t BT_ENABLE_PIN = PA1;
static constexpr uint8_t BT_STATUS_PIN = PA2;

static constexpr uint32_t TFT_SPI_MOSI = PA7;
static constexpr uint32_t TFT_SPI_MISO = PA6;
static constexpr uint32_t TFT_SPI_SCK = PA5;
static constexpr uint8_t TFT_CS_PIN = PA4;
static constexpr uint8_t TFT_DC_PIN = PB0;
static constexpr uint8_t TFT_RST_PIN = PB1;
static constexpr uint8_t TFT_BL_PIN = PB10;
static constexpr uint32_t TFT_SPI_HZ = 24000000;

static constexpr uint32_t DSP_SPI_MOSI = PB15;
static constexpr uint32_t DSP_SPI_MISO = PB14;
static constexpr uint32_t DSP_SPI_SCK = PB13;
static constexpr uint32_t DSP_SPI_HZ = 1000000;
static constexpr uint8_t DSP_SPI_WRITE_OPCODE = 0x00;
static constexpr uint8_t DSP_SPI_READ_OPCODE = 0x01;
static constexpr uint8_t DSP_FRAC_BITS = 24;

static constexpr uint8_t POWER_BUTTON_PIN = PE3;
static constexpr uint8_t AMP_ENABLE_PIN = PE5;
static constexpr uint8_t AMP_MUTE_PIN = PE6;
static constexpr uint8_t DSP_CS_PIN = PD8;
static constexpr uint8_t DSP_RST_PIN = PD9;
static constexpr uint32_t FLASH_SPI_MOSI = PB5;
static constexpr uint32_t FLASH_SPI_MISO = PB4;
static constexpr uint32_t FLASH_SPI_SCK = PB3;
static constexpr uint32_t FLASH_SPI_HZ = 1000000;
static constexpr uint8_t FLASH_CS_PIN = PB6;
static constexpr uint8_t FLASH_ALT_CS_PIN = PB7;
static constexpr uint8_t ENCODER_A_PIN = PD14;  // CLK
static constexpr uint8_t ENCODER_B_PIN = PD13;  // DT
static constexpr uint8_t ENCODER_SW_PIN = PD12;
static constexpr uint8_t IR_RECEIVER_PIN = PD15;

static constexpr int16_t SCREEN_W = 320;
static constexpr int16_t SCREEN_H = 240;

static constexpr int16_t VU_Y = 44;
static constexpr int16_t VU_WIDTH = 320;
static constexpr int16_t NUM_SEGMENTS = 16;
static constexpr int16_t SEGMENT_WIDTH = (VU_WIDTH / NUM_SEGMENTS) - 2;
static constexpr int16_t BAR_HEIGHT = 30;
static constexpr int16_t TITLE_Y = 166;
static constexpr int16_t BOTTOM_INFO_Y = 201;
static constexpr int16_t BOTTOM_INFO_BASELINE_Y = 227;
static constexpr uint32_t METER_UPDATE_MS = 100;
static constexpr uint32_t AUDIO_SETTINGS_SAVE_DELAY_MS = 2000;
static constexpr uint32_t BT_PAUSE_SETTLE_MS = 1800;
static constexpr uint32_t BT_TAKEOVER_CONFIRM_MS = 7000;
static constexpr uint32_t BT_STANDBY_DISCONNECT_DELAY_MS = 1000;
static constexpr uint32_t BT_RECONNECT_DELAY_MS = 1000;
static constexpr uint32_t BT_NAME_REQUEST_DELAY_MS = 200;
static constexpr uint32_t LP_JSON_PLAY_AFTER_BT_IGNORE_MS = 3000;
static constexpr uint32_t MENU_TIMEOUT_MS = 20000;
static constexpr uint32_t STANDBY_AUTO_WAKE_GUARD_MS = 2000;
static constexpr uint32_t STREAM_MUTE_STANDBY_MS = 10UL * 60UL * 1000UL;
static constexpr uint32_t LINKPLAY_RTC_SYNC_INTERVAL_MS = 30UL * 60UL * 1000UL;
static constexpr uint32_t LINKPLAY_METADATA_FORCE_MIN_INTERVAL_MS = 500;
static constexpr uint32_t LINKPLAY_HARD_OFF_TIMEOUT_MS = 8000;
static constexpr uint32_t SYSTEM_BOOT_ANIM_MS = 800;
static constexpr uint32_t SYSTEM_BOOT_TIMEOUT_MS = 30000;
static constexpr uint16_t HARD_OFF_SUCCESS_BLINK_MS = 180;
static constexpr uint16_t HARD_OFF_ALERT_BLINK_MS = 110;
static constexpr float DSP_VOLUME_TABLE_MAX_INDEX = 127.0f;
static constexpr float DSP_VOLUME_LOG_STEP = 0.05f;
static constexpr uint8_t ENCODER_DETENT_DIVISOR = 2;
static constexpr uint8_t ENCODER_VOLUME_STEP = 1;
static constexpr uint32_t ENCODER_BUTTON_DEBOUNCE_MS = 40;
static constexpr uint32_t ENCODER_BUTTON_LONG_MS = 2000;
static constexpr uint8_t IR_MAX_PULSES = 100;
static constexpr uint32_t IR_FRAME_GAP_US = 14000;
static constexpr uint16_t IR_REMOTE_ADDR = 0x4040;
static constexpr uint32_t IR_REPEAT_ACTION_MS = 120;
static constexpr uint32_t CLOCK_STORE_MAGIC = 0x48475443UL;  // HGTC
static constexpr uint32_t AUDIO_SETTINGS_MAGIC = 0x48474136UL;  // HGA6
static constexpr uint32_t RTC_LSE_START_TIMEOUT_MS = 2500;
static constexpr uint32_t EXTERNAL_CLOCK_MAGIC = 0x4847434CUL;  // HGCL
static constexpr uint32_t EXTERNAL_AUDIO_MAGIC = 0x48474153UL;  // HGAS
static constexpr uint32_t EXTERNAL_SCREEN_MAGIC = 0x48475343UL;  // HGSC
static constexpr uint16_t EXTERNAL_STORAGE_VERSION = 1;
static constexpr uint16_t EXTERNAL_STORAGE_SLOT_SIZE = 256;
static constexpr uint32_t EXTERNAL_CLOCK_SLOT_A = 0x0000UL;
static constexpr uint32_t EXTERNAL_CLOCK_SLOT_B = 0x0100UL;
static constexpr uint32_t EXTERNAL_AUDIO_SLOT_A = 0x0200UL;
static constexpr uint32_t EXTERNAL_AUDIO_SLOT_B = 0x0300UL;
static constexpr uint32_t EXTERNAL_SCREEN_SLOT_A = 0x0400UL;
static constexpr uint32_t EXTERNAL_SCREEN_SLOT_B = 0x0500UL;
static constexpr int AUDIO_SETTINGS_EEPROM_ADDR = 16;
static constexpr uint32_t CLOCK_EEPROM_SAVE_DRIFT_SEC = 5UL * 60UL;
static constexpr uint32_t CLOCK_EEPROM_SHUTDOWN_SNAPSHOT_SEC = 60UL;
static constexpr int INPUT_GAIN_MIN_DB = -10;
static constexpr int INPUT_GAIN_DEFAULT_MAX_DB = 0;
static constexpr int INPUT_GAIN_XLR_MAX_DB = 3;
static constexpr int INPUT_GAIN_BT_MAX_DB = 3;
static constexpr const char *LINKPLAY_CAP_PROJECT = "UP2STREAM_PRO_V4";
static constexpr const char *LINKPLAY_CAP_PTV = "002";
static constexpr const char *LINKPLAY_CAP1 = "00305200";
static constexpr const char *LINKPLAY_CAP2 = "00000000";
static constexpr const char *LINKPLAY_CAP_PLM = "00008214";  // USB host + BT + XLR + optical.
static constexpr const char *LINKPLAY_CAP_LANGUAGE = "en_us";
static constexpr const char *LINKPLAY_DEFAULT_SSID = "DSP_Amp";

static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;
static constexpr uint16_t COLOR_RED = 0xF800;
static constexpr uint16_t COLOR_GREEN = 0x07E0;
static constexpr uint16_t COLOR_YELLOW = 0xFFE0;
static constexpr uint16_t COLOR_ORANGE = 0xFD20;
static constexpr uint16_t COLOR_DARKGREEN = 0x0320;
static constexpr uint16_t COLOR_DARKGREY = 0x39E7;
static constexpr uint16_t COLOR_LIGHTGREY = 0xD6BA;
static constexpr uint16_t COLOR_DIM_SEGMENT = 0x0841;
static constexpr uint16_t COLOR_GREENYELLOW = 0xAFE5;

HardwareSerial DebugSerial(DEBUG_UART_RX, DEBUG_UART_TX);
HardwareSerial LinkplaySerial(LINKPLAY_UART_RX, LINKPLAY_UART_TX);
HardwareSerial BtSerial(BT_UART_RX, BT_UART_TX);
SPIClass TftSpi(TFT_SPI_MOSI, TFT_SPI_MISO, TFT_SPI_SCK);
SPIClass DspSpi(DSP_SPI_MOSI, DSP_SPI_MISO, DSP_SPI_SCK);
SPISettings tftSpiSettings(TFT_SPI_HZ, MSBFIRST, SPI_MODE0);
SPISettings dspSpiSettings(DSP_SPI_HZ, MSBFIRST, SPI_MODE0);

enum DspSource : uint8_t {
  SOURCE_LINKPLAY = 0,
  SOURCE_XLR = 1,
  SOURCE_BLUETOOTH = 2,
  SOURCE_OPTICAL = 3,
};

enum LinkplayGroupRole : uint8_t {
  LINKPLAY_GROUP_NONE = 0,
  LINKPLAY_GROUP_MASTER,
  LINKPLAY_GROUP_SLAVE,
};

enum MenuItem : uint8_t {
  MENU_ITEM_SOURCE = 0,
  MENU_ITEM_OUTPUT_MODE,
  MENU_ITEM_TONE,
  MENU_ITEM_INPUT_GAINS,
  MENU_ITEM_LCD_BL,
  MENU_ITEM_CLOCK,
  MENU_ITEM_EXIT,
  MENU_ITEM_COUNT,
};

enum MenuPage : uint8_t {
  MENU_PAGE_MAIN = 0,
  MENU_PAGE_TONE,
  MENU_PAGE_INPUT_GAINS,
  MENU_PAGE_SCREEN_SETTINGS,
  MENU_PAGE_BACKLIGHT,
  MENU_PAGE_CLOCK,
};

enum OutputMode : uint8_t {
  OUTPUT_NORMAL = 0,
  OUTPUT_CINEMA = 1,
  OUTPUT_BIAMP = 2,
  OUTPUT_ADI_CLUB = 3,
  OUTPUT_ADI_MUSIC = 4,
  OUTPUT_ADI_STADIUM = 5,
  OUTPUT_ADI_HALL = 6,
  OUTPUT_ADI_ROCK = 7,
  OUTPUT_ADI_JAZZ = 8,
};

static constexpr OutputMode kOutputModeMenuOrder[] = {
    OUTPUT_NORMAL,
    OUTPUT_ADI_CLUB,
    OUTPUT_ADI_MUSIC,
    OUTPUT_ADI_STADIUM,
    OUTPUT_CINEMA,
    OUTPUT_ADI_HALL,
    OUTPUT_ADI_ROCK,
    OUTPUT_ADI_JAZZ,
    OUTPUT_BIAMP,
};
static constexpr uint8_t OUTPUT_MODE_MENU_COUNT = sizeof(kOutputModeMenuOrder) / sizeof(kOutputModeMenuOrder[0]);

enum ScreenPanelType : uint8_t {
  SCREEN_PANEL_20 = 20,
  SCREEN_PANEL_24 = 24,
};

uint8_t tftMadctlValue();
bool tftInvertColors();

class RawST7789GFX : public Adafruit_GFX {
public:
  RawST7789GFX() : Adafruit_GFX(SCREEN_W, SCREEN_H) {}

  void begin() {
    pinMode(TFT_CS_PIN, OUTPUT);
    pinMode(TFT_DC_PIN, OUTPUT);
    pinMode(TFT_RST_PIN, OUTPUT);
    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_CS_PIN, HIGH);
    digitalWrite(TFT_DC_PIN, LOW);
    digitalWrite(TFT_RST_PIN, HIGH);
    digitalWrite(TFT_BL_PIN, HIGH);

    TftSpi.begin();
    reset();

    command(0x01);  // SWRESET
    delay(150);
    command(0x11);  // SLPOUT
    delay(120);

    const uint8_t colorMode[] = {0x55};  // 16-bit RGB565
    commandData(0x3A, colorMode, sizeof(colorMode));

    const uint8_t madctl[] = {tftMadctlValue()};
    commandData(0x36, madctl, sizeof(madctl));

    command(tftInvertColors() ? 0x21 : 0x20);  // INVON/INVOFF
    delay(10);
    command(0x13);  // NORON
    delay(10);
    command(0x29);  // DISPON
    delay(120);
    setTextWrap(false);
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || y < 0 || x >= width() || y >= height()) {
      return;
    }
    setAddressWindow(x, y, 1, 1);
    dataStart();
    writeColor(color, 1);
    dataEnd();
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    if (x >= width() || y >= height() || w <= 0 || h <= 0) {
      return;
    }
    if (x < 0) {
      w += x;
      x = 0;
    }
    if (y < 0) {
      h += y;
      y = 0;
    }
    if (x + w > width()) {
      w = width() - x;
    }
    if (y + h > height()) {
      h = height() - y;
    }
    if (w <= 0 || h <= 0) {
      return;
    }

    setAddressWindow(x, y, w, h);
    dataStart();
    writeColor(color, static_cast<uint32_t>(w) * h);
    dataEnd();
  }

  void fillScreen(uint16_t color) {
    fillRect(0, 0, width(), height(), color);
  }

  void drawCanvas1(int16_t x, int16_t y, const GFXcanvas1 &canvas, uint16_t fg, uint16_t bg) {
    int16_t w = canvas.width();
    int16_t h = canvas.height();
    if (x < 0 || y < 0 || x + w > width() || y + h > height()) {
      return;
    }

    const uint8_t *buffer = canvas.getBuffer();
    int16_t byteWidth = (w + 7) / 8;
    uint8_t line[320 * 2];
    setAddressWindow(x, y, w, h);
    dataStart();
    for (int16_t py = 0; py < h; py++) {
      uint16_t out = 0;
      for (int16_t px = 0; px < w; px++) {
        bool pixelOn = buffer[py * byteWidth + (px / 8)] & (0x80 >> (px & 7));
        uint16_t color = pixelOn ? fg : bg;
        line[out++] = static_cast<uint8_t>(color >> 8);
        line[out++] = static_cast<uint8_t>(color & 0xFF);
      }
      TftSpi.transfer(line, out);
    }
    dataEnd();
  }

private:
  void select() {
    digitalWrite(TFT_CS_PIN, LOW);
  }

  void deselect() {
    digitalWrite(TFT_CS_PIN, HIGH);
  }

  void reset() {
    digitalWrite(TFT_RST_PIN, HIGH);
    delay(10);
    digitalWrite(TFT_RST_PIN, LOW);
    delay(20);
    digitalWrite(TFT_RST_PIN, HIGH);
    delay(120);
  }

  void command(uint8_t cmd) {
    TftSpi.beginTransaction(tftSpiSettings);
    digitalWrite(TFT_DC_PIN, LOW);
    select();
    TftSpi.transfer(cmd);
    deselect();
    TftSpi.endTransaction();
  }

  void data(const uint8_t *bytes, size_t len) {
    TftSpi.beginTransaction(tftSpiSettings);
    digitalWrite(TFT_DC_PIN, HIGH);
    select();
    for (size_t i = 0; i < len; i++) {
      TftSpi.transfer(bytes[i]);
    }
    deselect();
    TftSpi.endTransaction();
  }

  void commandData(uint8_t cmd, const uint8_t *bytes, size_t len) {
    command(cmd);
    data(bytes, len);
  }

  void setAddressWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t x1 = x + w - 1;
    uint16_t y1 = y + h - 1;
    uint8_t col[] = {
        static_cast<uint8_t>(x >> 8),
        static_cast<uint8_t>(x & 0xFF),
        static_cast<uint8_t>(x1 >> 8),
        static_cast<uint8_t>(x1 & 0xFF),
    };
    uint8_t row[] = {
        static_cast<uint8_t>(y >> 8),
        static_cast<uint8_t>(y & 0xFF),
        static_cast<uint8_t>(y1 >> 8),
        static_cast<uint8_t>(y1 & 0xFF),
    };
    commandData(0x2A, col, sizeof(col));
    commandData(0x2B, row, sizeof(row));
    command(0x2C);
  }

  void dataStart() {
    TftSpi.beginTransaction(tftSpiSettings);
    digitalWrite(TFT_DC_PIN, HIGH);
    select();
  }

  void dataEnd() {
    deselect();
    TftSpi.endTransaction();
  }

  void writeColor(uint16_t color, uint32_t count) {
    uint8_t hi = static_cast<uint8_t>(color >> 8);
    uint8_t lo = static_cast<uint8_t>(color & 0xFF);
    uint8_t chunk[64];
    while (count > 0) {
      uint32_t pixels = min<uint32_t>(count, sizeof(chunk) / 2);
      for (uint32_t i = 0; i < pixels * 2; i += 2) {
        chunk[i] = hi;
        chunk[i + 1] = lo;
      }
      TftSpi.transfer(chunk, pixels * 2);
      count -= pixels;
    }
  }
};

RawST7789GFX tft;
GFXcanvas1 titleCanvas(320, 29);
GFXcanvas1 headerClockCanvas(72, 25);
GFXcanvas1 standbyClockCanvas(190, 52);

bool ampEnabled = false;
bool splashActive = false;
bool ampMuteAsserted = true;
bool mainScreenVisible = false;
bool hardOffScreenVisible = false;
bool systemBootScreenVisible = false;
bool menuVisible = false;
bool menuEditing = false;
uint32_t splashStartMs = 0;
uint32_t systemBootStartedMs = 0;
uint32_t lastSystemBootAnimMs = 0;
String debugLine;
String linkplayLine;
String btLine;
String currentArtist = "";
String currentTitle = "No Info";
String currentAlbum = "";
String networkModeName = "Network";
String playbackState = "stop";
String currentTime = "--:--";
String lastScrollText;
String systemBootMessage = "";
int16_t scrollOffset = 320;
uint32_t scrollIdleStartMs = 0;
uint32_t lastScrollUpdateMs = 0;
uint32_t lastTimerUpdateMs = 0;
uint32_t lastLinkplayRxMs = 0;
uint32_t lastLinkplayPendingLogMs = 0;
uint32_t lastLinkplayMetadataRequestMs = 0;
uint32_t lastLinkplayRtcRequestMs = 0;
uint32_t linkplayHardOffDeadlineMs = 0;
uint8_t systemBootFrame = 0;
uint8_t systemBootStep = 0;
bool systemBootTimedOut = false;
uint32_t lastBtRxMs = 0;
uint32_t bottomStatusUntilMs = 0;
uint32_t lastAudioSettingsChangeMs = 0;
uint32_t lastMenuActivityMs = 0;
uint32_t standbyAutoWakeBlockedUntilMs = 0;
uint32_t streamMuteStartedMs = 0;
uint32_t clockEpochAtSync = 0;
uint32_t clockMillisAtSync = 0;
uint32_t lastClockEepromEpoch = 0;
uint32_t externalClockSequence = 0;
uint32_t externalAudioSequence = 0;
uint32_t externalScreenSequence = 0;
int32_t lastDisplayedClockMinute = -1;
int32_t lastDisplayedStandbyDay = -1;
bool scrollIdle = true;
bool rtcHardwareReady = false;
uint32_t trackLengthMs = 0;
uint32_t trackPositionMs = 0;
uint32_t lastTrackTickMs = 0;
uint8_t linkplayLoopMode = 4;
bool linkplayMediaReady = false;
bool linkplayMetadataPending = false;
bool linkplayPinfPending = false;
bool linkplayBootDoneReported = false;
bool linkplayReadyReported = false;
bool linkplayWifiReady = false;
bool linkplayWifiStatusRequestPending = false;
bool linkplayMuted = false;
bool linkplayRawLogEnabled = true;
bool btRawLogEnabled = true;
bool linkplayHardOffPending = false;
bool subEnabled = true;
bool btModuleEnabled = false;
bool screenLightTheme = false;
bool linkplayTransportPlaying = false;
bool btTransportPlaying = false;
bool btPauseSettleActive = false;
bool btTakeoverPending = false;
bool btStandbyDisconnectPending = false;
bool btWaitingSplashShown = false;
bool btNameRequestPending = false;
bool btReconnectPending = false;
bool audioSettingsDirty = false;
bool audioSettingsSaveOnStandby = false;
bool clockDirty = false;
bool clockValid = false;
bool localSourceTitleLocked = false;
uint8_t displayedVolumePercent = 30;
uint8_t menuIndex = 0;
MenuPage menuPage = MENU_PAGE_MAIN;
DspSource currentSource = SOURCE_LINKPLAY;
LinkplayGroupRole linkplayGroupRole = LINKPLAY_GROUP_NONE;
OutputMode currentOutputMode = OUTPUT_NORMAL;
ScreenPanelType screenPanelType = SCREEN_PANEL_24;
uint16_t screenRotationDegrees = 0;
String bottomStatusText;
String btConnectedDeviceName = "";
String linkplayNetworkModeCode = "000";
int bassGainDb = 0;
int trebleGainDb = 0;
int lpGainDb = 0;
int xlrGainDb = 0;
int btGainDb = 0;
int opticalGainDb = 0;
uint8_t backlightOnPercent = 85;
uint8_t backlightStandbyPercent = 25;
int displayedVuLeft = -1;
int displayedVuRight = -1;
int displayedPeakLeft = -1;
int displayedPeakRight = -1;
int lastDrawnLeftSegments = -1;
int lastDrawnRightSegments = -1;
int lastDrawnLeftPeak = -1;
int lastDrawnRightPeak = -1;
uint32_t btPauseSettleUntilMs = 0;
uint32_t lastBtSourceSelectMs = 0;
uint32_t btTakeoverAtMs = 0;
uint32_t btStandbyDisconnectAtMs = 0;
uint32_t btNameRequestAtMs = 0;
uint32_t btReconnectAtMs = 0;
uint8_t encoderLastState = 0;
int8_t encoderAccumulator = 0;
bool encoderButtonStable = HIGH;
bool encoderButtonLastReading = HIGH;
uint32_t encoderButtonLastChangeMs = 0;
uint32_t encoderButtonPressedMs = 0;
bool encoderButtonLongHandled = false;
uint8_t lastIrCommand = 0;
uint32_t lastIrRepeatActionMs = 0;
volatile uint16_t irPulseBuffers[2][IR_MAX_PULSES];
volatile uint8_t irActiveBuffer = 0;
volatile uint8_t irPulseCount = 0;
volatile uint8_t irFrameBuffer = 0;
volatile uint8_t irFrameLength = 0;
volatile bool irFrameReady = false;
volatile uint32_t irLastEdgeUs = 0;

void drawVolumeValue(uint8_t percent);
void drawSourceHeader();
void drawHeaderClock();
void drawTimers();
void drawScrollingTitle(bool force);
void drawPlaybackIcon(const char *state);
void drawTopStatusIcon();
String currentScrollText();
void resetScroll();
void requestLinkplayMetadata(bool force = false);
void pauseLinkplayTransport(const char *reason);
void pauseLinkplayForStandby();
void pauseBtTransport(const char *reason);
void pauseBtForStandby();
void sendLinkplayCommand(const String &cmd);
void sendBtCommand(const String &cmd);
void setBtModuleEnable(bool enabled);
void updateBtEnableForSource(bool showWaiting);
void showBtWaitingSplash();
void requestBtConnectedNames(uint32_t delayMs = BT_NAME_REQUEST_DELAY_MS);
void pollBtReconnect();
void pollBtNameRequest();
void noteLinkplayTransportPlay(const char *reason);
void noteLocalLinkplayModePlay(const char *reason);
void noteBtTransportPlay(const char *reason);
bool wakeFromTransport(const char *reason);
void pollStreamMuteStandby();
void setSubEnabled(bool enabled, bool persist = true);
void drawStandby();
void drawStandbyClock();
void drawStandbyDate(bool force = false);
void drawStandbyWifiIcon();
const char *weekdayName(uint8_t weekday);
void drawHardOffScreen(const char *title, const char *message);
void startSystemBootScreen();
void updateSystemBootScreen(const char *message, uint8_t step);
void finishSystemBootScreen();
void pollSystemBootScreen();
void showBootScreen();
void drawMainScreen();
void showBottomStatus(const String &text, uint32_t durationMs = 2000);
void scheduleAudioSettingsSave();
void enterMenu();
void exitMenu();
void drawMenuScreen();
void drawMenuRow(uint8_t index, bool selected);
void menuMove(int delta);
void menuAdjust(int delta);
void menuSelect();
void encoderMenuPress();
void menuBack();
bool menuSelectedItemIsAdjustable();
void cycleSourceFromControl(const char *source);
void togglePlaybackFromControl(const char *source);
void skipTrackFromControl(int direction, const char *source);
void saveClockToEeprom(uint32_t epoch);
void flushClockIfDirty();
void saveClockShutdownSnapshot();
void requestLinkplayRtcTime();
void requestLinkplayWifiStatus();
void handleLinkplayWifiState(bool ready);
void pollLinkplayRtcSync();
void saveAudioSettingsToEeprom();
void printClockDebug();
bool initRtcClockSource();
void printRtcClockDebug();
bool setHardwareRtcFromEpoch(uint32_t epoch);
bool readHardwareRtcEpoch(uint32_t &epoch);
uint8_t dbToLpEqValue(int gainDb);
void sendLinkplayEqReport();
void sendLinkplayEqNeutralReport();
void sendLinkplayVolumeReport();
void printExternalEepromBitBangStatus();
void testExternalEepromBitBang();
void externalEepromBitBangRead(uint32_t address, uint8_t *data, size_t length);
bool externalEepromBitBangWrite(uint32_t address, const uint8_t *data, size_t length);
bool readExternalStorageRecord(uint32_t slotAddress, uint32_t magic, void *payload, uint16_t payloadSize, uint32_t &sequence);
bool writeExternalStorageRecord(uint32_t slotAddress, uint32_t magic, const void *payload, uint16_t payloadSize, uint32_t sequence);
void applyVolumePercent(uint8_t percent, bool reportToLinkplay, bool persist, const char *source);
void setOutputMode(OutputMode mode, bool persist = true);
void setInputGainDb(DspSource source, int gainDb, bool persist = true);
void setBacklightPercent(uint8_t percent);
void applyBacklightForState();
void adjustClockMinutes(int deltaMinutes);
void adjustClockDays(int deltaDays);
void adjustClockMonths(int deltaMonths);
void adjustClockYears(int deltaYears);
void requestClockNtpFromMenu();
void setAmpPower(bool enabled, const char *source);
void toggleAmpPowerFromControl(const char *source);
void startLinkplayHardOffRequest();
void pollLinkplayHardOffRequest();
void pollEncoder();
void pollIrReceiver();
void handleIrEdge();
void handleIrCommand(uint8_t command, bool repeat);
void toggleMuteFromControl();
uint16_t uiBgColor();
uint16_t uiFgColor();
uint16_t uiSelectedBgColor();
uint16_t uiSelectedFgColor();
void setScreenLightTheme(bool light, bool persist = true);
void redrawActiveUi();
void loadScreenSettingsFromEeprom();
void saveScreenSettingsToEeprom();
void printScreenSettings();
bool setScreenPanelConfig(ScreenPanelType panel, uint16_t rotationDegrees, bool persist = true);
void printHexByte(uint8_t value);

struct StoredClock {
  uint32_t magic;
  uint32_t epoch;
  uint32_t inverseEpoch;
};

struct StoredAudioSettings {
  uint32_t magic;
  uint8_t volume;
  int8_t bass;
  int8_t treble;
  int8_t lpGain;
  int8_t xlrGain;
  int8_t btGain;
  int8_t opticalGain;
  uint8_t outputMode;
  uint8_t backlightOn;
  uint8_t backlightStandby;
  uint8_t subEnabled;
  uint8_t source;
  uint8_t checksum;
};

struct StoredScreenSettings {
  uint8_t panelType;
  uint8_t rotation;
  uint8_t reserved[2];
};

struct __attribute__((packed)) ExternalStorageHeader {
  uint32_t magic;
  uint32_t sequence;
  uint16_t version;
  uint16_t length;
  uint8_t checksum;
  uint8_t reserved[3];
};

void setupSafePins() {
  pinMode(AMP_ENABLE_PIN, OUTPUT);
  pinMode(AMP_MUTE_PIN, OUTPUT);
  pinMode(BT_ENABLE_PIN, OUTPUT);
  digitalWrite(AMP_ENABLE_PIN, LOW);
  digitalWrite(AMP_MUTE_PIN, HIGH);
  digitalWrite(BT_ENABLE_PIN, LOW);

  pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BT_STATUS_PIN, INPUT);
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
  pinMode(IR_RECEIVER_PIN, INPUT_PULLUP);
  encoderLastState = ((digitalRead(ENCODER_A_PIN) ? 1 : 0) << 1) |
                     (digitalRead(ENCODER_B_PIN) ? 1 : 0);
  encoderButtonStable = digitalRead(ENCODER_SW_PIN);
  encoderButtonLastReading = encoderButtonStable;
  encoderButtonLastChangeMs = millis();

  pinMode(DSP_CS_PIN, OUTPUT);
  pinMode(DSP_RST_PIN, OUTPUT);
  pinMode(FLASH_CS_PIN, OUTPUT);
  pinMode(FLASH_ALT_CS_PIN, OUTPUT);
  digitalWrite(DSP_CS_PIN, HIGH);
  digitalWrite(DSP_RST_PIN, HIGH);
  digitalWrite(FLASH_CS_PIN, HIGH);
  digitalWrite(FLASH_ALT_CS_PIN, HIGH);
}

uint8_t tftMadctlValue() {
  return screenRotationDegrees == 180 ? 0xA0 : 0x60;
}

bool tftInvertColors() {
  return screenPanelType == SCREEN_PANEL_20;
}

uint16_t uiBgColor() {
  return screenLightTheme ? COLOR_WHITE : COLOR_BLACK;
}

uint16_t uiFgColor() {
  return screenLightTheme ? COLOR_BLACK : COLOR_WHITE;
}

uint16_t uiSelectedBgColor() {
  return uiFgColor();
}

uint16_t uiSelectedFgColor() {
  return uiBgColor();
}

uint16_t uiDimSegmentColor() {
  return screenLightTheme ? COLOR_LIGHTGREY : COLOR_DIM_SEGMENT;
}

void setAmpMute(bool asserted) {
  ampMuteAsserted = asserted;
  digitalWrite(AMP_MUTE_PIN, asserted ? HIGH : LOW);
  DebugSerial.print("AMP_MUTE ");
  DebugSerial.println(asserted ? "ON" : "OFF");
}

void setAmpPower(bool enabled, const char *source) {
  if (ampEnabled == enabled) {
    return;
  }

  ampEnabled = enabled;
  digitalWrite(AMP_ENABLE_PIN, ampEnabled ? HIGH : LOW);

  if (ampEnabled) {
    hardOffScreenVisible = false;
    systemBootScreenVisible = false;
    linkplayBootDoneReported = false;
    linkplayReadyReported = false;
    showBootScreen();
  } else {
    menuVisible = false;
    menuEditing = false;
    standbyAutoWakeBlockedUntilMs = millis() + STANDBY_AUTO_WAKE_GUARD_MS;
    pauseLinkplayForStandby();
    pauseBtForStandby();
    setBtModuleEnable(false);
    setAmpMute(true);
    if (audioSettingsSaveOnStandby) {
      saveAudioSettingsToEeprom();
      audioSettingsSaveOnStandby = false;
    }
    flushClockIfDirty();
    saveClockShutdownSnapshot();
    drawStandby();
  }

  DebugSerial.print("AMP_ENABLE ");
  DebugSerial.print(ampEnabled ? "ON" : "OFF");
  DebugSerial.print(" from ");
  DebugSerial.println(source);
}

void toggleAmpPowerFromControl(const char *source) {
  setAmpPower(!ampEnabled, source);
}

void printHex32(uint32_t value) {
  DebugSerial.print("0x");
  for (int shift = 28; shift >= 0; shift -= 4) {
    DebugSerial.print("0123456789ABCDEF"[(value >> shift) & 0x0F]);
  }
}

void dspSelect() {
  digitalWrite(DSP_CS_PIN, LOW);
}

void dspDeselect() {
  digitalWrite(DSP_CS_PIN, HIGH);
}

bool writeDspBlock(uint16_t address, const uint8_t *data, size_t length) {
  if (data == nullptr || length == 0) {
    return false;
  }

  DspSpi.beginTransaction(dspSpiSettings);
  dspSelect();
  DspSpi.transfer(DSP_SPI_WRITE_OPCODE);
  DspSpi.transfer(static_cast<uint8_t>(address >> 8));
  DspSpi.transfer(static_cast<uint8_t>(address & 0xFF));
  for (size_t i = 0; i < length; i++) {
    DspSpi.transfer(data[i]);
  }
  dspDeselect();
  DspSpi.endTransaction();
  return true;
}

bool readDspBlock(uint16_t address, uint8_t *data, size_t length) {
  if (data == nullptr || length == 0) {
    return false;
  }

  DspSpi.beginTransaction(dspSpiSettings);
  dspSelect();
  DspSpi.transfer(DSP_SPI_READ_OPCODE);
  DspSpi.transfer(static_cast<uint8_t>(address >> 8));
  DspSpi.transfer(static_cast<uint8_t>(address & 0xFF));
  for (size_t i = 0; i < length; i++) {
    data[i] = DspSpi.transfer(0x00);
  }
  dspDeselect();
  DspSpi.endTransaction();
  return true;
}

bool writeDspU32(uint16_t address, uint32_t value) {
  uint8_t data[4] = {
      static_cast<uint8_t>((value >> 24) & 0xFF),
      static_cast<uint8_t>((value >> 16) & 0xFF),
      static_cast<uint8_t>((value >> 8) & 0xFF),
      static_cast<uint8_t>(value & 0xFF),
  };
  return writeDspBlock(address, data, sizeof(data));
}

bool readDspU32(uint16_t address, uint32_t &value) {
  uint8_t data[4] = {};
  if (!readDspBlock(address, data, sizeof(data))) {
    return false;
  }
  value = (static_cast<uint32_t>(data[0]) << 24) |
          (static_cast<uint32_t>(data[1]) << 16) |
          (static_cast<uint32_t>(data[2]) << 8) |
          static_cast<uint32_t>(data[3]);
  return true;
}

bool readDspReg16(uint16_t address, uint16_t &value) {
  uint8_t data[2] = {};
  if (!readDspBlock(address, data, sizeof(data))) {
    return false;
  }
  value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  return true;
}

uint32_t dspFloatToFixed8_24(float value) {
  return static_cast<uint32_t>(lroundf(value * static_cast<float>(1UL << DSP_FRAC_BITS)));
}

uint32_t volumePercentToSigma(uint8_t percent) {
  percent = constrain(percent, 0, 100);
  if (percent == 0) {
    return 0;
  }
  if (percent >= 100) {
    return 0x01000000UL;
  }
  float tableIndex = roundf(static_cast<float>(percent) * DSP_VOLUME_TABLE_MAX_INDEX / 100.0f);
  float gain = expf((tableIndex - DSP_VOLUME_TABLE_MAX_INDEX) * DSP_VOLUME_LOG_STEP);
  return dspFloatToFixed8_24(gain);
}

float dspFixed8_24ToFloat(uint32_t raw) {
  int32_t signedRaw = static_cast<int32_t>(raw);
  return static_cast<float>(signedRaw) / static_cast<float>(1UL << DSP_FRAC_BITS);
}

uint8_t sigmaVolumeToPercent(uint32_t raw) {
  float gain = dspFixed8_24ToFloat(raw);
  if (gain <= 0.0f) {
    return 0;
  }
  int percent = static_cast<int>(lroundf(((logf(gain) / DSP_VOLUME_LOG_STEP) + DSP_VOLUME_TABLE_MAX_INDEX) * 100.0f / DSP_VOLUME_TABLE_MAX_INDEX));
  return static_cast<uint8_t>(constrain(percent, 0, 100));
}

int rtaRawToSegments(uint32_t raw) {
  float level = fabsf(dspFixed8_24ToFloat(raw));
  if (level < 0.00001f) {
    return 0;
  }
  float db = 20.0f * log10f(level);
  int segments = static_cast<int>(lroundf((db + 60.0f) * static_cast<float>(NUM_SEGMENTS) / 63.0f));
  return constrain(segments, 0, NUM_SEGMENTS);
}

void printDspU32(const char *label, uint16_t address) {
  uint32_t value = 0;
  DebugSerial.print(label);
  DebugSerial.print("@");
  DebugSerial.print(address);
  DebugSerial.print("=");
  if (readDspU32(address, value)) {
    printHex32(value);
  } else {
    DebugSerial.print("ERR");
  }
  DebugSerial.println();
}

void printDspReg16(const char *label, uint16_t address) {
  uint16_t value = 0;
  DebugSerial.print(label);
  DebugSerial.print("@0x");
  DebugSerial.print(address, HEX);
  DebugSerial.print("=");
  if (readDspReg16(address, value)) {
    DebugSerial.print("0x");
    if (value < 0x1000) DebugSerial.print("0");
    if (value < 0x0100) DebugSerial.print("0");
    if (value < 0x0010) DebugSerial.print("0");
    DebugSerial.print(value, HEX);
  } else {
    DebugSerial.print("ERR");
  }
  DebugSerial.println();
}

void printDspMap() {
  DebugSerial.println("DSP export: Smart1466Ver5");
  DebugSerial.print("  master=");
  DebugSerial.print(DspMap::kMasterVolume);
  DebugSerial.print(" inputMux=");
  DebugSerial.print(DspMap::kInputMux);
  DebugSerial.print(" mute=");
  DebugSerial.print(DspMap::kMuteMain);
  DebugSerial.print("/");
  DebugSerial.print(DspMap::kMuteSub);
  DebugSerial.print(" rta=");
  DebugSerial.print(DspMap::kRtaLeft);
  DebugSerial.print("/");
  DebugSerial.print(DspMap::kRtaRight);
  DebugSerial.print(" lpGain=");
  DebugSerial.println(DspMap::kLpGain);
}

void printDspStatus() {
  printDspMap();
  printDspReg16("CORE_STATUS", DspMap::kRegCoreStatus);
  printDspReg16("PANIC_FLAG", DspMap::kRegPanicFlag);
  printDspReg16("PANIC_CODE", DspMap::kRegPanicCode);
  printDspReg16("ASRC_INPUT0", DspMap::kRegAsrcInput0);
  printDspReg16("ASRC_OUT_RATE0", DspMap::kRegAsrcOutRate0);
  printDspReg16("ASRC_LOCK", DspMap::kRegAsrcLock);
  printDspReg16("ASRC_MUTE", DspMap::kRegAsrcMute);
  printDspU32("PM_CHECKSUM", DspMap::kChecksumPm);
  printDspU32("DM0_CHECKSUM", DspMap::kChecksumDm0);
  printDspU32("DM1_CHECKSUM", DspMap::kChecksumDm1);
  printDspU32("MASTER", DspMap::kMasterVolume);
  printDspU32("RTA_L", DspMap::kRtaLeft);
  printDspU32("RTA_R", DspMap::kRtaRight);
}

uint32_t parseNumber(const String &token) {
  return strtoul(token.c_str(), nullptr, 0);
}

bool parseNumberStrict(const String &token, uint32_t &value) {
  if (token.length() == 0) {
    return false;
  }
  char *end = nullptr;
  value = strtoul(token.c_str(), &end, 0);
  return end != token.c_str() && *end == '\0';
}

bool parseDecimalStrict(const String &token, uint32_t &value) {
  if (token.length() == 0) {
    return false;
  }
  char *end = nullptr;
  value = strtoul(token.c_str(), &end, 10);
  return end != token.c_str() && *end == '\0';
}

String nextToken(String &line) {
  line.trim();
  int space = line.indexOf(' ');
  if (space < 0) {
    String token = line;
    line = "";
    return token;
  }
  String token = line.substring(0, space);
  line = line.substring(space + 1);
  return token;
}

bool isHexDigitChar(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int hexCharToInt(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

String replaceTurkish(String s);

String decodeHex(String hexStr) {
  String cleanHex;
  cleanHex.reserve(hexStr.length());
  for (uint16_t i = 0; i < hexStr.length(); i++) {
    char c = hexStr[i];
    if (isHexDigitChar(c)) {
      cleanHex += c;
    }
  }
  if ((cleanHex.length() & 1) != 0) {
    return hexStr;
  }

  String result;
  result.reserve(cleanHex.length() / 2);
  for (uint16_t i = 0; i < cleanHex.length(); i += 2) {
    result += static_cast<char>((hexCharToInt(cleanHex[i]) << 4) | hexCharToInt(cleanHex[i + 1]));
  }
  return result;
}

bool looksLikeHexPayload(const String &value) {
  if (value.length() == 0 || (value.length() & 1) != 0) {
    return false;
  }
  for (uint16_t i = 0; i < value.length(); i++) {
    if (!isHexDigitChar(value[i])) {
      return false;
    }
  }
  return true;
}

String decodeMetadataText(String value) {
  value.trim();
  if (!looksLikeHexPayload(value)) {
    return replaceTurkish(value);
  }

  String out;
  out.reserve(value.length() / 2);
  for (uint16_t i = 0; i + 1 < value.length(); i += 2) {
    uint8_t b = static_cast<uint8_t>((hexCharToInt(value[i]) << 4) | hexCharToInt(value[i + 1]));
    if (b < 0x80) {
      out += static_cast<char>(b);
      continue;
    }

    if (i + 3 >= value.length()) {
      continue;
    }
    uint8_t b2 = static_cast<uint8_t>((hexCharToInt(value[i + 2]) << 4) | hexCharToInt(value[i + 3]));
    if (b == 0xC3) {
      if (b2 == 0x87) out += 'C';
      else if (b2 == 0xA7) out += 'c';
      else if (b2 == 0x96) out += 'O';
      else if (b2 == 0xB6) out += 'o';
      else if (b2 == 0x9C) out += 'U';
      else if (b2 == 0xBC) out += 'u';
      else if (b2 == 0x82 || b2 == 0xA2) out += 'a';
      i += 2;
      continue;
    }
    if (b == 0xC4) {
      if (b2 == 0xB1) out += 'i';
      else if (b2 == 0xB0) out += 'I';
      else if (b2 == 0x9F) out += 'g';
      else if (b2 == 0x9E) out += 'G';
      i += 2;
      continue;
    }
    if (b == 0xC5) {
      if (b2 == 0x9F) out += 's';
      else if (b2 == 0x9E) out += 'S';
      i += 2;
      continue;
    }
  }
  return out;
}

String replaceTurkish(String s) {
  String out;
  out.reserve(s.length());
  for (uint16_t i = 0; i < s.length(); i++) {
    uint8_t c = static_cast<uint8_t>(s[i]);
    if (c < 0x80) {
      out += static_cast<char>(c);
      continue;
    }

    uint8_t next = (i + 1 < s.length()) ? static_cast<uint8_t>(s[i + 1]) : 0;
    if (c == 0xC3) {
      if (next == 0x87) out += 'C';       // C cedilla
      else if (next == 0xA7) out += 'c';
      else if (next == 0x96) out += 'O';
      else if (next == 0xB6) out += 'o';
      else if (next == 0x9C) out += 'U';
      else if (next == 0xBC) out += 'u';
      else if (next == 0xA2) out += 'a';
      else if (next == 0x82) out += 'A';
      else if (next == 0xAE) out += 'i';
      else if (next == 0x8E) out += 'I';
      i++;
      continue;
    }
    if (c == 0xC4) {
      if (next == 0xB1) out += 'i';       // dotless i
      else if (next == 0xB0) out += 'I';  // dotted capital I
      else if (next == 0x9F) out += 'g';
      else if (next == 0x9E) out += 'G';
      i++;
      continue;
    }
    if (c == 0xC5) {
      if (next == 0x9F) out += 's';
      else if (next == 0x9E) out += 'S';
      i++;
      continue;
    }
  }
  return out;
}

String extractJsonString(const String &json, const char *key) {
  String pattern = String("\"") + key + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) {
    return "";
  }
  int colon = json.indexOf(':', keyPos + pattern.length());
  if (colon < 0) {
    return "";
  }
  int firstQuote = json.indexOf('"', colon + 1);
  if (firstQuote < 0) {
    return "";
  }
  String value;
  for (int i = firstQuote + 1; i < json.length(); i++) {
    char c = json[i];
    if (c == '"' && json[i - 1] != '\\') {
      break;
    }
    value += c;
  }
  return value;
}

bool jsonHasKey(const String &json, const char *key) {
  String pattern = String("\"") + key + "\"";
  return json.indexOf(pattern) >= 0;
}

bool extractJsonUint(const String &json, const char *key, uint32_t &value) {
  String pattern = String("\"") + key + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) {
    return false;
  }
  int colon = json.indexOf(':', keyPos + pattern.length());
  if (colon < 0) {
    return false;
  }
  int start = colon + 1;
  while (start < json.length() && (json[start] == ' ' || json[start] == '"')) {
    start++;
  }
  int end = start;
  while (end < json.length() && isDigit(json[end])) {
    end++;
  }
  if (end == start) {
    return false;
  }
  value = strtoul(json.substring(start, end).c_str(), nullptr, 10);
  return true;
}

uint32_t normalizeLpTimeMs(uint32_t value) {
  return value;
}

String mapLpModeToName(const String &modeStr) {
  int mode = modeStr.toInt();
  switch (mode) {
    case 31: return "Spotify";
    case 1: return "AirPlay";
    case 10: return "Radio";
    case 11: return "USB";
    case 40: return "XLR";
    case 41: return "Bluetooth";
    case 43: return "Optical";
    case 48: return "XLR";
    case 99: return "Slave";
    case 0: return "Network";
    default: return "Network";
  }
}

DspSource sourceForLpMode(const String &modeStr) {
  int mode = modeStr.toInt();
  switch (mode) {
    case 40: return SOURCE_XLR;
    case 41: return SOURCE_BLUETOOTH;
    case 43: return SOURCE_OPTICAL;
    case 48: return SOURCE_XLR;
    default: return SOURCE_LINKPLAY;
  }
}

bool lpModeUsesNetworkMetadata(const String &modeStr) {
  int mode = modeStr.toInt();
  return mode == 0 || mode == 1 || mode == 10 || mode == 31;
}

String normalizeLpModeCode(const String &modeStr) {
  int mode = constrain(modeStr.toInt(), 0, 999);
  char buf[4];
  snprintf(buf, sizeof(buf), "%03d", mode);
  return String(buf);
}

const char *localSourceTitle(DspSource source) {
  switch (source) {
    case SOURCE_XLR: return "Playing from XLR";
    case SOURCE_BLUETOOTH: return "Playing from Bluetooth";
    case SOURCE_OPTICAL: return "Playing from Optical";
    default: return "";
  }
}

const char *sourceDisplayName(DspSource source) {
  switch (source) {
    case SOURCE_LINKPLAY: return "Network";
    case SOURCE_XLR: return "XLR";
    case SOURCE_BLUETOOTH: return "Bluetooth";
    case SOURCE_OPTICAL: return "Optical";
    default: return "Network";
  }
}

uint32_t dspMuxValueForSource(DspSource source) {
  return static_cast<uint32_t>(source) * 2UL;
}

const char *outputModeName(OutputMode mode) {
  switch (mode) {
    case OUTPUT_ADI_CLUB: return "Club";
    case OUTPUT_ADI_MUSIC: return "Music";
    case OUTPUT_ADI_STADIUM: return "Stadium";
    case OUTPUT_CINEMA: return "Cinema";
    case OUTPUT_ADI_HALL: return "Hall";
    case OUTPUT_ADI_ROCK: return "Rock";
    case OUTPUT_ADI_JAZZ: return "Jazz";
    case OUTPUT_BIAMP: return "Bi-Amp";
    default: return "Normal";
  }
}

bool outputModeIsValid(OutputMode mode) {
  switch (mode) {
    case OUTPUT_NORMAL:
    case OUTPUT_CINEMA:
    case OUTPUT_BIAMP:
    case OUTPUT_ADI_CLUB:
    case OUTPUT_ADI_MUSIC:
    case OUTPUT_ADI_STADIUM:
    case OUTPUT_ADI_HALL:
    case OUTPUT_ADI_ROCK:
    case OUTPUT_ADI_JAZZ:
      return true;
    default:
      return false;
  }
}

OutputMode normalizeOutputMode(OutputMode mode) {
  return outputModeIsValid(mode) ? mode : OUTPUT_NORMAL;
}

bool outputModeUsesAdiSurround(OutputMode mode) {
  switch (mode) {
    case OUTPUT_CINEMA:
    case OUTPUT_ADI_CLUB:
    case OUTPUT_ADI_MUSIC:
    case OUTPUT_ADI_STADIUM:
    case OUTPUT_ADI_HALL:
    case OUTPUT_ADI_ROCK:
    case OUTPUT_ADI_JAZZ:
      return true;
    default:
      return false;
  }
}

uint8_t outputModeMenuIndex(OutputMode mode) {
  for (uint8_t i = 0; i < OUTPUT_MODE_MENU_COUNT; i++) {
    if (kOutputModeMenuOrder[i] == mode) {
      return i;
    }
  }
  return 0;
}

OutputMode outputModeAdjusted(OutputMode mode, int delta) {
  int next = static_cast<int>(outputModeMenuIndex(mode)) + delta;
  while (next < 0) {
    next += OUTPUT_MODE_MENU_COUNT;
  }
  return kOutputModeMenuOrder[next % OUTPUT_MODE_MENU_COUNT];
}

uint32_t dspMuxValueForOutputMode(OutputMode mode) {
  if (outputModeUsesAdiSurround(mode)) {
    return 2;
  }
  return mode == OUTPUT_BIAMP ? 4 : 0;
}

struct AdiSurroundPreset {
  uint32_t gain1;
  uint32_t gain2;
  uint32_t gain3;
  uint32_t gain4;
  uint32_t dil;
};

const AdiSurroundPreset *adiSurroundPresetForMode(OutputMode mode) {
  static constexpr AdiSurroundPreset club = {0x01000000UL, 0x00000000UL, 0x002892C2UL, 0x0050F793UL, 0x000005A0UL};
  static constexpr AdiSurroundPreset music = {0x00B53BEFUL, 0x002892C2UL, 0x00404DE6UL, 0x00804DCEUL, 0x00000960UL};
  static constexpr AdiSurroundPreset stadium = {0x00804DCFUL, 0x005AD50DUL, 0x00804DCEUL, 0x00B53BEFUL, 0x00000B40UL};
  static constexpr AdiSurroundPreset cinema = {0x00B53BEFUL, 0x00804DCEUL, 0x00404DE6UL, 0x00B53BEFUL, 0x00000780UL};
  static constexpr AdiSurroundPreset hall = {0x0050F793UL, 0x006BF44DUL, 0x0055C04CUL, 0x00AB1896UL, 0x00000B40UL};
  static constexpr AdiSurroundPreset rock = {0x00B53BEFUL, 0x00000000UL, 0x00804DCEUL, 0x0050F793UL, 0x00000780UL};
  static constexpr AdiSurroundPreset jazz = {0x006BF44DUL, 0x00804DCEUL, 0x00404DE6UL, 0x00B53BEFUL, 0x000005A0UL};

  switch (mode) {
    case OUTPUT_ADI_CLUB: return &club;
    case OUTPUT_ADI_MUSIC: return &music;
    case OUTPUT_ADI_STADIUM: return &stadium;
    case OUTPUT_CINEMA: return &cinema;
    case OUTPUT_ADI_HALL: return &hall;
    case OUTPUT_ADI_ROCK: return &rock;
    case OUTPUT_ADI_JAZZ: return &jazz;
    default: return nullptr;
  }
}

bool applyAdiSurroundPreset(OutputMode mode) {
  const AdiSurroundPreset *preset = adiSurroundPresetForMode(mode);
  if (preset == nullptr) {
    return true;
  }
  bool ok = true;
  ok = writeDspU32(DspMap::kAdiSurroundGain1, preset->gain1) && ok;
  ok = writeDspU32(DspMap::kAdiSurroundGain2, preset->gain2) && ok;
  ok = writeDspU32(DspMap::kAdiSurroundGain3, preset->gain3) && ok;
  ok = writeDspU32(DspMap::kAdiSurroundGain4, preset->gain4) && ok;
  ok = writeDspU32(DspMap::kAdiSurroundDil, preset->dil) && ok;
  return ok;
}

void setOutputMode(OutputMode mode, bool persist) {
  currentOutputMode = normalizeOutputMode(mode);
  applyAdiSurroundPreset(currentOutputMode);
  uint32_t value = dspMuxValueForOutputMode(currentOutputMode);
  uint32_t centerMuteValue = outputModeUsesAdiSurround(currentOutputMode) ? 1UL : 0UL;
  writeDspU32(DspMap::kOutputMuxA, value);
  writeDspU32(DspMap::kOutputMuxB, value);
  writeDspU32(DspMap::kMuteCenter, centerMuteValue);
  if (persist) {
    scheduleAudioSettingsSave();
  }
}

uint32_t gainDbToSigma(int gainDb) {
  float gain = powf(10.0f, static_cast<float>(gainDb) / 20.0f);
  return dspFloatToFixed8_24(gain);
}

uint16_t inputGainAddress(DspSource source) {
  switch (source) {
    case SOURCE_XLR: return DspMap::kXlrGain;
    case SOURCE_BLUETOOTH: return DspMap::kBtGain;
    case SOURCE_OPTICAL: return DspMap::kOpticalGain;
    default: return DspMap::kLpGain;
  }
}

int *inputGainState(DspSource source) {
  switch (source) {
    case SOURCE_XLR: return &xlrGainDb;
    case SOURCE_BLUETOOTH: return &btGainDb;
    case SOURCE_OPTICAL: return &opticalGainDb;
    default: return &lpGainDb;
  }
}

int inputGainMaxDb(DspSource source) {
  if (source == SOURCE_XLR) return INPUT_GAIN_XLR_MAX_DB;
  if (source == SOURCE_BLUETOOTH) return INPUT_GAIN_BT_MAX_DB;
  return INPUT_GAIN_DEFAULT_MAX_DB;
}

int constrainInputGainDb(DspSource source, int gainDb) {
  return constrain(gainDb, INPUT_GAIN_MIN_DB, inputGainMaxDb(source));
}

void setInputGainDb(DspSource source, int gainDb, bool persist) {
  int *state = inputGainState(source);
  *state = constrainInputGainDb(source, gainDb);
  writeDspU32(inputGainAddress(source), gainDbToSigma(*state));
  if (persist) {
    scheduleAudioSettingsSave();
  }
}

void setBacklightPercent(uint8_t percent) {
  percent = constrain(static_cast<int>(percent), 0, 100);
  uint8_t duty = static_cast<uint8_t>(map(percent, 0, 100, 0, 255));
  analogWrite(TFT_BL_PIN, duty);
}

void applyBacklightForState() {
  setBacklightPercent(mainScreenVisible || splashActive || systemBootScreenVisible ? backlightOnPercent : backlightStandbyPercent);
}

void setDspSource(DspSource source, const String &modeName, bool redraw = true, bool persist = true) {
  bool sourceChanged = currentSource != source;
  currentSource = source;
  networkModeName = modeName;
  uint32_t dspMuxValue = dspMuxValueForSource(source);
  if (sourceChanged || !persist) {
  writeDspU32(DspMap::kInputMux, dspMuxValue);
  }

  if (source != SOURCE_LINKPLAY) {
    linkplayNetworkModeCode = "000";
    linkplayGroupRole = LINKPLAY_GROUP_NONE;
    localSourceTitleLocked = true;
    currentArtist = "";
    currentAlbum = "";
    if (source == SOURCE_BLUETOOTH && btConnectedDeviceName.length() > 0) {
      currentTitle = "Connected to " + btConnectedDeviceName;
    } else {
      currentTitle = localSourceTitle(source);
    }
    resetScroll();
  } else if (source == SOURCE_LINKPLAY && sourceChanged) {
    localSourceTitleLocked = false;
  }

  if (redraw && mainScreenVisible) {
    drawSourceHeader();
    drawScrollingTitle(true);
  }
  if (persist && sourceChanged) {
    scheduleAudioSettingsSave();
  }
  updateBtEnableForSource(sourceChanged && source == SOURCE_BLUETOOTH);
}

void setSourceFromLpMode(const String &mode, bool redraw = true, bool refreshMetadata = false) {
  DspSource nextSource = sourceForLpMode(mode);
  String modeName = mapLpModeToName(mode);
  if (nextSource == SOURCE_LINKPLAY) {
    linkplayNetworkModeCode = normalizeLpModeCode(mode);
    if (lpModeUsesNetworkMetadata(mode)) {
      modeName = "Network";
    }
  } else if (nextSource != SOURCE_LINKPLAY) {
    linkplayNetworkModeCode = "000";
  }
  setDspSource(nextSource, modeName, redraw);
  if (refreshMetadata && nextSource == SOURCE_LINKPLAY) {
    localSourceTitleLocked = false;
    requestLinkplayMetadata(true);
  }
}

bool isLeapYear(uint16_t year) {
  return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

uint8_t daysInMonthFor(uint16_t year, uint8_t month) {
  static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) {
    return 31;
  }
  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  return daysInMonth[month - 1];
}

uint32_t daysBeforeYear(uint16_t year) {
  uint32_t days = 0;
  for (uint16_t y = 1970; y < year; y++) {
    days += isLeapYear(y) ? 366UL : 365UL;
  }
  return days;
}

uint32_t dateTimeToEpoch(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  uint32_t days = daysBeforeYear(year);
  for (uint8_t m = 1; m < month; m++) {
    days += daysInMonthFor(year, m);
  }
  days += day > 0 ? day - 1 : 0;
  return (((days * 24UL) + hour) * 60UL + minute) * 60UL + second;
}

void epochToDateTime(uint32_t epoch, uint16_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second, uint8_t &weekday) {
  static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint32_t days = epoch / 86400UL;
  uint32_t secondsInDay = epoch % 86400UL;
  hour = secondsInDay / 3600UL;
  minute = (secondsInDay % 3600UL) / 60UL;
  second = secondsInDay % 60UL;

  weekday = static_cast<uint8_t>(((days + 3UL) % 7UL) + 1UL);  // RTC uses 1=Monday..7=Sunday.
  year = 1970;
  while (true) {
    uint16_t daysThisYear = isLeapYear(year) ? 366 : 365;
    if (days < daysThisYear) {
      break;
    }
    days -= daysThisYear;
    year++;
  }

  month = 1;
  while (month <= 12) {
    uint8_t dim = daysInMonth[month - 1];
    if (month == 2 && isLeapYear(year)) {
      dim++;
    }
    if (days < dim) {
      break;
    }
    days -= dim;
    month++;
  }
  day = static_cast<uint8_t>(days + 1);
}

uint8_t toBcd(uint8_t value) {
  return static_cast<uint8_t>(((value / 10U) << 4) | (value % 10U));
}

uint8_t fromBcd(uint8_t value) {
  return static_cast<uint8_t>(((value >> 4) * 10U) + (value & 0x0FU));
}

uint32_t currentClockEpoch() {
  if (!clockValid) {
    return 0;
  }
  uint32_t rtcEpoch = 0;
  if (readHardwareRtcEpoch(rtcEpoch)) {
    return rtcEpoch;
  }
  return clockEpochAtSync + ((millis() - clockMillisAtSync) / 1000UL);
}

void updateCurrentTimeString(bool force = false) {
  if (!clockValid) {
    if (currentTime.length() < 5) {
      currentTime = "00:00";
    }
    return;
  }
  uint32_t epoch = currentClockEpoch();
  int32_t minuteKey = static_cast<int32_t>(epoch / 60UL);
  if (!force && minuteKey == lastDisplayedClockMinute) {
    return;
  }
  lastDisplayedClockMinute = minuteKey;
  uint32_t secondsInDay = epoch % 86400UL;
  uint8_t hour = secondsInDay / 3600UL;
  uint8_t minute = (secondsInDay % 3600UL) / 60UL;
  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", hour, minute);
  currentTime = timeBuf;
}

void saveClockToEeprom(uint32_t epoch) {
  StoredClock stored = {CLOCK_STORE_MAGIC, epoch, ~epoch};
  uint32_t nextSequence = externalClockSequence + 1;
  uint32_t slotAddress = (nextSequence & 1U) ? EXTERNAL_CLOCK_SLOT_A : EXTERNAL_CLOCK_SLOT_B;
  bool ok = writeExternalStorageRecord(slotAddress, EXTERNAL_CLOCK_MAGIC, &stored, sizeof(stored), nextSequence);
  lastClockEepromEpoch = epoch;
  if (ok) {
    externalClockSequence = nextSequence;
  }
  DebugSerial.print("RTC ext EEPROM save epoch=");
  DebugSerial.print(epoch);
  DebugSerial.print(" seq=");
  DebugSerial.print(nextSequence);
  DebugSerial.print(" verify=");
  DebugSerial.println(ok ? "OK" : "ERR");
}

void flushClockIfDirty() {
  if (!clockDirty) {
    return;
  }
  saveClockToEeprom(currentClockEpoch());
  clockDirty = false;
}

void saveClockShutdownSnapshot() {
  if (!clockValid) {
    return;
  }
  uint32_t epoch = currentClockEpoch();
  uint32_t deltaSec = epoch > lastClockEepromEpoch ? epoch - lastClockEepromEpoch : lastClockEepromEpoch - epoch;
  if (lastClockEepromEpoch != 0 && deltaSec < CLOCK_EEPROM_SHUTDOWN_SNAPSHOT_SEC) {
    return;
  }
  DebugSerial.print("RTC shutdown snapshot deltaSec=");
  DebugSerial.println(deltaSec);
  saveClockToEeprom(epoch);
  clockDirty = false;
}

void loadClockFromEeprom() {
  StoredClock storedA = {};
  StoredClock storedB = {};
  uint32_t seqA = 0;
  uint32_t seqB = 0;
  bool okA = readExternalStorageRecord(EXTERNAL_CLOCK_SLOT_A, EXTERNAL_CLOCK_MAGIC, &storedA, sizeof(storedA), seqA);
  bool okB = readExternalStorageRecord(EXTERNAL_CLOCK_SLOT_B, EXTERNAL_CLOCK_MAGIC, &storedB, sizeof(storedB), seqB);
  StoredClock stored = {};
  if (okA && (!okB || seqA >= seqB)) {
    stored = storedA;
    externalClockSequence = seqA;
  } else if (okB) {
    stored = storedB;
    externalClockSequence = seqB;
  } else {
    EEPROM.get(0, stored);
    if (stored.magic == CLOCK_STORE_MAGIC && stored.inverseEpoch == ~stored.epoch && stored.epoch > 1700000000UL) {
      DebugSerial.println("RTC migrating internal EEPROM -> external EEPROM");
      externalClockSequence = 0;
      saveClockToEeprom(stored.epoch);
    }
  }
  bool storedValid = stored.magic == CLOCK_STORE_MAGIC &&
                     stored.inverseEpoch == ~stored.epoch &&
                     stored.epoch > 1700000000UL;
  uint32_t hardwareEpoch = 0;
  if (readHardwareRtcEpoch(hardwareEpoch)) {
    clockEpochAtSync = hardwareEpoch;
    clockMillisAtSync = millis();
    lastClockEepromEpoch = storedValid ? stored.epoch : hardwareEpoch;
    clockValid = true;
    DebugSerial.print("RTC restored from hardware epoch=");
    DebugSerial.println(hardwareEpoch);
    updateCurrentTimeString(true);
    return;
  }

  if (storedValid) {
    clockEpochAtSync = stored.epoch;
    clockMillisAtSync = millis();
    lastClockEepromEpoch = stored.epoch;
    clockValid = true;
    setHardwareRtcFromEpoch(stored.epoch);
    DebugSerial.print("RTC restored from ext EEPROM epoch=");
    DebugSerial.println(stored.epoch);
    updateCurrentTimeString(true);
  }
}

void printClockDebug() {
  StoredClock storedA = {};
  StoredClock storedB = {};
  StoredClock stored = {};
  uint32_t seqA = 0;
  uint32_t seqB = 0;
  bool okA = readExternalStorageRecord(EXTERNAL_CLOCK_SLOT_A, EXTERNAL_CLOCK_MAGIC, &storedA, sizeof(storedA), seqA);
  bool okB = readExternalStorageRecord(EXTERNAL_CLOCK_SLOT_B, EXTERNAL_CLOCK_MAGIC, &storedB, sizeof(storedB), seqB);
  if (okA && (!okB || seqA >= seqB)) {
    stored = storedA;
  } else if (okB) {
    stored = storedB;
  }
  uint32_t runningEpoch = currentClockEpoch();
  bool storedValid = (okA || okB) && stored.magic == CLOCK_STORE_MAGIC && stored.inverseEpoch == ~stored.epoch && stored.epoch > 1700000000UL;
  uint32_t hardwareEpoch = 0;
  bool hardwareValid = readHardwareRtcEpoch(hardwareEpoch);
  DebugSerial.print("RTC RAM valid=");
  DebugSerial.print(clockValid ? "1" : "0");
  DebugSerial.print(" epoch=");
  DebugSerial.print(runningEpoch);
  DebugSerial.print(" time=");
  updateCurrentTimeString(true);
  DebugSerial.print(currentTime);
  DebugSerial.print(" source=");
  DebugSerial.println(hardwareValid ? "RTC" : "millis");
  if (hardwareValid) {
    DebugSerial.print("RTC HW epoch=");
    DebugSerial.println(hardwareEpoch);
  }
  DebugSerial.print("RTC ext EEPROM valid=");
  DebugSerial.print(storedValid ? "1" : "0");
  DebugSerial.print(" epoch=");
  DebugSerial.print(stored.epoch);
  DebugSerial.print(" seq=");
  DebugSerial.print(okA && (!okB || seqA >= seqB) ? seqA : (okB ? seqB : 0));
  if (storedValid && clockValid) {
    DebugSerial.print(" deltaSec=");
    DebugSerial.print(static_cast<int32_t>(runningEpoch) - static_cast<int32_t>(stored.epoch));
  }
  DebugSerial.println();
}

bool initRtcClockSource() {
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  (void)RCC->APB1ENR;
  PWR->CR |= PWR_CR_DBP;

  uint32_t rtcSel = (RCC->BDCR & RCC_BDCR_RTCSEL_Msk) >> RCC_BDCR_RTCSEL_Pos;
  if (rtcSel != 0 && rtcSel != 1) {
    rtcHardwareReady = false;
    DebugSerial.print("RTC LSE init skipped: RTCSEL=");
    DebugSerial.println(rtcSel);
    return (RCC->BDCR & RCC_BDCR_LSERDY) != 0;
  }

  RCC->BDCR &= ~RCC_BDCR_LSEBYP;
  RCC->BDCR |= RCC_BDCR_LSEON;

  uint32_t startMs = millis();
  while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {
    if (millis() - startMs >= RTC_LSE_START_TIMEOUT_MS) {
      rtcHardwareReady = false;
      DebugSerial.println("RTC LSE init timeout");
      return false;
    }
    delay(10);
  }

  RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL_Msk) | RCC_BDCR_RTCSEL_0 | RCC_BDCR_RTCEN;
  rtcHardwareReady = true;
  DebugSerial.println("RTC LSE init OK");
  return true;
}

bool waitRtcInitMode(bool enter) {
  uint32_t startMs = millis();
  if (enter) {
    RTC->ISR |= RTC_ISR_INIT;
    while ((RTC->ISR & RTC_ISR_INITF) == 0) {
      if (millis() - startMs >= 1000) {
        return false;
      }
    }
  } else {
    RTC->ISR &= ~RTC_ISR_INIT;
  }
  return true;
}

bool setHardwareRtcFromEpoch(uint32_t epoch) {
  if (!rtcHardwareReady || epoch < 1700000000UL) {
    return false;
  }

  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  uint8_t weekday = 0;
  epochToDateTime(epoch, year, month, day, hour, minute, second, weekday);
  if (year < 2000 || year > 2099) {
    return false;
  }

  RTC->WPR = 0xCA;
  RTC->WPR = 0x53;
  if (!waitRtcInitMode(true)) {
    RTC->WPR = 0xFF;
    DebugSerial.println("RTC calendar write init timeout");
    return false;
  }

  RTC->PRER = (127UL << RTC_PRER_PREDIV_A_Pos) | 255UL;
  RTC->CR = (RTC->CR & ~RTC_CR_FMT) | RTC_CR_BYPSHAD;
  RTC->TR = (static_cast<uint32_t>(toBcd(hour)) << 16) |
            (static_cast<uint32_t>(toBcd(minute)) << 8) |
            toBcd(second);
  RTC->DR = (static_cast<uint32_t>(toBcd(static_cast<uint8_t>(year - 2000))) << 16) |
            (static_cast<uint32_t>(weekday & 0x07U) << 13) |
            (static_cast<uint32_t>(toBcd(month)) << 8) |
            toBcd(day);

  waitRtcInitMode(false);
  RTC->ISR &= ~RTC_ISR_RSF;
  RTC->WPR = 0xFF;
  return true;
}

bool readHardwareRtcEpoch(uint32_t &epoch) {
  if (!rtcHardwareReady || (RCC->BDCR & RCC_BDCR_LSERDY) == 0 || (RCC->BDCR & RCC_BDCR_RTCEN) == 0) {
    return false;
  }

  uint32_t tr1 = RTC->TR;
  uint32_t dr1 = RTC->DR;
  uint32_t tr2 = RTC->TR;
  uint32_t dr2 = RTC->DR;
  if (tr1 != tr2 || dr1 != dr2) {
    tr1 = RTC->TR;
    dr1 = RTC->DR;
  }

  uint8_t second = fromBcd(static_cast<uint8_t>(((tr1 & RTC_TR_ST) >> RTC_TR_ST_Pos) << 4 | ((tr1 & RTC_TR_SU) >> RTC_TR_SU_Pos)));
  uint8_t minute = fromBcd(static_cast<uint8_t>(((tr1 & RTC_TR_MNT) >> RTC_TR_MNT_Pos) << 4 | ((tr1 & RTC_TR_MNU) >> RTC_TR_MNU_Pos)));
  uint8_t hour = fromBcd(static_cast<uint8_t>(((tr1 & RTC_TR_HT) >> RTC_TR_HT_Pos) << 4 | ((tr1 & RTC_TR_HU) >> RTC_TR_HU_Pos)));
  uint8_t day = fromBcd(static_cast<uint8_t>(((dr1 & RTC_DR_DT) >> RTC_DR_DT_Pos) << 4 | ((dr1 & RTC_DR_DU) >> RTC_DR_DU_Pos)));
  uint8_t month = fromBcd(static_cast<uint8_t>(((dr1 & RTC_DR_MT) >> RTC_DR_MT_Pos) << 4 | ((dr1 & RTC_DR_MU) >> RTC_DR_MU_Pos)));
  uint16_t year = static_cast<uint16_t>(2000 + fromBcd(static_cast<uint8_t>(((dr1 & RTC_DR_YT) >> RTC_DR_YT_Pos) << 4 | ((dr1 & RTC_DR_YU) >> RTC_DR_YU_Pos))));

  if (year < 2020 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59) {
    return false;
  }
  epoch = dateTimeToEpoch(year, month, day, hour, minute, second);
  return true;
}

void printRtcClockDebug() {
  uint32_t bdcr = RCC->BDCR;
  uint32_t csr = RCC->CSR;
  uint32_t rtcSel = (bdcr & RCC_BDCR_RTCSEL_Msk) >> RCC_BDCR_RTCSEL_Pos;

  DebugSerial.print("RCC_BDCR=");
  printHex32(bdcr);
  DebugSerial.print(" LSEON=");
  DebugSerial.print((bdcr & RCC_BDCR_LSEON) ? "1" : "0");
  DebugSerial.print(" LSERDY=");
  DebugSerial.print((bdcr & RCC_BDCR_LSERDY) ? "1" : "0");
  DebugSerial.print(" LSEBYP=");
  DebugSerial.print((bdcr & RCC_BDCR_LSEBYP) ? "1" : "0");
  DebugSerial.print(" RTCEN=");
  DebugSerial.print((bdcr & RCC_BDCR_RTCEN) ? "1" : "0");
  DebugSerial.print(" RTCSEL=");
  DebugSerial.print(rtcSel);
  DebugSerial.print(" (");
  switch (rtcSel) {
    case 0:
      DebugSerial.print("none");
      break;
    case 1:
      DebugSerial.print("LSE");
      break;
    case 2:
      DebugSerial.print("LSI");
      break;
    case 3:
      DebugSerial.print("HSE/128");
      break;
    default:
      DebugSerial.print("unknown");
      break;
  }
  DebugSerial.println(")");

  DebugSerial.print("RCC_CSR=");
  printHex32(csr);
  DebugSerial.print(" LSION=");
  DebugSerial.print((csr & RCC_CSR_LSION) ? "1" : "0");
  DebugSerial.print(" LSIRDY=");
  DebugSerial.println((csr & RCC_CSR_LSIRDY) ? "1" : "0");

  DebugSerial.print("RTC_ISR=");
  printHex32(RTC->ISR);
  DebugSerial.print(" RTC_PRER=");
  printHex32(RTC->PRER);
  DebugSerial.print(" RTC_TR=");
  printHex32(RTC->TR);
  DebugSerial.print(" RTC_DR=");
  printHex32(RTC->DR);
  DebugSerial.print(" HWREADY=");
  DebugSerial.println(rtcHardwareReady ? "1" : "0");
}

uint8_t audioSettingsChecksum(const StoredAudioSettings &settings) {
  return static_cast<uint8_t>(
      (settings.magic >> 24) ^ (settings.magic >> 16) ^ (settings.magic >> 8) ^ settings.magic ^
      settings.volume ^ static_cast<uint8_t>(settings.bass) ^ static_cast<uint8_t>(settings.treble) ^
      static_cast<uint8_t>(settings.lpGain) ^ static_cast<uint8_t>(settings.xlrGain) ^
      static_cast<uint8_t>(settings.btGain) ^ static_cast<uint8_t>(settings.opticalGain) ^
      settings.outputMode ^ settings.backlightOn ^ settings.backlightStandby ^ settings.subEnabled ^
      settings.source);
}

void saveAudioSettingsToEeprom() {
  StoredAudioSettings settings = {
      AUDIO_SETTINGS_MAGIC,
      static_cast<uint8_t>(constrain(static_cast<int>(displayedVolumePercent), 0, 100)),
      static_cast<int8_t>(constrain(bassGainDb, -12, 12)),
      static_cast<int8_t>(constrain(trebleGainDb, -12, 12)),
      static_cast<int8_t>(constrainInputGainDb(SOURCE_LINKPLAY, lpGainDb)),
      static_cast<int8_t>(constrainInputGainDb(SOURCE_XLR, xlrGainDb)),
      static_cast<int8_t>(constrainInputGainDb(SOURCE_BLUETOOTH, btGainDb)),
      static_cast<int8_t>(constrainInputGainDb(SOURCE_OPTICAL, opticalGainDb)),
      static_cast<uint8_t>(normalizeOutputMode(currentOutputMode)),
      static_cast<uint8_t>(constrain(static_cast<int>(backlightOnPercent), 5, 100)),
      static_cast<uint8_t>(constrain(static_cast<int>(backlightStandbyPercent), 0, 100)),
      static_cast<uint8_t>((subEnabled ? 1 : 0) | (screenLightTheme ? 2 : 0)),
      static_cast<uint8_t>(constrain(static_cast<int>(currentSource), 0, 3)),
      0,
  };
  settings.checksum = audioSettingsChecksum(settings);

  StoredAudioSettings currentA = {};
  StoredAudioSettings currentB = {};
  uint32_t seqA = 0;
  uint32_t seqB = 0;
  bool okA = readExternalStorageRecord(EXTERNAL_AUDIO_SLOT_A, EXTERNAL_AUDIO_MAGIC, &currentA, sizeof(currentA), seqA);
  bool okB = readExternalStorageRecord(EXTERNAL_AUDIO_SLOT_B, EXTERNAL_AUDIO_MAGIC, &currentB, sizeof(currentB), seqB);
  StoredAudioSettings current = {};
  bool haveCurrent = false;
  if (okA && (!okB || seqA >= seqB)) {
    current = currentA;
    haveCurrent = true;
  } else if (okB) {
    current = currentB;
    haveCurrent = true;
  }
  if (!haveCurrent || memcmp(&current, &settings, sizeof(settings)) != 0) {
    uint32_t nextSequence = externalAudioSequence + 1;
    uint32_t slotAddress = (nextSequence & 1U) ? EXTERNAL_AUDIO_SLOT_A : EXTERNAL_AUDIO_SLOT_B;
    bool ok = writeExternalStorageRecord(slotAddress, EXTERNAL_AUDIO_MAGIC, &settings, sizeof(settings), nextSequence);
    if (ok) {
      externalAudioSequence = nextSequence;
    }
    DebugSerial.print("Audio ext EEPROM save seq=");
    DebugSerial.print(nextSequence);
    DebugSerial.print(" verify=");
    DebugSerial.println(ok ? "OK" : "ERR");
  }
  audioSettingsDirty = false;
}

void scheduleAudioSettingsSave() {
  audioSettingsDirty = true;
  lastAudioSettingsChangeMs = millis();
}

void loadAudioSettingsFromEeprom() {
  StoredAudioSettings settingsA = {};
  StoredAudioSettings settingsB = {};
  uint32_t seqA = 0;
  uint32_t seqB = 0;
  bool okA = readExternalStorageRecord(EXTERNAL_AUDIO_SLOT_A, EXTERNAL_AUDIO_MAGIC, &settingsA, sizeof(settingsA), seqA);
  bool okB = readExternalStorageRecord(EXTERNAL_AUDIO_SLOT_B, EXTERNAL_AUDIO_MAGIC, &settingsB, sizeof(settingsB), seqB);
  StoredAudioSettings settings = {};
  if (okA && (!okB || seqA >= seqB)) {
    settings = settingsA;
    externalAudioSequence = seqA;
  } else if (okB) {
    settings = settingsB;
    externalAudioSequence = seqB;
  } else {
    EEPROM.get(AUDIO_SETTINGS_EEPROM_ADDR, settings);
    if (settings.magic == AUDIO_SETTINGS_MAGIC && settings.checksum == audioSettingsChecksum(settings)) {
      DebugSerial.println("Audio settings migrating internal EEPROM -> external EEPROM");
      externalAudioSequence = 0;
      uint32_t slotAddress = EXTERNAL_AUDIO_SLOT_A;
      if (writeExternalStorageRecord(slotAddress, EXTERNAL_AUDIO_MAGIC, &settings, sizeof(settings), 1)) {
        externalAudioSequence = 1;
      }
    }
  }
  if (settings.magic != AUDIO_SETTINGS_MAGIC || settings.checksum != audioSettingsChecksum(settings)) {
    saveAudioSettingsToEeprom();
    return;
  }
  displayedVolumePercent = constrain(static_cast<int>(settings.volume), 0, 100);
  bassGainDb = constrain(static_cast<int>(settings.bass), -12, 12);
  trebleGainDb = constrain(static_cast<int>(settings.treble), -12, 12);
  lpGainDb = constrainInputGainDb(SOURCE_LINKPLAY, settings.lpGain);
  xlrGainDb = constrainInputGainDb(SOURCE_XLR, settings.xlrGain);
  btGainDb = constrainInputGainDb(SOURCE_BLUETOOTH, settings.btGain);
  opticalGainDb = constrainInputGainDb(SOURCE_OPTICAL, settings.opticalGain);
  currentOutputMode = normalizeOutputMode(static_cast<OutputMode>(settings.outputMode));
  backlightOnPercent = constrain(static_cast<int>(settings.backlightOn), 5, 100);
  backlightStandbyPercent = constrain(static_cast<int>(settings.backlightStandby), 0, 100);
  subEnabled = (settings.subEnabled & 0x01) != 0;
  screenLightTheme = (settings.subEnabled & 0x02) != 0;
  currentSource = static_cast<DspSource>(constrain(static_cast<int>(settings.source), 0, 3));
}

void saveScreenSettingsToEeprom() {
  StoredScreenSettings settings = {
      static_cast<uint8_t>(screenPanelType),
      static_cast<uint8_t>(screenRotationDegrees == 180 ? 180 : 0),
      {0xFF, 0xFF},
  };
  uint32_t nextSequence = externalScreenSequence + 1;
  uint32_t slotAddress = (nextSequence & 1U) ? EXTERNAL_SCREEN_SLOT_A : EXTERNAL_SCREEN_SLOT_B;
  bool ok = writeExternalStorageRecord(slotAddress, EXTERNAL_SCREEN_MAGIC, &settings, sizeof(settings), nextSequence);
  if (ok) {
    externalScreenSequence = nextSequence;
  }
  DebugSerial.print("Screen ext EEPROM save seq=");
  DebugSerial.print(nextSequence);
  DebugSerial.print(" verify=");
  DebugSerial.println(ok ? "OK" : "ERR");
}

void loadScreenSettingsFromEeprom() {
  StoredScreenSettings settingsA = {};
  StoredScreenSettings settingsB = {};
  uint32_t seqA = 0;
  uint32_t seqB = 0;
  bool okA = readExternalStorageRecord(EXTERNAL_SCREEN_SLOT_A, EXTERNAL_SCREEN_MAGIC, &settingsA, sizeof(settingsA), seqA);
  bool okB = readExternalStorageRecord(EXTERNAL_SCREEN_SLOT_B, EXTERNAL_SCREEN_MAGIC, &settingsB, sizeof(settingsB), seqB);
  StoredScreenSettings settings = {};
  bool haveSettings = false;
  if (okA && (!okB || seqA >= seqB)) {
    settings = settingsA;
    externalScreenSequence = seqA;
    haveSettings = true;
  } else if (okB) {
    settings = settingsB;
    externalScreenSequence = seqB;
    haveSettings = true;
  }
  if (!haveSettings) {
    return;
  }
  ScreenPanelType panel = settings.panelType == SCREEN_PANEL_20 ? SCREEN_PANEL_20 : SCREEN_PANEL_24;
  uint16_t rotation = settings.rotation == 180 ? 180 : 0;
  setScreenPanelConfig(panel, rotation, false);
}

void printScreenSettings() {
  DebugSerial.print("SCREEN panel=");
  DebugSerial.print(screenPanelType == SCREEN_PANEL_20 ? "2.0" : "2.4");
  DebugSerial.print(" rotation=");
  DebugSerial.print(screenRotationDegrees);
  DebugSerial.print(" madctl=0x");
  printHexByte(tftMadctlValue());
  DebugSerial.print(" invert=");
  DebugSerial.print(tftInvertColors() ? "ON" : "OFF");
  DebugSerial.print(" theme=");
  DebugSerial.println(screenLightTheme ? "Light" : "Dark");
}

bool setScreenPanelConfig(ScreenPanelType panel, uint16_t rotationDegrees, bool persist) {
  if (rotationDegrees != 0 && rotationDegrees != 180) {
    return false;
  }
  screenPanelType = panel;
  screenRotationDegrees = rotationDegrees;
  if (persist) {
    saveScreenSettingsToEeprom();
    tft.begin();
    redrawActiveUi();
  }
  return true;
}

void applyStoredAudioSettingsToDsp() {
  writeDspU32(DspMap::kMasterVolume, volumePercentToSigma(displayedVolumePercent));
  setDspSource(currentSource, sourceDisplayName(currentSource), false, false);
  writeDspU32(DspMap::kBassGainDb, dspFloatToFixed8_24(static_cast<float>(bassGainDb)));
  writeDspU32(DspMap::kTrebleGainDb, dspFloatToFixed8_24(static_cast<float>(trebleGainDb)));
  setOutputMode(currentOutputMode, false);
  setInputGainDb(SOURCE_LINKPLAY, lpGainDb, false);
  setInputGainDb(SOURCE_XLR, xlrGainDb, false);
  setInputGainDb(SOURCE_BLUETOOTH, btGainDb, false);
  setInputGainDb(SOURCE_OPTICAL, opticalGainDb, false);
  setSubEnabled(subEnabled, false);
}

void syncClockFromRtcString(const String &rtc) {
  if (rtc.length() < 14) {
    return;
  }
  uint16_t year = rtc.substring(0, 4).toInt();
  uint8_t month = rtc.substring(4, 6).toInt();
  uint8_t day = rtc.substring(6, 8).toInt();
  uint8_t hour = rtc.substring(8, 10).toInt();
  uint8_t minute = rtc.substring(10, 12).toInt();
  uint8_t second = rtc.substring(12, 14).toInt();
  if (year < 2020 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59) {
    return;
  }
  uint32_t previousEpoch = clockValid ? currentClockEpoch() : 0;
  uint32_t newEpoch = dateTimeToEpoch(year, month, day, hour, minute, second);
  uint32_t correctionSec = clockValid
      ? (newEpoch > previousEpoch ? newEpoch - previousEpoch : previousEpoch - newEpoch)
      : CLOCK_EEPROM_SAVE_DRIFT_SEC;
  clockEpochAtSync = newEpoch;
  clockMillisAtSync = millis();
  clockValid = true;
  setHardwareRtcFromEpoch(newEpoch);
  updateCurrentTimeString(true);
  DebugSerial.print("RTC sync epoch=");
  DebugSerial.print(clockEpochAtSync);
  DebugSerial.print(" correctionSec=");
  DebugSerial.println(correctionSec);

  bool shouldSaveSnapshot =
      lastClockEepromEpoch == 0 ||
      correctionSec >= CLOCK_EEPROM_SAVE_DRIFT_SEC;
  if (shouldSaveSnapshot) {
    if (!mainScreenVisible && !splashActive) {
      saveClockToEeprom(clockEpochAtSync);
      clockDirty = false;
    } else {
      clockDirty = true;
      DebugSerial.println("RTC EEPROM save deferred");
    }
  }
}

void adjustClockMinutes(int deltaMinutes) {
  if (!clockValid) {
    clockEpochAtSync = dateTimeToEpoch(2026, 1, 1, 0, 0, 0);
    clockMillisAtSync = millis();
    clockValid = true;
  }
  uint32_t epoch = currentClockEpoch();
  int32_t secondsInDay = static_cast<int32_t>(epoch % 86400UL) + (deltaMinutes * 60);
  while (secondsInDay < 0) {
    secondsInDay += 86400;
    epoch -= 86400UL;
  }
  while (secondsInDay >= 86400) {
    secondsInDay -= 86400;
    epoch += 86400UL;
  }
  clockEpochAtSync = (epoch - (epoch % 86400UL)) + static_cast<uint32_t>(secondsInDay);
  clockMillisAtSync = millis();
  setHardwareRtcFromEpoch(clockEpochAtSync);
  updateCurrentTimeString(true);
  clockDirty = true;
}

void setClockDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  year = constrain(static_cast<int>(year), 2020, 2099);
  month = constrain(static_cast<int>(month), 1, 12);
  day = constrain(static_cast<int>(day), 1, daysInMonthFor(year, month));
  uint32_t epoch = dateTimeToEpoch(year, month, day, hour, minute, second);
  clockEpochAtSync = epoch;
  clockMillisAtSync = millis();
  clockValid = true;
  setHardwareRtcFromEpoch(epoch);
  updateCurrentTimeString(true);
  clockDirty = true;
}

void currentClockParts(uint16_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second) {
  uint8_t weekday = 1;
  uint32_t epoch = clockValid ? currentClockEpoch() : dateTimeToEpoch(2026, 1, 1, 0, 0, 0);
  epochToDateTime(epoch, year, month, day, hour, minute, second, weekday);
}

void adjustClockDays(int deltaDays) {
  if (!clockValid) {
    setClockDateTime(2026, 1, 1, 0, 0, 0);
  }
  uint32_t epoch = currentClockEpoch();
  if (deltaDays < 0) {
    uint32_t days = static_cast<uint32_t>(-deltaDays);
    uint32_t deltaSeconds = days * 86400UL;
    epoch = epoch > deltaSeconds ? epoch - deltaSeconds : 0;
  } else {
    epoch += static_cast<uint32_t>(deltaDays) * 86400UL;
  }
  clockEpochAtSync = epoch;
  clockMillisAtSync = millis();
  setHardwareRtcFromEpoch(epoch);
  updateCurrentTimeString(true);
  clockDirty = true;
}

void adjustClockMonths(int deltaMonths) {
  uint16_t year = 2026;
  uint8_t month = 1;
  uint8_t day = 1;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  currentClockParts(year, month, day, hour, minute, second);

  int totalMonths = static_cast<int>(year) * 12 + static_cast<int>(month) - 1 + deltaMonths;
  if (totalMonths < 2020 * 12) {
    totalMonths = 2020 * 12;
  }
  if (totalMonths > 2099 * 12 + 11) {
    totalMonths = 2099 * 12 + 11;
  }
  year = static_cast<uint16_t>(totalMonths / 12);
  month = static_cast<uint8_t>((totalMonths % 12) + 1);
  setClockDateTime(year, month, day, hour, minute, second);
}

void adjustClockYears(int deltaYears) {
  uint16_t year = 2026;
  uint8_t month = 1;
  uint8_t day = 1;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  currentClockParts(year, month, day, hour, minute, second);
  year = constrain(static_cast<int>(year) + deltaYears, 2020, 2099);
  setClockDateTime(year, month, day, hour, minute, second);
}

void requestClockNtpFromMenu() {
  if (!linkplayWifiReady) {
    DebugSerial.println("RTC NTP skipped: WiFi not ready");
    return;
  }
  DebugSerial.println("RTC NTP request sent");
  requestLinkplayRtcTime();
}

void requestLinkplayWifiStatus() {
  linkplayWifiStatusRequestPending = true;
  sendLinkplayCommand("MCU+WWW+GET");
}

void handleLinkplayWifiState(bool ready) {
  linkplayWifiReady = ready;
  if (linkplayWifiStatusRequestPending) {
    linkplayWifiStatusRequestPending = false;
    DebugSerial.print("LP WiFi=");
    DebugSerial.println(linkplayWifiReady ? "READY" : "NO_WIFI");
  }
  if (linkplayWifiReady) {
    finishSystemBootScreen();
  } else if (systemBootScreenVisible && !ampEnabled) {
    updateSystemBootScreen("Setup WiFi in app", 3);
  }
  drawStandbyWifiIcon();
}

void resetScroll() {
  lastScrollText = "";
  scrollOffset = 320;
  scrollIdle = true;
  scrollIdleStartMs = millis();
}

void logLinkplayRaw(const char *direction, const String &payload) {
  if (!linkplayRawLogEnabled || payload.length() == 0) {
    return;
  }
  DebugSerial.print("[");
  DebugSerial.print(millis());
  DebugSerial.print("] LP ");
  DebugSerial.print(direction);
  DebugSerial.print(": ");
  DebugSerial.println(payload);
}

void sendLinkplayCommand(const String &cmd) {
  logLinkplayRaw("TX", cmd);
  LinkplaySerial.print(cmd);
  LinkplaySerial.print("\r\n");
}

void blinkMuteFeedback(uint8_t count, uint16_t intervalMs) {
  for (uint8_t i = 0; i < count; i++) {
    setAmpMute(false);
    delay(intervalMs);
    setAmpMute(true);
    delay(intervalMs);
  }
}

void finishLinkplayHardOffRequest(const char *result, uint8_t blinkCount, uint16_t intervalMs) {
  if (!linkplayHardOffPending) {
    return;
  }
  linkplayHardOffPending = false;
  DebugSerial.print("LP hard-off ");
  DebugSerial.println(result);
  DebugSerial.print("LP hard-off feedback blinks=");
  DebugSerial.print(blinkCount);
  DebugSerial.print(" intervalMs=");
  DebugSerial.println(intervalMs);
  if (strcmp(result, "READY") == 0) {
    drawHardOffScreen("POWER OFF", "Unplug power now");
  } else if (strcmp(result, "BURNING") == 0) {
    drawHardOffScreen("UPDATE ACTIVE", "Do not unplug power");
  } else {
    drawHardOffScreen("POWER OFF", "Wait and retry");
  }
  blinkMuteFeedback(blinkCount, intervalMs);
  setAmpMute(true);
}

void startLinkplayHardOffRequest() {
  if (ampEnabled || splashActive) {
    DebugSerial.println("ERR: hard-off request only allowed in standby");
    return;
  }
  if (linkplayHardOffPending) {
    DebugSerial.println("LP hard-off already pending");
    return;
  }
  linkplayHardOffPending = true;
  linkplayHardOffDeadlineMs = millis() + LINKPLAY_HARD_OFF_TIMEOUT_MS;
  btTakeoverPending = false;
  btTransportPlaying = false;
  pauseBtForStandby();
  setBtModuleEnable(false);
  saveClockShutdownSnapshot();
  linkplayTransportPlaying = false;
  linkplayMetadataPending = false;
  linkplayPinfPending = false;
  playbackState = "pause";
  setAmpMute(true);
  drawHardOffScreen("Powering Off", "Waiting for Linkplay");
  DebugSerial.print("LP hard-off request timeoutMs=");
  DebugSerial.println(LINKPLAY_HARD_OFF_TIMEOUT_MS);
  sendLinkplayCommand("MCU+POW+OFF");
}

void pollLinkplayHardOffRequest() {
  if (!linkplayHardOffPending) {
    return;
  }
  if (static_cast<int32_t>(millis() - linkplayHardOffDeadlineMs) >= 0) {
    finishLinkplayHardOffRequest("TIMEOUT", 4, HARD_OFF_ALERT_BLINK_MS);
  }
}

void sendBtCommand(const String &cmd) {
  if (btRawLogEnabled) {
    DebugSerial.print("[");
    DebugSerial.print(millis());
    DebugSerial.print("] BT TX: ");
    DebugSerial.println(cmd);
  }
  BtSerial.print(cmd);
  BtSerial.print("\r\n");
}

void setBtModuleEnable(bool enabled) {
  btModuleEnabled = enabled;
  digitalWrite(BT_ENABLE_PIN, enabled ? HIGH : LOW);
  if (!enabled) {
    btTransportPlaying = false;
    btPauseSettleActive = false;
    btTakeoverPending = false;
    btStandbyDisconnectPending = false;
    btWaitingSplashShown = false;
    btNameRequestPending = false;
    btReconnectPending = false;
    btConnectedDeviceName = "";
  }
  DebugSerial.print("BT_ENABLE ");
  DebugSerial.println(enabled ? "ON" : "OFF");
}

bool btStatusConnected() {
  return digitalRead(BT_STATUS_PIN) == HIGH;
}

void printBtStatus() {
  DebugSerial.print("BT_STATUS=");
  DebugSerial.print(btStatusConnected() ? "HIGH" : "LOW");
  DebugSerial.print(" BT_ENABLE=");
  DebugSerial.println(btModuleEnabled ? "ON" : "OFF");
}

void prepareExternalEepromBitBangPins() {
  digitalWrite(FLASH_CS_PIN, HIGH);
  digitalWrite(FLASH_ALT_CS_PIN, HIGH);
  pinMode(FLASH_CS_PIN, OUTPUT);
  pinMode(FLASH_ALT_CS_PIN, OUTPUT);
  pinMode(FLASH_SPI_SCK, OUTPUT);
  pinMode(FLASH_SPI_MOSI, OUTPUT);
  pinMode(FLASH_SPI_MISO, INPUT);
  digitalWrite(FLASH_SPI_SCK, LOW);
  digitalWrite(FLASH_SPI_MOSI, LOW);
  delayMicroseconds(5);
}

uint8_t externalEepromBitBangTransfer(uint8_t value) {
  uint8_t incoming = 0;
  for (int8_t bit = 7; bit >= 0; bit--) {
    digitalWrite(FLASH_SPI_MOSI, (value & (1U << bit)) ? HIGH : LOW);
    delayMicroseconds(2);
    digitalWrite(FLASH_SPI_SCK, HIGH);
    delayMicroseconds(2);
    incoming = static_cast<uint8_t>((incoming << 1) | (digitalRead(FLASH_SPI_MISO) == HIGH ? 1 : 0));
    digitalWrite(FLASH_SPI_SCK, LOW);
    delayMicroseconds(2);
  }
  return incoming;
}

uint8_t externalEepromBitBangReadStatus() {
  prepareExternalEepromBitBangPins();
  digitalWrite(FLASH_CS_PIN, LOW);
  delayMicroseconds(2);
  externalEepromBitBangTransfer(0x05);
  uint8_t status = externalEepromBitBangTransfer(0x00);
  digitalWrite(FLASH_CS_PIN, HIGH);
  delayMicroseconds(5);
  return status;
}

void externalEepromBitBangCommand(uint8_t command) {
  prepareExternalEepromBitBangPins();
  digitalWrite(FLASH_CS_PIN, LOW);
  delayMicroseconds(2);
  externalEepromBitBangTransfer(command);
  digitalWrite(FLASH_CS_PIN, HIGH);
  delayMicroseconds(5);
}

void printExternalEepromBitBangStatus() {
  uint8_t before = externalEepromBitBangReadStatus();
  externalEepromBitBangCommand(0x06);
  uint8_t afterWren = externalEepromBitBangReadStatus();
  externalEepromBitBangCommand(0x04);
  uint8_t afterWrdi = externalEepromBitBangReadStatus();

  DebugSerial.print("EEPROM BB RDSR before=0x");
  if (before < 0x10) DebugSerial.print('0');
  DebugSerial.print(before, HEX);
  DebugSerial.print(" afterWREN=0x");
  if (afterWren < 0x10) DebugSerial.print('0');
  DebugSerial.print(afterWren, HEX);
  DebugSerial.print(" afterWRDI=0x");
  if (afterWrdi < 0x10) DebugSerial.print('0');
  DebugSerial.println(afterWrdi, HEX);
}

void externalEepromBitBangSendAddress(uint32_t address) {
  externalEepromBitBangTransfer(static_cast<uint8_t>((address >> 16) & 0xFF));
  externalEepromBitBangTransfer(static_cast<uint8_t>((address >> 8) & 0xFF));
  externalEepromBitBangTransfer(static_cast<uint8_t>(address & 0xFF));
}

void externalEepromBitBangRead(uint32_t address, uint8_t *data, size_t length) {
  prepareExternalEepromBitBangPins();
  digitalWrite(FLASH_CS_PIN, LOW);
  delayMicroseconds(2);
  externalEepromBitBangTransfer(0x03);
  externalEepromBitBangSendAddress(address);
  for (size_t i = 0; i < length; i++) {
    data[i] = externalEepromBitBangTransfer(0x00);
  }
  digitalWrite(FLASH_CS_PIN, HIGH);
  delayMicroseconds(5);
}

bool externalEepromBitBangWaitReady(uint32_t timeoutMs = 20) {
  uint32_t startMs = millis();
  while (externalEepromBitBangReadStatus() & 0x01) {
    if (millis() - startMs >= timeoutMs) {
      return false;
    }
    delay(1);
  }
  return true;
}

bool externalEepromBitBangWrite(uint32_t address, const uint8_t *data, size_t length) {
  if (length == 0) {
    return true;
  }
  externalEepromBitBangCommand(0x06);
  if ((externalEepromBitBangReadStatus() & 0x02) == 0) {
    return false;
  }

  prepareExternalEepromBitBangPins();
  digitalWrite(FLASH_CS_PIN, LOW);
  delayMicroseconds(2);
  externalEepromBitBangTransfer(0x02);
  externalEepromBitBangSendAddress(address);
  for (size_t i = 0; i < length; i++) {
    externalEepromBitBangTransfer(data[i]);
  }
  digitalWrite(FLASH_CS_PIN, HIGH);
  delayMicroseconds(5);
  return externalEepromBitBangWaitReady();
}

void printHexByte(uint8_t value) {
  if (value < 0x10) DebugSerial.print('0');
  DebugSerial.print(value, HEX);
}

void printHexBytes(const uint8_t *data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (i > 0) DebugSerial.print(' ');
    printHexByte(data[i]);
  }
}

uint8_t externalStorageChecksum(ExternalStorageHeader header, const uint8_t *payload) {
  header.checksum = 0;
  uint8_t sum = 0;
  const uint8_t *headerBytes = reinterpret_cast<const uint8_t *>(&header);
  for (size_t i = 0; i < sizeof(header); i++) {
    sum = static_cast<uint8_t>(sum + headerBytes[i]);
  }
  for (uint16_t i = 0; i < header.length; i++) {
    sum = static_cast<uint8_t>(sum + payload[i]);
  }
  return static_cast<uint8_t>(~sum);
}

bool readExternalStorageRecord(uint32_t slotAddress, uint32_t magic, void *payload, uint16_t payloadSize, uint32_t &sequence) {
  ExternalStorageHeader header = {};
  externalEepromBitBangRead(slotAddress, reinterpret_cast<uint8_t *>(&header), sizeof(header));
  if (header.magic != magic ||
      header.version != EXTERNAL_STORAGE_VERSION ||
      header.length != payloadSize ||
      header.length > (EXTERNAL_STORAGE_SLOT_SIZE - sizeof(ExternalStorageHeader))) {
    return false;
  }
  externalEepromBitBangRead(slotAddress + sizeof(header), reinterpret_cast<uint8_t *>(payload), header.length);
  if (header.checksum != externalStorageChecksum(header, reinterpret_cast<const uint8_t *>(payload))) {
    return false;
  }
  sequence = header.sequence;
  return true;
}

bool writeExternalStorageRecord(uint32_t slotAddress, uint32_t magic, const void *payload, uint16_t payloadSize, uint32_t sequence) {
  if (payloadSize > (EXTERNAL_STORAGE_SLOT_SIZE - sizeof(ExternalStorageHeader))) {
    return false;
  }
  uint8_t record[EXTERNAL_STORAGE_SLOT_SIZE];
  memset(record, 0xFF, sizeof(record));
  ExternalStorageHeader header = {
      magic,
      sequence,
      EXTERNAL_STORAGE_VERSION,
      payloadSize,
      0,
      {0xFF, 0xFF, 0xFF},
  };
  header.checksum = externalStorageChecksum(header, reinterpret_cast<const uint8_t *>(payload));
  memcpy(record, &header, sizeof(header));
  memcpy(record + sizeof(header), payload, payloadSize);
  return externalEepromBitBangWrite(slotAddress, record, sizeof(header) + payloadSize);
}

void testExternalEepromBitBang() {
  static constexpr uint32_t TEST_ADDRESS = 0x1FE00UL;
  const uint8_t pattern[] = {0x48, 0x47, 0x54, 0x43, 0xA5, 0x5A, 0x3C, 0xC3};
  uint8_t before[sizeof(pattern)] = {};
  uint8_t after[sizeof(pattern)] = {};

  externalEepromBitBangRead(TEST_ADDRESS, before, sizeof(before));
  bool okWrite = externalEepromBitBangWrite(TEST_ADDRESS, pattern, sizeof(pattern));
  externalEepromBitBangRead(TEST_ADDRESS, after, sizeof(after));
  bool okVerify = okWrite && memcmp(pattern, after, sizeof(pattern)) == 0;

  DebugSerial.print("EEPROM BB TEST addr=0x");
  DebugSerial.print(TEST_ADDRESS, HEX);
  DebugSerial.print(" before=");
  printHexBytes(before, sizeof(before));
  DebugSerial.print(" after=");
  printHexBytes(after, sizeof(after));
  DebugSerial.print(" verify=");
  DebugSerial.println(okVerify ? "OK" : "ERR");
}

void updateBtEnableForSource(bool showWaiting) {
  bool shouldEnable = ampEnabled && currentSource == SOURCE_BLUETOOTH &&
                      !linkplayHardOffPending && !hardOffScreenVisible;
  if (shouldEnable) {
    if (!btModuleEnabled) {
      setBtModuleEnable(true);
      btWaitingSplashShown = false;
      btReconnectPending = true;
      btReconnectAtMs = millis() + BT_RECONNECT_DELAY_MS;
    }
    if (showWaiting && !btStatusConnected() && !btWaitingSplashShown) {
      showBtWaitingSplash();
      btWaitingSplashShown = true;
    } else if (btStatusConnected()) {
      requestBtConnectedNames();
    }
  } else if (btModuleEnabled) {
    setBtModuleEnable(false);
  }
}

void requestBtConnectedNames(uint32_t delayMs) {
  if (!btModuleEnabled || currentSource != SOURCE_BLUETOOTH) {
    return;
  }
  btNameRequestPending = true;
  btNameRequestAtMs = millis() + delayMs;
}

void pollBtReconnect() {
  if (!btReconnectPending) {
    return;
  }
  if (!btModuleEnabled || currentSource != SOURCE_BLUETOOTH || !ampEnabled) {
    btReconnectPending = false;
    return;
  }
  if (btStatusConnected()) {
    btReconnectPending = false;
    requestBtConnectedNames();
    return;
  }
  if (static_cast<int32_t>(millis() - btReconnectAtMs) >= 0) {
    btReconnectPending = false;
    sendBtCommand("AT+AC");
  }
}

void pollBtNameRequest() {
  if (!btNameRequestPending) {
    return;
  }
  if (!btModuleEnabled || currentSource != SOURCE_BLUETOOTH || !ampEnabled) {
    btNameRequestPending = false;
    return;
  }
  if (static_cast<int32_t>(millis() - btNameRequestAtMs) >= 0) {
    btNameRequestPending = false;
    sendBtCommand("AT+GRBN");
  }
}

String currentLinkplayModeCode() {
  if (currentSource == SOURCE_LINKPLAY) return linkplayNetworkModeCode;
  if (!ampEnabled) return "000";
  if (currentSource == SOURCE_XLR) return "048";
  if (currentSource == SOURCE_BLUETOOTH) return "041";
  if (currentSource == SOURCE_OPTICAL) return "043";
  return "000";
}

String linkplaySourceSelectCode(DspSource source) {
  switch (source) {
    case SOURCE_XLR: return "013";
    case SOURCE_BLUETOOTH: return "006";
    case SOURCE_OPTICAL: return "008";
    default: return "000";
  }
}

void sendLinkplayVolumeReport() {
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "MCU+VOL+%03u", displayedVolumePercent);
  sendLinkplayCommand(cmd);
}

void sendLinkplayModeReport() {
  sendLinkplayCommand("MCU+PLM+" + currentLinkplayModeCode());
}

void sendLinkplaySourceSelect(DspSource source) {
  sendLinkplayCommand("MCU+PLM+" + linkplaySourceSelectCode(source));
}

void sendLinkplayChannelReport() {
  sendLinkplayCommand("MCU+CHN+000");
}

void sendLinkplayCapabilityReport() {
  sendLinkplayCommand(String("MCU+CAP+PRJ") + LINKPLAY_CAP_PROJECT + "&");
  sendLinkplayCommand(String("MCU+PTV+") + LINKPLAY_CAP_PTV);
  sendLinkplayCommand(String("MCU+CAP+001") + LINKPLAY_CAP1 + "&");
  sendLinkplayCommand(String("MCU+CAP+002") + LINKPLAY_CAP2 + "&");
  sendLinkplayCommand(String("MCU+CAP+LAU") + LINKPLAY_CAP_LANGUAGE + "&");
  sendLinkplayCommand(String("MCU+CAP+PLM") + LINKPLAY_CAP_PLM + "&");
  sendLinkplayCommand(String("MCU+SID+") + LINKPLAY_DEFAULT_SSID + "&");
}

void sendLinkplayInputCapabilityReport() {
  sendLinkplayCommand(String("MCU+CAP+PLM") + LINKPLAY_CAP_PLM + "&");
}

void sendLinkplayBootDoneReport() {
  sendLinkplayCommand("MCU+BOT+DON");
}

void requestLinkplayRtcTime() {
  lastLinkplayRtcRequestMs = millis();
  sendLinkplayCommand("MCU+RTC+GET");
}

void pollLinkplayRtcSync() {
  if (!linkplayWifiReady || linkplayHardOffPending) {
    return;
  }
  if (lastLinkplayRtcRequestMs != 0 &&
      millis() - lastLinkplayRtcRequestMs < LINKPLAY_RTC_SYNC_INTERVAL_MS) {
    return;
  }
  requestLinkplayRtcTime();
}

void sendLinkplayVersionReport() {
  sendLinkplayCommand("MCU+VER+0035&MCU+PLM-" + currentLinkplayModeCode());
  sendLinkplayInputCapabilityReport();
}

void sendLinkplayEqReport() {
  char cmd[72];
  snprintf(cmd, sizeof(cmd),
           "MCU+PAS+EQSet:bass:%02u&MCU+PAS+EQSet:treble:%02u&",
           dbToLpEqValue(bassGainDb), dbToLpEqValue(trebleGainDb));
  sendLinkplayCommand(cmd);
}

void sendLinkplayEqNeutralReport() {
  sendLinkplayCommand("MCU+PAS+EQSet:bass:05&MCU+PAS+EQSet:treble:05&");
}

void requestLinkplayMetadata(bool force) {
  if (currentSource != SOURCE_LINKPLAY || linkplayHardOffPending || hardOffScreenVisible) {
    linkplayMetadataPending = false;
    linkplayPinfPending = false;
    return;
  }
  if (force && millis() - lastLinkplayMetadataRequestMs < LINKPLAY_METADATA_FORCE_MIN_INTERVAL_MS) {
    return;
  }
  if (!force && millis() - lastLinkplayMetadataRequestMs < 5000) {
    return;
  }
  lastLinkplayMetadataRequestMs = millis();
  linkplayMetadataPending = false;
  linkplayPinfPending = true;
  sendLinkplayCommand("MCU+MEA+GET");
}

void pollLinkplayMetadataRequest() {
  if (!linkplayMetadataPending) {
    return;
  }
  if (currentSource != SOURCE_LINKPLAY || linkplayHardOffPending || hardOffScreenVisible) {
    linkplayMetadataPending = false;
    linkplayPinfPending = false;
    return;
  }
  if (millis() - lastLinkplayMetadataRequestMs >= LINKPLAY_METADATA_FORCE_MIN_INTERVAL_MS) {
    requestLinkplayMetadata(true);
  }
}

void pauseLinkplayTransport(const char *reason) {
  if (!linkplayTransportPlaying) {
    return;
  }
  sendLinkplayCommand("MCU+PLY+PUS");
  linkplayTransportPlaying = false;
}

void pauseLinkplayForStandby() {
  if (!linkplayTransportPlaying || playbackState != "play") {
    linkplayTransportPlaying = false;
    playbackState = "pause";
    return;
  }
  sendLinkplayCommand("MCU+PLY+PUS");
  linkplayTransportPlaying = false;
  playbackState = "pause";
}

void pauseBtTransport(const char *reason) {
  if (!btTransportPlaying && !btModuleEnabled && !btStatusConnected() && currentSource != SOURCE_BLUETOOTH) {
    return;
  }
  sendBtCommand("AT+PU");
  btTransportPlaying = false;
  btPauseSettleActive = true;
  btPauseSettleUntilMs = millis() + BT_PAUSE_SETTLE_MS;
}

void pauseBtForStandby() {
  sendBtCommand("AT+PU");
  btTransportPlaying = false;
  btStandbyDisconnectPending = true;
  btStandbyDisconnectAtMs = millis() + BT_STANDBY_DISCONNECT_DELAY_MS;
  btPauseSettleActive = true;
  btPauseSettleUntilMs = millis() + BT_PAUSE_SETTLE_MS;
}

void pollBtStandbyDisconnect() {
  if (!btStandbyDisconnectPending) {
    return;
  }
  if (ampEnabled) {
    btStandbyDisconnectPending = false;
    return;
  }
  if (static_cast<int32_t>(millis() - btStandbyDisconnectAtMs) >= 0) {
    btStandbyDisconnectPending = false;
    sendBtCommand("AT+DC");
  }
}

bool wakeFromTransport(const char *reason) {
  if (ampEnabled) {
    return true;
  }
  if (linkplayHardOffPending || hardOffScreenVisible) {
    return false;
  }
  if (static_cast<int32_t>(millis() - standbyAutoWakeBlockedUntilMs) < 0) {
    return false;
  }
  setAmpPower(true, reason);
  return true;
}

bool linkplayPlaybackActive() {
  return currentSource == SOURCE_LINKPLAY && linkplayTransportPlaying && playbackState == "play";
}

void noteLinkplayTransportPlay(const char *reason) {
  linkplayTransportPlaying = true;
  if (!wakeFromTransport(reason)) {
    linkplayTransportPlaying = false;
    return;
  }
  pauseBtTransport(reason);
  if (currentSource != SOURCE_LINKPLAY) {
    setDspSource(SOURCE_LINKPLAY, networkModeName.length() > 0 ? networkModeName : "Network");
    requestLinkplayMetadata(true);
  } else {
    writeDspU32(DspMap::kInputMux, dspMuxValueForSource(SOURCE_LINKPLAY));
  }
  playbackState = "play";
  if (mainScreenVisible) {
    drawPlaybackIcon(playbackState.c_str());
  }
}

void noteLocalLinkplayModePlay(const char *reason) {
  if (!wakeFromTransport(reason)) {
    return;
  }
  playbackState = "play";
  if (mainScreenVisible) {
    drawPlaybackIcon(playbackState.c_str());
  }
}

void noteBtTransportPlay(const char *reason) {
  btTakeoverPending = false;
  btTransportPlaying = true;
  if (!wakeFromTransport(reason)) {
    btTransportPlaying = false;
    return;
  }
  if (currentSource != SOURCE_BLUETOOTH) {
    pauseLinkplayTransport(reason);
    setDspSource(SOURCE_BLUETOOTH, "Bluetooth");
    sendLinkplaySourceSelect(SOURCE_BLUETOOTH);
    lastBtSourceSelectMs = millis();
  } else {
    writeDspU32(DspMap::kInputMux, dspMuxValueForSource(SOURCE_BLUETOOTH));
  }
  playbackState = "play";
  if (mainScreenVisible) {
    drawPlaybackIcon(playbackState.c_str());
  }
}

void scheduleBtTakeover(const char *reason) {
  if (linkplayHardOffPending || hardOffScreenVisible) {
    btTransportPlaying = false;
    btTakeoverPending = false;
    return;
  }
  if (currentSource == SOURCE_BLUETOOTH) {
    noteBtTransportPlay(reason);
    return;
  }
  if (btPauseSettleActive && static_cast<int32_t>(millis() - btPauseSettleUntilMs) < 0) {
    return;
  }
  btTransportPlaying = true;
  btTakeoverPending = true;
  btTakeoverAtMs = millis() + BT_TAKEOVER_CONFIRM_MS;
}

void cancelBtTakeover() {
  btTakeoverPending = false;
}

void pollBtTakeover() {
  if (!btTakeoverPending) {
    return;
  }
  if (linkplayHardOffPending || hardOffScreenVisible) {
    btTakeoverPending = false;
    btTransportPlaying = false;
    return;
  }
  if (static_cast<int32_t>(millis() - btTakeoverAtMs) >= 0) {
    noteBtTransportPlay("bt-confirmed");
  }
}

void setDspMute(bool muted) {
  uint32_t mainValue = muted ? 0UL : 1UL;
  uint32_t subValue = (muted || !subEnabled) ? 0UL : 1UL;
  writeDspU32(DspMap::kMuteMain, mainValue);
  writeDspU32(DspMap::kMuteSub, subValue);
}

void setSubEnabled(bool enabled, bool persist) {
  subEnabled = enabled;
  writeDspU32(DspMap::kMuteSub, (linkplayMuted || !subEnabled) ? 0UL : 1UL);
  if (persist) {
    scheduleAudioSettingsSave();
  }
  showBottomStatus(subEnabled ? "Sub On" : "Sub Off");
}

void redrawActiveUi() {
  if (menuVisible) {
    drawMenuScreen();
  } else if (mainScreenVisible) {
    drawMainScreen();
  } else if (!splashActive && !hardOffScreenVisible && !systemBootScreenVisible) {
    drawStandby();
  }
}

void setScreenLightTheme(bool light, bool persist) {
  if (screenLightTheme == light) {
    return;
  }
  screenLightTheme = light;
  if (persist) {
    scheduleAudioSettingsSave();
  }
  redrawActiveUi();
}

void applyLinkplayMute(bool muted) {
  linkplayMuted = muted;
  setDspMute(linkplayMuted);
  if (mainScreenVisible) {
    setAmpMute(linkplayMuted);
    drawSourceHeader();
    drawVolumeValue(displayedVolumePercent);
  }
}

void toggleMuteFromControl() {
  bool nextMuted = !linkplayMuted;
  applyLinkplayMute(nextMuted);
  sendLinkplayCommand(String("MCU+MUT+") + (nextMuted ? "001" : "000"));
}

void pollStreamMuteStandby() {
  bool streamingSource = currentSource == SOURCE_LINKPLAY || currentSource == SOURCE_BLUETOOTH;
  if (!ampEnabled || !mainScreenVisible || splashActive || !streamingSource || !linkplayMuted) {
    streamMuteStartedMs = 0;
    return;
  }

  uint32_t now = millis();
  if (streamMuteStartedMs == 0) {
    streamMuteStartedMs = now;
    return;
  }
  if (static_cast<int32_t>(now - streamMuteStartedMs - STREAM_MUTE_STANDBY_MS) >= 0) {
    DebugSerial.println("Stream mute timeout -> standby");
    setAmpPower(false, "stream-mute-timeout");
    streamMuteStartedMs = 0;
  }
}

void applyVolumeDeltaFromControl(int delta, const char *source) {
  if (!mainScreenVisible) {
    return;
  }
  int nextVolume = constrain(static_cast<int>(displayedVolumePercent) + delta, 0, 100);
  if (nextVolume == displayedVolumePercent) {
    return;
  }
  applyVolumePercent(static_cast<uint8_t>(nextVolume), true, true, source);
  audioSettingsSaveOnStandby = true;
}

const char *irCommandName(uint8_t command) {
  switch (command) {
    case 0x4D: return "POWER";
    case 0x43: return "MUTE";
    case 0x1A: return "HOME";
    case 0x45: return "MENU";
    case 0x42: return "BACK";
    case 0x18: return "VOLUME_UP";
    case 0x17: return "VOLUME_DOWN";
    case 0x47: return "MOUSE";
    case 0x0B: return "UP";
    case 0x11: return "RIGHT";
    case 0x0E: return "DOWN";
    case 0x10: return "LEFT";
    case 0x0D: return "OK";
    default: return "UNKNOWN";
  }
}

void handleIrCommand(uint8_t command, bool repeat) {
  if (systemBootScreenVisible && !ampEnabled) {
    return;
  }

  uint32_t now = millis();
  if (repeat) {
    command = lastIrCommand;
    if (command != 0x18 && command != 0x17) {
      return;
    }
    if (now - lastIrRepeatActionMs < IR_REPEAT_ACTION_MS) {
      return;
    }
    lastIrRepeatActionMs = now;
  } else {
    lastIrCommand = command;
    lastIrRepeatActionMs = now;
  }

  if (menuVisible) {
    switch (command) {
      case 0x1A:
        if (!repeat) {
          exitMenu();
        }
        return;
      case 0x42:
        if (!repeat) {
          menuBack();
        }
        return;
      case 0x0B:
        if (!repeat) {
          menuMove(-1);
        }
        return;
      case 0x0E:
        if (!repeat) {
          menuMove(1);
        }
        return;
      case 0x10:
        menuAdjust(-1);
        return;
      case 0x11:
        menuAdjust(1);
        return;
      case 0x0D:
        if (!repeat) {
          menuSelect();
        }
        return;
      default:
        break;
    }
  }

  switch (command) {
    case 0x4D:
      if (!repeat) {
        toggleAmpPowerFromControl("ir");
      }
      break;
    case 0x43:
      if (!repeat && mainScreenVisible) {
        toggleMuteFromControl();
      }
      break;
    case 0x45:
      if (!repeat && mainScreenVisible) {
        enterMenu();
      }
      break;
    case 0x1A:
      if (!repeat && mainScreenVisible) {
        drawMainScreen();
      }
      break;
    case 0x47:
      if (!repeat && mainScreenVisible) {
        cycleSourceFromControl("ir-mouse");
      }
      break;
    case 0x0B:
    case 0x11:
      if (!repeat) {
        skipTrackFromControl(1, "ir");
      }
      break;
    case 0x0E:
    case 0x10:
      if (!repeat) {
        skipTrackFromControl(-1, "ir");
      }
      break;
    case 0x0D:
      if (!repeat && mainScreenVisible) {
        togglePlaybackFromControl("ir-ok");
      }
      break;
    case 0x18:
      applyVolumeDeltaFromControl(1, "ir");
      break;
    case 0x17:
      applyVolumeDeltaFromControl(-1, "ir");
      break;
    default:
      break;
  }
}

int lpEqValueToDb(int value) {
  value = constrain(value, 0, 10);
  float db = (static_cast<float>(value) - 5.0f) * 2.0f;
  return static_cast<int>(lroundf(db));
}

uint8_t dbToLpEqValue(int gainDb) {
  gainDb = constrain(gainDb, -10, 10);
  int value = static_cast<int>(lroundf((static_cast<float>(gainDb) / 2.0f) + 5.0f));
  return static_cast<uint8_t>(constrain(value, 0, 10));
}

void setBassGainDb(int gainDb, bool reportToLinkplay = false, bool persist = true) {
  int previousGainDb = bassGainDb;
  bassGainDb = constrain(gainDb, -12, 12);
  writeDspU32(DspMap::kBassGainDb, dspFloatToFixed8_24(static_cast<float>(bassGainDb)));
  if (persist && bassGainDb != previousGainDb) {
    scheduleAudioSettingsSave();
  }
  char label[20];
  snprintf(label, sizeof(label), "Bass %+ddB", bassGainDb);
  showBottomStatus(label);
  if (reportToLinkplay) {
    sendLinkplayEqReport();
  }
}

void setTrebleGainDb(int gainDb, bool reportToLinkplay = false, bool persist = true) {
  int previousGainDb = trebleGainDb;
  trebleGainDb = constrain(gainDb, -12, 12);
  writeDspU32(DspMap::kTrebleGainDb, dspFloatToFixed8_24(static_cast<float>(trebleGainDb)));
  if (persist && trebleGainDb != previousGainDb) {
    scheduleAudioSettingsSave();
  }
  char label[20];
  snprintf(label, sizeof(label), "Treble %+ddB", trebleGainDb);
  showBottomStatus(label);
  if (reportToLinkplay) {
    sendLinkplayEqReport();
  }
}

void applyVolumePercent(uint8_t percent, bool reportToLinkplay = false, bool persist = true, const char *source = "unknown") {
  (void)source;
  percent = constrain(percent, 0, 100);
  uint8_t previousPercent = displayedVolumePercent;
  writeDspU32(DspMap::kMasterVolume, volumePercentToSigma(percent));
  displayedVolumePercent = percent;
  if (persist && displayedVolumePercent != previousPercent) {
    scheduleAudioSettingsSave();
  }
  if (mainScreenVisible) {
    drawVolumeValue(displayedVolumePercent);
  }
  if (reportToLinkplay) {
    sendLinkplayVolumeReport();
  }
}

void printHelp() {
  DebugSerial.println("Commands:");
  DebugSerial.println("  dsp              - read DSP status/checksum/levels");
  DebugSerial.println("  rd <addr>        - read 32-bit parameter");
  DebugSerial.println("  wr <addr> <val>  - write 32-bit parameter");
  DebugSerial.println("  reg <addr>       - read 16-bit control register");
  DebugSerial.println("  mute on|off      - write DSP mute params");
  DebugSerial.println("  vol 0..100       - write DSP master volume");
  DebugSerial.println("  bass -12..12     - write DSP bass shelf");
  DebugSerial.println("  treble -12..12   - write DSP treble shelf");
  DebugSerial.println("  input <n>        - write DSP input mux raw value");
  DebugSerial.println("  src lp|xlr|bt|opt - select DSP source");
  DebugSerial.println("  title <text>     - set LCD scrolling title text");
  DebugSerial.println("  t <text>         - short alias for title");
  DebugSerial.println("  lp <cmd>         - send raw Linkplay command");
  DebugSerial.println("  lplog on|off     - show raw Linkplay TX/RX frames");
  DebugSerial.println("  bt <cmd>         - send raw Bluetooth UART command");
  DebugSerial.println("  bten on|off      - drive Bluetooth enable pin PA1");
  DebugSerial.println("  btstat           - read Bluetooth status pin PA2");
  DebugSerial.println("  btlog on|off     - show raw Bluetooth TX/RX lines");
  DebugSerial.println("  screen [2.0|2.4]/[0|180] - set TFT panel profile");
  DebugSerial.println("  eestatus         - read external 25AA/25LC EEPROM status");
  DebugSerial.println("  eebbtest         - bit-bang EEPROM read/write verify");
  DebugSerial.println("  fcsp high|low    - drive external flash/EEPROM CS pin PB6");
  DebugSerial.println("  next|prev        - skip current LP/BT track");
  DebugSerial.println("  www              - request Linkplay WiFi status");
  DebugSerial.println("  lpget            - request Linkplay metadata");
  DebugSerial.println("  clock            - show RTC RAM/EEPROM state");
  DebugSerial.println("  rtcsrc           - show STM32 RTC/LSE clock registers");
  DebugSerial.println("  rtclse           - retry STM32 LSE RTC clock init");
}

void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) {
    return;
  }
  DebugSerial.print("> ");
  DebugSerial.println(line);

  String cmd = nextToken(line);
  cmd.toLowerCase();

  if (cmd == "help" || cmd == "?") {
    printHelp();
    return;
  }
  if (cmd == "dsp") {
    printDspStatus();
    return;
  }
  if (cmd == "rd") {
    uint16_t address = static_cast<uint16_t>(parseNumber(nextToken(line)));
    uint32_t value = 0;
    DebugSerial.print("DSP rd ");
    DebugSerial.print(address);
    DebugSerial.print(" = ");
    if (readDspU32(address, value)) {
      printHex32(value);
    } else {
      DebugSerial.print("ERR");
    }
    DebugSerial.println();
    return;
  }
  if (cmd == "wr") {
    uint16_t address = static_cast<uint16_t>(parseNumber(nextToken(line)));
    uint32_t value = parseNumber(nextToken(line));
    bool ok = writeDspU32(address, value);
    DebugSerial.print("DSP wr ");
    DebugSerial.print(address);
    DebugSerial.print(" = ");
    printHex32(value);
    DebugSerial.println(ok ? " OK" : " ERR");
    return;
  }
  if (cmd == "reg") {
    uint16_t address = static_cast<uint16_t>(parseNumber(nextToken(line)));
    printDspReg16("REG", address);
    return;
  }
  if (cmd == "mute") {
    String state = nextToken(line);
    state.toLowerCase();
    bool muted = (state == "on" || state == "1" || state == "true");
    uint32_t value = muted ? 0UL : 1UL;
    bool okMain = writeDspU32(DspMap::kMuteMain, value);
    bool okSub = writeDspU32(DspMap::kMuteSub, value);
    linkplayMuted = muted;
    if (mainScreenVisible) {
      drawSourceHeader();
      drawVolumeValue(displayedVolumePercent);
    }
    DebugSerial.print("DSP mute ");
    DebugSerial.print(muted ? "ON" : "OFF");
    DebugSerial.println(okMain && okSub ? " OK" : " ERR");
    return;
  }
  if (cmd == "vol") {
    uint32_t requested = 0;
    if (!parseNumberStrict(nextToken(line), requested)) {
      DebugSerial.println("ERR: vol needs 0..100");
      return;
    }
    applyVolumePercent(static_cast<uint8_t>(constrain(static_cast<int>(requested), 0, 100)), true, true, "terminal");
    return;
  }
  if (cmd == "bass") {
    line.trim();
    setBassGainDb(constrain(line.toInt(), -12, 12), true);
    return;
  }
  if (cmd == "treble") {
    line.trim();
    setTrebleGainDb(constrain(line.toInt(), -12, 12), true);
    return;
  }
  if (cmd == "input") {
    uint32_t value = parseNumber(nextToken(line));
    bool ok = writeDspU32(DspMap::kInputMux, value);
    DebugSerial.print("DSP input mux = ");
    DebugSerial.print(value);
    DebugSerial.println(ok ? " OK" : " ERR");
    return;
  }
  if (cmd == "src") {
    String sourceName = nextToken(line);
    sourceName.toLowerCase();
    if (sourceName == "lp" || sourceName == "network") {
      setDspSource(SOURCE_LINKPLAY, "Network");
    } else if (sourceName == "xlr" || sourceName == "aux") {
      setDspSource(SOURCE_XLR, "XLR");
    } else if (sourceName == "bt" || sourceName == "bluetooth") {
      setDspSource(SOURCE_BLUETOOTH, "Bluetooth");
    } else if (sourceName == "opt" || sourceName == "optical") {
      setDspSource(SOURCE_OPTICAL, "Optical");
    } else {
      DebugSerial.println("ERR: src needs lp|xlr|bt|opt");
    }
    return;
  }
  if (cmd == "title" || cmd == "t") {
    line.trim();
    if (line.length() == 0) {
      DebugSerial.println("ERR: title needs text");
      return;
    }
    currentArtist = "";
    currentAlbum = "";
    currentTitle = line;
    lastScrollText = "";
    DebugSerial.print("LCD title = ");
    DebugSerial.println(currentTitle);
    return;
  }
  if (cmd == "lp") {
    line.trim();
    if (line.length() == 0) {
      DebugSerial.println("ERR: lp needs command");
      return;
    }
    if (line == "MCU+WWW+GET") {
      linkplayWifiStatusRequestPending = true;
    }
    sendLinkplayCommand(line);
    return;
  }
  if (cmd == "lplog") {
    String state = nextToken(line);
    state.toLowerCase();
    if (state == "on" || state == "1" || state == "true") {
      linkplayRawLogEnabled = true;
    } else if (state == "off" || state == "0" || state == "false") {
      linkplayRawLogEnabled = false;
    } else {
      DebugSerial.println("ERR: lplog needs on|off");
      return;
    }
    DebugSerial.print("LP raw log ");
    DebugSerial.println(linkplayRawLogEnabled ? "ON" : "OFF");
    return;
  }
  if (cmd == "btlog") {
    String state = nextToken(line);
    state.toLowerCase();
    if (state == "on" || state == "1" || state == "true") {
      btRawLogEnabled = true;
    } else if (state == "off" || state == "0" || state == "false") {
      btRawLogEnabled = false;
    } else {
      DebugSerial.println("ERR: btlog needs on|off");
      return;
    }
    DebugSerial.print("BT raw log ");
    DebugSerial.println(btRawLogEnabled ? "ON" : "OFF");
    return;
  }
  if (cmd == "bt") {
    line.trim();
    if (line.length() == 0) {
      DebugSerial.println("ERR: bt needs command");
      return;
    }
    sendBtCommand(line);
    return;
  }
  if (cmd == "bten") {
    String state = nextToken(line);
    state.toLowerCase();
    if (state.length() == 0) {
      printBtStatus();
      return;
    }
    if (state == "on" || state == "1" || state == "true") {
      setBtModuleEnable(true);
      printBtStatus();
    } else if (state == "off" || state == "0" || state == "false") {
      setBtModuleEnable(false);
      printBtStatus();
    } else {
      DebugSerial.println("ERR: bten needs on|off");
    }
    return;
  }
  if (cmd == "btstat") {
    printBtStatus();
    return;
  }
  if (cmd == "screen") {
    line.trim();
    if (line.length() == 0 || line == "status" || line == "?") {
      printScreenSettings();
      return;
    }
    String panelToken = nextToken(line);
    panelToken.toLowerCase();
    String rotationToken;
    int slash = panelToken.indexOf('/');
    if (slash >= 0) {
      rotationToken = panelToken.substring(slash + 1);
      panelToken = panelToken.substring(0, slash);
    } else {
      rotationToken = nextToken(line);
    }
    rotationToken.toLowerCase();

    ScreenPanelType panel = SCREEN_PANEL_24;
    if (panelToken == "2.0" || panelToken == "20") {
      panel = SCREEN_PANEL_20;
    } else if (panelToken == "2.4" || panelToken == "24") {
      panel = SCREEN_PANEL_24;
    } else {
      DebugSerial.println("ERR: screen panel needs 2.0 or 2.4");
      return;
    }

    uint16_t rotation = 0;
    if (rotationToken == "0") {
      rotation = 0;
    } else if (rotationToken == "180") {
      rotation = 180;
    } else {
      DebugSerial.println("ERR: screen rotation needs 0 or 180");
      return;
    }

    if (!setScreenPanelConfig(panel, rotation)) {
      DebugSerial.println("ERR: screen needs <2.0|2.4>/<0|180>");
      return;
    }
    printScreenSettings();
    return;
  }
  if (cmd == "eestatus") {
    printExternalEepromBitBangStatus();
    return;
  }
  if (cmd == "eebbstatus") {
    printExternalEepromBitBangStatus();
    return;
  }
  if (cmd == "eebbtest") {
    testExternalEepromBitBang();
    return;
  }
  if (cmd == "fcsp") {
    String state = nextToken(line);
    state.toLowerCase();
    if (state == "high" || state == "h" || state == "1" || state == "on") {
      digitalWrite(FLASH_CS_PIN, HIGH);
    } else if (state == "low" || state == "l" || state == "0" || state == "off") {
      digitalWrite(FLASH_CS_PIN, LOW);
    } else {
      DebugSerial.println("ERR: fcsp needs high|low");
      return;
    }
    DebugSerial.print("FLASH_CS PB6=");
    DebugSerial.println(digitalRead(FLASH_CS_PIN) == HIGH ? "HIGH" : "LOW");
    return;
  }
  if (cmd == "next") {
    skipTrackFromControl(1, "terminal");
    return;
  }
  if (cmd == "prev" || cmd == "previous") {
    skipTrackFromControl(-1, "terminal");
    return;
  }
  if (cmd == "www") {
    requestLinkplayWifiStatus();
    return;
  }
  if (cmd == "lpget") {
    requestLinkplayMetadata(true);
    return;
  }
  if (cmd == "clock") {
    printClockDebug();
    return;
  }
  if (cmd == "rtcsrc") {
    printRtcClockDebug();
    return;
  }
  if (cmd == "rtclse") {
    initRtcClockSource();
    printRtcClockDebug();
    return;
  }

  DebugSerial.print("Unknown command: ");
  DebugSerial.println(cmd);
}

void pollDebugSerial() {
  while (DebugSerial.available() > 0) {
    char c = static_cast<char>(DebugSerial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      handleCommand(debugLine);
      debugLine = "";
    } else if (debugLine.length() < 120) {
      debugLine += c;
    }
  }
}

void pollEncoder() {
  static const int8_t transitionTable[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0,
  };

  if (systemBootScreenVisible && !ampEnabled) {
    encoderLastState = ((digitalRead(ENCODER_A_PIN) ? 1 : 0) << 1) |
                       (digitalRead(ENCODER_B_PIN) ? 1 : 0);
    encoderAccumulator = 0;
    encoderButtonStable = digitalRead(ENCODER_SW_PIN);
    encoderButtonLastReading = encoderButtonStable;
    encoderButtonLastChangeMs = millis();
    encoderButtonLongHandled = false;
    return;
  }

  uint8_t state = ((digitalRead(ENCODER_A_PIN) ? 1 : 0) << 1) |
                  (digitalRead(ENCODER_B_PIN) ? 1 : 0);
  if (state != encoderLastState) {
    uint8_t transition = (encoderLastState << 2) | state;
    int8_t delta = transitionTable[transition & 0x0F];
    encoderAccumulator += delta;
    encoderLastState = state;

    if (encoderAccumulator >= ENCODER_DETENT_DIVISOR) {
      encoderAccumulator = 0;
      if (menuVisible) {
        if (menuEditing || menuSelectedItemIsAdjustable()) {
          menuAdjust(1);
        } else {
          menuMove(1);
        }
      } else if (mainScreenVisible) {
        applyVolumeDeltaFromControl(ENCODER_VOLUME_STEP, "encoder");
      }
    } else if (encoderAccumulator <= -ENCODER_DETENT_DIVISOR) {
      encoderAccumulator = 0;
      if (menuVisible) {
        if (menuEditing || menuSelectedItemIsAdjustable()) {
          menuAdjust(-1);
        } else {
          menuMove(-1);
        }
      } else if (mainScreenVisible) {
        applyVolumeDeltaFromControl(-ENCODER_VOLUME_STEP, "encoder");
      }
    }
  }

  bool buttonReading = digitalRead(ENCODER_SW_PIN);
  uint32_t now = millis();
  if (buttonReading != encoderButtonLastReading) {
    encoderButtonLastReading = buttonReading;
    encoderButtonLastChangeMs = now;
  }
  if (now - encoderButtonLastChangeMs > ENCODER_BUTTON_DEBOUNCE_MS &&
      buttonReading != encoderButtonStable) {
    encoderButtonStable = buttonReading;
    if (encoderButtonStable == LOW) {
      encoderButtonPressedMs = now;
      encoderButtonLongHandled = false;
    } else {
      if (!encoderButtonLongHandled) {
        if (menuVisible) {
          encoderMenuPress();
        } else if (mainScreenVisible) {
          enterMenu();
        } else if (!splashActive) {
          toggleAmpPowerFromControl("encoder");
        }
      }
    }
  }
  if (encoderButtonStable == LOW && !encoderButtonLongHandled &&
      now - encoderButtonPressedMs >= ENCODER_BUTTON_LONG_MS) {
    encoderButtonLongHandled = true;
    if (menuVisible) {
      menuBack();
    } else if (!ampEnabled && !splashActive) {
      startLinkplayHardOffRequest();
    } else {
      toggleAmpPowerFromControl("encoder-long");
    }
  }
}

void handleIrEdge() {
  uint32_t now = micros();
  uint32_t duration = now - irLastEdgeUs;
  irLastEdgeUs = now;

  if (duration > IR_FRAME_GAP_US) {
    if (irPulseCount >= 3 && !irFrameReady) {
      irFrameBuffer = irActiveBuffer;
      irFrameLength = irPulseCount;
      irFrameReady = true;
      irActiveBuffer ^= 1;
    }
    irPulseCount = 0;
    return;
  }

  if (irPulseCount < IR_MAX_PULSES) {
    irPulseBuffers[irActiveBuffer][irPulseCount++] =
        static_cast<uint16_t>(duration > 0xFFFF ? 0xFFFF : duration);
  } else {
    irPulseCount = 0;
  }
}

bool irMatch(uint16_t value, uint16_t target, uint16_t tolerance) {
  return value >= target - tolerance && value <= target + tolerance;
}

bool decodeNecFrame(const uint16_t *pulses, uint8_t length, uint32_t &code, bool &repeat) {
  code = 0;
  repeat = false;
  if (length >= 3 &&
      irMatch(pulses[0], 9000, 1200) &&
      irMatch(pulses[1], 2250, 700) &&
      irMatch(pulses[2], 560, 300)) {
    repeat = true;
    return true;
  }

  if (length < 66 ||
      !irMatch(pulses[0], 9000, 1200) ||
      !irMatch(pulses[1], 4500, 900)) {
    return false;
  }

  for (uint8_t bit = 0; bit < 32; bit++) {
    uint8_t lowIndex = 2 + (bit * 2);
    uint8_t highIndex = lowIndex + 1;
    if (!irMatch(pulses[lowIndex], 560, 300)) {
      return false;
    }
    if (irMatch(pulses[highIndex], 1690, 500)) {
      code |= (1UL << bit);
    } else if (!irMatch(pulses[highIndex], 560, 300)) {
      return false;
    }
  }
  return true;
}

void printIrFrame(const uint16_t *pulses, uint8_t length) {
  uint32_t code = 0;
  bool repeat = false;
  if (decodeNecFrame(pulses, length, code, repeat)) {
    if (repeat) {
      handleIrCommand(0, true);
      return;
    }

    uint16_t address = code & 0xFFFF;
    uint8_t command = (code >> 16) & 0xFF;
    uint8_t commandInv = (code >> 24) & 0xFF;
    if (address == IR_REMOTE_ADDR && static_cast<uint8_t>(command ^ commandInv) == 0xFF) {
      handleIrCommand(command, false);
    }
    return;
  }
}

void pollIrReceiver() {
  uint16_t pulses[IR_MAX_PULSES];
  uint8_t length = 0;

  noInterrupts();
  if (irFrameReady) {
    uint8_t buffer = irFrameBuffer;
    length = irFrameLength;
    for (uint8_t i = 0; i < length && i < IR_MAX_PULSES; i++) {
      pulses[i] = irPulseBuffers[buffer][i];
    }
    irFrameReady = false;
  } else if (irPulseCount >= 3 && (micros() - irLastEdgeUs) > IR_FRAME_GAP_US) {
    uint8_t buffer = irActiveBuffer;
    length = irPulseCount;
    for (uint8_t i = 0; i < length && i < IR_MAX_PULSES; i++) {
      pulses[i] = irPulseBuffers[buffer][i];
    }
    irPulseCount = 0;
  }
  interrupts();

  if (length > 0) {
    printIrFrame(pulses, length);
  }
}

void parseBtLine(String line) {
  line.trim();
  if (line.length() == 0) {
    return;
  }
  if (btRawLogEnabled) {
    DebugSerial.print("[");
    DebugSerial.print(millis());
    DebugSerial.print("] BT RX: ");
    DebugSerial.println(line);
  }
  bool ignoreBtTakeover =
      btPauseSettleActive && static_cast<int32_t>(millis() - btPauseSettleUntilMs) < 0;

  if (line == "BT_CN") {
    btWaitingSplashShown = true;
    if (ampEnabled && currentSource == SOURCE_BLUETOOTH) {
      requestBtConnectedNames();
    }
    return;
  }

  if (line.startsWith("+REMOTE_NAME")) {
    int equals = line.indexOf('=');
    if (equals >= 0) {
      String name = replaceTurkish(line.substring(equals + 1));
      name.trim();
      if (name.length() > 0) {
        btConnectedDeviceName = name;
        if (currentSource == SOURCE_BLUETOOTH) {
          currentArtist = "";
          currentAlbum = "";
          currentTitle = "Connected to " + btConnectedDeviceName;
          resetScroll();
          if (splashActive) {
            splashActive = false;
            drawMainScreen();
          } else if (mainScreenVisible) {
            drawScrollingTitle(true);
          }
        }
      }
    }
    return;
  }

  if (line.startsWith("+SRC=")) {
    String src = line.substring(5);
    src.trim();
    if (src == "BT1" || src == "BT2" || src == "BT") {
      btTransportPlaying = true;
    } else if (src == "NONE") {
      btTransportPlaying = false;
      btConnectedDeviceName = "";
      cancelBtTakeover();
      playbackState = "stop";
      if (mainScreenVisible && currentSource == SOURCE_BLUETOOTH) {
        drawPlaybackIcon(playbackState.c_str());
      }
    }
    return;
  }

  if (line == "BT_PA") {
    btTransportPlaying = true;
    if (currentSource == SOURCE_BLUETOOTH && !ignoreBtTakeover) {
      writeDspU32(DspMap::kInputMux, dspMuxValueForSource(SOURCE_BLUETOOTH));
      playbackState = "play";
      if (mainScreenVisible) {
        drawPlaybackIcon(playbackState.c_str());
      }
    } else if (!ignoreBtTakeover) {
      scheduleBtTakeover("bt-pa");
    }
    return;
  }
  if (line == "BT_PU") {
    btTransportPlaying = false;
    cancelBtTakeover();
    if (currentSource == SOURCE_BLUETOOTH) {
      playbackState = "pause";
      if (mainScreenVisible) {
        drawPlaybackIcon(playbackState.c_str());
      }
    }
    return;
  }

  if (line.startsWith("+TITL=") || line.startsWith("+ARTS=") || line.startsWith("+ALBM=")) {
    scheduleBtTakeover("bt-meta");
  }

  if (currentSource != SOURCE_BLUETOOTH) {
    return;
  }

  bool changedText = false;
  if (line.startsWith("+TITL=")) {
    currentTitle = replaceTurkish(line.substring(6));
    changedText = true;
  } else if (line.startsWith("+ARTS=")) {
    currentArtist = replaceTurkish(line.substring(6));
    changedText = true;
  } else if (line.startsWith("+ALBM=")) {
    currentAlbum = replaceTurkish(line.substring(6));
    changedText = true;
  } else if (line.startsWith("+PYTM=") || line.startsWith("+PTIM=")) {
    int equals = line.indexOf('=');
    if (equals >= 0) {
      String value = line.substring(equals + 1);
      value.replace("ms", "");
      value.trim();
      trackLengthMs = value.toInt();
      if (mainScreenVisible) {
        drawTimers();
      }
    }
  }

  if (changedText) {
    localSourceTitleLocked = false;
    resetScroll();
    if (mainScreenVisible) {
      drawScrollingTitle(true);
    }
  }
}

void pollBtSerial() {
  bool gotByte = false;
  while (BtSerial.available() > 0) {
    int raw = BtSerial.read();
    char c = static_cast<char>(raw);
    gotByte = true;
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      parseBtLine(btLine);
      btLine = "";
    } else if (btLine.length() < 240) {
      btLine += c;
    } else {
      btLine = "";
      DebugSerial.println("BT RX overflow");
    }
  }
  if (gotByte) {
    lastBtRxMs = millis();
  }
  if (btLine.length() > 0 && millis() - lastBtRxMs > 60) {
    parseBtLine(btLine);
    btLine = "";
  }
}

void applyLinkplayJsonMetadata(const String &json, bool plyInfo) {
  String title = extractJsonString(json, "title");
  String artist = extractJsonString(json, "artist");
  String album = extractJsonString(json, "album");
  String titleCaps = extractJsonString(json, "Title");
  String artistCaps = extractJsonString(json, "Artist");
  String albumCaps = extractJsonString(json, "Album");
  String status = extractJsonString(json, "status");
  String vendor = extractJsonString(json, "vendor");
  String mode = extractJsonString(json, "mode");
  uint32_t numericValue = 0;
  bool changedText = false;

  bool acceptMetadataText = currentSource == SOURCE_LINKPLAY && !localSourceTitleLocked;

  if (currentSource == SOURCE_LINKPLAY) {
    String nextModeName = "";
    if (decodeMetadataText(vendor) == "Spotify") {
      nextModeName = "Spotify";
    } else if (mode.length() > 0) {
      nextModeName = mapLpModeToName(mode);
    } else if (extractJsonUint(json, "mode", numericValue)) {
      nextModeName = mapLpModeToName(String(numericValue));
    }
    if (nextModeName.length() > 0 && nextModeName != networkModeName) {
      networkModeName = nextModeName;
      if (mainScreenVisible) {
        drawSourceHeader();
      }
    }
  }

  if (acceptMetadataText && (title.length() > 0 || (plyInfo && jsonHasKey(json, "title")))) {
    currentTitle = decodeMetadataText(title);
    if (currentArtist == "HEGES") currentArtist = "";
    if (currentAlbum == "Network") currentAlbum = "";
    changedText = true;
  }
  if (acceptMetadataText && (artist.length() > 0 || (plyInfo && jsonHasKey(json, "artist")))) {
    currentArtist = decodeMetadataText(artist);
    changedText = true;
  }
  if (acceptMetadataText && (album.length() > 0 || (plyInfo && jsonHasKey(json, "album")))) {
    currentAlbum = decodeMetadataText(album);
    changedText = true;
  }
  if (acceptMetadataText && (titleCaps.length() > 0 || (plyInfo && jsonHasKey(json, "Title")))) {
    currentTitle = decodeMetadataText(titleCaps);
    changedText = true;
  }
  if (acceptMetadataText && (artistCaps.length() > 0 || (plyInfo && jsonHasKey(json, "Artist")))) {
    currentArtist = decodeMetadataText(artistCaps);
    changedText = true;
  }
  if (acceptMetadataText && (albumCaps.length() > 0 || (plyInfo && jsonHasKey(json, "Album")))) {
    currentAlbum = decodeMetadataText(albumCaps);
    changedText = true;
  }
  if (status.length() > 0) {
    if (currentSource == SOURCE_LINKPLAY && status == "play") {
      noteLinkplayTransportPlay("lp-status");
      playbackState = status;
      if (mainScreenVisible) {
        drawPlaybackIcon(playbackState.c_str());
      }
    } else if (status == "pause" || status == "stop") {
      linkplayTransportPlaying = false;
      if (currentSource == SOURCE_LINKPLAY) {
        playbackState = status;
        if (mainScreenVisible) {
          drawPlaybackIcon(playbackState.c_str());
        }
      }
    }
  }
  if (changedText && playbackState == "stop" && currentSource == SOURCE_LINKPLAY) {
    linkplayTransportPlaying = true;
    playbackState = "play";
    if (mainScreenVisible) {
      drawPlaybackIcon(playbackState.c_str());
    }
  }
  if (extractJsonUint(json, "totlen", numericValue)) {
    trackLengthMs = normalizeLpTimeMs(numericValue);
  }
  if (extractJsonUint(json, "curpos", numericValue)) {
    trackPositionMs = normalizeLpTimeMs(numericValue);
    lastTrackTickMs = millis();
  }
  if (extractJsonUint(json, "loop", numericValue)) {
    uint8_t nextLoopMode = static_cast<uint8_t>(numericValue);
    if (nextLoopMode != linkplayLoopMode) {
      linkplayLoopMode = nextLoopMode;
      if (mainScreenVisible) {
        drawTopStatusIcon();
      }
    }
  }
  if (extractJsonUint(json, "vol", numericValue)) {
    applyVolumePercent(static_cast<uint8_t>(constrain(static_cast<int>(numericValue), 0, 100)), false, true, plyInfo ? "lp-pinf" : "lp-meta");
  }
  if (extractJsonUint(json, "mute", numericValue)) {
    applyLinkplayMute(numericValue != 0);
  }

  if (changedText) {
    resetScroll();
  }
  if (mainScreenVisible) {
    drawTimers();
    drawScrollingTitle(true);
  }
}

void parseLinkplayFrame(String frame) {
  frame.trim();
  if (frame.length() == 0 || !frame.startsWith("AXX+")) {
    return;
  }

  if (frame == "AXX+BURNING" || frame.startsWith("AXX+BURNING&")) {
    finishLinkplayHardOffRequest("BURNING", 3, HARD_OFF_ALERT_BLINK_MS);
    return;
  }

  int plusIdx = frame.indexOf('+', 4);
  if (plusIdx < 0) {
    return;
  }
  String cmd = frame.substring(4, plusIdx);
  String data = frame.substring(plusIdx + 1);
  data.trim();
  while (data.endsWith("&")) {
    data.remove(data.length() - 1);
    data.trim();
  }

  if (cmd == "BOT") {
    updateSystemBootScreen("Linkplay booting", 1);
    if ((data == "DON" || data == "UP0") && !linkplayBootDoneReported) {
      linkplayBootDoneReported = true;
      linkplayReadyReported = false;
      sendLinkplayBootDoneReport();
    }
    return;
  }
  if (cmd == "VOL") {
    if (data == "GET") {
      sendLinkplayVolumeReport();
      return;
    }
    uint32_t value = 0;
    if (parseDecimalStrict(data, value)) {
      applyVolumePercent(static_cast<uint8_t>(constrain(static_cast<int>(value), 0, 100)), false, true, "lp-vol");
    }
    return;
  }
  if (cmd == "GET") {
    if (data == "RTC") {
      requestLinkplayRtcTime();
    }
    return;
  }
  if (cmd == "SET") {
    int rtcPos = data.indexOf("RTC");
    if (rtcPos >= 0 && data.length() >= rtcPos + 15) {
      updateSystemBootScreen("Time synced", 4);
      String rtc = data.substring(rtcPos + 3, rtcPos + 17);
      syncClockFromRtcString(rtc);
      if (mainScreenVisible) {
        drawSourceHeader();
      } else if (!splashActive && !systemBootScreenVisible && !hardOffScreenVisible) {
        drawStandby();
      }
    }
    return;
  }
  if (cmd == "PLM") {
    if (data == "GET") {
      sendLinkplayModeReport();
      return;
    }
    setSourceFromLpMode(data, true, true);
    return;
  }
  if (cmd == "CHN") {
    if (data == "GET") {
      sendLinkplayChannelReport();
    }
    return;
  }
  if (cmd == "CAP") {
    if (data == "GET") {
      sendLinkplayCapabilityReport();
    }
    return;
  }
  if (cmd == "POW") {
    if (data == "OFF") {
      finishLinkplayHardOffRequest("READY", 2, HARD_OFF_SUCCESS_BLINK_MS);
    }
    return;
  }
  if (cmd == "WWW") {
    handleLinkplayWifiState(data == "001");
    return;
  }
  if (cmd == "STA") {
    if (systemBootScreenVisible && !ampEnabled) {
      if (data == "002") {
        updateSystemBootScreen("Connecting WiFi", 2);
      } else if (data == "001") {
        updateSystemBootScreen("Checking network", 3);
      } else if (data == "000") {
        updateSystemBootScreen("Setup WiFi in app", 3);
      }
    }
    return;
  }
  if (cmd == "SLV") {
    LinkplayGroupRole nextRole = linkplayGroupRole;
    if (data == "YES") {
      nextRole = LINKPLAY_GROUP_SLAVE;
    } else if (data == "001") {
      nextRole = LINKPLAY_GROUP_MASTER;
    } else if (data == "000" || data == "NOT") {
      nextRole = LINKPLAY_GROUP_NONE;
    }
    if (nextRole != linkplayGroupRole) {
      linkplayGroupRole = nextRole;
      if (mainScreenVisible) {
        drawSourceHeader();
      }
    }
    return;
  }
  if (cmd == "DEV") {
    if (data == "RST") {
      linkplayBootDoneReported = false;
      linkplayReadyReported = false;
      linkplayWifiReady = false;
    }
    return;
  }
  if (cmd == "PAS") {
    String pasData = data;
    pasData.toLowerCase();
    if (pasData == "eqget") {
      sendLinkplayEqReport();
      return;
    }
    if (pasData.startsWith("eqset:")) {
      int firstColon = data.indexOf(':');
      int secondColon = data.indexOf(':', firstColon + 1);
      if (firstColon >= 0 && secondColon >= 0) {
        String band = data.substring(firstColon + 1, secondColon);
        band.toLowerCase();
        int lpEqValue = constrain(data.substring(secondColon + 1).toInt(), 0, 10);
        int gainDb = lpEqValueToDb(lpEqValue);
        if (band == "bass") {
          setBassGainDb(gainDb);
        } else if (band == "treble") {
          setTrebleGainDb(gainDb);
        }
      }
    }
    return;
  }
  if (cmd == "MUT") {
    if (data == "001" || data == "000") {
      if (currentSource != SOURCE_LINKPLAY) {
        DebugSerial.println("LP MUT ignored outside Network source");
        return;
      }
      applyLinkplayMute(data == "001");
    }
    return;
  }
  if (cmd == "PLY") {
    if (data == "001") {
      if (currentSource == SOURCE_LINKPLAY) {
        noteLinkplayTransportPlay("lp-ply");
      } else {
        noteLocalLinkplayModePlay("lp-local-ply");
      }
      return;
    }
    if (data == "000") {
      if (currentSource == SOURCE_LINKPLAY) {
        linkplayTransportPlaying = false;
      }
      playbackState = "pause";
      if (mainScreenVisible) drawPlaybackIcon(playbackState.c_str());
      return;
    }
    if (data.startsWith("INF{")) {
      applyLinkplayJsonMetadata(data.substring(3), true);
      return;
    }
  }
  if (cmd == "MEA") {
    if (data.startsWith("DAT{")) {
      applyLinkplayJsonMetadata(data.substring(3), false);
      if (linkplayPinfPending && currentSource == SOURCE_LINKPLAY && !linkplayHardOffPending && !hardOffScreenVisible) {
        linkplayPinfPending = false;
        sendLinkplayCommand("MCU+PINFGET");
      } else if (currentSource != SOURCE_LINKPLAY || linkplayHardOffPending || hardOffScreenVisible) {
        linkplayPinfPending = false;
      }
      return;
    }
    if (data == "RDY") {
      linkplayMediaReady = true;
      if (linkplayHardOffPending || hardOffScreenVisible || currentSource != SOURCE_LINKPLAY) {
        linkplayMetadataPending = false;
        linkplayPinfPending = false;
        return;
      }
      linkplayMetadataPending = false;
      linkplayPinfPending = false;
      sendLinkplayCommand("MCU+PINFGET");
      return;
    }
  }
  if (cmd == "status") {
    if (data == "play" || data == "pause" || data == "stop") {
      playbackState = data;
      if (mainScreenVisible) drawPlaybackIcon(playbackState.c_str());
    }
    return;
  }
  if (cmd == "MCU" && data == "VER") {
    sendLinkplayVersionReport();
    return;
  }
  if (cmd == "MCU" && data == "RDY") {
    updateSystemBootScreen("Connecting WiFi", 2);
    if (!linkplayReadyReported) {
      linkplayReadyReported = true;
      sendLinkplayVersionReport();
      sendLinkplayVolumeReport();
      sendLinkplayEqNeutralReport();
      sendLinkplayEqReport();
    }
    return;
  }
}

void parseLinkplayResponse(String response) {
  int start = 0;
  while (true) {
    int axxIndex = response.indexOf("AXX+", start);
    if (axxIndex < 0) {
      break;
    }
    int nextAxx = response.indexOf("AXX+", axxIndex + 4);
    String frame = nextAxx < 0 ? response.substring(axxIndex) : response.substring(axxIndex, nextAxx);
    parseLinkplayFrame(frame);
    if (nextAxx < 0) {
      break;
    }
    start = nextAxx;
  }
}

void pollLinkplaySerial() {
  bool gotByte = false;
  while (LinkplaySerial.available() > 0) {
    char c = static_cast<char>(LinkplaySerial.read());
    gotByte = true;
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      bool metadataFrame = linkplayLine.startsWith("AXX+MEA+DAT{") || linkplayLine.startsWith("AXX+PLY+INF{");
      if (!metadataFrame) {
        logLinkplayRaw("RX", linkplayLine);
        parseLinkplayResponse(linkplayLine);
        linkplayLine = "";
      }
    } else if (linkplayLine.length() < 2200) {
      linkplayLine += c;
      if (c == '&') {
        logLinkplayRaw("RX", linkplayLine);
        parseLinkplayResponse(linkplayLine);
        linkplayLine = "";
      }
    } else {
      linkplayLine = "";
      DebugSerial.println("LP RX overflow");
    }
  }
  if (gotByte) {
    lastLinkplayRxMs = millis();
  }
  if (linkplayLine.length() > 0 && millis() - lastLinkplayRxMs > 180) {
    bool metadataFrame = linkplayLine.startsWith("AXX+MEA+DAT{") || linkplayLine.startsWith("AXX+PLY+INF{");
    if (metadataFrame && linkplayLine.indexOf('}') < 0) {
      return;
    }
    logLinkplayRaw("RX", linkplayLine);
    parseLinkplayResponse(linkplayLine);
    linkplayLine = "";
  }
}

int16_t textWidth(const char *text) {
  int16_t x1 = 0;
  int16_t y1 = 0;
  uint16_t w = 0;
  uint16_t h = 0;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return static_cast<int16_t>(w);
}

void drawStringCentered(const char *text, int16_t cx, int16_t baselineY) {
  int16_t w = textWidth(text);
  tft.setCursor(cx - (w / 2), baselineY);
  tft.print(text);
}

void showBootScreen() {
  menuVisible = false;
  menuEditing = false;
  splashActive = true;
  mainScreenVisible = false;
  systemBootScreenVisible = false;
  applyBacklightForState();
  splashStartMs = millis();
  setAmpMute(true);
  tft.fillScreen(COLOR_BLACK);
  tft.drawBitmap(0, 0, heges_logo, 320, 205, COLOR_WHITE);
  tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
  tft.setFont(&FreeSans18pt7b);
  drawStringCentered("Smart Amplifier", 160, 232);
}

void showBtWaitingSplash() {
  if (!ampEnabled) {
    return;
  }
  menuVisible = false;
  menuEditing = false;
  splashActive = true;
  mainScreenVisible = false;
  hardOffScreenVisible = false;
  systemBootScreenVisible = false;
  applyBacklightForState();
  splashStartMs = millis();
  setAmpMute(true);
  tft.fillScreen(COLOR_BLACK);
  tft.setFont(&FreeSans18pt7b);
  tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
  drawStringCentered("Bluetooth", 160, 96);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(COLOR_ORANGE, COLOR_BLACK);
  drawStringCentered("Waiting for BT connection", 160, 146);
}

void drawStandby() {
  menuVisible = false;
  menuEditing = false;
  splashActive = false;
  mainScreenVisible = false;
  hardOffScreenVisible = false;
  systemBootScreenVisible = false;
  applyBacklightForState();
  setAmpMute(true);
  tft.fillScreen(uiBgColor());
  drawStandbyWifiIcon();
  drawStandbyClock();
  drawStandbyDate(true);
}

void drawStandbyWifiIcon() {
  if (mainScreenVisible || hardOffScreenVisible || systemBootScreenVisible || splashActive || menuVisible) {
    return;
  }
  constexpr int16_t x = 144;
  constexpr int16_t y = 28;
  tft.fillRect(x - 4, y - 4, WIFI_SMALL_W + 8, WIFI_SMALL_H + 8, uiBgColor());
  const uint8_t *icon = linkplayWifiReady ? wifiSmallIcon32 : noWifiSmallIcon32;
  tft.drawBitmap(x, y, icon, WIFI_SMALL_W, WIFI_SMALL_H, uiFgColor());
}

const char *weekdayName(uint8_t weekday) {
  switch (weekday) {
    case 1: return "Monday";
    case 2: return "Tuesday";
    case 3: return "Wednesday";
    case 4: return "Thursday";
    case 5: return "Friday";
    case 6: return "Saturday";
    case 7: return "Sunday";
    default: return "";
  }
}

void drawStandbyClock() {
  standbyClockCanvas.fillScreen(0);
  standbyClockCanvas.setTextWrap(false);
  standbyClockCanvas.setTextColor(1, 0);
  standbyClockCanvas.setFont(&FreeSansBold24pt7b);
  const char *text = currentTime.length() >= 5 ? currentTime.c_str() : "00:00";
  int16_t x1 = 0;
  int16_t y1 = 0;
  uint16_t w = 0;
  uint16_t h = 0;
  standbyClockCanvas.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  standbyClockCanvas.setCursor((standbyClockCanvas.width() - static_cast<int16_t>(w)) / 2, 44);
  standbyClockCanvas.print(text);
  tft.drawCanvas1(65, 70, standbyClockCanvas, uiFgColor(), uiBgColor());
}

void drawStandbyDate(bool force) {
  if (mainScreenVisible || hardOffScreenVisible || systemBootScreenVisible || splashActive || menuVisible) {
    return;
  }
  int32_t dayKey = clockValid ? static_cast<int32_t>(currentClockEpoch() / 86400UL) : -1;
  if (!force && dayKey == lastDisplayedStandbyDay) {
    return;
  }
  lastDisplayedStandbyDay = dayKey;
  tft.fillRect(0, 133, 320, 58, uiBgColor());
  if (clockValid) {
    uint16_t year = 2026;
    uint8_t month = 1;
    uint8_t day = 1;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint8_t weekday = 1;
    epochToDateTime(currentClockEpoch(), year, month, day, hour, minute, second, weekday);

    char dateBuf[12];
    snprintf(dateBuf, sizeof(dateBuf), "%02u/%02u/%04u", day, month, year);
    tft.setFont(&FreeSans12pt7b);
    tft.setTextColor(uiFgColor(), uiBgColor());
    drawStringCentered(dateBuf, 160, 153);
    drawStringCentered(weekdayName(weekday), 160, 183);
  }
}

void drawHardOffScreen(const char *title, const char *message) {
  menuVisible = false;
  menuEditing = false;
  splashActive = false;
  mainScreenVisible = false;
  hardOffScreenVisible = true;
  systemBootScreenVisible = false;
  applyBacklightForState();
  tft.fillScreen(COLOR_BLACK);

  tft.drawRoundRect(39, 42, 242, 136, 8, COLOR_DARKGREY);
  tft.fillCircle(160, 82, 16, COLOR_RED);
  tft.fillRect(157, 64, 6, 18, COLOR_BLACK);

  tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
  tft.setFont(&FreeSans18pt7b);
  drawStringCentered(title, 160, 126);
  tft.setFont(&FreeSans12pt7b);
  uint16_t messageColor = COLOR_DARKGREY;
  if (strcmp(title, "Powering Off") == 0) {
    messageColor = COLOR_ORANGE;
  } else if (strcmp(message, "Unplug power now") == 0) {
    messageColor = COLOR_RED;
  }
  tft.setTextColor(messageColor, COLOR_BLACK);
  drawStringCentered(message, 160, 158);
}

void drawSystemBootMessage(const char *message) {
  tft.fillRect(35, 99, 250, 35, COLOR_BLACK);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(COLOR_ORANGE, COLOR_BLACK);
  drawStringCentered(message, 160, 124);
}

void drawSystemBootProgress(uint8_t step, uint8_t frame) {
  tft.fillRect(70, 144, 180, 25, COLOR_BLACK);
  const int16_t startX = 83;
  const int16_t y = 156;
  for (uint8_t i = 0; i < 5; i++) {
    uint16_t color = i < step ? COLOR_GREEN : COLOR_DARKGREY;
    if (i == step && (frame & 1)) {
      color = COLOR_ORANGE;
    }
    tft.fillCircle(startX + (i * 38), y, 7, color);
  }
}

void drawSystemBootScreen(const char *message, uint8_t step, uint8_t frame) {
  tft.fillScreen(COLOR_BLACK);
  tft.drawRoundRect(31, 35, 258, 155, 8, COLOR_DARKGREY);
  tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
  tft.setFont(&FreeSans18pt7b);
  drawStringCentered("DSP Amp", 160, 80);
  drawSystemBootMessage(message);
  drawSystemBootProgress(step, frame);
}

void startSystemBootScreen() {
  menuVisible = false;
  menuEditing = false;
  splashActive = false;
  mainScreenVisible = false;
  hardOffScreenVisible = false;
  systemBootScreenVisible = true;
  systemBootStartedMs = millis();
  lastSystemBootAnimMs = 0;
  systemBootFrame = 0;
  systemBootStep = 0;
  systemBootTimedOut = false;
  systemBootMessage = "Starting system";
  applyBacklightForState();
  setAmpMute(true);
  drawSystemBootScreen(systemBootMessage.c_str(), systemBootStep, systemBootFrame);
}

void updateSystemBootScreen(const char *message, uint8_t step) {
  if (!systemBootScreenVisible || ampEnabled) {
    return;
  }
  if (systemBootMessage != message) {
    systemBootMessage = message;
    drawSystemBootMessage(systemBootMessage.c_str());
  }
  if (systemBootStep != step) {
    systemBootStep = step;
    drawSystemBootProgress(systemBootStep, systemBootFrame);
  }
}

void finishSystemBootScreen() {
  if (!systemBootScreenVisible || ampEnabled) {
    return;
  }
  systemBootScreenVisible = false;
  drawStandby();
}

void timeoutSystemBootScreen() {
  systemBootTimedOut = true;
  linkplayBootDoneReported = false;
  linkplayReadyReported = false;
  linkplayWifiReady = false;
  setDspSource(SOURCE_XLR, "XLR", false, false);
  systemBootScreenVisible = false;
  drawStandby();
}

void pollSystemBootScreen() {
  if (!systemBootScreenVisible || ampEnabled) {
    return;
  }
  uint32_t now = millis();
  if (now - lastSystemBootAnimMs >= SYSTEM_BOOT_ANIM_MS) {
    lastSystemBootAnimMs = now;
    systemBootFrame++;
    uint8_t step = systemBootFrame / 8;
    if (step > 3) {
      step = 3;
    }
    if (systemBootMessage == "Starting system") {
      updateSystemBootScreen("Waiting for Linkplay", step);
    } else if (systemBootStep != step) {
      systemBootStep = step;
    }
    drawSystemBootProgress(systemBootStep, systemBootFrame);
  }
  if (!systemBootTimedOut && lastLinkplayRxMs == 0 && now - systemBootStartedMs >= SYSTEM_BOOT_TIMEOUT_MS) {
    timeoutSystemBootScreen();
  }
}

void drawPlaybackIcon(const char *state) {
  tft.fillRect(280, 6, 40, 28, uiBgColor());
  if (strcmp(state, "play") == 0) {
    tft.fillTriangle(305, 20, 285, 10, 285, 30, uiFgColor());
  } else if (strcmp(state, "pause") == 0) {
    tft.fillRect(290, 10, 5, 20, COLOR_ORANGE);
    tft.fillRect(300, 10, 5, 20, COLOR_ORANGE);
  } else {
    tft.fillRect(285, 12, 20, 20, COLOR_RED);
  }
}

void drawTopStatusIcon() {
  static constexpr int16_t MUTE_ICON_X = 172;
  static constexpr int16_t MUTE_ICON_Y = 4;
  tft.fillRect(MUTE_ICON_X, MUTE_ICON_Y, MUTE_W, MUTE_H, uiBgColor());
  if (linkplayMuted) {
    tft.drawBitmap(MUTE_ICON_X, MUTE_ICON_Y, muteIcon32, MUTE_W, MUTE_H, uiFgColor());
    return;
  }
  if (currentSource != SOURCE_LINKPLAY) {
    return;
  }
  const uint8_t *icon = nullptr;
  if (linkplayLoopMode == 0) {
    icon = repeatAllIcon32;
  } else if (linkplayLoopMode == 1) {
    icon = repeatOneIcon32;
  } else if (linkplayLoopMode == 3) {
    icon = shuffleIcon32;
  }
  if (icon != nullptr) {
    tft.drawBitmap(MUTE_ICON_X, MUTE_ICON_Y, icon, LOOP_ICON_W, LOOP_ICON_H, uiFgColor());
  }
}

void drawSourceHeader() {
  if (!mainScreenVisible || menuVisible) {
    return;
  }
  tft.fillRect(0, 0, 279, 39, uiBgColor());
  String headerName = linkplayGroupRole == LINKPLAY_GROUP_SLAVE ? "Slave" : networkModeName;
  int16_t textX = 7;
  if (linkplayGroupRole != LINKPLAY_GROUP_SLAVE && networkModeName == "Spotify") {
    tft.drawBitmap(7, 1, SpotifyIcon, 32, 32, uiFgColor());
    textX = 45;
  } else if (linkplayGroupRole != LINKPLAY_GROUP_SLAVE && networkModeName == "AirPlay") {
    tft.drawBitmap(7, 3, AirPlayIcon, 32, 30, uiFgColor());
    textX = 45;
  } else if (linkplayGroupRole != LINKPLAY_GROUP_SLAVE && networkModeName == "Radio") {
    tft.drawBitmap(7, 1, RadioIcon, 32, 32, uiFgColor());
    textX = 45;
  }
  tft.setFont(&FreeSans18pt7b);
  tft.setTextColor(uiFgColor(), uiBgColor());
  tft.setCursor(textX, 28);
  tft.print(headerName);
  if (linkplayGroupRole == LINKPLAY_GROUP_MASTER) {
    int16_t badgeX = textX + textWidth(headerName.c_str()) + 8;
    if (badgeX > 156) {
      badgeX = 156;
    }
    tft.setFont(&FreeSans9pt7b);
    tft.setCursor(badgeX, 15);
    tft.print("M");
  }
  drawTopStatusIcon();
  drawHeaderClock();
  drawPlaybackIcon(playbackState.c_str());
}

void drawHeaderClock() {
  if (!mainScreenVisible || menuVisible) {
    return;
  }
  headerClockCanvas.fillScreen(0);
  headerClockCanvas.setTextWrap(false);
  headerClockCanvas.setTextColor(1, 0);
  headerClockCanvas.setFont(&FreeSans12pt7b);
  const char *text = currentTime.length() >= 5 ? currentTime.c_str() : "--:--";
  int16_t x1 = 0;
  int16_t y1 = 0;
  uint16_t w = 0;
  uint16_t h = 0;
  headerClockCanvas.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  headerClockCanvas.setCursor((headerClockCanvas.width() - static_cast<int16_t>(w)) / 2, 20);
  headerClockCanvas.print(text);
  tft.drawCanvas1(208, 7, headerClockCanvas, uiFgColor(), uiBgColor());
}

void formatMs(uint32_t ms, char *out, size_t outSize) {
  uint32_t totalSeconds = ms / 1000UL;
  uint32_t minutes = totalSeconds / 60UL;
  uint32_t seconds = totalSeconds % 60UL;
  snprintf(out, outSize, "%02lu:%02lu", static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
}

void drawTimers() {
  if (!mainScreenVisible || menuVisible) {
    return;
  }
  uint32_t position = trackPositionMs;
  if (playbackState == "play" && lastTrackTickMs != 0) {
    position += millis() - lastTrackTickMs;
  }
  if (trackLengthMs > 0 && position > trackLengthMs) {
    position = trackLengthMs;
  }
  char elapsed[8];
  char total[8];
  formatMs(position, elapsed, sizeof(elapsed));
  formatMs(trackLengthMs, total, sizeof(total));

  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(uiFgColor(), uiBgColor());
  tft.fillRect(0, BOTTOM_INFO_Y, 86, 29, uiBgColor());
  tft.setCursor(4, BOTTOM_INFO_BASELINE_Y);
  tft.print(elapsed);
  tft.fillRect(242, BOTTOM_INFO_Y, 78, 29, uiBgColor());
  tft.setCursor(255, BOTTOM_INFO_BASELINE_Y);
  tft.print(total);
}

void drawStaticScale() {
  int scaleY = VU_Y + 90;
  tft.fillRect(0, scaleY - 6, VU_WIDTH, 31, uiBgColor());
  tft.fillRect(0, scaleY, VU_WIDTH, 3, COLOR_DARKGREEN);
  int dbValues[] = {-45, -30, -15, -5, 3};
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_ORANGE, uiBgColor());
  tft.setCursor(2, scaleY + 18);
  tft.print("dB");

  for (int i = 0; i < 5; i++) {
    int segIndex = map(dbValues[i], -60, 3, 0, NUM_SEGMENTS - 1);
    int x = segIndex * (SEGMENT_WIDTH + 2);
    tft.drawLine(x, scaleY - 5, x, scaleY + 5, COLOR_ORANGE);
    char label[8];
    snprintf(label, sizeof(label), dbValues[i] >= 0 ? "+%d" : "%d", dbValues[i]);
    tft.setCursor(x - 9, scaleY + 18);
    tft.print(label);
  }
}

uint16_t vuColorForSegment(int segment) {
  if (segment >= 14) return COLOR_RED;
  if (segment >= 11) return COLOR_ORANGE;
  if (segment >= 8) return COLOR_YELLOW;
  if (segment >= 4) return COLOR_GREENYELLOW;
  return COLOR_GREEN;
}

void drawVuRow(int16_t y, int activeSegments, int peakSegment) {
  tft.fillRect(0, y, VU_WIDTH, BAR_HEIGHT, uiBgColor());
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    uint16_t color = i < activeSegments ? vuColorForSegment(i) : uiDimSegmentColor();
    tft.fillRoundRect(i * (SEGMENT_WIDTH + 2), y, SEGMENT_WIDTH, BAR_HEIGHT, 3, color);
  }
  if (peakSegment >= 0 && peakSegment < NUM_SEGMENTS) {
    int x = peakSegment * (SEGMENT_WIDTH + 2);
    tft.fillRoundRect(x, y, SEGMENT_WIDTH, BAR_HEIGHT, 3, COLOR_RED);
  }
}

void drawVuSegment(int16_t y, int segment, bool active, bool peak) {
  if (segment < 0 || segment >= NUM_SEGMENTS) {
    return;
  }
  int x = segment * (SEGMENT_WIDTH + 2);
  uint16_t color = peak ? COLOR_RED : (active ? vuColorForSegment(segment) : uiDimSegmentColor());
  tft.fillRoundRect(x, y, SEGMENT_WIDTH, BAR_HEIGHT, 3, color);
}

void updateVuRowDiff(int16_t y, int activeSegments, int peakSegment, int &lastActive, int &lastPeak) {
  if (lastActive < 0) {
    drawVuRow(y, activeSegments, peakSegment);
    lastActive = activeSegments;
    lastPeak = peakSegment;
    return;
  }

  int low = min(lastActive, activeSegments);
  int high = max(lastActive, activeSegments);
  for (int i = low; i < high; i++) {
    drawVuSegment(y, i, i < activeSegments, i == peakSegment);
  }

  if (lastPeak != peakSegment) {
    if (lastPeak >= 0 && (lastPeak < low || lastPeak >= high)) {
      drawVuSegment(y, lastPeak, lastPeak < activeSegments, false);
    }
    if (peakSegment >= 0 && (peakSegment < low || peakSegment >= high)) {
      drawVuSegment(y, peakSegment, peakSegment < activeSegments, true);
    }
  }

  lastActive = activeSegments;
  lastPeak = peakSegment;
}

String currentScrollText() {
  if (currentTitle == "No Info" && currentArtist.length() == 0 && currentAlbum.length() == 0) {
    return "";
  }
  if (currentArtist.length() > 0 || currentAlbum.length() > 0) {
    String text;
    if (currentArtist.length() > 0) {
      text += currentArtist;
      text += " - ";
    }
    text += currentTitle.length() > 0 ? currentTitle : "No Info";
    if (currentAlbum.length() > 0 && currentAlbum != "Network" && currentAlbum != "Unknown") {
      text += " - ";
      text += currentAlbum;
    }
    return text;
  }
  return currentTitle.length() > 0 ? currentTitle : "No Info";
}

void drawScrollingTitle(bool force = false) {
  if (!mainScreenVisible || menuVisible) {
    return;
  }

  String text = currentScrollText();
  if (text.length() == 0) {
    if (force || lastScrollText.length() != 0) {
      titleCanvas.fillScreen(0);
      tft.drawCanvas1(0, TITLE_Y, titleCanvas, uiFgColor(), uiBgColor());
      lastScrollText = "";
    }
    return;
  }
  if (force || text != lastScrollText) {
    lastScrollText = text;
    scrollOffset = 320;
    scrollIdle = false;
    scrollIdleStartMs = millis();
  }

  titleCanvas.fillScreen(0);
  titleCanvas.setFont(&FreeSans12pt7b);
  titleCanvas.setTextColor(1, 0);
  titleCanvas.setTextWrap(false);

  int16_t textW = textWidth(text.c_str());
  const int16_t baselineY = 22;
  if (textW <= 300) {
    titleCanvas.setCursor((320 - textW) / 2, baselineY);
    titleCanvas.print(text);
    tft.drawCanvas1(0, TITLE_Y, titleCanvas, uiFgColor(), uiBgColor());
    return;
  }

  if (scrollIdle) {
    titleCanvas.setCursor(0, baselineY);
    titleCanvas.print(text);
    tft.drawCanvas1(0, TITLE_Y, titleCanvas, uiFgColor(), uiBgColor());
    if (millis() - scrollIdleStartMs > 3000) {
      scrollIdle = false;
      scrollOffset = 320;
    }
    return;
  }

  if (millis() - scrollIdleStartMs < 350) {
    titleCanvas.setCursor(0, baselineY);
    titleCanvas.print(text);
    tft.drawCanvas1(0, TITLE_Y, titleCanvas, uiFgColor(), uiBgColor());
    return;
  }

  scrollOffset -= 1;
  if (-scrollOffset > textW + 10) {
    scrollIdle = true;
    scrollIdleStartMs = millis();
    scrollOffset = 0;
  }
  titleCanvas.setCursor(scrollOffset, baselineY);
  titleCanvas.print(text);
  tft.drawCanvas1(0, TITLE_Y, titleCanvas, uiFgColor(), uiBgColor());
}

void drawVolumeValue(uint8_t percent) {
  if (menuVisible) {
    return;
  }
  tft.fillRect(86, BOTTOM_INFO_Y, 156, 29, uiBgColor());
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(uiFgColor(), uiBgColor());
  char label[20];
  if (bottomStatusText.length() > 0 && millis() < bottomStatusUntilMs) {
    snprintf(label, sizeof(label), "%s", bottomStatusText.c_str());
  } else if (linkplayMuted) {
    snprintf(label, sizeof(label), "Mute");
  } else {
    snprintf(label, sizeof(label), "Volume %u", percent);
  }
  drawStringCentered(label, 160, BOTTOM_INFO_BASELINE_Y);
}

void showBottomStatus(const String &text, uint32_t durationMs) {
  bottomStatusText = text;
  bottomStatusUntilMs = millis() + durationMs;
  if (mainScreenVisible && !menuVisible) {
    drawVolumeValue(displayedVolumePercent);
  }
}

void selectSourceFromMenu(DspSource source) {
  if (source == SOURCE_LINKPLAY) {
    pauseBtTransport("menu-source");
  } else if (source == SOURCE_BLUETOOTH) {
    pauseLinkplayTransport("menu-source");
  }
  setDspSource(source, sourceDisplayName(source), false);
  sendLinkplaySourceSelect(source);
  if (source == SOURCE_LINKPLAY) {
    localSourceTitleLocked = false;
    requestLinkplayMetadata(true);
  }
}

void cycleSourceFromControl(const char *source) {
  int next = (static_cast<int>(currentSource) + 1) % 4;
  DspSource nextSource = static_cast<DspSource>(next);
  if (nextSource == SOURCE_LINKPLAY) {
    pauseBtTransport(source);
  } else if (nextSource == SOURCE_BLUETOOTH) {
    pauseLinkplayTransport(source);
  }
  setDspSource(nextSource, sourceDisplayName(nextSource), !menuVisible);
  sendLinkplaySourceSelect(nextSource);
  if (nextSource == SOURCE_LINKPLAY) {
    localSourceTitleLocked = false;
    requestLinkplayMetadata(true);
  }
  if (menuVisible) {
    drawMenuScreen();
  }
}

void togglePlaybackFromControl(const char *source) {
  if (currentSource == SOURCE_LINKPLAY) {
    sendLinkplayCommand("MCU+PLY+PUS");
  } else if (currentSource == SOURCE_BLUETOOTH) {
    sendBtCommand("AT+PP");
  }
}

void skipTrackFromControl(int direction, const char *source) {
  (void)source;
  if (direction == 0 || menuVisible || linkplayHardOffPending || hardOffScreenVisible) {
    return;
  }
  if (currentSource == SOURCE_LINKPLAY) {
    sendLinkplayCommand(direction > 0 ? "MCU+PLY+NXT" : "MCU+PLY+PRV");
    requestLinkplayMetadata(true);
  } else if (currentSource == SOURCE_BLUETOOTH) {
    sendBtCommand(direction > 0 ? "AT+PN" : "AT+PV");
  }
}

uint8_t menuItemCount() {
  switch (menuPage) {
    case MENU_PAGE_TONE: return 4;
    case MENU_PAGE_INPUT_GAINS: return 5;
    case MENU_PAGE_SCREEN_SETTINGS: return 3;
    case MENU_PAGE_BACKLIGHT: return 3;
    case MENU_PAGE_CLOCK: return 7;
    default: return MENU_ITEM_COUNT;
  }
}

String menuTitle() {
  switch (menuPage) {
    case MENU_PAGE_TONE: return "Tone";
    case MENU_PAGE_INPUT_GAINS: return "Input Gains";
    case MENU_PAGE_SCREEN_SETTINGS: return "Screen Settings";
    case MENU_PAGE_BACKLIGHT: return "Brightness";
    case MENU_PAGE_CLOCK: return "Date/Time";
    default: return "Menu";
  }
}

String formatDb(int value) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%+ddB", value);
  return buf;
}

String menuItemName(uint8_t index) {
  if (menuPage == MENU_PAGE_MAIN) {
    switch (index) {
      case MENU_ITEM_SOURCE: return "Source";
      case MENU_ITEM_OUTPUT_MODE: return "Output Mode";
      case MENU_ITEM_TONE: return "Tone";
      case MENU_ITEM_INPUT_GAINS: return "Input Gains";
      case MENU_ITEM_LCD_BL: return "Screen Settings";
      case MENU_ITEM_CLOCK: return "Date/Time";
      case MENU_ITEM_EXIT: return "Exit";
      default: return "";
    }
  }
  if (menuPage == MENU_PAGE_TONE) {
    if (index == 0) return "Bass";
    if (index == 1) return "Treble";
    if (index == 2) return "Sub";
    return "Return";
  }
  if (menuPage == MENU_PAGE_INPUT_GAINS) {
    if (index == 0) return "Network";
    if (index == 1) return "XLR";
    if (index == 2) return "Bluetooth";
    if (index == 3) return "Optical";
    return "Return";
  }
  if (menuPage == MENU_PAGE_SCREEN_SETTINGS) {
    if (index == 0) return "Brightness";
    if (index == 1) return "Theme";
    return "Return";
  }
  if (menuPage == MENU_PAGE_BACKLIGHT) {
    if (index == 0) return "On Level";
    if (index == 1) return "Standby";
    return "Return";
  }
  if (menuPage == MENU_PAGE_CLOCK) {
    if (index == 0) return "Day";
    if (index == 1) return "Month";
    if (index == 2) return "Year";
    if (index == 3) return "Hour";
    if (index == 4) return "Minute";
    if (index == 5) return "Get NTP";
    return "Return";
  }
  return "";
}

String menuItemValue(uint8_t index) {
  char buf[24];
  if (menuPage == MENU_PAGE_MAIN) {
    switch (index) {
      case MENU_ITEM_SOURCE: return sourceDisplayName(currentSource);
      case MENU_ITEM_OUTPUT_MODE: return outputModeName(currentOutputMode);
      case MENU_ITEM_CLOCK: return currentTime;
      default: return "";
    }
  }
  if (menuPage == MENU_PAGE_TONE) {
    if (index == 0) return formatDb(bassGainDb);
    if (index == 1) return formatDb(trebleGainDb);
    if (index == 2) return subEnabled ? "On" : "Off";
    return "";
  }
  if (menuPage == MENU_PAGE_INPUT_GAINS) {
    if (index == 0) return formatDb(lpGainDb);
    if (index == 1) return formatDb(xlrGainDb);
    if (index == 2) return formatDb(btGainDb);
    if (index == 3) return formatDb(opticalGainDb);
    return "";
  }
  if (menuPage == MENU_PAGE_SCREEN_SETTINGS) {
    if (index == 0) {
      snprintf(buf, sizeof(buf), "%u/%u%%", backlightOnPercent, backlightStandbyPercent);
      return buf;
    }
    if (index == 1) return screenLightTheme ? "Light" : "Dark";
    return "";
  }
  if (menuPage == MENU_PAGE_BACKLIGHT) {
    if (index == 0) {
      snprintf(buf, sizeof(buf), "%u%%", backlightOnPercent);
      return buf;
    }
    if (index == 1) {
      snprintf(buf, sizeof(buf), "%u%%", backlightStandbyPercent);
      return buf;
    }
    return "";
  }
  if (menuPage == MENU_PAGE_CLOCK) {
    uint16_t year = 2026;
    uint8_t month = 1;
    uint8_t day = 1;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    currentClockParts(year, month, day, hour, minute, second);
    if (index == 0) {
      snprintf(buf, sizeof(buf), "%02u", day);
      return buf;
    }
    if (index == 1) {
      snprintf(buf, sizeof(buf), "%02u", month);
      return buf;
    }
    if (index == 2) {
      snprintf(buf, sizeof(buf), "%04u", year);
      return buf;
    }
    if (index == 3) {
      snprintf(buf, sizeof(buf), "%02u", hour);
      return buf;
    }
    if (index == 4) {
      snprintf(buf, sizeof(buf), "%02u", minute);
      return buf;
    }
    if (index == 5) return linkplayWifiReady ? "Ready" : "No WiFi";
  }
  return "";
}

void drawMenuRow(uint8_t index, bool selected) {
  int16_t y = 66 + (index * 29);
  uint16_t bg = selected ? uiSelectedBgColor() : uiBgColor();
  uint16_t fg = selected ? uiSelectedFgColor() : uiFgColor();
  tft.fillRect(0, y - 23, 320, 33, uiBgColor());
  tft.fillRoundRect(8, y - 19, 304, 25, 4, bg);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(fg, bg);
  tft.setCursor(22, y);
  tft.print(menuItemName(index));
  String value = menuItemValue(index);
  if (value.length() > 0) {
    int16_t w = textWidth(value.c_str());
    tft.setCursor(296 - w, y);
    tft.print(value);
  }
}

void drawMenuScreen() {
  if (!menuVisible) {
    return;
  }

  tft.fillScreen(uiBgColor());
  tft.fillRect(0, 40, 320, 3, COLOR_DARKGREEN);
  tft.setFont(&FreeSans18pt7b);
  tft.setTextColor(uiFgColor(), uiBgColor());
  tft.setCursor(8, 29);
  tft.print(menuTitle());
  uint8_t count = menuItemCount();
  for (uint8_t i = 0; i < count; i++) {
    drawMenuRow(i, i == menuIndex);
  }
}

void enterMenu() {
  if (!mainScreenVisible || splashActive) {
    return;
  }
  menuVisible = true;
  menuEditing = false;
  menuPage = MENU_PAGE_MAIN;
  menuIndex = 0;
  lastMenuActivityMs = millis();
  drawMenuScreen();
}

void exitMenu() {
  if (!menuVisible) {
    return;
  }
  flushClockIfDirty();
  menuVisible = false;
  menuEditing = false;
  if (audioSettingsDirty) {
    lastAudioSettingsChangeMs = millis();
  }
  drawMainScreen();
}

void menuMove(int delta) {
  if (!menuVisible) {
    return;
  }
  lastMenuActivityMs = millis();
  menuEditing = false;
  uint8_t previousIndex = menuIndex;
  int next = static_cast<int>(menuIndex) + delta;
  uint8_t count = menuItemCount();
  while (next < 0) {
    next += count;
  }
  menuIndex = static_cast<uint8_t>(next % count);
  if (menuIndex != previousIndex) {
    drawMenuRow(previousIndex, false);
    drawMenuRow(menuIndex, true);
  }
}

bool menuSelectedItemIsAdjustable() {
  if (!menuVisible) {
    return false;
  }
  if (menuPage == MENU_PAGE_MAIN) {
    return false;
  }
  if (menuPage == MENU_PAGE_CLOCK) {
    return menuIndex < 5;
  }
  if (menuPage == MENU_PAGE_SCREEN_SETTINGS) {
    return false;
  }
  return menuIndex < menuItemCount() - 1;
}

void menuAdjust(int delta) {
  if (!menuVisible || delta == 0) {
    return;
  }
  lastMenuActivityMs = millis();

  if (menuPage == MENU_PAGE_MAIN) {
    if (menuIndex == MENU_ITEM_SOURCE) {
      int next = static_cast<int>(currentSource) + delta;
      while (next < 0) {
        next += 4;
      }
      selectSourceFromMenu(static_cast<DspSource>(next % 4));
    } else if (menuIndex == MENU_ITEM_OUTPUT_MODE) {
      setOutputMode(outputModeAdjusted(currentOutputMode, delta));
    }
    drawMenuScreen();
    return;
  }

  if (menuPage == MENU_PAGE_TONE) {
    if (menuIndex == 0) {
      setBassGainDb(constrain(bassGainDb + delta, -12, 12), true, true);
    } else if (menuIndex == 1) {
      setTrebleGainDb(constrain(trebleGainDb + delta, -12, 12), true, true);
    } else if (menuIndex == 2) {
      setSubEnabled(!subEnabled);
    }
    drawMenuScreen();
    return;
  }

  if (menuPage == MENU_PAGE_INPUT_GAINS) {
    DspSource source = SOURCE_LINKPLAY;
    if (menuIndex == 1) source = SOURCE_XLR;
    else if (menuIndex == 2) source = SOURCE_BLUETOOTH;
    else if (menuIndex == 3) source = SOURCE_OPTICAL;
    if (menuIndex < 4) {
      int next = *inputGainState(source) + delta;
      setInputGainDb(source, next);
      audioSettingsSaveOnStandby = true;
    }
    drawMenuScreen();
    return;
  }

  if (menuPage == MENU_PAGE_BACKLIGHT) {
    if (menuIndex == 0) {
      backlightOnPercent = constrain(static_cast<int>(backlightOnPercent) + (delta * 5), 5, 100);
      applyBacklightForState();
      scheduleAudioSettingsSave();
    } else if (menuIndex == 1) {
      backlightStandbyPercent = constrain(static_cast<int>(backlightStandbyPercent) + (delta * 5), 0, 100);
      applyBacklightForState();
      scheduleAudioSettingsSave();
    }
    drawMenuScreen();
    return;
  }

  if (menuPage == MENU_PAGE_CLOCK) {
    if (menuIndex == 0) {
      adjustClockDays(delta);
    } else if (menuIndex == 1) {
      adjustClockMonths(delta);
    } else if (menuIndex == 2) {
      adjustClockYears(delta);
    } else if (menuIndex == 3) {
      adjustClockMinutes(delta * 60);
    } else if (menuIndex == 4) {
      adjustClockMinutes(delta);
    }
    drawMenuScreen();
  }
}

void menuBack() {
  if (!menuVisible) {
    return;
  }
  lastMenuActivityMs = millis();
  if (menuPage == MENU_PAGE_MAIN) {
    exitMenu();
    return;
  }
  if (menuPage == MENU_PAGE_CLOCK) {
    flushClockIfDirty();
  }
  menuPage = menuPage == MENU_PAGE_BACKLIGHT ? MENU_PAGE_SCREEN_SETTINGS : MENU_PAGE_MAIN;
  menuEditing = false;
  menuIndex = 0;
  drawMenuScreen();
}

void menuSelect() {
  if (!menuVisible) {
    return;
  }
  lastMenuActivityMs = millis();
  if (menuPage != MENU_PAGE_MAIN) {
    if (menuIndex == menuItemCount() - 1) {
      menuBack();
      return;
    }
    if (menuPage == MENU_PAGE_CLOCK && menuIndex == 5) {
      requestClockNtpFromMenu();
      drawMenuScreen();
      return;
    }
    if (menuPage == MENU_PAGE_SCREEN_SETTINGS) {
      if (menuIndex == 0) {
        menuPage = MENU_PAGE_BACKLIGHT;
        menuIndex = 0;
        menuEditing = false;
        drawMenuScreen();
      } else if (menuIndex == 1) {
        setScreenLightTheme(!screenLightTheme);
      }
      return;
    }
    return;
  }

  switch (menuIndex) {
    case MENU_ITEM_SOURCE:
    case MENU_ITEM_OUTPUT_MODE:
      menuAdjust(1);
      break;
    case MENU_ITEM_TONE:
      menuPage = MENU_PAGE_TONE;
      menuIndex = 0;
      menuEditing = false;
      drawMenuScreen();
      break;
    case MENU_ITEM_INPUT_GAINS:
      menuPage = MENU_PAGE_INPUT_GAINS;
      menuIndex = 0;
      menuEditing = false;
      drawMenuScreen();
      break;
    case MENU_ITEM_LCD_BL:
      menuPage = MENU_PAGE_SCREEN_SETTINGS;
      menuIndex = 0;
      menuEditing = false;
      drawMenuScreen();
      break;
    case MENU_ITEM_CLOCK:
      menuPage = MENU_PAGE_CLOCK;
      menuIndex = 0;
      menuEditing = false;
      drawMenuScreen();
      break;
    case MENU_ITEM_EXIT:
      exitMenu();
      break;
    default:
      break;
  }
}

void encoderMenuPress() {
  if (!menuVisible) {
    return;
  }
  if (menuPage != MENU_PAGE_MAIN) {
    if (menuIndex == menuItemCount() - 1) {
      menuBack();
    } else if (menuPage == MENU_PAGE_CLOCK && menuIndex == 5) {
      requestClockNtpFromMenu();
      drawMenuScreen();
    } else if (menuPage == MENU_PAGE_SCREEN_SETTINGS) {
      menuSelect();
    } else {
      menuMove(1);
    }
    return;
  }
  menuSelect();
}

void updateDspMetersOnDisplay(bool force = false) {
  if (!mainScreenVisible || menuVisible) {
    return;
  }
  static uint8_t peakDecayDivider = 0;

  uint32_t leftRaw = 0;
  uint32_t rightRaw = 0;
  if (readDspU32(DspMap::kRtaLeft, leftRaw) && readDspU32(DspMap::kRtaRight, rightRaw)) {
    int leftSegments = rtaRawToSegments(leftRaw);
    int rightSegments = rtaRawToSegments(rightRaw);
    int previousPeakLeft = displayedPeakLeft;
    int previousPeakRight = displayedPeakRight;
    bool decayPeak = force || (++peakDecayDivider >= 2);
    if (decayPeak) {
      peakDecayDivider = 0;
    }
    displayedPeakLeft = max(displayedPeakLeft - (decayPeak ? 1 : 0), leftSegments - 1);
    displayedPeakRight = max(displayedPeakRight - (decayPeak ? 1 : 0), rightSegments - 1);

    if (force || leftSegments != displayedVuLeft || displayedPeakLeft != previousPeakLeft) {
      displayedVuLeft = leftSegments;
      updateVuRowDiff(54, leftSegments, displayedPeakLeft, lastDrawnLeftSegments, lastDrawnLeftPeak);
    }
    if (force || rightSegments != displayedVuRight || displayedPeakRight != previousPeakRight) {
      displayedVuRight = rightSegments;
      updateVuRowDiff(94, rightSegments, displayedPeakRight, lastDrawnRightSegments, lastDrawnRightPeak);
    }
  }
}

void drawMainScreen() {
  menuVisible = false;
  menuEditing = false;
  splashActive = false;
  mainScreenVisible = true;
  hardOffScreenVisible = false;
  applyBacklightForState();
  tft.fillScreen(uiBgColor());

  drawSourceHeader();

  tft.fillRect(0, 40, 320, 3, COLOR_DARKGREEN);
  tft.fillRect(0, 163, 320, 3, COLOR_DARKGREEN);
  tft.fillRect(0, 198, 320, 3, COLOR_DARKGREEN);
  tft.fillRect(0, 232, 320, 3, COLOR_DARKGREEN);

  displayedVuLeft = -1;
  displayedVuRight = -1;
  displayedPeakLeft = -1;
  displayedPeakRight = -1;
  lastDrawnLeftSegments = -1;
  lastDrawnRightSegments = -1;
  lastDrawnLeftPeak = -1;
  lastDrawnRightPeak = -1;
  drawVuRow(54, 0, -1);
  drawVuRow(94, 0, -1);
  lastDrawnLeftSegments = 0;
  lastDrawnRightSegments = 0;
  lastDrawnLeftPeak = -1;
  lastDrawnRightPeak = -1;
  drawStaticScale();

  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(uiFgColor(), uiBgColor());
  lastScrollText = "";
  drawScrollingTitle(true);

  drawVolumeValue(displayedVolumePercent);
  drawTimers();
  updateDspMetersOnDisplay(true);
  updateBtEnableForSource(currentSource == SOURCE_BLUETOOTH);
  requestLinkplayMetadata();
}

void setup() {
  setupSafePins();

  DebugSerial.begin(DEBUG_BAUD);
  LinkplaySerial.begin(LINKPLAY_BAUD);
  BtSerial.begin(BT_BAUD);
  delay(50);
  DebugSerial.println("STM32DSP1466 raw ST7789 + GFX UI bring-up");
  DebugSerial.println("Linkplay USART1 ready on PA9/PA10");
  DebugSerial.println("Bluetooth USART6 ready on PC6/PC7");
  DebugSerial.println("Encoder ready on CLK=PD14 DT=PD13 SW=PD12");
  DebugSerial.println("IR receiver ready on PD15");
  initRtcClockSource();
  loadClockFromEeprom();
  loadAudioSettingsFromEeprom();
  loadScreenSettingsFromEeprom();

  DspSpi.begin();
  irLastEdgeUs = micros();
  attachInterrupt(digitalPinToInterrupt(IR_RECEIVER_PIN), handleIrEdge, CHANGE);
  applyStoredAudioSettingsToDsp();
  tft.begin();
  startSystemBootScreen();
  printHelp();
  printDspStatus();
}

void loop() {
  static bool lastButton = HIGH;
  static uint32_t lastToggleMs = 0;
  static uint32_t lastMeterUpdateMs = 0;

  pollLinkplaySerial();
  pollDebugSerial();
  pollBtSerial();
  pollBtReconnect();
  pollBtNameRequest();
  pollEncoder();
  pollIrReceiver();
  pollLinkplayHardOffRequest();

  bool button = digitalRead(POWER_BUTTON_PIN);
  if (lastButton == HIGH && button == LOW && millis() - lastToggleMs > 250) {
    lastToggleMs = millis();
    toggleAmpPowerFromControl("button");
  }
  lastButton = button;

  if (splashActive && millis() - splashStartMs >= 2000) {
    setAmpMute(linkplayMuted);
    drawMainScreen();
  }

  if (menuVisible && static_cast<int32_t>(millis() - lastMenuActivityMs - MENU_TIMEOUT_MS) >= 0) {
    exitMenu();
  }

  if (mainScreenVisible && millis() - lastMeterUpdateMs >= METER_UPDATE_MS) {
    lastMeterUpdateMs = millis();
    updateDspMetersOnDisplay();
    pollLinkplaySerial();
    pollBtSerial();
    pollEncoder();
    pollIrReceiver();
  }
  if (mainScreenVisible && millis() - lastScrollUpdateMs >= 31) {
    lastScrollUpdateMs = millis();
    drawScrollingTitle();
    pollLinkplaySerial();
    pollBtSerial();
    pollEncoder();
    pollIrReceiver();
  }
  if (mainScreenVisible && millis() - lastTimerUpdateMs >= 1000) {
    lastTimerUpdateMs = millis();
    String previousTime = currentTime;
    updateCurrentTimeString();
    if (currentTime != previousTime) {
      drawHeaderClock();
    }
    drawTimers();
    pollLinkplaySerial();
    pollBtSerial();
    pollEncoder();
    pollIrReceiver();
  }
  if (mainScreenVisible && bottomStatusText.length() > 0 && static_cast<int32_t>(millis() - bottomStatusUntilMs) >= 0) {
    bottomStatusText = "";
    drawVolumeValue(displayedVolumePercent);
  }
  pollSystemBootScreen();
  if (btPauseSettleActive && static_cast<int32_t>(millis() - btPauseSettleUntilMs) >= 0) {
    btPauseSettleActive = false;
  }
  pollBtStandbyDisconnect();
  pollBtTakeover();
  pollStreamMuteStandby();
  pollLinkplayHardOffRequest();
  pollLinkplayRtcSync();
  if (audioSettingsDirty &&
      !menuVisible &&
      bottomStatusText.length() == 0 &&
      static_cast<int32_t>(millis() - lastAudioSettingsChangeMs - AUDIO_SETTINGS_SAVE_DELAY_MS) >= 0) {
    saveAudioSettingsToEeprom();
  }
  if (!mainScreenVisible && !splashActive && !hardOffScreenVisible && !systemBootScreenVisible && clockValid && millis() - lastTimerUpdateMs >= 1000) {
    lastTimerUpdateMs = millis();
    String previousTime = currentTime;
    updateCurrentTimeString();
    if (currentTime != previousTime) {
      drawStandbyClock();
      drawStandbyDate();
    }
  }

  pollLinkplaySerial();
  pollLinkplayMetadataRequest();
  pollDebugSerial();
  pollIrReceiver();
  delay(5);
}
