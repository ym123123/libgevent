/*
 * PrivateData.cpp
 *
 *  Created on: 2017Äê12ÔÂ14ÈÕ
 *      Author: ym
 */


#include <iostream>
#include <pthread.h>
#include <cstdlib>

#include "PrivateData.hpp"
using namespace std;

pthread_once_t PrivateData::once;
PrivateData*PrivateData::pdata;
pthread_key_t PrivateData::key;


PrivateData*PrivateData::getPrivateDataDao()
{
	pthread_once(&once, init_private_data);
	return pdata;
}

void PrivateData::init_private_data()
{
	pdata = new PrivateData();

	if (pdata == NULL)
		throw "create private data error.";

	pthread_key_create(&key, des_key);
	atexit(PrivateData::des_private_data);
}

void PrivateData::des_key(void *arg)
{

}

void PrivateData::des_private_data()
{
	delete getPrivateDataDao();
}

void PrivateData::set_private_data(void *data)
{
	pthread_setspecific(key, data);
}

void *PrivateData::get_private_data()
{
	return pthread_getspecific(key);
}
