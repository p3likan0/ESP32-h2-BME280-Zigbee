# ESP32-H2 BME280 Zigbee Sensor

This project is an implementation of an ESP32-H2 based air sensor that reads data from a BME280 sensor over I2C and publishes the data using Zigbee. The sensor data includes temperature, humidity, and pressure, and it can be easily integrated with HomeAssistant.

## Project Overview

The project is based on the [esp32-h2-air-sensor](https://github.com/acha666/esp32-h2-air-sensor) and has been modified to work with the BME280 sensor and GPIOs 1 and 2 for I2C communication.

## Features

- Reads temperature, humidity, and pressure data from BME280 sensor.
- Publishes sensor data over Zigbee.
- Easy integration with HomeAssistant.

## Hardware Requirements

- ESP32-H2
- BME280 sensor
- Connecting wires

## Wiring Diagram

ESP32-H2 Pin | BME280 Pin
-------------|------------
GPIO 1       | SDA
GPIO 2       | SCL
GND          | GND
3.3V         | VCC

## Contributing

Feel free to submit issues and pull requests. Contributions are welcome!

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
