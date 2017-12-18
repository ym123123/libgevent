/*
 * Future.hpp
 *
 *  Created on: 2017Äê12ÔÂ14ÈÕ
 *      Author: ym
 */

#ifndef FUTURE_HPP_
#define FUTURE_HPP_

#include "context.hpp"

class Future
{
public:
	Future();
	~Future();

	void run_work();
private:
	static void* run(void *f);
	static void context_work(void *arg);
	void context_work();
	void thread_work();
private:
	pthread_t thread_id;
	context *main;
};

#endif /* FUTURE_HPP_ */
