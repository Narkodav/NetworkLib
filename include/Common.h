#pragma once
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


namespace Network::HTTP {

#ifndef CUSTOM_RETRY_COUNT
	static inline const size_t s_maxRetryCount = 5;
#else
	static inline const size_t s_maxRetryCount = CUSTOM_RETRY_COUNT;
#endif

#ifndef CUSTOM_BODY_LIMIT
	static inline const size_t s_maxBodySize = 1024 * 1024 * 16;  // 16MB
#else
	static inline const size_t s_maxBodySize = CUSTOM_BODY_LIMIT;
#endif

#ifndef CUSTOM_HEADER_LIMIT
	static inline const size_t s_maxHeaderSize = 1024 * 16; //16 KBs
#else
	static inline const size_t s_maxHeaderSize = CUSTOM_HEADER_LIMIT; //16 KBs
#endif

#ifndef CUSTOM_HEADER_NAME_LIMIT
	static inline const size_t s_maxHeaderNameLength = 256;
#else
	static inline const size_t s_maxHeaderNameLength = CUSTOM_HEADER_NAME_LIMIT;
#endif

#ifndef CUSTOM_HEADER_VALUE_LIMIT
	static inline const size_t s_maxHeaderValueLength = 8192;
#else
	static inline const size_t s_maxHeaderValueLength = CUSTOM_HEADER_VALUE_LIMIT;
#endif

}

namespace Network::Detail {
	struct CaseInsensitiveStringComparator
	{
		using is_transparent = void;

		bool operator()(std::string_view lhs, std::string_view rhs) const
		{
			return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin(),
				[](char a, char b) { return std::tolower(a) == std::tolower(b); });
		}
	};

	struct CaseInsensitiveHasher
	{
		using is_transparent = void;

		size_t operator()(const std::string& str) const
		{
			size_t hash = 0;
			for (char c : str)
			{
				hash = hash * 31 + std::tolower(c);
			}
			return hash;
		}
		size_t operator()(std::string_view str) const
		{
			size_t hash = 0;
			for (char c : str)
			{
				hash = hash * 31 + std::tolower(c);
			}
			return hash;
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

	struct TransparentStringHash {
		using is_transparent = void;  // Key feature!

		size_t operator()(const std::string& s) const {
			return std::hash<std::string>{}(s);
		}

		size_t operator()(std::string_view sv) const {
			return std::hash<std::string_view>{}(sv);
		}
	};

	struct TransparentStringEqual {
		using is_transparent = void;  // Key feature!

		bool operator()(const std::string& lhs, const std::string& rhs) const {
			return lhs == rhs;
		}

		bool operator()(const std::string& lhs, std::string_view rhs) const {
			return lhs == rhs;
		}

		bool operator()(std::string_view lhs, const std::string& rhs) const {
			return lhs == rhs;
		}
	};
}