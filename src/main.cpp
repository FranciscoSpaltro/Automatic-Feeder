#include <Arduino.h>
#include "wl134.h"

WL134 rfid;
QueueHandle_t cola_uart;    // Es un puntero a la cola de eventos UART, lo crea el driver

void setup() {
    Serial.begin(115200);

    if (!rfid.begin(cola_uart)) {
        Serial.println("Error iniciando UART");
        while(1);
    }
    
    Serial.println("Sistema esperando eventos UART...");
}

void loop() {
    uart_event_t event;

    if (xQueueReceive(cola_uart, (void *)&event, portMAX_DELAY)) {
        
        switch (event.type) {
            case UART_DATA:
                if (event.size >= 30) {
                   Serial.printf("Evento UART: Llegaron %d bytes\n", event.size);
                   
                   uint64_t id = rfid.read();
                   if (id > 0) {
                       Serial.printf("VACA DETECTADA ID: %llu\n", id);
                    

                       /** ESPACIO PARA LA LOGICA **/



                   }
                }
                break;
            case UART_FIFO_OVF:
                Serial.println("Error: Buffer lleno");
                uart_flush_input(UART_NUM_2);
                xQueueReset(cola_uart);
                break;

            default:
                break;
        }
    }
}