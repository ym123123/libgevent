/*
 * Future.hpp
 *
 *  Created on: 2017��12��22��
 *      Author: ym
 */

#ifndef FUTURE_HPP_
#define FUTURE_HPP_

#include <ev.h>
#include <set>
#include "context.hpp"
#include <pthread.h>
#include <vector>
#include <sys/eventfd.h>

class Future
{
public:
	Future();
	Future(context *main);
	~Future();

	void add_task(context *task);
	context *get_cur_context();

	static int my_usleep (__useconds_t __useconds);
	static unsigned int my_sleep (unsigned int __seconds);

	static void add_io(int fd, int revents);
private:
	int __usleep (__useconds_t __useconds);
	unsigned int __sleep (unsigned int __seconds);
	void __add_io(int fd, int revents);

private:
	static void *run(void *);
	void run();
	static void context_work(void *data);
	void context_work();

private:
	static void parse_event_fd(EV_P_ ev_io *w, int revents);
	void parse_event();
	static void parse_timer(EV_P_ ev_timer *w, int revents);
	void parse_timer(context *ctx);
	static void parse_io(EV_P_ ev_io *w, int revents);
	void parse_io(context *ctx);

private:
	pthread_t thread_id;
	int event_fd;
	struct ev_loop *loop;
	context *main;
	context *cur;
	pthread_mutex_t mutex;
	std::vector<context * > swap_contexts;
	std::set<context * > contexts;
};

#endif /* FUTURE_HPP_ */
