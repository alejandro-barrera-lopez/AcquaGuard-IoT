# Contenido de la carpeta `cloud/`

Esta carpeta contiene todos los ficheros necesarios para desplegar el entorno de software utilizado tanto en el **Nodo Fog (Raspberry Pi)** como en el **Nodo Cloudlet (PC)**.

## Despliegue del entorno

Siga estos pasos para un despliegue limpio:

1.  **Instalar dependencias de Node-RED:**
    Antes de levantar los servicios, es necesario instalar los nodos adicionales (como el de OpenWeatherMap). Desde la carpeta `cloud/`, ejecute:
    ```bash
    (cd node_red_data && npm install)
    ```

2.  **Levantar los servicios:**
    Una vez instaladas las dependencias, levante los contenedores con Docker Compose:
    ```bash
    docker-compose up -d
    ```

## Estructura de ficheros

La carpeta debe contener los siguientes elementos:

1.  **`docker-compose.yml`**:
    *   Este archivo define los servicios de `node-red` and `mosquitto` que componen la aplicación.

2.  **`mosquitto/`**:
    *   Este directorio contiene la configuración, la base de datos de persistencia y los logs del broker MQTT Mosquitto. Esencial para que el broker arranque con la configuración correcta y mantenga los datos.

3.  **`node_red_data/`**:
    *   Este directorio es el volumen de datos de Node-RED. Contiene toda la información crítica, incluyendo:
        *   **`flows.json`**: El fichero más importante, que contiene toda la lógica de tu sistema (los flujos que has diseñado).
        *   Nodos adicionales que hayas instalado (definidos en `package.json`).
        *   Credenciales y configuraciones del editor.
