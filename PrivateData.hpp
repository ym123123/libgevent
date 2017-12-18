/*
 * PrivateData.hpp
 *
 *  Created on: 2017Äê12ÔÂ14ÈÕ
 *      Author: ym
 */

#ifndef PRIVATEDATA_HPP_
#define PRIVATEDATA_HPP_

#include <pthread.h>

class PrivateData
{
public:
	static PrivateData *getPrivateDataDao();
	void *get_private_data();
	void set_private_data(void *data);
private:
	static void des_key(void *);

	static void init_private_data();
	static void des_private_data();
	static pthread_key_t key;
	static pthread_once_t once;
	static PrivateData *pdata;
};

#endif /* PRIVATEDATA_HPP_ */
