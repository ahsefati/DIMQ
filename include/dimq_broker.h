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
*/

/*
 * File: dimq_broker.h
 *
 * This header contains functions for use by plugins.
 */
#ifndef dimq_BROKER_H
#define dimq_BROKER_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) && defined(dimq_EXPORTS)
#	define dimq_EXPORT  __declspec(dllexport)
#else
#	define dimq_EXPORT
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

struct dimq;
typedef struct mqtt5__property dimq_property;

enum dimq_protocol {
	mp_mqtt,
	mp_mqttsn,
	mp_websockets
};

/* =========================================================================
 *
 * Section: Register callbacks.
 *
 * ========================================================================= */

/* Callback events */
enum dimq_plugin_event {
	dimq_EVT_RELOAD = 1,
	dimq_EVT_ACL_CHECK = 2,
	dimq_EVT_BASIC_AUTH = 3,
	dimq_EVT_EXT_AUTH_START = 4,
	dimq_EVT_EXT_AUTH_CONTINUE = 5,
	dimq_EVT_CONTROL = 6,
	dimq_EVT_MESSAGE = 7,
	dimq_EVT_PSK_KEY = 8,
	dimq_EVT_TICK = 9,
	dimq_EVT_DISCONNECT = 10,
};

/* Data for the dimq_EVT_RELOAD event */
struct dimq_evt_reload {
	void *future;
	struct dimq_opt *options;
	int option_count;
	void *future2[4];
};

/* Data for the dimq_EVT_ACL_CHECK event */
struct dimq_evt_acl_check {
	void *future;
	struct dimq *client;
	const char *topic;
	const void *payload;
	dimq_property *properties;
	int access;
	uint32_t payloadlen;
	uint8_t qos;
	bool retain;
	void *future2[4];
};

/* Data for the dimq_EVT_BASIC_AUTH event */
struct dimq_evt_basic_auth {
	void *future;
	struct dimq *client;
	char *username;
	char *password;
	void *future2[4];
};

/* Data for the dimq_EVT_PSK_KEY event */
struct dimq_evt_psk_key {
	void *future;
	struct dimq *client;
	const char *hint;
	const char *identity;
	char *key;
	int max_key_len;
	void *future2[4];
};

/* Data for the dimq_EVT_EXTENDED_AUTH event */
struct dimq_evt_extended_auth {
	void *future;
	struct dimq *client;
	const void *data_in;
	void *data_out;
	uint16_t data_in_len;
	uint16_t data_out_len;
	const char *auth_method;
	void *future2[3];
};

/* Data for the dimq_EVT_CONTROL event */
struct dimq_evt_control {
	void *future;
	struct dimq *client;
	const char *topic;
	const void *payload;
	const dimq_property *properties;
	char *reason_string;
	uint32_t payloadlen;
	uint8_t qos;
	uint8_t reason_code;
	bool retain;
	void *future2[4];
};

/* Data for the dimq_EVT_MESSAGE event */
struct dimq_evt_message {
	void *future;
	struct dimq *client;
	char *topic;
	void *payload;
	dimq_property *properties;
	char *reason_string;
	uint32_t payloadlen;
	uint8_t qos;
	uint8_t reason_code;
	bool retain;
	void *future2[4];
};


/* Data for the dimq_EVT_TICK event */
struct dimq_evt_tick {
	void *future;
	long now_ns;
	long next_ns;
	time_t now_s;
	time_t next_s;
	void *future2[4];
};

/* Data for the dimq_EVT_DISCONNECT event */
struct dimq_evt_disconnect {
	void *future;
	struct dimq *client;
	int reason;
	void *future2[4];
};


/* Callback definition */
typedef int (*dimq_FUNC_generic_callback)(int, void *, void *);

typedef struct dimq_plugin_id_t dimq_plugin_id_t;

/*
 * Function: dimq_callback_register
 *
 * Register a callback for an event.
 *
 * Parameters:
 *  identifier - the plugin identifier, as provided by <dimq_plugin_init>.
 *  event - the event to register a callback for. Can be one of:
 *          * dimq_EVT_RELOAD
 *          * dimq_EVT_ACL_CHECK
 *          * dimq_EVT_BASIC_AUTH
 *          * dimq_EVT_EXT_AUTH_START
 *          * dimq_EVT_EXT_AUTH_CONTINUE
 *          * dimq_EVT_CONTROL
 *          * dimq_EVT_MESSAGE
 *          * dimq_EVT_PSK_KEY
 *          * dimq_EVT_TICK
 *          * dimq_EVT_DISCONNECT
 *  cb_func - the callback function
 *  event_data - event specific data
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if cb_func is NULL
 *	dimq_ERR_NOMEM - on out of memory
 *	dimq_ERR_ALREADY_EXISTS - if cb_func has already been registered for this event
 *	dimq_ERR_NOT_SUPPORTED - if the event is not supported
 */
dimq_EXPORT int dimq_callback_register(
		dimq_plugin_id_t *identifier,
		int event,
		dimq_FUNC_generic_callback cb_func,
		const void *event_data,
		void *userdata);

/*
 * Function: dimq_callback_unregister
 *
 * Unregister a previously registered callback function.
 *
 * Parameters:
 *  identifier - the plugin identifier, as provided by <dimq_plugin_init>.
 *  event - the event to register a callback for. Can be one of:
 *          * dimq_EVT_RELOAD
 *          * dimq_EVT_ACL_CHECK
 *          * dimq_EVT_BASIC_AUTH
 *          * dimq_EVT_EXT_AUTH_START
 *          * dimq_EVT_EXT_AUTH_CONTINUE
 *          * dimq_EVT_CONTROL
 *          * dimq_EVT_MESSAGE
 *          * dimq_EVT_PSK_KEY
 *          * dimq_EVT_TICK
 *          * dimq_EVT_DISCONNECT
 *  cb_func - the callback function
 *  event_data - event specific data
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if cb_func is NULL
 *	dimq_ERR_NOT_FOUND - if cb_func was not registered for this event
 *	dimq_ERR_NOT_SUPPORTED - if the event is not supported
 */
dimq_EXPORT int dimq_callback_unregister(
		dimq_plugin_id_t *identifier,
		int event,
		dimq_FUNC_generic_callback cb_func,
		const void *event_data);


/* =========================================================================
 *
 * Section: Memory allocation.
 *
 * Use these functions when allocating or freeing memory to have your memory
 * included in the memory tracking on the broker.
 *
 * ========================================================================= */

/*
 * Function: dimq_calloc
 */
dimq_EXPORT void *dimq_calloc(size_t nmemb, size_t size);

/*
 * Function: dimq_free
 */
dimq_EXPORT void dimq_free(void *mem);

/*
 * Function: dimq_malloc
 */
dimq_EXPORT void *dimq_malloc(size_t size);

/*
 * Function: dimq_realloc
 */
dimq_EXPORT void *dimq_realloc(void *ptr, size_t size);

/*
 * Function: dimq_strdup
 */
dimq_EXPORT char *dimq_strdup(const char *s);

/* =========================================================================
 *
 * Section: Utility Functions
 *
 * Use these functions from within your plugin.
 *
 * ========================================================================= */


/*
 * Function: dimq_log_printf
 *
 * Write a log message using the broker configured logging.
 *
 * Parameters:
 * 	level -    Log message priority. Can currently be one of:
 *
 *             * dimq_LOG_INFO
 *             * dimq_LOG_NOTICE
 *             * dimq_LOG_WARNING
 *             * dimq_LOG_ERR
 *             * dimq_LOG_DEBUG
 *             * dimq_LOG_SUBSCRIBE (not recommended for use by plugins)
 *             * dimq_LOG_UNSUBSCRIBE (not recommended for use by plugins)
 *
 *             These values are defined in dimq.h.
 *
 *	fmt, ... - printf style format and arguments.
 */
dimq_EXPORT void dimq_log_printf(int level, const char *fmt, ...);


/* =========================================================================
 *
 * Client Functions
 *
 * Use these functions to access client information.
 *
 * ========================================================================= */

/*
 * Function: dimq_client_address
 *
 * Retrieve the IP address of the client as a string.
 */
dimq_EXPORT const char *dimq_client_address(const struct dimq *client);


/*
 * Function: dimq_client_clean_session
 *
 * Retrieve the clean session flag value for a client.
 */
dimq_EXPORT bool dimq_client_clean_session(const struct dimq *client);


/*
 * Function: dimq_client_id
 *
 * Retrieve the client id associated with a client.
 */
dimq_EXPORT const char *dimq_client_id(const struct dimq *client);


/*
 * Function: dimq_client_keepalive
 *
 * Retrieve the keepalive value for a client.
 */
dimq_EXPORT int dimq_client_keepalive(const struct dimq *client);


/*
 * Function: dimq_client_certificate
 *
 * If TLS support is enabled, return the certificate provided by a client as an
 * X509 pointer from openssl. If the client did not provide a certificate, then
 * NULL will be returned. This function will only ever return a non-NULL value
 * if the `require_certificate` option is set to true.
 *
 * When you have finished with the x509 pointer, it must be freed using
 * X509_free().
 *
 * If TLS is not supported, this function will always return NULL.
 */
dimq_EXPORT void *dimq_client_certificate(const struct dimq *client);


/*
 * Function: dimq_client_protocol
 *
 * Retrieve the protocol with which the client has connected. Can be one of:
 *
 * mp_mqtt (MQTT over TCP)
 * mp_mqttsn (MQTT-SN)
 * mp_websockets (MQTT over Websockets)
 */
dimq_EXPORT int dimq_client_protocol(const struct dimq *client);


/*
 * Function: dimq_client_protocol_version
 *
 * Retrieve the MQTT protocol version with which the client has connected. Can be one of:
 *
 * Returns:
 *   3 - for MQTT v3 / v3.1
 *   4 - for MQTT v3.1.1
 *   5 - for MQTT v5
 */
dimq_EXPORT int dimq_client_protocol_version(const struct dimq *client);


/*
 * Function: dimq_client_sub_count
 *
 * Retrieve the number of subscriptions that have been made by a client.
 */
dimq_EXPORT int dimq_client_sub_count(const struct dimq *client);


/*
 * Function: dimq_client_username
 *
 * Retrieve the username associated with a client.
 */
dimq_EXPORT const char *dimq_client_username(const struct dimq *client);


/* Function: dimq_set_username
 *
 * Set the username for a client.
 *
 * This removes and replaces the current username for a client and hence
 * updates its access.
 *
 * username can be NULL, in which case the client will become anonymous, but
 * must not be zero length.
 *
 * In the case of error, the client will be left with its original username.
 *
 * Returns:
 *   dimq_ERR_SUCCESS - on success
 *   dimq_ERR_INVAL - if client is NULL, or if username is zero length
 *   dimq_ERR_NOMEM - on out of memory
 */
dimq_EXPORT int dimq_set_username(struct dimq *client, const char *username);


/* =========================================================================
 *
 * Section: Client control
 *
 * ========================================================================= */

/* Function: dimq_kick_client_by_clientid
 *
 * Forcefully disconnect a client from the broker.
 *
 * If clientid != NULL, then the client with the matching client id is
 *   disconnected from the broker.
 * If clientid == NULL, then all clients are disconnected from the broker.
 *
 * If with_will == true, then if the client has a Last Will and Testament
 *   defined then this will be sent. If false, the LWT will not be sent.
 */
dimq_EXPORT int dimq_kick_client_by_clientid(const char *clientid, bool with_will);

/* Function: dimq_kick_client_by_username
 *
 * Forcefully disconnect a client from the broker.
 *
 * If username != NULL, then all clients with a matching username are kicked
 *   from the broker.
 * If username == NULL, then all clients that do not have a username are
 *   kicked.
 *
 * If with_will == true, then if the client has a Last Will and Testament
 *   defined then this will be sent. If false, the LWT will not be sent.
 */
dimq_EXPORT int dimq_kick_client_by_username(const char *username, bool with_will);


/* =========================================================================
 *
 * Section: Publishing functions
 *
 * ========================================================================= */

/* Function: dimq_broker_publish
 *
 * Publish a message from within a plugin.
 *
 * This function allows a plugin to publish a message. Messages published in
 * this way are treated as coming from the broker and so will not be passed to
 * `dimq_auth_acl_check(, dimq_ACL_WRITE, , )` for checking. Read access
 * will be enforced as normal for individual clients when they are due to
 * receive the message.
 *
 * It can be used to send messages to all clients that have a matching
 * subscription, or to a single client whether or not it has a matching
 * subscription.
 *
 * Parameters:
 *  clientid -   optional string. If set to NULL, the message is delivered to all
 *               clients. If non-NULL, the message is delivered only to the
 *               client with the corresponding client id. If the client id
 *               specified is not connected, the message will be dropped.
 *  topic -      message topic
 *  payloadlen - payload length in bytes. Can be 0 for an empty payload.
 *  payload -    payload bytes. If payloadlen > 0 this must not be NULL. Must
 *               be allocated on the heap. Will be freed by dimq after use if the
 *               function returns success.
 *  qos -        message QoS to use.
 *  retain -     should retain be set on the message. This does not apply if
 *               clientid is non-NULL.
 *  properties - MQTT v5 properties to attach to the message. If the function
 *               returns success, then properties is owned by the broker and
 *               will be freed at a later point.
 *
 * Returns:
 *   dimq_ERR_SUCCESS - on success
 *   dimq_ERR_INVAL - if topic is NULL, if payloadlen < 0, if payloadlen > 0
 *                    and payload is NULL, if qos is not 0, 1, or 2.
 *   dimq_ERR_NOMEM - on out of memory
 */
dimq_EXPORT int dimq_broker_publish(
		const char *clientid,
		const char *topic,
		int payloadlen,
		void *payload,
		int qos,
		bool retain,
		dimq_property *properties);


/* Function: dimq_broker_publish_copy
 *
 * Publish a message from within a plugin.
 *
 * This function is identical to dimq_broker_publish, except that a copy
 * of `payload` is taken.
 *
 * Parameters:
 *  clientid -   optional string. If set to NULL, the message is delivered to all
 *               clients. If non-NULL, the message is delivered only to the
 *               client with the corresponding client id. If the client id
 *               specified is not connected, the message will be dropped.
 *  topic -      message topic
 *  payloadlen - payload length in bytes. Can be 0 for an empty payload.
 *  payload -    payload bytes. If payloadlen > 0 this must not be NULL.
 *	             Memory remains the property of the calling function.
 *  qos -        message QoS to use.
 *  retain -     should retain be set on the message. This does not apply if
 *               clientid is non-NULL.
 *  properties - MQTT v5 properties to attach to the message. If the function
 *               returns success, then properties is owned by the broker and
 *               will be freed at a later point.
 *
 * Returns:
 *   dimq_ERR_SUCCESS - on success
 *   dimq_ERR_INVAL - if topic is NULL, if payloadlen < 0, if payloadlen > 0
 *                    and payload is NULL, if qos is not 0, 1, or 2.
 *   dimq_ERR_NOMEM - on out of memory
 */
dimq_EXPORT int dimq_broker_publish_copy(
		const char *clientid,
		const char *topic,
		int payloadlen,
		const void *payload,
		int qos,
		bool retain,
		dimq_property *properties);

#ifdef __cplusplus
}
#endif

#endif
