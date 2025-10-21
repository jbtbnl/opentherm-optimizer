// Dependencies through PlatformIO
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <math.h>
#include <OpenTherm.h>

// from ./include
#include <config.h>
#include <ha_mqtt_discovery_payload.h>
#include <SmoothedFloatValue.h>
#include <DampenedBoolValue.h>

const int MQTT_BUFFER_SIZE = 4096; // Should be large enough to accomodate HA MQTT discovery payload

unsigned long mqttLastPublished;

struct OpenThermState {
  DampenedBoolValue chEnable = DampenedBoolValue(
    false, // initial value
    10 // transition delay in seconds
  );
  DampenedBoolValue chMode = DampenedBoolValue(
    false, // initial value
    10 // transition delay in seconds
  );
  DampenedBoolValue flameStatus = DampenedBoolValue(
    false, // initial value
    10 // transition delay in seconds
  );
  float tSet = 0;
  float tRoom = 0;
  float tRoomSet = 0;
  bool chEnableOptimized = false;
  SmoothedFloatValue tSetOptimized = SmoothedFloatValue(
    10, // initial value
    1.0f / 60.0f, // upward rate: 1 degree step per 60 seconds
    1.0f / 20.0f // downward rate: 1 degree step per 20 seconds
  );
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
      otState.chEnable.setTarget((masterStatusFlags & 0x1) != 0);
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

void eavesdropOnResponse(unsigned long response) {
  OpenThermMessageID messageId = sOT.getDataID(response);

  switch (messageId) {
    case OpenThermMessageID::Status: { // ID: 0, Master and Slave Status flags
      uint8_t slaveStatusFlags = sOT.getUInt(response) & 0xFF;
      otState.chMode.setTarget((slaveStatusFlags & 0x2) != 0);
      otState.flameStatus.setTarget((slaveStatusFlags & 0x8) != 0);
      break;
    }
    default: {} // To prevent "enumeration value ... not handled" warnings
  }
}

unsigned long optimizeRequest(unsigned long request) {
  OpenThermMessageType messageType = sOT.getMessageType(request);
  OpenThermMessageID messageId = sOT.getDataID(request);
  unsigned long optimizedRequest = request;

  switch (messageId) {
    case OpenThermMessageID::Status: { // ID: 0, Master and Slave Status flags
      uint16_t statusFlags = sOT.getUInt(request);

      // Disregard chEnable when tSet is less then 20 degrees
      otState.chEnableOptimized = otState.chEnable.get() && otState.tSetOptimized.get() >= 20;
      
      // Overwrite CH enable bit with optimized value
      statusFlags = (statusFlags & ~(1 << 8)) | (otState.chEnableOptimized ? (1 << 8) : 0);

      optimizedRequest = sOT.buildRequest(messageType, messageId, statusFlags);
      break;
    }
    case OpenThermMessageID::TSet: { // ID: 1, Control setpoint ie CH water temperature setpoint (°C)
      float tSet = sOT.getFloat(request);
      if (tSet >= 30) {
        if (otState.tSetOptimized.get() < 25) {
          // make sure that it doesn't take too long
          otState.tSetOptimized.reset(25);
        }
        // from 30 degrees: reduce setpoint (lineair scale down)
        otState.tSetOptimized.setTarget(((tSet - 30) / 2) + 30);
      } else if (tSet >= 25) {
        if (otState.tSetOptimized.get() < 20) {
          // make sure that it doesn't take too long
          otState.tSetOptimized.reset(20);
        }
        // from 25 till 30 degrees: use setpoint as it is
        otState.tSetOptimized.setTarget(tSet);
      } else {
        // below 25 degrees: disregard setpoint (override with 10 degrees)
        otState.tSetOptimized.setTarget(10);
      }
      optimizedRequest = sOT.buildSetBoilerTemperatureRequest(otState.tSetOptimized.get());
      break;
    }
    default: {} // To prevent "enumeration value ... not handled" warnings
  }

  return optimizedRequest;
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
    logFrame("Master:    ", request);
    eavesdropOnRequest(request);

    unsigned long optimizedRequest = optimizeRequest(request);
    if (request != optimizedRequest) {
      logFrame("Optimized: ", optimizedRequest);
    }

    logFrame("Sending:   ", optimizedRequest);
    unsigned long response = mOT.sendRequest(optimizedRequest); // forward optimized request to slave
    if (response) {
      logFrame("Slave:     ", response);
      eavesdropOnResponse(response);

      sOT.sendResponse(response); // send response back to master
    }
  } else {
    logFrame("Invalid:   ", request);
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

      doc["ch_enable"] = otState.chEnable.get() ? "ON" : "OFF";
      doc["ch_mode"] = otState.chMode.get() ? "ON" : "OFF";
      doc["flame_status"] = otState.flameStatus.get() ? "ON" : "OFF";
      doc["tset"] = otState.tSet;
      doc["tr"] = otState.tRoom;
      doc["trset"] = otState.tRoomSet;
      doc["ch_enable_optimized"] = otState.chEnableOptimized ? "ON" : "OFF";
      doc["tset_optimized"] = round(otState.tSetOptimized.get());

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
