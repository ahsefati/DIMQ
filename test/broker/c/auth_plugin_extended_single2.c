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


int dimq_auth_start(void *user_data, struct dimq *client, const char *method, bool reauth, const void *data, uint16_t data_len, void **data_out, uint16_t *data_out_len)
{
	int i;

	if(!strcmp(method, "error2")){
		return dimq_ERR_INVAL;
	}else if(!strcmp(method, "non-matching2")){
		return dimq_ERR_NOT_SUPPORTED;
	}else if(!strcmp(method, "single2")){
		data_len = data_len>strlen("data")?strlen("data"):data_len;
		if(!memcmp(data, "data", data_len)){
			return dimq_ERR_SUCCESS;
		}else{
			return dimq_ERR_AUTH;
		}
	}else if(!strcmp(method, "change2")){
		return dimq_set_username(client, "new_username");
	}else if(!strcmp(method, "mirror2")){
		if(data_len > 0){
			*data_out = malloc(data_len);
			if(!(*data_out)){
				return dimq_ERR_NOMEM;
			}
			for(i=0; i<data_len; i++){
				((uint8_t *)(*data_out))[i] = ((uint8_t *)data)[data_len-i-1];
			}
			*data_out_len = data_len;

			return dimq_ERR_SUCCESS;
		}else{
			return dimq_ERR_INVAL;
		}
	}
	return dimq_ERR_NOT_SUPPORTED;
}

int dimq_auth_continue(void *user_data, struct dimq *client, const char *method, const void *data, uint16_t data_len, void **data_out, uint16_t *data_out_len)
{
	return dimq_ERR_AUTH;
}
