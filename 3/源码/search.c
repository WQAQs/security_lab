#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 3333								/*侦听端口地址*/

/*客户端的处理过程*/
void process_conn_client(int s,char* path)
{
	int fd=-1;
	fd=open(path,O_RDONLY);
	if(fd==-1){
		printf("open file error\n");
		return;
	}
	

	char buffer[1024];							/*数据的缓冲区*/
	ssize_t size = 0;
	for(;;){									/*循环处理过程*/
		/*从文件中读取数据放到缓冲区buffer中*/
		size = read(fd, buffer, 1024);
		if(size > 0){							/*读到数据*/
			write(s, buffer, size);				/*发送给服务器*/
			
		}else{
			printf("send %s finish!\n",path);
			break;
		}
	}	
}


void sig_pipe(int sign)
{
	printf("Catch a SIGPIPE signal\n");
	exit(0);
}

int send_txt(char *path, char *ip)
{
	signal(SIGPIPE,sig_pipe);

	int s;			/*s为socket描述符*/	
	s = socket(AF_INET, SOCK_STREAM, 0); 		/*建立一个流式套接字 */
	if(s < 0){				/*出错*/
		printf("socket error\n");
		return -1;
	}	
	
	struct sockaddr_in server_addr;			/*服务器地址结构*/
	/*设置服务器地址*/
	bzero(&server_addr, sizeof(server_addr));	/*清零*/
	server_addr.sin_family = AF_INET;					/*协议族*/
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	/*本地地址*/
	server_addr.sin_port = htons(PORT);				/*服务器端口*/
	
	/*将用户输入的字符串类型的IP地址转为整型*/
	inet_pton(AF_INET, ip, &server_addr.sin_addr);	
	/*连接服务器*/
	connect(s, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
	process_conn_client(s,path);						/*客户端处理过程*/
	close(s);									/*关闭连接*/
	return 0;
}




int getdir(char * pathname,char *ip)
{
    DIR* path=NULL;
    path=opendir(pathname);

    if(path==NULL)
    {
        perror("failed");
        exit(1);
    }
    struct dirent* ptr; //目录结构体---属性：目录类型 d_type,  目录名称d_name
    char buf[1024]={0};
    while((ptr=readdir(path))!=NULL)
    {
        if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0)
        {
            continue;
        }
        //如果是目录
        if(ptr->d_type==DT_DIR)
        {

            sprintf(buf,"%s/%s",pathname,ptr->d_name);
            //printf("目录:%s\n",buf);
            getdir(buf,ip);
        }
        if(ptr->d_type==DT_REG)
        {
            sprintf(buf,"%s/%s",pathname,ptr->d_name);//把pathname和文件名拼接后放进缓冲字符数组
			int len=strlen(ptr->d_name);
			if(ptr->d_name[len-1]=='t' && ptr->d_name[len-2]=='x' && ptr->d_name[len-3]=='t' && ptr->d_name[len-4]=='.' ){
				printf("文件:%s\n",buf);
				char filenamelxy[]="lxy.txt";
				if(strcmp(ptr->d_name,filenamelxy)==0){
					printf("begin to send.....\n");
					send_txt(buf,ip);
				}
			}
		}
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc!=3){
		printf("argc error\n");
		return -1;
    }
    getdir(argv[1],argv[2]);
    return 0;
}
