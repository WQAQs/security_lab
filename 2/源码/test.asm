Section .text  
	global _start  
_start:  
	add esp,48
	xor eax, eax  
	push eax  
	push 0x68732f2f  
	push 0x6e69622f  
	mov ebx,esp  
	push eax
	mov ecx, 0xffffffff
	xor ecx, 0xffff9cd2 
	push ecx  
	mov ecx,esp  
	xor edx,edx  
	push edx  
	push 0x7478742e
	push 0x79786c3e
	push 0x3e223830
	push 0x32303035
	push 0x31303337
	push 0x31303222
	push 0x20202020
	push 0x6f686365        
	mov edx,esp  
	push eax  
	push edx  
	push ecx  
	push ebx  
	mov ecx,esp  
	xor edx,edx  
	mov al,0xb  
	int 0x80

