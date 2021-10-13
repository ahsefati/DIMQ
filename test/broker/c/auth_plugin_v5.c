#include <stdio.h>
#include <string.h>
#include <dimq.h>
#include <dimq_broker.h>
#include <dimq_plugin.h>

int dimq_auth_acl_check_v5(int event, void *event_data, void *user_data);
int dimq_auth_unpwd_check_v5(int event, void *event_data, void *user_data);

static dimq_plugin_id_t *plg_id;


int dimq_plugin_version(int supported_version_count, const int *supported_versions)
{
	return 5;
}

int dimq_plugin_init(dimq_plugin_id_t *identifier, void **user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	plg_id = identifier;

	dimq_callback_register(plg_id, dimq_EVT_ACL_CHECK, dimq_auth_acl_check_v5, NULL, NULL);
	dimq_callback_register(plg_id, dimq_EVT_BASIC_AUTH, dimq_auth_unpwd_check_v5, NULL, NULL);

	return dimq_ERR_SUCCESS;
}

int dimq_plugin_cleanup(void *user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	dimq_callback_unregister(plg_id, dimq_EVT_ACL_CHECK, dimq_auth_acl_check_v5, NULL);
	dimq_callback_unregister(plg_id, dimq_EVT_BASIC_AUTH, dimq_auth_unpwd_check_v5, NULL);

	return dimq_ERR_SUCCESS;
}

int dimq_auth_acl_check_v5(int event, void *event_data, void *user_data)
{
	struct dimq_evt_acl_check *ed = event_data;
	const char *username = dimq_client_username(ed->client);

	if(username && !strcmp(username, "readonly") && ed->access == dimq_ACL_READ){
		return dimq_ERR_SUCCESS;
	}else if(username && !strcmp(username, "readonly") && ed->access == dimq_ACL_SUBSCRIBE &&!strchr(ed->topic, '#') && !strchr(ed->topic, '+')) {
		return dimq_ERR_SUCCESS;
	}else if(username && !strcmp(username, "readwrite")){
		if((!strcmp(ed->topic, "readonly") && ed->access == dimq_ACL_READ)
				|| !strcmp(ed->topic, "writeable")){

			return dimq_ERR_SUCCESS;
		}else{
			return dimq_ERR_ACL_DENIED;
		}

	}else{
		return dimq_ERR_ACL_DENIED;
	}
}

int dimq_auth_unpwd_check_v5(int event, void *event_data, void *user_data)
{
	struct dimq_evt_basic_auth *ed = event_data;

	if(!strcmp(ed->username, "test-username") && ed->password && !strcmp(ed->password, "cnwTICONIURW")){
		return dimq_ERR_SUCCESS;
	}else if(!strcmp(ed->username, "readonly") || !strcmp(ed->username, "readwrite")){
		return dimq_ERR_SUCCESS;
	}else if(!strcmp(ed->username, "test-username@v2")){
		return dimq_ERR_PLUGIN_DEFER;
	}else{
		return dimq_ERR_AUTH;
	}
}
