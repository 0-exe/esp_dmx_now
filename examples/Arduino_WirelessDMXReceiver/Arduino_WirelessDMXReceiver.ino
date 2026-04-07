/*

  Wireless DMX Receiver (ESP-NOW -> DMX OUT)

  Receives chunked DMX frames over ESP-NOW, rebuilds full DMX packets, verifies
  CRC, and transmits DMX on UART2 through an RS-485 transceiver.

  Created 7 April 2026

*/
#include <Arduino.h>
#include <WiFi.h>
#include <esp_dmx.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ---------------- DMX wiring ----------------
constexpr dmx_port_t DMX_PORT = DMX_NUM_1;
constexpr int DMX_TX_PIN = 17;  // TX2
constexpr int DMX_RX_PIN = DMX_PIN_NO_CHANGE;
constexpr int DMX_EN_PIN = DMX_PIN_NO_CHANGE;

// ---------------- ESP-NOW config ----------------
constexpr uint8_t ESPNOW_CHANNEL = 1;
constexpr uint8_t FRAME_MAGIC_0 = 0xD5;
constexpr uint8_t FRAME_MAGIC_1 = 0x4D;
constexpr size_t CHUNK_HEADER_SIZE = 14;
constexpr uint16_t FRAME_TIMEOUT_MS = 100;
constexpr uint32_t BLACKOUT_TIMEOUT_MS = 1500;

portMUX_TYPE frameMux = portMUX_INITIALIZER_UNLOCKED;

uint8_t assembledData[DMX_PACKET_SIZE] = {0};
bool assembledMask[DMX_PACKET_SIZE] = {false};
uint16_t assembledFrameId = 0;
uint16_t assembledSize = 0;
uint32_t assembledCrc = 0;
uint16_t assembledCount = 0;
uint32_t lastChunkAtMs = 0;

uint8_t readyData[DMX_PACKET_SIZE] = {0};
uint16_t readySize = 0;
volatile bool frameReady = false;
uint32_t lastFrameOutMs = 0;

uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  while (length--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      const bool lsb = crc & 1U;
      crc >>= 1;
      if (lsb) {
        crc ^= 0xEDB88320;
      }
    }
  }
  return ~crc;
}

void resetAssembly() {
  memset(assembledMask, 0, sizeof(assembledMask));
  assembledFrameId = 0;
  assembledSize = 0;
  assembledCrc = 0;
  assembledCount = 0;
  lastChunkAtMs = 0;
}

void onDataRecv(const esp_now_recv_info_t *, const uint8_t *data, int len) {
  if (len < static_cast<int>(CHUNK_HEADER_SIZE)) return;
  if (data[0] != FRAME_MAGIC_0 || data[1] != FRAME_MAGIC_1) return;

  const uint16_t frameId = static_cast<uint16_t>(data[2] | (data[3] << 8));
  const uint16_t frameSize = static_cast<uint16_t>(data[4] | (data[5] << 8));
  const uint16_t offset = static_cast<uint16_t>(data[6] | (data[7] << 8));
  const uint8_t chunkLen = data[8];
  const uint32_t frameCrc = static_cast<uint32_t>(data[10]) |
                            (static_cast<uint32_t>(data[11]) << 8) |
                            (static_cast<uint32_t>(data[12]) << 16) |
                            (static_cast<uint32_t>(data[13]) << 24);

  if (frameSize == 0 || frameSize > DMX_PACKET_SIZE) return;
  if (len < static_cast<int>(CHUNK_HEADER_SIZE + chunkLen)) return;
  if (offset + chunkLen > frameSize) return;

  portENTER_CRITICAL(&frameMux);

  const uint32_t now = millis();
  const bool stale = (lastChunkAtMs != 0) && (now - lastChunkAtMs > FRAME_TIMEOUT_MS);
  if (stale || assembledFrameId != frameId || assembledSize != frameSize ||
      (assembledFrameId != 0 && assembledCrc != frameCrc)) {
    memset(assembledMask, 0, sizeof(assembledMask));
    assembledFrameId = frameId;
    assembledSize = frameSize;
    assembledCrc = frameCrc;
    assembledCount = 0;
  }
  lastChunkAtMs = now;

  const uint8_t *payload = data + CHUNK_HEADER_SIZE;
  for (uint16_t i = 0; i < chunkLen; ++i) {
    const uint16_t idx = offset + i;
    if (!assembledMask[idx]) {
      assembledMask[idx] = true;
      assembledCount++;
    }
    assembledData[idx] = payload[i];
  }

  if (assembledCount == assembledSize && !frameReady) {
    if (crc32(assembledData, assembledSize) == assembledCrc) {
      memcpy(readyData, assembledData, assembledSize);
      readySize = assembledSize;
      frameReady = true;
    }
    memset(assembledMask, 0, sizeof(assembledMask));
    assembledFrameId = 0;
    assembledSize = 0;
    assembledCrc = 0;
    assembledCount = 0;
    lastChunkAtMs = 0;
  }

  portEXIT_CRITICAL(&frameMux);
}

bool initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onDataRecv);
  return true;
}

void setup() {
  Serial.begin(115200);

  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = {};
  if (!dmx_driver_install(DMX_PORT, &config, personalities, 0)) {
    Serial.println("DMX install failed.");
    while (true) delay(1000);
  }
  if (!dmx_set_pin(DMX_PORT, DMX_TX_PIN, DMX_RX_PIN, DMX_EN_PIN)) {
    Serial.println("DMX pin setup failed.");
    while (true) delay(1000);
  }

  if (!initEspNow()) {
    Serial.println("ESP-NOW init failed.");
    while (true) delay(1000);
  }

  resetAssembly();
  Serial.println("Wireless DMX receiver ready.");
}

void loop() {
  bool hasFrame = false;
  uint16_t frameSize = 0;
  uint8_t frame[DMX_PACKET_SIZE];

  portENTER_CRITICAL(&frameMux);
  if (frameReady) {
    memcpy(frame, readyData, readySize);
    frameSize = readySize;
    frameReady = false;
    hasFrame = true;
  }
  portEXIT_CRITICAL(&frameMux);

  if (hasFrame && frameSize > 0) {
    dmx_write(DMX_PORT, frame, frameSize);
    dmx_send_num(DMX_PORT, frameSize);
    dmx_wait_sent(DMX_PORT, DMX_TIMEOUT_TICK);
    lastFrameOutMs = millis();
  }

  const uint32_t now = millis();
  if (lastFrameOutMs != 0 && (now - lastFrameOutMs) > BLACKOUT_TIMEOUT_MS) {
    uint8_t blackout[DMX_PACKET_SIZE] = {0};
    dmx_write(DMX_PORT, blackout, DMX_PACKET_SIZE);
    dmx_send_num(DMX_PORT, DMX_PACKET_SIZE);
    dmx_wait_sent(DMX_PORT, DMX_TIMEOUT_TICK);
    lastFrameOutMs = now;
  }
}
