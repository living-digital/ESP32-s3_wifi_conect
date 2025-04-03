#pragma once

#include "esp_err.h"

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
