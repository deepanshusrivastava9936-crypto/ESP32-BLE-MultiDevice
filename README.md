This project demonstrates a Bluetooth Low Energy (BLE) server running on an ESP32. The server can handle multiple device connections simultaneously, and allows clients (like the nRF Connect app) to read from and write to the ESP32.

It includes functionality for controlling virtual devices:
LIGHT ON / OFF
FAN ON / OFF
Every client’s interaction is logged with the device number, so you can track which device sent data or disconnected.

Features
Supports multiple BLE clients
Allows reconnects and continuous advertising
Logs device-specific interactions
Easy to extend for IoT applications like smart home devices

Hardware Requirements
ESP32 development board
USB cable for programming
nRF Connect (or any BLE client)

Software Requirements
ESP-IDF v5.5.1
NimBLE stack (built into ESP-IDF)
FreeRTOS (comes with ESP-IDF)
