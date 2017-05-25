#ifndef __UART_UTIL_H__
#define __UART_UTIL_H__

/*
 *
 * Copyright (C) 2017 258633901@qq.com
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 *
 */
 
int uart_open(char* port);
int uart_set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);
int uart_recv(int fd, char *rcv_buf,int data_len);
int uart_send(int fd, char *send_buf,int data_len);

#endif

