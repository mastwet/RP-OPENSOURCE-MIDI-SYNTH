# RAPIDPULZE BLUEBOTTLE

## Project Overview

RAPIDPULZE BLUEBOTTLE is a BLE MIDI project based on ESP32. This project implements MIDI message transmission and reception through Bluetooth Low Energy (BLE) technology. The code uses FreeRTOS as the real-time operating system and leverages the ESP-IDF framework for development.

## Features

- **BLE MIDI Transmission**: Transmit MIDI messages via Bluetooth Low Energy technology.
- **Multi-Port Support**: Support for multiple MIDI ports, currently configured for a single port (`BLEMIDI_NUM_PORTS = 1`).
- **Automatic Timestamp Handling**: Automatically handle MIDI message timestamps to ensure real-time messaging.
- **Output Buffer Management**: Manage the output buffer for MIDI messages to prevent message loss.
- **Callback Mechanism**: Provide a callback function for receiving MIDI messages, facilitating user handling of received MIDI messages.

## Code Structure

### Main Files

- `main.c`: Main program file, containing BLE MIDI initialization and main loop.
- `blemidi.c`: BLE MIDI driver file, containing MIDI message reception and transmission logic.

### Main Functions

- `app_main()`: Main function, initializes the BLE MIDI device and starts the main loop.
- `blemidi_init()`: Initializes the BLE MIDI service.
- `blemidi_send_message()`: Sends MIDI messages.
- `blemidi_receive_packet()`: Receives MIDI data packets and processes them.
- `callback_midi_message_received()`: Callback function for receiving MIDI messages.

### Configuration Parameters

- `BLEMIDI_DEVICE_NAME`: BLE device name, currently set to `"RP Ble midi"`.
- `BLEMIDI_NUM_PORTS`: Number of supported MIDI ports, currently set to `1`.
- `BLEMIDI_OUTBUFFER_FLUSH_MS`: Output buffer flush interval, currently set to `15` milliseconds.
- `BLEMIDI_ENABLE_CONSOLE`: Whether to enable console output, currently set to `1` (enabled).

## Usage Instructions

### Compilation and Flashing

1. Ensure you have the ESP-IDF development environment installed.
2. Clone the project code to your local machine.
3. Run `idf.py build` in the project root directory to compile.
4. Use `idf.py -p <PORT> flash` to flash the firmware to the ESP32 device.

### Running

1. After flashing, the ESP32 device will automatically start and begin broadcasting the BLE MIDI service.
2. Connect to the `RP Ble midi` device using a BLE MIDI-supported device (such as a smartphone or computer).
3. The device will automatically send MIDI messages and output received MIDI messages to the console.

## Dependencies

- `esp-idf`: Official development framework for ESP32.
- `FreeRTOS`: Real-time operating system for task management and scheduling.
- `esp_bt`: Bluetooth controller library for ESP32.
- `esp_gap_ble_api` and `esp_gatts_api`: APIs related to BLE connection and GATT services.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contribution

Feel free to submit issues and improvement suggestions. If you are interested in contributing code, please submit a Pull Request.

## Contact Information

For any questions or suggestions, please contact the project maintainer
