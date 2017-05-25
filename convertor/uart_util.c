#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>


int uart_open(char* port)  
{  
	int fd;
	fd = open( port, O_RDWR|O_NOCTTY|O_NDELAY);  
	if (0 == fd)  
	{  
		perror("Can't Open Serial Port");  
		return -1;  
	}  
	             
	if(fcntl(fd, F_SETFL, 0) < 0)  
	{  
		printf("fcntl failed!\n");  
		return -1;  
	}       
	else  
	{  
	
	}
	
	
	//if(0 == isatty(STDIN_FILENO))  
	//{  
	//	printf("standard input is not a terminal device\n");  
	//	return -1;  
	//}  
	//else  
	//{  
	//	
	//}                

 	return fd;  
}  


int uart_set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)
{
   

	int   i;
	int   status;
	struct termios options;
	int   speed_arr[] = { 	B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300};
	int   name_arr[] =  { 	115200, 38400, 19200, 9600, 4800, 2400, 1200, 300};

	if(tcgetattr( fd,&options)!=0)
	{
		perror("tcgetattr fail\n");    
		return -1; 
	}
	bzero(&options,sizeof(options));
	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
	{
		if  (speed == name_arr[i])
		{       
			
			cfsetispeed(&options, speed_arr[i]); 
			cfsetospeed(&options, speed_arr[i]);  
		}
	}     

	
	switch(flow_ctrl)
	{
	case 0 :
	  options.c_cflag &= ~CRTSCTS;
	  break;   
	case 1 :
	  options.c_cflag |= CRTSCTS;
	  break;
	case 2 :
	  options.c_cflag |= IXON | IXOFF | IXANY;
	  break;
	}

	options.c_cflag &= ~CSIZE; 
	
	switch (databits)
	{  
	case 5:
		options.c_cflag |= CS5;
		break;
	case 6:
		options.c_cflag |= CS6;
		break;
	case 7:    
		options.c_cflag |= CS7;
		break;
	case 8:  
		options.c_cflag |= CS8;
		break;  
	default:   
		fprintf(stderr,"Unsupported data size/n");
		return -1; 
	}

	
	switch (parity)
	{  
	case 'n':
	case 'N': 
         options.c_cflag &= ~PARENB; 
         options.c_iflag &= ~INPCK;    
         break; 
	case 'o':  
	case 'O':
         options.c_cflag |= (PARODD | PARENB); 
         options.c_iflag |= (INPCK | ISTRIP);             
         break; 
	case 'e': 
	case 'E':
         options.c_cflag |= PARENB;       
         options.c_cflag &= ~PARODD;       
         options.c_iflag |= (INPCK | ISTRIP);       
         break;
	case 's':
	case 'S':
         options.c_cflag &= ~PARENB;
         options.c_cflag &= ~CSTOPB;
         break; 
	default:  
         fprintf(stderr,"Unsupported parity/n");   
         return -1; 
	} 

	
	switch (stopbits)
	{  
	case 1:   
		options.c_cflag &= ~CSTOPB; 
		break; 
	case 2:   
		options.c_cflag |= CSTOPB; 
		break;
	default:   
		fprintf(stderr,"Unsupported stop bits/n"); 
		return -1;
	}

	
	options.c_cflag |= CLOCAL;
	options.c_cflag |= CREAD;
	options.c_oflag &= ~OPOST;
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 0; 

	
	tcflush(fd,TCIFLUSH);
	if (tcsetattr(fd,TCSANOW,&options) != 0)  
	{
		perror("com set error!/n");  
		return -1; 
	}
	return 0; 
}





int uart_recv(int fd, char *rcv_buf,int data_len)
{
    int len;
	int need_read;
	int total_read;
	int retry;
    fd_set fs_read;
    struct timeval time;
   
    FD_ZERO(&fs_read);
    FD_SET(fd,&fs_read);
   
    time.tv_sec = 1;
    time.tv_usec = 0;

	need_read=data_len;
	total_read=0;
	retry=0;
	while(need_read>0)
	{
		switch(select(fd+1,&fs_read,NULL,NULL,&time))
	    {
	        case -1: 
			{
				printf("recv err\n");
				break;
	    	}
			case 0:
			{
				
				break;
			}
		    default:
			{
				if(FD_ISSET(fd,&fs_read))
		        {
		          len = read(fd,rcv_buf+total_read,need_read);
				  need_read-=len;
				  total_read+=len;
			    }	
	    	}
	       
	    }
		if(need_read<data_len)
		{
			 retry+=1;
			 if(retry==3)
		 	{
		 		//we will only try read 3 times 
		 		printf("read time out\n");
		 		break;
		 	}
		}
		else
		{
			break;
		}
		
	}
	return total_read;
}

int uart_send(int fd, char *send_buf,int data_len)
{
    int len = 0;
    len = write(fd,send_buf,data_len);
    if (len == data_len )
	{
	    return len;
	}     
    else   
	{
		tcflush(fd,TCOFLUSH);
		return 0;
	}
}

