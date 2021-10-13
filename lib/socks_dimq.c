/*
Copyright (c) 2014-2020 Roger Light <roger@atchoo.org>

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

#include <errno.h>
#include <string.h>
#include <limits.h>
#ifdef WIN32
#  include <ws2tcpip.h>
#elif defined(__QNX__)
#  include <sys/socket.h>
#  include <arpa/inet.h>
#  include <netinet/in.h>
#else
#  include <arpa/inet.h>
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__)
#  include <sys/socket.h>
#  include <netinet/in.h>
#endif

#include "dimq_internal.h"
#include "memory_dimq.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "send_dimq.h"
#include "socks_dimq.h"
#include "util_dimq.h"

#define SOCKS_AUTH_NONE 0x00U
#define SOCKS_AUTH_GSS 0x01U
#define SOCKS_AUTH_USERPASS 0x02U
#define SOCKS_AUTH_NO_ACCEPTABLE 0xFFU

#define SOCKS_ATYPE_IP_V4 1U /* four bytes */
#define SOCKS_ATYPE_DOMAINNAME 3U /* one byte length, followed by fqdn no null, 256 max chars */
#define SOCKS_ATYPE_IP_V6 4U /* 16 bytes */

#define SOCKS_REPLY_SUCCEEDED 0x00U
#define SOCKS_REPLY_GENERAL_FAILURE 0x01U
#define SOCKS_REPLY_CONNECTION_NOT_ALLOWED 0x02U
#define SOCKS_REPLY_NETWORK_UNREACHABLE 0x03U
#define SOCKS_REPLY_HOST_UNREACHABLE 0x04U
#define SOCKS_REPLY_CONNECTION_REFUSED 0x05U
#define SOCKS_REPLY_TTL_EXPIRED 0x06U
#define SOCKS_REPLY_COMMAND_NOT_SUPPORTED 0x07U
#define SOCKS_REPLY_ADDRESS_TYPE_NOT_SUPPORTED 0x08U

int dimq_socks5_set(struct dimq *dimq, const char *host, int port, const char *username, const char *password)
{
#ifdef WITH_SOCKS
	if(!dimq) return dimq_ERR_INVAL;
	if(!host || strlen(host) > 256) return dimq_ERR_INVAL;
	if(port < 1 || port > UINT16_MAX) return dimq_ERR_INVAL;

	dimq__free(dimq->socks5_host);
	dimq->socks5_host = NULL;

	dimq->socks5_host = dimq__strdup(host);
	if(!dimq->socks5_host){
		return dimq_ERR_NOMEM;
	}

	dimq->socks5_port = (uint16_t)port;

	dimq__free(dimq->socks5_username);
	dimq->socks5_username = NULL;

	dimq__free(dimq->socks5_password);
	dimq->socks5_password = NULL;

	if(username){
		if(strlen(username) > UINT8_MAX){
			return dimq_ERR_INVAL;
		}
		dimq->socks5_username = dimq__strdup(username);
		if(!dimq->socks5_username){
			return dimq_ERR_NOMEM;
		}

		if(password){
			if(strlen(password) > UINT8_MAX){
				return dimq_ERR_INVAL;
			}
			dimq->socks5_password = dimq__strdup(password);
			if(!dimq->socks5_password){
				dimq__free(dimq->socks5_username);
				return dimq_ERR_NOMEM;
			}
		}
	}

	return dimq_ERR_SUCCESS;
#else
	UNUSED(dimq);
	UNUSED(host);
	UNUSED(port);
	UNUSED(username);
	UNUSED(password);

	return dimq_ERR_NOT_SUPPORTED;
#endif
}

#ifdef WITH_SOCKS
int socks5__send(struct dimq *dimq)
{
	struct dimq__packet *packet;
	size_t slen;
	uint8_t ulen, plen;

	struct in_addr addr_ipv4;
	struct in6_addr addr_ipv6;
	int ipv4_pton_result;
	int ipv6_pton_result;
	enum dimq_client_state state;

	state = dimq__get_state(dimq);

	if(state == dimq_cs_socks5_new){
		packet = dimq__calloc(1, sizeof(struct dimq__packet));
		if(!packet) return dimq_ERR_NOMEM;

		if(dimq->socks5_username){
			packet->packet_length = 4;
		}else{
			packet->packet_length = 3;
		}
		packet->payload = dimq__malloc(sizeof(uint8_t)*packet->packet_length);

		packet->payload[0] = 0x05;
		if(dimq->socks5_username){
			packet->payload[1] = 2;
			packet->payload[2] = SOCKS_AUTH_NONE;
			packet->payload[3] = SOCKS_AUTH_USERPASS;
		}else{
			packet->payload[1] = 1;
			packet->payload[2] = SOCKS_AUTH_NONE;
		}

		dimq__set_state(dimq, dimq_cs_socks5_start);

		dimq->in_packet.pos = 0;
		dimq->in_packet.packet_length = 2;
		dimq->in_packet.to_process = 2;
		dimq->in_packet.payload = dimq__malloc(sizeof(uint8_t)*2);
		if(!dimq->in_packet.payload){
			dimq__free(packet->payload);
			dimq__free(packet);
			return dimq_ERR_NOMEM;
		}

		return packet__queue(dimq, packet);
	}else if(state == dimq_cs_socks5_auth_ok){
		packet = dimq__calloc(1, sizeof(struct dimq__packet));
		if(!packet) return dimq_ERR_NOMEM;

		ipv4_pton_result = inet_pton(AF_INET, dimq->host, &addr_ipv4);
		ipv6_pton_result = inet_pton(AF_INET6, dimq->host, &addr_ipv6);

		if(ipv4_pton_result == 1){
			packet->packet_length = 10;
			packet->payload = dimq__malloc(sizeof(uint8_t)*packet->packet_length);
			if(!packet->payload){
				dimq__free(packet);
				return dimq_ERR_NOMEM;
			}
			packet->payload[3] = SOCKS_ATYPE_IP_V4;
			memcpy(&(packet->payload[4]), (const void*)&addr_ipv4, 4);
			packet->payload[4+4] = dimq_MSB(dimq->port);
			packet->payload[4+4+1] = dimq_LSB(dimq->port);

		}else if(ipv6_pton_result == 1){
			packet->packet_length = 22;
			packet->payload = dimq__malloc(sizeof(uint8_t)*packet->packet_length);
			if(!packet->payload){
				dimq__free(packet);
				return dimq_ERR_NOMEM;
			}
			packet->payload[3] = SOCKS_ATYPE_IP_V6;
			memcpy(&(packet->payload[4]), (const void*)&addr_ipv6, 16);
			packet->payload[4+16] = dimq_MSB(dimq->port);
			packet->payload[4+16+1] = dimq_LSB(dimq->port);

		}else{
			slen = strlen(dimq->host);
			if(slen > UCHAR_MAX){
				dimq__free(packet);
				return dimq_ERR_NOMEM;
			}
			packet->packet_length = 7U + (uint32_t)slen;
			packet->payload = dimq__malloc(sizeof(uint8_t)*packet->packet_length);
			if(!packet->payload){
				dimq__free(packet);
				return dimq_ERR_NOMEM;
			}
			packet->payload[3] = SOCKS_ATYPE_DOMAINNAME;
			packet->payload[4] = (uint8_t)slen;
			memcpy(&(packet->payload[5]), dimq->host, slen);
			packet->payload[5+slen] = dimq_MSB(dimq->port);
			packet->payload[6+slen] = dimq_LSB(dimq->port);
		}
		packet->payload[0] = 0x05;
		packet->payload[1] = 0x01;
		packet->payload[2] = 0x00;

		dimq__set_state(dimq, dimq_cs_socks5_request);

		dimq->in_packet.pos = 0;
		dimq->in_packet.packet_length = 5;
		dimq->in_packet.to_process = 5;
		dimq->in_packet.payload = dimq__malloc(sizeof(uint8_t)*5);
		if(!dimq->in_packet.payload){
			dimq__free(packet->payload);
			dimq__free(packet);
			return dimq_ERR_NOMEM;
		}

		return packet__queue(dimq, packet);
	}else if(state == dimq_cs_socks5_send_userpass){
		packet = dimq__calloc(1, sizeof(struct dimq__packet));
		if(!packet) return dimq_ERR_NOMEM;

		ulen = (uint8_t)strlen(dimq->socks5_username);
		plen = (uint8_t)strlen(dimq->socks5_password);
		packet->packet_length = 3U + ulen + plen;
		packet->payload = dimq__malloc(sizeof(uint8_t)*packet->packet_length);


		packet->payload[0] = 0x01;
		packet->payload[1] = ulen;
		memcpy(&(packet->payload[2]), dimq->socks5_username, ulen);
		packet->payload[2+ulen] = plen;
		memcpy(&(packet->payload[3+ulen]), dimq->socks5_password, plen);

		dimq__set_state(dimq, dimq_cs_socks5_userpass_reply);

		dimq->in_packet.pos = 0;
		dimq->in_packet.packet_length = 2;
		dimq->in_packet.to_process = 2;
		dimq->in_packet.payload = dimq__malloc(sizeof(uint8_t)*2);
		if(!dimq->in_packet.payload){
			dimq__free(packet->payload);
			dimq__free(packet);
			return dimq_ERR_NOMEM;
		}

		return packet__queue(dimq, packet);
	}
	return dimq_ERR_SUCCESS;
}

int socks5__read(struct dimq *dimq)
{
	ssize_t len;
	uint8_t *payload;
	uint8_t i;
	enum dimq_client_state state;

	state = dimq__get_state(dimq);
	if(state == dimq_cs_socks5_start){
		while(dimq->in_packet.to_process > 0){
			len = net__read(dimq, &(dimq->in_packet.payload[dimq->in_packet.pos]), dimq->in_packet.to_process);
			if(len > 0){
				dimq->in_packet.pos += (uint32_t)len;
				dimq->in_packet.to_process -= (uint32_t)len;
			}else{
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
					return dimq_ERR_SUCCESS;
				}else{
					packet__cleanup(&dimq->in_packet);
					switch(errno){
						case 0:
							return dimq_ERR_PROXY;
						case COMPAT_ECONNRESET:
							return dimq_ERR_CONN_LOST;
						default:
							return dimq_ERR_ERRNO;
					}
				}
			}
		}
		if(dimq->in_packet.payload[0] != 5){
			packet__cleanup(&dimq->in_packet);
			return dimq_ERR_PROXY;
		}
		switch(dimq->in_packet.payload[1]){
			case SOCKS_AUTH_NONE:
				packet__cleanup(&dimq->in_packet);
				dimq__set_state(dimq, dimq_cs_socks5_auth_ok);
				return socks5__send(dimq);
			case SOCKS_AUTH_USERPASS:
				packet__cleanup(&dimq->in_packet);
				dimq__set_state(dimq, dimq_cs_socks5_send_userpass);
				return socks5__send(dimq);
			default:
				packet__cleanup(&dimq->in_packet);
				return dimq_ERR_AUTH;
		}
	}else if(state == dimq_cs_socks5_userpass_reply){
		while(dimq->in_packet.to_process > 0){
			len = net__read(dimq, &(dimq->in_packet.payload[dimq->in_packet.pos]), dimq->in_packet.to_process);
			if(len > 0){
				dimq->in_packet.pos += (uint32_t)len;
				dimq->in_packet.to_process -= (uint32_t)len;
			}else{
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
					return dimq_ERR_SUCCESS;
				}else{
					packet__cleanup(&dimq->in_packet);
					switch(errno){
						case 0:
							return dimq_ERR_PROXY;
						case COMPAT_ECONNRESET:
							return dimq_ERR_CONN_LOST;
						default:
							return dimq_ERR_ERRNO;
					}
				}
			}
		}
		if(dimq->in_packet.payload[0] != 1){
			packet__cleanup(&dimq->in_packet);
			return dimq_ERR_PROXY;
		}
		if(dimq->in_packet.payload[1] == 0){
			packet__cleanup(&dimq->in_packet);
			dimq__set_state(dimq, dimq_cs_socks5_auth_ok);
			return socks5__send(dimq);
		}else{
			i = dimq->in_packet.payload[1];
			packet__cleanup(&dimq->in_packet);
			switch(i){
				case SOCKS_REPLY_CONNECTION_NOT_ALLOWED:
					return dimq_ERR_AUTH;

				case SOCKS_REPLY_NETWORK_UNREACHABLE:
				case SOCKS_REPLY_HOST_UNREACHABLE:
				case SOCKS_REPLY_CONNECTION_REFUSED:
					return dimq_ERR_NO_CONN;

				case SOCKS_REPLY_GENERAL_FAILURE:
				case SOCKS_REPLY_TTL_EXPIRED:
				case SOCKS_REPLY_COMMAND_NOT_SUPPORTED:
				case SOCKS_REPLY_ADDRESS_TYPE_NOT_SUPPORTED:
					return dimq_ERR_PROXY;

				default:
					return dimq_ERR_INVAL;
			}
			return dimq_ERR_PROXY;
		}
	}else if(state == dimq_cs_socks5_request){
		while(dimq->in_packet.to_process > 0){
			len = net__read(dimq, &(dimq->in_packet.payload[dimq->in_packet.pos]), dimq->in_packet.to_process);
			if(len > 0){
				dimq->in_packet.pos += (uint32_t)len;
				dimq->in_packet.to_process -= (uint32_t)len;
			}else{
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
					return dimq_ERR_SUCCESS;
				}else{
					packet__cleanup(&dimq->in_packet);
					switch(errno){
						case 0:
							return dimq_ERR_PROXY;
						case COMPAT_ECONNRESET:
							return dimq_ERR_CONN_LOST;
						default:
							return dimq_ERR_ERRNO;
					}
				}
			}
		}

		if(dimq->in_packet.packet_length == 5){
			/* First part of the packet has been received, we now know what else to expect. */
			if(dimq->in_packet.payload[3] == SOCKS_ATYPE_IP_V4){
				dimq->in_packet.to_process += 4+2-1; /* 4 bytes IPv4, 2 bytes port, -1 byte because we've already read the first byte */
				dimq->in_packet.packet_length += 4+2-1;
			}else if(dimq->in_packet.payload[3] == SOCKS_ATYPE_IP_V6){
				dimq->in_packet.to_process += 16+2-1; /* 16 bytes IPv6, 2 bytes port, -1 byte because we've already read the first byte */
				dimq->in_packet.packet_length += 16+2-1;
			}else if(dimq->in_packet.payload[3] == SOCKS_ATYPE_DOMAINNAME){
				if(dimq->in_packet.payload[4] > 0){
					dimq->in_packet.to_process += dimq->in_packet.payload[4];
					dimq->in_packet.packet_length += dimq->in_packet.payload[4];
				}
			}else{
				packet__cleanup(&dimq->in_packet);
				return dimq_ERR_PROTOCOL;
			}
			payload = dimq__realloc(dimq->in_packet.payload, dimq->in_packet.packet_length);
			if(payload){
				dimq->in_packet.payload = payload;
			}else{
				packet__cleanup(&dimq->in_packet);
				return dimq_ERR_NOMEM;
			}
			return dimq_ERR_SUCCESS;
		}

		/* Entire packet is now read. */
		if(dimq->in_packet.payload[0] != 5){
			packet__cleanup(&dimq->in_packet);
			return dimq_ERR_PROXY;
		}
		if(dimq->in_packet.payload[1] == 0){
			/* Auth passed */
			packet__cleanup(&dimq->in_packet);
			dimq__set_state(dimq, dimq_cs_new);
			if(dimq->socks5_host){
				int rc = net__socket_connect_step3(dimq, dimq->host);
				if(rc) return rc;
			}
			return send__connect(dimq, dimq->keepalive, dimq->clean_start, NULL);
		}else{
			i = dimq->in_packet.payload[1];
			packet__cleanup(&dimq->in_packet);
			dimq__set_state(dimq, dimq_cs_socks5_new);
			switch(i){
				case SOCKS_REPLY_CONNECTION_NOT_ALLOWED:
					return dimq_ERR_AUTH;

				case SOCKS_REPLY_NETWORK_UNREACHABLE:
				case SOCKS_REPLY_HOST_UNREACHABLE:
				case SOCKS_REPLY_CONNECTION_REFUSED:
					return dimq_ERR_NO_CONN;

				case SOCKS_REPLY_GENERAL_FAILURE:
				case SOCKS_REPLY_TTL_EXPIRED:
				case SOCKS_REPLY_COMMAND_NOT_SUPPORTED:
				case SOCKS_REPLY_ADDRESS_TYPE_NOT_SUPPORTED:
					return dimq_ERR_PROXY;

				default:
					return dimq_ERR_INVAL;
			}
		}
	}else{
		return packet__read(dimq);
	}
	return dimq_ERR_SUCCESS;
}
#endif
