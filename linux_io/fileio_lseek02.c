#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

//iotest	open close read write lseek
int main()
{
	int fd=open("iotest02.txt",O_RDWR | O_CREAT,0777);
	if(fd<0)
		{
			perror("open error");
			return -1;
		}
char cb[10]="a";	
	int lenth=	lseek(fd,100,SEEK_END);
	write(fd,cb,sizeof(cb));	
close(fd);
	return 0;
}
