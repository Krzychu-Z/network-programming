#include <unistd.h>
#include <stdio.h>

main(){
char line[10];
while(1){
scanf ("%[^\n]%*c", line);
printf("i:%s\n",line);
}
}
