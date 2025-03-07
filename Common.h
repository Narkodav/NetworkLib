#pragma once
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <memory>
#include <string>
#include <array>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <chrono>
#include <map>
#include <set>
#include <unordered_map>
#include <filesystem>

//Vendor
#include <Multithreading/ThreadPool.h>


namespace http {

#ifndef CUSTOM_RETRY_COUNT
	static inline const size_t MAX_RETRY_COUNT = 5;
#else
	static inline const size_t MAX_RETRY_COUNT = CUSTOM_RETRY_COUNT;
#endif

#ifndef CUSTOM_BODY_LIMIT
	static inline const size_t MAX_BODY_SIZE = 1024 * 1024 * 16;  // 16MB
#else
	static inline const size_t MAX_BODY_SIZE = CUSTOM_BODY_LIMIT;
#endif

#ifndef CUSTOM_HEADER_LIMIT
	static inline const size_t MAX_HEADER_SIZE = 1024 * 16; //16 KBs
#else
	static inline const size_t MAX_HEADER_SIZE = CUSTOM_HEADER_LIMIT; //16 KBs
#endif

#ifndef CUSTOM_HEADER_NAME_LIMIT
	static inline const size_t MAX_HEADER_NAME_LENGTH = 256;
#else
	static inline const size_t MAX_HEADER_NAME_LENGTH = CUSTOM_HEADER_NAME_LIMIT;
#endif

#ifndef CUSTOM_HEADER_VALUE_LIMIT
	static inline const size_t MAX_HEADER_VALUE_LENGTH = 8192;
#else
	static inline const size_t MAX_HEADER_VALUE_LENGTH = CUSTOM_HEADER_VALUE_LIMIT;
#endif

}

struct CaseInsensetiveStringComparator
{
	bool operator()(const std::string& lhs, const std::string& rhs) const
	{
		return std::lexicographical_compare(
			lhs.begin(), lhs.end(),
			rhs.begin(), rhs.end(),
			[](char lhs, char rhs) { return std::tolower(lhs) < std::tolower(rhs); }
		);
	}


};

static inline uint32_t hexToDec(const std::string& hex)
{
	try {
		return static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
	}
	catch (const std::exception& e) {
		throw std::invalid_argument("Invalid hex string: " + hex);
	}
}