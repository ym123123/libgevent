/*
 * context.cpp
 *
 *  Created on: 2017��12��22��
 *      Author: ym
 */

#include <iostream>
#include "context.hpp"

using namespace std;

sig_atomic_t context::num;

void context::__work(context *ctx, context_handler_t handler, void *data)
{
	handler(data);
	ctx->set_dead();
	ctx->swap_exec();
}

context::context(context_handler_t handler, void *data)
{
	getcontext(&base);
	base.uc_stack.ss_sp = stack;
	base.uc_stack.ss_size = MAX_STACK_LEN;
	makecontext(&base, (void (*)())__work,3, this, handler, data);
	cur = num++ % 100;
	dead = false;
}

context::context()
{
	cur = num++ % 100;
	dead = false;
	getcontext(&base);
}

context::~context()
{

}

bool context::is_dead()
{
	return dead;
}
void context::set_dead()
{
	dead = true;
}

ev_timer *context::get_timer()
{
	return &timer_base;
}
ev_io *context::get_io()
{
	return &io_base;
}

ucontext_t *context::get_context()
{
	return &base;
}

void context::swap_context(context *ctx)
{

	ucontext *p = ctx->get_context();
	p->uc_link = &base;
	swapcontext(&base, p);
}
void context::swap_exec()
{
	swapcontext(&base, base.uc_link);
}

void context::swap_self()
{
	setcontext(&base);
}


unsigned int context::get_index()
{
	return cur;
}
