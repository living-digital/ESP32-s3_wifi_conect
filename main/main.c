/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "wifi_conect.h"


void app_main(void)
{
    printf("Inicio de main\n");

    printf("Lanzamos wifi_conect()\n");
    wifi_conect("elArreglador", "1234567890", "192.168.43.101", "192.168.43.1", "255.255.255.0");
    // wifi_conect("MOVISTAR_PLUS_606E", "111D3321BD3B5C34AA26", "192.168.18.250", "192.168.18.1", "255.255.255.0");
    printf("Regresamos al main...\n");

}
