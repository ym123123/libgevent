/*
 * context.hpp
 *
 *  Created on: 2017��12��22��
 *      Author: ym
 */

#ifndef CONTEXT_HPP_
#define CONTEXT_HPP_
#include <ev.h>
#include <signal.h>
#include <ucontext.h>

typedef void (* context_handler_t)(void *data);
#define MAX_STACK_LEN	(1<<20)

class context
{
public:
	context(context_handler_t handler, void *data);
	context();
	~context();
public:
	ev_timer *get_timer();
	ev_io *get_io();
	ucontext_t *get_context();
	void swap_context(context *ctx);
	void swap_exec();
	void swap_self();
	bool is_dead();
	void set_dead();
	unsigned int get_index();
private:
	static void __work(context *ctx, context_handler_t handler, void *data);
private:
	ucontext_t base;
	unsigned char stack[MAX_STACK_LEN];
	ev_timer timer_base;
	ev_io io_base;
	int cur;
	bool dead;
private:
	static sig_atomic_t num;
};

#endif /* CONTEXT_HPP_ */
