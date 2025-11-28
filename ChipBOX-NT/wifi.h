#ifndef WIFI_H
#define WIFI_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "lwip/err.h"
#include "lwip/sys.h"

esp_ip4_addr_t s_ip_addr;

static QueueHandle_t wifi_event_queue;

typedef struct {
    esp_event_base_t event_base;
    int32_t event_id;
    void *event_data;
} wifi_global_event_t;

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) {
    wifi_global_event_t event;
    event.event_base = event_base;
    event.event_id = event_id;
    event.event_data = event_data;

    if (xQueueSend(wifi_event_queue, &event, portMAX_DELAY) != pdPASS) {
        printf("wifi_station: Failed to send event to queue");
    }
}

void wifi_init_sta(const char *ssid, const char *password) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };
    
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
}

void wifi_disconnect(void) {
    esp_err_t ret = esp_wifi_stop();
    if (ret == ESP_OK) {
        printf("wifi_station: Wi-Fi disconnected successfully\n");
    } else {
        printf("wifi_station: Failed to disconnect Wi-Fi\n");
    }

    ret = esp_wifi_deinit();
    if (ret == ESP_OK) {
        printf("wifi_station: Wi-Fi deinitialized successfully\n");
    } else {
        printf("wifi_station: Failed to deinitialize Wi-Fi\n");
    }
}

#endif