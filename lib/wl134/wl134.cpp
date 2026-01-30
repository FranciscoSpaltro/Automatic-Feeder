#include "wl134.h"

bool WL134::begin(QueueHandle_t &uart_queue) {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Param 5: Tamaño de la cola (10 eventos)
    // Param 6: Puntero donde guardar el handle de la cola (&uart_queue)
    if (uart_driver_install(uart_num, BUF_SIZE * 2, 0, 10, &uart_queue, 0) != ESP_OK) {
        return false;
    }
    
    return true;
}

uint64_t WL134::read() {
    size_t length = 0;
    uart_get_buffered_data_len(uart_num, &length);
    
    int len = uart_read_bytes(uart_num, buffer, length, 100 / portTICK_PERIOD_MS);
    
    if (len >= 30) { 
        return parse(buffer);
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
    
    // Imprimo los bytes recibidos para debug
    Serial.print("Datos recibidos: ");
    for(int i = 0; i < 30; i++) {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();

    if(data[0] != 0x02 || data[29] != 0x03)
        return 0;

    /* Para leer el card number:
     * 1. Invertir los bytes
     * 2. Traducir a ASCII
     * 3. Convertir el numero a DECIMAL
    */

    uint64_t result = 0;
    for(int i = 9; i >= 0; i--) {
        uint8_t byte = data[1 + i];
        if(byte >= 'A' && byte <= 'F')
            result = (result << 4) + (byte - 'A' + 10);
        else if(byte >= '0' && byte <= '9')
            result = (result << 4) + (byte - '0');
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