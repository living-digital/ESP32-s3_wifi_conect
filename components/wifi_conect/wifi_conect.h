#pragma once

#include "esp_err.h"
#include <stdbool.h> 

/**
 * @brief Conecta a una red Wi-Fi utilizando IP estática o DHCP.
 * 
 * @param ssid     Nombre de la red Wi-Fi.
 * @param password Contraseña de la red Wi-Fi.
 * @param ip       Dirección IP estática, o NULL para usar DHCP.
 * @param gw       Puerta de enlace, o NULL para usar DHCP.
 * @param netmask  Máscara de red, o NULL para usar DHCP.
 * @param dns      Dirección IP del servidor DNS, solo si se usa IP estática (puede ser NULL).
 * 
 * @return esp_err_t ESP_OK si fue exitoso, otro código de error si falla.
 */
esp_err_t wifi_conect(
    const char *ssid, 
    const char *password, 
    const char *ip, 
    const char *gw, 
    const char *netmask,
    const char *dns
);

/**
 * @brief Devuelve la IP actual.
 * 
 * @return const char* IP en formato texto ("192.168.x.x") o NULL si 
 * no hay conexión.
 * El puntero apunta a una cadena estática, no hace falta liberarla.
 */
const char* wifi_conect_get_ip(void);

// Indica si estamos conectados a una red wifi o no
bool wifi_conect_is_connected(void);

// Indica si estamos conectados internet
bool internet_connected_ip(void);

bool internet_connected_dns(void);

/**
 * @brief Desconecta el ESP32 de la red WiFi actual y detiene el driver WiFi.
 * @return esp_err_t ESP_OK si fue exitoso, otro código de error si falla.
 */
esp_err_t wifi_conect_disconnect(void);
