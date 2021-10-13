#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dimq.h>
#include <dimq_broker.h>
#include <dimq_plugin.h>

static int handle_publish(int event, void *event_data, void *user_data);

static dimq_plugin_id_t *plg_id;


int dimq_plugin_version(int supported_version_count, const int *supported_versions)
{
	return 5;
}

int dimq_plugin_init(dimq_plugin_id_t *identifier, void **user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	plg_id = identifier;

	dimq_callback_register(plg_id, dimq_EVT_MESSAGE, handle_publish, NULL, NULL);

	return dimq_ERR_SUCCESS;
}

int dimq_plugin_cleanup(void *user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	dimq_callback_unregister(plg_id, dimq_EVT_MESSAGE, handle_publish, NULL);

	return dimq_ERR_SUCCESS;
}

int handle_publish(int event, void *event_data, void *user_data)
{
	struct dimq_evt_message *ed = event_data;

	dimq_free(ed->topic);
	ed->topic = dimq_strdup("fixed-topic");
	return dimq_ERR_SUCCESS;
}
