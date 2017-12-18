/*
 * EventList.cpp
 *
 *  Created on: 2017��12��14��
 *      Author: ym
 */

#include <iostream>
#include <sys/eventfd.h>
#include "EventList.hpp"
#include <unistd.h>
#include <cstdlib>
#include <list>
#include <pthread.h>
using namespace std;

pthread_once_t EventList::once;
EventList *EventList::evList;


void EventList::initEventList()
{
	evList = new EventList();
	atexit(EventList::desEventList);
}

void EventList::desEventList()
{
	if (evList)
	{
		delete evList;
		evList = NULL;
	}
}


EventList *EventList::getEventDao()
{
	pthread_once(&once, EventList::initEventList);
	return evList;
}


EventList::EventList()
{
	evfd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
	if (evfd < 0)
	{
		throw "create eventfd error.";
	}

	pthread_cond_init(&cond, NULL);
	pthread_mutex_init(&cond_mutex, NULL);
	pthread_mutex_init(&event_mutex, NULL);

	events.clear();
	ret_events.clear();
}


EventList::~EventList()
{
	close(evfd);
	events.clear();
	ret_events.clear();

	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&event_mutex);
	pthread_mutex_destroy(&cond_mutex);
}


context* EventList::get_event()
{
	context *context;
	pthread_mutex_lock(&cond_mutex);

	while (events.size() == 0)
		pthread_cond_wait(&cond, &cond_mutex);
	context = *(events.begin());
	events.pop_front();
	pthread_mutex_unlock(&cond_mutex);

	return context;
}


void EventList::set_event(context *ctx)
{
	pthread_mutex_lock(&cond_mutex);

	events.push_back(ctx);

	if (events.size() <= 1)
		pthread_cond_broadcast(&cond);

	pthread_mutex_unlock(&cond_mutex);
}


context *EventList::get_ret_event()
{
	list<context*>::iterator mit;
	context *ctx = NULL;
	pthread_mutex_lock(&event_mutex);

	mit = ret_events.begin();
	if (mit != ret_events.end())
	{
		ctx = *mit;
		ret_events.erase(mit);
	}

	pthread_mutex_unlock(&event_mutex);

	return ctx;
}


void EventList::set_ret_event(context *ctx)
{
	eventfd_t num = 1;
	pthread_mutex_lock(&event_mutex);
	ret_events.push_back(ctx);
	pthread_mutex_unlock(&event_mutex);
	eventfd_write(evfd, num);
}

void EventList::add_timer_event(context *ctx)
{
	eventfd_t num = 1;
	pthread_mutex_lock(&event_mutex);
	timers.push_back(ctx);
	pthread_mutex_unlock(&event_mutex);
	eventfd_write(evfd, num);
}
void EventList::add_io_event(context *ctx)
{
	eventfd_t num = 1;
	pthread_mutex_lock(&event_mutex);
	ios.push_back(ctx);
	pthread_mutex_unlock(&event_mutex);
	eventfd_write(evfd, num);
}

context *EventList::get_timer_event()
{
	list<context*>::iterator mit;
	context *ctx = NULL;
	pthread_mutex_lock(&event_mutex);

	mit = timers.begin();
	if (mit != timers.end())
	{
		ctx = *mit;
		timers.erase(mit);
	}

	pthread_mutex_unlock(&event_mutex);

	return ctx;
}

context *EventList::get_io_event()
{
	list<context*>::iterator mit;
	context *ctx = NULL;
	pthread_mutex_lock(&event_mutex);

	mit = ios.begin();
	if (mit != ios.end())
	{
		ctx = *mit;
		ios.erase(mit);
	}

	pthread_mutex_unlock(&event_mutex);

	return ctx;
}

EventList::operator int()
		{
	return evfd;
		}
