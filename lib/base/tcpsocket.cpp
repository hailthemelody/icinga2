/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "base/tcpsocket.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <iostream>

using namespace icinga;

/**
 * Creates a socket and binds it to the specified service.
 *
 * @param service The service.
 * @param family The address family for the socket.
 */
void TcpSocket::Bind(const String& service, int family)
{
	Bind(String(), service, family);
}

/**
 * Creates a socket and binds it to the specified node and service.
 *
 * @param node The node.
 * @param service The service.
 * @param family The address family for the socket.
 */
void TcpSocket::Bind(const String& node, const String& service, int family)
{
	addrinfo hints;
	addrinfo *result;
	int error;
	const char *func;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int rc = getaddrinfo(node.IsEmpty() ? NULL : node.CStr(),
	    service.CStr(), &hints, &result);

	if (rc != 0) {
		Log(LogCritical, "TcpSocket")
		    << "getaddrinfo() failed with error code " << rc << ", \"" << gai_strerror(rc) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("getaddrinfo")
		    << errinfo_getaddrinfo_error(rc));
	}

	int fd = INVALID_SOCKET;

	for (addrinfo *info = result; info != NULL; info = info->ai_next) {
		fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		if (fd == INVALID_SOCKET) {
#ifdef _WIN32           
			error = WSAGetLastError();
#else /* _WIN32 */
			error = errno;
#endif /* _WIN32 */
			func = "socket";

			continue;
		}

		const int optFalse = 0;
		setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char *>(&optFalse), sizeof(optFalse));

#ifndef _WIN32
		const int optTrue = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&optTrue), sizeof(optTrue));
#endif /* _WIN32 */

		int rc = bind(fd, info->ai_addr, info->ai_addrlen);

		if (rc < 0) {
#ifdef _WIN32
			error = WSAGetLastError();
#else /* _WIN32 */
			error = errno;
#endif /* _WIN32 */
			func = "bind";

			closesocket(fd);

			continue;
		}

		SetFD(fd);

		break;
	}

	freeaddrinfo(result);

	if (GetFD() == INVALID_SOCKET) {
		Log(LogCritical, "TcpSocket")
		    << "Invalid socket: " << Utility::FormatErrorNumber(error);

#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function(func)
		    << boost::errinfo_errno(error));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function(func)
		    << errinfo_win32_error(error));
#endif /* _WIN32 */
	}
}

/**
 * Creates a socket and connects to the specified node and service.
 *
 * @param node The node.
 * @param service The service.
 */
void TcpSocket::Connect(const String& node, const String& service)
{
	addrinfo hints;
	addrinfo *result;
	int error;
	const char *func;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int rc = getaddrinfo(node.CStr(), service.CStr(), &hints, &result);

	if (rc != 0) {
		Log(LogCritical, "TcpSocket")
		    << "getaddrinfo() failed with error code " << rc << ", \"" << gai_strerror(rc) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("getaddrinfo")
		    << errinfo_getaddrinfo_error(rc));
	}

	int fd = INVALID_SOCKET;

	for (addrinfo *info = result; info != NULL; info = info->ai_next) {
		fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		if (fd == INVALID_SOCKET) {
#ifdef _WIN32
			error = WSAGetLastError();
#else /* _WIN32 */
			error = errno;
#endif /* _WIN32 */
			func = "socket";

			continue;
		}

		rc = connect(fd, info->ai_addr, info->ai_addrlen);

		if (rc < 0) {
#ifdef _WIN32
			error = WSAGetLastError();
#else /* _WIN32 */
			error = errno;
#endif /* _WIN32 */
			func = "connect";

			closesocket(fd);

			continue;
		}

		SetFD(fd);

		break;
	}

	freeaddrinfo(result);

	if (GetFD() == INVALID_SOCKET) {
		Log(LogCritical, "TcpSocket")
		    << "Invalid socket: " << Utility::FormatErrorNumber(error);

#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function(func)
		    << boost::errinfo_errno(error));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function(func)
		    << errinfo_win32_error(error));
#endif /* _WIN32 */
	}
}
