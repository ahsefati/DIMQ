#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dimq.h>
#include <dimq_broker.h>
#include <dimq_plugin.h>

int dimq_auth_plugin_version(void)
{
	return 4;
}

int dimq_auth_plugin_init(void **user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	return dimq_ERR_SUCCESS;
}

int dimq_auth_plugin_cleanup(void *user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	return dimq_ERR_SUCCESS;
}

int dimq_auth_security_init(void *user_data, struct dimq_opt *auth_opts, int auth_opt_count, bool reload)
{
	return dimq_ERR_SUCCESS;
}

int dimq_auth_security_cleanup(void *user_data, struct dimq_opt *auth_opts, int auth_opt_count, bool reload)
{
	return dimq_ERR_SUCCESS;
}

int dimq_auth_acl_check(void *user_data, int access, struct dimq *client, const struct dimq_acl_msg *msg)
{
	if(access == dimq_ACL_SUBSCRIBE){
		return dimq_ERR_SUCCESS;
	}

	if(!msg->topic || strcmp(msg->topic, "param/topic")){
		abort();
		return dimq_ERR_ACL_DENIED;
	}

	if(!msg->payload || strncmp(msg->payload, "payload contents", strlen("payload contents"))){
		abort();
		return dimq_ERR_ACL_DENIED;
	}

	if(msg->payloadlen != strlen("payload contents")){
		abort();
		return dimq_ERR_ACL_DENIED;
	}

	if(msg->qos != 1){
		abort();
		return dimq_ERR_ACL_DENIED;
	}

	if(!msg->retain){
		abort();
		return dimq_ERR_ACL_DENIED;
	}
	
	return dimq_ERR_SUCCESS;
}

int dimq_auth_unpwd_check(void *user_data, struct dimq *client, const char *username, const char *password)
{
	return dimq_ERR_PLUGIN_DEFER;
}

int dimq_auth_psk_key_get(void *user_data, struct dimq *client, const char *hint, const char *identity, char *key, int max_key_len)
{
	return dimq_ERR_AUTH;
}

