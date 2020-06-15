#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 8888								/*侦听端口地址*/

/*客户端的处理过程*/
void process_conn_client(int s)
{	
	ssize_t size = 0;
	char buffer[1024];							/*数据的缓冲区*/

	for(;;){									/*循环处理过程*/
		/*从命令行中读取数据放到缓冲区buffer中*/
		memset(buffer,0,1024);
		size = read(0, buffer, 1024);
		if(size > 0){							/*读到数据*/
			write(s, buffer, size);				/*发送给服务器*/
			
			memset(buffer,0,1024);
			size = read(s, buffer, 1024);		/*从服务器读取数据*/
			printf("%s\n",buffer);				/*写到标准输出*/
			
		}else{
			printf("no data\n");
			break;
		}
	}	
}

void sig_pipe(int sign)
{
	printf("Catch a SIGPIPE signal\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int s;			/*s为socket描述符*/
	
	signal(SIGPIPE,sig_pipe);

	if(argc!=2){
		printf("argc error!");
		return -1;
	}


	
	struct sockaddr_in server_addr;			/*服务器地址结构*/
	
	s = socket(AF_INET, SOCK_STREAM, 0); 		/*建立一个流式套接字 */
	if(s < 0){									/*出错*/
		printf("socket error\n");
		return -1;
	}	
	
	/*设置服务器地址*/
	bzero(&server_addr, sizeof(server_addr));	/*清零*/
	server_addr.sin_family = AF_INET;					/*协议族*/
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	/*本地地址*/
	server_addr.sin_port = htons(PORT);				/*服务器端口*/
	
	/*将用户输入的字符串类型的IP地址转为整型*/
	inet_pton(AF_INET, argv[1], &server_addr.sin_addr);	
	/*连接服务器*/
	connect(s, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
	process_conn_client(s);						/*客户端处理过程*/
	close(s);									/*关闭连接*/
	return 0;
}

