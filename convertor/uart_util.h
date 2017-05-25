#ifndef __UART_UTIL_H__
#define __UART_UTIL_H__

int uart_open(char* port);
int uart_set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);
int uart_recv(int fd, char *rcv_buf,int data_len);
int uart_send(int fd, char *send_buf,int data_len);

#endif

