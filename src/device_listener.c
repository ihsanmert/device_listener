/*
 * device_listener.c
 *
 *  Created on: Aug 31, 2016
 *      Author: ihsanmert
 */

#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <json.h>
#include "cli.h"
#include "device_listener.h"

void push_event(const char *event, const char *payload, char **result);
void dump_all(const char *event, const char *payload, char **result);
void dump_of(const char *event, const char *payload, char **result);
void clear_all(const char *event, const char *payload, char **result);

struct subscription {
	const char *event;
	void (*handler) (const char *event, const char *payload, char **result);
};

struct subscription *cli_subscriptions[] = {
	&(struct subscription) { "cli-push-event", push_event },
	&(struct subscription) { "cli-dump-all", dump_all },
	&(struct subscription) { "cli-dump-of", dump_of },
	&(struct subscription) { "cli-clear-all", clear_all },
	NULL,
};

int subscribe_all_cli_events(struct cli *pstCli)
{
	struct subscription **cli_subscription;
	int rc;
	for (cli_subscription = cli_subscriptions; cli_subscription && *cli_subscription; cli_subscription++) {
		rc = cli_subscribe(pstCli, (*cli_subscription)->event, (*cli_subscription)->handler);
		if (rc != 0) {
			fprintf(stderr,"can not subscribe cli event: %s", (*cli_subscription)->event);
			return -1;
		}
	}
	return 0;
}

void push_event(const char *event, const char *payload, char **result)
{
	struct json_object *json, *value;
	char* device_name = NULL;
	char* message = NULL;
	device_t* device;
	int ret;

	(void) event;
	(void) result;

	json = json_tokener_parse(payload);
	if (json == NULL) {
		return;
	}

	json_object_object_get_ex(json, "device_name",&value);
	if (value != NULL && json_object_is_type(value, json_type_string) ) {
		device_name = (char*) json_object_get_string(value);
	}

	json_object_object_get_ex(json, "message",&value);
	if (value != NULL && json_object_is_type(value, json_type_string) ) {
		message = (char*) json_object_get_string(value);
	}

	if (device_name == NULL || message == NULL) {
		goto bail;
	}

	ret = hashmap_contains(device_map, device_name, (void**)(&device));
	if (ret == MAP_OK) {
		hashmap_get(device_map, device_name, (void**)(&device));
		device->total_message_counter ++;
		device->total_byte_counter += strlen(message);
	}

	if (ret == MAP_MISSING) {
		device = (device_t*) malloc(sizeof(device_t));
		strncpy(device->device_name, device_name, KEY_MAX_LEN-1);
		device->total_byte_counter =  strlen(message);
		device->total_message_counter = 1;
		hashmap_put(device_map, device->device_name, device);
	}

	bail:
	json_object_put(json);
}
void dump_all(const char *event, const char *payload, char **result){
	int i;
	unsigned total_messages = 0;
	unsigned total_bytes = 0;
	/* Cast the hashmap */
	hashmap_map* m = device_map;

	/* On empty hashmap, return immediately */
	if (hashmap_length(m) <= 0)
		return;

	printf("number of all devices: [%d]\n\n", m->size);
	/* Linear probing */
	for(i = 0; i< m->table_size; i++)
		if(m->data[i].in_use != 0) {
			device_t* data = (device_t*) (m->data[i].data);
			printf("device_name: %s, msg_count: %ld, byte_count: %ld\n",data->device_name, data->total_message_counter, data->total_byte_counter);
			total_bytes += data->total_byte_counter;
			total_messages += data->total_message_counter;
		}
	printf("\n# total msg.s = %u\n", total_messages);
	printf("# total bytes = %u\n", total_bytes);
}

void dump_of(const char *event, const char *payload, char **result)
{
	struct json_object *json, *value;
	char* device_name = NULL;
	char* message = NULL;
	device_t* device;
	int ret;

	(void) event;
	(void) result;

	json = json_tokener_parse(payload);
	if (json == NULL) {
		return;
	}

	json_object_object_get_ex(json, "device_name",&value);
	if (value != NULL && json_object_is_type(value, json_type_string) ) {
		device_name = (char*) json_object_get_string(value);
	}

	if (device_name == NULL) {
		goto bail;
	}

	ret = hashmap_contains(device_map, device_name, (void**)(&device));
	if (ret == MAP_OK) {
		hashmap_get(device_map, device_name, (void**)(&device));
		printf("device_name: %s, msg_count: %ld, byte_count: %ld\r\n",device->device_name, device->total_message_counter, device->total_byte_counter);
	}

	if (ret == MAP_MISSING) {
		printf("no such a device with name=[%s]\r\n", device_name);
	}

	bail:
	json_object_put(json);
}

void clear_all(const char *event, const char *payload, char **result)
{
	int i;
	device_t* device;
	char device_name[KEY_MAX_LEN] = {0};
	int ret;

	/* Cast the hashmap */
	hashmap_map* m = device_map;

	/* On empty hashmap, return immediately */
	if (hashmap_length(m) <= 0)
		return;

	printf("number of all devices to clear: [%d]\r\n", m->size);
    /* Free all of the values we allocated and remove them from the map */
	for(i = 0; i< m->table_size; i++)
    {
		if(m->data[i].in_use != 0) {
			device_t* value = (device_t*) (m->data[i].data);
			strncpy(device_name,value->device_name,KEY_MAX_LEN-1);

			ret = hashmap_get(device_map, device_name, (void**)(&value));
			if (ret != MAP_OK) {
				fprintf(stderr, "mapping in hashmap is problematic!\r\n");
				return;
			}

			hashmap_remove(device_map, device_name);
			free(value);
		}
    }
	printf("Cleared all devices information and their counters\r\n");
}

