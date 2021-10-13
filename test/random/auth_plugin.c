#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dimq.h>
#include <dimq_broker.h>
#include <dimq_plugin.h>

int dimq_auth_plugin_version(void)
{
	return dimq_AUTH_PLUGIN_VERSION;
}

int dimq_auth_plugin_init(void **user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	srandom(time(NULL));
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
	if(random() % 2 == 0){
		return dimq_ERR_SUCCESS;
	}else{
		return dimq_ERR_ACL_DENIED;
	}
}

int dimq_auth_unpwd_check(void *user_data, struct dimq *client, const char *username, const char *password)
{
	if(random() % 2 == 0){
		return dimq_ERR_SUCCESS;
	}else{
		return dimq_ERR_AUTH;
	}
}

int dimq_auth_psk_key_get(void *user_data, struct dimq *client, const char *hint, const char *identity, char *key, int max_key_len)
{
	return dimq_ERR_AUTH;
}

