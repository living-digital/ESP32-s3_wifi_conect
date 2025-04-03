# ESP32 WiFi Conexión con IP Estática o DHCP
Este proyecto permite conectar un ESP32 (en particular, un ESP32-S3) a una red WiFi, usando ya sea una IP asignada por DHCP o una IP fija configurada manualmente.

## Características principales:

Conexión WiFi en modo estación (STA)

Soporte para DHCP y configuración manual de IP

Reconexión automática en caso de desconexión

Impresión en consola de la IP obtenida, gateway y máscara

Detección del tipo de conexión (fija o dinámica)

Registro del motivo de desconexión para facilitar depuración

## Requisitos previos:

ESP-IDF instalado (v5.0 o superior recomendado)

Cable USB para flashear el ESP32

Red WiFi de 2.4 GHz (el ESP32 no es compatible con redes de 5 GHz)

Estructura de archivos:

'''
main/
   wifi_conect.c → Implementación de la lógica de conexión WiFi
   wifi_conect.h → Declaración de la función wifi_conect
   hello_world_main.c → Ejemplo de uso
   CMakeLists.txt → Inclusión de archivos fuente en la compilación
'''

## Uso básico desde el código:

- Para conectar usando DHCP:
wifi_conect("nombre_de_red", "clave_wifi", NULL, NULL, NULL);

- Para conectar usando IP fija:
wifi_conect("nombre_de_red", "clave_wifi", "192.168.1.50", "192.168.1.1", "255.255.255.0");

La función mostrará en consola si la conexión fue exitosa y los detalles de la IP asignada.

## Notas importantes:

Asegúrate de que la red sea visible y opere en la banda de 2.4 GHz

Si estás usando IP fija, evita conflictos con el rango DHCP del router

El sistema intentará reconectarse automáticamente si se pierde la conexión

## Motivos de desconexión:
Se muestran en consola como "Disconnected, reason: X".
Por ejemplo:

reason 2: Fallo de autenticación

reason 205: Rechazo del router (ej. roaming forzado, red de 5 GHz)

## Solución de problemas:

Si el ESP32 no se conecta, prueba compartir una red WiFi simple desde el móvil

Verifica el canal y banda de frecuencia de tu red

Puedes consultar la documentación de ESP-IDF para interpretar más códigos de error

## Notas:
Desarrollado con ESP-IDF para ESP32-S3