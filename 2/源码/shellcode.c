#include <unistd.h>
int main(){
    char *argv[][4] = { {"/bin/sh", "-c  ", "echo \"2019202210045\">>sunxu.txt", 0},}; 
    execve(argv[0][0],&argv[0][0],NULL);
    perror();  
}  