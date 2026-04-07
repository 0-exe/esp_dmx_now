/*

  Wireless DMX Sender (DMX IN -> ESP-NOW)

  Reads DMX from RS-485 on UART2 and forwards each DMX frame to a receiver ESP32
  using ESP-NOW. This sketch uses chunking so full 513-byte DMX packets are sent.

  Created 7 April 2026

*/
#include <Arduino.h>
#include <WiFi.h>
#include <esp_dmx.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ---------------- DMX wiring ----------------
constexpr dmx_port_t DMX_PORT = DMX_NUM_1;
constexpr int DMX_TX_PIN = DMX_PIN_NO_CHANGE;
constexpr int DMX_RX_PIN = 16;  // RX2
constexpr int DMX_EN_PIN = DMX_PIN_NO_CHANGE;

// ---------------- ESP-NOW config ----------------
constexpr uint8_t ESPNOW_CHANNEL = 1;
// Set this to your receiver ESP32 MAC address.
uint8_t RECEIVER_MAC[6] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x00};

constexpr uint8_t FRAME_MAGIC_0 = 0xD5;
constexpr uint8_t FRAME_MAGIC_1 = 0x4D;
constexpr uint8_t FRAME_FLAG_LAST = 0x01;
constexpr size_t ESPNOW_MAX_PAYLOAD = 250;
constexpr size_t CHUNK_HEADER_SIZE = 14;
constexpr size_t CHUNK_DATA_MAX = ESPNOW_MAX_PAYLOAD - CHUNK_HEADER_SIZE;

uint8_t dmxData[DMX_PACKET_SIZE] = {0};
uint16_t frameId = 0;
volatile bool sendFailure = false;

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

bool sendFrame(const uint8_t *frame, uint16_t frameSize) {
  if (frameSize == 0 || frameSize > DMX_PACKET_SIZE) return false;

  const uint32_t frameCrc = crc32(frame, frameSize);
  uint8_t packet[ESPNOW_MAX_PAYLOAD];
  uint16_t offset = 0;
  frameId++;

  while (offset < frameSize) {
    const uint16_t remaining = frameSize - offset;
    const uint8_t chunkLen = static_cast<uint8_t>(min<size_t>(CHUNK_DATA_MAX, remaining));
    const bool isLast = (offset + chunkLen) >= frameSize;

    packet[0] = FRAME_MAGIC_0;
    packet[1] = FRAME_MAGIC_1;
    packet[2] = static_cast<uint8_t>(frameId & 0xFF);
    packet[3] = static_cast<uint8_t>((frameId >> 8) & 0xFF);
    packet[4] = static_cast<uint8_t>(frameSize & 0xFF);
    packet[5] = static_cast<uint8_t>((frameSize >> 8) & 0xFF);
    packet[6] = static_cast<uint8_t>(offset & 0xFF);
    packet[7] = static_cast<uint8_t>((offset >> 8) & 0xFF);
    packet[8] = chunkLen;
    packet[9] = isLast ? FRAME_FLAG_LAST : 0;
    packet[10] = static_cast<uint8_t>(frameCrc & 0xFF);
    packet[11] = static_cast<uint8_t>((frameCrc >> 8) & 0xFF);
    packet[12] = static_cast<uint8_t>((frameCrc >> 16) & 0xFF);
    packet[13] = static_cast<uint8_t>((frameCrc >> 24) & 0xFF);
    memcpy(packet + CHUNK_HEADER_SIZE, frame + offset, chunkLen);

    if (esp_now_send(RECEIVER_MAC, packet, CHUNK_HEADER_SIZE + chunkLen) != ESP_OK) {
      return false;
    }
    offset += chunkLen;
    delayMicroseconds(600);
  }
  return true;
}

void onDataSent(const uint8_t *, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    sendFailure = true;
  }
}

bool initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, RECEIVER_MAC, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) return false;
  return true;
}

void setup() {
  Serial.begin(115200);

  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = {
      {1, "Wireless DMX Sender"},
  };
  if (!dmx_driver_install(DMX_PORT, &config, personalities, 1)) {
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

  Serial.println("Wireless DMX sender ready.");
}

void loop() {
  dmx_packet_t packet;
  const size_t size = dmx_receive(DMX_PORT, &packet, DMX_TIMEOUT_TICK);
  if (size > 0 && !packet.err) {
    dmx_read(DMX_PORT, dmxData, size);
    if (!sendFrame(dmxData, static_cast<uint16_t>(size))) {
      Serial.println("Frame send failed.");
    }
  }

  if (sendFailure) {
    sendFailure = false;
    Serial.println("ESP-NOW delivery reported a failed chunk.");
  }
}
