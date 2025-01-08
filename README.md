# P1 Wi-Fi Gateway

Firmware for the P1 Reader module, designed to capture and transmit counting information via Wi-Fi. This project facilitates the integration of P1 data into home automation systems such as Home Assistant and Domoticz.

Don't forget to check out the very comprehensive Wiki : [Le Wiki](https://github.com/narfight/P1-wifi-gateway/wiki)

## Table of contents
- [Features](#Features)
  - [Catpures d'écran](#catpures-d%C3%A9cran)
- [Purchase the module](#Purchase-the-module)
- [Prerequisites](#Prerequisites)
- [Installation](#installation)
- [Module Configuration](#Module-Configuration)
- [Use](#Use)
- [Roadmap](#roadmap)
- [Contribution](#contribution)
- [Related](#related)
- [License](#license)

## Features

- **MQTT transmission** : Sends P1 data to home automation servers via MQTT, compatible with platforms like Home Assistant and Domoticz.
- **Configuration via Wi-Fi** : Create a Wi-Fi hotspot (SSID : `P1_setup_XXXX`) to configure the module, accessible at the address [http://192.168.4.1](http://192.168.4.1).
- **Multilingual support** : Multilingual interface with the ability to add languages.
- **Firmware update** : Firmware update possible via web interface.
- **International Compatibility** : Initially designed for Belgium, but expandable for other countries on request
- **Integration** : Compatible with Home Assistant and Domoticz
### Screenshots
<img src="https://github.com/user-attachments/assets/ae05256c-f895-4f6a-bbab-7d369eba7c81" width="400"/>
<img src="https://github.com/user-attachments/assets/3048b403-0873-40e3-9426-1f866c38b29c" width="50%"/>
<img src="https://github.com/user-attachments/assets/bbeff2c2-e0a4-48f6-986a-942417444dd0" width="400"/>
<img src="https://github.com/user-attachments/assets/0c39660f-1bcf-4faf-8f9e-087691f3a860" width="50%"/>

## Purchase the module

The module is developed by Ronald Leenes (ronaldleenes@icloud.com) and you can order the module for €22 all costs included (as of 2024) via his email.
For more details on the project source : http://www.esp8266thingies.nl/

## Prerequisites

To compile and deploy this project you will need the following :
- [Visual Studio Code](https://code.visualstudio.com/) with the PlatformIO extension.
- Python 3.x (for advanced build scripts).
- A connection to the P1 port of your meter (often for smart meters).
- [Home Assistant](https://www.home-assistant.io/) on [Domoticz](https://www.domoticz.com/) for home automation integration.

## Installation

1. **Firmware download** : Download the latest firmware version from the builds section.
2. **Module update** : Use the module's web interface to download and flash the firmware.
3. **Initial connection** : The module creates a Wi-Fi network `P1_setup_XXXX` Connect to this network.
4. **Access to the interface** : Access the configuration interface via [http://192.168.4.1](http://192.168.4.1).

## Module Configuration

1. **Authentication** : When logging in for the first time, a login and password are requested. If the fields are left empty, the module will not have password protection.
2. **Network Settings** : Configure the Wi-Fi network information to allow the module to connect to the Internet.
3. **MQTT settings** :
   - **MQTT Server** : Enter the address of your MQTT server.
   - **Port** : By default, the port is 1883.
   - **Identifiers** : Fill in the credentials if your MQTT server is protected.
4. **Integration into Home Assistant or Domoticz** : Use the MQTT configuration file (`mqtt-P1Meter.yaml`) to easily configure Home Assistant.

## Use

Once configured, the module will start sending counting data via MQTT. This data may include:
- **Real-time power consumption**.
- **Consumption history** : The module can record consumption data for analysis.
- **Custom alerts** : Set up alerts in Home Assistant or Domoticz to monitor consumption thresholds.

### Monitoring and Diagnostics

The module offers diagnostic tools accessible via the web interface, where you can consult:
- **Event log** : Monitoring connections and errors.
- **Network information** : Check Wi-Fi signal strength and connection status.
- **MQTT Logs** : View messages sent and received via MQTT.

## Roadmap

- **Extensive multilingual support** : One firmware per language
- **International Compatibility** : Adaptation for wider compatibility outside Belgium.

## Contribution

Contributions are welcome! Please follow the steps below:
1. Fork the project and clone it locally.
2. Create a branch for your feature (`git checkout -b new-feature`).
3. Make your changes and test them.
4. Submit a Pull Request for review.

## Related

For more information about the original hardware and software project: [romix123 on GitHub](https://github.com/romix123/P1-wifi-gateway)

## License

This project is distributed under the GNU General Public License. See [GNU General Public License](http://www.gnu.org/licenses/) for details.
