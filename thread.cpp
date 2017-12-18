/*
 * thread.cpp
 *
 *  Created on: 2017��12��13��
 *      Author: ym
 */

#include <pthread.h>
#include <dlfcn.h>
#include "IOLoop.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>
#include "PrivateData.hpp"
#include "EventList.hpp"
#include <errno.h>
#include <set>

using namespace std;

set<int> sets;

g_pthread_create_t g_pthread_create = (g_pthread_create_t)dlsym(RTLD_NEXT, "pthread_create");
g_pthread_kill_t g_pthread_kill = (g_pthread_kill_t)dlsym(RTLD_NEXT, "pthread_kill");

static g_sleep_t g_sleep = (g_sleep_t)dlsym(RTLD_NEXT, "sleep");
static g_usleep_t g_usleep = (g_usleep_t)dlsym(RTLD_NEXT, "usleep");
static g_read_t g_read = (g_read_t)dlsym(RTLD_NEXT, "read");
static g_write_t g_write = (g_write_t)dlsym(RTLD_NEXT, "write");
static g_close_t g_close = (g_close_t)dlsym(RTLD_NEXT, "close");
static g_socket_t g_socket = (g_socket_t)dlsym(RTLD_NEXT, "socket");
static g_accept_t g_accept = (g_accept_t)dlsym(RTLD_NEXT, "accept");

unsigned int sleep (unsigned int __seconds)
{
	IOLoop *loop = IOLoop::getIOLoopDao();
	loop->add_sleep(__seconds);
	context *ctx = (context *)PrivateData::getPrivateDataDao()->get_private_data();
	ctx->swap_exec();
	return 0;
}

int usleep (__useconds_t __useconds)
{
	IOLoop *loop = IOLoop::getIOLoopDao();
	loop->add_usleep(__useconds);
	context *ctx = (context *)PrivateData::getPrivateDataDao()->get_private_data();
	ctx->swap_exec();
	return 0;
}

typedef void *(*thread_work_t)(void *);

int pthread_create (pthread_t *__restrict __newthread,
		   const pthread_attr_t *__restrict __attr,
		   thread_work_t fun,
		   void *__restrict __arg)
{
	usleep(1);
	EventList*elist = EventList::getEventDao();
	context *ctx = new context((void (*)(void *))fun, __arg);
	*__newthread = (char *)ctx - (char *)NULL;

	elist->set_event(ctx);
	return 0;
}


int pthread_kill (pthread_t __threadid, int __signo)
{
	context *ctx = (context *)((char *)NULL + (size_t)(__threadid));
	ctx->set_dead();
	return 0;
}

int socket (int __domain, int __type, int __protocol)
{
	if (__type == SOCK_STREAM)
	{
		int fd = g_socket(__domain, __type, __protocol);
		fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
		sets.insert(fd);
		return fd;
	}

	return g_socket(__domain, __type, __protocol);
}

int close (int __fd)
{
	sets.erase(__fd);
	return g_close(__fd);
}

ssize_t read (int __fd, void *__buf, size_t __nbytes)
{
	size_t len;

	if (sets.find(__fd) != sets.end())
	{
		len = 0;
		ssize_t ret = 0;

		while (__nbytes - len)
		{
		label:
			ret = g_read(__fd, (char *)__buf + len, __nbytes - len);
			if ((ret <= 0))
			{
				if (ret == 0)
					return len;

				if (errno == EAGAIN || errno == EINTR)
				{
					IOLoop::getIOLoopDao()->add_io(__fd, LOOP_READ);
					((context *)PrivateData::getPrivateDataDao()->get_private_data())->swap_exec();
					goto label;
				}

				return ret;
			}
			len += ret;

			if (len != __nbytes)
			{
				IOLoop::getIOLoopDao()->add_io(__fd, LOOP_READ);
				((context *)PrivateData::getPrivateDataDao()->get_private_data())->swap_exec();
			}
		}

		return __nbytes;
	}
	else
	{
		return g_read(__fd, __buf, __nbytes);
	}
}

ssize_t write (int __fd, const void *__buf, size_t __n)
{
	size_t len;
	if (sets.find(__fd) != sets.end())
	{
		len = 0;
		ssize_t ret = 0;

		while (__n - len)
		{
		label:
			ret = g_write(__fd, (const char *)__buf + len, __n - len);

			if ((ret <= 0))
			{
				if (ret == 0)
					return len;
				if (errno == EAGAIN || errno == EINTR)
				{
					IOLoop::getIOLoopDao()->add_io(__fd, LOOP_WRITE);
					((context *)PrivateData::getPrivateDataDao()->get_private_data())->swap_exec();
					goto label;
				}

				return ret;
			}
			len += ret;

			if (len != __n)
			{
				IOLoop::getIOLoopDao()->add_io(__fd, LOOP_WRITE);
				((context *)PrivateData::getPrivateDataDao()->get_private_data())->swap_exec();
			}
		}

		return __n;
	}
	else
	{
		return g_write(__fd, __buf, __n);
	}
}

int accept (int __fd, __SOCKADDR_ARG __addr,
		   socklen_t *__restrict __addr_len)
{
	if (sets.find(__fd) == sets.end())
	{
		return g_accept(__fd, __addr, __addr_len);
	}
label:
	int cfd = g_accept(__fd, __addr, __addr_len);

	if ((cfd == -1))
	{
		if (errno == EAGAIN || errno == EINTR)
		{
			IOLoop::getIOLoopDao()->add_io(__fd, LOOP_READ);
			((context *)PrivateData::getPrivateDataDao()->get_private_data())->swap_exec();
			goto label;
		}

		return cfd;
	}

	fcntl(cfd, F_SETFL, fcntl(cfd, F_GETFL)|O_NONBLOCK);
	sets.insert(cfd);
	return cfd;
}
