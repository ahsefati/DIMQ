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
	return dimq_ERR_PLUGIN_DEFER;
}

int dimq_auth_unpwd_check(void *user_data, struct dimq *client, const char *username, const char *password)
{
	const char *tmp;

	tmp = dimq_client_address(client);
	if(!tmp || strcmp(tmp, "127.0.0.1")){
		return dimq_ERR_AUTH;
	}

	if(!dimq_client_clean_session(client)){
		fprintf(stderr, "dimq_auth_unpwd_check clean_session error: %d\n", dimq_client_clean_session(client));
		return dimq_ERR_AUTH;
	}

	tmp = dimq_client_id(client);
	if(!tmp || strcmp(tmp, "client-params-test")){
		fprintf(stderr, "dimq_auth_unpwd_check client_id error: %s\n", tmp);
		return dimq_ERR_AUTH;
	}

	if(dimq_client_keepalive(client) != 42){
		fprintf(stderr, "dimq_auth_unpwd_check keepalive error: %d\n", dimq_client_keepalive(client));
		return dimq_ERR_AUTH;
	}

	if(!dimq_client_certificate(client)){
		// FIXME
		//return dimq_ERR_AUTH;
	}

	if(dimq_client_protocol(client) != mp_mqtt){
		fprintf(stderr, "dimq_auth_unpwd_check protocol error: %d\n", dimq_client_protocol(client));
		return dimq_ERR_AUTH;
	}

	if(dimq_client_sub_count(client)){
		fprintf(stderr, "dimq_auth_unpwd_check sub_count error: %d\n", dimq_client_sub_count(client));
		return dimq_ERR_AUTH;
	}

	tmp = dimq_client_username(client);
	if(!tmp || strcmp(tmp, "client-username")){
		fprintf(stderr, "dimq_auth_unpwd_check username error: %s\n", tmp);
		return dimq_ERR_AUTH;
	}

	return dimq_ERR_SUCCESS;
}

int dimq_auth_psk_key_get(void *user_data, struct dimq *client, const char *hint, const char *identity, char *key, int max_key_len)
{
	return dimq_ERR_AUTH;
}

