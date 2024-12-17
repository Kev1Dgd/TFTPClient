#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    char print_buffer[30];
    if (argc != 3) {
        sprintf(print_buffer, "Usage: gettftp HOST FILE\n", argv[0]);
        write(STDOUT_FILENO, print_buffer, strlen(print_buffer));
        return 1;
    } 


    
       
    return 0;
}