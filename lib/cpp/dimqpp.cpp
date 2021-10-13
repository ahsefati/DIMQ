/*
Copyright (c) 2010-2019 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <cstdlib>
#include <dimq.h>
#include <dimqpp.h>

#define UNUSED(A) (void)(A)

namespace dimqpp {

static void on_connect_wrapper(struct dimq *dimq, void *userdata, int rc)
{
	class dimqpp *m = (class dimqpp *)userdata;

	UNUSED(dimq);

	m->on_connect(rc);
}

static void on_connect_with_flags_wrapper(struct dimq *dimq, void *userdata, int rc, int flags)
{
	class dimqpp *m = (class dimqpp *)userdata;
	UNUSED(dimq);
	m->on_connect_with_flags(rc, flags);
}

static void on_disconnect_wrapper(struct dimq *dimq, void *userdata, int rc)
{
	class dimqpp *m = (class dimqpp *)userdata;
	UNUSED(dimq);
	m->on_disconnect(rc);
}

static void on_publish_wrapper(struct dimq *dimq, void *userdata, int mid)
{
	class dimqpp *m = (class dimqpp *)userdata;
	UNUSED(dimq);
	m->on_publish(mid);
}

static void on_message_wrapper(struct dimq *dimq, void *userdata, const struct dimq_message *message)
{
	class dimqpp *m = (class dimqpp *)userdata;
	UNUSED(dimq);
	m->on_message(message);
}

static void on_subscribe_wrapper(struct dimq *dimq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	class dimqpp *m = (class dimqpp *)userdata;
	UNUSED(dimq);
	m->on_subscribe(mid, qos_count, granted_qos);
}

static void on_unsubscribe_wrapper(struct dimq *dimq, void *userdata, int mid)
{
	class dimqpp *m = (class dimqpp *)userdata;
	UNUSED(dimq);
	m->on_unsubscribe(mid);
}


static void on_log_wrapper(struct dimq *dimq, void *userdata, int level, const char *str)
{
	class dimqpp *m = (class dimqpp *)userdata;
	UNUSED(dimq);
	m->on_log(level, str);
}

int lib_version(int *major, int *minor, int *revision)
{
	if(major) *major = LIBdimq_MAJOR;
	if(minor) *minor = LIBdimq_MINOR;
	if(revision) *revision = LIBdimq_REVISION;
	return LIBdimq_VERSION_NUMBER;
}

int lib_init()
{
	return dimq_lib_init();
}

int lib_cleanup()
{
	return dimq_lib_cleanup();
}

const char* strerror(int dimq_errno)
{
	return dimq_strerror(dimq_errno);
}

const char* connack_string(int connack_code)
{
	return dimq_connack_string(connack_code);
}

int sub_topic_tokenise(const char *subtopic, char ***topics, int *count)
{
	return dimq_sub_topic_tokenise(subtopic, topics, count);
}

int sub_topic_tokens_free(char ***topics, int count)
{
	return dimq_sub_topic_tokens_free(topics, count);
}

int topic_matches_sub(const char *sub, const char *topic, bool *result)
{
	return dimq_topic_matches_sub(sub, topic, result);
}

int validate_utf8(const char *str, int len)
{
	return dimq_validate_utf8(str, len);
}

int subscribe_simple(
		struct dimq_message **messages,
		int msg_count,
		bool retained,
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
		const struct libdimq_tls *tls)
{
	return dimq_subscribe_simple(
			messages, msg_count, retained,
			topic, qos,
			host, port, client_id, keepalive, clean_session,
			username, password,
			will, tls);
}

dimqpp_EXPORT int subscribe_callback(
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
		const struct libdimq_tls *tls)
{
	return dimq_subscribe_callback(
			callback, userdata,
			topic, qos,
			host, port, client_id, keepalive, clean_session,
			username, password,
			will, tls);
}


dimqpp::dimqpp(const char *id, bool clean_session)
{
	m_dimq = dimq_new(id, clean_session, this);
	dimq_connect_callback_set(m_dimq, on_connect_wrapper);
	dimq_connect_with_flags_callback_set(m_dimq, on_connect_with_flags_wrapper);
	dimq_disconnect_callback_set(m_dimq, on_disconnect_wrapper);
	dimq_publish_callback_set(m_dimq, on_publish_wrapper);
	dimq_message_callback_set(m_dimq, on_message_wrapper);
	dimq_subscribe_callback_set(m_dimq, on_subscribe_wrapper);
	dimq_unsubscribe_callback_set(m_dimq, on_unsubscribe_wrapper);
	dimq_log_callback_set(m_dimq, on_log_wrapper);
}

dimqpp::~dimqpp()
{
	dimq_destroy(m_dimq);
}

int dimqpp::reinitialise(const char *id, bool clean_session)
{
	int rc;
	rc = dimq_reinitialise(m_dimq, id, clean_session, this);
	if(rc == dimq_ERR_SUCCESS){
		dimq_connect_callback_set(m_dimq, on_connect_wrapper);
		dimq_connect_with_flags_callback_set(m_dimq, on_connect_with_flags_wrapper);
		dimq_disconnect_callback_set(m_dimq, on_disconnect_wrapper);
		dimq_publish_callback_set(m_dimq, on_publish_wrapper);
		dimq_message_callback_set(m_dimq, on_message_wrapper);
		dimq_subscribe_callback_set(m_dimq, on_subscribe_wrapper);
		dimq_unsubscribe_callback_set(m_dimq, on_unsubscribe_wrapper);
		dimq_log_callback_set(m_dimq, on_log_wrapper);
	}
	return rc;
}

int dimqpp::connect(const char *host, int port, int keepalive)
{
	return dimq_connect(m_dimq, host, port, keepalive);
}

int dimqpp::connect(const char *host, int port, int keepalive, const char *bind_address)
{
	return dimq_connect_bind(m_dimq, host, port, keepalive, bind_address);
}

int dimqpp::connect_async(const char *host, int port, int keepalive)
{
	return dimq_connect_async(m_dimq, host, port, keepalive);
}

int dimqpp::connect_async(const char *host, int port, int keepalive, const char *bind_address)
{
	return dimq_connect_bind_async(m_dimq, host, port, keepalive, bind_address);
}

int dimqpp::reconnect()
{
	return dimq_reconnect(m_dimq);
}

int dimqpp::reconnect_async()
{
	return dimq_reconnect_async(m_dimq);
}

int dimqpp::disconnect()
{
	return dimq_disconnect(m_dimq);
}

int dimqpp::socket()
{
	return dimq_socket(m_dimq);
}

int dimqpp::will_set(const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	return dimq_will_set(m_dimq, topic, payloadlen, payload, qos, retain);
}

int dimqpp::will_clear()
{
	return dimq_will_clear(m_dimq);
}

int dimqpp::username_pw_set(const char *username, const char *password)
{
	return dimq_username_pw_set(m_dimq, username, password);
}

int dimqpp::publish(int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	return dimq_publish(m_dimq, mid, topic, payloadlen, payload, qos, retain);
}

void dimqpp::reconnect_delay_set(unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff)
{
	dimq_reconnect_delay_set(m_dimq, reconnect_delay, reconnect_delay_max, reconnect_exponential_backoff);
}

int dimqpp::max_inflight_messages_set(unsigned int max_inflight_messages)
{
	return dimq_max_inflight_messages_set(m_dimq, max_inflight_messages);
}

void dimqpp::message_retry_set(unsigned int message_retry)
{
	dimq_message_retry_set(m_dimq, message_retry);
}

int dimqpp::subscribe(int *mid, const char *sub, int qos)
{
	return dimq_subscribe(m_dimq, mid, sub, qos);
}

int dimqpp::unsubscribe(int *mid, const char *sub)
{
	return dimq_unsubscribe(m_dimq, mid, sub);
}

int dimqpp::loop(int timeout, int max_packets)
{
	return dimq_loop(m_dimq, timeout, max_packets);
}

int dimqpp::loop_misc()
{
	return dimq_loop_misc(m_dimq);
}

int dimqpp::loop_read(int max_packets)
{
	return dimq_loop_read(m_dimq, max_packets);
}

int dimqpp::loop_write(int max_packets)
{
	return dimq_loop_write(m_dimq, max_packets);
}

int dimqpp::loop_forever(int timeout, int max_packets)
{
	return dimq_loop_forever(m_dimq, timeout, max_packets);
}

int dimqpp::loop_start()
{
	return dimq_loop_start(m_dimq);
}

int dimqpp::loop_stop(bool force)
{
	return dimq_loop_stop(m_dimq, force);
}

bool dimqpp::want_write()
{
	return dimq_want_write(m_dimq);
}

int dimqpp::opts_set(enum dimq_opt_t option, void *value)
{
	return dimq_opts_set(m_dimq, option, value);
}

int dimqpp::threaded_set(bool threaded)
{
	return dimq_threaded_set(m_dimq, threaded);
}

void dimqpp::user_data_set(void *userdata)
{
	dimq_user_data_set(m_dimq, userdata);
}

int dimqpp::socks5_set(const char *host, int port, const char *username, const char *password)
{
	return dimq_socks5_set(m_dimq, host, port, username, password);
}


int dimqpp::tls_set(const char *cafile, const char *capath, const char *certfile, const char *keyfile, int (*pw_callback)(char *buf, int size, int rwflag, void *userdata))
{
	return dimq_tls_set(m_dimq, cafile, capath, certfile, keyfile, pw_callback);
}

int dimqpp::tls_opts_set(int cert_reqs, const char *tls_version, const char *ciphers)
{
	return dimq_tls_opts_set(m_dimq, cert_reqs, tls_version, ciphers);
}

int dimqpp::tls_insecure_set(bool value)
{
	return dimq_tls_insecure_set(m_dimq, value);
}

int dimqpp::tls_psk_set(const char *psk, const char *identity, const char *ciphers)
{
	return dimq_tls_psk_set(m_dimq, psk, identity, ciphers);
}

}
