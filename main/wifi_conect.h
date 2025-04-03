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
