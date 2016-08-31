/*
 * test.c
 *
 *  Created on: Aug 31, 2016
 *      Author: ihsanmert
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>

#ifndef bzero
#define bzero(s,n) (memset((s), '\0', (n)), (void) 0)
#endif
#ifndef bcopy
#define bcopy(src,dest,n) (memmove((dest), (src), (n)), (void) 0)
#endif

static struct option options[] = {
	{"hostname", required_argument, 0, 0x100 },
	{"port"    , required_argument, 0, 0x101},
	{"command" , required_argument, 0, 0x200 },
	{"payload" , required_argument, 0, 0x201 },
	{0         , 0                , 0, 0 }
};

static void print_help (const char *name)
{
	struct option *option;
	fprintf(stdout, "%s options:\n", name);
	for (option = options; option->name != NULL; option++) {
		fprintf(stdout, "  %s\n", option->name);
	}

	fprintf(stdout, "%s --command push-event --payload '{\"device_name\":\"dev1\", \"message\":\"message bla bla to bla bla\" }'\n", name);
	fprintf(stdout, "%s --command dump-all\n", name);
	fprintf(stdout, "%s --command dump-of --payload '{\"device_name\":\"dev1\"}'\n", name);
	fprintf(stdout, "%s --command clear-all\n", name);

}

int main (int argc, char *argv[])
{
	int c;
	int option_index;

	const char *hostname;
	int port;

	const char *command;
	const char *payload;

	const char *prefix;
	const char *seperator;

	int l;
	int s;
	int r;
	int fd;
	struct hostent *server;
	struct sockaddr_in serv_addr;

	int rc;
	char *result;

	result = NULL;

	hostname = "127.0.0.1";
	port = 9999;

	command = NULL;
	payload = "{}";

	prefix = "cli-";
	seperator = ":";

	while (1) {
		c = getopt_long(argc, argv, "h", options, &option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
			case 'h':
				print_help(argv[0]);
				return 0;
			case 0x100:
				hostname = optarg;
				break;
			case 0x101:
				port = atoi(optarg);
				break;
			case 0x200:
				command = optarg;
				break;
			case 0x201:
				payload = optarg;
				break;
		}
	}

	if (command == NULL) {
		fprintf(stderr, "command is null\n");
		return -1;
	}

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "can not open socket\n");
		return -1;
	}

	server = gethostbyname(hostname);
	if (server == NULL) {
		fprintf(stderr,"no such host\n");
		return -1;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	if (connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "can not connect\n");
		return -1;
	}

	int val = 1;
	if (setsockopt(fd, SOL_TCP, TCP_NODELAY, &val, sizeof(val)) != 0)
		goto bail;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) != 0)
		goto bail;
	/* After 10 secs of inactivity, probe 5 times in intervals of 10 secs. Max closure time is 60 secs. */
	val = 5;
	if (setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &val, sizeof(val)) != 0)
		goto bail;
	val = 10;
	if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &val, sizeof(val)) != 0)
		goto bail;
	val = 10;
	if (setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &val, sizeof(val)) != 0)
		goto bail;

	l  = strlen(prefix);
	l += strlen(command);
	l += strlen(seperator);
	l += strlen(payload);
	l = htonl(l);
	s = send(fd, &l, sizeof(l), MSG_NOSIGNAL);
	if (s != sizeof(l)) {
		fprintf(stderr, "can not write to daemon\n");
		return -1;
	}
	s = send(fd, prefix, strlen(prefix), MSG_NOSIGNAL);
	if (s != (int) strlen(prefix)) {
		fprintf(stderr, "can not write to daemon\n");
		return -1;
	}
	s = send(fd, command, strlen(command), MSG_NOSIGNAL);
	if (s != (int) strlen(command)) {
		fprintf(stderr, "can not write to daemon\n");
		return -1;
	}
	s = send(fd, seperator, strlen(seperator), MSG_NOSIGNAL);
	if (s != (int) strlen(seperator)) {
		fprintf(stderr, "can not write to daemon\n");
		return -1;
	}
	s = send(fd, payload, strlen(payload), MSG_NOSIGNAL);
	if (s != (int) strlen(payload)) {
		fprintf(stderr, "can not write to daemon\n");
		return -1;
	}

	s = recv(fd, &rc, sizeof(rc), MSG_NOSIGNAL);
	rc = ntohl(rc);
	if (s != sizeof(rc)) {
		fprintf(stderr, "can not read from daemon\n");
		return -1;
	}
	s = recv(fd, &l, sizeof(l), MSG_NOSIGNAL);
	if (s != sizeof(l)) {
		fprintf(stderr, "can not read from daemon\n");
		return -1;
	}
	l = ntohl(l);
	if (l > 0) {
		result = malloc(l + 1);
		if (result == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			return -1;
		}
		r = 0;
		while (r < l) {
			s = recv(fd, result + r, l - r, MSG_NOSIGNAL);
			if (s <= 0) {
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
	return rc;
	bail:

	if (fd >= 0) {
		close(fd);
	}

	return -1;
}
