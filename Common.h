#pragma once
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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

//Vendor

#include <Multithreading/ThreadPool.h>

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

uint32_t hexToDec(const std::string& hex)
{
	try {
		return static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
	}
	catch (const std::exception& e) {
		throw std::invalid_argument("Invalid hex string: " + hex);
	}
}