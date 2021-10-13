#include <stdio.h>
#include <string.h>
#include <mqtt_protocol.h>
#include <dimq.h>
#include <dimq_broker.h>
#include <dimq_plugin.h>

static dimq_plugin_id_t *plg_id = NULL;

int control_callback(int event, void *event_data, void *userdata)
{
	struct dimq_evt_control *ed = event_data;

	dimq_broker_publish_copy(NULL, ed->topic, ed->payloadlen, ed->payload, 0, 0, NULL);

	return 0;
}


int dimq_plugin_version(int supported_version_count, const int *supported_versions)
{
	return dimq_PLUGIN_VERSION;
}

int dimq_plugin_init(dimq_plugin_id_t *identifier, void **user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	int i;
	char buf[100];

	plg_id = identifier;

	for(i=0; i<100; i++){
		snprintf(buf, sizeof(buf), "$CONTROL/user-management/v%d", i);
		dimq_callback_register(plg_id, dimq_EVT_CONTROL, control_callback, "$CONTROL/user-management/v1", NULL);
	}
	return dimq_ERR_SUCCESS;
}

int dimq_plugin_cleanup(void *user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	int i;
	char buf[100];

	for(i=0; i<100; i++){
		snprintf(buf, sizeof(buf), "$CONTROL/user-management/v%d", i);
		dimq_callback_unregister(plg_id, dimq_EVT_CONTROL, control_callback, "$CONTROL/user-management/v1");
	}
	return dimq_ERR_SUCCESS;
}
