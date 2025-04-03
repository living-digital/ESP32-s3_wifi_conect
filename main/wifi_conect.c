#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "string.h"
#include "esp_system.h"
#include "esp_err.h"

static const char *TAG = "wifi_conect";

#define WIFI_CONNECTED_BIT BIT0
static EventGroupHandle_t wifi_event_group;

// Manejador de eventos WiFi/IP
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG, "Disconnected, reason: %d", event->reason);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Función principal para conectarse a WiFi con o sin IP estática
esp_err_t wifi_conect(const char *ssid, const char *password, const char *ip, const char *gw, const char *netmask)
{
    esp_err_t ret;

    // Inicializar NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Crear interfaz STA
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Configurar IP estática solo si se pasan los parámetros
    if (ip && gw && netmask) {
        ESP_LOGI(TAG, "Usando IP estática: %s", ip);
        esp_netif_ip_info_t ip_info;
        ESP_ERROR_CHECK(esp_netif_str_to_ip4(ip, &ip_info.ip));
        ESP_ERROR_CHECK(esp_netif_str_to_ip4(gw, &ip_info.gw));
        ESP_ERROR_CHECK(esp_netif_str_to_ip4(netmask, &ip_info.netmask));
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta_netif));
        ESP_ERROR_CHECK(esp_netif_set_ip_info(sta_netif, &ip_info));
    } else {
        ESP_LOGI(TAG, "Usando DHCP");
    }

    // Inicializar WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Registrar eventos
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Configuración WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Esperar conexión
    ESP_LOGI(TAG, "Connecting to WiFi...");
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                                           pdFALSE, pdTRUE, pdMS_TO_TICKS(100000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to SSID: %s", ssid);

        // Obtener IP final
        esp_netif_ip_info_t final_ip_info;
        esp_netif_get_ip_info(sta_netif, &final_ip_info);

        char ip_str[16], gw_str[16], netmask_str[16];
        esp_ip4addr_ntoa(&final_ip_info.ip, ip_str, sizeof(ip_str));
        esp_ip4addr_ntoa(&final_ip_info.gw, gw_str, sizeof(gw_str));
        esp_ip4addr_ntoa(&final_ip_info.netmask, netmask_str, sizeof(netmask_str));

        if (ip && gw && netmask) {
            ESP_LOGI(TAG, "Conexión con IP estática:");
        } else {
            ESP_LOGI(TAG, "Conexión con DHCP:");
        }

        ESP_LOGI(TAG, "  IP      : %s", ip_str);
        ESP_LOGI(TAG, "  Gateway : %s", gw_str);
        ESP_LOGI(TAG, "  Máscara : %s", netmask_str);

        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to connect.");
        return ESP_FAIL;
    }
}
