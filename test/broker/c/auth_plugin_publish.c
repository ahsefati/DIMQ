#include <stdio.h>
#include <string.h>
#include <mqtt_protocol.h>
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
	static int count = 0;
	dimq_property *props = NULL;

	if(access == dimq_ACL_WRITE){
		if(count == 0){
			/* "missing-client" isn't connected, so we can check memory usage properly. */
			dimq_broker_publish_copy("missing-client", "topic/2", strlen("test-message-2"), "test-message-2", 2, true, NULL);
			dimq_broker_publish_copy("test-client", "topic/0", strlen("test-message-0"), "test-message-0", 0, true, NULL);
			dimq_broker_publish_copy("missing-client", "topic/2", strlen("test-message-2"), "test-message-2", 2, true, NULL);
			dimq_broker_publish_copy("test-client", "topic/1", strlen("test-message-1"), "test-message-1", 1, true, NULL);
			dimq_broker_publish_copy("missing-client", "topic/2", strlen("test-message-2"), "test-message-2", 2, true, NULL);
			dimq_broker_publish_copy("test-client", "topic/2", strlen("test-message-2"), "test-message-2", 2, true, NULL);
			count = 1;
		}else{
			dimq_property_add_byte(&props, MQTT_PROP_PAYLOAD_FORMAT_INDICATOR, 1);
			dimq_broker_publish_copy("test-client", "topic/0", strlen("test-message-0"), "test-message-0", 0, true, props);
			props = NULL;
			dimq_property_add_byte(&props, MQTT_PROP_PAYLOAD_FORMAT_INDICATOR, 1);
			dimq_broker_publish_copy("test-client", "topic/1", strlen("test-message-1"), "test-message-1", 1, true, props);
			props = NULL;
			dimq_property_add_byte(&props, MQTT_PROP_PAYLOAD_FORMAT_INDICATOR, 1);
			dimq_broker_publish_copy("test-client", "topic/2", strlen("test-message-2"), "test-message-2", 2, true, props);
		}
	}

	return dimq_ERR_SUCCESS;
}

int dimq_auth_unpwd_check(void *user_data, struct dimq *client, const char *username, const char *password)
{
	return dimq_ERR_SUCCESS;
}

int dimq_auth_psk_key_get(void *user_data, struct dimq *client, const char *hint, const char *identity, char *key, int max_key_len)
{
	return dimq_ERR_AUTH;
}

