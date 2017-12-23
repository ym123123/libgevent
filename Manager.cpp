/*
 * Manager.cpp
 *
 *  Created on: 2017��12��22��
 *      Author: ym
 */

#include <dlfcn.h>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>
#include "PrivateData.hpp"
#include <errno.h>
#include <set>
#include "Manager.hpp"

using namespace std;

typedef int (* g_pthread_create_t) (pthread_t *__restrict __newthread,
		   const pthread_attr_t *__restrict __attr,
		   void *(*__start_routine) (void *),
		   void *__restrict __arg) ;

typedef int (*g_pthread_kill_t) (pthread_t __threadid, int __signo);
typedef unsigned int (*g_sleep_t) (unsigned int __seconds);
typedef int (*g_usleep_t) (__useconds_t __useconds);
typedef ssize_t (*g_read_t) (int __fd, void *__buf, size_t __nbytes);
typedef ssize_t (*g_write_t) (int __fd, const void *__buf, size_t __n);
typedef int (*g_socket_t) (int __domain, int __type, int __protocol);
typedef int (*g_close_t) (int __fd);

typedef int (*g_accept_t) (int __fd, __SOCKADDR_ARG __addr,
		   socklen_t *__restrict __addr_len);
typedef int (*g_connect_t) (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);
g_pthread_create_t g_pthread_create = (g_pthread_create_t)dlsym(RTLD_NEXT, "pthread_create");
g_pthread_kill_t g_pthread_kill = (g_pthread_kill_t)dlsym(RTLD_NEXT, "pthread_kill");
static g_connect_t g_connect = (g_connect_t)dlsym(RTLD_NEXT, "connect");
static g_sleep_t g_sleep = (g_sleep_t)dlsym(RTLD_NEXT, "sleep");
static g_usleep_t g_usleep = (g_usleep_t)dlsym(RTLD_NEXT, "usleep");
static g_read_t g_read = (g_read_t)dlsym(RTLD_NEXT, "read");
static g_write_t g_write = (g_write_t)dlsym(RTLD_NEXT, "write");
static g_close_t g_close = (g_close_t)dlsym(RTLD_NEXT, "close");
static g_socket_t g_socket = (g_socket_t)dlsym(RTLD_NEXT, "socket");
static g_accept_t g_accept = (g_accept_t)dlsym(RTLD_NEXT, "accept");


set<int> sets;

pthread_once_t Manager::once = PTHREAD_ONCE_INIT;
pthread_mutex_t Manager::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Manager::cond = PTHREAD_COND_INITIALIZER;
list<task_t *> Manager::tasks;
vector<Future *>Manager::futures;
vector<pthread_t> Manager::threads;
int Manager::numCpu = 0;

void Manager::once_handler()
{
	pthread_t pid;
	Future *f;
	int i = 0;

	if (numCpu == 0)
		numCpu = sysconf(_SC_NPROCESSORS_CONF);

	f = new Future(new context());
	futures.push_back(f);
	for (i = 0; i < numCpu - 1; i++)
	{
		f = new Future();
		futures.push_back(f);
	}

	numCpu += numCpu;

	for (i = 0; i < numCpu; i++)
	{
		g_pthread_create(&pid, NULL, parse_task, NULL);
		threads.push_back(pid);
	}
}

void Manager::add_context(context *ctx)
{
	int index = ctx->get_index() % futures.size();
	printf("index = %d\n", index);
	futures[index]->add_task(ctx);

}

void Manager::push_task(task_t *task)
{
	pthread_mutex_lock(&mutex);
	tasks.push_back(task);
	if (tasks.size() == 1)
		pthread_cond_signal(&cond);

	pthread_mutex_unlock(&mutex);

}

int readn(int fd, char *buf, size_t n)
{
	int len;
	int ret;
	while (n - len)
	{
		ret = g_read(fd, buf + len, n - len);

		switch (ret)
		{
		case 0:
			return 1;
			break;
		case -1:
			if (errno == EINTR||errno == EAGAIN)
				continue;
			return 1;
			break;
		default:
			len += ret;
			break;
		}
	}

	return 0;
}

int writen(int fd, const char *buf, size_t n)
{
	int len;
	int ret;
	while (n - len)
	{
		ret = g_write(fd, buf + len, n - len);

		switch (ret)
		{
		case 0:
			return 1;
			break;
		case -1:
			if (errno == EINTR||errno == EAGAIN)
				continue;
			return 1;
			break;
		default:
			len += ret;
			break;
		}
	}

	return 0;
}

void *Manager::parse_task(void *data)
{
	task_t *task;
	list<task_t *>::iterator mit;

	while (true)
	{
		pthread_mutex_lock(&mutex);

		while (tasks.size() == 0)
			pthread_cond_wait(&cond, &mutex);

		mit = tasks.begin();
		task = *mit;
		tasks.erase(mit);

		pthread_mutex_unlock(&mutex);

		if (task->flag == 0)
		{
			task->ret = readn(task->fd, (char *)task->rdata, task->size);
		}
		else
		{
			task->ret = writen(task->fd, (const char *)task->wdata, task->size);
		}
		add_context(task->c);
	}

	return NULL;
}

unsigned int sleep (unsigned int __seconds)
{
	pthread_once(&Manager::once, Manager::once_handler);
	return Future::my_sleep(__seconds);
}

int usleep (__useconds_t __useconds)
{
	pthread_once(&Manager::once, Manager::once_handler);
	return Future::my_usleep(__useconds);
}

typedef void *(*thread_work_t)(void *);

int pthread_create (pthread_t *__restrict __newthread,
		   const pthread_attr_t *__restrict __attr,
		   thread_work_t fun,
		   void *__restrict __arg)
{
	pthread_once(&Manager::once, Manager::once_handler);
	context *ctx = new context((context_handler_t)fun, __arg);
	*__newthread = (char *)ctx - (char *)NULL;
	Manager::add_context(ctx);
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
	pthread_once(&Manager::once, Manager::once_handler);
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
	ssize_t ret = 0;
	if (sets.find(__fd) != sets.end())
	{

	label:
		ret = g_read(__fd, (char *)__buf, __nbytes);
		if (ret >= 0)
		{
			return ret;
		}

		if (errno == EAGAIN || errno == EINTR)
		{
			Future::add_io(__fd, EV_READ);
			goto label;
		}

		return ret;
	}
	else
	{
		task_t task;
		Future *f = (Future *)PrivateData::getPrivateDataDao()->get_private_data();
		task.c = f->get_cur_context();
		task.rdata = __buf;
		task.flag = 0;
		task.size = __nbytes;
		task.fd = __fd;
		Manager::push_task(&task);
		task.c->swap_exec();

		if (task.ret != 0)
			return -1;
		return __nbytes;
	}
}

ssize_t write (int __fd, const void *__buf, size_t __n)
{
	size_t len;
	if (sets.find(__fd) != sets.end())
	{
	label:
		len = g_write(__fd, (char *)__buf, __n);
		if (len >= 0)
			return len;

		if (errno == EAGAIN || errno == EINTR)
		{
			//防止出错
			Future::add_io(__fd, EV_READ|EV_WRITE);
			goto label;
		}

		return len;
	}
	else
	{
		task_t task;
		Future *f = (Future *)PrivateData::getPrivateDataDao()->get_private_data();
		task.c = f->get_cur_context();
		task.wdata = __buf;
		task.flag = 1;
		task.size = __n;
		task.fd = __fd;
		Manager::push_task(&task);
		task.c->swap_exec();

		if (task.ret != 0)
			return -1;
		return __n;
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

	if (cfd < 0)
	{
		if (errno == EINTR|| errno == EAGAIN)
		{
			Future::add_io(__fd, EV_READ);
			goto label;
		}

		return cfd;
	}

	fcntl(cfd, F_SETFL, fcntl(cfd, F_GETFL)|O_NONBLOCK);
	sets.insert(cfd);
	return cfd;
}


int connect (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
	if (sets.find(__fd) == sets.end())
	{
		return g_connect(__fd, __addr, __len);
	}
	else
	{
		g_connect(__fd, __addr, __len);
		return 0;
	}
}
