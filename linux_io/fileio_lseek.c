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
	int fd=open("iotest.txt",O_RDWR | O_CREAT,0777);
	if(fd<0)
		{
			perror("open error");
			return -1;
		}
	
	
	int lenth=	lseek(fd,0,SEEK_END);
	printf("[%d]\n",lenth);
	close(fd);
	return 0;
}
