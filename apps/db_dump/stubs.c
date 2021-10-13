#include <stdlib.h>
#include <string.h>

#include "misc_dimq.h"
#include "dimq_broker_internal.h"
#include "dimq_internal.h"
#include "util_dimq.h"

#ifndef UNUSED
#  define UNUSED(A) (void)(A)
#endif

struct dimq *context__init(dimq_sock_t sock)
{
	UNUSED(sock);

	return NULL;
}

int db__message_store(const struct dimq *source, struct dimq_msg_store *stored, uint32_t message_expiry_interval, dbid_t store_id, enum dimq_msg_origin origin)
{
	UNUSED(source);
	UNUSED(stored);
	UNUSED(message_expiry_interval);
	UNUSED(store_id);
	UNUSED(origin);
    return 0;
}

void db__msg_store_ref_inc(struct dimq_msg_store *store)
{
	UNUSED(store);
}

int handle__packet(struct dimq *context)
{
	UNUSED(context);
	return 0;
}

int log__printf(struct dimq *dimq, unsigned int level, const char *fmt, ...)
{
	UNUSED(dimq);
	UNUSED(level);
	UNUSED(fmt);
	return 0;
}

FILE *dimq__fopen(const char *path, const char *mode, bool restrict_read)
{
	UNUSED(path);
	UNUSED(mode);
	UNUSED(restrict_read);
	return NULL;
}

enum dimq_client_state dimq__get_state(struct dimq *dimq)
{
	UNUSED(dimq);
	return dimq_cs_new;
}

int mux__add_out(struct dimq *dimq)
{
	UNUSED(dimq);
	return 0;
}

int mux__remove_out(struct dimq *dimq)
{
	UNUSED(dimq);
	return 0;
}

ssize_t net__read(struct dimq *dimq, void *buf, size_t count)
{
	UNUSED(dimq);
	UNUSED(buf);
	UNUSED(count);
	return 0;
}

ssize_t net__write(struct dimq *dimq, const void *buf, size_t count)
{
	UNUSED(dimq);
	UNUSED(buf);
	UNUSED(count);
	return 0;
}

int retain__store(const char *topic, struct dimq_msg_store *stored, char **split_topics)
{
	UNUSED(topic);
	UNUSED(stored);
	UNUSED(split_topics);
	return 0;
}

int sub__add(struct dimq *context, const char *sub, uint8_t qos, uint32_t identifier, int options, struct dimq__subhier **root)
{
	UNUSED(context);
	UNUSED(sub);
	UNUSED(qos);
	UNUSED(identifier);
	UNUSED(options);
	UNUSED(root);
	return 0;
}

int sub__messages_queue(const char *source_id, const char *topic, uint8_t qos, int retain, struct dimq_msg_store **stored)
{
	UNUSED(source_id);
	UNUSED(topic);
	UNUSED(qos);
	UNUSED(retain);
	UNUSED(stored);
	return 0;
}

int keepalive__update(struct dimq *context)
{
	UNUSED(context);
	return 0;
}
