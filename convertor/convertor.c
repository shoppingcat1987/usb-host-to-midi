#include<stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include<stdlib.h>

#include <sys/types.h>
#include <sys/poll.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <pthread.h>
#include<signal.h>
#include "uart_util.h"

#define SUPPORT_LED_INDICATOR 1

#define USB_READ_BUF_SIZE 64
#define USB_MIDI_EVENT_SIZE 4
#define RECV_BUF_SIZE  512
#define CONV_SLOT_NUM 32
#define MAX_UART_NUM 3

struct sigaction old_int;

struct midi_convertor
{
	int busy;
	int midi_minor;
	int midi_dev;
	char midi_dev_name[64];
	int uart_minor;
	int uart_dev;
	char uart_dev_name[64];
	char cable_num;
	pthread_t uart_to_usb_thread;
	pthread_t usb_to_uart_thread;
};


struct midi_convertor conv_slot[CONV_SLOT_NUM]={0};



void decode_uart_midi_packet(char midi_status,char midi_channel,long slot_num,char *prev_midi_status,char *prev_midi_channel,unsigned char *usb_midi_event,unsigned char *uart_midi_event,int repeat)
{
	int i;
	int write_len;
	int read_len;
	char midi_size=0;
	
	switch(midi_status)
	{
	case 0x08:
		//note off
		midi_size=3;

		if((midi_size-1-repeat)==0)
			read_len=0;
		else
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1+repeat,midi_size-1-repeat);

		if(read_len==(midi_size-1-repeat))
		{
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|midi_status);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=uart_midi_event[1];
			usb_midi_event[3]=uart_midi_event[2];
		}
		else
		{
			printf("uart recv fail\n");
			midi_size=0;
		}
		break;
	case 0x09:
		//note on
		midi_size=3;

		if((midi_size-1-repeat)==0)
			read_len=0;
		else
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1+repeat,midi_size-1-repeat);

		if(read_len==(midi_size-1-repeat))
		{
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|midi_status);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=uart_midi_event[1];
			usb_midi_event[3]=uart_midi_event[2];
		}
		else
		{
			printf("uart recv fail\n");
			midi_size=0;
		}
		break;
	case 0x0a:
		//key after touch
		midi_size=3;

		if((midi_size-1-repeat)==0)
			read_len=0;
		else
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1+repeat,midi_size-1-repeat);

		if(read_len==(midi_size-1-repeat))
		{
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|midi_status);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=uart_midi_event[1];
			usb_midi_event[3]=uart_midi_event[2];
		}
		else
		{
			printf("uart recv fail\n");
			midi_size=0;
		}
		break;
	case 0x0b:
		//control change
		midi_size=3;

		if((midi_size-1-repeat)==0)
			read_len=0;
		else
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1+repeat,midi_size-1-repeat);

		if(read_len==(midi_size-1-repeat))
		{
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|midi_status);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=uart_midi_event[1];
			usb_midi_event[3]=uart_midi_event[2];
		}
		else
		{
			printf("uart recv fail\n");
			midi_size=0;
		}
		break;
	case 0x0c:
		//program change
		midi_size=2;

		if((midi_size-1-repeat)==0)
			read_len=0;
		else
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1+repeat,midi_size-1-repeat);

		if(read_len==(midi_size-1-repeat))
		{
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|midi_status);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=uart_midi_event[1];
			usb_midi_event[3]=0;
		}
		else
		{
			printf("uart recv fail\n");
			midi_size=0;
		}
		break;
	case 0x0d:
		//channel after touch
		midi_size=2;

		if((midi_size-1-repeat)==0)
			read_len=0;
		else
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1+repeat,midi_size-1-repeat);

		if(read_len==(midi_size-1-repeat))
		{
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|midi_status);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=uart_midi_event[1];
			usb_midi_event[3]=0;
		}
		else
		{
			printf("uart recv fail\n");
			midi_size=0;
		}
		break;
	case 0x0e:
		//pitch bend
		midi_size=3;

		if((midi_size-1-repeat)==0)
			read_len=0;
		else
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1+repeat,midi_size-1-repeat);
		
		if(read_len==(midi_size-1-repeat))
		{
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|midi_status);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=uart_midi_event[1];
			usb_midi_event[3]=uart_midi_event[2];
		}
		else
		{
			printf("uart recv fail\n");
			midi_size=0;
		}
		break;
	case 0x0f:
		
	{
		switch(midi_channel)
		{
		case 0x00:
		case 0x04:
			{
				char uart_data[1];
				int extra_data_count=1;
				int midi_data_pos=1;

				uart_data[0]=((midi_status<<4)|midi_channel);
				usb_midi_event[midi_data_pos]=uart_data[0];
				uart_midi_event[midi_data_pos-1]=uart_data[0];
				midi_data_pos++;
				
				while(extra_data_count<127)
				{
					//load the extra data from UART
					read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_data,1);
					if(read_len==1)
					{
						extra_data_count++;
						if(uart_data[0]==0xf7)
						{
							//reach the end of sys extra
							usb_midi_event[midi_data_pos]=uart_data[0];
							uart_midi_event[midi_data_pos-1]=uart_data[0];
							midi_data_pos++;
							break;
						}
						else
						{
							//load the extra data
							usb_midi_event[midi_data_pos]=uart_data[0];
							uart_midi_event[midi_data_pos-1]=uart_data[0];
							midi_data_pos++;
							if(midi_data_pos==4)
							{
								midi_data_pos=1;
								//reach the usb midi package size ,send it out
								//SysEx starts or continues
								usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x04);
								
								midi_size=3;
								if(midi_size)
								{
									//should always sent 4 byte per packet
									for(i=midi_size+1;i<4;i++)
										usb_midi_event[i]=0;
									write_len=write(conv_slot[slot_num].midi_dev,usb_midi_event,4);
									if(write_len==4)
									{
										switch(midi_size)
										{
										case 0:
											break;
										case 1:
											printf("UART(0x%02x)  -->  USB(0x%02x 0x%02x 0x%02x 0x%02x)\n",uart_midi_event[0],usb_midi_event[0],usb_midi_event[1],usb_midi_event[2],usb_midi_event[3]);
											break;
										case 2:
											printf("UART(0x%02x 0x%02x)  -->  USB(0x%02x 0x%02x 0x%02x 0x%02x)\n",uart_midi_event[0],uart_midi_event[1],usb_midi_event[0],usb_midi_event[1],usb_midi_event[2],usb_midi_event[3]);
											break;
										case 3:
											printf("UART(0x%02x 0x%02x 0x%02x)  -->  USB(0x%02x 0x%02x 0x%02x 0x%02x)\n",uart_midi_event[0],uart_midi_event[1],uart_midi_event[2],usb_midi_event[0],usb_midi_event[1],usb_midi_event[2],usb_midi_event[3]);
											break;
										default:
											break;
										}
										
									}
									
									*prev_midi_status=midi_status;
									*prev_midi_channel=midi_channel;
								}
							}
						}
					}
					else
					{
						printf("uart recv fail\n");
						midi_size=0;
						break;
					}
				}
				if(uart_data[0]==0xf7)
				{
					//SysEx ends 
					if(midi_data_pos==4)
					{
						//witch three bytes
						usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x07);
						midi_size=3;
					}	
					else if(midi_data_pos==3)
					{
						//SysEx ends with following two bytes.
						usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x06);
						midi_size=2;
					}
					else if(midi_data_pos==2)
					{
						//SysEx ends with following single byte.
						usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x05);
						midi_size=1;
					}
					else
					{
						//should not goto this position
					}
				}
				else
				{
					//reach maximon package or UART recv fail , drop the package
					midi_size=0;
				}
	
			}
			break;
		case 0x01:
			midi_size=2;
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1,midi_size-1);
			if(read_len==(midi_size-1))
			{
				usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x02);
				usb_midi_event[1]=uart_midi_event[0];
				usb_midi_event[2]=uart_midi_event[1];
				usb_midi_event[3]=0;
			}
			else
			{
				printf("uart recv fail\n");
				midi_size=0;
			}

			
			break;
		case 0x02:
			midi_size=3;
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1,midi_size-1);
			if(read_len==(midi_size-1))
			{
				usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x03);
				usb_midi_event[1]=uart_midi_event[0];
				usb_midi_event[2]=uart_midi_event[1];
				usb_midi_event[3]=uart_midi_event[2];
				
			}
			else
			{
				printf("uart recv fail\n");
				midi_size=0;
			}
			break;
		case 0x03:
			midi_size=2;
			read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event+1,midi_size-1);
			if(read_len==(midi_size-1))
			{
				usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x02);
				usb_midi_event[1]=uart_midi_event[0];
				usb_midi_event[2]=uart_midi_event[1];
				usb_midi_event[3]=0;
			}
			else
			{
				printf("uart recv fail\n");
				midi_size=0;
			}
			
			break;
		case 0x06:

			midi_size=1;
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x05);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=0;
			usb_midi_event[3]=0;

			
			break;
		case 0x07:
			//variable extra sys info , not support	
			midi_size=0;
			break;
		case 0x08:
		case 0x09:
			midi_size=1;
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x05);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=0;
			usb_midi_event[3]=0;
			break;
	
		case 0x0a:
			midi_size=1;
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x05);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=0;
			usb_midi_event[3]=0;
			break;
		case 0x0b:
			midi_size=1;
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x05);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=0;
			usb_midi_event[3]=0;
			break;
		case 0x0c:
			midi_size=1;
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x05);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=0;
			usb_midi_event[3]=0;
			break;
		case 0x0e:
			midi_size=1;
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x05);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=0;
			usb_midi_event[3]=0;
			break;
		case 0x0f:
			midi_size=1;
			usb_midi_event[0]=((conv_slot[slot_num].cable_num<<4)|0x05);
			usb_midi_event[1]=uart_midi_event[0];
			usb_midi_event[2]=0;
			usb_midi_event[3]=0;
			break;
		}
		
	
	}
	break;
	default:
	{
		//anonymouse status code packet, use the last status code
		//status code should be less than 0X7F
		
		if(((midi_status<<4)|midi_channel)<0x7f)
		{
			if(((*prev_midi_status)!=0)&&((*prev_midi_status)!=0x0f))
			{
				uart_midi_event[0]=((*prev_midi_status<<4)|*prev_midi_channel);
				uart_midi_event[1]=((midi_status<<4)|midi_channel);
				decode_uart_midi_packet(*prev_midi_status,*prev_midi_channel,slot_num,prev_midi_status,prev_midi_channel,usb_midi_event,uart_midi_event,1);
			}
			else
			{
				printf("err anonymouse status code packet, drop this packet last midi_status=%x last midi_channel=%x\n",*prev_midi_status,*prev_midi_channel);
			}
		}
		else
		{
			printf("err anonymouse status code packet, drop this packet midi_status=%x midi_channel=%x\n",midi_status,midi_channel);
		}
		midi_size=0;
	}
	break;
	
	}
	
	if(midi_size)
	{
		//should always sent 4 byte per packet
		for(i=midi_size+1;i<4;i++)
			usb_midi_event[i]=0;
		write_len=write(conv_slot[slot_num].midi_dev,usb_midi_event,4);
		if(write_len==4)
		{
			switch(midi_size)
			{
			case 0:
				break;
			case 1:
				printf("UART(0x%02x)  -->  USB(0x%02x 0x%02x 0x%02x 0x%02x)\n",uart_midi_event[0],usb_midi_event[0],usb_midi_event[1],usb_midi_event[2],usb_midi_event[3]);
				break;
			case 2:
				printf("UART(0x%02x 0x%02x)  -->  USB(0x%02x 0x%02x 0x%02x 0x%02x)\n",uart_midi_event[0],uart_midi_event[1],usb_midi_event[0],usb_midi_event[1],usb_midi_event[2],usb_midi_event[3]);
				break;
			case 3:
				printf("UART(0x%02x 0x%02x 0x%02x)  -->  USB(0x%02x 0x%02x 0x%02x 0x%02x)\n",uart_midi_event[0],uart_midi_event[1],uart_midi_event[2],usb_midi_event[0],usb_midi_event[1],usb_midi_event[2],usb_midi_event[3]);
				break;
			default:
				break;
			}
			
		}
		
		*prev_midi_status=midi_status;
		*prev_midi_channel=midi_channel;
		
	}
	}


	

void *uart_to_usb_thread(void *param)
{
	
	long slot_num;
	int read_len;
	char midi_status=0;
	char midi_channel=0;
	char prev_midi_status=0;
	char prev_midi_channel=0;
	unsigned char usb_midi_event[4];
	unsigned char uart_midi_event[64];
	slot_num=(long)param;

	while(conv_slot[slot_num].midi_dev&&conv_slot[slot_num].uart_dev)
	{	
		memset(usb_midi_event,0,4);
		memset(uart_midi_event,0,64);
		read_len=uart_recv(conv_slot[slot_num].uart_dev, uart_midi_event,1);
		if(read_len==1)
		{
			midi_status=(uart_midi_event[0]>>4)&0x0f;
			midi_channel=(uart_midi_event[0]>>0)&0x0f;
			decode_uart_midi_packet(midi_status,midi_channel,slot_num,&prev_midi_status,&prev_midi_channel,usb_midi_event,uart_midi_event,0);
		}
	}
	printf("uart_to_usb_thread quit\n");
	return 0;
}



void *usb_to_uart_thread(void *param)
{
	int i;
	int j;
	long slot_num;
	int write_len;
	int read_len;
	unsigned char usb_midi_event[USB_MIDI_EVENT_SIZE];
	unsigned char usb_read_buffer[USB_READ_BUF_SIZE];
	unsigned char uart_midi_event[3];
	struct pollfd		pfd;
	
	
	slot_num=(long)param;
	pfd.events = POLLIN;
	pfd.fd = conv_slot[slot_num].midi_dev;
	
	while(conv_slot[slot_num].midi_dev&&conv_slot[slot_num].uart_dev)
	{	
		if (poll(&pfd, 1, -1) <= 0)
			continue;

		memset(usb_read_buffer,0,USB_READ_BUF_SIZE);
		read_len=read(conv_slot[slot_num].midi_dev,usb_read_buffer,USB_READ_BUF_SIZE);

		for(j=0;j<read_len;j+=USB_MIDI_EVENT_SIZE)
		{
			char cable_num;
			char code_idx;
			char midi_size=0;
			
			
			memset(usb_midi_event,0,USB_MIDI_EVENT_SIZE);
			memcpy(usb_midi_event,usb_read_buffer+j,USB_MIDI_EVENT_SIZE);

			cable_num=(usb_midi_event[0]>>4)&0x0f;
			code_idx=(usb_midi_event[0]>>0)&0x0f;

			
			conv_slot[slot_num].cable_num=cable_num;
			switch(code_idx)
			{
			case 0x00:
				//Miscellaneous function codes. Reserved for future extensions.
				midi_size=0;
				break;
			case 0x01:
				//Cable events. Reserved for future expansion.
				midi_size=0;
				break;
			case 0x02:
				//Two-byte System Common messages like MTC, SongSelect, etc.
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				midi_size=2;
				break;
			case 0x03:
				//Three-byte System Common messages like SPP, etc.
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				uart_midi_event[2]=usb_midi_event[3];
				midi_size=3;
				break;
			case 0x04:
				//SysEx starts or continues
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				uart_midi_event[2]=usb_midi_event[3];
				midi_size=3;
				
				break;
			case 0x05:
				//Single-byte System Common Message or SysEx ends with following single byte
				uart_midi_event[0]=usb_midi_event[1];
				midi_size=1;
				break;
			case 0x06:
				//SysEx ends with following two bytes.
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				midi_size=2;
				break;
			case 0x07:
				//SysEx ends with following three bytes.
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				uart_midi_event[2]=usb_midi_event[3];
				midi_size=3;
				break;
			case 0x08:
				//Note-off
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				uart_midi_event[2]=usb_midi_event[3];
				midi_size=3;
				break;
			case 0x09:
				//Note-on
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				uart_midi_event[2]=usb_midi_event[3];
				midi_size=3;

				
				break;
			case 0x0a:
				//Poly-KeyPress
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				uart_midi_event[2]=usb_midi_event[3];
				midi_size=3;
				break;
			case 0x0b:
				//Control Change
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				uart_midi_event[2]=usb_midi_event[3];
				midi_size=3;
				break;
			case 0x0c:
				//Program Change
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				midi_size=2;
				break;
			case 0x0d:
				//Channel Pressure
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				midi_size=2;
				break;
			case 0x0e:
				//PitchBend Change
				uart_midi_event[0]=usb_midi_event[1];
				uart_midi_event[1]=usb_midi_event[2];
				uart_midi_event[2]=usb_midi_event[3];
				midi_size=3;
				break;
			case 0x0f:
				//Single Byte
				uart_midi_event[0]=usb_midi_event[1];
				midi_size=1;
				break;
			default:
				break;
			}
			
			
			write_len=uart_send(conv_slot[slot_num].uart_dev, uart_midi_event,midi_size);
			switch(midi_size)
			{
			case 0:
				break;
			case 1:
				printf("UART(0x%02x)  <---   USB(0x%02x 0x%02x)\n",uart_midi_event[0],usb_midi_event[0],usb_midi_event[1]);
				break;
			case 2:
				printf("UART(0x%02x 0x%02x)  <---   USB(0x%02x 0x%02x 0x%02x)\n",uart_midi_event[0],uart_midi_event[1],usb_midi_event[0],usb_midi_event[1],usb_midi_event[2]);
				break;
			case 3:
				printf("UART(0x%02x 0x%02x 0x%02x)  <---   USB(0x%02x 0x%02x 0x%02x 0x%02x)\n",uart_midi_event[0],uart_midi_event[1],uart_midi_event[2],usb_midi_event[0],usb_midi_event[1],usb_midi_event[2],usb_midi_event[3]);
			default:
				break;
			}
	
		}

		
		
		
	}
	
	printf("usb_to_uart_thread quit\n");
	return 0;
}

void start_convert(char *action, char *devpath, char *subsys, int major, int minor)
{
	
	if(subsys)
	if((strcmp(subsys,"usb")==0)||(strcmp(subsys,"usbmisc")==0))
	{
		unsigned char *dev_name=0;
		unsigned char dev_path[1024]={0};
		dev_name=strrchr(devpath,'/')+1;
		if(strstr(dev_name,"USBMidi")!=0)
		{
			long i,j,k;
			long dev_minor=0;
			char uart_dev_path[64]={0};
			char uart_dev_name[64]={0};
			sprintf(dev_path,"/dev/%s",dev_name);
			dev_minor=atoi(&(dev_name[strlen("USBMidi")]));
			
			if(action)
			if(strcmp(action,"add")==0)
			{
				
				for(i=0;i<CONV_SLOT_NUM;i++)
				{
					if(conv_slot[i].midi_minor==dev_minor)
						break;
				}
				if(i<CONV_SLOT_NUM)
				{
				
					return;
				}
				for(i=0;i<CONV_SLOT_NUM;i++)
				{
					if(!conv_slot[i].busy)
						break;
				}
				if(i==CONV_SLOT_NUM)
				{
					printf("no empty slot available\n");
					return;
				}

				
				
				if(conv_slot[i].midi_dev!=0)
				{
					printf("midi device already opened\n");
					close(conv_slot[i].midi_dev);
					conv_slot[i].midi_dev=0;
				}


				
				conv_slot[i].midi_dev=open(dev_path,O_RDWR);
				if(conv_slot[i].midi_dev==0)
				{
					printf("fail to open midi device\n");
					conv_slot[i].busy=0;
					return;
				}
				else
				{
					conv_slot[i].midi_minor=dev_minor;
					strcpy(conv_slot[i].midi_dev_name,dev_name);
				}
				
	
				
				if(conv_slot[i].uart_dev!=0)
				{
					printf("uart device already opened\n");
					close(conv_slot[i].uart_dev);
					conv_slot[i].uart_dev=0;
				}

				
				for(k=1;k<MAX_UART_NUM;k++)
				{
					for(j=0;j<CONV_SLOT_NUM;j++)
					{
						if(conv_slot[j].uart_minor==k)
							break;
					}
					if(j==CONV_SLOT_NUM)
						break;
				}

				if(k==MAX_UART_NUM)
				{
					printf("no uart port available\n");
					close(conv_slot[i].midi_dev);
					conv_slot[i].midi_dev=0;
					conv_slot[i].uart_dev=0;
					conv_slot[i].busy=0;
					return;
				}
						
				
				sprintf(uart_dev_path,"/dev/ttyS%d",(int)k);
				sprintf(uart_dev_name,"ttyS%d",(int)k);
				

				conv_slot[i].uart_dev=uart_open(uart_dev_path);
				if(conv_slot[i].uart_dev<=0)
				{
					printf("fail to open uart device:%s\n",uart_dev_path);
					close(conv_slot[i].midi_dev);
					conv_slot[i].midi_dev=0;
					conv_slot[i].uart_dev=0;
					conv_slot[i].busy=0;
					return;
				}
				else
				{
					conv_slot[i].uart_minor=k;
					strcpy(conv_slot[i].uart_dev_name,uart_dev_name);
					if(uart_set(conv_slot[i].uart_dev,38400,0,8,1,'N')<0)
					{
						printf("fail to configure UART : %s\n",conv_slot[i].uart_dev_name);
					}

				}

				#if SUPPORT_LED_INDICATOR
				if(conv_slot[i].uart_minor==1)
					system("echo 1 > /sys/class/gpio_sw/PA6/data");
				else
					system("echo 1 > /sys/class/gpio_sw/PG11/data");
				#endif
				
				printf("start convert %s <--> %s \n",conv_slot[i].midi_dev_name,conv_slot[i].uart_dev_name);
				
				if(pthread_create(&conv_slot[i].usb_to_uart_thread,0,usb_to_uart_thread,(void*)i)==-1)
				{
					printf("create usb_to_uart_thread fail\n");
				}
				if(pthread_create(&conv_slot[i].uart_to_usb_thread,0,uart_to_usb_thread,(void*)i)==-1)
				{
					printf("create uart_to_usb_thread fail\n");
				}
				
				conv_slot[i].busy=1;

				
				
			}
			else if(strcmp(action,"remove")==0)
			{
				
				for(i=0;i<CONV_SLOT_NUM;i++)
				{
					if(conv_slot[i].midi_minor==dev_minor)
						break;
				}
				
				if(i==CONV_SLOT_NUM)
				{
				
					return;
				}
				

				
				if(conv_slot[i].midi_dev!=0)
				{
					void *retval;
					strcpy(conv_slot[i].midi_dev_name,"");
					close(conv_slot[i].midi_dev);
					conv_slot[i].midi_dev=0;
					conv_slot[i].midi_minor=-1;

					strcpy(conv_slot[i].uart_dev_name,"");
					close(conv_slot[i].uart_dev);
					conv_slot[i].uart_dev=0;
					
					
					pthread_join(conv_slot[i].uart_to_usb_thread, &retval);
					pthread_join(conv_slot[i].usb_to_uart_thread, &retval);
					conv_slot[i].uart_to_usb_thread=0;
					conv_slot[i].usb_to_uart_thread=0;
					conv_slot[i].busy=0;

					#if SUPPORT_LED_INDICATOR
					if(conv_slot[i].uart_minor==1)
						system("echo 0 > /sys/class/gpio_sw/PA6/data");
					else if(conv_slot[i].uart_minor==2)
						system("echo 0 > /sys/class/gpio_sw/PG11/data");
					#endif
					conv_slot[i].uart_minor=-1;
				}
				else
				{
					printf("device already closed\n");
				}
			}
			else
			{
				printf("unkown action\n");
			}


			

			
			
		}
	}

	
	

	
}


void sighandler(int signo,siginfo_t *info,void *context)
{
	switch(signo)
	{
	case SIGINT:
		{
			int i;
			for(i=0;i<CONV_SLOT_NUM;i++)
			{
				if(conv_slot[i].busy)
				{
					void *retval;
					
					
					strcpy(conv_slot[i].midi_dev_name,"");
					strcpy(conv_slot[i].uart_dev_name,"");
					conv_slot[i].midi_minor=-1;
					conv_slot[i].uart_minor=-1;

					if(conv_slot[i].midi_dev)
					{
						close(conv_slot[i].midi_dev);
						conv_slot[i].midi_dev=0;
						
					}
						
					if(conv_slot[i].uart_dev)
					{
						close(conv_slot[i].uart_dev);
						conv_slot[i].uart_dev=0;
					}
					if(conv_slot[i].uart_to_usb_thread)
					{
						pthread_join(conv_slot[i].uart_to_usb_thread, &retval);
						conv_slot[i].uart_to_usb_thread=0;
					}

					if(conv_slot[i].usb_to_uart_thread)
					{
						pthread_join(conv_slot[i].usb_to_uart_thread, &retval);
						conv_slot[i].usb_to_uart_thread=0;
					}

					conv_slot[i].busy=0;
				}
			}
			sigaction(SIGINT,&old_int,NULL);
			kill(getpid(), SIGINT);
			
		}
		break;
	}
}


int init_signal()
{
	int ret;
	struct sigaction sa;

	sa.sa_sigaction=&sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags=SA_SIGINFO;
	if((ret=sigaction(SIGINT,&sa,&old_int))!=0) return ret;
	return 0;
}


int monitor_midi_device()
{
	static char  recv_buf[RECV_BUF_SIZE];
	struct sockaddr_nl	nls;
	struct pollfd		pfd;
	memset(&nls, 0, sizeof(struct sockaddr_nl));
	nls.nl_family = AF_NETLINK;
	nls.nl_pid	  = getpid();
	nls.nl_groups = -1;
	pfd.events = POLLIN;
	printf("convertor on the fly!\n");
	if ((pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)) == -1)
		return -1;
	
	if (bind(pfd.fd, (void*)&nls, sizeof(struct sockaddr_nl)))
		return -1;


	while (1)
	{
		if (poll(&pfd, 1, -1) <= 0)
			continue;
		char*  action  = NULL;
		char*  devpath = NULL;
		char*  subsys  = NULL;
		int    major = -1;
		int    minor = -1;
		int    len = recv(pfd.fd, recv_buf, RECV_BUF_SIZE, MSG_DONTWAIT);
		int    i   = 0;
		char*  p   = recv_buf;
		
		if (len <= 0) continue;
		
		while (i < len)
		{
			int  j = strlen(p) + 1;
			
			if (strncmp(p, "ACTION=",	   7) == 0) action	= p + 7;
			else if (strncmp(p, "DEVPATH=",    8) == 0) devpath = p + 8;
			else if (strncmp(p, "SUBSYSTEM=", 10) == 0) subsys	= p + 10;
			else if (strncmp(p, "MAJOR=",	   6) == 0) major = atoi(p + 6);
			else if (strncmp(p, "MINOR=",	   6) == 0) minor = atoi(p + 6);
			
			i += j;
			p += j;
		}
		printf("action:%s subsys:%s devpath:%s\n",action,subsys,devpath);
		start_convert(action, devpath, subsys, major, minor);
		
	}
	
}

int main()
{
	int i;
	for(i=0;i<CONV_SLOT_NUM;i++)
	{
		conv_slot[i].midi_minor=-1;
		conv_slot[i].uart_minor=-1;
	}

	#if SUPPORT_LED_INDICATOR
	system("echo 0 > /sys/class/gpio_sw/PA6/data");			//PA6 indicate USB MIDI<--->UART1 convertion established
	system("echo 0 > /sys/class/gpio_sw/PG11/data");		//PG11 indicate USB MIDI<--->UART2 convertion establishedS
	#endif				
	init_signal();
	monitor_midi_device();
	return 0;
}

