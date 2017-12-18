/*
 * context.hpp
 *
 *  Created on: 2017��12��14��
 *      Author: ym
 */

#ifndef CONTEXT_HPP_
#define CONTEXT_HPP_
#include <ucontext.h>
#include <ev++.h>
#include <sys/socket.h>
#define MAX_STACK_LEN	(1<<20)

typedef int (* g_pthread_create_t) (pthread_t *__restrict __newthread,
		   const pthread_attr_t *__restrict __attr,
		   void *(*__start_routine) (void *),
		   void *__restrict __arg) ;

typedef int (*g_pthread_kill_t) (pthread_t __threadid, int __signo);
typedef unsigned int (*g_sleep_t) (unsigned int __seconds);
typedef int (*g_usleep_t) (__useconds_t __useconds);
typedef ssize_t (*g_read_t) (int __fd, void *__buf, size_t __nbytes);
typedef ssize_t (*g_write_t) (int __fd, const void *__buf, size_t __n);
typedef int (*g_socket_t) (int __domain, int __type, int __protocol);
typedef int (*g_close_t) (int __fd);

typedef int (*g_accept_t) (int __fd, __SOCKADDR_ARG __addr,
		   socklen_t *__restrict __addr_len);

#define LOOP_READ 0
#define LOOP_WRITE 1

class context
{
public:
	typedef void (*context_fun_t)(void *data);

	context(context_fun_t handler, void *data);
	context();
	virtual ~context();
public:
	void swap_context(context *ctx);
	void swap_exec();
	void swap_self();

	bool is_dead();
	void set_dead();

	bool in_list();
	void set_in_list(bool flag);

	bool is_dealy();
	void set_dealy(bool flag);

	static void __work(context *ctx, context_fun_t handler, void *data);
public:
	operator ucontext_t *();
	operator ev_io*();
	operator ev_timer*();

private:
	unsigned char stack[MAX_STACK_LEN];
	ucontext_t base;
	ev_io io_base;
	ev_timer timer_base;
	bool dead;
	bool in_list_flag;
	bool flag;
};

#endif /* CONTEXT_HPP_ */
