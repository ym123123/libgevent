/*
 * Future.cpp
 *
 *  Created on: 2017��12��13��
 *      Author: ym
 */

#include <iostream>
#include "EventList.hpp"
#include "Future.hpp"
#include "PrivateData.hpp"
#include <signal.h>
using namespace std;

extern g_pthread_create_t g_pthread_create;
extern g_pthread_kill_t g_pthread_kill;

Future::Future()
{
	main = NULL;
	thread_id = 0;
}

Future::~Future()
{
	g_pthread_kill(thread_id, SIGINT);
	if (main)
		delete main;
}

void Future::run_work()
{
	g_pthread_create(&thread_id, NULL, run, this);
	pthread_detach(thread_id);
}

void *Future::run(void *arg)
{
	Future *f = static_cast<Future *>(arg);
	f->thread_work();

	return NULL;
}

void Future::context_work(void *arg)
{
	Future *f = static_cast<Future *>(arg);
	f->context_work();
}

void Future::context_work()
{
	PrivateData *pdata = PrivateData::getPrivateDataDao();
	EventList *evList = EventList::getEventDao();
	context *ctx;

	while (true)
	{
		ctx = evList->get_event();
		pdata->set_private_data(ctx);
		main->swap_context(ctx);
		evList->set_ret_event(ctx);
	}
}

void Future::thread_work()
{
	main = new context(context_work, this);
	main->swap_self();
}
