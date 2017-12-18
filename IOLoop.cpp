/*
 * IOLoop.cpp
 *
 *  Created on: 2017��12��14��
 *      Author: ym
 */

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/eventfd.h>
#include "IOLoop.hpp"
#include "EventList.hpp"
#include "PrivateData.hpp"


using namespace std;

int IOLoop::numCpu = 0;
int IOLoop::blockCpu = 1;
pthread_once_t IOLoop::once = PTHREAD_ONCE_INIT;
IOLoop * IOLoop::loop;
struct ev_loop *IOLoop::evs;

IOLoop* IOLoop::getIOLoopDao()
{
	pthread_once(&once, IOLoop::create_loop);
	return loop;
}

void IOLoop::create_loop()
{
	new IOLoop();
	atexit(IOLoop::destroy_loop);
}

void IOLoop::destroy_loop()
{
	delete getIOLoopDao();
	loop = NULL;
}

void IOLoop::sched_timer_context(struct ev_loop *evs, ev_timer *timer, int events)
{
	context *ctx = (context *)timer->data;

	if (ctx->in_list())
	{
		ctx->set_dealy(true);
		return;
	}

	ctx->set_in_list(true);
	EventList::getEventDao()->set_event(ctx);
}

void IOLoop::sched_io_context(struct ev_loop *evs, ev_io *io, int events)
{
	context *ctx = (context *)io->data;
	ev_io_stop(evs, io);

	if (ctx->in_list())
	{
		ctx->set_dealy(true);
		return;
	}

	ctx->set_in_list(true);
	EventList::getEventDao()->set_event(ctx);
}

void IOLoop::sched_elist_context(struct ev_loop *evs, ev_io *io, int events)
{
	eventfd_t len;
	context *ctx;
	EventList *elist = EventList::getEventDao();

	eventfd_read(io->fd, &len);

	while (true)
	{
		ctx = elist->get_ret_event();

		if (ctx == NULL)
			break;

		if (ctx->is_dead())
		{
			delete ctx;
			continue;
		}

		if (ctx->is_dealy())
		{
			ctx->set_dealy(false);
			elist->set_event(ctx);
		}

		ctx->set_in_list(false);
	}

	while (true)
	{
		ctx = elist->get_timer_event();
		if (ctx == NULL)
			break;

		ev_timer_start(evs, (ev_timer *)(*ctx));
	}

	while (true)
	{
		ctx = elist->get_io_event();

		if (ctx == NULL)
			break;

		ev_io_start(evs, (ev_io *)(*ctx));
	}
}

void IOLoop::run_work()
{
	ev_io io;
	EventList *ev = EventList::getEventDao();
	ev_timer *timer = (ev_timer *)(*main);
	timer->data = main;
	ev_timer_init(timer, sched_timer_context, 0.001, 0);
	ev_timer_start(evs, timer);

	ev_io_init(&io, sched_elist_context, int(*ev), EV_READ);
	ev_io_start(evs, &io);

	ev_loop(evs, 0);
}

void IOLoop::run(void *data)
{
	IOLoop *loop = static_cast<IOLoop * >(data);
	loop->run_work();
}

IOLoop::IOLoop()
{
	loop = this;
	pthread_mutex_init(&mutex, NULL);
	evs = ev_default_loop(0);

	if (numCpu == 0)
	{
		numCpu = sysconf(_SC_NPROCESSORS_CONF);
	}
	for (int i = 0; i < numCpu; i++)
	{
		Future *f = new Future();
		f->run_work();
		futures.push_back(f);
	}


	main = new context();
	base = new context(run, this);
	base->swap_self();
}

IOLoop::~IOLoop()
{
	for (vector<Future * >::iterator mit = futures.begin();
			mit != futures.end(); ++mit)
	{
		delete *mit;
	}

	pthread_mutex_destroy(&mutex);
	delete main;
	delete base;
}


void IOLoop::add_sleep(unsigned int ts)
{
	if (ts == 0)
		return;

	context *ctx = (context *)PrivateData::getPrivateDataDao()->get_private_data();
	ev_timer *timer = (ev_timer *)(*ctx);
	timer->data = ctx;

	ev_timer_init(timer, sched_timer_context, ts, 0);

	EventList::getEventDao()->add_timer_event(ctx);
}

void IOLoop::add_usleep(unsigned int ts)
{
	if (ts == 0)
		return;

	context *ctx = (context *)PrivateData::getPrivateDataDao()->get_private_data();
	ev_timer *timer = (ev_timer *)(*ctx);
	timer->data = ctx;
	double d = ((ts - 1) + 1.0) / 1000000;

	ev_timer_init(timer, sched_timer_context, d, 0);
	EventList::getEventDao()->add_timer_event(ctx);
}

void IOLoop::add_io(int fd, int events)
{
	context *ctx = (context *)PrivateData::getPrivateDataDao()->get_private_data();
	ev_io *io = (ev_io *)(*ctx);

	io->data = ctx;


	ev_io_init(io, sched_io_context, fd, ((events)?EV_WRITE:EV_READ));
	EventList::getEventDao()->add_io_event(ctx);
}
