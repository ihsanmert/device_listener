/*
 * cli.c
 *
 *  Created on: Aug 31, 2016
 *      Author: ihsanmert
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <poll.h>

#include "subscription.h"
#include "cli.h"

struct cli {
	struct cli_options *options;
	struct subscriptions *subscriptions;
	int fd;
};

int cli_fd (struct cli *cli)
{
	if (cli == NULL) {
		fprintf(stderr,"cli is invalid");
		return -1;
	}
	return cli->fd;
}


int cli_run (struct cli *cli)
{
	int rc;
	struct pollfd pollfd;

	int fd;
	socklen_t clilen;
	struct sockaddr_in cli_addr;

	int r;
	int w;
	int len;
	char *buffer;

	char *event;
	char *payload;
	char *result;
	struct subscription *subscription;

again:
	fd = -1;
	buffer = NULL;
	result = NULL;

	if (cli == NULL) {
		fprintf(stderr,"cli is null");
		goto bail;
	}

	pollfd.fd = cli->fd;
	pollfd.events = POLLIN;
	pollfd.revents = 0;

	rc = poll(&pollfd, 1, 1);
	if (rc < 0) {
		fprintf(stderr,"poll failed with: %d", rc);
		goto bail;
	}
	if (rc == 0) {
		goto skip;
	}
	if ((pollfd.revents & POLLIN) == 0) {
		goto skip;
	}

	clilen = sizeof(cli_addr);
	fd = accept(cli->fd, (struct sockaddr *) &cli_addr, &clilen);
	if (fd < 0) {
		fprintf(stderr,"can not accept client");
		goto bail;
	}

	rc = recv(fd, &len, sizeof(len), MSG_NOSIGNAL);
	if (rc != sizeof(len)) {
		fprintf(stderr,"can not read from client");
		goto bail;
	}
	len = ntohl(len); // to guarantee that it works well in different-endianness platforms
	buffer = malloc(len + 1);
	if (buffer == NULL) {
		fprintf(stderr,"can not allocate memory");
		goto bail;
	}

	//printf("waiting for %d bytes", len);
	r = 0;
	while (r < len) {
		rc = recv(fd, buffer + r, len - r, MSG_NOSIGNAL);
		if (rc <= 0) {
			fprintf(stderr,"can not read from client");
			goto bail;
		}
		r += rc;
	}
	buffer[len] = '\0';

	event = buffer;
	payload = strstr(buffer, ":");
	if (payload == NULL) {
		fprintf(stderr,"not an cli event");
		goto skip;
	}
	*payload = '\0';
	payload = strstr(payload + 1, "{");
	if (payload == NULL) {
		fprintf(stderr,"not an cli event");
		goto skip;
	}

//	printf("search subscription for: '%s'\r\n", event);
	subscription = subscriptions_get(cli->subscriptions, event);
	if (subscription != NULL) {
//		printf("found subscription for: '%s'\r\n", event);
		subscription_function(subscription)(event, payload, &result);
	} else {
		printf("could not find subscription for: '%s'\r\n", event);
	}

	/*The below part is to send response to devices
	 * if there is such a specification */
	r = 0;
	r = htonl(r);
	rc = send(fd, &r, sizeof(r), MSG_NOSIGNAL);
	if (rc != sizeof(r)) {
		fprintf(stderr,"can not send to client");
		goto bail;
	}
	if (result != NULL) {
		len = strlen(result);
	} else {
		len = 0;
	}
	len = htonl(len);
	rc = send(fd, &len, sizeof(len), MSG_NOSIGNAL);
	if (rc != sizeof(len)) {
		fprintf(stderr,"can not send to client");
		goto bail;
	}
	len = ntohl(len);
	w = 0;
	while (w < len) {
		rc = send(fd, result + w, len - w, MSG_NOSIGNAL);
		if (rc <= 0) {
			fprintf(stderr,"can not read from client");
			goto bail;
		}
		w += rc;
	}

	if (fd >= 0) {
		close(fd);
	}
	if (buffer != NULL) {
		free(buffer);
	}
	if (result != NULL) {
		free(result);
	}
	goto again;

skip:	if (fd >= 0) {
		close(fd);
	}
	if (buffer != NULL) {
		free(buffer);
	}
	if (result != NULL) {
		free(result);
	}
	return 0;
bail:	if (fd >= 0) {
		close(fd);
	}
	if (buffer != NULL) {
		free(buffer);
	}
	if (result != NULL) {
		free(result);
	}
	return -1;
}

struct cli * cli_create (struct cli_options *options)
{
	int rc;
	struct sockaddr_in serv_addr;

	struct cli *cli;

	cli = malloc(sizeof(struct cli));
	if (cli == NULL) {
		fprintf(stderr,"can not allocate memory");
		goto bail;
	}
	memset(cli, 0, sizeof(struct cli));
	cli->fd = -1;

	cli->options = options;
	cli->subscriptions = subscriptions_create();
	if (cli->subscriptions == NULL) {
		fprintf(stderr,"can not create subscriptions");
		goto bail;
	}

	cli->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (cli->fd < 0) {
		fprintf(stderr,"can not open socket");
		goto bail;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	if (options->bindip == NULL) {
		serv_addr.sin_addr.s_addr = INADDR_ANY;
	} else {
		if (!inet_aton(options->bindip, &serv_addr.sin_addr)) {
			fprintf(stderr,"can not convert IP address: %s", options->bindip);
			goto bail;
		}
	}
	serv_addr.sin_port = htons(options->port);

	int yes = 1;
	setsockopt(cli->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	rc = bind(cli->fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (rc < 0) {
		fprintf(stderr,"can not bind cli");
		goto bail;
	}
	listen(cli->fd, 5);

	return cli;
bail:	if (cli != NULL) {
		cli_destroy(cli);
	}
	return NULL;
}

void cli_destroy (struct cli *cli)
{
	if (cli == NULL) {
		return;
	}
	if (cli->subscriptions != NULL) {
		subscriptions_destroy(cli->subscriptions);
	}
	if (cli->fd >= 0) {
		close(cli->fd);
	}
	free(cli);
}

int cli_subscribe (struct cli *cli, const char *event, void (*function) (const char *event, const char *payload, char **result))
{
	if (cli == NULL) {
		fprintf(stderr,"cli is invalid");
		goto bail;
	}
	if (event == NULL) {
		fprintf(stderr,"event is invalid");
		goto bail;
	}
	if (function == NULL) {
		fprintf(stderr,"function is invalid");
		goto bail;
	}
	return subscriptions_add(cli->subscriptions, event, function);
bail:	return -1;
}

int cli_unsubscribe (struct cli *cli, const char *event)
{
	if (cli == NULL) {
		fprintf(stderr,"cli is invalid");
		goto bail;
	}
	if (event == NULL) {
		fprintf(stderr,"event is invalid");
		goto bail;
	}
	return subscriptions_del(cli->subscriptions, event);
bail:	return -1;
}
