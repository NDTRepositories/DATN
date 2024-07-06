#ifndef LIB_UARTS3_H_
#define LIB_UARTS3_H_

// Định nghĩa số hiệu UART và kích thước bộ đệm
#define UART_NUM UART_NUM_1
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

void uart_init(int tx_pin, int rx_pin);
void uart_send_data(const char* data, size_t length);
void uart_receive_data(char* data, size_t max_length);

#endif /* LIB_UARTS3_H_ */