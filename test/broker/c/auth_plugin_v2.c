#include <string.h>
#include <stdbool.h>
#include "dimq_plugin_v2.h"

/*
 * Following constant come from dimq.h
 *
 * They are copied here to fix value of those constant at the time of dimq_AUTH_PLUGIN_VERSION == 2
 */
enum dimq_err_t {
	dimq_ERR_SUCCESS = 0,
	dimq_ERR_AUTH = 11,
	dimq_ERR_ACL_DENIED = 12
};

int dimq_auth_plugin_version(void)
{
	return 2;
}

int dimq_auth_plugin_init(void **user_data, struct dimq_auth_opt *auth_opts, int auth_opt_count)
{
	return dimq_ERR_SUCCESS;
}

int dimq_auth_plugin_cleanup(void *user_data, struct dimq_auth_opt *auth_opts, int auth_opt_count)
{
	return dimq_ERR_SUCCESS;
}

int dimq_auth_security_init(void *user_data, struct dimq_auth_opt *auth_opts, int auth_opt_count, bool reload)
{
	return dimq_ERR_SUCCESS;
}

int dimq_auth_security_cleanup(void *user_data, struct dimq_auth_opt *auth_opts, int auth_opt_count, bool reload)
{
	return dimq_ERR_SUCCESS;
}

int dimq_auth_acl_check(void *user_data, const char *clientid, const char *username, const char *topic, int access)
{
	if(!strcmp(username, "readonly") && access == dimq_ACL_READ){
		return dimq_ERR_SUCCESS;
	}else{
		return dimq_ERR_ACL_DENIED;
	}
}

int dimq_auth_unpwd_check(void *user_data, const char *username, const char *password)
{
	if(!strcmp(username, "test-username") && password && !strcmp(password, "cnwTICONIURW")){
		return dimq_ERR_SUCCESS;
	}else if(!strcmp(username, "readonly")){
		return dimq_ERR_SUCCESS;
	}else if(!strcmp(username, "test-username@v2")){
		return dimq_ERR_SUCCESS;
	}else{
		return dimq_ERR_AUTH;
	}
}

int dimq_auth_psk_key_get(void *user_data, const char *hint, const char *identity, char *key, int max_key_len)
{
	return dimq_ERR_AUTH;
}

