#ifndef WL134_H
#define WL134_H

#include <Arduino.h>

class WL134 {
public:
    void begin();

    uint64_t read();

private:
    const int rx_pin = 16;
    const int tx_pin = 17;
    const long baud_rate = 9600;
    
    uint8_t buffer[32]; 

    uint64_t parse(uint8_t* data);
};

#endif