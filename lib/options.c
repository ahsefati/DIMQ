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

#include "config.h"

#ifndef WIN32
#  include <strings.h>
#endif

#include <string.h>

#ifdef WITH_TLS
#  ifdef WIN32
#    include <winsock2.h>
#  endif
#  include <openssl/engine.h>
#endif

#include "dimq.h"
#include "dimq_internal.h"
#include "memory_dimq.h"
#include "misc_dimq.h"
#include "mqtt_protocol.h"
#include "util_dimq.h"
#include "will_dimq.h"


int dimq_will_set(struct dimq *dimq, const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	return dimq_will_set_v5(dimq, topic, payloadlen, payload, qos, retain, NULL);
}


int dimq_will_set_v5(struct dimq *dimq, const char *topic, int payloadlen, const void *payload, int qos, bool retain, dimq_property *properties)
{
	int rc;

	if(!dimq) return dimq_ERR_INVAL;

	if(properties){
		rc = dimq_property_check_all(CMD_WILL, properties);
		if(rc) return rc;
	}

	return will__set(dimq, topic, payloadlen, payload, qos, retain, properties);
}


int dimq_will_clear(struct dimq *dimq)
{
	if(!dimq) return dimq_ERR_INVAL;
	return will__clear(dimq);
}


int dimq_username_pw_set(struct dimq *dimq, const char *username, const char *password)
{
	size_t slen;

	if(!dimq) return dimq_ERR_INVAL;

	if(dimq->protocol == dimq_p_mqtt311 || dimq->protocol == dimq_p_mqtt31){
		if(password != NULL && username == NULL){
			return dimq_ERR_INVAL;
		}
	}

	dimq__free(dimq->username);
	dimq->username = NULL;

	dimq__free(dimq->password);
	dimq->password = NULL;

	if(username){
		slen = strlen(username);
		if(slen > UINT16_MAX){
			return dimq_ERR_INVAL;
		}
		if(dimq_validate_utf8(username, (int)slen)){
			return dimq_ERR_MALFORMED_UTF8;
		}
		dimq->username = dimq__strdup(username);
		if(!dimq->username) return dimq_ERR_NOMEM;
	}

	if(password){
		dimq->password = dimq__strdup(password);
		if(!dimq->password){
			dimq__free(dimq->username);
			dimq->username = NULL;
			return dimq_ERR_NOMEM;
		}
	}
	return dimq_ERR_SUCCESS;
}


int dimq_reconnect_delay_set(struct dimq *dimq, unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff)
{
	if(!dimq) return dimq_ERR_INVAL;
	
	if(reconnect_delay == 0) reconnect_delay = 1;

	dimq->reconnect_delay = reconnect_delay;
	dimq->reconnect_delay_max = reconnect_delay_max;
	dimq->reconnect_exponential_backoff = reconnect_exponential_backoff;
	
	return dimq_ERR_SUCCESS;
	
}


int dimq_tls_set(struct dimq *dimq, const char *cafile, const char *capath, const char *certfile, const char *keyfile, int (*pw_callback)(char *buf, int size, int rwflag, void *userdata))
{
#ifdef WITH_TLS
	FILE *fptr;

	if(!dimq || (!cafile && !capath) || (certfile && !keyfile) || (!certfile && keyfile)) return dimq_ERR_INVAL;

	dimq__free(dimq->tls_cafile);
	dimq->tls_cafile = NULL;
	if(cafile){
		fptr = dimq__fopen(cafile, "rt", false);
		if(fptr){
			fclose(fptr);
		}else{
			return dimq_ERR_INVAL;
		}
		dimq->tls_cafile = dimq__strdup(cafile);

		if(!dimq->tls_cafile){
			return dimq_ERR_NOMEM;
		}
	}

	dimq__free(dimq->tls_capath);
	dimq->tls_capath = NULL;
	if(capath){
		dimq->tls_capath = dimq__strdup(capath);
		if(!dimq->tls_capath){
			return dimq_ERR_NOMEM;
		}
	}

	dimq__free(dimq->tls_certfile);
	dimq->tls_certfile = NULL;
	if(certfile){
		fptr = dimq__fopen(certfile, "rt", false);
		if(fptr){
			fclose(fptr);
		}else{
			dimq__free(dimq->tls_cafile);
			dimq->tls_cafile = NULL;

			dimq__free(dimq->tls_capath);
			dimq->tls_capath = NULL;
			return dimq_ERR_INVAL;
		}
		dimq->tls_certfile = dimq__strdup(certfile);
		if(!dimq->tls_certfile){
			return dimq_ERR_NOMEM;
		}
	}

	dimq__free(dimq->tls_keyfile);
	dimq->tls_keyfile = NULL;
	if(keyfile){
		fptr = dimq__fopen(keyfile, "rt", false);
		if(fptr){
			fclose(fptr);
		}else{
			dimq__free(dimq->tls_cafile);
			dimq->tls_cafile = NULL;

			dimq__free(dimq->tls_capath);
			dimq->tls_capath = NULL;

			dimq__free(dimq->tls_certfile);
			dimq->tls_certfile = NULL;
			return dimq_ERR_INVAL;
		}
		dimq->tls_keyfile = dimq__strdup(keyfile);
		if(!dimq->tls_keyfile){
			return dimq_ERR_NOMEM;
		}
	}

	dimq->tls_pw_callback = pw_callback;


	return dimq_ERR_SUCCESS;
#else
	UNUSED(dimq);
	UNUSED(cafile);
	UNUSED(capath);
	UNUSED(certfile);
	UNUSED(keyfile);
	UNUSED(pw_callback);

	return dimq_ERR_NOT_SUPPORTED;

#endif
}


int dimq_tls_opts_set(struct dimq *dimq, int cert_reqs, const char *tls_version, const char *ciphers)
{
#ifdef WITH_TLS
	if(!dimq) return dimq_ERR_INVAL;

	dimq->tls_cert_reqs = cert_reqs;
	if(tls_version){
		if(!strcasecmp(tls_version, "tlsv1.3")
				|| !strcasecmp(tls_version, "tlsv1.2")
				|| !strcasecmp(tls_version, "tlsv1.1")){

			dimq->tls_version = dimq__strdup(tls_version);
			if(!dimq->tls_version) return dimq_ERR_NOMEM;
		}else{
			return dimq_ERR_INVAL;
		}
	}else{
		dimq->tls_version = dimq__strdup("tlsv1.2");
		if(!dimq->tls_version) return dimq_ERR_NOMEM;
	}
	if(ciphers){
		dimq->tls_ciphers = dimq__strdup(ciphers);
		if(!dimq->tls_ciphers) return dimq_ERR_NOMEM;
	}else{
		dimq->tls_ciphers = NULL;
	}


	return dimq_ERR_SUCCESS;
#else
	UNUSED(dimq);
	UNUSED(cert_reqs);
	UNUSED(tls_version);
	UNUSED(ciphers);

	return dimq_ERR_NOT_SUPPORTED;
#endif
}


int dimq_tls_insecure_set(struct dimq *dimq, bool value)
{
#ifdef WITH_TLS
	if(!dimq) return dimq_ERR_INVAL;
	dimq->tls_insecure = value;
	return dimq_ERR_SUCCESS;
#else
	UNUSED(dimq);
	UNUSED(value);

	return dimq_ERR_NOT_SUPPORTED;
#endif
}


int dimq_string_option(struct dimq *dimq, enum dimq_opt_t option, const char *value)
{
#ifdef WITH_TLS
	ENGINE *eng;
	char *str;
#endif

	if(!dimq) return dimq_ERR_INVAL;

	switch(option){
		case dimq_OPT_TLS_ENGINE:
#if defined(WITH_TLS) && !defined(OPENSSL_NO_ENGINE)
			eng = ENGINE_by_id(value);
			if(!eng){
				return dimq_ERR_INVAL;
			}
			ENGINE_free(eng); /* release the structural reference from ENGINE_by_id() */
			dimq->tls_engine = dimq__strdup(value);
			if(!dimq->tls_engine){
				return dimq_ERR_NOMEM;
			}
			return dimq_ERR_SUCCESS;
#else
			return dimq_ERR_NOT_SUPPORTED;
#endif
			break;

		case dimq_OPT_TLS_KEYFORM:
#ifdef WITH_TLS
			if(!value) return dimq_ERR_INVAL;
			if(!strcasecmp(value, "pem")){
				dimq->tls_keyform = dimq_k_pem;
			}else if (!strcasecmp(value, "engine")){
				dimq->tls_keyform = dimq_k_engine;
			}else{
				return dimq_ERR_INVAL;
			}
			return dimq_ERR_SUCCESS;
#else
			return dimq_ERR_NOT_SUPPORTED;
#endif
			break;


		case dimq_OPT_TLS_ENGINE_KPASS_SHA1:
#ifdef WITH_TLS
			if(dimq__hex2bin_sha1(value, (unsigned char**)&str) != dimq_ERR_SUCCESS){
				return dimq_ERR_INVAL;
			}
			dimq->tls_engine_kpass_sha1 = str;
			return dimq_ERR_SUCCESS;
#else
			return dimq_ERR_NOT_SUPPORTED;
#endif
			break;

		case dimq_OPT_TLS_ALPN:
#ifdef WITH_TLS
			dimq->tls_alpn = dimq__strdup(value);
			if(!dimq->tls_alpn){
				return dimq_ERR_NOMEM;
			}
			return dimq_ERR_SUCCESS;
#else
			return dimq_ERR_NOT_SUPPORTED;
#endif
			break;

		case dimq_OPT_BIND_ADDRESS:
			dimq__free(dimq->bind_address);
			if(value){
				dimq->bind_address = dimq__strdup(value);
				if(dimq->bind_address){
					return dimq_ERR_SUCCESS;
				}else{
					return dimq_ERR_NOMEM;
				}
			}else{
				return dimq_ERR_SUCCESS;
			}


		default:
			return dimq_ERR_INVAL;
	}
}


int dimq_tls_psk_set(struct dimq *dimq, const char *psk, const char *identity, const char *ciphers)
{
#ifdef FINAL_WITH_TLS_PSK
	if(!dimq || !psk || !identity) return dimq_ERR_INVAL;

	/* Check for hex only digits */
	if(strspn(psk, "0123456789abcdefABCDEF") < strlen(psk)){
		return dimq_ERR_INVAL;
	}
	dimq->tls_psk = dimq__strdup(psk);
	if(!dimq->tls_psk) return dimq_ERR_NOMEM;

	dimq->tls_psk_identity = dimq__strdup(identity);
	if(!dimq->tls_psk_identity){
		dimq__free(dimq->tls_psk);
		return dimq_ERR_NOMEM;
	}
	if(ciphers){
		dimq->tls_ciphers = dimq__strdup(ciphers);
		if(!dimq->tls_ciphers) return dimq_ERR_NOMEM;
	}else{
		dimq->tls_ciphers = NULL;
	}

	return dimq_ERR_SUCCESS;
#else
	UNUSED(dimq);
	UNUSED(psk);
	UNUSED(identity);
	UNUSED(ciphers);

	return dimq_ERR_NOT_SUPPORTED;
#endif
}


int dimq_opts_set(struct dimq *dimq, enum dimq_opt_t option, void *value)
{
	int ival;

	if(!dimq) return dimq_ERR_INVAL;

	switch(option){
		case dimq_OPT_PROTOCOL_VERSION:
			if(value == NULL){
				return dimq_ERR_INVAL;
			}
			ival = *((int *)value);
			return dimq_int_option(dimq, option, ival);
		case dimq_OPT_SSL_CTX:
			return dimq_void_option(dimq, option, value);
		default:
			return dimq_ERR_INVAL;
	}
	return dimq_ERR_SUCCESS;
}


int dimq_int_option(struct dimq *dimq, enum dimq_opt_t option, int value)
{
	if(!dimq) return dimq_ERR_INVAL;

	switch(option){
		case dimq_OPT_PROTOCOL_VERSION:
			if(value == MQTT_PROTOCOL_V31){
				dimq->protocol = dimq_p_mqtt31;
			}else if(value == MQTT_PROTOCOL_V311){
				dimq->protocol = dimq_p_mqtt311;
			}else if(value == MQTT_PROTOCOL_V5){
				dimq->protocol = dimq_p_mqtt5;
			}else{
				return dimq_ERR_INVAL;
			}
			break;

		case dimq_OPT_RECEIVE_MAXIMUM:
			if(value < 0 || value > UINT16_MAX){
				return dimq_ERR_INVAL;
			}
			if(value == 0){
				dimq->msgs_in.inflight_maximum = UINT16_MAX;
			}else{
				dimq->msgs_in.inflight_maximum = (uint16_t)value;
			}
			break;

		case dimq_OPT_SEND_MAXIMUM:
			if(value < 0 || value > UINT16_MAX){
				return dimq_ERR_INVAL;
			}
			if(value == 0){
				dimq->msgs_out.inflight_maximum = UINT16_MAX;
			}else{
				dimq->msgs_out.inflight_maximum = (uint16_t)value;
			}
			break;

		case dimq_OPT_SSL_CTX_WITH_DEFAULTS:
#if defined(WITH_TLS) && OPENSSL_VERSION_NUMBER >= 0x10100000L
			if(value){
				dimq->ssl_ctx_defaults = true;
			}else{
				dimq->ssl_ctx_defaults = false;
			}
			break;
#else
			return dimq_ERR_NOT_SUPPORTED;
#endif

		case dimq_OPT_TLS_USE_OS_CERTS:
#ifdef WITH_TLS
			if(value){
				dimq->tls_use_os_certs = true;
			}else{
				dimq->tls_use_os_certs = false;
			}
			break;
#else
			return dimq_ERR_NOT_SUPPORTED;
#endif

		case dimq_OPT_TLS_OCSP_REQUIRED:
#ifdef WITH_TLS
			dimq->tls_ocsp_required = (bool)value;
#else
			return dimq_ERR_NOT_SUPPORTED;
#endif
			break;

		case dimq_OPT_TCP_NODELAY:
			dimq->tcp_nodelay = (bool)value;
			break;

		default:
			return dimq_ERR_INVAL;
	}
	return dimq_ERR_SUCCESS;
}


int dimq_void_option(struct dimq *dimq, enum dimq_opt_t option, void *value)
{
	if(!dimq) return dimq_ERR_INVAL;

	switch(option){
		case dimq_OPT_SSL_CTX:
#ifdef WITH_TLS
			dimq->user_ssl_ctx = (SSL_CTX *)value;
			if(dimq->user_ssl_ctx){
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
				SSL_CTX_up_ref(dimq->user_ssl_ctx);
#else
				CRYPTO_add(&(dimq->user_ssl_ctx)->references, 1, CRYPTO_LOCK_SSL_CTX);
#endif
			}
			break;
#else
			return dimq_ERR_NOT_SUPPORTED;
#endif
		default:
			return dimq_ERR_INVAL;
	}
	return dimq_ERR_SUCCESS;
}


void dimq_user_data_set(struct dimq *dimq, void *userdata)
{
	if(dimq){
		dimq->userdata = userdata;
	}
}

void *dimq_userdata(struct dimq *dimq)
{
	return dimq->userdata;
}
