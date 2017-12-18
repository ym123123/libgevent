//============================================================================
// Name        : gevent.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <iostream>
#include "IOLoop.hpp"

using namespace std;

void *__work(void *arg)
{
	char buf[1<<10];
	int fd = (char *)arg - (char *)NULL;

	while (true)
	{
		if (read(fd, buf, 10) != 10 || write(fd, buf, 10) != 10)
		{
			close(fd);
			return NULL;
		}
		usleep(1);
	}
}

int tmain(int argc,char *argv[])
{
	struct sockaddr_in sin;
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(8080);
	sin.sin_addr.s_addr = htonl(0);

	bind(fd, (struct sockaddr *)&sin, sizeof(sin));
	listen(fd, SOMAXCONN);
	pthread_t pid;

	while (1)
	{
		int cfd = accept(fd, NULL, NULL);
		pthread_create(&pid, NULL, __work, (char *)NULL + cfd);
	}
}

void *work(void *arg)
{
	while (true)
	{
		usleep(1);
		cout << "hello world" << endl;
	}
}

int main()
{
	IOLoop::numCpu = 1;
	pthread_t pid;
	pthread_create(&pid, NULL, work, NULL);
	pthread_create(&pid, NULL, work, NULL);
	sleep(10);
	cout << "main" << endl;
}
