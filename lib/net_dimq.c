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

#define _GNU_SOURCE
#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#define _GNU_SOURCE
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifdef __ANDROID__
#include <linux/in.h>
#include <linux/in6.h>
#include <sys/endian.h>
#endif

#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif

#ifdef WITH_UNIX_SOCKETS
#  include <sys/un.h>
#endif

#ifdef __QNX__
#include <net/netbyte.h>
#endif

#ifdef WITH_TLS
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/ui.h>
#include <tls_dimq.h>
#endif

#ifdef WITH_BROKER
#  include "dimq_broker_internal.h"
#  ifdef WITH_WEBSOCKETS
#    include <libwebsockets.h>
#  endif
#else
#  include "read_handle.h"
#endif

#include "logging_dimq.h"
#include "memory_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "time_dimq.h"
#include "util_dimq.h"

#ifdef WITH_TLS
int tls_ex_index_dimq = -1;
UI_METHOD *_ui_method = NULL;

static bool is_tls_initialized = false;

/* Functions taken from OpenSSL s_server/s_client */
static int ui_open(UI *ui)
{
	return UI_method_get_opener(UI_OpenSSL())(ui);
}

static int ui_read(UI *ui, UI_STRING *uis)
{
	return UI_method_get_reader(UI_OpenSSL())(ui, uis);
}

static int ui_write(UI *ui, UI_STRING *uis)
{
	return UI_method_get_writer(UI_OpenSSL())(ui, uis);
}

static int ui_close(UI *ui)
{
	return UI_method_get_closer(UI_OpenSSL())(ui);
}

static void setup_ui_method(void)
{
	_ui_method = UI_create_method("OpenSSL application user interface");
	UI_method_set_opener(_ui_method, ui_open);
	UI_method_set_reader(_ui_method, ui_read);
	UI_method_set_writer(_ui_method, ui_write);
	UI_method_set_closer(_ui_method, ui_close);
}

static void cleanup_ui_method(void)
{
	if(_ui_method){
		UI_destroy_method(_ui_method);
		_ui_method = NULL;
	}
}

UI_METHOD *net__get_ui_method(void)
{
	return _ui_method;
}

#endif

int net__init(void)
{
#ifdef WIN32
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
		return dimq_ERR_UNKNOWN;
	}
#endif

#ifdef WITH_SRV
	ares_library_init(ARES_LIB_INIT_ALL);
#endif

	return dimq_ERR_SUCCESS;
}

void net__cleanup(void)
{
#ifdef WITH_TLS
#  if OPENSSL_VERSION_NUMBER < 0x10100000L
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
	ERR_remove_thread_state(NULL);
	EVP_cleanup();

#    if !defined(OPENSSL_NO_ENGINE)
	ENGINE_cleanup();
#    endif
	is_tls_initialized = false;
#  endif

	CONF_modules_unload(1);
	cleanup_ui_method();
#endif

#ifdef WITH_SRV
	ares_library_cleanup();
#endif

#ifdef WIN32
	WSACleanup();
#endif
}

#ifdef WITH_TLS
void net__init_tls(void)
{
	if(is_tls_initialized) return;

#  if OPENSSL_VERSION_NUMBER < 0x10100000L
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
#  else
	OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS \
			| OPENSSL_INIT_ADD_ALL_DIGESTS \
			| OPENSSL_INIT_LOAD_CONFIG, NULL);
#  endif
#if !defined(OPENSSL_NO_ENGINE)
	ENGINE_load_builtin_engines();
#endif
	setup_ui_method();
	if(tls_ex_index_dimq == -1){
		tls_ex_index_dimq = SSL_get_ex_new_index(0, "client context", NULL, NULL, NULL);
	}

	is_tls_initialized = true;
}
#endif

/* Close a socket associated with a context and set it to -1.
 * Returns 1 on failure (context is NULL)
 * Returns 0 on success.
 */
int net__socket_close(struct dimq *dimq)
{
	int rc = 0;
#ifdef WITH_BROKER
	struct dimq *dimq_found;
#endif

	assert(dimq);
#ifdef WITH_TLS
#ifdef WITH_WEBSOCKETS
	if(!dimq->wsi)
#endif
	{
		if(dimq->ssl){
			if(!SSL_in_init(dimq->ssl)){
				SSL_shutdown(dimq->ssl);
			}
			SSL_free(dimq->ssl);
			dimq->ssl = NULL;
		}
	}
#endif

#ifdef WITH_WEBSOCKETS
	if(dimq->wsi)
	{
		if(dimq->state != dimq_cs_disconnecting){
			dimq__set_state(dimq, dimq_cs_disconnect_ws);
		}
		lws_callback_on_writable(dimq->wsi);
	}else
#endif
	{
		if(dimq->sock != INVALID_SOCKET){
#ifdef WITH_BROKER
			HASH_FIND(hh_sock, db.contexts_by_sock, &dimq->sock, sizeof(dimq->sock), dimq_found);
			if(dimq_found){
				HASH_DELETE(hh_sock, db.contexts_by_sock, dimq_found);
			}
#endif
			rc = COMPAT_CLOSE(dimq->sock);
			dimq->sock = INVALID_SOCKET;
		}
	}

#ifdef WITH_BROKER
	if(dimq->listener){
		dimq->listener->client_count--;
		dimq->listener = NULL;
	}
#endif

	return rc;
}


#ifdef FINAL_WITH_TLS_PSK
static unsigned int psk_client_callback(SSL *ssl, const char *hint,
		char *identity, unsigned int max_identity_len,
		unsigned char *psk, unsigned int max_psk_len)
{
	struct dimq *dimq;
	int len;

	UNUSED(hint);

	dimq = SSL_get_ex_data(ssl, tls_ex_index_dimq);
	if(!dimq) return 0;

	snprintf(identity, max_identity_len, "%s", dimq->tls_psk_identity);

	len = dimq__hex2bin(dimq->tls_psk, psk, (int)max_psk_len);
	if (len < 0) return 0;
	return (unsigned int)len;
}
#endif

#if defined(WITH_BROKER) && defined(__GLIBC__) && defined(WITH_ADNS)
/* Async connect, part 1 (dns lookup) */
int net__try_connect_step1(struct dimq *dimq, const char *host)
{
	int s;
	void *sevp = NULL;
	struct addrinfo *hints;

	if(dimq->adns){
		gai_cancel(dimq->adns);
		dimq__free((struct addrinfo *)dimq->adns->ar_request);
		dimq__free(dimq->adns);
	}
	dimq->adns = dimq__calloc(1, sizeof(struct gaicb));
	if(!dimq->adns){
		return dimq_ERR_NOMEM;
	}

	hints = dimq__calloc(1, sizeof(struct addrinfo));
	if(!hints){
		dimq__free(dimq->adns);
		dimq->adns = NULL;
		return dimq_ERR_NOMEM;
	}

	hints->ai_family = AF_UNSPEC;
	hints->ai_socktype = SOCK_STREAM;

	dimq->adns->ar_name = host;
	dimq->adns->ar_request = hints;

	s = getaddrinfo_a(GAI_NOWAIT, &dimq->adns, 1, sevp);
	if(s){
		errno = s;
		if(dimq->adns){
			dimq__free((struct addrinfo *)dimq->adns->ar_request);
			dimq__free(dimq->adns);
			dimq->adns = NULL;
		}
		return dimq_ERR_EAI;
	}

	return dimq_ERR_SUCCESS;
}

/* Async connect part 2, the connection. */
int net__try_connect_step2(struct dimq *dimq, uint16_t port, dimq_sock_t *sock)
{
	struct addrinfo *ainfo, *rp;
	int rc;

	ainfo = dimq->adns->ar_result;

	for(rp = ainfo; rp != NULL; rp = rp->ai_next){
		*sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(*sock == INVALID_SOCKET) continue;

		if(rp->ai_family == AF_INET){
			((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(port);
		}else if(rp->ai_family == AF_INET6){
			((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(port);
		}else{
			COMPAT_CLOSE(*sock);
			*sock = INVALID_SOCKET;
			continue;
		}

		/* Set non-blocking */
		if(net__socket_nonblock(sock)){
			continue;
		}

		rc = connect(*sock, rp->ai_addr, rp->ai_addrlen);
#ifdef WIN32
		errno = WSAGetLastError();
#endif
		if(rc == 0 || errno == EINPROGRESS || errno == COMPAT_EWOULDBLOCK){
			if(rc < 0 && (errno == EINPROGRESS || errno == COMPAT_EWOULDBLOCK)){
				rc = dimq_ERR_CONN_PENDING;
			}

			/* Set non-blocking */
			if(net__socket_nonblock(sock)){
				continue;
			}
			break;
		}

		COMPAT_CLOSE(*sock);
		*sock = INVALID_SOCKET;
	}
	freeaddrinfo(dimq->adns->ar_result);
	dimq->adns->ar_result = NULL;

	dimq__free((struct addrinfo *)dimq->adns->ar_request);
	dimq__free(dimq->adns);
	dimq->adns = NULL;

	if(!rp){
		return dimq_ERR_ERRNO;
	}

	return rc;
}

#endif


static int net__try_connect_tcp(const char *host, uint16_t port, dimq_sock_t *sock, const char *bind_address, bool blocking)
{
	struct addrinfo hints;
	struct addrinfo *ainfo, *rp;
	struct addrinfo *ainfo_bind, *rp_bind;
	int s;
	int rc = dimq_ERR_SUCCESS;

	ainfo_bind = NULL;

	*sock = INVALID_SOCKET;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	s = getaddrinfo(host, NULL, &hints, &ainfo);
	if(s){
		errno = s;
		return dimq_ERR_EAI;
	}

	if(bind_address){
		s = getaddrinfo(bind_address, NULL, &hints, &ainfo_bind);
		if(s){
			freeaddrinfo(ainfo);
			errno = s;
			return dimq_ERR_EAI;
		}
	}

	for(rp = ainfo; rp != NULL; rp = rp->ai_next){
		*sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(*sock == INVALID_SOCKET) continue;

		if(rp->ai_family == AF_INET){
			((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(port);
		}else if(rp->ai_family == AF_INET6){
			((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(port);
		}else{
			COMPAT_CLOSE(*sock);
			*sock = INVALID_SOCKET;
			continue;
		}

		if(bind_address){
			for(rp_bind = ainfo_bind; rp_bind != NULL; rp_bind = rp_bind->ai_next){
				if(bind(*sock, rp_bind->ai_addr, rp_bind->ai_addrlen) == 0){
					break;
				}
			}
			if(!rp_bind){
				COMPAT_CLOSE(*sock);
				*sock = INVALID_SOCKET;
				continue;
			}
		}

		if(!blocking){
			/* Set non-blocking */
			if(net__socket_nonblock(sock)){
				continue;
			}
		}

		rc = connect(*sock, rp->ai_addr, rp->ai_addrlen);
#ifdef WIN32
		errno = WSAGetLastError();
#endif
		if(rc == 0 || errno == EINPROGRESS || errno == COMPAT_EWOULDBLOCK){
			if(rc < 0 && (errno == EINPROGRESS || errno == COMPAT_EWOULDBLOCK)){
				rc = dimq_ERR_CONN_PENDING;
			}

			if(blocking){
				/* Set non-blocking */
				if(net__socket_nonblock(sock)){
					continue;
				}
			}
			break;
		}

		COMPAT_CLOSE(*sock);
		*sock = INVALID_SOCKET;
	}
	freeaddrinfo(ainfo);
	if(bind_address){
		freeaddrinfo(ainfo_bind);
	}
	if(!rp){
		return dimq_ERR_ERRNO;
	}
	return rc;
}


#ifdef WITH_UNIX_SOCKETS
static int net__try_connect_unix(const char *host, dimq_sock_t *sock)
{
	struct sockaddr_un addr;
	int s;
	int rc;

	if(host == NULL || strlen(host) == 0 || strlen(host) > sizeof(addr.sun_path)-1){
		return dimq_ERR_INVAL;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, host, sizeof(addr.sun_path)-1);

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s < 0){
		return dimq_ERR_ERRNO;
	}
	rc = net__socket_nonblock(&s);
	if(rc) return rc;

	rc = connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
	if(rc < 0){
		close(s);
		return dimq_ERR_ERRNO;
	}

	*sock = s;

	return 0;
}
#endif


int net__try_connect(const char *host, uint16_t port, dimq_sock_t *sock, const char *bind_address, bool blocking)
{
	if(port == 0){
#ifdef WITH_UNIX_SOCKETS
		return net__try_connect_unix(host, sock);
#else
		return dimq_ERR_NOT_SUPPORTED;
#endif
	}else{
		return net__try_connect_tcp(host, port, sock, bind_address, blocking);
	}
}


#ifdef WITH_TLS
void net__print_ssl_error(struct dimq *dimq)
{
	char ebuf[256];
	unsigned long e;
	int num = 0;

	e = ERR_get_error();
	while(e){
		log__printf(dimq, dimq_LOG_ERR, "OpenSSL Error[%d]: %s", num, ERR_error_string(e, ebuf));
		e = ERR_get_error();
		num++;
	}
}


int net__socket_connect_tls(struct dimq *dimq)
{
	int ret, err;
	long res;

	ERR_clear_error();
	if (dimq->tls_ocsp_required) {
		/* Note: OCSP is available in all currently supported OpenSSL versions. */
		if ((res=SSL_set_tlsext_status_type(dimq->ssl, TLSEXT_STATUSTYPE_ocsp)) != 1) {
			log__printf(dimq, dimq_LOG_ERR, "Could not activate OCSP (error: %ld)", res);
			return dimq_ERR_OCSP;
		}
		if ((res=SSL_CTX_set_tlsext_status_cb(dimq->ssl_ctx, dimq__verify_ocsp_status_cb)) != 1) {
			log__printf(dimq, dimq_LOG_ERR, "Could not activate OCSP (error: %ld)", res);
			return dimq_ERR_OCSP;
		}
		if ((res=SSL_CTX_set_tlsext_status_arg(dimq->ssl_ctx, dimq)) != 1) {
			log__printf(dimq, dimq_LOG_ERR, "Could not activate OCSP (error: %ld)", res);
			return dimq_ERR_OCSP;
		}
	}

	ret = SSL_connect(dimq->ssl);
	if(ret != 1) {
		err = SSL_get_error(dimq->ssl, ret);
		if (err == SSL_ERROR_SYSCALL) {
			dimq->want_connect = true;
			return dimq_ERR_SUCCESS;
		}
		if(err == SSL_ERROR_WANT_READ){
			dimq->want_connect = true;
			/* We always try to read anyway */
		}else if(err == SSL_ERROR_WANT_WRITE){
			dimq->want_write = true;
			dimq->want_connect = true;
		}else{
			net__print_ssl_error(dimq);

			COMPAT_CLOSE(dimq->sock);
			dimq->sock = INVALID_SOCKET;
			net__print_ssl_error(dimq);
			return dimq_ERR_TLS;
		}
	}else{
		dimq->want_connect = false;
	}
	return dimq_ERR_SUCCESS;
}
#endif


#ifdef WITH_TLS
static int net__tls_load_ca(struct dimq *dimq)
{
	int ret;

	if(dimq->tls_use_os_certs){
		SSL_CTX_set_default_verify_paths(dimq->ssl_ctx);
	}
#if OPENSSL_VERSION_NUMBER < 0x30000000L
	if(dimq->tls_cafile || dimq->tls_capath){
		ret = SSL_CTX_load_verify_locations(dimq->ssl_ctx, dimq->tls_cafile, dimq->tls_capath);
		if(ret == 0){
#  ifdef WITH_BROKER
			if(dimq->tls_cafile && dimq->tls_capath){
				log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check bridge_cafile \"%s\" and bridge_capath \"%s\".", dimq->tls_cafile, dimq->tls_capath);
			}else if(dimq->tls_cafile){
				log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check bridge_cafile \"%s\".", dimq->tls_cafile);
			}else{
				log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check bridge_capath \"%s\".", dimq->tls_capath);
			}
#  else
			if(dimq->tls_cafile && dimq->tls_capath){
				log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check cafile \"%s\" and capath \"%s\".", dimq->tls_cafile, dimq->tls_capath);
			}else if(dimq->tls_cafile){
				log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check cafile \"%s\".", dimq->tls_cafile);
			}else{
				log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check capath \"%s\".", dimq->tls_capath);
			}
#  endif
			return dimq_ERR_TLS;
		}
	}
#else
	if(dimq->tls_cafile){
		ret = SSL_CTX_load_verify_file(dimq->ssl_ctx, dimq->tls_cafile);
		if(ret == 0){
#  ifdef WITH_BROKER
			log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check bridge_cafile \"%s\".", dimq->tls_cafile);
#  else
			log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check cafile \"%s\".", dimq->tls_cafile);
#  endif
			return dimq_ERR_TLS;
		}
	}
	if(dimq->tls_capath){
		ret = SSL_CTX_load_verify_dir(dimq->ssl_ctx, dimq->tls_capath);
		if(ret == 0){
#  ifdef WITH_BROKER
			log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check bridge_capath \"%s\".", dimq->tls_capath);
#  else
			log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load CA certificates, check capath \"%s\".", dimq->tls_capath);
#  endif
			return dimq_ERR_TLS;
		}
	}
#endif
	return dimq_ERR_SUCCESS;
}


static int net__init_ssl_ctx(struct dimq *dimq)
{
	int ret;
	ENGINE *engine = NULL;
	uint8_t tls_alpn_wire[256];
	uint8_t tls_alpn_len;
#if !defined(OPENSSL_NO_ENGINE)
	EVP_PKEY *pkey;
#endif
 
#ifndef WITH_BROKER
	if(dimq->user_ssl_ctx){
		dimq->ssl_ctx = dimq->user_ssl_ctx;
		if(!dimq->ssl_ctx_defaults){
			return dimq_ERR_SUCCESS;
		}else if(!dimq->tls_cafile && !dimq->tls_capath && !dimq->tls_psk){
			log__printf(dimq, dimq_LOG_ERR, "Error: If you use dimq_OPT_SSL_CTX then dimq_OPT_SSL_CTX_WITH_DEFAULTS must be true, or at least one of cafile, capath or psk must be specified.");
			return dimq_ERR_INVAL;
		}
	}
#endif

	/* Apply default SSL_CTX settings. This is only used if dimq_OPT_SSL_CTX
	 * has not been set, or if both of dimq_OPT_SSL_CTX and
	 * dimq_OPT_SSL_CTX_WITH_DEFAULTS are set. */
	if(dimq->tls_cafile || dimq->tls_capath || dimq->tls_psk || dimq->tls_use_os_certs){
		if(!dimq->ssl_ctx){
			net__init_tls();

#if OPENSSL_VERSION_NUMBER < 0x10100000L
			dimq->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
#else
			dimq->ssl_ctx = SSL_CTX_new(TLS_client_method());
#endif

			if(!dimq->ssl_ctx){
				log__printf(dimq, dimq_LOG_ERR, "Error: Unable to create TLS context.");
				net__print_ssl_error(dimq);
				return dimq_ERR_TLS;
			}
		}

#ifdef SSL_OP_NO_TLSv1_3
		if(dimq->tls_psk){
			SSL_CTX_set_options(dimq->ssl_ctx, SSL_OP_NO_TLSv1_3);
		}
#endif

		if(!dimq->tls_version){
			SSL_CTX_set_options(dimq->ssl_ctx, SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1);
#ifdef SSL_OP_NO_TLSv1_3
		}else if(!strcmp(dimq->tls_version, "tlsv1.3")){
			SSL_CTX_set_options(dimq->ssl_ctx, SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2);
#endif
		}else if(!strcmp(dimq->tls_version, "tlsv1.2")){
			SSL_CTX_set_options(dimq->ssl_ctx, SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
		}else if(!strcmp(dimq->tls_version, "tlsv1.1")){
			SSL_CTX_set_options(dimq->ssl_ctx, SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1);
		}else{
			log__printf(dimq, dimq_LOG_ERR, "Error: Protocol %s not supported.", dimq->tls_version);
			return dimq_ERR_INVAL;
		}

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
		/* Allow use of DHE ciphers */
		SSL_CTX_set_dh_auto(dimq->ssl_ctx, 1);
#endif
		/* Disable compression */
		SSL_CTX_set_options(dimq->ssl_ctx, SSL_OP_NO_COMPRESSION);

		/* Set ALPN */
		if(dimq->tls_alpn) {
			tls_alpn_len = (uint8_t) strnlen(dimq->tls_alpn, 254);
			tls_alpn_wire[0] = tls_alpn_len;  /* first byte is length of string */
			memcpy(tls_alpn_wire + 1, dimq->tls_alpn, tls_alpn_len);
			SSL_CTX_set_alpn_protos(dimq->ssl_ctx, tls_alpn_wire, tls_alpn_len + 1U);
		}

#ifdef SSL_MODE_RELEASE_BUFFERS
			/* Use even less memory per SSL connection. */
			SSL_CTX_set_mode(dimq->ssl_ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

#if !defined(OPENSSL_NO_ENGINE)
		if(dimq->tls_engine){
			engine = ENGINE_by_id(dimq->tls_engine);
			if(!engine){
				log__printf(dimq, dimq_LOG_ERR, "Error loading %s engine\n", dimq->tls_engine);
				return dimq_ERR_TLS;
			}
			if(!ENGINE_init(engine)){
				log__printf(dimq, dimq_LOG_ERR, "Failed engine initialisation\n");
				ENGINE_free(engine);
				return dimq_ERR_TLS;
			}
			ENGINE_set_default(engine, ENGINE_METHOD_ALL);
			ENGINE_free(engine); /* release the structural reference from ENGINE_by_id() */
		}
#endif

		if(dimq->tls_ciphers){
			ret = SSL_CTX_set_cipher_list(dimq->ssl_ctx, dimq->tls_ciphers);
			if(ret == 0){
				log__printf(dimq, dimq_LOG_ERR, "Error: Unable to set TLS ciphers. Check cipher list \"%s\".", dimq->tls_ciphers);
#if !defined(OPENSSL_NO_ENGINE)
				ENGINE_FINISH(engine);
#endif
				net__print_ssl_error(dimq);
				return dimq_ERR_TLS;
			}
		}
		if(dimq->tls_cafile || dimq->tls_capath || dimq->tls_use_os_certs){
			ret = net__tls_load_ca(dimq);
			if(ret != dimq_ERR_SUCCESS){
#  if !defined(OPENSSL_NO_ENGINE)
				ENGINE_FINISH(engine);
#  endif
				net__print_ssl_error(dimq);
				return dimq_ERR_TLS;
			}
			if(dimq->tls_cert_reqs == 0){
				SSL_CTX_set_verify(dimq->ssl_ctx, SSL_VERIFY_NONE, NULL);
			}else{
				SSL_CTX_set_verify(dimq->ssl_ctx, SSL_VERIFY_PEER, dimq__server_certificate_verify);
			}

			if(dimq->tls_pw_callback){
				SSL_CTX_set_default_passwd_cb(dimq->ssl_ctx, dimq->tls_pw_callback);
				SSL_CTX_set_default_passwd_cb_userdata(dimq->ssl_ctx, dimq);
			}

			if(dimq->tls_certfile){
				ret = SSL_CTX_use_certificate_chain_file(dimq->ssl_ctx, dimq->tls_certfile);
				if(ret != 1){
#ifdef WITH_BROKER
					log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load client certificate, check bridge_certfile \"%s\".", dimq->tls_certfile);
#else
					log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load client certificate \"%s\".", dimq->tls_certfile);
#endif
#if !defined(OPENSSL_NO_ENGINE)
					ENGINE_FINISH(engine);
#endif
					net__print_ssl_error(dimq);
					return dimq_ERR_TLS;
				}
			}
			if(dimq->tls_keyfile){
				if(dimq->tls_keyform == dimq_k_engine){
#if !defined(OPENSSL_NO_ENGINE)
					UI_METHOD *ui_method = net__get_ui_method();
					if(dimq->tls_engine_kpass_sha1){
						if(!ENGINE_ctrl_cmd(engine, ENGINE_SECRET_MODE, ENGINE_SECRET_MODE_SHA, NULL, NULL, 0)){
							log__printf(dimq, dimq_LOG_ERR, "Error: Unable to set engine secret mode sha1");
							ENGINE_FINISH(engine);
							net__print_ssl_error(dimq);
							return dimq_ERR_TLS;
						}
						if(!ENGINE_ctrl_cmd(engine, ENGINE_PIN, 0, dimq->tls_engine_kpass_sha1, NULL, 0)){
							log__printf(dimq, dimq_LOG_ERR, "Error: Unable to set engine pin");
							ENGINE_FINISH(engine);
							net__print_ssl_error(dimq);
							return dimq_ERR_TLS;
						}
						ui_method = NULL;
					}
					pkey = ENGINE_load_private_key(engine, dimq->tls_keyfile, ui_method, NULL);
					if(!pkey){
						log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load engine private key file \"%s\".", dimq->tls_keyfile);
						ENGINE_FINISH(engine);
						net__print_ssl_error(dimq);
						return dimq_ERR_TLS;
					}
					if(SSL_CTX_use_PrivateKey(dimq->ssl_ctx, pkey) <= 0){
						log__printf(dimq, dimq_LOG_ERR, "Error: Unable to use engine private key file \"%s\".", dimq->tls_keyfile);
						ENGINE_FINISH(engine);
						net__print_ssl_error(dimq);
						return dimq_ERR_TLS;
					}
#endif
				}else{
					ret = SSL_CTX_use_PrivateKey_file(dimq->ssl_ctx, dimq->tls_keyfile, SSL_FILETYPE_PEM);
					if(ret != 1){
#ifdef WITH_BROKER
						log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load client key file, check bridge_keyfile \"%s\".", dimq->tls_keyfile);
#else
						log__printf(dimq, dimq_LOG_ERR, "Error: Unable to load client key file \"%s\".", dimq->tls_keyfile);
#endif
#if !defined(OPENSSL_NO_ENGINE)
						ENGINE_FINISH(engine);
#endif
						net__print_ssl_error(dimq);
						return dimq_ERR_TLS;
					}
				}
				ret = SSL_CTX_check_private_key(dimq->ssl_ctx);
				if(ret != 1){
					log__printf(dimq, dimq_LOG_ERR, "Error: Client certificate/key are inconsistent.");
#if !defined(OPENSSL_NO_ENGINE)
					ENGINE_FINISH(engine);
#endif
					net__print_ssl_error(dimq);
					return dimq_ERR_TLS;
				}
			}
#ifdef FINAL_WITH_TLS_PSK
		}else if(dimq->tls_psk){
			SSL_CTX_set_psk_client_callback(dimq->ssl_ctx, psk_client_callback);
			if(dimq->tls_ciphers == NULL){
				SSL_CTX_set_cipher_list(dimq->ssl_ctx, "PSK");
			}
#endif
		}
	}

	return dimq_ERR_SUCCESS;
}
#endif


int net__socket_connect_step3(struct dimq *dimq, const char *host)
{
#ifdef WITH_TLS
	BIO *bio;

	int rc = net__init_ssl_ctx(dimq);
	if(rc){
		net__socket_close(dimq);
		return rc;
	}

	if(dimq->ssl_ctx){
		if(dimq->ssl){
			SSL_free(dimq->ssl);
		}
		dimq->ssl = SSL_new(dimq->ssl_ctx);
		if(!dimq->ssl){
			net__socket_close(dimq);
			net__print_ssl_error(dimq);
			return dimq_ERR_TLS;
		}

		SSL_set_ex_data(dimq->ssl, tls_ex_index_dimq, dimq);
		bio = BIO_new_socket(dimq->sock, BIO_NOCLOSE);
		if(!bio){
			net__socket_close(dimq);
			net__print_ssl_error(dimq);
			return dimq_ERR_TLS;
		}
		SSL_set_bio(dimq->ssl, bio, bio);

		/*
		 * required for the SNI resolving
		 */
		if(SSL_set_tlsext_host_name(dimq->ssl, host) != 1) {
			net__socket_close(dimq);
			return dimq_ERR_TLS;
		}

		if(net__socket_connect_tls(dimq)){
			net__socket_close(dimq);
			return dimq_ERR_TLS;
		}

	}
#else
	UNUSED(dimq);
	UNUSED(host);
#endif
	return dimq_ERR_SUCCESS;
}

/* Create a socket and connect it to 'ip' on port 'port'.  */
int net__socket_connect(struct dimq *dimq, const char *host, uint16_t port, const char *bind_address, bool blocking)
{
	int rc, rc2;

	if(!dimq || !host) return dimq_ERR_INVAL;

	rc = net__try_connect(host, port, &dimq->sock, bind_address, blocking);
	if(rc > 0) return rc;

	if(dimq->tcp_nodelay){
		int flag = 1;
		if(setsockopt(dimq->sock, IPPROTO_TCP, TCP_NODELAY, (const void*)&flag, sizeof(int)) != 0){
			log__printf(dimq, dimq_LOG_WARNING, "Warning: Unable to set TCP_NODELAY.");
		}
	}

#if defined(WITH_SOCKS) && !defined(WITH_BROKER)
	if(!dimq->socks5_host)
#endif
	{
		rc2 = net__socket_connect_step3(dimq, host);
		if(rc2) return rc2;
	}

	return rc;
}


#ifdef WITH_TLS
static int net__handle_ssl(struct dimq* dimq, int ret)
{
	int err;

	err = SSL_get_error(dimq->ssl, ret);
	if (err == SSL_ERROR_WANT_READ) {
		ret = -1;
		errno = EAGAIN;
	}
	else if (err == SSL_ERROR_WANT_WRITE) {
		ret = -1;
#ifdef WITH_BROKER
		mux__add_out(dimq);
#else
		dimq->want_write = true;
#endif
		errno = EAGAIN;
	}
	else {
		net__print_ssl_error(dimq);
		errno = EPROTO;
	}
	ERR_clear_error();
#ifdef WIN32
	WSASetLastError(errno);
#endif

	return ret;
}
#endif

ssize_t net__read(struct dimq *dimq, void *buf, size_t count)
{
#ifdef WITH_TLS
	int ret;
#endif
	assert(dimq);
	errno = 0;
#ifdef WITH_TLS
	if(dimq->ssl){
		ret = SSL_read(dimq->ssl, buf, (int)count);
		if(ret <= 0){
			ret = net__handle_ssl(dimq, ret);
		}
		return (ssize_t )ret;
	}else{
		/* Call normal read/recv */

#endif

#ifndef WIN32
	return read(dimq->sock, buf, count);
#else
	return recv(dimq->sock, buf, count, 0);
#endif

#ifdef WITH_TLS
	}
#endif
}

ssize_t net__write(struct dimq *dimq, const void *buf, size_t count)
{
#ifdef WITH_TLS
	int ret;
#endif
	assert(dimq);

	errno = 0;
#ifdef WITH_TLS
	if(dimq->ssl){
		dimq->want_write = false;
		ret = SSL_write(dimq->ssl, buf, (int)count);
		if(ret < 0){
			ret = net__handle_ssl(dimq, ret);
		}
		return (ssize_t )ret;
	}else{
		/* Call normal write/send */
#endif

#ifndef WIN32
	return write(dimq->sock, buf, count);
#else
	return send(dimq->sock, buf, count, 0);
#endif

#ifdef WITH_TLS
	}
#endif
}


int net__socket_nonblock(dimq_sock_t *sock)
{
#ifndef WIN32
	int opt;
	/* Set non-blocking */
	opt = fcntl(*sock, F_GETFL, 0);
	if(opt == -1){
		COMPAT_CLOSE(*sock);
		*sock = INVALID_SOCKET;
		return dimq_ERR_ERRNO;
	}
	if(fcntl(*sock, F_SETFL, opt | O_NONBLOCK) == -1){
		/* If either fcntl fails, don't want to allow this client to connect. */
		COMPAT_CLOSE(*sock);
		*sock = INVALID_SOCKET;
		return dimq_ERR_ERRNO;
	}
#else
	unsigned long opt = 1;
	if(ioctlsocket(*sock, FIONBIO, &opt)){
		COMPAT_CLOSE(*sock);
		*sock = INVALID_SOCKET;
		return dimq_ERR_ERRNO;
	}
#endif
	return dimq_ERR_SUCCESS;
}


#ifndef WITH_BROKER
int net__socketpair(dimq_sock_t *pairR, dimq_sock_t *pairW)
{
#ifdef WIN32
	int family[2] = {AF_INET, AF_INET6};
	int i;
	struct sockaddr_storage ss;
	struct sockaddr_in *sa = (struct sockaddr_in *)&ss;
	struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&ss;
	socklen_t ss_len;
	dimq_sock_t spR, spW;

	dimq_sock_t listensock;

	*pairR = INVALID_SOCKET;
	*pairW = INVALID_SOCKET;

	for(i=0; i<2; i++){
		memset(&ss, 0, sizeof(ss));
		if(family[i] == AF_INET){
			sa->sin_family = family[i];
			sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			sa->sin_port = 0;
			ss_len = sizeof(struct sockaddr_in);
		}else if(family[i] == AF_INET6){
			sa6->sin6_family = family[i];
			sa6->sin6_addr = in6addr_loopback;
			sa6->sin6_port = 0;
			ss_len = sizeof(struct sockaddr_in6);
		}else{
			return dimq_ERR_INVAL;
		}

		listensock = socket(family[i], SOCK_STREAM, IPPROTO_TCP);
		if(listensock == -1){
			continue;
		}

		if(bind(listensock, (struct sockaddr *)&ss, ss_len) == -1){
			COMPAT_CLOSE(listensock);
			continue;
		}

		if(listen(listensock, 1) == -1){
			COMPAT_CLOSE(listensock);
			continue;
		}
		memset(&ss, 0, sizeof(ss));
		ss_len = sizeof(ss);
		if(getsockname(listensock, (struct sockaddr *)&ss, &ss_len) < 0){
			COMPAT_CLOSE(listensock);
			continue;
		}

		if(family[i] == AF_INET){
			sa->sin_family = family[i];
			sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			ss_len = sizeof(struct sockaddr_in);
		}else if(family[i] == AF_INET6){
			sa6->sin6_family = family[i];
			sa6->sin6_addr = in6addr_loopback;
			ss_len = sizeof(struct sockaddr_in6);
		}

		spR = socket(family[i], SOCK_STREAM, IPPROTO_TCP);
		if(spR == -1){
			COMPAT_CLOSE(listensock);
			continue;
		}
		if(net__socket_nonblock(&spR)){
			COMPAT_CLOSE(listensock);
			continue;
		}
		if(connect(spR, (struct sockaddr *)&ss, ss_len) < 0){
#ifdef WIN32
			errno = WSAGetLastError();
#endif
			if(errno != EINPROGRESS && errno != COMPAT_EWOULDBLOCK){
				COMPAT_CLOSE(spR);
				COMPAT_CLOSE(listensock);
				continue;
			}
		}
		spW = accept(listensock, NULL, 0);
		if(spW == -1){
#ifdef WIN32
			errno = WSAGetLastError();
#endif
			if(errno != EINPROGRESS && errno != COMPAT_EWOULDBLOCK){
				COMPAT_CLOSE(spR);
				COMPAT_CLOSE(listensock);
				continue;
			}
		}

		if(net__socket_nonblock(&spW)){
			COMPAT_CLOSE(spR);
			COMPAT_CLOSE(listensock);
			continue;
		}
		COMPAT_CLOSE(listensock);

		*pairR = spR;
		*pairW = spW;
		return dimq_ERR_SUCCESS;
	}
	return dimq_ERR_UNKNOWN;
#else
	int sv[2];

	*pairR = INVALID_SOCKET;
	*pairW = INVALID_SOCKET;

	if(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1){
		return dimq_ERR_ERRNO;
	}
	if(net__socket_nonblock(&sv[0])){
		COMPAT_CLOSE(sv[1]);
		return dimq_ERR_ERRNO;
	}
	if(net__socket_nonblock(&sv[1])){
		COMPAT_CLOSE(sv[0]);
		return dimq_ERR_ERRNO;
	}
	*pairR = sv[0];
	*pairW = sv[1];
	return dimq_ERR_SUCCESS;
#endif
}
#endif

#ifndef WITH_BROKER
void *dimq_ssl_get(struct dimq *dimq)
{
#ifdef WITH_TLS
	return dimq->ssl;
#else
	UNUSED(dimq);

	return NULL;
#endif
}
#endif
