/*
 * Future.cpp
 *
 *  Created on: 2017��12��22��
 *      Author: ym
 */

#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "Future.hpp"
#include "PrivateData.hpp"

using namespace std;
typedef int (* g_pthread_create_t) (pthread_t *__restrict __newthread,
		   const pthread_attr_t *__restrict __attr,
		   void *(*__start_routine) (void *),
		   void *__restrict __arg) ;
extern g_pthread_create_t g_pthread_create;

typedef int (*g_pthread_kill_t) (pthread_t __threadid, int __signo);
extern g_pthread_kill_t g_pthread_kill;

Future::Future()
{
	event_fd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
	pthread_mutex_init(&mutex, NULL);
	loop = ev_loop_new(0);
	cur = NULL;
	main = new context(context_work, this);
	g_pthread_create(&thread_id, NULL, run, this);
	pthread_detach(thread_id);
}

Future::Future(context *ctx)
{
	event_fd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
	pthread_mutex_init(&mutex, NULL);
	loop = ev_loop_new(0);
	cur = NULL;
	add_task(ctx);
	main = new context(context_work, this);

	ctx->swap_context(main);
}

Future::~Future()
{
	g_pthread_kill(thread_id, SIGINT);
	ev_loop_destroy(loop);
	for (vector<context *>::iterator mit = swap_contexts.begin();
			mit != swap_contexts.end(); ++mit)
	{
		delete *mit;
	}

	for (set<context *>::iterator mit = contexts.begin();
			mit != contexts.end(); ++mit)
	{
		delete *mit;
	}

	close(event_fd);
}

void *Future::run(void *arg)
{
	Future *f = static_cast<Future *>(arg);
	f->run();
	return NULL;
}

void Future::run()
{
	cur = NULL;
	main->swap_self();
}

void Future::context_work(void *data)
{
	Future *f = static_cast<Future *>(data);
	f->context_work();
}

void Future::context_work()
{
	PrivateData::getPrivateDataDao()->set_private_data(this);
	ev_io *pio = main->get_io();
	ev_io_init(pio, parse_event_fd, event_fd, EV_READ);
	ev_io_start(loop, pio);
	ev_loop(loop, 0);
}


int Future::my_usleep (__useconds_t __useconds)
{
	Future *f = (Future *)PrivateData::getPrivateDataDao()->get_private_data();
	return f->__usleep(__useconds);
}

unsigned int Future::my_sleep (unsigned int __seconds)
{
	Future *f = (Future *)PrivateData::getPrivateDataDao()->get_private_data();
	return f->__sleep(__seconds);
}

int Future::__usleep (__useconds_t __useconds)
{
	double d = (__useconds + 0.0) / 1000000;
	ev_timer *timer = cur->get_timer();
	timer->data = cur;
	ev_timer_init(timer, parse_timer, d / 1000000, 0);
	ev_timer_start(loop, timer);
	cur->swap_exec();
	return 0;
}

unsigned int Future::__sleep (unsigned int __seconds)
{
	ev_timer *timer = cur->get_timer();
	timer->data = cur;
	ev_timer_init(timer, parse_timer, __seconds, 0);
	ev_timer_start(loop, timer);
	cur->swap_exec();
	return 0;
}


void Future::parse_event_fd(EV_P_ ev_io *w, int revents)
{
	Future *f = (Future *)PrivateData::getPrivateDataDao()->get_private_data();
	f->parse_event();
}

void Future::parse_event()
{
	eventfd_t len;
	eventfd_read(event_fd, &len);
	context *ctx;
	vector<context *>::iterator mit;
	while (true)
	{
		pthread_mutex_lock(&mutex);

		mit = swap_contexts.begin();
		if (mit == swap_contexts.end())
		{
			pthread_mutex_unlock(&mutex);
			break;
		}
		ctx = *mit;
		swap_contexts.erase(mit);
		pthread_mutex_unlock(&mutex);

		cur = ctx;
		main->swap_context(ctx);
		if (ctx->is_dead())
		{
			contexts.erase(ctx);
			delete ctx;
		}

		contexts.insert(ctx);
	}

}


void Future::parse_timer(EV_P_ ev_timer *w, int revents)
{
	Future *f = (Future *)PrivateData::getPrivateDataDao()->get_private_data();
	f->parse_timer((context *)w->data);
}

void Future::parse_timer(context *ctx)
{
	cur = ctx;
	main->swap_context(ctx);

	if (ctx->is_dead())
	{
		contexts.erase(ctx);
		delete ctx;
	}
}

void Future::add_io(int fd, int revents)
{
	Future *f = (Future *)PrivateData::getPrivateDataDao()->get_private_data();
	f->__add_io(fd, revents);
}

void Future::__add_io(int fd, int revents)
{
	ev_io *pio = cur->get_io();
	pio->data = cur;
	ev_io_init(pio, parse_io, fd, revents);
	ev_io_start(loop, pio);
	cur->swap_exec();
}

void Future::parse_io(EV_P_ ev_io *w, int revents)
{
	Future *f = (Future *)PrivateData::getPrivateDataDao()->get_private_data();
	f->parse_io((context *)w->data);
}

void Future::parse_io(context *ctx)
{
	cur = ctx;
	ev_io_stop(loop, ctx->get_io());
	main->swap_context(ctx);

	if (cur->is_dead())
	{
		contexts.erase(cur);
		delete ctx;
	}
}

void Future::add_task(context *task)
{
	eventfd_t len = 1;
	pthread_mutex_lock(&mutex);
	swap_contexts.push_back(task);
	pthread_mutex_unlock(&mutex);
	eventfd_write(event_fd, len);
}

context *Future::get_cur_context()
{
	return cur;
}
