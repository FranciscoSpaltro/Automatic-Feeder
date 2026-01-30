#ifndef WL134_H
#define WL134_H

#include <Arduino.h>
#include <driver/uart.h>

class WL134 {
public:
    bool begin(QueueHandle_t &uart_queue);

    uint64_t read();

private:
    const uart_port_t uart_num = UART_NUM_2;
    const int tx_pin = 17;
    const int rx_pin = 16;
    const int baud_rate = 9600;
    
    static const int BUF_SIZE = 256;
    uint8_t buffer[BUF_SIZE];

    uint64_t parse(uint8_t* data);
};

#endif