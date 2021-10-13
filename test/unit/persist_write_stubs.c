#include <time.h>

#define WITH_BROKER

#include <logging_dimq.h>
#include <memory_dimq.h>
#include <dimq_broker_internal.h>
#include <net_dimq.h>
#include <send_dimq.h>
#include <time_dimq.h>

extern uint64_t last_retained;
extern char *last_sub;
extern int last_qos;

struct dimq *context__init(dimq_sock_t sock)
{
	UNUSED(sock);

	return dimq__calloc(1, sizeof(struct dimq));
}

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

int net__socket_close(struct dimq *dimq)
{
	UNUSED(dimq);

	return dimq_ERR_SUCCESS;
}

int send__pingreq(struct dimq *dimq)
{
	UNUSED(dimq);

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

int acl__find_acls(struct dimq *context)
{
	UNUSED(context);

	return dimq_ERR_SUCCESS;
}


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
