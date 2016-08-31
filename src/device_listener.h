/*
 * device_listener.h
 *
 *  Created on: Aug 30, 2016
 *      Author: mert
 */
#include "hashmap.h"
#ifndef DEVICE_LISTENER_H_
#define DEVICE_LISTENER_H_

#define KEY_MAX_LEN 256

typedef struct device_s
{
	char device_name[KEY_MAX_LEN];
	unsigned long  total_message_counter;
	unsigned long  total_byte_counter;
} device_t;

/*device map based on hash map
 * to keep device info as
 * key-value pair of (device name) - (its counters)*/
map_t device_map;

#endif /* DEVICE_LISTENER_H_ */
