/*
 * cli.h
 *
 *  Created on: Aug 31, 2016
 *      Author: ihsanmert
 */
struct cli;

struct cli_options {
	const char *bindip;
	int port;
};

struct cli * cli_create (struct cli_options *options);
void cli_destroy (struct cli *cli);

int cli_fd (struct cli *cli);
int cli_run (struct cli *cli);

int cli_subscribe (struct cli *cli, const char *event, void (*function) (const char *event, const char *payload, char **result));
int cli_unsubscribe (struct cli *cli, const char *event);
