idf_component_register(SRCS "wifi_conect.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_event esp_wifi esp_netif nvs_flash)