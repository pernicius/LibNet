#pragma once

#include <memory>
#include <deque>
#include <mutex>
#include <vector>
#include <iostream>


// for ASIO only
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif


// for ASIO only
#define _WINSOCK_DEPRECATED_NO_WARNINGS
// setup ASIO to be used without boost
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


// Set default namespace (can be set by application)
#ifndef NETLIB_NAMESPACE
#	define NETLIB_NAMESPACE Net
#endif
