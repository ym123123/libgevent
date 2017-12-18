/*
 * context.cpp
 *
 *  Created on: 2017Äê12ÔÂ14ÈÕ
 *      Author: ym
 */
#include <iostream>
#include "context.hpp"

using namespace std;

void context::__work(context *ctx, context_fun_t handler, void *data)
{
	handler(data);
	ctx->set_dead();
	ctx->swap_exec();
}

context::context(context_fun_t handler, void *data)
{
	getcontext(&base);
	base.uc_stack.ss_sp = stack;
	base.uc_stack.ss_size = MAX_STACK_LEN;
	makecontext(&base, (void (*)())__work, 3, this, handler, data);
	dead = false;
	in_list_flag = false;
	flag = false;
}

context::context()
{
	getcontext(&base);
	dead = false;
	in_list_flag = false;
	flag = false;
}

context::~context()
{

}


void context::swap_context(context *ctx)
{
	ucontext_t *p = (ucontext_t *)(*ctx);
	p->uc_link = &base;
	swapcontext(&base, p);
}

void context::swap_exec()
{
	ucontext_t *p = base.uc_link;
	swapcontext(&base, p);
}

void context::swap_self()
{
	setcontext(&base);
}

context::operator ucontext_t *()
{
	return &base;
}

bool context::is_dead()
{
	return dead;
}

void context::set_dead()
{
	dead = true;
}

bool context::in_list()
{
	return in_list_flag;
}

void context::set_in_list(bool f)
{
	in_list_flag = f;
}

context::operator ev_io *()
		{
	return &io_base;
		}

context::operator ev_timer*()
		{
	return &timer_base;
		}

bool context::is_dealy()
{
	return flag;
}
void context::set_dealy(bool flag)
{
	this->flag = flag;
}
