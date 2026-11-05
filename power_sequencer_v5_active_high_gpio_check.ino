/*
 * DIY POWER SEQUENCER 8 RELAY - ESP32
 * VERSION 5 - ACTIVE HIGH + GPIO STATE CHECK
 *
 * Logika output sesuai permintaan:
 *   GPIO HIGH = relay ON
 *   GPIO LOW  = relay OFF
 *
 * Startup:
 *   - Semua output diinisialisasi LOW (OFF).
 *   - Tidak menjalankan sequence otomatis.
 *   - Menunggu tombol manual atau MASTER.
 *
 * MASTER (tahan 3 detik):
 *   - Jika semua GPIO relay LOW/OFF: sequence ON relay 1 -> 8.
 *   - Jika ada GPIO relay HIGH/ON: sequence OFF relay 1 -> 8.
 *
 * Catatan:
 *   digitalRead() pada pin OUTPUT membaca level GPIO yang diperintahkan
 *   ESP32. Ini bukan sensor/feedback untuk memastikan kontak relay fisik
 *   benar-benar berpindah.
 */

// ===================== PENGATURAN =====================
const unsigned long STEP_DELAY_MS = 3500;
const unsigned long LONG_PRESS_MS = 3000;
const unsigned long DEBOUNCE_MS = 30;

const uint8_t NUM_RELAY = 8;
const uint8_t RELAY_PINS[NUM_RELAY] = {16, 17, 18, 19, 21, 22, 23, 25};
const uint8_t BUTTON_PINS[NUM_RELAY] = {13, 14, 26, 27, 32, 33, 4, 5};
const uint8_t MASTER_BTN_PIN = 15;
// ======================================================

enum Mode { SEQ_ON, SEQ_OFF, IDLE };
Mode mode = IDLE;

uint8_t seqIndex = 0;
unsigned long nextStepAt = 0;

const uint8_t NUM_BTN = NUM_RELAY + 1;
bool lastReading[NUM_BTN] = {false};
bool stablePressed[NUM_BTN] = {false};
unsigned long lastChangeAt[NUM_BTN] = {0};

unsigned long masterHeldSince = 0;
bool masterFired = false;

// HIGH = ON, LOW = OFF
bool readRelayState(uint8_t i) {
  if (i >= NUM_RELAY) return false;
  return digitalRead(RELAY_PINS[i]) == HIGH;
}

void printRelayStatus(uint8_t i, bool on) {
  const uint8_t pinLevel = on ? HIGH : LOW;
  Serial.printf(
    "Relay %u | Status GPIO: %s | GPIO %u: %s\n",
    (unsigned int)(i + 1),
    on ? "ON" : "OFF",
    (unsigned int)RELAY_PINS[i],
    pinLevel == HIGH ? "HIGH" : "LOW"
  );
}

void setRelay(uint8_t i, bool on) {
  if (i >= NUM_RELAY) return;

  // ACTIVE HIGH: ON = HIGH, OFF = LOW
  digitalWrite(RELAY_PINS[i], on ? HIGH : LOW);
  printRelayStatus(i, readRelayState(i));
}

void startSequence(Mode newMode) {
  mode = newMode;
  seqIndex = 0;
  nextStepAt = millis();

  Serial.println();
  Serial.println(newMode == SEQ_ON
    ? "=== MASTER: SEMUA RELAY MENUJU ON (GPIO HIGH), URUT 1 -> 8 ==="
    : "=== MASTER: SEMUA RELAY MENUJU OFF (GPIO LOW), URUT 1 -> 8 ===");
}

void runSequence() {
  if ((long)(millis() - nextStepAt) < 0) return;

  const bool targetState = (mode == SEQ_ON);

  // Baca level GPIO aktual sebelum memutuskan relay perlu diubah atau dilewati.
  while (seqIndex < NUM_RELAY &&
         readRelayState(seqIndex) == targetState) {
    seqIndex++;
  }

  if (seqIndex >= NUM_RELAY) {
    mode = IDLE;
    Serial.println("=== SEQUENCE SELESAI ===\n");
    return;
  }

  setRelay(seqIndex, targetState);
  seqIndex++;
  nextStepAt = millis() + STEP_DELAY_MS;

  while (seqIndex < NUM_RELAY &&
         readRelayState(seqIndex) == targetState) {
    seqIndex++;
  }

  if (seqIndex >= NUM_RELAY) {
    mode = IDLE;
    Serial.println("=== SEQUENCE SELESAI ===\n");
  }
}

// Return: 1 = baru ditekan, -1 = baru dilepas, 0 = tidak ada perubahan.
int updateButton(uint8_t idx, uint8_t pin) {
  const bool reading = (digitalRead(pin) == LOW); // INPUT_PULLUP: LOW = ditekan
  const unsigned long now = millis();

  if (reading != lastReading[idx]) {
    lastReading[idx] = reading;
    lastChangeAt[idx] = now;
  }

  if ((now - lastChangeAt[idx]) >= DEBOUNCE_MS &&
      reading != stablePressed[idx]) {
    stablePressed[idx] = reading;
    return reading ? 1 : -1;
  }

  return 0;
}

void handleMasterLongPress() {
  if (mode == SEQ_ON) {
    Serial.println("MASTER: sequence dibalik menjadi OFF.");
    startSequence(SEQ_OFF);
    return;
  }

  if (mode == SEQ_OFF) {
    Serial.println("MASTER: sequence dibalik menjadi ON.");
    startSequence(SEQ_ON);
    return;
  }

  // Periksa seluruh level GPIO relay terlebih dahulu.
  bool allOff = true;
  bool allOn = true;

  Serial.println("MASTER: memeriksa status GPIO seluruh relay...");
  for (uint8_t i = 0; i < NUM_RELAY; i++) {
    const bool isOn = readRelayState(i);
    if (isOn) allOff = false;
    else      allOn = false;

    Serial.printf("  Relay %u / GPIO %u = %s\n",
                  (unsigned int)(i + 1),
                  (unsigned int)RELAY_PINS[i],
                  isOn ? "ON (HIGH)" : "OFF (LOW)");
  }

  if (allOff) {
    Serial.println("Semua relay OFF. MASTER memilih sequence ON.");
    startSequence(SEQ_ON);
  } else if (allOn) {
    Serial.println("Semua relay ON. MASTER memilih sequence OFF.");
    startSequence(SEQ_OFF);
  } else {
    Serial.println("Kondisi relay campuran. MASTER memilih sequence OFF.");
    startSequence(SEQ_OFF);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("====================================");
  Serial.println("       POWER SEQUENCER ESP32");
  Serial.println("       VERSION 5 - ACTIVE HIGH");
  Serial.println("====================================");
  Serial.println("Logika output: ON = HIGH | OFF = LOW");

  // Siapkan LOW sebelum pin dijadikan OUTPUT.
  for (uint8_t i = 0; i < NUM_RELAY; i++) {
    digitalWrite(RELAY_PINS[i], LOW);
    pinMode(RELAY_PINS[i], OUTPUT);
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  pinMode(MASTER_BTN_PIN, INPUT_PULLUP);

  for (uint8_t i = 0; i < NUM_BTN; i++) {
    const uint8_t pin = (i < NUM_RELAY) ? BUTTON_PINS[i] : MASTER_BTN_PIN;
    const bool pressed = (digitalRead(pin) == LOW);
    lastReading[i] = pressed;
    stablePressed[i] = pressed;
    lastChangeAt[i] = millis();
  }

  Serial.println("STARTUP: semua GPIO relay diset LOW (OFF).");
  Serial.println("MODE IDLE: tidak ada sequence otomatis.");
  Serial.println("Tombol 1-8: toggle relay masing-masing.");
  Serial.println("MASTER: tahan 3 detik untuk sequence.");
  Serial.println("Status yang dibaca adalah level GPIO, bukan feedback kontak relay.");
  Serial.println();
}

void loop() {
  const unsigned long now = millis();

  if (mode == SEQ_ON || mode == SEQ_OFF) {
    runSequence();
  }

  // Tombol individual hanya aktif saat tidak ada sequence master.
  for (uint8_t i = 0; i < NUM_RELAY; i++) {
    const int event = updateButton(i, BUTTON_PINS[i]);
    if (event == 1 && mode == IDLE) {
      const bool currentState = readRelayState(i);
      setRelay(i, !currentState);
    }
  }

  // MASTER: tahan selama 3 detik.
  const int masterEvent = updateButton(NUM_RELAY, MASTER_BTN_PIN);
  if (masterEvent == 1) {
    masterHeldSince = now;
    masterFired = false;
  }

  if (stablePressed[NUM_RELAY] && !masterFired &&
      (now - masterHeldSince >= LONG_PRESS_MS)) {
    masterFired = true;
    handleMasterLongPress();
  }
}
