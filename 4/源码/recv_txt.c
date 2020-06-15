#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "rsa.h"
#include "rc4.h"
#include "CRC16.h"


#define PORT 3333						/*侦听端口地址*/
#define BACKLOG 2						/*侦听队列长度*/




/*服务器对客户端的处理*/
void process_conn_server(int s)
{
	/*****协商会话密钥******/
	int size=0;
	long long *encrypted = malloc(sizeof(long long)*128);
	size = read(s,encrypted,1024);
/*	printf("Encrypted:\n");
	for(int i=0; i < 128; i++){
		printf("%lld ", (long long)encrypted[i]);
	}  
*/
	struct private_key_class priv[1];
	priv[0].modulus=2439149849;
	priv[0].exponent=1698774401;
	char *decrypted = rsa_decrypt(encrypted, 1024, priv);
  	if (!decrypted){
    		printf("Error in decryption!\n");
    		return;
  	}

	
	char message[128]={0};
	memcpy(message,decrypted,128);
	free(decrypted);
	free(encrypted);
/*  	printf("Decrypted:\n");
	for(int i=0; i < 128; i++){
    		printf("%02x ",(unsigned char)message[i]);
 	}	  
*/
	/*****接收文件名******/
	char filename_en[1024]={0};
	size = read(s, filename_en, 1024);
	char filename[1024]={0};
	rc4_encrypt(filename_en, filename, message, strlen(filename_en));


	int fd=-1;
	fd=open(filename,O_WRONLY|O_CREAT|O_APPEND,S_IRWXU);
	if(fd==-1){
		printf("open file error\n");
		return;
	}
	/*****接收文件内容******/
	char buffer[1024];	/*数据的缓冲区*/
	char buffer_de[1024];	/*数据的缓冲区*/
	for(;;){	/*循环处理过程*/		
		memset(buffer,0,1024);
		size = read(s, buffer, 1024);	/*从套接字中读取数据到缓冲区buffer中*/
		size = strlen(buffer);
		if(size == 0){			/*没有数据*/
			printf("%s:receive finish!\n",filename);
			return;	
		}

		
		memset(buffer_de,0,1024);
		rc4_encrypt(buffer, buffer_de, message, size);		//解密

    		int result = CalCrc(0, buffer_de, size-1) & 0xffff;	//校验完整性
		if(result!=0){
			printf("crc error!\n");
			close(fd);
			return;
		}
		write(fd,buffer_de,size-3);
	}	
	
}

void sig_int(int sign)
{
	int status;
	wait(&status);
	exit(0);
}

int main(int argc, char *argv[])
{
	int ss,sc;		/*ss为服务器的socket描述符，sc为客户端的socket描述符*/
	signal(SIGINT,sig_int);
	struct sockaddr_in server_addr;	/*服务器地址结构*/
	struct sockaddr_in client_addr;	/*客户端地址结构*/
	int err;							/*返回值*/
	pid_t pid;							/*分叉的进行ID*/

	/*建立一个流式套接字*/
	ss = socket(AF_INET, SOCK_STREAM, 0);
	if(ss < 0){							/*出错*/
		printf("socket error\n");
		return -1;	
	}
	
	/*设置服务器地址*/
	bzero(&server_addr, sizeof(server_addr));			/*清零*/
	server_addr.sin_family = AF_INET;					/*协议族*/
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	/*本地地址*/
	server_addr.sin_port = htons(PORT);				/*服务器端口*/
	
	/*绑定地址结构到套接字描述符*/
	err = bind(ss, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(err < 0){/*出错*/
		printf("bind error\n");
		return -1;	
	}
	
	/*设置侦听*/
	err = listen(ss, BACKLOG);
	if(err < 0){				/*出错*/
		printf("listen error\n");
		return -1;	
	}
	
		/*主循环过程*/
	for(;;)	{
		socklen_t addrlen = sizeof(struct sockaddr);
		
		sc = accept(ss, (struct sockaddr*)&client_addr, &addrlen); 
		/*接收客户端连接*/
		if(sc < 0){							/*出错*/
			continue;						/*结束本次循环*/
		}	

		
		/*建立一个新的进程处理到来的连接*/
		pid = fork();						/*分叉进程*/
		if( pid == 0 ){						/*子进程中*/
			process_conn_server(sc);	/*处理连接*/
			close(ss);			/*在子进程中关闭服务器的侦听*/
		}else{
			close(sc);			/*在父进程中关闭客户端的连接*/
		}
	}
}



