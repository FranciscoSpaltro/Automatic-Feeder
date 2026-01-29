#include "wl134.h"

void WL134::begin() {
    Serial2.begin(baud_rate, SERIAL_8N1, rx_pin, tx_pin);
    Serial2.setRxBufferSize(256); 
}

uint64_t WL134::read() {
    if (Serial2.available() >= 30) {
        int len = Serial2.readBytes(buffer, 30);
        
        if (len == 30) {
            return parse(buffer);
        }
    }
    return 0;
}

uint64_t WL134::parse(uint8_t* data) {
    /* TRAMA DE 30 BYTES
     * HEADER 0x02      1 BYTE
     * CARD NUMBER      10 BYTES
     * COUNTRY NUMBER   4 BYTES
     * DATA BLOCK       1 BYTE
     * ANIMAL FLAG      1 BYTE
     * RESERVED         4 BYTES
     * RESERVED         6 BYTES
     * CHECKSUM         1 BYTE
     * CHECKSUM INVERT  1 BYTE
     * END 0X03         1 BYTE
     */

    if(data[0] != 0x02 || data[29] != 0x03)
        return 0;

    /* Para leer el card number:
     * 1. Invertir los bytes
     * 2. Traducir a ASCII
     * 3. Convertir el numero a DECIMAL
    */

    uint64_t result = 0;
    for(int i = 0; i < 10; i++) {
        if(data[1 + i] >= 'A' && data[1 + i] <= 'F')
            result = (result << 4) + (data[1 + i] - 'A' + 10);
        else if(data[1 + i] >= '0' && data[1 + i] <= '9')
            result = (result << 4) + (data[1 + i] - '0');
        else
            return 0;
    }

    /* Para evitar el costo de las potencias, se hace un corrimiento de 4 bits (multiplicar por 16) y suma
     * Por ejemplo: 0x1A5 = (1 * 16^2) + (10 * 16^1) + (5 * 16^0) = 4261
     *                    = (((1 << 4) + 10) << 4) + 5 =
     *                    = (1 * 2^4 + 10) * 2^4 + 5 =
     *                    = 1 * 16^2 + 10 * 16 + 5 * 16^0
     */

    return result;
}