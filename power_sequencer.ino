/*
 * DIY Power Sequencer 8 Relay - ESP32
 *
 * Fitur:
 *  1. Setelah ESP32 menyala, tunggu 5 detik, baru mulai menyalakan relay.
 *  2. Relay menyala berurutan 1 -> 8, jeda 5 detik antar relay.
 *  3. Tombol 1-8 untuk menyalakan/mematikan relay secara manual
 *     (aktif setelah urutan selesai). LED indikator per relay.
 *  4. Tombol master: tahan 3 detik untuk menyalakan/mematikan semua relay.
 *     Mati juga berurutan 1 -> 8 dengan jeda yang sama.
 *     Jika ditekan lagi saat urutan berjalan, arah urutan dibalik.
 *
 * Wiring:
 *  - Relay IN1..IN8   -> GPIO 16, 17, 18, 19, 21, 22, 23, 25
 *  - Tombol 1..8      -> GPIO 13, 14, 26, 27, 32, 33, 4, 5  (sisi lain ke GND)
 *  - Tombol master    -> GPIO 15                            (sisi lain ke GND)
 *  - LED indikator    -> dipasang PARALEL dengan pin IN relay:
 *                        anoda LED -> resistor 330 ohm -> 3.3V
 *                        katoda LED -> pin IN relay yang sama
 *                        (untuk modul relay active-low, LED menyala saat relay ON)
 *
 * Catatan: Tombol memakai INPUT_PULLUP, jadi tidak perlu resistor tambahan.
 */

// ===================== PENGATURAN =====================
const bool RELAY_ACTIVE_LOW = true;          // true = relay menyala saat pin LOW
const unsigned long BOOT_DELAY_MS  = 5000;   // jeda awal setelah ESP32 menyala
const unsigned long STEP_DELAY_MS  = 5000;   // jeda antar relay (nyala maupun mati)
const unsigned long LONG_PRESS_MS  = 3000;   // lama tahan tombol master
const unsigned long DEBOUNCE_MS    = 30;     // debounce tombol

const uint8_t NUM_RELAY = 8;
const uint8_t RELAY_PINS[NUM_RELAY]  = {16, 17, 18, 19, 21, 22, 23, 25};
const uint8_t BUTTON_PINS[NUM_RELAY] = {13, 14, 26, 27, 32, 33, 4, 5};
const uint8_t MASTER_BTN_PIN = 15;
// ======================================================

enum Mode { WAIT_BOOT, SEQ_ON, SEQ_OFF, IDLE };
Mode mode = WAIT_BOOT;

bool relayOn[NUM_RELAY];
uint8_t seqIndex = 0;
unsigned long nextStepAt = 0;

// Status debounce: indeks 0-7 = tombol relay, indeks 8 = tombol master
const uint8_t NUM_BTN = NUM_RELAY + 1;
bool lastReading[NUM_BTN];
bool stablePressed[NUM_BTN];
unsigned long lastChangeAt[NUM_BTN];

unsigned long masterHeldSince = 0;
bool masterFired = false;

void setRelay(uint8_t i, bool on) {
  relayOn[i] = on;
  digitalWrite(RELAY_PINS[i], (on == RELAY_ACTIVE_LOW) ? LOW : HIGH);
  Serial.printf("Relay %d -> %s\n", i + 1, on ? "ON" : "OFF");
}

void startSequence(Mode m) {
  mode = m;
  seqIndex = 0;
  nextStepAt = millis();
  Serial.println(m == SEQ_ON ? "Urutan: MENYALAKAN 1 -> 8" : "Urutan: MEMATIKAN 1 -> 8");
}

void runSequence() {
  if ((long)(millis() - nextStepAt) < 0) return;

  bool target = (mode == SEQ_ON);

  // Lewati relay yang sudah berada di kondisi target
  while (seqIndex < NUM_RELAY && relayOn[seqIndex] == target) seqIndex++;
  if (seqIndex >= NUM_RELAY) {
    mode = IDLE;
    Serial.println("Urutan selesai.");
    return;
  }

  setRelay(seqIndex, target);
  seqIndex++;
  nextStepAt = millis() + STEP_DELAY_MS;

  // Jika tidak ada lagi yang perlu diubah, selesai sekarang (tanpa menunggu jeda terakhir)
  while (seqIndex < NUM_RELAY && relayOn[seqIndex] == target) seqIndex++;
  if (seqIndex >= NUM_RELAY) {
    mode = IDLE;
    Serial.println("Urutan selesai.");
  }
}

// Mengembalikan 1 saat baru ditekan, -1 saat baru dilepas, 0 jika tidak ada perubahan
int updateButton(uint8_t idx, uint8_t pin) {
  bool reading = (digitalRead(pin) == LOW);  // LOW = ditekan
  unsigned long now = millis();

  if (reading != lastReading[idx]) {
    lastReading[idx] = reading;
    lastChangeAt[idx] = now;
  }

  if ((now - lastChangeAt[idx]) >= DEBOUNCE_MS && reading != stablePressed[idx]) {
    stablePressed[idx] = reading;
    return reading ? 1 : -1;
  }
  return 0;
}

void handleMasterLongPress() {
  if (mode == SEQ_ON) {
    startSequence(SEQ_OFF);
  } else if (mode == SEQ_OFF) {
    startSequence(SEQ_ON);
  } else {
    bool anyOn = false;
    for (uint8_t i = 0; i < NUM_RELAY; i++) {
      if (relayOn[i]) anyOn = true;
    }
    startSequence(anyOn ? SEQ_OFF : SEQ_ON);
  }
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < NUM_RELAY; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    setRelay(i, false);  // semua relay mati saat start
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  pinMode(MASTER_BTN_PIN, INPUT_PULLUP);

  for (uint8_t i = 0; i < NUM_BTN; i++) {
    lastReading[i] = false;
    stablePressed[i] = false;
    lastChangeAt[i] = 0;
  }

  Serial.println("Power sequencer siap. Menunggu 5 detik...");
}

void loop() {
  unsigned long now = millis();

  // 1. Jeda awal setelah ESP32 menyala
  if (mode == WAIT_BOOT) {
    if (now >= BOOT_DELAY_MS) startSequence(SEQ_ON);
  }

  // 2. Jalankan urutan menyala / mati
  if (mode == SEQ_ON || mode == SEQ_OFF) runSequence();

  // 3. Tombol manual 1-8 (hanya aktif saat tidak ada urutan berjalan)
  for (uint8_t i = 0; i < NUM_RELAY; i++) {
    int e = updateButton(i, BUTTON_PINS[i]);
    if (e == 1 && mode == IDLE) {
      setRelay(i, !relayOn[i]);
    }
  }

  // 4. Tombol master: tahan 3 detik
  int m = updateButton(NUM_RELAY, MASTER_BTN_PIN);
  if (m == 1) {
    masterHeldSince = now;
    masterFired = false;
  }
  if (stablePressed[NUM_RELAY] && !masterFired && mode != WAIT_BOOT &&
      (now - masterHeldSince) >= LONG_PRESS_MS) {
    masterFired = true;
    handleMasterLongPress();
  }
}
