#include <time.h>

#define WITH_BROKER

#include <logging_dimq.h>
#include <memory_dimq.h>
#include <dimq_broker_internal.h>
#include <net_dimq.h>
#include <send_dimq.h>
#include <time_dimq.h>

extern char *last_sub;
extern int last_qos;
extern uint32_t last_identifier;
extern struct dimq_db db;

struct dimq *context__init(dimq_sock_t sock)
{
	struct dimq *m;

	UNUSED(sock);

	m = dimq__calloc(1, sizeof(struct dimq));
	if(m){
		m->msgs_in.inflight_maximum = 20;
		m->msgs_out.inflight_maximum = 20;
		m->msgs_in.inflight_quota = 20;
		m->msgs_out.inflight_quota = 20;
	}
	return m;
}

void db__msg_store_free(struct dimq_msg_store *store)
{
	int i;

	dimq__free(store->source_id);
	dimq__free(store->source_username);
	if(store->dest_ids){
		for(i=0; i<store->dest_id_count; i++){
			dimq__free(store->dest_ids[i]);
		}
		dimq__free(store->dest_ids);
	}
	dimq__free(store->topic);
	dimq_property_free_all(&store->properties);
	dimq__free(store->payload);
	dimq__free(store);
}

int db__message_store(const struct dimq *source, struct dimq_msg_store *stored, uint32_t message_expiry_interval, dbid_t store_id, enum dimq_msg_origin origin)
{
    int rc = dimq_ERR_SUCCESS;

	UNUSED(origin);

    if(source && source->id){
        stored->source_id = dimq__strdup(source->id);
    }else{
        stored->source_id = dimq__strdup("");
    }
    if(!stored->source_id){
        rc = dimq_ERR_NOMEM;
        goto error;
    }

    if(source && source->username){
        stored->source_username = dimq__strdup(source->username);
        if(!stored->source_username){
            rc = dimq_ERR_NOMEM;
            goto error;
        }
    }
    if(source){
        stored->source_listener = source->listener;
    }
    stored->mid = 0;
    if(message_expiry_interval > 0){
        stored->message_expiry_time = time(NULL) + message_expiry_interval;
    }else{
        stored->message_expiry_time = 0;
    }

    stored->dest_ids = NULL;
    stored->dest_id_count = 0;
    db.msg_store_count++;
    db.msg_store_bytes += stored->payloadlen;

    if(!store_id){
        stored->db_id = ++db.last_db_id;
    }else{
        stored->db_id = store_id;
    }

	db.msg_store = stored;

    return dimq_ERR_SUCCESS;
error:
	db__msg_store_free(stored);
    return rc;
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


int sub__add(struct dimq *context, const char *sub, uint8_t qos, uint32_t identifier, int options, struct dimq__subhier **root)
{
	UNUSED(context);
	UNUSED(options);
	UNUSED(root);

	last_sub = strdup(sub);
	last_qos = qos;
	last_identifier = identifier;

	return dimq_ERR_SUCCESS;
}

int db__message_insert(struct dimq *context, uint16_t mid, enum dimq_msg_direction dir, uint8_t qos, bool retain, struct dimq_msg_store *stored, dimq_property *properties, bool update)
{
	UNUSED(context);
	UNUSED(mid);
	UNUSED(dir);
	UNUSED(qos);
	UNUSED(retain);
	UNUSED(stored);
	UNUSED(properties);
	UNUSED(update);

	return dimq_ERR_SUCCESS;
}

void db__msg_store_ref_dec(struct dimq_msg_store **store)
{
	UNUSED(store);
}

void db__msg_store_ref_inc(struct dimq_msg_store *store)
{
	store->ref_count++;
}

