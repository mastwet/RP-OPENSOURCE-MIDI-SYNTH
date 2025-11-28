// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include "bsp/board_api.h"
// #include "tusb.h"
// #include "pico/stdlib.h"

// enum BlinkInterval {
//     BLINK_NOT_MOUNTED = 250,
//     BLINK_MOUNTED = 1000,
//     BLINK_SUSPENDED = 2500
// };

// class MidiController {
// public:
//     MidiController() {
//         instance = this;
//         stdio_init_all();
        
//         tusb_rhport_init_t dev_init{};
//         dev_init.role = TUSB_ROLE_DEVICE;
//         dev_init.speed = TUSB_SPEED_AUTO;
//         tusb_init(BOARD_TUD_RHPORT, &dev_init);
//     }

//     void run() {
//         while (true) {
//             tud_task();
//             midi_task();
//             led_blinking_task();
//         }
//     }

// private:
//     static MidiController* instance;
//     uint32_t note_pos = 0;
//     static constexpr uint8_t note_sequence[] = {
//         74,78,81,86,90,93,98,102,57,61,66,69,73,78,81,85,88,92,97,100,97,92,88,85,81,78,
//         74,69,66,62,57,62,66,69,74,78,81,86,90,93,97,102,97,93,90,85,81,78,73,68,64,61,
//         56,61,64,68,74,78,81,86,90,93,98,102
//     };
    
//     uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
//     uint32_t midi_start_ms = 0;
//     uint32_t led_start_ms = 0;
//     bool led_state = false;

//     void midi_task() {
//         const uint8_t cable = 0;
//         const uint8_t channel = 0;
        
//         // Drain MIDI RX queue
//         while (tud_midi_available()) {
//             uint8_t packet[4];
//             tud_midi_packet_read(packet);
//         }

//         // Send notes every 286ms
//         if (board_millis() - midi_start_ms < 286) return;
//         midi_start_ms += 286;

//         int previous = note_pos - 1;
//         if (previous < 0) previous = sizeof(note_sequence) - 1;

//         // Note On
//         uint8_t on[3] = {0x90 | channel, note_sequence[note_pos], 127};
//         tud_midi_stream_write(cable, on, 3);

//         // Note Off
//         uint8_t off[3] = {0x80 | channel, note_sequence[previous], 0};
//         tud_midi_stream_write(cable, off, 3);

//         note_pos = (note_pos + 1) % sizeof(note_sequence);
//     }

//     void led_blinking_task() {
//         if (board_millis() - led_start_ms < blink_interval_ms) return;
//         led_start_ms += blink_interval_ms;
        
//         board_led_write(led_state);
//         led_state = !led_state;
//     }

//     // Callback handlers
//     void on_mount()   { blink_interval_ms = BLINK_MOUNTED; }
//     void on_unmount() { blink_interval_ms = BLINK_NOT_MOUNTED; }
//     void on_suspend(bool) { blink_interval_ms = BLINK_SUSPENDED; }
//     void on_resume()  { blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED; }

//     // USB callbacks
//     friend void tud_mount_cb(void);
//     friend void tud_umount_cb(void);
//     friend void tud_suspend_cb(bool);
//     friend void tud_resume_cb(void);
// };

// MidiController* MidiController::instance = nullptr;

// // USB event handlers
// extern "C" {
// void tud_mount_cb(void)    { MidiController::instance->on_mount(); }
// void tud_umount_cb(void)   { MidiController::instance->on_unmount(); }
// void tud_suspend_cb(bool remote_wakeup_en) { MidiController::instance->on_suspend(remote_wakeup_en); }
// void tud_resume_cb(void)   { MidiController::instance->on_resume(); }
// }

// // Test code
// int usbmidi_test() {
//     MidiController controller;
//     controller.run();
//     return 0;
// }