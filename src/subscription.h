/*
 * subscription.h
 *
 *  Created on: Aug 31, 2016
 *      Author: ihsanmert
 */
struct subscription;
struct subscriptions;

const char * subscription_event (struct subscription *subscription);
void (*subscription_function (struct subscription *subscription)) (const char *event, const char *data, char **result);
void * subscription_context (struct subscription *subscription);

struct subscription * subscriptions_get (struct subscriptions *subscriptions, const char *event);
int subscriptions_del (struct subscriptions *subscriptions, const char *event);
int subscriptions_add (struct subscriptions *subscriptions, const char *event, void (*function) (const char *event, const char *payload, char **result));
void subscriptions_destroy (struct subscriptions *subscriptions);
struct subscriptions * subscriptions_create (void);
