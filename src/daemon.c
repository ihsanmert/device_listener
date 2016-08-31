/*
 * daemon.c
 *
 *  Created on: Aug 31, 2016
 *      Author: ihsanmert
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "device_listener.h"

struct cli *pstCli;
#define DEFAULT_PORT 9999

static struct option options[] = {
	{"cli-bindip"                , required_argument, 0, 0x100 },
	{"port"                      , required_argument, 0, 0x101 },
	{0                           , 0                , 0, 0 }
};

int main(int argc, char *argv[])
{
	char *cli_bind_ip = NULL;
	int port = DEFAULT_PORT;
	int c;
	int option_index;
	struct option *option;
	struct cli_options cli_options;

	while (1) {
		c = getopt_long(argc, argv, "h", options, &option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
			case 'h':
				fprintf(stdout, "%s help:\n", argv[0]);
				fprintf(stdout, "It's a kind of broker (messaging server) to collect messages among devices\n"
								 "and to count number of messages received from a specific device or all devices\n");
				fprintf(stdout, "%s options:\n", argv[0]);
				for (option = options; option->name != NULL; option++) {
					fprintf(stdout, "  %s\n", option->name);
				}
				return 0;
			case 0x100: /*bind ip. If it is not specified, use INADDR_ANY for binding*/
				cli_bind_ip = optarg;
				break;
			case 0x101: /*port*/
				port = atoi(optarg);
				break;
		}
	}

	cli_options.bindip = cli_bind_ip;
	cli_options.port = port;

	pstCli = cli_create(&cli_options);
	if (pstCli == NULL) {
		fprintf(stderr,"cli cannot be created\r\n");
		return EXIT_FAILURE;
	}

	subscribe_all_cli_events(pstCli);

	device_map = hashmap_new();
	if (device_map == NULL) {
		fprintf(stderr,"device map cannot be created\r\n");
		return EXIT_FAILURE;
	}

	while(1){
		/*Polling all incoming messages from devices and process if there exists any messages*/
		if(cli_run(pstCli) == -1){/*To process */
			fprintf(stderr,"cli run failed, try after 1 sec.\r\n");
			sleep(1);
		}
	}

	return EXIT_SUCCESS;
}
