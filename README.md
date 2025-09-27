# OpenTherm Optimizer

This project is designed to manipulate OpenTherm communication between a thermostat and boiler. By acting as a transparent proxy, the ESP8266 intercepts OpenTherm messages, modifies selected parameters (such as setpoint temperatures or operational modes), and forwards the adjusted data to the boiler. This allows for optimization, without requiring changes to the original thermostat or boiler.

The device automatically integrates with Home Assistant via MQTT Discovery, enabling seamless setup and monitoring with minimal configuration.

## 🔩 Hardware

This project is built for use with the **Wemos D1 Mini** and compatible OpenTherm shields such as developed by [TheHogNL](https://www.tindie.com/stores/thehognl/): [OpenTherm Slave Shield](https://www.tindie.com/products/thehognl/opentherm-slave-shield-for-wemoslolin/) and [OpenTherm Master Shield](https://www.tindie.com/products/thehognl/opentherm-master-shield-for-wemoslolin/).

These shields handle the electrical interface required for OpenTherm communication and are designed to stack directly onto the Wemos D1 Mini.

While the project is optimized for the hardware setup described above, other ESP8266 boards or OpenTherm interfaces may be compatible. However, using alternative hardware will likely require adjustments to pin assignments or configuration, which are beyond the scope of this README.

## 🚀 Getting Started

To build and upload this project, you'll need:

- [PlatformIO IDE](https://platformio.org/install) (VS Code extension recommended)
- A compatible ESP8266 board (e.g., Wemos D1 Mini)

## 📁 Project Structure

- `src/` — main application code (`main.cpp`)
- `include/` — Header files
- `platformio.ini` — Project configuration file

## ⚙️ Configuration

Before building, copy the example config file and customize it:

```bash
cp include/config.h.example include/config.h
```

Edit `include/config.h` to define your project-specific settings, such as:
- Device-specific parameters
- WiFi credentials
- MQTT server credentials

This file is excluded from version control to protect sensitive data.

## ⚙️ Build Instructions

1. **Install PlatformIO**: Follow instructions at [platformio.org/install](https://platformio.org/install)
2. **Open Project**: Use PlatformIO IDE or CLI to open this folder.
3. **Select Board**: Ensure `platformio.ini` specifies the correct board (e.g., `board = nodemcuv2`)
4. **Build & Upload**:
   - In IDE: Click “Build” and “Upload”
   - CLI: Run `pio run --target upload`

## 🔍 Monitor Serial Output

Use the built-in serial monitor:
- IDE: Click “Monitor”
- CLI: Run `pio device monitor`

## 🏠 Home Assistant Integration

This project supports automatic device registration in Home Assistant via MQTT Discovery.

### Requirements

To enable seamless integration, ensure the following:

- Home Assistant is running and accessible on your network
- MQTT integration is enabled in Home Assistant
- An MQTT broker (e.g., Mosquitto) is installed and configured
- Your ESP8266 device is connected to WiFi and has valid MQTT credentials defined in `include/config.h`

### How It Works

Once configured, the device will publish its capabilities and sensor data using the MQTT Discovery protocol. Home Assistant will automatically detect and add the device without manual configuration.

For more details, see [Home Assistant MQTT Discovery Docs](https://www.home-assistant.io/docs/mqtt/discovery/).

## 🛠️ OpenTherm message tampering / optimization

The logic for intercepting and modifying OpenTherm messages is implemented in `src/main.cpp`, specifically within the `tamperWithRequest()` function.

This function analyzes incoming OpenTherm requests and selectively alters them before forwarding to the boiler. For example, when the thermostat sends a control setpoint (`TSet`) above 30 °C, the function applies a custom transformation to reduce the requested temperature. This allows for dynamic control strategies such as limiting boiler demand or applying external overrides.

```cpp
unsigned long tamperWithRequest(unsigned long request) {
  OpenThermMessageID messageId = sOT.getDataID(request);
  unsigned long tamperedRequest = request;

  switch (messageId) {
    case OpenThermMessageID::TSet: {
      float tSet = sOT.getFloat(request);
      if (tSet > 30) {
        otState.tSetTampered = ((tSet - 30) / 2) + 30;
      } else {
        otState.tSetTampered = tSet;
      }
      tamperedRequest = sOT.buildSetBoilerTemperatureRequest(otState.tSetTampered);
      break;
    }
    default: {}
  }

  return tamperedRequest;
}
```

## 📚 Resources

- [PlatformIO ESP8266 Docs](https://docs.platformio.org/en/latest/boards/espressif8266/index.html)
- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino)
- [PlatformIO CLI Guide](https://docs.platformio.org/en/latest/core/index.html)

---

Happy hacking! 🛠️
