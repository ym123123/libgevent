//============================================================================
// Name        : libevent.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

using namespace std;

int tmain()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);
	sin.sin_port = htons(8000);
	connect(fd, (struct sockaddr *)&sin, sizeof(sin));
	char buf[100];
	while (1)
	{
		read(fd, buf, 10);
		write(fd, buf,10);
	}
	return 0;
}

int num;
void *thread_work(void *arg)
{
	while (1)
	{

		num++;
		usleep(1000);
	}
}

int main()
{
	pthread_t pid;
	pthread_create(&pid, NULL, thread_work , NULL);
	sleep(10);
	printf("%d\n", num);
	return 0;
}

