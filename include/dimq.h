/*
Copyright (c) 2010-2020 Roger Light <roger@atchoo.org>

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

#ifndef dimq_H
#define dimq_H

/*
 * File: dimq.h
 *
 * This header contains functions and definitions for use with libdimq, the dimq client library.
 *
 * The definitions are also used in dimq broker plugins, and some functions are available to plugins.
 */
#ifdef __cplusplus
extern "C" {
#endif


#ifdef WIN32
#  ifdef dimq_EXPORTS
#    define libdimq_EXPORT __declspec(dllexport)
#  else
#    ifndef LIBdimq_STATIC
#      ifdef libdimq_EXPORTS
#        define libdimq_EXPORT  __declspec(dllexport)
#      else
#        define libdimq_EXPORT  __declspec(dllimport)
#      endif
#    else
#      define libdimq_EXPORT
#    endif
#  endif
#else
#  define libdimq_EXPORT
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900 && !defined(bool)
#	ifndef __cplusplus
#		define bool char
#		define true 1
#		define false 0
#	endif
#else
#	ifndef __cplusplus
#		include <stdbool.h>
#	endif
#endif

#include <stddef.h>
#include <stdint.h>

#define LIBdimq_MAJOR 2
#define LIBdimq_MINOR 0
#define LIBdimq_REVISION 12
/* LIBdimq_VERSION_NUMBER looks like 1002001 for e.g. version 1.2.1. */
#define LIBdimq_VERSION_NUMBER (LIBdimq_MAJOR*1000000+LIBdimq_MINOR*1000+LIBdimq_REVISION)

/* Log types */
#define dimq_LOG_NONE			0
#define dimq_LOG_INFO			(1<<0)
#define dimq_LOG_NOTICE			(1<<1)
#define dimq_LOG_WARNING		(1<<2)
#define dimq_LOG_ERR			(1<<3)
#define dimq_LOG_DEBUG			(1<<4)
#define dimq_LOG_SUBSCRIBE		(1<<5)
#define dimq_LOG_UNSUBSCRIBE	(1<<6)
#define dimq_LOG_WEBSOCKETS		(1<<7)
#define dimq_LOG_INTERNAL		0x80000000U
#define dimq_LOG_ALL			0xFFFFFFFFU

/* Error values */
enum dimq_err_t {
	dimq_ERR_AUTH_CONTINUE = -4,
	dimq_ERR_NO_SUBSCRIBERS = -3,
	dimq_ERR_SUB_EXISTS = -2,
	dimq_ERR_CONN_PENDING = -1,
	dimq_ERR_SUCCESS = 0,
	dimq_ERR_NOMEM = 1,
	dimq_ERR_PROTOCOL = 2,
	dimq_ERR_INVAL = 3,
	dimq_ERR_NO_CONN = 4,
	dimq_ERR_CONN_REFUSED = 5,
	dimq_ERR_NOT_FOUND = 6,
	dimq_ERR_CONN_LOST = 7,
	dimq_ERR_TLS = 8,
	dimq_ERR_PAYLOAD_SIZE = 9,
	dimq_ERR_NOT_SUPPORTED = 10,
	dimq_ERR_AUTH = 11,
	dimq_ERR_ACL_DENIED = 12,
	dimq_ERR_UNKNOWN = 13,
	dimq_ERR_ERRNO = 14,
	dimq_ERR_EAI = 15,
	dimq_ERR_PROXY = 16,
	dimq_ERR_PLUGIN_DEFER = 17,
	dimq_ERR_MALFORMED_UTF8 = 18,
	dimq_ERR_KEEPALIVE = 19,
	dimq_ERR_LOOKUP = 20,
	dimq_ERR_MALFORMED_PACKET = 21,
	dimq_ERR_DUPLICATE_PROPERTY = 22,
	dimq_ERR_TLS_HANDSHAKE = 23,
	dimq_ERR_QOS_NOT_SUPPORTED = 24,
	dimq_ERR_OVERSIZE_PACKET = 25,
	dimq_ERR_OCSP = 26,
	dimq_ERR_TIMEOUT = 27,
	dimq_ERR_RETAIN_NOT_SUPPORTED = 28,
	dimq_ERR_TOPIC_ALIAS_INVALID = 29,
	dimq_ERR_ADMINISTRATIVE_ACTION = 30,
	dimq_ERR_ALREADY_EXISTS = 31,
};

/* Option values */
enum dimq_opt_t {
	dimq_OPT_PROTOCOL_VERSION = 1,
	dimq_OPT_SSL_CTX = 2,
	dimq_OPT_SSL_CTX_WITH_DEFAULTS = 3,
	dimq_OPT_RECEIVE_MAXIMUM = 4,
	dimq_OPT_SEND_MAXIMUM = 5,
	dimq_OPT_TLS_KEYFORM = 6,
	dimq_OPT_TLS_ENGINE = 7,
	dimq_OPT_TLS_ENGINE_KPASS_SHA1 = 8,
	dimq_OPT_TLS_OCSP_REQUIRED = 9,
	dimq_OPT_TLS_ALPN = 10,
	dimq_OPT_TCP_NODELAY = 11,
	dimq_OPT_BIND_ADDRESS = 12,
	dimq_OPT_TLS_USE_OS_CERTS = 13,
};


/* MQTT specification restricts client ids to a maximum of 23 characters */
#define dimq_MQTT_ID_MAX_LENGTH 23

#define MQTT_PROTOCOL_V31 3
#define MQTT_PROTOCOL_V311 4
#define MQTT_PROTOCOL_V5 5

struct dimq_message{
	int mid;
	char *topic;
	void *payload;
	int payloadlen;
	int qos;
	bool retain;
};

struct dimq;
typedef struct mqtt5__property dimq_property;

/*
 * Topic: Threads
 *	libdimq provides thread safe operation, with the exception of
 *	<dimq_lib_init> which is not thread safe.
 *
 *	If the library has been compiled without thread support it is *not*
 *	guaranteed to be thread safe.
 *
 *	If your application uses threads you must use <dimq_threaded_set> to
 *	tell the library this is the case, otherwise it makes some optimisations
 *	for the single threaded case that may result in unexpected behaviour for
 *	the multi threaded case.
 */
/***************************************************
 * Important note
 *
 * The following functions that deal with network operations will return
 * dimq_ERR_SUCCESS on success, but this does not mean that the operation has
 * taken place. An attempt will be made to write the network data, but if the
 * socket is not available for writing at that time then the packet will not be
 * sent. To ensure the packet is sent, call dimq_loop() (which must also
 * be called to process incoming network data).
 * This is especially important when disconnecting a client that has a will. If
 * the broker does not receive the DISCONNECT command, it will assume that the
 * client has disconnected unexpectedly and send the will.
 *
 * dimq_connect()
 * dimq_disconnect()
 * dimq_subscribe()
 * dimq_unsubscribe()
 * dimq_publish()
 ***************************************************/


/* ======================================================================
 *
 * Section: Library version, init, and cleanup
 *
 * ====================================================================== */
/*
 * Function: dimq_lib_version
 *
 * Can be used to obtain version information for the dimq library.
 * This allows the application to compare the library version against the
 * version it was compiled against by using the LIBdimq_MAJOR,
 * LIBdimq_MINOR and LIBdimq_REVISION defines.
 *
 * Parameters:
 *  major -    an integer pointer. If not NULL, the major version of the
 *             library will be returned in this variable.
 *  minor -    an integer pointer. If not NULL, the minor version of the
 *             library will be returned in this variable.
 *  revision - an integer pointer. If not NULL, the revision of the library will
 *             be returned in this variable.
 *
 * Returns:
 *	LIBdimq_VERSION_NUMBER - which is a unique number based on the major,
 *		minor and revision values.
 * See Also:
 * 	<dimq_lib_cleanup>, <dimq_lib_init>
 */
libdimq_EXPORT int dimq_lib_version(int *major, int *minor, int *revision);

/*
 * Function: dimq_lib_init
 *
 * Must be called before any other dimq functions.
 *
 * This function is *not* thread safe.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_UNKNOWN - on Windows, if sockets couldn't be initialized.
 *
 * See Also:
 * 	<dimq_lib_cleanup>, <dimq_lib_version>
 */
libdimq_EXPORT int dimq_lib_init(void);

/*
 * Function: dimq_lib_cleanup
 *
 * Call to free resources associated with the library.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - always
 *
 * See Also:
 * 	<dimq_lib_init>, <dimq_lib_version>
 */
libdimq_EXPORT int dimq_lib_cleanup(void);


/* ======================================================================
 *
 * Section: Client creation, destruction, and reinitialisation
 *
 * ====================================================================== */
/*
 * Function: dimq_new
 *
 * Create a new dimq client instance.
 *
 * Parameters:
 * 	id -            String to use as the client id. If NULL, a random client id
 * 	                will be generated. If id is NULL, clean_session must be true.
 * 	clean_session - set to true to instruct the broker to clean all messages
 *                  and subscriptions on disconnect, false to instruct it to
 *                  keep them. See the man page mqtt(7) for more details.
 *                  Note that a client will never discard its own outgoing
 *                  messages on disconnect. Calling <dimq_connect> or
 *                  <dimq_reconnect> will cause the messages to be resent.
 *                  Use <dimq_reinitialise> to reset a client to its
 *                  original state.
 *                  Must be set to true if the id parameter is NULL.
 * 	obj -           A user pointer that will be passed as an argument to any
 *                  callbacks that are specified.
 *
 * Returns:
 * 	Pointer to a struct dimq on success.
 * 	NULL on failure. Interrogate errno to determine the cause for the failure:
 *      - ENOMEM on out of memory.
 *      - EINVAL on invalid input parameters.
 *
 * See Also:
 * 	<dimq_reinitialise>, <dimq_destroy>, <dimq_user_data_set>
 */
libdimq_EXPORT struct dimq *dimq_new(const char *id, bool clean_session, void *obj);

/*
 * Function: dimq_destroy
 *
 * Use to free memory associated with a dimq client instance.
 *
 * Parameters:
 * 	dimq - a struct dimq pointer to free.
 *
 * See Also:
 * 	<dimq_new>, <dimq_reinitialise>
 */
libdimq_EXPORT void dimq_destroy(struct dimq *dimq);

/*
 * Function: dimq_reinitialise
 *
 * This function allows an existing dimq client to be reused. Call on a
 * dimq instance to close any open network connections, free memory
 * and reinitialise the client with the new parameters. The end result is the
 * same as the output of <dimq_new>.
 *
 * Parameters:
 * 	dimq -          a valid dimq instance.
 * 	id -            string to use as the client id. If NULL, a random client id
 * 	                will be generated. If id is NULL, clean_session must be true.
 * 	clean_session - set to true to instruct the broker to clean all messages
 *                  and subscriptions on disconnect, false to instruct it to
 *                  keep them. See the man page mqtt(7) for more details.
 *                  Must be set to true if the id parameter is NULL.
 * 	obj -           A user pointer that will be passed as an argument to any
 *                  callbacks that are specified.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 *
 * See Also:
 * 	<dimq_new>, <dimq_destroy>
 */
libdimq_EXPORT int dimq_reinitialise(struct dimq *dimq, const char *id, bool clean_session, void *obj);


/* ======================================================================
 *
 * Section: Will
 *
 * ====================================================================== */
/*
 * Function: dimq_will_set
 *
 * Configure will information for a dimq instance. By default, clients do
 * not have a will.  This must be called before calling <dimq_connect>.
 *
 * It is valid to use this function for clients using all MQTT protocol versions.
 * If you need to set MQTT v5 Will properties, use <dimq_will_set_v5> instead.
 *
 * Parameters:
 * 	dimq -       a valid dimq instance.
 * 	topic -      the topic on which to publish the will.
 * 	payloadlen - the size of the payload (bytes). Valid values are between 0 and
 *               268,435,455.
 * 	payload -    pointer to the data to send. If payloadlen > 0 this must be a
 *               valid memory location.
 * 	qos -        integer value 0, 1 or 2 indicating the Quality of Service to be
 *               used for the will.
 * 	retain -     set to true to make the will a retained message.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS -      on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_PAYLOAD_SIZE -   if payloadlen is too large.
 * 	dimq_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8.
 */
libdimq_EXPORT int dimq_will_set(struct dimq *dimq, const char *topic, int payloadlen, const void *payload, int qos, bool retain);

/*
 * Function: dimq_will_set_v5
 *
 * Configure will information for a dimq instance, with attached
 * properties. By default, clients do not have a will.  This must be called
 * before calling <dimq_connect>.
 *
 * If the dimq instance `dimq` is using MQTT v5, the `properties` argument
 * will be applied to the Will. For MQTT v3.1.1 and below, the `properties`
 * argument will be ignored.
 *
 * Set your client to use MQTT v5 immediately after it is created:
 *
 * dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 *
 * Parameters:
 * 	dimq -       a valid dimq instance.
 * 	topic -      the topic on which to publish the will.
 * 	payloadlen - the size of the payload (bytes). Valid values are between 0 and
 *               268,435,455.
 * 	payload -    pointer to the data to send. If payloadlen > 0 this must be a
 *               valid memory location.
 * 	qos -        integer value 0, 1 or 2 indicating the Quality of Service to be
 *               used for the will.
 * 	retain -     set to true to make the will a retained message.
 * 	properties - list of MQTT 5 properties. Can be NULL. On success only, the
 * 	             property list becomes the property of libdimq once this
 * 	             function is called and will be freed by the library. The
 * 	             property list must be freed by the application on error.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS -      on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_PAYLOAD_SIZE -   if payloadlen is too large.
 * 	dimq_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8.
 * 	dimq_ERR_NOT_SUPPORTED -  if properties is not NULL and the client is not
 * 	                          using MQTT v5
 * 	dimq_ERR_PROTOCOL -       if a property is invalid for use with wills.
 *	dimq_ERR_DUPLICATE_PROPERTY - if a property is duplicated where it is forbidden.
 */
libdimq_EXPORT int dimq_will_set_v5(struct dimq *dimq, const char *topic, int payloadlen, const void *payload, int qos, bool retain, dimq_property *properties);

/*
 * Function: dimq_will_clear
 *
 * Remove a previously configured will. This must be called before calling
 * <dimq_connect>.
 *
 * Parameters:
 * 	dimq - a valid dimq instance.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 */
libdimq_EXPORT int dimq_will_clear(struct dimq *dimq);


/* ======================================================================
 *
 * Section: Username and password
 *
 * ====================================================================== */
/*
 * Function: dimq_username_pw_set
 *
 * Configure username and password for a dimq instance. By default, no
 * username or password will be sent. For v3.1 and v3.1.1 clients, if username
 * is NULL, the password argument is ignored.
 *
 * This is must be called before calling <dimq_connect>.
 *
 * Parameters:
 * 	dimq -     a valid dimq instance.
 * 	username - the username to send as a string, or NULL to disable
 *             authentication.
 * 	password - the password to send as a string. Set to NULL when username is
 * 	           valid in order to send just a username.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 */
libdimq_EXPORT int dimq_username_pw_set(struct dimq *dimq, const char *username, const char *password);


/* ======================================================================
 *
 * Section: Connecting, reconnecting, disconnecting
 *
 * ====================================================================== */
/*
 * Function: dimq_connect
 *
 * Connect to an MQTT broker.
 *
 * It is valid to use this function for clients using all MQTT protocol versions.
 * If you need to set MQTT v5 CONNECT properties, use <dimq_connect_bind_v5>
 * instead.
 *
 * Parameters:
 * 	dimq -      a valid dimq instance.
 * 	host -      the hostname or ip address of the broker to connect to.
 * 	port -      the network port to connect to. Usually 1883.
 * 	keepalive - the number of seconds after which the broker should send a PING
 *              message to the client if no other messages have been exchanged
 *              in that time.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid, which could be any of:
 * 	                   * dimq == NULL
 * 	                   * host == NULL
 * 	                   * port < 0
 * 	                   * keepalive < 5
 * 	dimq_ERR_ERRNO -   if a system call returned an error. The variable errno
 *                     contains the error code, even on Windows.
 *                     Use strerror_r() where available or FormatMessage() on
 *                     Windows.
 *
 * See Also:
 * 	<dimq_connect_bind>, <dimq_connect_async>, <dimq_reconnect>, <dimq_disconnect>, <dimq_tls_set>
 */
libdimq_EXPORT int dimq_connect(struct dimq *dimq, const char *host, int port, int keepalive);

/*
 * Function: dimq_connect_bind
 *
 * Connect to an MQTT broker. This extends the functionality of
 * <dimq_connect> by adding the bind_address parameter. Use this function
 * if you need to restrict network communication over a particular interface.
 *
 * Parameters:
 * 	dimq -         a valid dimq instance.
 * 	host -         the hostname or ip address of the broker to connect to.
 * 	port -         the network port to connect to. Usually 1883.
 * 	keepalive -    the number of seconds after which the broker should send a PING
 *                 message to the client if no other messages have been exchanged
 *                 in that time.
 *  bind_address - the hostname or ip address of the local network interface to
 *                 bind to. If you do not want to bind to a specific interface,
 *                 set this to NULL.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_ERRNO -   if a system call returned an error. The variable errno
 *                     contains the error code, even on Windows.
 *                     Use strerror_r() where available or FormatMessage() on
 *                     Windows.
 *
 * See Also:
 * 	<dimq_connect>, <dimq_connect_async>, <dimq_connect_bind_async>
 */
libdimq_EXPORT int dimq_connect_bind(struct dimq *dimq, const char *host, int port, int keepalive, const char *bind_address);

/*
 * Function: dimq_connect_bind_v5
 *
 * Connect to an MQTT broker. This extends the functionality of
 * <dimq_connect> by adding the bind_address parameter and MQTT v5
 * properties. Use this function if you need to restrict network communication
 * over a particular interface.
 *
 * Use e.g. <dimq_property_add_string> and similar to create a list of
 * properties, then attach them to this publish. Properties need freeing with
 * <dimq_property_free_all>.
 *
 * If the dimq instance `dimq` is using MQTT v5, the `properties` argument
 * will be applied to the CONNECT message. For MQTT v3.1.1 and below, the
 * `properties` argument will be ignored.
 *
 * Set your client to use MQTT v5 immediately after it is created:
 *
 * dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 *
 * Parameters:
 * 	dimq -         a valid dimq instance.
 * 	host -         the hostname or ip address of the broker to connect to.
 * 	port -         the network port to connect to. Usually 1883.
 * 	keepalive -    the number of seconds after which the broker should send a PING
 *                 message to the client if no other messages have been exchanged
 *                 in that time.
 *  bind_address - the hostname or ip address of the local network interface to
 *                 bind to. If you do not want to bind to a specific interface,
 *                 set this to NULL.
 *  properties - the MQTT 5 properties for the connect (not for the Will).
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid, which could be any of:
 * 	                   * dimq == NULL
 * 	                   * host == NULL
 * 	                   * port < 0
 * 	                   * keepalive < 5
 * 	dimq_ERR_ERRNO -   if a system call returned an error. The variable errno
 *                     contains the error code, even on Windows.
 *                     Use strerror_r() where available or FormatMessage() on
 *                     Windows.
 *	dimq_ERR_DUPLICATE_PROPERTY - if a property is duplicated where it is forbidden.
 *	dimq_ERR_PROTOCOL - if any property is invalid for use with CONNECT.
 *
 * See Also:
 * 	<dimq_connect>, <dimq_connect_async>, <dimq_connect_bind_async>
 */
libdimq_EXPORT int dimq_connect_bind_v5(struct dimq *dimq, const char *host, int port, int keepalive, const char *bind_address, const dimq_property *properties);

/*
 * Function: dimq_connect_async
 *
 * Connect to an MQTT broker. This is a non-blocking call. If you use
 * <dimq_connect_async> your client must use the threaded interface
 * <dimq_loop_start>. If you need to use <dimq_loop>, you must use
 * <dimq_connect> to connect the client.
 *
 * May be called before or after <dimq_loop_start>.
 *
 * Parameters:
 * 	dimq -      a valid dimq instance.
 * 	host -      the hostname or ip address of the broker to connect to.
 * 	port -      the network port to connect to. Usually 1883.
 * 	keepalive - the number of seconds after which the broker should send a PING
 *              message to the client if no other messages have been exchanged
 *              in that time.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_ERRNO -   if a system call returned an error. The variable errno
 *                     contains the error code, even on Windows.
 *                     Use strerror_r() where available or FormatMessage() on
 *                     Windows.
 *
 * See Also:
 * 	<dimq_connect_bind_async>, <dimq_connect>, <dimq_reconnect>, <dimq_disconnect>, <dimq_tls_set>
 */
libdimq_EXPORT int dimq_connect_async(struct dimq *dimq, const char *host, int port, int keepalive);

/*
 * Function: dimq_connect_bind_async
 *
 * Connect to an MQTT broker. This is a non-blocking call. If you use
 * <dimq_connect_bind_async> your client must use the threaded interface
 * <dimq_loop_start>. If you need to use <dimq_loop>, you must use
 * <dimq_connect> to connect the client.
 *
 * This extends the functionality of <dimq_connect_async> by adding the
 * bind_address parameter. Use this function if you need to restrict network
 * communication over a particular interface.
 *
 * May be called before or after <dimq_loop_start>.
 *
 * Parameters:
 * 	dimq -         a valid dimq instance.
 * 	host -         the hostname or ip address of the broker to connect to.
 * 	port -         the network port to connect to. Usually 1883.
 * 	keepalive -    the number of seconds after which the broker should send a PING
 *                 message to the client if no other messages have been exchanged
 *                 in that time.
 *  bind_address - the hostname or ip address of the local network interface to
 *                 bind to. If you do not want to bind to a specific interface,
 *                 set this to NULL.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid, which could be any of:
 * 	                   * dimq == NULL
 * 	                   * host == NULL
 * 	                   * port < 0
 * 	                   * keepalive < 5
 * 	dimq_ERR_ERRNO -   if a system call returned an error. The variable errno
 *                     contains the error code, even on Windows.
 *                     Use strerror_r() where available or FormatMessage() on
 *                     Windows.
 *
 * See Also:
 * 	<dimq_connect_async>, <dimq_connect>, <dimq_connect_bind>
 */
libdimq_EXPORT int dimq_connect_bind_async(struct dimq *dimq, const char *host, int port, int keepalive, const char *bind_address);

/*
 * Function: dimq_connect_srv
 *
 * Connect to an MQTT broker.
 *
 * If you set `host` to `example.com`, then this call will attempt to retrieve
 * the DNS SRV record for `_secure-mqtt._tcp.example.com` or
 * `_mqtt._tcp.example.com` to discover which actual host to connect to.
 *
 * DNS SRV support is not usually compiled in to libdimq, use of this call
 * is not recommended.
 *
 * Parameters:
 * 	dimq -         a valid dimq instance.
 * 	host -         the hostname to search for an SRV record.
 * 	keepalive -    the number of seconds after which the broker should send a PING
 *                 message to the client if no other messages have been exchanged
 *                 in that time.
 *  bind_address - the hostname or ip address of the local network interface to
 *                 bind to. If you do not want to bind to a specific interface,
 *                 set this to NULL.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid, which could be any of:
 * 	                   * dimq == NULL
 * 	                   * host == NULL
 * 	                   * port < 0
 * 	                   * keepalive < 5
 * 	dimq_ERR_ERRNO -   if a system call returned an error. The variable errno
 *                     contains the error code, even on Windows.
 *                     Use strerror_r() where available or FormatMessage() on
 *                     Windows.
 *
 * See Also:
 * 	<dimq_connect_async>, <dimq_connect>, <dimq_connect_bind>
 */
libdimq_EXPORT int dimq_connect_srv(struct dimq *dimq, const char *host, int keepalive, const char *bind_address);

/*
 * Function: dimq_reconnect
 *
 * Reconnect to a broker.
 *
 * This function provides an easy way of reconnecting to a broker after a
 * connection has been lost. It uses the values that were provided in the
 * <dimq_connect> call. It must not be called before
 * <dimq_connect>.
 *
 * Parameters:
 * 	dimq - a valid dimq instance.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 * 	dimq_ERR_ERRNO -   if a system call returned an error. The variable errno
 *                     contains the error code, even on Windows.
 *                     Use strerror_r() where available or FormatMessage() on
 *                     Windows.
 *
 * See Also:
 * 	<dimq_connect>, <dimq_disconnect>, <dimq_reconnect_async>
 */
libdimq_EXPORT int dimq_reconnect(struct dimq *dimq);

/*
 * Function: dimq_reconnect_async
 *
 * Reconnect to a broker. Non blocking version of <dimq_reconnect>.
 *
 * This function provides an easy way of reconnecting to a broker after a
 * connection has been lost. It uses the values that were provided in the
 * <dimq_connect> or <dimq_connect_async> calls. It must not be
 * called before <dimq_connect>.
 *
 * Parameters:
 * 	dimq - a valid dimq instance.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 * 	dimq_ERR_ERRNO -   if a system call returned an error. The variable errno
 *                     contains the error code, even on Windows.
 *                     Use strerror_r() where available or FormatMessage() on
 *                     Windows.
 *
 * See Also:
 * 	<dimq_connect>, <dimq_disconnect>
 */
libdimq_EXPORT int dimq_reconnect_async(struct dimq *dimq);

/*
 * Function: dimq_disconnect
 *
 * Disconnect from the broker.
 *
 * It is valid to use this function for clients using all MQTT protocol versions.
 * If you need to set MQTT v5 DISCONNECT properties, use
 * <dimq_disconnect_v5> instead.
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NO_CONN -  if the client isn't connected to a broker.
 */
libdimq_EXPORT int dimq_disconnect(struct dimq *dimq);

/*
 * Function: dimq_disconnect_v5
 *
 * Disconnect from the broker, with attached MQTT properties.
 *
 * Use e.g. <dimq_property_add_string> and similar to create a list of
 * properties, then attach them to this publish. Properties need freeing with
 * <dimq_property_free_all>.
 *
 * If the dimq instance `dimq` is using MQTT v5, the `properties` argument
 * will be applied to the DISCONNECT message. For MQTT v3.1.1 and below, the
 * `properties` argument will be ignored.
 *
 * Set your client to use MQTT v5 immediately after it is created:
 *
 * dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *	reason_code - the disconnect reason code.
 * 	properties - a valid dimq_property list, or NULL.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NO_CONN -  if the client isn't connected to a broker.
 *	dimq_ERR_DUPLICATE_PROPERTY - if a property is duplicated where it is forbidden.
 *	dimq_ERR_PROTOCOL - if any property is invalid for use with DISCONNECT.
 */
libdimq_EXPORT int dimq_disconnect_v5(struct dimq *dimq, int reason_code, const dimq_property *properties);


/* ======================================================================
 *
 * Section: Publishing, subscribing, unsubscribing
 *
 * ====================================================================== */
/*
 * Function: dimq_publish
 *
 * Publish a message on a given topic.
 *
 * It is valid to use this function for clients using all MQTT protocol versions.
 * If you need to set MQTT v5 PUBLISH properties, use <dimq_publish_v5>
 * instead.
 *
 * Parameters:
 * 	dimq -       a valid dimq instance.
 * 	mid -        pointer to an int. If not NULL, the function will set this
 *               to the message id of this particular message. This can be then
 *               used with the publish callback to determine when the message
 *               has been sent.
 *               Note that although the MQTT protocol doesn't use message ids
 *               for messages with QoS=0, libdimq assigns them message ids
 *               so they can be tracked with this parameter.
 *  topic -      null terminated string of the topic to publish to.
 * 	payloadlen - the size of the payload (bytes). Valid values are between 0 and
 *               268,435,455.
 * 	payload -    pointer to the data to send. If payloadlen > 0 this must be a
 *               valid memory location.
 * 	qos -        integer value 0, 1 or 2 indicating the Quality of Service to be
 *               used for the message.
 * 	retain -     set to true to make the message retained.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS -        on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -        if the client isn't connected to a broker.
 *	dimq_ERR_PROTOCOL -       if there is a protocol error communicating with the
 *                            broker.
 * 	dimq_ERR_PAYLOAD_SIZE -   if payloadlen is too large.
 * 	dimq_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8
 *	dimq_ERR_QOS_NOT_SUPPORTED - if the QoS is greater than that supported by
 *	                             the broker.
 *	dimq_ERR_OVERSIZE_PACKET - if the resulting packet would be larger than
 *	                           supported by the broker.
 *
 * See Also:
 *	<dimq_max_inflight_messages_set>
 */
libdimq_EXPORT int dimq_publish(struct dimq *dimq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);


/*
 * Function: dimq_publish_v5
 *
 * Publish a message on a given topic, with attached MQTT properties.
 *
 * Use e.g. <dimq_property_add_string> and similar to create a list of
 * properties, then attach them to this publish. Properties need freeing with
 * <dimq_property_free_all>.
 *
 * If the dimq instance `dimq` is using MQTT v5, the `properties` argument
 * will be applied to the PUBLISH message. For MQTT v3.1.1 and below, the
 * `properties` argument will be ignored.
 *
 * Set your client to use MQTT v5 immediately after it is created:
 *
 * dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 *
 * Parameters:
 * 	dimq -       a valid dimq instance.
 * 	mid -        pointer to an int. If not NULL, the function will set this
 *               to the message id of this particular message. This can be then
 *               used with the publish callback to determine when the message
 *               has been sent.
 *               Note that although the MQTT protocol doesn't use message ids
 *               for messages with QoS=0, libdimq assigns them message ids
 *               so they can be tracked with this parameter.
 *  topic -      null terminated string of the topic to publish to.
 * 	payloadlen - the size of the payload (bytes). Valid values are between 0 and
 *               268,435,455.
 * 	payload -    pointer to the data to send. If payloadlen > 0 this must be a
 *               valid memory location.
 * 	qos -        integer value 0, 1 or 2 indicating the Quality of Service to be
 *               used for the message.
 * 	retain -     set to true to make the message retained.
 * 	properties - a valid dimq_property list, or NULL.
 *
 * Returns:
 * 	dimq_ERR_SUCCESS -        on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -        if the client isn't connected to a broker.
 *	dimq_ERR_PROTOCOL -       if there is a protocol error communicating with the
 *                            broker.
 * 	dimq_ERR_PAYLOAD_SIZE -   if payloadlen is too large.
 * 	dimq_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8
 *	dimq_ERR_DUPLICATE_PROPERTY - if a property is duplicated where it is forbidden.
 *	dimq_ERR_PROTOCOL - if any property is invalid for use with PUBLISH.
 *	dimq_ERR_QOS_NOT_SUPPORTED - if the QoS is greater than that supported by
 *	                             the broker.
 *	dimq_ERR_OVERSIZE_PACKET - if the resulting packet would be larger than
 *	                           supported by the broker.
 */
libdimq_EXPORT int dimq_publish_v5(
		struct dimq *dimq,
		int *mid,
		const char *topic,
		int payloadlen,
		const void *payload,
		int qos,
		bool retain,
		const dimq_property *properties);


/*
 * Function: dimq_subscribe
 *
 * Subscribe to a topic.
 *
 * It is valid to use this function for clients using all MQTT protocol versions.
 * If you need to set MQTT v5 SUBSCRIBE properties, use <dimq_subscribe_v5>
 * instead.
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *	mid -  a pointer to an int. If not NULL, the function will set this to
 *	       the message id of this particular message. This can be then used
 *	       with the subscribe callback to determine when the message has been
 *	       sent.
 *	sub -  the subscription pattern.
 *	qos -  the requested Quality of Service for this subscription.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -        on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -        if the client isn't connected to a broker.
 * 	dimq_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8
 *	dimq_ERR_OVERSIZE_PACKET - if the resulting packet would be larger than
 *	                           supported by the broker.
 */
libdimq_EXPORT int dimq_subscribe(struct dimq *dimq, int *mid, const char *sub, int qos);

/*
 * Function: dimq_subscribe_v5
 *
 * Subscribe to a topic, with attached MQTT properties.
 *
 * Use e.g. <dimq_property_add_string> and similar to create a list of
 * properties, then attach them to this publish. Properties need freeing with
 * <dimq_property_free_all>.
 *
 * If the dimq instance `dimq` is using MQTT v5, the `properties` argument
 * will be applied to the PUBLISH message. For MQTT v3.1.1 and below, the
 * `properties` argument will be ignored.
 *
 * Set your client to use MQTT v5 immediately after it is created:
 *
 * dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *	mid -  a pointer to an int. If not NULL, the function will set this to
 *	       the message id of this particular message. This can be then used
 *	       with the subscribe callback to determine when the message has been
 *	       sent.
 *	sub -  the subscription pattern.
 *	qos -  the requested Quality of Service for this subscription.
 *	options - options to apply to this subscription, OR'd together. Set to 0 to
 *	          use the default options, otherwise choose from list of <mqtt5_sub_options>
 * 	properties - a valid dimq_property list, or NULL.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -        on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -        if the client isn't connected to a broker.
 * 	dimq_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8
 *	dimq_ERR_DUPLICATE_PROPERTY - if a property is duplicated where it is forbidden.
 *	dimq_ERR_PROTOCOL - if any property is invalid for use with SUBSCRIBE.
 *	dimq_ERR_OVERSIZE_PACKET - if the resulting packet would be larger than
 *	                           supported by the broker.
 */
libdimq_EXPORT int dimq_subscribe_v5(struct dimq *dimq, int *mid, const char *sub, int qos, int options, const dimq_property *properties);

/*
 * Function: dimq_subscribe_multiple
 *
 * Subscribe to multiple topics.
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *	mid -  a pointer to an int. If not NULL, the function will set this to
 *	       the message id of this particular message. This can be then used
 *	       with the subscribe callback to determine when the message has been
 *	       sent.
 *  sub_count - the count of subscriptions to be made
 *	sub -  array of sub_count pointers, each pointing to a subscription string.
 *	       The "char *const *const" datatype ensures that neither the array of
 *	       pointers nor the strings that they point to are mutable. If you aren't
 *	       familiar with this, just think of it as a safer "char **",
 *	       equivalent to "const char *" for a simple string pointer.
 *	qos -  the requested Quality of Service for each subscription.
 *	options - options to apply to this subscription, OR'd together. This
 *	          argument is not used for MQTT v3 susbcriptions. Set to 0 to use
 *	          the default options, otherwise choose from list of <mqtt5_sub_options>
 * 	properties - a valid dimq_property list, or NULL. Only used with MQTT
 * 	             v5 clients.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -        on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -        if the client isn't connected to a broker.
 * 	dimq_ERR_MALFORMED_UTF8 - if a topic is not valid UTF-8
 *	dimq_ERR_OVERSIZE_PACKET - if the resulting packet would be larger than
 *	                           supported by the broker.
 */
libdimq_EXPORT int dimq_subscribe_multiple(struct dimq *dimq, int *mid, int sub_count, char *const *const sub, int qos, int options, const dimq_property *properties);

/*
 * Function: dimq_unsubscribe
 *
 * Unsubscribe from a topic.
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *	mid -  a pointer to an int. If not NULL, the function will set this to
 *	       the message id of this particular message. This can be then used
 *	       with the unsubscribe callback to determine when the message has been
 *	       sent.
 *	sub -  the unsubscription pattern.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -        on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -        if the client isn't connected to a broker.
 * 	dimq_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8
 *	dimq_ERR_OVERSIZE_PACKET - if the resulting packet would be larger than
 *	                           supported by the broker.
 */
libdimq_EXPORT int dimq_unsubscribe(struct dimq *dimq, int *mid, const char *sub);

/*
 * Function: dimq_unsubscribe_v5
 *
 * Unsubscribe from a topic, with attached MQTT properties.
 *
 * It is valid to use this function for clients using all MQTT protocol versions.
 * If you need to set MQTT v5 UNSUBSCRIBE properties, use
 * <dimq_unsubscribe_v5> instead.
 *
 * Use e.g. <dimq_property_add_string> and similar to create a list of
 * properties, then attach them to this publish. Properties need freeing with
 * <dimq_property_free_all>.
 *
 * If the dimq instance `dimq` is using MQTT v5, the `properties` argument
 * will be applied to the PUBLISH message. For MQTT v3.1.1 and below, the
 * `properties` argument will be ignored.
 *
 * Set your client to use MQTT v5 immediately after it is created:
 *
 * dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *	mid -  a pointer to an int. If not NULL, the function will set this to
 *	       the message id of this particular message. This can be then used
 *	       with the unsubscribe callback to determine when the message has been
 *	       sent.
 *	sub -  the unsubscription pattern.
 * 	properties - a valid dimq_property list, or NULL. Only used with MQTT
 * 	             v5 clients.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -        on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -        if the client isn't connected to a broker.
 * 	dimq_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8
 *	dimq_ERR_DUPLICATE_PROPERTY - if a property is duplicated where it is forbidden.
 *	dimq_ERR_PROTOCOL - if any property is invalid for use with UNSUBSCRIBE.
 *	dimq_ERR_OVERSIZE_PACKET - if the resulting packet would be larger than
 *	                           supported by the broker.
 */
libdimq_EXPORT int dimq_unsubscribe_v5(struct dimq *dimq, int *mid, const char *sub, const dimq_property *properties);

/*
 * Function: dimq_unsubscribe_multiple
 *
 * Unsubscribe from multiple topics.
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *	mid -  a pointer to an int. If not NULL, the function will set this to
 *	       the message id of this particular message. This can be then used
 *	       with the subscribe callback to determine when the message has been
 *	       sent.
 *  sub_count - the count of unsubscriptions to be made
 *	sub -  array of sub_count pointers, each pointing to an unsubscription string.
 *	       The "char *const *const" datatype ensures that neither the array of
 *	       pointers nor the strings that they point to are mutable. If you aren't
 *	       familiar with this, just think of it as a safer "char **",
 *	       equivalent to "const char *" for a simple string pointer.
 * 	properties - a valid dimq_property list, or NULL. Only used with MQTT
 * 	             v5 clients.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -        on success.
 * 	dimq_ERR_INVAL -          if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -        if the client isn't connected to a broker.
 * 	dimq_ERR_MALFORMED_UTF8 - if a topic is not valid UTF-8
 *	dimq_ERR_OVERSIZE_PACKET - if the resulting packet would be larger than
 *	                           supported by the broker.
 */
libdimq_EXPORT int dimq_unsubscribe_multiple(struct dimq *dimq, int *mid, int sub_count, char *const *const sub, const dimq_property *properties);


/* ======================================================================
 *
 * Section: Struct dimq_message helper functions
 *
 * ====================================================================== */
/*
 * Function: dimq_message_copy
 *
 * Copy the contents of a dimq message to another message.
 * Useful for preserving a message received in the on_message() callback.
 *
 * Parameters:
 *	dst - a pointer to a valid dimq_message struct to copy to.
 *	src - a pointer to a valid dimq_message struct to copy from.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 *
 * See Also:
 * 	<dimq_message_free>
 */
libdimq_EXPORT int dimq_message_copy(struct dimq_message *dst, const struct dimq_message *src);

/*
 * Function: dimq_message_free
 *
 * Completely free a dimq_message struct.
 *
 * Parameters:
 *	message - pointer to a dimq_message pointer to free.
 *
 * See Also:
 * 	<dimq_message_copy>, <dimq_message_free_contents>
 */
libdimq_EXPORT void dimq_message_free(struct dimq_message **message);

/*
 * Function: dimq_message_free_contents
 *
 * Free a dimq_message struct contents, leaving the struct unaffected.
 *
 * Parameters:
 *	message - pointer to a dimq_message struct to free its contents.
 *
 * See Also:
 * 	<dimq_message_copy>, <dimq_message_free>
 */
libdimq_EXPORT void dimq_message_free_contents(struct dimq_message *message);


/* ======================================================================
 *
 * Section: Network loop (managed by libdimq)
 *
 * The internal network loop must be called at a regular interval. The two
 * recommended approaches are to use either <dimq_loop_forever> or
 * <dimq_loop_start>. <dimq_loop_forever> is a blocking call and is
 * suitable for the situation where you only want to handle incoming messages
 * in callbacks. <dimq_loop_start> is a non-blocking call, it creates a
 * separate thread to run the loop for you. Use this function when you have
 * other tasks you need to run at the same time as the MQTT client, e.g.
 * reading data from a sensor.
 *
 * ====================================================================== */

/*
 * Function: dimq_loop_forever
 *
 * This function call loop() for you in an infinite blocking loop. It is useful
 * for the case where you only want to run the MQTT client loop in your
 * program.
 *
 * It handles reconnecting in case server connection is lost. If you call
 * dimq_disconnect() in a callback it will return.
 *
 * Parameters:
 *  dimq - a valid dimq instance.
 *	timeout -     Maximum number of milliseconds to wait for network activity
 *	              in the select() call before timing out. Set to 0 for instant
 *	              return.  Set negative to use the default of 1000ms.
 *	max_packets - this parameter is currently unused and should be set to 1 for
 *	              future compatibility.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -   on success.
 * 	dimq_ERR_INVAL -     if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -     if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -   if the client isn't connected to a broker.
 *  dimq_ERR_CONN_LOST - if the connection to the broker was lost.
 *	dimq_ERR_PROTOCOL -  if there is a protocol error communicating with the
 *                       broker.
 * 	dimq_ERR_ERRNO -     if a system call returned an error. The variable errno
 *                       contains the error code, even on Windows.
 *                       Use strerror_r() where available or FormatMessage() on
 *                       Windows.
 *
 * See Also:
 *	<dimq_loop>, <dimq_loop_start>
 */
libdimq_EXPORT int dimq_loop_forever(struct dimq *dimq, int timeout, int max_packets);

/*
 * Function: dimq_loop_start
 *
 * This is part of the threaded client interface. Call this once to start a new
 * thread to process network traffic. This provides an alternative to
 * repeatedly calling <dimq_loop> yourself.
 *
 * Parameters:
 *  dimq - a valid dimq instance.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -       on success.
 * 	dimq_ERR_INVAL -         if the input parameters were invalid.
 *	dimq_ERR_NOT_SUPPORTED - if thread support is not available.
 *
 * See Also:
 *	<dimq_connect_async>, <dimq_loop>, <dimq_loop_forever>, <dimq_loop_stop>
 */
libdimq_EXPORT int dimq_loop_start(struct dimq *dimq);

/*
 * Function: dimq_loop_stop
 *
 * This is part of the threaded client interface. Call this once to stop the
 * network thread previously created with <dimq_loop_start>. This call
 * will block until the network thread finishes. For the network thread to end,
 * you must have previously called <dimq_disconnect> or have set the force
 * parameter to true.
 *
 * Parameters:
 *  dimq - a valid dimq instance.
 *	force - set to true to force thread cancellation. If false,
 *	        <dimq_disconnect> must have already been called.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -       on success.
 * 	dimq_ERR_INVAL -         if the input parameters were invalid.
 *	dimq_ERR_NOT_SUPPORTED - if thread support is not available.
 *
 * See Also:
 *	<dimq_loop>, <dimq_loop_start>
 */
libdimq_EXPORT int dimq_loop_stop(struct dimq *dimq, bool force);

/*
 * Function: dimq_loop
 *
 * The main network loop for the client. This must be called frequently
 * to keep communications between the client and broker working. This is
 * carried out by <dimq_loop_forever> and <dimq_loop_start>, which
 * are the recommended ways of handling the network loop. You may also use this
 * function if you wish. It must not be called inside a callback.
 *
 * If incoming data is present it will then be processed. Outgoing commands,
 * from e.g.  <dimq_publish>, are normally sent immediately that their
 * function is called, but this is not always possible. <dimq_loop> will
 * also attempt to send any remaining outgoing messages, which also includes
 * commands that are part of the flow for messages with QoS>0.
 *
 * This calls select() to monitor the client network socket. If you want to
 * integrate dimq client operation with your own select() call, use
 * <dimq_socket>, <dimq_loop_read>, <dimq_loop_write> and
 * <dimq_loop_misc>.
 *
 * Threads:
 *
 * Parameters:
 *	dimq -        a valid dimq instance.
 *	timeout -     Maximum number of milliseconds to wait for network activity
 *	              in the select() call before timing out. Set to 0 for instant
 *	              return.  Set negative to use the default of 1000ms.
 *	max_packets - this parameter is currently unused and should be set to 1 for
 *	              future compatibility.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -   on success.
 * 	dimq_ERR_INVAL -     if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -     if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -   if the client isn't connected to a broker.
 *  dimq_ERR_CONN_LOST - if the connection to the broker was lost.
 *	dimq_ERR_PROTOCOL -  if there is a protocol error communicating with the
 *                       broker.
 * 	dimq_ERR_ERRNO -     if a system call returned an error. The variable errno
 *                       contains the error code, even on Windows.
 *                       Use strerror_r() where available or FormatMessage() on
 *                       Windows.
 * See Also:
 *	<dimq_loop_forever>, <dimq_loop_start>, <dimq_loop_stop>
 */
libdimq_EXPORT int dimq_loop(struct dimq *dimq, int timeout, int max_packets);

/* ======================================================================
 *
 * Section: Network loop (for use in other event loops)
 *
 * ====================================================================== */
/*
 * Function: dimq_loop_read
 *
 * Carry out network read operations.
 * This should only be used if you are not using dimq_loop() and are
 * monitoring the client network socket for activity yourself.
 *
 * Parameters:
 *	dimq -        a valid dimq instance.
 *	max_packets - this parameter is currently unused and should be set to 1 for
 *	              future compatibility.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -   on success.
 * 	dimq_ERR_INVAL -     if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -     if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -   if the client isn't connected to a broker.
 *  dimq_ERR_CONN_LOST - if the connection to the broker was lost.
 *	dimq_ERR_PROTOCOL -  if there is a protocol error communicating with the
 *                       broker.
 * 	dimq_ERR_ERRNO -     if a system call returned an error. The variable errno
 *                       contains the error code, even on Windows.
 *                       Use strerror_r() where available or FormatMessage() on
 *                       Windows.
 *
 * See Also:
 *	<dimq_socket>, <dimq_loop_write>, <dimq_loop_misc>
 */
libdimq_EXPORT int dimq_loop_read(struct dimq *dimq, int max_packets);

/*
 * Function: dimq_loop_write
 *
 * Carry out network write operations.
 * This should only be used if you are not using dimq_loop() and are
 * monitoring the client network socket for activity yourself.
 *
 * Parameters:
 *	dimq -        a valid dimq instance.
 *	max_packets - this parameter is currently unused and should be set to 1 for
 *	              future compatibility.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -   on success.
 * 	dimq_ERR_INVAL -     if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -     if an out of memory condition occurred.
 * 	dimq_ERR_NO_CONN -   if the client isn't connected to a broker.
 *  dimq_ERR_CONN_LOST - if the connection to the broker was lost.
 *	dimq_ERR_PROTOCOL -  if there is a protocol error communicating with the
 *                       broker.
 * 	dimq_ERR_ERRNO -     if a system call returned an error. The variable errno
 *                       contains the error code, even on Windows.
 *                       Use strerror_r() where available or FormatMessage() on
 *                       Windows.
 *
 * See Also:
 *	<dimq_socket>, <dimq_loop_read>, <dimq_loop_misc>, <dimq_want_write>
 */
libdimq_EXPORT int dimq_loop_write(struct dimq *dimq, int max_packets);

/*
 * Function: dimq_loop_misc
 *
 * Carry out miscellaneous operations required as part of the network loop.
 * This should only be used if you are not using dimq_loop() and are
 * monitoring the client network socket for activity yourself.
 *
 * This function deals with handling PINGs and checking whether messages need
 * to be retried, so should be called fairly frequently, around once per second
 * is sufficient.
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -   on success.
 * 	dimq_ERR_INVAL -     if the input parameters were invalid.
 * 	dimq_ERR_NO_CONN -   if the client isn't connected to a broker.
 *
 * See Also:
 *	<dimq_socket>, <dimq_loop_read>, <dimq_loop_write>
 */
libdimq_EXPORT int dimq_loop_misc(struct dimq *dimq);


/* ======================================================================
 *
 * Section: Network loop (helper functions)
 *
 * ====================================================================== */
/*
 * Function: dimq_socket
 *
 * Return the socket handle for a dimq instance. Useful if you want to
 * include a dimq client in your own select() calls.
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *
 * Returns:
 *	The socket for the dimq client or -1 on failure.
 */
libdimq_EXPORT int dimq_socket(struct dimq *dimq);

/*
 * Function: dimq_want_write
 *
 * Returns true if there is data ready to be written on the socket.
 *
 * Parameters:
 *	dimq - a valid dimq instance.
 *
 * See Also:
 *	<dimq_socket>, <dimq_loop_read>, <dimq_loop_write>
 */
libdimq_EXPORT bool dimq_want_write(struct dimq *dimq);

/*
 * Function: dimq_threaded_set
 *
 * Used to tell the library that your application is using threads, but not
 * using <dimq_loop_start>. The library operates slightly differently when
 * not in threaded mode in order to simplify its operation. If you are managing
 * your own threads and do not use this function you will experience crashes
 * due to race conditions.
 *
 * When using <dimq_loop_start>, this is set automatically.
 *
 * Parameters:
 *  dimq -     a valid dimq instance.
 *  threaded - true if your application is using threads, false otherwise.
 */
libdimq_EXPORT int dimq_threaded_set(struct dimq *dimq, bool threaded);


/* ======================================================================
 *
 * Section: Client options
 *
 * ====================================================================== */
/*
 * Function: dimq_opts_set
 *
 * Used to set options for the client.
 *
 * This function is deprecated, the replacement <dimq_int_option>,
 * <dimq_string_option> and <dimq_void_option> functions should
 * be used instead.
 *
 * Parameters:
 *	dimq -   a valid dimq instance.
 *	option - the option to set.
 *	value -  the option specific value.
 *
 * Options:
 *	dimq_OPT_PROTOCOL_VERSION - Value must be an int, set to either
 *	          MQTT_PROTOCOL_V31 or MQTT_PROTOCOL_V311. Must be set
 *	          before the client connects.
 *	          Defaults to MQTT_PROTOCOL_V31.
 *
 *	dimq_OPT_SSL_CTX - Pass an openssl SSL_CTX to be used when creating
 *	          TLS connections rather than libdimq creating its own.
 *	          This must be called before connecting to have any effect.
 *	          If you use this option, the onus is on you to ensure that
 *	          you are using secure settings.
 *	          Setting to NULL means that libdimq will use its own SSL_CTX
 *	          if TLS is to be used.
 *	          This option is only available for openssl 1.1.0 and higher.
 *
 *	dimq_OPT_SSL_CTX_WITH_DEFAULTS - Value must be an int set to 1 or 0.
 *	          If set to 1, then the user specified SSL_CTX passed in using
 *	          dimq_OPT_SSL_CTX will have the default options applied to it.
 *	          This means that you only need to change the values that are
 *	          relevant to you. If you use this option then you must configure
 *	          the TLS options as normal, i.e. you should use
 *	          <dimq_tls_set> to configure the cafile/capath as a minimum.
 *	          This option is only available for openssl 1.1.0 and higher.
 */
libdimq_EXPORT int dimq_opts_set(struct dimq *dimq, enum dimq_opt_t option, void *value);

/*
 * Function: dimq_int_option
 *
 * Used to set integer options for the client.
 *
 * Parameters:
 *	dimq -   a valid dimq instance.
 *	option - the option to set.
 *	value -  the option specific value.
 *
 * Options:
 *	dimq_OPT_TCP_NODELAY - Set to 1 to disable Nagle's algorithm on client
 *	          sockets. This has the effect of reducing latency of individual
 *	          messages at the potential cost of increasing the number of
 *	          packets being sent.
 *	          Defaults to 0, which means Nagle remains enabled.
 *
 *	dimq_OPT_PROTOCOL_VERSION - Value must be set to either MQTT_PROTOCOL_V31,
 *	          MQTT_PROTOCOL_V311, or MQTT_PROTOCOL_V5. Must be set before the
 *	          client connects.  Defaults to MQTT_PROTOCOL_V311.
 *
 *	dimq_OPT_RECEIVE_MAXIMUM - Value can be set between 1 and 65535 inclusive,
 *	          and represents the maximum number of incoming QoS 1 and QoS 2
 *	          messages that this client wants to process at once. Defaults to
 *	          20. This option is not valid for MQTT v3.1 or v3.1.1 clients.
 *	          Note that if the MQTT_PROP_RECEIVE_MAXIMUM property is in the
 *	          proplist passed to dimq_connect_v5(), then that property
 *	          will override this option. Using this option is the recommended
 *	          method however.
 *
 *	dimq_OPT_SEND_MAXIMUM - Value can be set between 1 and 65535 inclusive,
 *	          and represents the maximum number of outgoing QoS 1 and QoS 2
 *	          messages that this client will attempt to have "in flight" at
 *	          once. Defaults to 20.
 *	          This option is not valid for MQTT v3.1 or v3.1.1 clients.
 *	          Note that if the broker being connected to sends a
 *	          MQTT_PROP_RECEIVE_MAXIMUM property that has a lower value than
 *	          this option, then the broker provided value will be used.
 *
 *	dimq_OPT_SSL_CTX_WITH_DEFAULTS - If value is set to a non zero value,
 *	          then the user specified SSL_CTX passed in using dimq_OPT_SSL_CTX
 *	          will have the default options applied to it. This means that
 *	          you only need to change the values that are relevant to you.
 *	          If you use this option then you must configure the TLS options
 *	          as normal, i.e.  you should use <dimq_tls_set> to
 *	          configure the cafile/capath as a minimum.
 *	          This option is only available for openssl 1.1.0 and higher.
 *
 *	dimq_OPT_TLS_OCSP_REQUIRED - Set whether OCSP checking on TLS
 *	          connections is required. Set to 1 to enable checking,
 *	          or 0 (the default) for no checking.
 *
 *	dimq_OPT_TLS_USE_OS_CERTS - Set to 1 to instruct the client to load and
 *	          trust OS provided CA certificates for use with TLS connections.
 *	          Set to 0 (the default) to only use manually specified CA certs.
 */
libdimq_EXPORT int dimq_int_option(struct dimq *dimq, enum dimq_opt_t option, int value);


/*
 * Function: dimq_string_option
 *
 * Used to set const char* options for the client.
 *
 * Parameters:
 *	dimq -   a valid dimq instance.
 *	option - the option to set.
 *	value -  the option specific value.
 *
 * Options:
 *	dimq_OPT_TLS_ENGINE - Configure the client for TLS Engine support.
 *	          Pass a TLS Engine ID to be used when creating TLS
 *	          connections. Must be set before <dimq_connect>.
 *
 *	dimq_OPT_TLS_KEYFORM - Configure the client to treat the keyfile
 *	          differently depending on its type.  Must be set
 *	          before <dimq_connect>.
 *	          Set as either "pem" or "engine", to determine from where the
 *	          private key for a TLS connection will be obtained. Defaults to
 *	          "pem", a normal private key file.
 *
 *	dimq_OPT_TLS_KPASS_SHA1 - Where the TLS Engine requires the use of
 *	          a password to be accessed, this option allows a hex encoded
 *	          SHA1 hash of the private key password to be passed to the
 *	          engine directly. Must be set before <dimq_connect>.
 *
 *	dimq_OPT_TLS_ALPN - If the broker being connected to has multiple
 *	          services available on a single TLS port, such as both MQTT
 *	          and WebSockets, use this option to configure the ALPN
 *	          option for the connection.
 *
 *	dimq_OPT_BIND_ADDRESS - Set the hostname or ip address of the local network
 *	          interface to bind to when connecting.
 */
libdimq_EXPORT int dimq_string_option(struct dimq *dimq, enum dimq_opt_t option, const char *value);


/*
 * Function: dimq_void_option
 *
 * Used to set void* options for the client.
 *
 * Parameters:
 *	dimq -   a valid dimq instance.
 *	option - the option to set.
 *	value -  the option specific value.
 *
 * Options:
 *	dimq_OPT_SSL_CTX - Pass an openssl SSL_CTX to be used when creating TLS
 *	          connections rather than libdimq creating its own.  This must
 *	          be called before connecting to have any effect. If you use this
 *	          option, the onus is on you to ensure that you are using secure
 *	          settings.
 *	          Setting to NULL means that libdimq will use its own SSL_CTX
 *	          if TLS is to be used.
 *	          This option is only available for openssl 1.1.0 and higher.
 */
libdimq_EXPORT int dimq_void_option(struct dimq *dimq, enum dimq_opt_t option, void *value);

/*
 * Function: dimq_reconnect_delay_set
 *
 * Control the behaviour of the client when it has unexpectedly disconnected in
 * <dimq_loop_forever> or after <dimq_loop_start>. The default
 * behaviour if this function is not used is to repeatedly attempt to reconnect
 * with a delay of 1 second until the connection succeeds.
 *
 * Use reconnect_delay parameter to change the delay between successive
 * reconnection attempts. You may also enable exponential backoff of the time
 * between reconnections by setting reconnect_exponential_backoff to true and
 * set an upper bound on the delay with reconnect_delay_max.
 *
 * Example 1:
 *	delay=2, delay_max=10, exponential_backoff=False
 *	Delays would be: 2, 4, 6, 8, 10, 10, ...
 *
 * Example 2:
 *	delay=3, delay_max=30, exponential_backoff=True
 *	Delays would be: 3, 6, 12, 24, 30, 30, ...
 *
 * Parameters:
 *  dimq -                          a valid dimq instance.
 *  reconnect_delay -               the number of seconds to wait between
 *                                  reconnects.
 *  reconnect_delay_max -           the maximum number of seconds to wait
 *                                  between reconnects.
 *  reconnect_exponential_backoff - use exponential backoff between
 *                                  reconnect attempts. Set to true to enable
 *                                  exponential backoff.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 */
libdimq_EXPORT int dimq_reconnect_delay_set(struct dimq *dimq, unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff);

/*
 * Function: dimq_max_inflight_messages_set
 *
 * This function is deprected. Use the <dimq_int_option> function with the
 * dimq_OPT_SEND_MAXIMUM option instead.
 *
 * Set the number of QoS 1 and 2 messages that can be "in flight" at one time.
 * An in flight message is part way through its delivery flow. Attempts to send
 * further messages with <dimq_publish> will result in the messages being
 * queued until the number of in flight messages reduces.
 *
 * A higher number here results in greater message throughput, but if set
 * higher than the maximum in flight messages on the broker may lead to
 * delays in the messages being acknowledged.
 *
 * Set to 0 for no maximum.
 *
 * Parameters:
 *  dimq -                  a valid dimq instance.
 *  max_inflight_messages - the maximum number of inflight messages. Defaults
 *                          to 20.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 */
libdimq_EXPORT int dimq_max_inflight_messages_set(struct dimq *dimq, unsigned int max_inflight_messages);

/*
 * Function: dimq_message_retry_set
 *
 * This function now has no effect.
 */
libdimq_EXPORT void dimq_message_retry_set(struct dimq *dimq, unsigned int message_retry);

/*
 * Function: dimq_user_data_set
 *
 * When <dimq_new> is called, the pointer given as the "obj" parameter
 * will be passed to the callbacks as user data. The <dimq_user_data_set>
 * function allows this obj parameter to be updated at any time. This function
 * will not modify the memory pointed to by the current user data pointer. If
 * it is dynamically allocated memory you must free it yourself.
 *
 * Parameters:
 *  dimq - a valid dimq instance.
 * 	obj -  A user pointer that will be passed as an argument to any callbacks
 * 	       that are specified.
 */
libdimq_EXPORT void dimq_user_data_set(struct dimq *dimq, void *obj);

/* Function: dimq_userdata
 *
 * Retrieve the "userdata" variable for a dimq client.
 *
 * Parameters:
 * 	dimq - a valid dimq instance.
 *
 * Returns:
 *	A pointer to the userdata member variable.
 */
libdimq_EXPORT void *dimq_userdata(struct dimq *dimq);


/* ======================================================================
 *
 * Section: TLS support
 *
 * ====================================================================== */
/*
 * Function: dimq_tls_set
 *
 * Configure the client for certificate based SSL/TLS support. Must be called
 * before <dimq_connect>.
 *
 * Cannot be used in conjunction with <dimq_tls_psk_set>.
 *
 * Define the Certificate Authority certificates to be trusted (ie. the server
 * certificate must be signed with one of these certificates) using cafile.
 *
 * If the server you are connecting to requires clients to provide a
 * certificate, define certfile and keyfile with your client certificate and
 * private key. If your private key is encrypted, provide a password callback
 * function or you will have to enter the password at the command line.
 *
 * Parameters:
 *  dimq -        a valid dimq instance.
 *  cafile -      path to a file containing the PEM encoded trusted CA
 *                certificate files. Either cafile or capath must not be NULL.
 *  capath -      path to a directory containing the PEM encoded trusted CA
 *                certificate files. See dimq.conf for more details on
 *                configuring this directory. Either cafile or capath must not
 *                be NULL.
 *  certfile -    path to a file containing the PEM encoded certificate file
 *                for this client. If NULL, keyfile must also be NULL and no
 *                client certificate will be used.
 *  keyfile -     path to a file containing the PEM encoded private key for
 *                this client. If NULL, certfile must also be NULL and no
 *                client certificate will be used.
 *  pw_callback - if keyfile is encrypted, set pw_callback to allow your client
 *                to pass the correct password for decryption. If set to NULL,
 *                the password must be entered on the command line.
 *                Your callback must write the password into "buf", which is
 *                "size" bytes long. The return value must be the length of the
 *                password. "userdata" will be set to the calling dimq
 *                instance. The dimq userdata member variable can be
 *                retrieved using <dimq_userdata>.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 *
 * See Also:
 *	<dimq_tls_opts_set>, <dimq_tls_psk_set>,
 *	<dimq_tls_insecure_set>, <dimq_userdata>
 */
libdimq_EXPORT int dimq_tls_set(struct dimq *dimq,
		const char *cafile, const char *capath,
		const char *certfile, const char *keyfile,
		int (*pw_callback)(char *buf, int size, int rwflag, void *userdata));

/*
 * Function: dimq_tls_insecure_set
 *
 * Configure verification of the server hostname in the server certificate. If
 * value is set to true, it is impossible to guarantee that the host you are
 * connecting to is not impersonating your server. This can be useful in
 * initial server testing, but makes it possible for a malicious third party to
 * impersonate your server through DNS spoofing, for example.
 * Do not use this function in a real system. Setting value to true makes the
 * connection encryption pointless.
 * Must be called before <dimq_connect>.
 *
 * Parameters:
 *  dimq -  a valid dimq instance.
 *  value - if set to false, the default, certificate hostname checking is
 *          performed. If set to true, no hostname checking is performed and
 *          the connection is insecure.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 *
 * See Also:
 *	<dimq_tls_set>
 */
libdimq_EXPORT int dimq_tls_insecure_set(struct dimq *dimq, bool value);

/*
 * Function: dimq_tls_opts_set
 *
 * Set advanced SSL/TLS options. Must be called before <dimq_connect>.
 *
 * Parameters:
 *  dimq -        a valid dimq instance.
 *	cert_reqs -   an integer defining the verification requirements the client
 *	              will impose on the server. This can be one of:
 *	              * SSL_VERIFY_NONE (0): the server will not be verified in any way.
 *	              * SSL_VERIFY_PEER (1): the server certificate will be verified
 *	                and the connection aborted if the verification fails.
 *	              The default and recommended value is SSL_VERIFY_PEER. Using
 *	              SSL_VERIFY_NONE provides no security.
 *	tls_version - the version of the SSL/TLS protocol to use as a string. If NULL,
 *	              the default value is used. The default value and the
 *	              available values depend on the version of openssl that the
 *	              library was compiled against. For openssl >= 1.0.1, the
 *	              available options are tlsv1.2, tlsv1.1 and tlsv1, with tlv1.2
 *	              as the default. For openssl < 1.0.1, only tlsv1 is available.
 *	ciphers -     a string describing the ciphers available for use. See the
 *	              "openssl ciphers" tool for more information. If NULL, the
 *	              default ciphers will be used.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 *
 * See Also:
 *	<dimq_tls_set>
 */
libdimq_EXPORT int dimq_tls_opts_set(struct dimq *dimq, int cert_reqs, const char *tls_version, const char *ciphers);

/*
 * Function: dimq_tls_psk_set
 *
 * Configure the client for pre-shared-key based TLS support. Must be called
 * before <dimq_connect>.
 *
 * Cannot be used in conjunction with <dimq_tls_set>.
 *
 * Parameters:
 *  dimq -     a valid dimq instance.
 *  psk -      the pre-shared-key in hex format with no leading "0x".
 *  identity - the identity of this client. May be used as the username
 *             depending on the server settings.
 *	ciphers -  a string describing the PSK ciphers available for use. See the
 *	           "openssl ciphers" tool for more information. If NULL, the
 *	           default ciphers will be used.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success.
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 *
 * See Also:
 *	<dimq_tls_set>
 */
libdimq_EXPORT int dimq_tls_psk_set(struct dimq *dimq, const char *psk, const char *identity, const char *ciphers);


/*
 * Function: dimq_ssl_get
 *
 * Retrieve a pointer to the SSL structure used for TLS connections in this
 * client. This can be used in e.g. the connect callback to carry out
 * additional verification steps.
 *
 * Parameters:
 *  dimq - a valid dimq instance
 *
 * Returns:
 *  A valid pointer to an openssl SSL structure - if the client is using TLS.
 *  NULL - if the client is not using TLS, or TLS support is not compiled in.
 */
libdimq_EXPORT void *dimq_ssl_get(struct dimq *dimq);


/* ======================================================================
 *
 * Section: Callbacks
 *
 * ====================================================================== */
/*
 * Function: dimq_connect_callback_set
 *
 * Set the connect callback. This is called when the library receives a CONNACK
 * message in response to a connection.
 *
 * Parameters:
 *  dimq -       a valid dimq instance.
 *  on_connect - a callback function in the following form:
 *               void callback(struct dimq *dimq, void *obj, int rc)
 *
 * Callback Parameters:
 *  dimq - the dimq instance making the callback.
 *  obj - the user data provided in <dimq_new>
 *  rc -  the return code of the connection response. The values are defined by
 *        the MQTT protocol version in use.
 *        For MQTT v5.0, look at section 3.2.2.2 Connect Reason code: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html
 *        For MQTT v3.1.1, look at section 3.2.2.3 Connect Return code: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
 */
libdimq_EXPORT void dimq_connect_callback_set(struct dimq *dimq, void (*on_connect)(struct dimq *, void *, int));

/*
 * Function: dimq_connect_with_flags_callback_set
 *
 * Set the connect callback. This is called when the library receives a CONNACK
 * message in response to a connection.
 *
 * Parameters:
 *  dimq -       a valid dimq instance.
 *  on_connect - a callback function in the following form:
 *               void callback(struct dimq *dimq, void *obj, int rc)
 *
 * Callback Parameters:
 *  dimq - the dimq instance making the callback.
 *  obj - the user data provided in <dimq_new>
 *  rc -  the return code of the connection response. The values are defined by
 *        the MQTT protocol version in use.
 *        For MQTT v5.0, look at section 3.2.2.2 Connect Reason code: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html
 *        For MQTT v3.1.1, look at section 3.2.2.3 Connect Return code: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
 *  flags - the connect flags.
 */
libdimq_EXPORT void dimq_connect_with_flags_callback_set(struct dimq *dimq, void (*on_connect)(struct dimq *, void *, int, int));

/*
 * Function: dimq_connect_v5_callback_set
 *
 * Set the connect callback. This is called when the library receives a CONNACK
 * message in response to a connection.
 *
 * It is valid to set this callback for all MQTT protocol versions. If it is
 * used with MQTT clients that use MQTT v3.1.1 or earlier, then the `props`
 * argument will always be NULL.
 *
 * Parameters:
 *  dimq -       a valid dimq instance.
 *  on_connect - a callback function in the following form:
 *               void callback(struct dimq *dimq, void *obj, int rc)
 *
 * Callback Parameters:
 *  dimq - the dimq instance making the callback.
 *  obj - the user data provided in <dimq_new>
 *  rc -  the return code of the connection response. The values are defined by
 *        the MQTT protocol version in use.
 *        For MQTT v5.0, look at section 3.2.2.2 Connect Reason code: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html
 *        For MQTT v3.1.1, look at section 3.2.2.3 Connect Return code: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
 *  flags - the connect flags.
 *  props - list of MQTT 5 properties, or NULL
 *
 */
libdimq_EXPORT void dimq_connect_v5_callback_set(struct dimq *dimq, void (*on_connect)(struct dimq *, void *, int, int, const dimq_property *props));

/*
 * Function: dimq_disconnect_callback_set
 *
 * Set the disconnect callback. This is called when the broker has received the
 * DISCONNECT command and has disconnected the client.
 *
 * Parameters:
 *  dimq -          a valid dimq instance.
 *  on_disconnect - a callback function in the following form:
 *                  void callback(struct dimq *dimq, void *obj)
 *
 * Callback Parameters:
 *  dimq - the dimq instance making the callback.
 *  obj -  the user data provided in <dimq_new>
 *  rc -   integer value indicating the reason for the disconnect. A value of 0
 *         means the client has called <dimq_disconnect>. Any other value
 *         indicates that the disconnect is unexpected.
 */
libdimq_EXPORT void dimq_disconnect_callback_set(struct dimq *dimq, void (*on_disconnect)(struct dimq *, void *, int));

/*
 * Function: dimq_disconnect_v5_callback_set
 *
 * Set the disconnect callback. This is called when the broker has received the
 * DISCONNECT command and has disconnected the client.
 *
 * It is valid to set this callback for all MQTT protocol versions. If it is
 * used with MQTT clients that use MQTT v3.1.1 or earlier, then the `props`
 * argument will always be NULL.
 *
 * Parameters:
 *  dimq -          a valid dimq instance.
 *  on_disconnect - a callback function in the following form:
 *                  void callback(struct dimq *dimq, void *obj)
 *
 * Callback Parameters:
 *  dimq - the dimq instance making the callback.
 *  obj -  the user data provided in <dimq_new>
 *  rc -   integer value indicating the reason for the disconnect. A value of 0
 *         means the client has called <dimq_disconnect>. Any other value
 *         indicates that the disconnect is unexpected.
 *  props - list of MQTT 5 properties, or NULL
 */
libdimq_EXPORT void dimq_disconnect_v5_callback_set(struct dimq *dimq, void (*on_disconnect)(struct dimq *, void *, int, const dimq_property *props));

/*
 * Function: dimq_publish_callback_set
 *
 * Set the publish callback. This is called when a message initiated with
 * <dimq_publish> has been sent to the broker. "Sent" means different
 * things depending on the QoS of the message:
 *
 * QoS 0: The PUBLISH was passed to the local operating system for delivery,
 *        there is no guarantee that it was delivered to the remote broker.
 * QoS 1: The PUBLISH was sent to the remote broker and the corresponding
 *        PUBACK was received by the library.
 * QoS 2: The PUBLISH was sent to the remote broker and the corresponding
 *        PUBCOMP was received by the library.
 *
 * Parameters:
 *  dimq -       a valid dimq instance.
 *  on_publish - a callback function in the following form:
 *               void callback(struct dimq *dimq, void *obj, int mid)
 *
 * Callback Parameters:
 *  dimq - the dimq instance making the callback.
 *  obj -  the user data provided in <dimq_new>
 *  mid -  the message id of the sent message.
 */
libdimq_EXPORT void dimq_publish_callback_set(struct dimq *dimq, void (*on_publish)(struct dimq *, void *, int));

/*
 * Function: dimq_publish_v5_callback_set
 *
 * Set the publish callback. This is called when a message initiated with
 * <dimq_publish> has been sent to the broker. This callback will be
 * called both if the message is sent successfully, or if the broker responded
 * with an error, which will be reflected in the reason_code parameter.
 * "Sent" means different things depending on the QoS of the message:
 *
 * QoS 0: The PUBLISH was passed to the local operating system for delivery,
 *        there is no guarantee that it was delivered to the remote broker.
 * QoS 1: The PUBLISH was sent to the remote broker and the corresponding
 *        PUBACK was received by the library.
 * QoS 2: The PUBLISH was sent to the remote broker and the corresponding
 *        PUBCOMP was received by the library.
 *
 *
 * It is valid to set this callback for all MQTT protocol versions. If it is
 * used with MQTT clients that use MQTT v3.1.1 or earlier, then the `props`
 * argument will always be NULL.
 *
 * Parameters:
 *  dimq -       a valid dimq instance.
 *  on_publish - a callback function in the following form:
 *               void callback(struct dimq *dimq, void *obj, int mid)
 *
 * Callback Parameters:
 *  dimq - the dimq instance making the callback.
 *  obj -  the user data provided in <dimq_new>
 *  mid -  the message id of the sent message.
 *  reason_code - the MQTT 5 reason code
 *  props - list of MQTT 5 properties, or NULL
 */
libdimq_EXPORT void dimq_publish_v5_callback_set(struct dimq *dimq, void (*on_publish)(struct dimq *, void *, int, int, const dimq_property *props));

/*
 * Function: dimq_message_callback_set
 *
 * Set the message callback. This is called when a message is received from the
 * broker and the required QoS flow has completed.
 *
 * Parameters:
 *  dimq -       a valid dimq instance.
 *  on_message - a callback function in the following form:
 *               void callback(struct dimq *dimq, void *obj, const struct dimq_message *message)
 *
 * Callback Parameters:
 *  dimq -    the dimq instance making the callback.
 *  obj -     the user data provided in <dimq_new>
 *  message - the message data. This variable and associated memory will be
 *            freed by the library after the callback completes. The client
 *            should make copies of any of the data it requires.
 *
 * See Also:
 * 	<dimq_message_copy>
 */
libdimq_EXPORT void dimq_message_callback_set(struct dimq *dimq, void (*on_message)(struct dimq *, void *, const struct dimq_message *));

/*
 * Function: dimq_message_v5_callback_set
 *
 * Set the message callback. This is called when a message is received from the
 * broker and the required QoS flow has completed.
 *
 * It is valid to set this callback for all MQTT protocol versions. If it is
 * used with MQTT clients that use MQTT v3.1.1 or earlier, then the `props`
 * argument will always be NULL.
 *
 * Parameters:
 *  dimq -       a valid dimq instance.
 *  on_message - a callback function in the following form:
 *               void callback(struct dimq *dimq, void *obj, const struct dimq_message *message)
 *
 * Callback Parameters:
 *  dimq -    the dimq instance making the callback.
 *  obj -     the user data provided in <dimq_new>
 *  message - the message data. This variable and associated memory will be
 *            freed by the library after the callback completes. The client
 *            should make copies of any of the data it requires.
 *  props - list of MQTT 5 properties, or NULL
 *
 * See Also:
 * 	<dimq_message_copy>
 */
libdimq_EXPORT void dimq_message_v5_callback_set(struct dimq *dimq, void (*on_message)(struct dimq *, void *, const struct dimq_message *, const dimq_property *props));

/*
 * Function: dimq_subscribe_callback_set
 *
 * Set the subscribe callback. This is called when the library receives a
 * SUBACK message in response to a SUBSCRIBE.
 *
 * Parameters:
 *  dimq -         a valid dimq instance.
 *  on_subscribe - a callback function in the following form:
 *                 void callback(struct dimq *dimq, void *obj, int mid, int qos_count, const int *granted_qos)
 *
 * Callback Parameters:
 *  dimq -        the dimq instance making the callback.
 *  obj -         the user data provided in <dimq_new>
 *  mid -         the message id of the subscribe message.
 *  qos_count -   the number of granted subscriptions (size of granted_qos).
 *  granted_qos - an array of integers indicating the granted QoS for each of
 *                the subscriptions.
 */
libdimq_EXPORT void dimq_subscribe_callback_set(struct dimq *dimq, void (*on_subscribe)(struct dimq *, void *, int, int, const int *));

/*
 * Function: dimq_subscribe_v5_callback_set
 *
 * Set the subscribe callback. This is called when the library receives a
 * SUBACK message in response to a SUBSCRIBE.
 *
 * It is valid to set this callback for all MQTT protocol versions. If it is
 * used with MQTT clients that use MQTT v3.1.1 or earlier, then the `props`
 * argument will always be NULL.
 *
 * Parameters:
 *  dimq -         a valid dimq instance.
 *  on_subscribe - a callback function in the following form:
 *                 void callback(struct dimq *dimq, void *obj, int mid, int qos_count, const int *granted_qos)
 *
 * Callback Parameters:
 *  dimq -        the dimq instance making the callback.
 *  obj -         the user data provided in <dimq_new>
 *  mid -         the message id of the subscribe message.
 *  qos_count -   the number of granted subscriptions (size of granted_qos).
 *  granted_qos - an array of integers indicating the granted QoS for each of
 *                the subscriptions.
 *  props - list of MQTT 5 properties, or NULL
 */
libdimq_EXPORT void dimq_subscribe_v5_callback_set(struct dimq *dimq, void (*on_subscribe)(struct dimq *, void *, int, int, const int *, const dimq_property *props));

/*
 * Function: dimq_unsubscribe_callback_set
 *
 * Set the unsubscribe callback. This is called when the library receives a
 * UNSUBACK message in response to an UNSUBSCRIBE.
 *
 * Parameters:
 *  dimq -           a valid dimq instance.
 *  on_unsubscribe - a callback function in the following form:
 *                   void callback(struct dimq *dimq, void *obj, int mid)
 *
 * Callback Parameters:
 *  dimq - the dimq instance making the callback.
 *  obj -  the user data provided in <dimq_new>
 *  mid -  the message id of the unsubscribe message.
 */
libdimq_EXPORT void dimq_unsubscribe_callback_set(struct dimq *dimq, void (*on_unsubscribe)(struct dimq *, void *, int));

/*
 * Function: dimq_unsubscribe_v5_callback_set
 *
 * Set the unsubscribe callback. This is called when the library receives a
 * UNSUBACK message in response to an UNSUBSCRIBE.
 *
 * It is valid to set this callback for all MQTT protocol versions. If it is
 * used with MQTT clients that use MQTT v3.1.1 or earlier, then the `props`
 * argument will always be NULL.
 *
 * Parameters:
 *  dimq -           a valid dimq instance.
 *  on_unsubscribe - a callback function in the following form:
 *                   void callback(struct dimq *dimq, void *obj, int mid)
 *
 * Callback Parameters:
 *  dimq - the dimq instance making the callback.
 *  obj -  the user data provided in <dimq_new>
 *  mid -  the message id of the unsubscribe message.
 *  props - list of MQTT 5 properties, or NULL
 */
libdimq_EXPORT void dimq_unsubscribe_v5_callback_set(struct dimq *dimq, void (*on_unsubscribe)(struct dimq *, void *, int, const dimq_property *props));

/*
 * Function: dimq_log_callback_set
 *
 * Set the logging callback. This should be used if you want event logging
 * information from the client library.
 *
 *  dimq -   a valid dimq instance.
 *  on_log - a callback function in the following form:
 *           void callback(struct dimq *dimq, void *obj, int level, const char *str)
 *
 * Callback Parameters:
 *  dimq -  the dimq instance making the callback.
 *  obj -   the user data provided in <dimq_new>
 *  level - the log message level from the values:
 *	        dimq_LOG_INFO
 *	        dimq_LOG_NOTICE
 *	        dimq_LOG_WARNING
 *	        dimq_LOG_ERR
 *	        dimq_LOG_DEBUG
 *	str -   the message string.
 */
libdimq_EXPORT void dimq_log_callback_set(struct dimq *dimq, void (*on_log)(struct dimq *, void *, int, const char *));


/* =============================================================================
 *
 * Section: SOCKS5 proxy functions
 *
 * =============================================================================
 */

/*
 * Function: dimq_socks5_set
 *
 * Configure the client to use a SOCKS5 proxy when connecting. Must be called
 * before connecting. "None" and "username/password" authentication is
 * supported.
 *
 * Parameters:
 *   dimq - a valid dimq instance.
 *   host - the SOCKS5 proxy host to connect to.
 *   port - the SOCKS5 proxy port to use.
 *   username - if not NULL, use this username when authenticating with the proxy.
 *   password - if not NULL and username is not NULL, use this password when
 *              authenticating with the proxy.
 */
libdimq_EXPORT int dimq_socks5_set(struct dimq *dimq, const char *host, int port, const char *username, const char *password);


/* =============================================================================
 *
 * Section: Utility functions
 *
 * =============================================================================
 */

/*
 * Function: dimq_strerror
 *
 * Call to obtain a const string description of a dimq error number.
 *
 * Parameters:
 *	dimq_errno - a dimq error number.
 *
 * Returns:
 *	A constant string describing the error.
 */
libdimq_EXPORT const char *dimq_strerror(int dimq_errno);

/*
 * Function: dimq_connack_string
 *
 * Call to obtain a const string description of an MQTT connection result.
 *
 * Parameters:
 *	connack_code - an MQTT connection result.
 *
 * Returns:
 *	A constant string describing the result.
 */
libdimq_EXPORT const char *dimq_connack_string(int connack_code);

/*
 * Function: dimq_reason_string
 *
 * Call to obtain a const string description of an MQTT reason code.
 *
 * Parameters:
 *	reason_code - an MQTT reason code.
 *
 * Returns:
 *	A constant string describing the reason.
 */
libdimq_EXPORT const char *dimq_reason_string(int reason_code);

/* Function: dimq_string_to_command
 *
 * Take a string input representing an MQTT command and convert it to the
 * libdimq integer representation.
 *
 * Parameters:
 *   str - the string to parse.
 *   cmd - pointer to an int, for the result.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - on an invalid input.
 *
 * Example:
 * (start code)
 *  dimq_string_to_command("CONNECT", &cmd);
 *  // cmd == CMD_CONNECT
 * (end)
 */
libdimq_EXPORT int dimq_string_to_command(const char *str, int *cmd);

/*
 * Function: dimq_sub_topic_tokenise
 *
 * Tokenise a topic or subscription string into an array of strings
 * representing the topic hierarchy.
 *
 * For example:
 *
 *    subtopic: "a/deep/topic/hierarchy"
 *
 *    Would result in:
 *
 *       topics[0] = "a"
 *       topics[1] = "deep"
 *       topics[2] = "topic"
 *       topics[3] = "hierarchy"
 *
 *    and:
 *
 *    subtopic: "/a/deep/topic/hierarchy/"
 *
 *    Would result in:
 *
 *       topics[0] = NULL
 *       topics[1] = "a"
 *       topics[2] = "deep"
 *       topics[3] = "topic"
 *       topics[4] = "hierarchy"
 *
 * Parameters:
 *	subtopic - the subscription/topic to tokenise
 *	topics -   a pointer to store the array of strings
 *	count -    an int pointer to store the number of items in the topics array.
 *
 * Returns:
 *	dimq_ERR_SUCCESS -        on success
 * 	dimq_ERR_NOMEM -          if an out of memory condition occurred.
 * 	dimq_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8
 *
 * Example:
 *
 * > char **topics;
 * > int topic_count;
 * > int i;
 * >
 * > dimq_sub_topic_tokenise("$SYS/broker/uptime", &topics, &topic_count);
 * >
 * > for(i=0; i<token_count; i++){
 * >     printf("%d: %s\n", i, topics[i]);
 * > }
 *
 * See Also:
 *	<dimq_sub_topic_tokens_free>
 */
libdimq_EXPORT int dimq_sub_topic_tokenise(const char *subtopic, char ***topics, int *count);

/*
 * Function: dimq_sub_topic_tokens_free
 *
 * Free memory that was allocated in <dimq_sub_topic_tokenise>.
 *
 * Parameters:
 *	topics - pointer to string array.
 *	count - count of items in string array.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 *
 * See Also:
 *	<dimq_sub_topic_tokenise>
 */
libdimq_EXPORT int dimq_sub_topic_tokens_free(char ***topics, int count);

/*
 * Function: dimq_topic_matches_sub
 *
 * Check whether a topic matches a subscription.
 *
 * For example:
 *
 * foo/bar would match the subscription foo/# or +/bar
 * non/matching would not match the subscription non/+/+
 *
 * Parameters:
 *	sub - subscription string to check topic against.
 *	topic - topic to check.
 *	result - bool pointer to hold result. Will be set to true if the topic
 *	         matches the subscription.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 * 	dimq_ERR_INVAL -   if the input parameters were invalid.
 * 	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 */
libdimq_EXPORT int dimq_topic_matches_sub(const char *sub, const char *topic, bool *result);


/*
 * Function: dimq_topic_matches_sub2
 *
 * Check whether a topic matches a subscription.
 *
 * For example:
 *
 * foo/bar would match the subscription foo/# or +/bar
 * non/matching would not match the subscription non/+/+
 *
 * Parameters:
 *	sub - subscription string to check topic against.
 *	sublen - length in bytes of sub string
 *	topic - topic to check.
 *	topiclen - length in bytes of topic string
 *	result - bool pointer to hold result. Will be set to true if the topic
 *	         matches the subscription.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL -   if the input parameters were invalid.
 *	dimq_ERR_NOMEM -   if an out of memory condition occurred.
 */
libdimq_EXPORT int dimq_topic_matches_sub2(const char *sub, size_t sublen, const char *topic, size_t topiclen, bool *result);

/*
 * Function: dimq_pub_topic_check
 *
 * Check whether a topic to be used for publishing is valid.
 *
 * This searches for + or # in a topic and checks its length.
 *
 * This check is already carried out in <dimq_publish> and
 * <dimq_will_set>, there is no need to call it directly before them. It
 * may be useful if you wish to check the validity of a topic in advance of
 * making a connection for example.
 *
 * Parameters:
 *   topic - the topic to check
 *
 * Returns:
 *   dimq_ERR_SUCCESS -        for a valid topic
 *   dimq_ERR_INVAL -          if the topic contains a + or a #, or if it is too long.
 *   dimq_ERR_MALFORMED_UTF8 - if topic is not valid UTF-8
 *
 * See Also:
 *   <dimq_sub_topic_check>
 */
libdimq_EXPORT int dimq_pub_topic_check(const char *topic);

/*
 * Function: dimq_pub_topic_check2
 *
 * Check whether a topic to be used for publishing is valid.
 *
 * This searches for + or # in a topic and checks its length.
 *
 * This check is already carried out in <dimq_publish> and
 * <dimq_will_set>, there is no need to call it directly before them. It
 * may be useful if you wish to check the validity of a topic in advance of
 * making a connection for example.
 *
 * Parameters:
 *   topic - the topic to check
 *   topiclen - length of the topic in bytes
 *
 * Returns:
 *   dimq_ERR_SUCCESS -        for a valid topic
 *   dimq_ERR_INVAL -          if the topic contains a + or a #, or if it is too long.
 *   dimq_ERR_MALFORMED_UTF8 - if topic is not valid UTF-8
 *
 * See Also:
 *   <dimq_sub_topic_check>
 */
libdimq_EXPORT int dimq_pub_topic_check2(const char *topic, size_t topiclen);

/*
 * Function: dimq_sub_topic_check
 *
 * Check whether a topic to be used for subscribing is valid.
 *
 * This searches for + or # in a topic and checks that they aren't in invalid
 * positions, such as with foo/#/bar, foo/+bar or foo/bar#, and checks its
 * length.
 *
 * This check is already carried out in <dimq_subscribe> and
 * <dimq_unsubscribe>, there is no need to call it directly before them.
 * It may be useful if you wish to check the validity of a topic in advance of
 * making a connection for example.
 *
 * Parameters:
 *   topic - the topic to check
 *
 * Returns:
 *   dimq_ERR_SUCCESS -        for a valid topic
 *   dimq_ERR_INVAL -          if the topic contains a + or a # that is in an
 *                             invalid position, or if it is too long.
 *   dimq_ERR_MALFORMED_UTF8 - if topic is not valid UTF-8
 *
 * See Also:
 *   <dimq_sub_topic_check>
 */
libdimq_EXPORT int dimq_sub_topic_check(const char *topic);

/*
 * Function: dimq_sub_topic_check2
 *
 * Check whether a topic to be used for subscribing is valid.
 *
 * This searches for + or # in a topic and checks that they aren't in invalid
 * positions, such as with foo/#/bar, foo/+bar or foo/bar#, and checks its
 * length.
 *
 * This check is already carried out in <dimq_subscribe> and
 * <dimq_unsubscribe>, there is no need to call it directly before them.
 * It may be useful if you wish to check the validity of a topic in advance of
 * making a connection for example.
 *
 * Parameters:
 *   topic - the topic to check
 *   topiclen - the length in bytes of the topic
 *
 * Returns:
 *   dimq_ERR_SUCCESS -        for a valid topic
 *   dimq_ERR_INVAL -          if the topic contains a + or a # that is in an
 *                             invalid position, or if it is too long.
 *   dimq_ERR_MALFORMED_UTF8 - if topic is not valid UTF-8
 *
 * See Also:
 *   <dimq_sub_topic_check>
 */
libdimq_EXPORT int dimq_sub_topic_check2(const char *topic, size_t topiclen);


/*
 * Function: dimq_validate_utf8
 *
 * Helper function to validate whether a UTF-8 string is valid, according to
 * the UTF-8 spec and the MQTT additions.
 *
 * Parameters:
 *   str - a string to check
 *   len - the length of the string in bytes
 *
 * Returns:
 *   dimq_ERR_SUCCESS -        on success
 *   dimq_ERR_INVAL -          if str is NULL or len<0 or len>65536
 *   dimq_ERR_MALFORMED_UTF8 - if str is not valid UTF-8
 */
libdimq_EXPORT int dimq_validate_utf8(const char *str, int len);


/* =============================================================================
 *
 * Section: One line client helper functions
 *
 * =============================================================================
 */

struct libdimq_will {
	char *topic;
	void *payload;
	int payloadlen;
	int qos;
	bool retain;
};

struct libdimq_auth {
	char *username;
	char *password;
};

struct libdimq_tls {
	char *cafile;
	char *capath;
	char *certfile;
	char *keyfile;
	char *ciphers;
	char *tls_version;
	int (*pw_callback)(char *buf, int size, int rwflag, void *userdata);
	int cert_reqs;
};

/*
 * Function: dimq_subscribe_simple
 *
 * Helper function to make subscribing to a topic and retrieving some messages
 * very straightforward.
 *
 * This connects to a broker, subscribes to a topic, waits for msg_count
 * messages to be received, then returns after disconnecting cleanly.
 *
 * Parameters:
 *   messages - pointer to a "struct dimq_message *". The received
 *              messages will be returned here. On error, this will be set to
 *              NULL.
 *   msg_count - the number of messages to retrieve.
 *   want_retained - if set to true, stale retained messages will be treated as
 *                   normal messages with regards to msg_count. If set to
 *                   false, they will be ignored.
 *   topic - the subscription topic to use (wildcards are allowed).
 *   qos - the qos to use for the subscription.
 *   host - the broker to connect to.
 *   port - the network port the broker is listening on.
 *   client_id - the client id to use, or NULL if a random client id should be
 *               generated.
 *   keepalive - the MQTT keepalive value.
 *   clean_session - the MQTT clean session flag.
 *   username - the username string, or NULL for no username authentication.
 *   password - the password string, or NULL for an empty password.
 *   will - a libdimq_will struct containing will information, or NULL for
 *          no will.
 *   tls - a libdimq_tls struct containing TLS related parameters, or NULL
 *         for no use of TLS.
 *
 *
 * Returns:
 *   dimq_ERR_SUCCESS - on success
 *   Greater than 0 - on error.
 */
libdimq_EXPORT int dimq_subscribe_simple(
		struct dimq_message **messages,
		int msg_count,
		bool want_retained,
		const char *topic,
		int qos,
		const char *host,
		int port,
		const char *client_id,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libdimq_will *will,
		const struct libdimq_tls *tls);


/*
 * Function: dimq_subscribe_callback
 *
 * Helper function to make subscribing to a topic and processing some messages
 * very straightforward.
 *
 * This connects to a broker, subscribes to a topic, then passes received
 * messages to a user provided callback. If the callback returns a 1, it then
 * disconnects cleanly and returns.
 *
 * Parameters:
 *   callback - a callback function in the following form:
 *              int callback(struct dimq *dimq, void *obj, const struct dimq_message *message)
 *              Note that this is the same as the normal on_message callback,
 *              except that it returns an int.
 *   userdata - user provided pointer that will be passed to the callback.
 *   topic - the subscription topic to use (wildcards are allowed).
 *   qos - the qos to use for the subscription.
 *   host - the broker to connect to.
 *   port - the network port the broker is listening on.
 *   client_id - the client id to use, or NULL if a random client id should be
 *               generated.
 *   keepalive - the MQTT keepalive value.
 *   clean_session - the MQTT clean session flag.
 *   username - the username string, or NULL for no username authentication.
 *   password - the password string, or NULL for an empty password.
 *   will - a libdimq_will struct containing will information, or NULL for
 *          no will.
 *   tls - a libdimq_tls struct containing TLS related parameters, or NULL
 *         for no use of TLS.
 *
 *
 * Returns:
 *   dimq_ERR_SUCCESS - on success
 *   Greater than 0 - on error.
 */
libdimq_EXPORT int dimq_subscribe_callback(
		int (*callback)(struct dimq *, void *, const struct dimq_message *),
		void *userdata,
		const char *topic,
		int qos,
		const char *host,
		int port,
		const char *client_id,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libdimq_will *will,
		const struct libdimq_tls *tls);


/* =============================================================================
 *
 * Section: Properties
 *
 * =============================================================================
 */


/*
 * Function: dimq_property_add_byte
 *
 * Add a new byte property to a property list.
 *
 * If *proplist == NULL, a new list will be created, otherwise the new property
 * will be appended to the list.
 *
 * Parameters:
 *	proplist - pointer to dimq_property pointer, the list of properties
 *	identifier - property identifier (e.g. MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 *	value - integer value for the new property
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if identifier is invalid, or if proplist is NULL
 *	dimq_ERR_NOMEM - on out of memory
 *
 * Example:
 * > dimq_property *proplist = NULL;
 * > dimq_property_add_byte(&proplist, MQTT_PROP_PAYLOAD_FORMAT_IDENTIFIER, 1);
 */
libdimq_EXPORT int dimq_property_add_byte(dimq_property **proplist, int identifier, uint8_t value);

/*
 * Function: dimq_property_add_int16
 *
 * Add a new int16 property to a property list.
 *
 * If *proplist == NULL, a new list will be created, otherwise the new property
 * will be appended to the list.
 *
 * Parameters:
 *	proplist - pointer to dimq_property pointer, the list of properties
 *	identifier - property identifier (e.g. MQTT_PROP_RECEIVE_MAXIMUM)
 *	value - integer value for the new property
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if identifier is invalid, or if proplist is NULL
 *	dimq_ERR_NOMEM - on out of memory
 *
 * Example:
 * > dimq_property *proplist = NULL;
 * > dimq_property_add_int16(&proplist, MQTT_PROP_RECEIVE_MAXIMUM, 1000);
 */
libdimq_EXPORT int dimq_property_add_int16(dimq_property **proplist, int identifier, uint16_t value);

/*
 * Function: dimq_property_add_int32
 *
 * Add a new int32 property to a property list.
 *
 * If *proplist == NULL, a new list will be created, otherwise the new property
 * will be appended to the list.
 *
 * Parameters:
 *	proplist - pointer to dimq_property pointer, the list of properties
 *	identifier - property identifier (e.g. MQTT_PROP_MESSAGE_EXPIRY_INTERVAL)
 *	value - integer value for the new property
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if identifier is invalid, or if proplist is NULL
 *	dimq_ERR_NOMEM - on out of memory
 *
 * Example:
 * > dimq_property *proplist = NULL;
 * > dimq_property_add_int32(&proplist, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, 86400);
 */
libdimq_EXPORT int dimq_property_add_int32(dimq_property **proplist, int identifier, uint32_t value);

/*
 * Function: dimq_property_add_varint
 *
 * Add a new varint property to a property list.
 *
 * If *proplist == NULL, a new list will be created, otherwise the new property
 * will be appended to the list.
 *
 * Parameters:
 *	proplist - pointer to dimq_property pointer, the list of properties
 *	identifier - property identifier (e.g. MQTT_PROP_SUBSCRIPTION_IDENTIFIER)
 *	value - integer value for the new property
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if identifier is invalid, or if proplist is NULL
 *	dimq_ERR_NOMEM - on out of memory
 *
 * Example:
 * > dimq_property *proplist = NULL;
 * > dimq_property_add_varint(&proplist, MQTT_PROP_SUBSCRIPTION_IDENTIFIER, 1);
 */
libdimq_EXPORT int dimq_property_add_varint(dimq_property **proplist, int identifier, uint32_t value);

/*
 * Function: dimq_property_add_binary
 *
 * Add a new binary property to a property list.
 *
 * If *proplist == NULL, a new list will be created, otherwise the new property
 * will be appended to the list.
 *
 * Parameters:
 *	proplist - pointer to dimq_property pointer, the list of properties
 *	identifier - property identifier (e.g. MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 *	value - pointer to the property data
 *	len - length of property data in bytes
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if identifier is invalid, or if proplist is NULL
 *	dimq_ERR_NOMEM - on out of memory
 *
 * Example:
 * > dimq_property *proplist = NULL;
 * > dimq_property_add_binary(&proplist, MQTT_PROP_AUTHENTICATION_DATA, auth_data, auth_data_len);
 */
libdimq_EXPORT int dimq_property_add_binary(dimq_property **proplist, int identifier, const void *value, uint16_t len);

/*
 * Function: dimq_property_add_string
 *
 * Add a new string property to a property list.
 *
 * If *proplist == NULL, a new list will be created, otherwise the new property
 * will be appended to the list.
 *
 * Parameters:
 *	proplist - pointer to dimq_property pointer, the list of properties
 *	identifier - property identifier (e.g. MQTT_PROP_CONTENT_TYPE)
 *	value - string value for the new property, must be UTF-8 and zero terminated
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if identifier is invalid, if value is NULL, or if proplist is NULL
 *	dimq_ERR_NOMEM - on out of memory
 *	dimq_ERR_MALFORMED_UTF8 - value is not valid UTF-8.
 *
 * Example:
 * > dimq_property *proplist = NULL;
 * > dimq_property_add_string(&proplist, MQTT_PROP_CONTENT_TYPE, "application/json");
 */
libdimq_EXPORT int dimq_property_add_string(dimq_property **proplist, int identifier, const char *value);

/*
 * Function: dimq_property_add_string_pair
 *
 * Add a new string pair property to a property list.
 *
 * If *proplist == NULL, a new list will be created, otherwise the new property
 * will be appended to the list.
 *
 * Parameters:
 *	proplist - pointer to dimq_property pointer, the list of properties
 *	identifier - property identifier (e.g. MQTT_PROP_USER_PROPERTY)
 *	name - string name for the new property, must be UTF-8 and zero terminated
 *	value - string value for the new property, must be UTF-8 and zero terminated
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if identifier is invalid, if name or value is NULL, or if proplist is NULL
 *	dimq_ERR_NOMEM - on out of memory
 *	dimq_ERR_MALFORMED_UTF8 - if name or value are not valid UTF-8.
 *
 * Example:
 * > dimq_property *proplist = NULL;
 * > dimq_property_add_string_pair(&proplist, MQTT_PROP_USER_PROPERTY, "client", "dimq_pub");
 */
libdimq_EXPORT int dimq_property_add_string_pair(dimq_property **proplist, int identifier, const char *name, const char *value);


/*
 * Function: dimq_property_identifier
 *
 * Return the property identifier for a single property.
 *
 * Parameters:
 *	property - pointer to a valid dimq_property pointer.
 *
 * Returns:
 *  A valid property identifier on success
 *  0 - on error
 */
libdimq_EXPORT int dimq_property_identifier(const dimq_property *property);


/*
 * Function: dimq_property_next
 *
 * Return the next property in a property list. Use to iterate over a property
 * list, e.g.:
 *
 * (start code)
 * for(prop = proplist; prop != NULL; prop = dimq_property_next(prop)){
 * 	if(dimq_property_identifier(prop) == MQTT_PROP_CONTENT_TYPE){
 * 		...
 * 	}
 * }
 * (end)
 *
 * Parameters:
 *	proplist - pointer to dimq_property pointer, the list of properties
 *
 * Returns:
 *	Pointer to the next item in the list
 *	NULL, if proplist is NULL, or if there are no more items in the list.
 */
libdimq_EXPORT const dimq_property *dimq_property_next(const dimq_property *proplist);


/*
 * Function: dimq_property_read_byte
 *
 * Attempt to read a byte property matching an identifier, from a property list
 * or single property. This function can search for multiple entries of the
 * same identifier by using the returned value and skip_first. Note however
 * that it is forbidden for most properties to be duplicated.
 *
 * If the property is not found, *value will not be modified, so it is safe to
 * pass a variable with a default value to be potentially overwritten:
 *
 * (start code)
 * uint16_t keepalive = 60; // default value
 * // Get value from property list, or keep default if not found.
 * dimq_property_read_int16(proplist, MQTT_PROP_SERVER_KEEP_ALIVE, &keepalive, false);
 * (end)
 *
 * Parameters:
 *	proplist - dimq_property pointer, the list of properties or single property
 *	identifier - property identifier (e.g. MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 *	value - pointer to store the value, or NULL if the value is not required.
 *	skip_first - boolean that indicates whether the first item in the list
 *	             should be ignored or not. Should usually be set to false.
 *
 * Returns:
 *	A valid property pointer if the property is found
 *	NULL, if the property is not found, or proplist is NULL.
 *
 * Example:
 * (start code)
 *	// proplist is obtained from a callback
 *	dimq_property *prop;
 *	prop = dimq_property_read_byte(proplist, identifier, &value, false);
 *	while(prop){
 *		printf("value: %s\n", value);
 *		prop = dimq_property_read_byte(prop, identifier, &value);
 *	}
 * (end)
 */
libdimq_EXPORT const dimq_property *dimq_property_read_byte(
		const dimq_property *proplist,
		int identifier,
		uint8_t *value,
		bool skip_first);

/*
 * Function: dimq_property_read_int16
 *
 * Read an int16 property value from a property.
 *
 * Parameters:
 *	property - property to read
 *	identifier - property identifier (e.g. MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 *	value - pointer to store the value, or NULL if the value is not required.
 *	skip_first - boolean that indicates whether the first item in the list
 *	             should be ignored or not. Should usually be set to false.
 *
 * Returns:
 *	A valid property pointer if the property is found
 *	NULL, if the property is not found, or proplist is NULL.
 *
 * Example:
 *	See <dimq_property_read_byte>
 */
libdimq_EXPORT const dimq_property *dimq_property_read_int16(
		const dimq_property *proplist,
		int identifier,
		uint16_t *value,
		bool skip_first);

/*
 * Function: dimq_property_read_int32
 *
 * Read an int32 property value from a property.
 *
 * Parameters:
 *	property - pointer to dimq_property pointer, the list of properties
 *	identifier - property identifier (e.g. MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 *	value - pointer to store the value, or NULL if the value is not required.
 *	skip_first - boolean that indicates whether the first item in the list
 *	             should be ignored or not. Should usually be set to false.
 *
 * Returns:
 *	A valid property pointer if the property is found
 *	NULL, if the property is not found, or proplist is NULL.
 *
 * Example:
 *	See <dimq_property_read_byte>
 */
libdimq_EXPORT const dimq_property *dimq_property_read_int32(
		const dimq_property *proplist,
		int identifier,
		uint32_t *value,
		bool skip_first);

/*
 * Function: dimq_property_read_varint
 *
 * Read a varint property value from a property.
 *
 * Parameters:
 *	property - property to read
 *	identifier - property identifier (e.g. MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 *	value - pointer to store the value, or NULL if the value is not required.
 *	skip_first - boolean that indicates whether the first item in the list
 *	             should be ignored or not. Should usually be set to false.
 *
 * Returns:
 *	A valid property pointer if the property is found
 *	NULL, if the property is not found, or proplist is NULL.
 *
 * Example:
 *	See <dimq_property_read_byte>
 */
libdimq_EXPORT const dimq_property *dimq_property_read_varint(
		const dimq_property *proplist,
		int identifier,
		uint32_t *value,
		bool skip_first);

/*
 * Function: dimq_property_read_binary
 *
 * Read a binary property value from a property.
 *
 * On success, value must be free()'d by the application.
 *
 * Parameters:
 *	property - property to read
 *	identifier - property identifier (e.g. MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 *	value - pointer to store the value, or NULL if the value is not required.
 *	skip_first - boolean that indicates whether the first item in the list
 *	             should be ignored or not. Should usually be set to false.
 *
 * Returns:
 *	A valid property pointer if the property is found
 *	NULL, if the property is not found, or proplist is NULL, or if an out of memory condition occurred.
 *
 * Example:
 *	See <dimq_property_read_byte>
 */
libdimq_EXPORT const dimq_property *dimq_property_read_binary(
		const dimq_property *proplist,
		int identifier,
		void **value,
		uint16_t *len,
		bool skip_first);

/*
 * Function: dimq_property_read_string
 *
 * Read a string property value from a property.
 *
 * On success, value must be free()'d by the application.
 *
 * Parameters:
 *	property - property to read
 *	identifier - property identifier (e.g. MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 *	value - pointer to char*, for the property data to be stored in, or NULL if
 *	        the value is not required.
 *	skip_first - boolean that indicates whether the first item in the list
 *	             should be ignored or not. Should usually be set to false.
 *
 * Returns:
 *	A valid property pointer if the property is found
 *	NULL, if the property is not found, or proplist is NULL, or if an out of memory condition occurred.
 *
 * Example:
 *	See <dimq_property_read_byte>
 */
libdimq_EXPORT const dimq_property *dimq_property_read_string(
		const dimq_property *proplist,
		int identifier,
		char **value,
		bool skip_first);

/*
 * Function: dimq_property_read_string_pair
 *
 * Read a string pair property value pair from a property.
 *
 * On success, name and value must be free()'d by the application.
 *
 * Parameters:
 *	property - property to read
 *	identifier - property identifier (e.g. MQTT_PROP_PAYLOAD_FORMAT_INDICATOR)
 *	name - pointer to char* for the name property data to be stored in, or NULL
 *	       if the name is not required.
 *	value - pointer to char*, for the property data to be stored in, or NULL if
 *	        the value is not required.
 *	skip_first - boolean that indicates whether the first item in the list
 *	             should be ignored or not. Should usually be set to false.
 *
 * Returns:
 *	A valid property pointer if the property is found
 *	NULL, if the property is not found, or proplist is NULL, or if an out of memory condition occurred.
 *
 * Example:
 *	See <dimq_property_read_byte>
 */
libdimq_EXPORT const dimq_property *dimq_property_read_string_pair(
		const dimq_property *proplist,
		int identifier,
		char **name,
		char **value,
		bool skip_first);

/*
 * Function: dimq_property_free_all
 *
 * Free all properties from a list of properties. Frees the list and sets *properties to NULL.
 *
 * Parameters:
 *   properties - list of properties to free
 *
 * Example:
 * > dimq_properties *properties = NULL;
 * > // Add properties
 * > dimq_property_free_all(&properties);
 */
libdimq_EXPORT void dimq_property_free_all(dimq_property **properties);

/*
 * Function: dimq_property_copy_all
 *
 * Parameters:
 *    dest - pointer for new property list
 *    src - property list
 *
 * Returns:
 *    dimq_ERR_SUCCESS - on successful copy
 *    dimq_ERR_INVAL - if dest is NULL
 *    dimq_ERR_NOMEM - on out of memory (dest will be set to NULL)
 */
libdimq_EXPORT int dimq_property_copy_all(dimq_property **dest, const dimq_property *src);

/*
 * Function: dimq_property_check_command
 *
 * Check whether a property identifier is valid for the given command.
 *
 * Parameters:
 *   command - MQTT command (e.g. CMD_CONNECT)
 *   identifier - MQTT property (e.g. MQTT_PROP_USER_PROPERTY)
 *
 * Returns:
 *   dimq_ERR_SUCCESS - if the identifier is valid for command
 *   dimq_ERR_PROTOCOL - if the identifier is not valid for use with command.
 */
libdimq_EXPORT int dimq_property_check_command(int command, int identifier);


/*
 * Function: dimq_property_check_all
 *
 * Check whether a list of properties are valid for a particular command,
 * whether there are duplicates, and whether the values are valid where
 * possible.
 *
 * Note that this function is used internally in the library whenever
 * properties are passed to it, so in basic use this is not needed, but should
 * be helpful to check property lists *before* the point of using them.
 *
 * Parameters:
 *	command - MQTT command (e.g. CMD_CONNECT)
 *	properties - list of MQTT properties to check.
 *
 * Returns:
 *	dimq_ERR_SUCCESS - if all properties are valid
 *	dimq_ERR_DUPLICATE_PROPERTY - if a property is duplicated where it is forbidden.
 *	dimq_ERR_PROTOCOL - if any property is invalid
 */
libdimq_EXPORT int dimq_property_check_all(int command, const dimq_property *properties);

/*
 * Function: dimq_property_identifier_to_string
 *
 * Return the property name as a string for a property identifier.
 * The property name is as defined in the MQTT specification, with - as a
 * separator, for example: payload-format-indicator.
 *
 * Parameters:
 *	identifier - valid MQTT property identifier integer
 *
 * Returns:
 *  A const string to the property name on success
 *  NULL on failure
 */
libdimq_EXPORT const char *dimq_property_identifier_to_string(int identifier);


/* Function: dimq_string_to_property_info
 *
 * Parse a property name string and convert to a property identifier and data type.
 * The property name is as defined in the MQTT specification, with - as a
 * separator, for example: payload-format-indicator.
 *
 * Parameters:
 *	propname - the string to parse
 *	identifier - pointer to an int to receive the property identifier
 *	type - pointer to an int to receive the property type
 *
 * Returns:
 *	dimq_ERR_SUCCESS - on success
 *	dimq_ERR_INVAL - if the string does not match a property
 *
 * Example:
 * (start code)
 *	dimq_string_to_property_info("response-topic", &id, &type);
 *	// id == MQTT_PROP_RESPONSE_TOPIC
 *	// type == MQTT_PROP_TYPE_STRING
 * (end)
 */
libdimq_EXPORT int dimq_string_to_property_info(const char *propname, int *identifier, int *type);


#ifdef __cplusplus
}
#endif

#endif
