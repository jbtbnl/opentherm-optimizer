// Dependencies through PlatformIO
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <OpenTherm.h>

// from ./include
#include <config.h>
#include <ha_mqtt_discovery_payload.h>

const int MQTT_BUFFER_SIZE = 4096; // Should be large enough to accomodate HA MQTT discovery payload

unsigned long mqttLastPublished;

struct OpenThermState {
  bool chEnable = false;
  float tSet = 0;
  float tRoom = 0;
  float tRoomSet = 0;
  float tSetTampered = 0;
} otState;

WiFiClient wifiClient;

PubSubClient client(wifiClient);

OpenTherm mOT(PIN_OT_MASTER_IN, PIN_OT_MASTER_OUT);
OpenTherm sOT(PIN_OT_SLAVE_IN, PIN_OT_SLAVE_OUT, true);

// Master interrupt routine
void IRAM_ATTR mOTHandleInterrupt() {
  mOT.handleInterrupt();
}

// Slave interrupt routine
void IRAM_ATTR sOTHandleInterrupt() {
  sOT.handleInterrupt();
}

void eavesdropOnRequest(unsigned long request) {
  OpenThermMessageID messageId = sOT.getDataID(request);

  switch (messageId) {
    case OpenThermMessageID::Status: { // ID: 0, Master and Slave Status flags
      uint8_t masterStatusFlags = sOT.getUInt(request) >> 8;
      otState.chEnable = (masterStatusFlags & 0x1) != 0;
      break;
    }
    case OpenThermMessageID::TSet: { // ID: 1, Control setpoint  ie CH  water temperature setpoint (°C)
      otState.tSet = sOT.getFloat(request);
      break;
    }
    case OpenThermMessageID::Tr: { // ID: 24, Room temperature (°C)
      otState.tRoom = sOT.getFloat(request);
      break;
    }
    case OpenThermMessageID::TrSet: { // ID: 16, Room Setpoint (°C)
      otState.tRoomSet = sOT.getFloat(request);
      break;
    }
    default: {} // To prevent "enumeration value ... not handled" warnings
  }
}

unsigned long tamperWithRequest(unsigned long request) {
  OpenThermMessageID messageId = sOT.getDataID(request);
  unsigned long tamperedRequest = request;

  switch (messageId) {
    case OpenThermMessageID::TSet: { // ID: 1, Control setpoint  ie CH  water temperature setpoint (°C)
      float tSet = sOT.getFloat(request);
      if (tSet > 30) {
        otState.tSetTampered = ((tSet - 30) / 2) + 30;
      } else {
        otState.tSetTampered = tSet;
      }
      tamperedRequest = sOT.buildSetBoilerTemperatureRequest(otState.tSetTampered);
      break;
    }
    default: {} // To prevent "enumeration value ... not handled" warnings
  }

  return tamperedRequest;
}

String frameToBinaryString(unsigned long frame) {
  char output[37];
  int outIndex = 0;
  for (int i = 31; i >= 0; --i) {
    output[outIndex++] = (frame & (1UL << i)) ? '1' : '0';

    // Insert space after 1st, 4th, 8th, and 16th bits (from left)
    int bitPosFromLeft = 32 - i;
    if (bitPosFromLeft == 1 || bitPosFromLeft == 4 || bitPosFromLeft == 8 || bitPosFromLeft == 16) {
      output[outIndex++] = ' ';
    }
  }
  output[outIndex] = '\0';

  return String(output);
}

void logFrame(String frameType, unsigned long frame) {
  String msgMaster = frameType + frameToBinaryString(frame);

  Serial.println(msgMaster);
  client.publish(MQTT_LOG_TOPIC, msgMaster.c_str(), false);
}

void processRequest(unsigned long request, OpenThermResponseStatus status) {
  if (sOT.isValidRequest(request)) {
    logFrame("Master:   ", request);
    eavesdropOnRequest(request);

    unsigned long tamperedRequest = tamperWithRequest(request);
    if (request != tamperedRequest) {
      logFrame("Tampered: ", tamperedRequest);
    }

    logFrame("Sending:  ", tamperedRequest);
    unsigned long response = mOT.sendRequest(tamperedRequest); // forward tampered request to slave
    if (response) {
      sOT.sendResponse(response); // send response back to master
      logFrame("Slave:    ", response);
    }
  } else {
    logFrame("Invalid:  ", request);
  }

  Serial.println();
}

void setup() {
  Serial.begin(115200);

  WiFi.persistent(false);               // Avoid flash writes
  WiFi.mode(WIFI_STA);                  // Station mode
  WiFi.setAutoReconnect(true);          // Enable auto-reconnect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Initial connection

  Serial.print("WiFi:     Connecting...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi:     Connected");

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setBufferSize(MQTT_BUFFER_SIZE);

  mOT.begin(mOTHandleInterrupt);
  sOT.begin(sOTHandleInterrupt, processRequest);
}

void loop() {
  while (!client.connected()) {
    Serial.println("MQTT:     Connecting...");

    if (client.connect(MQTT_DEVICE_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("MQTT:     Connected");
      
      client.publish(HA_MQTT_DISCOVERY_TOPIC, HA_MQTT_DISCOVERY_PAYLOAD, true);
      mqttLastPublished = millis();
    } else {
      Serial.printf("MQTT:     Failed with state %i, retrying", client.state());
      delay(2000);
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - mqttLastPublished > 10000) {
      mqttLastPublished = millis();

      JsonDocument doc;
      char buffer[256];

      doc["ch_enable"] = otState.chEnable ? "ON" : "OFF";
      doc["tset"] = otState.tSet;
      doc["tr"] = otState.tRoom;
      doc["trset"] = otState.tRoomSet;
      doc["tset_tampered"] = otState.tSetTampered;

      serializeJson(doc, buffer);

      Serial.print("MQTT:     Sending state... ");
      if (!client.publish(MQTT_STATE_TOPIC, buffer, false)) {
        Serial.println("FAILED");
      } else {
        Serial.println("DONE");
      }
    }

    client.loop();
  }
  else {
    Serial.println("WiFi:     Disconnected");
  }

  sOT.process();
  mOT.process();
}
