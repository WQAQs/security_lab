#include <unistd.h>
int main(){
    char *argv[][4] = { {"/bin/sh", "-c  ", "echo \"2017301500186\">>wx.txt", 0},}; 
    execve(argv[0][0],&argv[0][0],NULL);
    perror();  
}  