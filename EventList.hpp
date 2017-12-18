/*
 * EventList.hpp
 *
 *  Created on: 2017��12��14��
 *      Author: ym
 */

#ifndef EVENTLIST_HPP_
#define EVENTLIST_HPP_

#include <pthread.h>
#include <list>
#include "context.hpp"

class EventList
{
public:
	context *get_event();
	void set_event(context *);
	context*get_ret_event();
	void set_ret_event(context *);

	void add_timer_event(context *ctx);
	void add_io_event(context *ctx);

	context *get_timer_event();
	context *get_io_event();
	//��ȡfd
	operator int();

public:
	static EventList *getEventDao();

private:
	EventList();
	~EventList();

private:
	static void initEventList();
	static void desEventList();

private:
	static EventList *evList;
	static pthread_once_t once;

private:
	int evfd;
	pthread_mutex_t event_mutex;
	pthread_cond_t cond;
	pthread_mutex_t cond_mutex;
	std::list<context *> events;
	std::list<context *>ret_events;
	std::list<context *> timers;
	std::list<context *> ios;
};


#endif /* EVENTLIST_HPP_ */
