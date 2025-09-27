#include <Arduino.h>

#ifndef HA_AUTODISCOVERY_MSG_H
#define HA_AUTODISCOVERY_MSG_H

// https://www.home-assistant.io/integrations/mqtt/#device-discovery-payload
const char* HA_MQTT_DISCOVERY_PAYLOAD = R"(
{
    "dev": {
        "ids": [
            "ea334450945afc"
        ],
        "name": "OpenTherm Optimizer",
        "sw": "1.0.0",
        "sn": "ea334450945afc"
    },
    "o": {
        "name": "",
        "sw": ""
    },
    "cmps": {
        "oo_ch_enable": {
            "p": "binary_sensor",
            "name": "CH enable",
            "device_class": null,
            "value_template": "{{ value_json.ch_enable}}",
            "unique_id": "oo_ch_enable",
            "state_topic": "opentherm_optimizer/state"
        },
        "oo_tset": {
            "p": "sensor",
            "name": "Control setpoint",
            "device_class": "temperature",
            "unit_of_measurement": "°C",
            "value_template": "{{ value_json.tset}}",
            "unique_id": "oo_tset",
            "state_topic": "opentherm_optimizer/state"
        },
        "oo_tr": {
            "p": "sensor",
            "name": "Room temperature",
            "device_class": "temperature",
            "unit_of_measurement": "°C",
            "value_template": "{{ value_json.tr}}",
            "unique_id": "oo_tr",
            "state_topic": "opentherm_optimizer/state"
        },
        "oo_trset": {
            "p": "sensor",
            "name": "Room setpoint",
            "device_class": "temperature",
            "unit_of_measurement": "°C",
            "value_template": "{{ value_json.trset}}",
            "unique_id": "oo_trset",
            "state_topic": "opentherm_optimizer/state"
        },
        "oo_tset_tampered": {
            "p": "sensor",
            "name": "Tampered control setpoint",
            "device_class": "temperature",
            "unit_of_measurement": "°C",
            "value_template": "{{ value_json.tset_tampered}}",
            "unique_id": "oo_tset_tampered",
            "state_topic": "opentherm_optimizer/state"
        }
    },
    "qos": 2
}
)";

#endif
