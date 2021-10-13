/*
Copyright (c) 2009-2020 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

Contributors:
   Roger Light - initial implementation and documentation.
   Tatsuzo Osawa - Add epoll.
*/

#ifndef dimq_BROKER_INTERNAL_H
#define dimq_BROKER_INTERNAL_H

#include "config.h"
#include <stdio.h>

#ifdef WITH_WEBSOCKETS
#  include <libwebsockets.h>
#  if LWS_LIBRARY_VERSION_NUMBER >= 3002000 && !defined(LWS_WITH_EXTERNAL_POLL)
#    warning "libwebsockets is not compiled with LWS_WITH_EXTERNAL_POLL support. Websocket performance will be unusable."
#  endif
#endif

#include "dimq_internal.h"
#include "dimq_broker.h"
#include "dimq_plugin.h"
#include "dimq.h"
#include "logging_dimq.h"
#include "password_dimq.h"
#include "tls_dimq.h"
#include "uthash.h"

#ifndef __GNUC__
#define __attribute__(attrib)
#endif

/* Log destinations */
#define MQTT3_LOG_NONE 0x00
#define MQTT3_LOG_SYSLOG 0x01
#define MQTT3_LOG_FILE 0x02
#define MQTT3_LOG_STDOUT 0x04
#define MQTT3_LOG_STDERR 0x08
#define MQTT3_LOG_TOPIC 0x10
#define MQTT3_LOG_DLT 0x20
#define MQTT3_LOG_ALL 0xFF

#define WEBSOCKET_CLIENT -2

#define CMD_PORT_LIMIT 10
#define TOPIC_HIERARCHY_LIMIT 200

typedef uint64_t dbid_t;

typedef int (*FUNC_plugin_init_v5)(dimq_plugin_id_t *, void **, struct dimq_opt *, int);
typedef int (*FUNC_plugin_cleanup_v5)(void *, struct dimq_opt *, int);

typedef int (*FUNC_auth_plugin_init_v4)(void **, struct dimq_opt *, int);
typedef int (*FUNC_auth_plugin_cleanup_v4)(void *, struct dimq_opt *, int);
typedef int (*FUNC_auth_plugin_security_init_v4)(void *, struct dimq_opt *, int, bool);
typedef int (*FUNC_auth_plugin_security_cleanup_v4)(void *, struct dimq_opt *, int, bool);
typedef int (*FUNC_auth_plugin_acl_check_v4)(void *, int, struct dimq *, struct dimq_acl_msg *);
typedef int (*FUNC_auth_plugin_unpwd_check_v4)(void *, struct dimq *, const char *, const char *);
typedef int (*FUNC_auth_plugin_psk_key_get_v4)(void *, struct dimq *, const char *, const char *, char *, int);
typedef int (*FUNC_auth_plugin_auth_start_v4)(void *, struct dimq *, const char *, bool, const void *, uint16_t, void **, uint16_t *);
typedef int (*FUNC_auth_plugin_auth_continue_v4)(void *, struct dimq *, const char *, const void *, uint16_t, void **, uint16_t *);

typedef int (*FUNC_auth_plugin_init_v3)(void **, struct dimq_opt *, int);
typedef int (*FUNC_auth_plugin_cleanup_v3)(void *, struct dimq_opt *, int);
typedef int (*FUNC_auth_plugin_security_init_v3)(void *, struct dimq_opt *, int, bool);
typedef int (*FUNC_auth_plugin_security_cleanup_v3)(void *, struct dimq_opt *, int, bool);
typedef int (*FUNC_auth_plugin_acl_check_v3)(void *, int, const struct dimq *, struct dimq_acl_msg *);
typedef int (*FUNC_auth_plugin_unpwd_check_v3)(void *, const struct dimq *, const char *, const char *);
typedef int (*FUNC_auth_plugin_psk_key_get_v3)(void *, const struct dimq *, const char *, const char *, char *, int);

typedef int (*FUNC_auth_plugin_init_v2)(void **, struct dimq_auth_opt *, int);
typedef int (*FUNC_auth_plugin_cleanup_v2)(void *, struct dimq_auth_opt *, int);
typedef int (*FUNC_auth_plugin_security_init_v2)(void *, struct dimq_auth_opt *, int, bool);
typedef int (*FUNC_auth_plugin_security_cleanup_v2)(void *, struct dimq_auth_opt *, int, bool);
typedef int (*FUNC_auth_plugin_acl_check_v2)(void *, const char *, const char *, const char *, int);
typedef int (*FUNC_auth_plugin_unpwd_check_v2)(void *, const char *, const char *);
typedef int (*FUNC_auth_plugin_psk_key_get_v2)(void *, const char *, const char *, char *, int);


enum dimq_msg_origin{
	dimq_mo_client = 0,
	dimq_mo_broker = 1
};

struct dimq__auth_plugin{
	void *lib;
	void *user_data;
	int (*plugin_version)(void);
	struct dimq_plugin_id_t *identifier;

	FUNC_plugin_init_v5 plugin_init_v5;
	FUNC_plugin_cleanup_v5 plugin_cleanup_v5;

	FUNC_auth_plugin_init_v4 plugin_init_v4;
	FUNC_auth_plugin_cleanup_v4 plugin_cleanup_v4;
	FUNC_auth_plugin_security_init_v4 security_init_v4;
	FUNC_auth_plugin_security_cleanup_v4 security_cleanup_v4;
	FUNC_auth_plugin_acl_check_v4 acl_check_v4;
	FUNC_auth_plugin_unpwd_check_v4 unpwd_check_v4;
	FUNC_auth_plugin_psk_key_get_v4 psk_key_get_v4;
	FUNC_auth_plugin_auth_start_v4 auth_start_v4;
	FUNC_auth_plugin_auth_continue_v4 auth_continue_v4;

	FUNC_auth_plugin_init_v3 plugin_init_v3;
	FUNC_auth_plugin_cleanup_v3 plugin_cleanup_v3;
	FUNC_auth_plugin_security_init_v3 security_init_v3;
	FUNC_auth_plugin_security_cleanup_v3 security_cleanup_v3;
	FUNC_auth_plugin_acl_check_v3 acl_check_v3;
	FUNC_auth_plugin_unpwd_check_v3 unpwd_check_v3;
	FUNC_auth_plugin_psk_key_get_v3 psk_key_get_v3;

	FUNC_auth_plugin_init_v2 plugin_init_v2;
	FUNC_auth_plugin_cleanup_v2 plugin_cleanup_v2;
	FUNC_auth_plugin_security_init_v2 security_init_v2;
	FUNC_auth_plugin_security_cleanup_v2 security_cleanup_v2;
	FUNC_auth_plugin_acl_check_v2 acl_check_v2;
	FUNC_auth_plugin_unpwd_check_v2 unpwd_check_v2;
	FUNC_auth_plugin_psk_key_get_v2 psk_key_get_v2;
	int version;
};

struct dimq__auth_plugin_config
{
	char *path;
	struct dimq_opt *options;
	int option_count;
	bool deny_special_chars;

	struct dimq__auth_plugin plugin;
};

struct dimq__callback{
	UT_hash_handle hh; /* For callbacks that register for e.g. a specific topic */
	struct dimq__callback *next, *prev; /* For typical callbacks */
	dimq_FUNC_generic_callback cb;
	void *userdata;
	char *data; /* e.g. topic for control event */
};

struct plugin__callbacks{
	struct dimq__callback *tick;
	struct dimq__callback *acl_check;
	struct dimq__callback *basic_auth;
	struct dimq__callback *control;
	struct dimq__callback *disconnect;
	struct dimq__callback *ext_auth_continue;
	struct dimq__callback *ext_auth_start;
	struct dimq__callback *message;
	struct dimq__callback *psk_key;
	struct dimq__callback *reload;
};

struct dimq__security_options {
	/* Any options that get added here also need considering
	 * in config__read() with regards whether allow_anonymous
	 * should be disabled when these options are set.
	 */
	struct dimq__unpwd *unpwd;
	struct dimq__unpwd *psk_id;
	struct dimq__acl_user *acl_list;
	struct dimq__acl *acl_patterns;
	char *password_file;
	char *psk_file;
	char *acl_file;
	struct dimq__auth_plugin_config *auth_plugin_configs;
	int auth_plugin_config_count;
	int8_t allow_anonymous;
	bool allow_zero_length_clientid;
	char *auto_id_prefix;
	uint16_t auto_id_prefix_len;
	struct plugin__callbacks plugin_callbacks;
	dimq_plugin_id_t *pid; /* For registering as a "plugin" */
};

#ifdef WITH_EPOLL
enum struct_ident{
	id_invalid = 0,
	id_listener = 1,
	id_client = 2,
	id_listener_ws = 3,
};
#endif

struct dimq__listener {
	uint16_t port;
	char *host;
	char *bind_interface;
	int max_connections;
	char *mount_point;
	dimq_sock_t *socks;
	int sock_count;
	int client_count;
	enum dimq_protocol protocol;
	int socket_domain;
	bool use_username_as_clientid;
	uint8_t max_qos;
	uint16_t max_topic_alias;
#ifdef WITH_TLS
	char *cafile;
	char *capath;
	char *certfile;
	char *keyfile;
	char *tls_engine;
	char *tls_engine_kpass_sha1;
	char *ciphers;
	char *ciphers_tls13;
	char *psk_hint;
	SSL_CTX *ssl_ctx;
	char *crlfile;
	char *tls_version;
	char *dhparamfile;
	bool use_identity_as_username;
	bool use_subject_as_username;
	bool require_certificate;
	enum dimq__keyform tls_keyform;
#endif
#ifdef WITH_WEBSOCKETS
	struct lws_context *ws_context;
	bool ws_in_init;
	char *http_dir;
	struct lws_protocols *ws_protocol;
#endif
	struct dimq__security_options security_options;
#ifdef WITH_UNIX_SOCKETS
	char *unix_socket_path;
#endif
};


struct dimq__listener_sock{
#ifdef WITH_EPOLL
	/* This *must* be the first element in the struct. */
	int ident;
#endif
	dimq_sock_t sock;
	struct dimq__listener *listener;
};

typedef struct dimq_plugin_id_t{
	struct dimq__listener *listener;
} dimq_plugin_id_t;

struct dimq__config {
	bool allow_duplicate_messages;
	int autosave_interval;
	bool autosave_on_changes;
	bool check_retain_source;
	char *clientid_prefixes;
	bool connection_messages;
	uint16_t cmd_port[CMD_PORT_LIMIT];
	int cmd_port_count;
	bool daemon;
	struct dimq__listener default_listener;
	struct dimq__listener *listeners;
	int listener_count;
	bool local_only;
	unsigned int log_dest;
	int log_facility;
	unsigned int log_type;
	bool log_timestamp;
	char *log_timestamp_format;
	char *log_file;
	FILE *log_fptr;
	size_t max_inflight_bytes;
	size_t max_queued_bytes;
	int max_queued_messages;
	uint32_t max_packet_size;
	uint32_t message_size_limit;
	uint16_t max_inflight_messages;
	uint16_t max_keepalive;
	uint8_t max_qos;
	bool persistence;
	char *persistence_location;
	char *persistence_file;
	char *persistence_filepath;
	time_t persistent_client_expiration;
	char *pid_file;
	bool queue_qos0_messages;
	bool per_listener_settings;
	bool retain_available;
	bool set_tcp_nodelay;
	int sys_interval;
	bool upgrade_outgoing_qos;
	char *user;
#ifdef WITH_WEBSOCKETS
	int websockets_log_level;
	uint16_t websockets_headers_size;
#endif
#ifdef WITH_BRIDGE
	struct dimq__bridge *bridges;
	int bridge_count;
#endif
	struct dimq__security_options security_options;
};


struct dimq__subleaf {
	struct dimq__subleaf *prev;
	struct dimq__subleaf *next;
	struct dimq *context;
	uint32_t identifier;
	uint8_t qos;
	bool no_local;
	bool retain_as_published;
};


struct dimq__subshared {
	UT_hash_handle hh;
	char *name;
	struct dimq__subleaf *subs;
};

struct dimq__subhier {
	UT_hash_handle hh;
	struct dimq__subhier *parent;
	struct dimq__subhier *children;
	struct dimq__subleaf *subs;
	struct dimq__subshared *shared;
	char *topic;
	uint16_t topic_len;
};

struct dimq__client_sub {
	struct dimq__subhier *hier;
	struct dimq__subshared *shared;
	char topic_filter[];
};

struct sub__token {
	struct sub__token *next;
	char *topic;
	uint16_t topic_len;
};

struct dimq__retainhier {
	UT_hash_handle hh;
	struct dimq__retainhier *parent;
	struct dimq__retainhier *children;
	struct dimq_msg_store *retained;
	char *topic;
	uint16_t topic_len;
};

struct dimq_msg_store_load{
	UT_hash_handle hh;
	dbid_t db_id;
	struct dimq_msg_store *store;
};

struct dimq_msg_store{
	struct dimq_msg_store *next;
	struct dimq_msg_store *prev;
	dbid_t db_id;
	char *source_id;
	char *source_username;
	struct dimq__listener *source_listener;
	char **dest_ids;
	int dest_id_count;
	int ref_count;
	char* topic;
	dimq_property *properties;
	void *payload;
	time_t message_expiry_time;
	uint32_t payloadlen;
	enum dimq_msg_origin origin;
	uint16_t source_mid;
	uint16_t mid;
	uint8_t qos;
	bool retain;
};

struct dimq_client_msg{
	struct dimq_client_msg *prev;
	struct dimq_client_msg *next;
	struct dimq_msg_store *store;
	dimq_property *properties;
	time_t timestamp;
	uint16_t mid;
	uint8_t qos;
	bool retain;
	enum dimq_msg_direction direction;
	enum dimq_msg_state state;
	bool dup;
};


struct dimq__unpwd{
	UT_hash_handle hh;
	char *username;
	char *password;
	char *clientid;
#ifdef WITH_TLS
	unsigned char *salt;
	unsigned int password_len;
	unsigned int salt_len;
	int iterations;
#endif
	enum dimq_pwhash_type hashtype;
};

struct dimq__acl{
	struct dimq__acl *next;
	char *topic;
	int access;
	int ucount;
	int ccount;
};

struct dimq__acl_user{
	struct dimq__acl_user *next;
	char *username;
	struct dimq__acl *acl;
};


struct dimq_message_v5{
	struct dimq_message_v5 *next, *prev;
	char *topic;
	void *payload;
	dimq_property *properties;
	char *clientid; /* Used only by dimq_broker_publish*() to indicate
					   this message is for a specific client. */
	int payloadlen;
	int qos;
	bool retain;
};


struct dimq_db{
	dbid_t last_db_id;
	struct dimq__subhier *subs;
	struct dimq__retainhier *retains;
	struct dimq *contexts_by_id;
	struct dimq *contexts_by_sock;
	struct dimq *contexts_for_free;
#ifdef WITH_BRIDGE
	struct dimq **bridges;
#endif
	struct clientid__index_hash *clientid_index_hash;
	struct dimq_msg_store *msg_store;
	struct dimq_msg_store_load *msg_store_load;
	time_t now_s; /* Monotonic clock, where possible */
	time_t now_real_s; /* Read clock, for measuring session/message expiry */
#ifdef WITH_BRIDGE
	int bridge_count;
#endif
	int msg_store_count;
	unsigned long msg_store_bytes;
	char *config_file;
	struct dimq__config *config;
	int auth_plugin_count;
	bool verbose;
#ifdef WITH_SYS_TREE
	int subscription_count;
	int shared_subscription_count;
	int retained_count;
#endif
	int persistence_changes;
	struct dimq *ll_for_free;
#ifdef WITH_EPOLL
	int epollfd;
#endif
	struct dimq_message_v5 *plugin_msgs;
};

enum dimq__bridge_direction{
	bd_out = 0,
	bd_in = 1,
	bd_both = 2
};

enum dimq_bridge_start_type{
	bst_automatic = 0,
	bst_lazy = 1,
	bst_manual = 2,
	bst_once = 3
};

struct dimq__bridge_topic{
	char *topic;
	char *local_prefix;
	char *remote_prefix;
	char *local_topic; /* topic prefixed with local_prefix */
	char *remote_topic; /* topic prefixed with remote_prefix */
	enum dimq__bridge_direction direction;
	uint8_t qos;
};

struct bridge_address{
	char *address;
	uint16_t port;
};

struct dimq__bridge{
	char *name;
	struct bridge_address *addresses;
	int cur_address;
	int address_count;
	time_t primary_retry;
	dimq_sock_t primary_retry_sock;
	bool round_robin;
	bool try_private;
	bool try_private_accepted;
	bool clean_start;
	int8_t clean_start_local;
	uint16_t keepalive;
	struct dimq__bridge_topic *topics;
	int topic_count;
	bool topic_remapping;
	enum dimq__protocol protocol_version;
	time_t restart_t;
	char *remote_clientid;
	char *remote_username;
	char *remote_password;
	char *local_clientid;
	char *local_username;
	char *local_password;
	char *notification_topic;
	char *bind_address;
	bool notifications;
	bool notifications_local_only;
	enum dimq_bridge_start_type start_type;
	int idle_timeout;
	int restart_timeout;
	int backoff_base;
	int backoff_cap;
	int threshold;
	uint32_t maximum_packet_size;
	bool lazy_reconnect;
	bool attempt_unsubscribe;
	bool initial_notification_done;
	bool outgoing_retain;
#ifdef WITH_TLS
	bool tls_insecure;
	bool tls_ocsp_required;
	char *tls_cafile;
	char *tls_capath;
	char *tls_certfile;
	char *tls_keyfile;
	char *tls_version;
	char *tls_alpn;
#  ifdef FINAL_WITH_TLS_PSK
	char *tls_psk_identity;
	char *tls_psk;
#  endif
#endif
};

#ifdef WITH_WEBSOCKETS
struct libws_mqtt_hack {
	char *http_dir;
	struct dimq__listener *listener;
};

struct libws_mqtt_data {
	struct dimq *dimq;
};
#endif

#include <net_dimq.h>


extern struct dimq_db db;

/* ============================================================
 * Main functions
 * ============================================================ */
int dimq_main_loop(struct dimq__listener_sock *listensock, int listensock_count);

/* ============================================================
 * Config functions
 * ============================================================ */
/* Initialise config struct to default values. */
void config__init(struct dimq__config *config);
/* Parse command line options into config. */
int config__parse_args(struct dimq__config *config, int argc, char *argv[]);
/* Read configuration data from config->config_file into config.
 * If reload is true, don't process config options that shouldn't be reloaded (listeners etc)
 * Returns 0 on success, 1 if there is a configuration error or if a file cannot be opened.
 */
int config__read(struct dimq__config *config, bool reload);
/* Free all config data. */
void config__cleanup(struct dimq__config *config);
int config__get_dir_files(const char *include_dir, char ***files, int *file_count);

int drop_privileges(struct dimq__config *config);

/* ============================================================
 * Server send functions
 * ============================================================ */
int send__connack(struct dimq *context, uint8_t ack, uint8_t reason_code, const dimq_property *properties);
int send__suback(struct dimq *context, uint16_t mid, uint32_t payloadlen, const void *payload);
int send__unsuback(struct dimq *context, uint16_t mid, int reason_code_count, uint8_t *reason_codes, const dimq_property *properties);
int send__auth(struct dimq *context, uint8_t reason_code, const void *auth_data, uint16_t auth_data_len);

/* ============================================================
 * Network functions
 * ============================================================ */
void net__broker_init(void);
void net__broker_cleanup(void);
struct dimq *net__socket_accept(struct dimq__listener_sock *listensock);
int net__socket_listen(struct dimq__listener *listener);
int net__socket_get_address(dimq_sock_t sock, char *buf, size_t len, uint16_t *remote_address);
int net__tls_load_verify(struct dimq__listener *listener);
int net__tls_server_ctx(struct dimq__listener *listener);
int net__load_certificates(struct dimq__listener *listener);

/* ============================================================
 * Read handling functions
 * ============================================================ */
int handle__packet(struct dimq *context);
int handle__connack(struct dimq *context);
int handle__connect(struct dimq *context);
int handle__disconnect(struct dimq *context);
int handle__publish(struct dimq *context);
int handle__subscribe(struct dimq *context);
int handle__unsubscribe(struct dimq *context);
int handle__auth(struct dimq *context);

/* ============================================================
 * Database handling
 * ============================================================ */
int db__open(struct dimq__config *config);
int db__close(void);
#ifdef WITH_PERSISTENCE
int persist__backup(bool shutdown);
int persist__restore(void);
#endif
/* Return the number of in-flight messages in count. */
int db__message_count(int *count);
int db__message_delete_outgoing(struct dimq *context, uint16_t mid, enum dimq_msg_state expect_state, int qos);
int db__message_insert(struct dimq *context, uint16_t mid, enum dimq_msg_direction dir, uint8_t qos, bool retain, struct dimq_msg_store *stored, dimq_property *properties, bool update);
int db__message_remove_incoming(struct dimq* context, uint16_t mid);
int db__message_release_incoming(struct dimq *context, uint16_t mid);
int db__message_update_outgoing(struct dimq *context, uint16_t mid, enum dimq_msg_state state, int qos);
void db__message_dequeue_first(struct dimq *context, struct dimq_msg_data *msg_data);
int db__messages_delete(struct dimq *context, bool force_free);
int db__messages_easy_queue(struct dimq *context, const char *topic, uint8_t qos, uint32_t payloadlen, const void *payload, int retain, uint32_t message_expiry_interval, dimq_property **properties);
int db__message_store(const struct dimq *source, struct dimq_msg_store *stored, uint32_t message_expiry_interval, dbid_t store_id, enum dimq_msg_origin origin);
int db__message_store_find(struct dimq *context, uint16_t mid, struct dimq_msg_store **stored);
void db__msg_store_add(struct dimq_msg_store *store);
void db__msg_store_remove(struct dimq_msg_store *store);
void db__msg_store_ref_inc(struct dimq_msg_store *store);
void db__msg_store_ref_dec(struct dimq_msg_store **store);
void db__msg_store_clean(void);
void db__msg_store_compact(void);
void db__msg_store_free(struct dimq_msg_store *store);
int db__message_reconnect_reset(struct dimq *context);
bool db__ready_for_flight(struct dimq *context, enum dimq_msg_direction dir, int qos);
bool db__ready_for_queue(struct dimq *context, int qos, struct dimq_msg_data *msg_data);
void sys_tree__init(void);
void sys_tree__update(int interval, time_t start_time);
int db__message_write_inflight_out_all(struct dimq *context);
int db__message_write_inflight_out_latest(struct dimq *context);
int db__message_write_queued_out(struct dimq *context);
int db__message_write_queued_in(struct dimq *context);

/* ============================================================
 * Subscription functions
 * ============================================================ */
int sub__add(struct dimq *context, const char *sub, uint8_t qos, uint32_t identifier, int options, struct dimq__subhier **root);
struct dimq__subhier *sub__add_hier_entry(struct dimq__subhier *parent, struct dimq__subhier **sibling, const char *topic, uint16_t len);
int sub__remove(struct dimq *context, const char *sub, struct dimq__subhier *root, uint8_t *reason);
void sub__tree_print(struct dimq__subhier *root, int level);
int sub__clean_session(struct dimq *context);
int sub__messages_queue(const char *source_id, const char *topic, uint8_t qos, int retain, struct dimq_msg_store **stored);
int sub__topic_tokenise(const char *subtopic, char **local_sub, char ***topics, const char **sharename);
void sub__topic_tokens_free(struct sub__token *tokens);

/* ============================================================
 * Context functions
 * ============================================================ */
struct dimq *context__init(dimq_sock_t sock);
void context__cleanup(struct dimq *context, bool force_free);
void context__disconnect(struct dimq *context);
void context__add_to_disused(struct dimq *context);
void context__free_disused(void);
void context__send_will(struct dimq *context);
void context__remove_from_by_id(struct dimq *context);

int connect__on_authorised(struct dimq *context, void *auth_data_out, uint16_t auth_data_out_len);


/* ============================================================
 * Control functions
 * ============================================================ */
#ifdef WITH_CONTROL
int control__process(struct dimq *context, struct dimq_msg_store *stored);
void control__cleanup(void);
#endif
int control__register_callback(struct dimq__security_options *opts, dimq_FUNC_generic_callback cb_func, const char *topic, void *userdata);
int control__unregister_callback(struct dimq__security_options *opts, dimq_FUNC_generic_callback cb_func, const char *topic);


/* ============================================================
 * Logging functions
 * ============================================================ */
int log__init(struct dimq__config *config);
int log__close(struct dimq__config *config);
void log__internal(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* ============================================================
 * Bridge functions
 * ============================================================ */
#ifdef WITH_BRIDGE
void bridge__start_all(void);
int bridge__new(struct dimq__bridge *bridge);
void bridge__cleanup(struct dimq *context);
int bridge__connect(struct dimq *context);
int bridge__connect_step1(struct dimq *context);
int bridge__connect_step2(struct dimq *context);
int bridge__connect_step3(struct dimq *context);
int bridge__on_connect(struct dimq *context);
void bridge__packet_cleanup(struct dimq *context);
void bridge_check(void);
int bridge__register_local_connections(void);
int bridge__add_topic(struct dimq__bridge *bridge, const char *topic, enum dimq__bridge_direction direction, uint8_t qos, const char *local_prefix, const char *remote_prefix);
int bridge__remap_topic_in(struct dimq *context, char **topic);
#endif

/* ============================================================
 * IO multiplex related functions
 * ============================================================ */
int mux__init(struct dimq__listener_sock *listensock, int listensock_count);
int mux__loop_prepare(void);
int mux__add_out(struct dimq *context);
int mux__remove_out(struct dimq *context);
int mux__add_in(struct dimq *context);
int mux__delete(struct dimq *context);
int mux__wait(void);
int mux__handle(struct dimq__listener_sock *listensock, int listensock_count);
int mux__cleanup(void);

/* ============================================================
 * Listener related functions
 * ============================================================ */
void listener__set_defaults(struct dimq__listener *listener);
void listeners__reload_all_certificates(void);
#ifdef WITH_WEBSOCKETS
void listeners__add_websockets(struct lws_context *ws_context, dimq_sock_t fd);
#endif

/* ============================================================
 * Plugin related functions
 * ============================================================ */
int plugin__load_v5(struct dimq__listener *listener, struct dimq__auth_plugin *plugin, struct dimq_opt *auth_options, int auth_option_count, void *lib);
void plugin__handle_disconnect(struct dimq *context, int reason);
int plugin__handle_message(struct dimq *context, struct dimq_msg_store *stored);
void LIB_ERROR(void);
void plugin__handle_tick(void);

/* ============================================================
 * Property related functions
 * ============================================================ */
int keepalive__add(struct dimq *context);
void keepalive__check(void);
int keepalive__remove(struct dimq *context);
void keepalive__remove_all(void);
int keepalive__update(struct dimq *context);

/* ============================================================
 * Property related functions
 * ============================================================ */
int property__process_connect(struct dimq *context, dimq_property **props);
int property__process_will(struct dimq *context, struct dimq_message_all *msg, dimq_property **props);
int property__process_disconnect(struct dimq *context, dimq_property **props);

/* ============================================================
 * Retain tree related functions
 * ============================================================ */
int retain__init(void);
void retain__clean(struct dimq__retainhier **retainhier);
int retain__queue(struct dimq *context, const char *sub, uint8_t sub_qos, uint32_t subscription_identifier);
int retain__store(const char *topic, struct dimq_msg_store *stored, char **split_topics);

/* ============================================================
 * Security related functions
 * ============================================================ */
int acl__find_acls(struct dimq *context);
int dimq_security_module_init(void);
int dimq_security_module_cleanup(void);

int dimq_security_init(bool reload);
int dimq_security_apply(void);
int dimq_security_cleanup(bool reload);
int dimq_acl_check(struct dimq *context, const char *topic, uint32_t payloadlen, void* payload, uint8_t qos, bool retain, int access);
int dimq_unpwd_check(struct dimq *context);
int dimq_psk_key_get(struct dimq *context, const char *hint, const char *identity, char *key, int max_key_len);

int dimq_security_init_default(bool reload);
int dimq_security_apply_default(void);
int dimq_security_cleanup_default(bool reload);
int dimq_psk_key_get_default(struct dimq *context, const char *hint, const char *identity, char *key, int max_key_len);

int dimq_security_auth_start(struct dimq *context, bool reauth, const void *data_in, uint16_t data_in_len, void **data_out, uint16_t *data_out_len);
int dimq_security_auth_continue(struct dimq *context, const void *data_in, uint16_t data_len, void **data_out, uint16_t *data_out_len);

void unpwd__free_item(struct dimq__unpwd **unpwd, struct dimq__unpwd *item);

/* ============================================================
 * Session expiry
 * ============================================================ */
int session_expiry__add(struct dimq *context);
void session_expiry__remove(struct dimq *context);
void session_expiry__remove_all(void);
void session_expiry__check(void);
void session_expiry__send_all(void);

/* ============================================================
 * Signals
 * ============================================================ */
void handle_sigint(int signal);
void handle_sigusr1(int signal);
void handle_sigusr2(int signal);
#ifdef SIGHUP
void handle_sighup(int signal);
#endif

/* ============================================================
 * Window service and signal related functions
 * ============================================================ */
#if defined(WIN32) || defined(__CYGWIN__)
void service_install(void);
void service_uninstall(void);
void service_run(void);

DWORD WINAPI SigThreadProc(void* data);
#endif

/* ============================================================
 * Websockets related functions
 * ============================================================ */
#ifdef WITH_WEBSOCKETS
void dimq_websockets_init(struct dimq__listener *listener, const struct dimq__config *conf);
#endif
void do_disconnect(struct dimq *context, int reason);

/* ============================================================
 * Will delay
 * ============================================================ */
int will_delay__add(struct dimq *context);
void will_delay__check(void);
void will_delay__send_all(void);
void will_delay__remove(struct dimq *dimq);


/* ============================================================
 * Other
 * ============================================================ */
#ifdef WITH_XTREPORT
void xtreport(void);
#endif

#endif
