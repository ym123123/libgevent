/*
 * Manager.hpp
 *
 *  Created on: 2017��12��22��
 *      Author: ym
 */

#ifndef MANAGER_HPP_
#define MANAGER_HPP_

#include <vector>
#include <list>
#include "Future.hpp"

typedef struct
{
	context *c;
	union
	{
		const void *write_data;
		void *read_data;
	}un;
	int fd;
	size_t size;
	short flag;
	short ret;
#define wdata un.write_data
#define rdata un.read_data
} task_t;

class Manager
{
public:
	static pthread_mutex_t mutex;
	static pthread_cond_t cond;
	static std::list<task_t *> tasks;

	static void push_task(task_t *task);
	static void *parse_task(void *data);

	static void add_context(context *ctx);
	static std::vector<Future *> futures;
	static std::vector<pthread_t> threads;
	static pthread_once_t once;
	static int numCpu;
	static void once_handler();
};

#endif /* MANAGER_HPP_ */
