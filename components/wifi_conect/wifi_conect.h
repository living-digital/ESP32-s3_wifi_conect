#pragma once

#include "esp_err.h"
#include <stdbool.h> 

// Función para conectarse a una red WiFi con IP estática
esp_err_t wifi_conect(
    const char *ssid, 
    const char *password, 
    const char *ip, 
    const char *gw, 
    const char *netmask
);

/**
 * @brief Devuelve la IP actual.
 * 
 * @return const char* IP en formato texto ("192.168.x.x") o NULL si 
 * no hay conexión.
 * El puntero apunta a una cadena estática, no hace falta liberarla.
 */
const char* wifi_conect_get_ip(void);

/**
 * @brief Desconecta el ESP32 de la red WiFi actual y detiene el driver WiFi.
 * @return esp_err_t ESP_OK si fue exitoso, otro código de error si falla.
 */
esp_err_t wifi_conect_disconnect(void);

// Indica si estamos conectados a una red wifi o no
bool wifi_conect_is_connected(void);

