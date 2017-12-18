/*
 * IOLoop.hpp
 *
 *  Created on: 2017��12��14��
 *      Author: ym
 */

#ifndef IOLOOP_HPP_
#define IOLOOP_HPP_

#include <set>
#include <pthread.h>
#include <vector>
#include <ev.h>
#include "Future.hpp"

class IOLoop
{
public:
	static int numCpu;
	static int blockCpu;
	static IOLoop *getIOLoopDao();

public:
	void add_sleep(unsigned int ts);
	void add_usleep(unsigned int ts);
	void add_io(int fd, int events);

private:
	IOLoop();
	~IOLoop();

	static void create_loop();
	static void destroy_loop();

	static void run(void *data);

	void run_work();

	static void sched_timer_context(struct ev_loop *evs, ev_timer *timer, int events);
	static void sched_io_context(struct ev_loop *evs, ev_io *io, int events);
	static void sched_elist_context(struct ev_loop *evs, ev_io *io, int events);
private:
	static IOLoop*loop;
	static pthread_once_t once;
	context *main;
	context *base;
	static struct ev_loop *evs;
	pthread_mutex_t mutex;
	std::set<int> fds;
	std::vector<Future * >futures;
};

#endif /* IOLOOP_HPP_ */
