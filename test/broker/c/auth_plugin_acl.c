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
	const char *username = dimq_client_username(client);

	if(username && !strcmp(username, "readonly") && access == dimq_ACL_READ){
		return dimq_ERR_SUCCESS;
	}else if(username && !strcmp(username, "readonly") && access == dimq_ACL_SUBSCRIBE &&!strchr(msg->topic, '#') && !strchr(msg->topic, '+')) {
		return dimq_ERR_SUCCESS;
	}else{
		return dimq_ERR_ACL_DENIED;
	}
}

int dimq_auth_unpwd_check(void *user_data, struct dimq *client, const char *username, const char *password)
{
	return dimq_ERR_PLUGIN_DEFER;
}

int dimq_auth_psk_key_get(void *user_data, struct dimq *client, const char *hint, const char *identity, char *key, int max_key_len)
{
	return dimq_ERR_AUTH;
}

