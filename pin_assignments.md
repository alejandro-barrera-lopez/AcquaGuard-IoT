# Pin Assignments for AcquaGuard-IoT Nodes

This document summarizes the pin assignments for the sensors and actuators used in Juan-Victor's and Alex's ESP32 nodes.

## Juan-Victor's Node (`AcquaGuard-IoT/juan-victor/nodo_jv/nodo_jv.ino`)

*   **Luminosity Sensor (Keyestudio TEMT6000)**:
    *   'S' pin (Analog Output) connected to **GPIO 14 (IO14)**.
*   **Traffic Light Actuator (Keyestudio Traffic Light Module)**:
    *   Red LED connected to **GPIO 10 (IO10)**.
    *   Yellow LED connected to **GPIO 6 (IO6)**.
    *   Green LED connected to **GPIO 4 (IO4)**.

## Alex's Node (`AcquaGuard-IoT/alejandro/nodo_alex.ino`)

*   **Temperature Sensor (Keyestudio Ks0033 Analog Temperature Sensor)**:
    *   Analog Output connected to **GPIO 4 (A0)**.
*   **Servo Motor (Keyestudio Ks0209 9G Servo Motor)**:
    *   Control Signal connected to **GPIO 3 (D2)**.

## Sebastian's Node (`AcquaGuard-IoT/sebastian/nodo_sebas/nodo_sebas.ino`)

*   **Vapor Sensor (Analog)**:
    *   Analog Output connected to **GPIO 2**.
*   **Buzzer (Keyestudio Buzzer)**:
    *   Control Signal connected to **GPIO 17**.
*   **Integrated LED**:
    *   Control Signal connected to **GPIO 21**.
