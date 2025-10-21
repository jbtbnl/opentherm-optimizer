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
        "oo_ch_mode": {
            "p": "binary_sensor",
            "name": "CH mode",
            "device_class": null,
            "value_template": "{{ value_json.ch_mode}}",
            "unique_id": "oo_ch_mode",
            "state_topic": "opentherm_optimizer/state"
        },
        "oo_flame_status": {
            "p": "binary_sensor",
            "name": "Flame status",
            "device_class": null,
            "value_template": "{{ value_json.flame_status}}",
            "unique_id": "oo_flame_status",
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
        "oo_ch_enable_optimized": {
            "p": "binary_sensor",
            "name": "Optimized CH enable",
            "device_class": null,
            "value_template": "{{ value_json.ch_enable_optimized}}",
            "unique_id": "oo_ch_enable_optimized",
            "state_topic": "opentherm_optimizer/state"
        },
        "oo_tset_optimized": {
            "p": "sensor",
            "name": "Optimized control setpoint",
            "device_class": "temperature",
            "unit_of_measurement": "°C",
            "value_template": "{{ value_json.tset_optimized}}",
            "unique_id": "oo_tset_optimized",
            "state_topic": "opentherm_optimizer/state"
        }
    },
    "qos": 2
}
)";

#endif
