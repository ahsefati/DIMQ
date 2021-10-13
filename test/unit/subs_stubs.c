#include <time.h>

#define WITH_BROKER

#include <logging_dimq.h>
#include <memory_dimq.h>
#include <dimq_broker_internal.h>
#include <net_dimq.h>
#include <send_dimq.h>
#include <time_dimq.h>

#if 0
extern uint64_t last_retained;
extern char *last_sub;
extern int last_qos;

struct dimq *context__init(dimq_sock_t sock)
{
	return dimq__calloc(1, sizeof(struct dimq));
}


int db__message_insert(struct dimq *context, uint16_t mid, enum dimq_msg_direction dir, uint8_t qos, bool retain, struct dimq_msg_store *stored, dimq_property *properties)
{
	return dimq_ERR_SUCCESS;
}

void db__msg_store_ref_dec(struct dimq_msg_store **store)
{
}

void db__msg_store_ref_inc(struct dimq_msg_store *store)
{
	store->ref_count++;
}
#endif

int log__printf(struct dimq *dimq, unsigned int priority, const char *fmt, ...)
{
	UNUSED(dimq);
	UNUSED(priority);
	UNUSED(fmt);

	return 0;
}

time_t dimq_time(void)
{
	return 123;
}

#if 0
int net__socket_close(struct dimq *dimq)
{
	return dimq_ERR_SUCCESS;
}

int send__pingreq(struct dimq *dimq)
{
	return dimq_ERR_SUCCESS;
}

int dimq_acl_check(struct dimq *context, const char *topic, uint32_tn payloadlen, void* payload, uint8_t qos, bool retain, int access)
{
	return dimq_ERR_SUCCESS;
}

int acl__find_acls(struct dimq *context)
{
	return dimq_ERR_SUCCESS;
}
#endif


int send__publish(struct dimq *dimq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, uint8_t qos, bool retain, bool dup, const dimq_property *cmsg_props, const dimq_property *store_props, uint32_t expiry_interval)
{
	UNUSED(dimq);
	UNUSED(mid);
	UNUSED(topic);
	UNUSED(payloadlen);
	UNUSED(payload);
	UNUSED(qos);
	UNUSED(retain);
	UNUSED(dup);
	UNUSED(cmsg_props);
	UNUSED(store_props);
	UNUSED(expiry_interval);

	return dimq_ERR_SUCCESS;
}

int send__pubcomp(struct dimq *dimq, uint16_t mid, const dimq_property *properties)
{
	UNUSED(dimq);
	UNUSED(mid);
	UNUSED(properties);

	return dimq_ERR_SUCCESS;
}

int send__pubrec(struct dimq *dimq, uint16_t mid, uint8_t reason_code, const dimq_property *properties)
{
	UNUSED(dimq);
	UNUSED(mid);
	UNUSED(reason_code);
	UNUSED(properties);

	return dimq_ERR_SUCCESS;
}

int send__pubrel(struct dimq *dimq, uint16_t mid, const dimq_property *properties)
{
	UNUSED(dimq);
	UNUSED(mid);
	UNUSED(properties);

	return dimq_ERR_SUCCESS;
}

int dimq_acl_check(struct dimq *context, const char *topic, uint32_t payloadlen, void* payload, uint8_t qos, bool retain, int access)
{
	UNUSED(context);
	UNUSED(topic);
	UNUSED(payloadlen);
	UNUSED(payload);
	UNUSED(qos);
	UNUSED(retain);
	UNUSED(access);

	return dimq_ERR_SUCCESS;
}

uint16_t dimq__mid_generate(struct dimq *dimq)
{
	static uint16_t mid = 1;

	UNUSED(dimq);

	return ++mid;
}

int dimq_property_add_varint(dimq_property **proplist, int identifier, uint32_t value)
{
	UNUSED(proplist);
	UNUSED(identifier);
	UNUSED(value);

	return dimq_ERR_SUCCESS;
}

int persist__backup(bool shutdown)
{
	UNUSED(shutdown);

	return dimq_ERR_SUCCESS;
}

int persist__restore(void)
{
	return dimq_ERR_SUCCESS;
}

void dimq_property_free_all(dimq_property **properties)
{
	UNUSED(properties);
}

int retain__init(void)
{
	return dimq_ERR_SUCCESS;
}

void retain__clean(struct dimq__retainhier **retainhier)
{
	UNUSED(retainhier);
}

int retain__queue(struct dimq *context, const char *sub, uint8_t sub_qos, uint32_t subscription_identifier)
{
	UNUSED(context);
	UNUSED(sub);
	UNUSED(sub_qos);
	UNUSED(subscription_identifier);

	return dimq_ERR_SUCCESS;
}

int retain__store(const char *topic, struct dimq_msg_store *stored, char **split_topics)
{
	UNUSED(topic);
	UNUSED(stored);
	UNUSED(split_topics);

	return dimq_ERR_SUCCESS;
}


void util__decrement_receive_quota(struct dimq *dimq)
{
	if(dimq->msgs_in.inflight_quota > 0){
		dimq->msgs_in.inflight_quota--;
	}
}

void util__decrement_send_quota(struct dimq *dimq)
{
	if(dimq->msgs_out.inflight_quota > 0){
		dimq->msgs_out.inflight_quota--;
	}
}


void util__increment_receive_quota(struct dimq *dimq)
{
	dimq->msgs_in.inflight_quota++;
}

void util__increment_send_quota(struct dimq *dimq)
{
	dimq->msgs_out.inflight_quota++;
}
