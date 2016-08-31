/*
 * client_api.h
 *
 *  Created on: Jan 15, 2016
 *      Author: ihsanmert
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

extern int push_message(char* server_ip, int port, const char* log_bulk_data);


