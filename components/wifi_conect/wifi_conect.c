#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "wifi_conect.h"


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
esp_err_t wifi_conect(
        const char *ssid, 
        const char *password,
        const char *ip, 
        const char *gw, 
        const char *netmask,
        const char *dns) // ⚠️ Parametro opcional
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

        // ⚠️ Configurar DNS si se proporciona
        if (dns) {
            esp_netif_dns_info_t dns_info;
            dns_info.ip.u_addr.ip4.addr = ipaddr_addr(dns);
            dns_info.ip.type = IPADDR_TYPE_V4;
            ESP_ERROR_CHECK(esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info));
            ESP_LOGI(TAG, "DNS configurado manualmente: %s", dns);
        }
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
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Esperar conexión
    ESP_LOGI(TAG, "Connecting to WiFi...");
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group, 
        WIFI_CONNECTED_BIT,
        pdFALSE, 
        pdTRUE, 
        pdMS_TO_TICKS(100000) // tiempo maximo de espera en milisegundos para conectar
    );

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


// Devuelve la IP actual si estamos conectados
const char* wifi_conect_get_ip(void)
{
    static char ip_str[16]; // espacio para "xxx.xxx.xxx.xxx"

    // Verificar si estamos conectados
    if (!wifi_conect_is_connected()) {
        ESP_LOGW(TAG, "No conectado. No se puede obtener IP.");
        return NULL;
    }

    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        ESP_LOGW(TAG, "No se pudo obtener el handle de la interfaz STA");
        return NULL;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        ESP_LOGW(TAG, "No se pudo obtener la IP");
        return NULL;
    }

    esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
    return ip_str;
}



// Indica si estamos conectados a una red wifi
bool wifi_conect_is_connected(void)
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return true;
    } else {
        return false;
    }
}

// Indica si hay acceso a internet conectando con 8.8.8.8
bool internet_connected_ip(void) {
    int sock;
    struct sockaddr_in server_addr;
    const char *server_ip = "8.8.8.8";  // Puedes usar la IP de Google DNS, por ejemplo.
    int port = 53; // Utilizamos el puerto 53 (DNS).
    struct timeval timeout = {5, 0};  // 5 segundos de timeout.

    // Crear el socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Error al crear el socket");
        return false;
    }

    // Configuramos los tiempos de espera (envío y recepción)
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_aton(server_ip, &server_addr.sin_addr) == 0) {
        ESP_LOGE(TAG, "IP inválida: %s", server_ip);
        close(sock);
        return false;
    }

    // Intentar conectarse al servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Fallo en la conexión a %s en el puerto %d", server_ip, port);
        close(sock);
        return false;
    }

    ESP_LOGI(TAG, "Conexión exitosa a %s en el puerto %d", server_ip, port);
    close(sock);
    return true;
}

// Verifica si hay acceso a internet conectando con google.com
bool internet_connected_dns(void)
{
    const char *host = "google.com";
    const int port = 80;
    struct sockaddr_in dest_addr;
    struct hostent *he;
    int sock;
    struct timeval timeout = {5, 0};  // timeout de 5 segundos

    ESP_LOGI(TAG, "Resolviendo DNS para %s...", host);
    he = gethostbyname(host);
    if (he == NULL || he->h_addr_list[0] == NULL) {
        ESP_LOGW(TAG, "No se pudo resolver el host: %s", host);
        return false;
    }

    // Configurar dirección destino
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    memcpy(&dest_addr.sin_addr, he->h_addr_list[0], he->h_length);

    // Crear socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Error al crear el socket");
        return false;
    }

    // Tiempo de espera
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Intentar conexión
    if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        ESP_LOGW(TAG, "No se pudo conectar con %s:%d", host, port);
        close(sock);
        return false;
    }

    ESP_LOGI(TAG, "Conexión exitosa con %s:%d", host, port);
    close(sock);
    return true;
}


// Desconecta el ESP32 de la red WiFi actual y detiene el driver WiFi.
esp_err_t wifi_conect_disconnect(void)
{
    ESP_LOGI(TAG, "Desconectando de la red WiFi...");

    esp_err_t err;

    // Detener el WiFi (esto implica desconexión)
    err = esp_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al detener WiFi: %s", esp_err_to_name(err));
        return err;
    }

    // Opcional: liberar recursos si no volverás a conectarte en esta sesión
    err = esp_wifi_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al desinicializar WiFi: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "WiFi desconectado correctamente.");
    return ESP_OK;
}
