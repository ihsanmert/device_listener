/*
 * subscription.c
 *
 *  Created on: Aug 31, 2016
 *      Author: ihsanmert
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/queue.h>

#include "subscription.h"

#if !defined(TAILQ_FOREACH_SAFE)
#define	TAILQ_FOREACH_SAFE(var, head, field, next)		\
	for ((var) = ((head)->tqh_first);			\
		(var) && ((next) = ((var)->field.tqe_next), 1);	\
		(var) = (next))
#endif

struct subscription {
	TAILQ_ENTRY(subscription) subscriptions;
	char *event;
	void (*function) (const char *event, const char *payload, char **result);
};

struct subscriptions {
	unsigned int count;
	TAILQ_HEAD(subscriptionq, subscription) subscriptions;
};

const char * subscription_event (struct subscription *subscription)
{
	if (subscription == NULL) {
		fprintf(stdout,"subscription is invalid");
		goto bail;
	}
	return subscription->event;
bail:	return NULL;
}

void (*subscription_function (struct subscription *subscription)) (const char *event, const char *data, char **result)
{
	if (subscription == NULL) {
		fprintf(stdout,"subscription is invalid");
		goto bail;
	}
	return subscription->function;
bail:	return NULL;
}

static void subscription_destroy (struct subscription *subscription)
{
	if (subscription == NULL) {
		return;
	}
	if (subscription->event != NULL) {
		free(subscription->event);
	}
	free(subscription);
}

static struct subscription * subscription_create (const char *event, void (*function) (const char *event, const char *payload, char **result))
{
	struct subscription *subscription;
	subscription = NULL;
	if (event == NULL) {
		fprintf(stdout,"event is invalid");
		goto bail;
	}
	if (function == NULL) {
		fprintf(stdout,"function is invalid");
		goto bail;
	}
	subscription = malloc(sizeof(struct subscription));
	if (subscription == NULL) {
		fprintf(stdout,"can not allocate memory");
		goto bail;
	}
	memset(subscription, 0, sizeof(struct subscription));
	subscription->event = strdup(event);
	if (subscription->event == NULL) {
		fprintf(stdout,"can not allocate memory");
		goto bail;
	}
	subscription->function = function;
	return subscription;
bail:	if (subscription != NULL) {
		subscription_destroy(subscription);
	}
	return NULL;
}

struct subscription * subscriptions_get (struct subscriptions *subscriptions, const char *event)
{
	struct subscription *subscription;
	if (subscriptions == NULL) {
		fprintf(stdout,"subscriptions is invalid");
		goto bail;
	}
	if (event == NULL) {
		fprintf(stdout,"event is invalid");
		goto bail;
	}
	TAILQ_FOREACH(subscription, &subscriptions->subscriptions, subscriptions) {
		if (strcasecmp(event, subscription_event(subscription)) == 0) {
			return subscription;
		}
	}
bail:	return NULL;
}

int subscriptions_del (struct subscriptions *subscriptions, const char *event)
{
	struct subscription *subscription;
	if (subscriptions == NULL) {
		fprintf(stdout,"subscriptions is invalid");
		goto bail;
	}
	if (event == NULL) {
		fprintf(stdout,"event is invalid");
		goto bail;
	}
	subscription = subscriptions_get(subscriptions, event);
	if (subscription == NULL) {
		goto out;
	}
	TAILQ_REMOVE(&subscriptions->subscriptions, subscription, subscriptions);
	subscriptions->count -= 1;
	subscription_destroy(subscription);
out:	return 0;
bail:	return -1;
}

int subscriptions_add (struct subscriptions *subscriptions, const char *event, void (*function) (const char *event, const char *payload, char **result))
{
	struct subscription *subscription;
	if (subscriptions == NULL) {
		fprintf(stdout,"subscriptions is invalid");
		goto bail;
	}
	if (event == NULL) {
		fprintf(stdout,"event is invalid");
		goto bail;
	}
	subscription = subscriptions_get(subscriptions, event);
	if (subscription != NULL) {
		fprintf(stdout,"subscription already exists");
		goto bail;
	}
	subscription = subscription_create(event, function);
	if (subscription == NULL) {
		fprintf(stdout,"can not create subscription");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&subscriptions->subscriptions, subscription, subscriptions);
	subscriptions->count += 1;
	return 0;
bail:	return -1;
}

void subscriptions_destroy (struct subscriptions *subscriptions)
{
	struct subscription *subscription;
	struct subscription *nsubscription;
	if (subscriptions == NULL) {
		return;
	}
	TAILQ_FOREACH_SAFE(subscription, &subscriptions->subscriptions, subscriptions, nsubscription) {
		TAILQ_REMOVE(&subscriptions->subscriptions, subscription, subscriptions);
		subscriptions->count -= 1;
		subscription_destroy(subscription);
	}
	free(subscriptions);
}

struct subscriptions * subscriptions_create (void)
{
	struct subscriptions *subscriptions;
	subscriptions = malloc(sizeof(struct subscriptions));
	if (subscriptions == NULL) {
		fprintf(stdout,"can not allocate memory");
		goto bail;
	}
	memset(subscriptions, 0, sizeof(struct subscriptions));
	TAILQ_INIT(&subscriptions->subscriptions);
	subscriptions->count = 0;
	return subscriptions;
bail:
	if (subscriptions != NULL) {
		subscriptions_destroy(subscriptions);
	}
	return NULL;
}
