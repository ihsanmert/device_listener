/*
 * device_api.c
 *
 *  Created on: Jan 15, 2016
 *      Author: ihsanmert
 */
#include <string.h>

#define CMD_PUSH_EVT "push-event"


#define MAX_PAYLOAD_FIX_PART 64

#ifndef bzero
#define bzero(s,n) (memset((s), '\0', (n)), (void) 0)
#endif
#ifndef bcopy
#define bcopy(src,dest,n) (memmove((dest), (src), (n)), (void) 0)
#endif

#include "device_api.h"

#define HOST_NAME "127.0.0.1"
#define PORT 9999
const char *prefix = "cli-";
const char *seperator = ":";

int push_message(char* server_ip, int port, const char* log_bulk_data)
{
	char *payload;
	int rv;
	int str_bulk_len;

	str_bulk_len = strlen(log_bulk_data);

	payload = malloc( str_bulk_len + MAX_PAYLOAD_FIX_PART );

	if (payload == NULL) {
		//printf("can not allocate memory\n");
		return -1;
	}

	snprintf(payload,str_bulk_len + MAX_PAYLOAD_FIX_PART ,"'{\"data\":\"%s\"}'",log_bulk_data);

	rv = complete_send(server_ip, port, CMD_PUSH_EVT,payload);
	free(payload);

	return rv;
}


int send_data(int fd,char *command,const char *payload)
{

	int l;
	int s;
	int r;
	int rc;

	l  = strlen(prefix);
	l += strlen(command);
	l += strlen(seperator);
	l += strlen(payload);

	char *result = NULL;
	l = htonl(l);
	s = send(fd, &l, sizeof(l), MSG_NOSIGNAL);
	if (s != sizeof(l)) {
		fprintf(stderr, "can not write to daemon\n");
		close(fd);
		return -1;
	}
	s = send(fd, prefix, strlen(prefix), MSG_NOSIGNAL);
	if (s != (int) strlen(prefix)) {
		fprintf(stderr, "can not write to daemon\n");
		close(fd);
		return -1;
	}
	s = send(fd, command, strlen(command), MSG_NOSIGNAL);
	if (s != (int) strlen(command)) {
		fprintf(stderr, "can not write to daemon\n");
		close(fd);
		return -1;
	}
	s = send(fd, seperator, strlen(seperator), MSG_NOSIGNAL);
	if (s != (int) strlen(seperator)) {
		fprintf(stderr, "can not write to daemon\n");
		close(fd);
		return -1;
	}
	s = send(fd, payload, strlen(payload), MSG_NOSIGNAL);
	if (s != (int) strlen(payload)) {
		fprintf(stderr, "can not write to daemon\n");
		close(fd);
		return -1;
	}

	s = recv(fd, &rc, sizeof(rc), MSG_NOSIGNAL);
	rc = ntohl(rc);
	if (s != sizeof(rc)) {
		fprintf(stderr, "can not read from daemon\n");
		close(fd);
		return -1;
	}
	s = recv(fd, &l, sizeof(l), MSG_NOSIGNAL);
	l = ntohl(l);
	if (s != sizeof(l)) {
		fprintf(stderr, "can not read from daemon\n");
		close(fd);
		return -1;
	}
	if (l > 0) {
		result = malloc(l + 1);
		if (result == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			close(fd);
			return -1;
		}
		r = 0;
		while (r < l) {
			s = recv(fd, result + r, l - r, MSG_NOSIGNAL);
			if (s <= 0) {
				free(result);
				close(fd);
				fprintf(stderr, "can not read from client\n");
				return -1;
			}
			r += s;
		}
		result[r] = '\0';
		fprintf(stdout, "%s", result);
	}

	close(fd);

	if (result != NULL) {
		free(result);
	}

	return 1;
}

int device_connect(char* srv_hostname,int srv_port){

	int fd;

	struct hostent *server;
	struct sockaddr_in serv_addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "can not open socket\n");
		return -1;
	}

	server = gethostbyname(srv_hostname);
	if (server == NULL) {
		close(fd);
		fprintf(stderr,"no such host\n");
		return -1;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(srv_port);

	if (connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "can not connect to external remote-monitor-daemon\n");
		close(fd);
		return -1;
	}

	return fd;

}

int complete_send(char* server_ip, int port, char *command,char *payload)
{
	int fd;

	if (port == -1) port = PORT;
	if (server_ip == NULL)
		fd = device_connect(HOST_NAME,port);
	else
		fd = device_connect(server_ip,port);

	if (fd == -1) return fd;

	return send_data(fd,command,payload);
}
