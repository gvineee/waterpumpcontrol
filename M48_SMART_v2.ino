/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║          მუხათწყაროს M48 SMART — v2.0 FINAL                ║
 * ║          ESP32 DEVKIT V1 | ჭის ტუმბოს კონტროლერი           ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  • WiFi + Telegram ბოტი + ვებ-გვერდი                        ║
 * ║  • Dry Run დაცვა + Overcurrent დაცვა                        ║
 * ║  • 15წთ ON / 1სთ REST duty cycle (ავტო-რეჟიმი)             ║
 * ║  • PIR ქურდობის საწინააღმდეგო სენსორი                       ║
 * ║  • ავზი: ≤50% → ჩართვა, 100% → გამორთვა                    ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  კვება: 5V / 3A (მინ. 2A)                                    ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * ════════════════ PINOUT / გამავლობა ════════════════
 *
 *  ESP32 DEVKIT V1 (30-pin)
 *  ┌─────────────────────────────┐
 *  │  GPIO 25 → PUMP RELAY  IN   │  ← ტუმბოს relay (active LOW)
 *  │  GPIO 26 → VALVE RELAY IN   │  ← სარქველის relay (active LOW)
 *  │  GPIO 27 → PIR DATA         │  ← PIR HC-SR501 გამოსვლა
 *  │  GPIO 32 → MID FLOAT        │  ← შუა სენსორი (INPUT_PULLUP, GND-ზე)
 *  │  GPIO 33 → BTM FLOAT        │  ← ქვედა სენსორი (INPUT_PULLUP, GND-ზე)
 *  │  GPIO 34 → CURRENT SENSOR   │  ← CT სენსორი (INPUT ONLY)
 *  │  GPIO 14 → TOP FLOAT        │  ← ზედა სენსორი (INPUT_PULLUP, GND-ზე)
 *  │                             │
 *  │  GPIO  4 → LCD RS           │  ← LCD 8x2, 4-bit რეჟიმი
 *  │  GPIO 16 → LCD E            │  ⚠️ WROVER-ზე PSRAM პინი!
 *  │  GPIO 17 → LCD D4           │  ⚠️ WROVER-ზე PSRAM პინი!
 *  │  GPIO  5 → LCD D5           │
 *  │  GPIO 18 → LCD D6           │
 *  │  GPIO 19 → LCD D7           │
 *  │                             │
 *  │  3.3V → PIR VCC             │
 *  │  3.3V → RELAY VCC           │  ⚠️ relay-ს VCC ≠ IN ლოგიკა
 *  │  GND  → ყველა GND           │
 *  └─────────────────────────────┘
 *
 * ════════════ გამავლობის სქემა (სრული) ════════════
 *
 *  ┌─────────────────────────────────────────────────────┐
 *  │                  5V / 3A PSU                        │
 *  │  +5V ────┬──── ESP32 VIN                            │
 *  │          ├──── RELAY VCC (JD-VCC)                   │
 *  │          └──── LCD VCC + Potentiometer              │
 *  │  GND ────┴──── ყველა კომპონენტი                    │
 *  └─────────────────────────────────────────────────────┘
 *
 *  RELAY MODULE (2-channel, active LOW, optocoupler):
 *  ┌──────────────────────────────────────────┐
 *  │  JD-VCC ← 5V                            │
 *  │  VCC    ← 3.3V (ESP32-დან)              │  ⚠️ ცალკე!
 *  │  GND    ← GND                           │
 *  │  IN1    ← GPIO 25 (PUMP)                │
 *  │  IN2    ← GPIO 26 (VALVE)               │
 *  │                                         │
 *  │  COM1 ──── 220V ფაზა                   │  ← ტუმბო
 *  │  NO1  ──── ტუმბოს L კვება              │
 *  │  COM2 ──── 220V ფაზა                   │  ← სარქველი
 *  │  NO2  ──── სარქველის L კვება           │
 *  └──────────────────────────────────────────┘
 *
 *  CT დენის სენსორი (SCT-013-100 ან 100mV/A):
 *  ┌──────────────────────────────────────────┐
 *  │  CT ნიჭა ├── ტუმბოს ერთ-ერთ კაბელზე   │
 *  │  OUT+ ───── GPIO 34                     │
 *  │  OUT- ───── GND                         │
 *  │  Burden R ─ 33Ω (ჩაშენებულია CT-ში)   │
 *  └──────────────────────────────────────────┘
 *
 *  FLOAT SWITCHES (ავზის სენსორები):
 *  ┌──────────────────────────────────────────┐
 *  │  TOP კონტაქტი 1 ── 3.3V                │
 *  │  TOP კონტაქტი 2 ── GPIO 36             │
 *  │                 └── 10kΩ ── GND         │  ← pull-down სავალდებულო!
 *  │                                         │
 *  │  MID: ზემოთ ანალოგიურად → GPIO 32      │
 *  │  BTM: ზემოთ ანალოგიურად → GPIO 33      │
 *  └──────────────────────────────────────────┘
 *
 *  PIR HC-SR501:
 *  ┌──────────────────────────────────────────┐
 *  │  VCC  ← 3.3V (ან 5V თუ ბმულია 5V-ზე)  │
 *  │  GND  ← GND                             │
 *  │  OUT  → GPIO 27                         │
 *  │  Sensit. pot → ½ (საშუალო)             │
 *  │  Time pot    → მინიმუმი (3 წამი)        │
 *  └──────────────────────────────────────────┘
 *
 *  LCD 16x2 / 8x2 (HD44780, 4-bit):
 *  ┌──────────────────────────────────────────┐
 *  │  LCD 1 (VSS)  ── GND                    │
 *  │  LCD 2 (VDD)  ── 5V                     │
 *  │  LCD 3 (V0)   ── 10kΩ pot (კონტრასტი)  │
 *  │  LCD 4 (RS)   ── GPIO 4                 │
 *  │  LCD 5 (RW)   ── GND (ყოველთვის)        │
 *  │  LCD 6 (E)    ── GPIO 16                │
 *  │  LCD 7-10     ── არ ერთვება (4-bit)     │
 *  │  LCD 11 (D4)  ── GPIO 17                │
 *  │  LCD 12 (D5)  ── GPIO 5                 │
 *  │  LCD 13 (D6)  ── GPIO 18                │
 *  │  LCD 14 (D7)  ── GPIO 19                │
 *  │  LCD 15 (A)   ── 5V (backlight +)       │
 *  │  LCD 16 (K)   ── GND (backlight -)      │
 *  └──────────────────────────────────────────┘
 *
 * ════════════════ ბიბლიოთეკები ════════════════
 *  Arduino IDE → Library Manager:
 *  • UniversalTelegramBot  (Brian Lough)
 *  • ArduinoJson           (Benoit Blanchon) v6.x
 *  Board: "ESP32 Dev Module" | Flash: 4MB | PSRAM: Disabled
 */

// ============================================================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>
#include <WebServer.h>

// ============================================================
// კონფიგურაცია — შეცვალე შენი მონაცემებით
// ============================================================
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
#define BOTtoken "YOUR_BOT_TOKEN"
#define CHAT_ID  "YOUR_CHAT_ID"

// ============================================================
// პინები
// ============================================================
const int PUMP_RELAY     = 25;   // relay active LOW — NO კონტაქტი
const int VALVE_RELAY    = 26;   // relay active LOW — NO კონტაქტი
const int PIR_SENSOR     = 27;   // HC-SR501 OUT
const int MID_FLOAT      = 32;   // GPIO 32 — INPUT_PULLUP რეჟიმი
const int BTM_FLOAT      = 33;   // GPIO 33 — INPUT_PULLUP რეჟიმი
const int CURRENT_SENSOR = 34;   // ADC1_CH6 — INPUT ONLY, CT სენსორი
const int TOP_FLOAT      = 14;   // GPIO 14 — INPUT_PULLUP რეჟიმი (36-ს pull-up არ აქვს)

// ============================================================
// LCD (RS, E, D4, D5, D6, D7)
// ⚠️ ESP32-WROVER: GPIO16/17 PSRAM-ს იყენებს — სხვა პინები გამოიყენე
// ============================================================
LiquidCrystal lcd(4, 16, 17, 5, 18, 19);
WebServer server(80);

// ============================================================
// სისტემის მდგომარეობა
// ============================================================
bool  pumpState           = false;
bool  valveState          = false;
bool  dryRunAlert         = false;
bool  sensorFaultAlert    = false;
bool  overcurrentAlert    = false;
bool  motionAlert         = false;
bool  manualMode          = false;
int   currentWaterPercent = 0;
float currentAmps         = 0.0;

// float სენსორების ქეში — loop-ში ერთხელ ვკითხულობთ
bool sens_top = false;
bool sens_mid = false;
bool sens_btm = false;

// ============================================================
// ტაიმერები
// ============================================================
unsigned long lastTelegramCheck  = 0;
const unsigned long TELEGRAM_DELAY     = 2000;

unsigned long pumpStartTime      = 0;
const unsigned long DRY_RUN_DELAY      = 8000;   // 8წმ — inrush-ის გამოტოვება
const unsigned long OVERCURRENT_GRACE  = 3000;   // 3წმ — inrush grace period

unsigned long lastLcdUpdate      = 0;
int lcdPage = 0;

unsigned long lastSensorRead     = 0;
const unsigned long SENSOR_INTERVAL   = 500;

unsigned long lastPumpReminder   = 0;
const unsigned long REMINDER_INTERVAL = 600000;  // 10 წუთი

// სტაბილურობის ფილტრები (ანტი-კარუსელი)
unsigned long stableFullTime  = 0;
unsigned long stableEmptyTime = 0;
const unsigned long CONFIRM_DELAY = 3000;  // 3წმ — სენსორის სტაბილურობა

// ============================================================
// Dry Run — ჭის დაცლის დაცვა
// ============================================================
unsigned long dryRunTimeOccurred = 0;
// ⚠️ პროდაქცია: 1800000 (30 წუთი) | ტესტი: 30000 (30 წამი)
const unsigned long WELL_RECOVERY_DELAY = 1800000UL;
int dryRunLowCount = 0;
const int DRY_RUN_CONFIRM = 3;   // 3 × 500ms = 1.5წმ

// ============================================================
// Overcurrent — ძრავის გაჭედვის დაცვა
// ============================================================
int overcurrentHighCount = 0;
const int OVERCURRENT_CONFIRM = 2;  // 2 × 500ms = 1წმ

// ============================================================
// Duty Cycle — ავტო-რეჟიმი: 15წთ ON → 1სთ REST
// ⚠️ ფორსირებული ON ამ ციკლს არ ექვემდებარება
// ============================================================
const unsigned long PUMP_RUN_LIMIT   = 900000UL;  // 15 წუთი
const unsigned long PUMP_REST_PERIOD = 3600000UL; // 1 საათი
bool          pumpResting       = false;
unsigned long pumpRestStartTime = 0;

// ============================================================
// უსაფრთხოების ზღვრები — კალიბრაცია
// ============================================================
const float DRY_RUN_THRESHOLD = 1.5;   // A — ამაზე ნაკლები = მშრალი სვლა
const float MAX_AMPS          = 7.5;   // A — ამაზე მეტი = overcurrent
// WTR_THRESHOLD ამოღებულია — digitalRead + INPUT_PULLUP გამოიყენება

// ============================================================
// Telegram
// ⚠️ sendMessage/getUpdates სინქრონულია (1-5წმ blocking).
//    relay ყოველთვის BOT call-ის წინ გადაირთვება → ფიზიკური
//    უსაფრთხოება შენარჩუნებულია. Async-ისთვის: AsyncTelegram2.
// ============================================================
const String menuKeyboard =
  "[[\"📊 სტატუსი\"],"
  "[\"⚡ ავტო ჩართვა\",\"⚡ ფორსირებული ON\"],"
  "[\"🛑 ტუმბო გამორთვა\",\"🌱 მორწყვა +/-\"],"
  "[\"✅ განგაშის მოხსნა\"]]";

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// ============================================================
// Telegram შეტყობინება — WiFi შემოწმებით
// ============================================================
void tgSend(const String& msg) {
  if (WiFi.status() != WL_CONNECTED) return;
  bot.sendMessageWithReplyKeyboard(CHAT_ID, msg, "", menuKeyboard, true);
}

// ============================================================
// WiFi — setup-ისთვის (blocking — ტუმბო ამ დროს გამორთულია)
// ============================================================
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 20000)
    delay(500);
  return (WiFi.status() == WL_CONNECTED);
}

// ============================================================
// CT დენის სენსორი — peak-to-peak RMS გაანგარიშება
// ============================================================
float readPumpCurrent() {
  unsigned long t = millis();
  int hi = 0, lo = 4095;
  while (millis() - t < 50) {
    int v = analogRead(CURRENT_SENSOR);
    if (v > hi) hi = v;
    if (v < lo) lo = v;
  }
  int pp = hi - lo;
  if (pp < 500) return 0.0;  // ფანტომური დენის ფილტრი (<500 = ხმაური)
  return (((pp * 3.3f) / 4095.0f) / 2.828f * 1.5f) / 0.100f;
}

// ============================================================
// Float სენსორები — ერთხელ ვკითხულობთ, ქეშში ვინახავთ
// ============================================================
void readFloatSensors() {
  // INPUT_PULLUP: switch ჩაკეტილი (წყალი) → LOW = true
  // switch ღია (ჰაერი) → HIGH (pull-up) = false
  sens_top = (digitalRead(TOP_FLOAT) == LOW);
  sens_mid = (digitalRead(MID_FLOAT) == LOW);
  sens_btm = (digitalRead(BTM_FLOAT) == LOW);
}

int getWaterLevelPercent() {
  if (sens_top) return 100;
  if (sens_mid) return 50;
  if (sens_btm) return 15;
  return 0;
}

// ============================================================
// LCD 8×2 — სტატუსის ეკრანი
// ============================================================
void updatePhysicalLCD() {
  lcd.clear();

  // პრიორიტეტი: DryRun > Overcurrent > SensorFault > ნორმალური
  if (dryRunAlert) {
    long s = ((long)WELL_RECOVERY_DELAY - (long)(millis() - dryRunTimeOccurred)) / 1000;
    if (s < 0) s = 0;
    lcd.setCursor(0, 0); lcd.print("DRY RUN ");
    lcd.setCursor(0, 1);
    String t = (s >= 100) ? ("RTRY:" + String(s/60) + "m  ")
                           : ("RTRY:" + String(s) + "s  ");
    lcd.print(t.substring(0, 8));
    return;
  }
  if (overcurrentAlert) {
    lcd.setCursor(0, 0); lcd.print("OVERLOAD");
    lcd.setCursor(0, 1); lcd.print("CHK PUMP");
    return;
  }
  if (sensorFaultAlert) {
    lcd.setCursor(0, 0); lcd.print("!ALERT! ");
    lcd.setCursor(0, 1); lcd.print("SYS JAM ");
    return;
  }

  if (lcdPage == 0) {
    // გვ.1: ტუმბოს მდგომარეობა + დენი
    lcd.setCursor(0, 0);
    lcd.print(pumpState ? (manualMode ? "PMP:MNL " : "PMP:AUTO") : "PMP:OFF ");
    lcd.setCursor(0, 1);
    String a = "A:" + String(currentAmps, 2) + "    ";
    lcd.print(a.substring(0, 8));
  } else {
    // გვ.2: წყლის დონე + სტატუსი / REST countdown
    lcd.setCursor(0, 0);
    String w = "WTR:" + String(currentWaterPercent) + "%   ";
    lcd.print(w.substring(0, 8));
    lcd.setCursor(0, 1);
    if (pumpResting) {
      long rm = ((long)PUMP_REST_PERIOD - (long)(millis() - pumpRestStartTime)) / 60000;
      if (rm < 0) rm = 0;
      String r = "RST:" + String(rm) + "m   ";
      lcd.print(r.substring(0, 8));
    } else {
      lcd.print(motionAlert ? "MOT:ALRT" : "SYS: OK ");
    }
  }
}

// ============================================================
// ტუმბოს გამორთვა
// ============================================================
void stopPump(const String& reason) {
  float lastAmps       = currentAmps;   // დენი relay-ს გამორთვამდე
  digitalWrite(PUMP_RELAY, HIGH);       // ← relay OFF — ყოველთვის პირველი!
  pumpState            = false;
  manualMode           = false;
  dryRunLowCount       = 0;
  overcurrentHighCount = 0;
  stableEmptyTime      = 0;
  stableFullTime       = 0;
  currentAmps          = 0.0;
  tgSend("🛑 ტუმბო გამორთულია.\n"
         "• მიზეზი: " + reason + "\n"
         "• ბოლო დატვირთვა: " + String(lastAmps, 2) + " A");
  updatePhysicalLCD();
}

// ============================================================
// ტუმბოს ჩართვა
// ============================================================
void startPump(bool isManual) {
  // ტუმბო უკვე ჩართულია — მხოლოდ რეჟიმი გადაცვლა, pumpStartTime ხელუხლებელია
  if (pumpState) {
    if (manualMode != isManual) {
      manualMode = isManual;
      if (isManual) pumpResting = false; // ფორსირებული — rest ბლოკი მოიხსნება
      tgSend(isManual ? "⚠️ გადართვა ფორსირებულ რეჟიმში."
                      : "✅ გადართვა ავტო-რეჟიმში.");
      updatePhysicalLCD();
    }
    return;
  }
  if (sensorFaultAlert || overcurrentAlert) {
    tgSend("⚠️ სისტემა დაბლოკილია!\nგანგაში მოხსენი, შემდეგ ჩართე.");
    return;
  }
  if (!isManual && currentWaterPercent >= 100) {
    tgSend("💧 ავზი უკვე სავსეა (100%). ტუმბო არ ჩაირთო.");
    return;
  }
  if (isManual) pumpResting = false; // ფორსირებული ON — duty cycle bypass

  digitalWrite(PUMP_RELAY, LOW);      // ← relay ON
  pumpState            = true;
  manualMode           = isManual;
  dryRunLowCount       = 0;
  overcurrentHighCount = 0;
  pumpStartTime        = millis();
  lastPumpReminder     = millis();
  tgSend(isManual
    ? "⚠️ ტუმბო ჩაირთო ფორსირებულად!\n(duty cycle გვერდის ავლით)"
    : "⚡ ტუმბო ჩაირთო ავტომატურად.");
  updatePhysicalLCD();
}

// ============================================================
// ვებ-გვერდი
// ============================================================
void handleRoot() {
  String html = F("<!DOCTYPE html><html><head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>M48 SMART</title><style>"
    "body{font-family:Arial,sans-serif;text-align:center;background:#f4f4f9;margin:0;padding:20px}"
    ".card{background:#fff;padding:20px;border-radius:15px;"
    "box-shadow:0 4px 8px rgba(0,0,0,.1);max-width:420px;margin:0 auto}"
    "h1{color:#4A90E2;font-size:22px;margin-bottom:10px}"
    ".s{font-size:17px;margin:8px 0;font-weight:bold}"
    ".btn{display:inline-block;padding:11px 20px;font-size:15px;font-weight:bold;"
    "color:#fff;border-radius:25px;text-decoration:none;margin:4px;transition:.2s}"
    ".on{background:#2ECC71}.forced{background:#9B59B6}"
    ".off{background:#E74C3C}.valve{background:#3498DB}"
    ".reset{background:#F1C40F;color:#333}"
    ".al{padding:9px;border-radius:6px;font-weight:bold;margin:5px 0}"
    ".red{background:#ffcccc;color:#c0392b}"
    ".blue{background:#d6eaf8;color:#1a5276}"
    ".yel{background:#fef9e7;color:#7d6608}"
    "</style><script>setTimeout(()=>location.reload(),3000)</script>"
    "</head><body><div class='card'>"
    "<h1>მუხათწყაროს M48 SMART 🏡</h1><hr>");

  if (pumpResting) {
    long rm = ((long)PUMP_REST_PERIOD-(long)(millis()-pumpRestStartTime))/60000;
    if (rm < 0) rm = 0;
    html += "<div class='al blue'>😴 duty cycle — ტუმბო ისვენებს: კიდევ "
            + String(rm) + " წუთი</div>";
  }
  if (dryRunAlert)      html += "<div class='al red'>🚨 მშრალი სვლა ჭაში! მოლოდინი...</div>";
  if (overcurrentAlert) html += "<div class='al red'>🚨 ძრავის გადატვირთვა! შეამოწმე ტუმბო.</div>";
  if (sensorFaultAlert) html += "<div class='al red'>🚨 სენსორის ლოგიკური ხარვეზი!</div>";
  if (motionAlert)      html += "<div class='al yel'>⚠️ მოძრაობა ყუთთან!</div>";

  html += "<div class='s'>ტუმბო: " +
    String(pumpState ? (manualMode ? "⚠️ ფორსირებული ON" : "🔵 ავტო მუშაობს") : "⚪ გამორთულია")
    + "</div>";
  html += "<div class='s'>⚡ დენი: " + String(currentAmps, 2) + " A</div>";
  html += "<div class='s'>💧 ავზი: " + String(currentWaterPercent) + "%</div>";
  html += "<div class='s'>სარქველი: " +
    String(valveState ? "🟢 ღიაა" : "⚪ დაკეტილია") + "</div>";
  html += "<div class='s' style='font-size:13px;color:#888'>📶 "
    + WiFi.localIP().toString() + "</div><hr>";
  html += "<a href='/pump/auto'    class='btn on'>ავტო ON</a>"
          "<a href='/pump/forced'  class='btn forced'>ფორსირებული ON</a>"
          "<a href='/pump/off'     class='btn off'>ტუმბო OFF</a><br>"
          "<a href='/valve/toggle' class='btn valve'>მორწყვა +/-</a>"
          "<a href='/reset'        class='btn reset'>განგაშის მოხსნა</a>"
          "</div></body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void handleWebPumpAuto()    { startPump(false); server.sendHeader("Location","/"); server.send(303); }
void handleWebPumpForced()  { startPump(true);  server.sendHeader("Location","/"); server.send(303); }
void handleWebPumpOff()     { stopPump("ბრაუზერის ბრძანება"); server.sendHeader("Location","/"); server.send(303); }
void handleWebValveToggle() {
  valveState = !valveState;
  digitalWrite(VALVE_RELAY, valveState ? LOW : HIGH);
  server.sendHeader("Location","/"); server.send(303);
}
void handleWebReset() {
  dryRunAlert = false; sensorFaultAlert = false;
  overcurrentAlert = false; motionAlert = false;
  dryRunLowCount = 0; overcurrentHighCount = 0;
  server.sendHeader("Location","/"); server.send(303);
}

// ============================================================
// Telegram ბრძანებები
// ============================================================
void handleNewMessages(int n) {
  for (int i = 0; i < n; i++) {
    if (String(bot.messages[i].chat_id) != CHAT_ID) continue;
    String t = bot.messages[i].text;

    if (t == "/start") {
      String ip = WiFi.localIP().toString();
      tgSend("👋 M48 SMART მზადაა!\n"
             "📶 IP: " + ip + "\n"
             "🔗 http://" + ip + "\n\n"
             "ქვემოთ მენიუდან ბრძანება აირჩიე:");
    }

    if (t == "📊 სტატუსი" || t == "/status") {
      String ip = WiFi.localIP().toString();
      String s =
        "📊 M48 სტატუსი:\n\n"
        "ტუმბო: " + String(pumpState
          ? (manualMode ? "⚠️ ფორსირებული" : "🔵 ავტო")
          : "⚪ გამორთულია") + "\n"
        "⚡ დენი: " + String(currentAmps, 2) + " A\n"
        "💧 ავზი: " + String(currentWaterPercent) + "%\n"
        "სარქველი: " + String(valveState ? "🟢 ღიაა" : "⚪ დაკეტილია") + "\n\n"
        "📶 IP: " + ip + "\n"
        "🔗 http://" + ip;
      if (pumpResting) {
        long rm = ((long)PUMP_REST_PERIOD-(long)(millis()-pumpRestStartTime))/60000;
        if (rm < 0) rm = 0;
        s += "\n\n😴 duty cycle — ისვენებს: კიდევ " + String(rm) + " წთ";
      }
      if (dryRunAlert)      s += "\n\n🚨 მშრალი სვლა ჭაში!";
      if (overcurrentAlert) s += "\n\n🚨 ძრავის გადატვირთვა!";
      if (sensorFaultAlert) s += "\n\n🚨 სენსორის ხარვეზი!";
      if (motionAlert)      s += "\n\n⚠️ მოძრაობა ყუთთან!";
      tgSend(s);
    }

    if (t == "⚡ ავტო ჩართვა")       startPump(false);
    if (t == "⚡ ფორსირებული ON")    startPump(true);
    if (t == "🛑 ტუმბო გამორთვა")   stopPump("მომხმარებლის ბრძანება");
    if (t == "🌱 მორწყვა +/-") {
      valveState = !valveState;
      digitalWrite(VALVE_RELAY, valveState ? LOW : HIGH);
      tgSend(valveState ? "🌱 სარქველი გაიღო." : "🌱 სარქველი დაიკეტა.");
    }
    if (t == "✅ განგაშის მოხსნა") {
      dryRunAlert = false; sensorFaultAlert = false;
      overcurrentAlert = false; motionAlert = false;
      dryRunLowCount = 0; overcurrentHighCount = 0;
      tgSend("✅ ყველა განგაში მოხსნილია.");
    }
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  // relay-ები: HIGH = OFF (active LOW)
  pinMode(PUMP_RELAY,  OUTPUT); digitalWrite(PUMP_RELAY,  HIGH);
  pinMode(VALVE_RELAY, OUTPUT); digitalWrite(VALVE_RELAY, HIGH);
  pinMode(PIR_SENSOR,  INPUT);
  // Float switches: INPUT_PULLUP — გარე წინაღობა საჭირო არ არის
  // switch → GPIO + switch → GND (NO 3.3V, NO რეზისტორი)
  pinMode(TOP_FLOAT, INPUT_PULLUP);  // GPIO 14
  pinMode(MID_FLOAT, INPUT_PULLUP);  // GPIO 32
  pinMode(BTM_FLOAT, INPUT_PULLUP);  // GPIO 33

  lcd.begin(8, 2);
  lcd.clear();
  lcd.print("M48 BOOT");

  client.setInsecure();
  client.setTimeout(5000); // Telegram API — 5წმ timeout, უსასრულო blocking-ის თავიდან არიდება

  lcd.clear(); lcd.print("CONNECT");
  if (!connectWiFi()) {
    Serial.println("[WiFi] ვერ დაუკავშირდა!");
    lcd.clear(); lcd.print("NO WiFi!");
    lcd.setCursor(0, 1); lcd.print("OFFLINE ");
  } else {
    String ip = WiFi.localIP().toString();
    Serial.println("[WiFi] " + ip);
    lcd.clear(); lcd.print("ONLINE! ");
    tgSend("🚀 M48 SMART ონლაინშია!\n📶 IP: " + ip + "\n🔗 http://" + ip);
  }

  server.on("/",             handleRoot);
  server.on("/pump/auto",    handleWebPumpAuto);
  server.on("/pump/forced",  handleWebPumpForced);
  server.on("/pump/off",     handleWebPumpOff);
  server.on("/valve/toggle", handleWebValveToggle);
  server.on("/reset",        handleWebReset);
  server.begin();
  Serial.println("[HTTP] სერვერი ჩაირთო.");

  updatePhysicalLCD();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  server.handleClient();

  // ── WiFi: NON-BLOCKING აღდგენა (Gemini fix) ────────────
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnect = 0;
    if (millis() - lastReconnect > 15000) {
      lastReconnect = millis();
      Serial.println("[WiFi] კავშირი დაიკარგა — ვახლებ ფონში...");
      lcd.clear(); lcd.print("RECONECT");
      WiFi.disconnect();
      WiFi.begin(ssid, password); // მომენტალურად ბრუნდება
    }
  }
  {
    static bool wasDown = false;
    if (WiFi.status() != WL_CONNECTED) {
      wasDown = true;
    } else if (wasDown) {
      wasDown = false;
      String ip = WiFi.localIP().toString();
      Serial.println("[WiFi] აღდგა — " + ip);
      tgSend("📶 WiFi კავშირი აღდგა!\n🌐 IP: " + ip + "\n🔗 http://" + ip);
    }
  }

  // ── PIR მოძრაობის დეტექცია ─────────────────────────────
  if (digitalRead(PIR_SENSOR) == HIGH && !motionAlert) {
    motionAlert = true;
    tgSend("🚨 ⚠️ განგაში! ყუთთან მოძრაობა!");
    Serial.println("[PIR] მოძრაობა!");
  }

  // ── სენსორების წაკითხვა (500ms) ───────────────────────
  if (millis() - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = millis();
    readFloatSensors();                  // TOP/MID/BTM → ქეში
    currentWaterPercent = getWaterLevelPercent();
    currentAmps         = readPumpCurrent();

    // სენსორების ლოგიკური შეუძლებელი მდგომარეობა
    // TOP=1 MID=0 / TOP=1 BTM=0 / MID=1 BTM=0 ► ფიზიკურად შეუძლებელია
    if ((sens_top && !sens_mid) || (sens_top && !sens_btm) || (sens_mid && !sens_btm)) {
      if (!sensorFaultAlert) {
        sensorFaultAlert = true;
        stopPump("სენსორების ლოგიკური გაჭედვა");
      }
    }
  }

  // ── Telegram polling (მაქს. 5 შეტყობინება ციკლში) ─────
  if (WiFi.status() == WL_CONNECTED && millis() - lastTelegramCheck > TELEGRAM_DELAY) {
    int n = bot.getUpdates(bot.last_message_received + 1);
    int cnt = 0;
    while (n && cnt < 5) {
      handleNewMessages(n);
      n = bot.getUpdates(bot.last_message_received + 1);
      cnt++;
    }
    lastTelegramCheck = millis();
  }

  // ── Dry Run-ის ჭის აღდგენის ტაიმერი ───────────────────
  if (dryRunAlert && millis() - dryRunTimeOccurred > WELL_RECOVERY_DELAY) {
    dryRunAlert    = false;
    dryRunLowCount = 0;
    if (currentWaterPercent >= 100) {
      tgSend("🔄 ჭა შეივსო. ავზიც სავსეა — ტუმბო საჭირო არ არის.");
    } else {
      tgSend("🔄 ჭის ტაიმერი ამოიწურა. ტუმბოს ხელახლა ვახლებ...");
      startPump(false);
    }
  }

  // ── 10-წუთიანი პერიოდული შეხსენება ────────────────────
  if (pumpState && millis() - lastPumpReminder > REMINDER_INTERVAL) {
    tgSend("⏰ ტუმბო 10 წუთია ჩართულია. დენი: " + String(currentAmps, 2) + " A");
    lastPumpReminder = millis();
  }

  // ── LCD გვერდების ტრიალი (2 წამი) ─────────────────────
  if (millis() - lastLcdUpdate > 2000) {
    lcdPage ^= 1;
    updatePhysicalLCD();
    lastLcdUpdate = millis();
  }

  // ── Duty Cycle: 15წთ ON → 1სთ REST (ავტო-რეჟიმი მხოლოდ)
  if (pumpState && !manualMode && millis() - pumpStartTime >= PUMP_RUN_LIMIT) {
    pumpResting       = true;
    pumpRestStartTime = millis();
    stopPump("⏱️ 15-წუთიანი ავტო-ლიმიტი. 1სთ დასვენება.");
    tgSend("😴 ტუმბო ისვენებს 1 საათი.\n"
           "ხელით ჩასართავად: ⚡ ფორსირებული ON");
  }
  if (pumpResting && millis() - pumpRestStartTime >= PUMP_REST_PERIOD) {
    pumpResting = false;
    tgSend("✅ 1-საათიანი დასვენება დასრულდა. ავტო-მართვა განახლდა.");
  }

  // ── ავტო-მართვა (ანტი-კარუსელი) ───────────────────────
  if (!manualMode && !sensorFaultAlert && !overcurrentAlert
      && !dryRunAlert && !pumpResting) {

    // ≤50% → ჩართვა (3წმ სტაბილურობის ფილტრი)
    if (currentWaterPercent <= 50 && !pumpState) {
      if (stableEmptyTime == 0) stableEmptyTime = millis();
      if (millis() - stableEmptyTime > CONFIRM_DELAY) {
        tgSend("🚰 ავზი " + String(currentWaterPercent) + "%-ზეა. ვრთავ ტუმბოს...");
        startPump(false);
        stableEmptyTime = 0;
      }
    } else { stableEmptyTime = 0; }

    // 100% → გამორთვა (5წმ გამოტოვება + 3წმ სტაბილურობა)
    if (currentWaterPercent == 100 && pumpState
        && millis() - pumpStartTime > 5000) {
      if (stableFullTime == 0) stableFullTime = millis();
      if (millis() - stableFullTime > CONFIRM_DELAY) {
        stopPump("ავზი შეივსო (100%)");
        stableFullTime = 0;
      }
    } else { stableFullTime = 0; }
  }

  // ── კრიტიკული დაცვები (ორივე რეჟიმში მუშაობს) ────────
  if (pumpState) {

    // Overcurrent — inrush grace period (3წმ) + 2-ჯერადი დასტური
    if (!overcurrentAlert && millis() - pumpStartTime > OVERCURRENT_GRACE) {
      if (currentAmps > MAX_AMPS) {
        overcurrentHighCount++;
        if (overcurrentHighCount >= OVERCURRENT_CONFIRM) {
          overcurrentAlert     = true;
          overcurrentHighCount = 0;
          stopPump("🚨 ძრავის გადატვირთვა / გაჭედვა! (overcurrent)");
        }
      } else {
        overcurrentHighCount = 0;
      }
    }

    // Dry Run — 8წმ დაყოვნება + 3-ჯერადი დასტური
    // re-check pumpState: overcurrent-მა შეიძლება უკვე გამოირთო
    if (pumpState && millis() - pumpStartTime > DRY_RUN_DELAY) {
      if (currentAmps < DRY_RUN_THRESHOLD) {
        dryRunLowCount++;
        if (dryRunLowCount >= DRY_RUN_CONFIRM) {
          dryRunAlert        = true;
          dryRunTimeOccurred = millis();
          dryRunLowCount     = 0;
          stopPump("🚨 მშრალი სვლა ჭაში! (" +
                   String(WELL_RECOVERY_DELAY / 60000UL) + "წთ ტაიმერი)");
        }
      } else {
        dryRunLowCount = 0;
      }
    }
  }
}
