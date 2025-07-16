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
	
	write(fd,"hello",strlen("hello"));
	
	lseek(fd,0,SEEK_SET);

	char cb[1024];
	memset(cb,0x00,sizeof(cb));
	int n=read(fd,cb,sizeof(cb));
	printf("n==[%d],buf==[%s]\n",n,cb);
	close(fd);
	return 0;
}
