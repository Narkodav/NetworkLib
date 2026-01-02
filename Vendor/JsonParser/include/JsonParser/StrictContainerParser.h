#pragma once
#include <stdint.h>
#include <array>
#include <stdexcept>
#include <memory>
#include <string>
#include <algorithm>
#include <fstream>

#include "Utils/SIMDUtils.h"
#include "Concepts.h"

namespace Json
{
	class Value;

	// Parser that strictly follows RFC 7159/8259, no comments or trailing commas or multiple roots allowed
	// Also no correctness checks for malformed json
	class StrictContainerParser
	{
	public:
		static constexpr char beginArray = '[';
		static constexpr char endArray = ']';
		static constexpr char beginObject = '{';
		static constexpr char endObject = '}';
		static constexpr char nameSeparator = ':';
		static constexpr char valueSeparator = ',';
		static constexpr char stringStart = '\"';
		static constexpr char stringEnd = '\"';
		static constexpr char escapedCharStart = '\\';
		static constexpr char decimalSeparator = '.';

		static inline const std::string nullLiteral = "null";
		static inline const std::string trueLiteral = "true";
		static inline const std::string falseLiteral = "false";

	private:
		static inline bool isWhitespace(char c) {
			return c == ' ' || c == '\t' || c == '\r' || c == '\n';
		}

		template<Container C>
		static inline void handleEscapedChar(C& input, size_t& i, std::string& string)
		{
			if (i >= input.size()) throw std::runtime_error("Unterminated escape sequence");
			char escaped = input[i];
			switch (escaped) {
			case '"':  string.push_back('"'); break;
			case '\\': string.push_back('\\'); break;
			case '/':  string.push_back('/'); break;
			case 'b':  string.push_back('\b'); break;
			case 'f':  string.push_back('\f'); break;
			case 'n':  string.push_back('\n'); break;
			case 'r':  string.push_back('\r'); break;
			case 't':  string.push_back('\t'); break;
			case 'u': {
				// Unicode escape \uXXXX
				i += 4;
				if (i >= input.size())
					throw std::runtime_error("Invalid unicode escape");

				std::string hex;
				hex.append(&input[i - 3], 4);

				// Convert hex to unicode
				unsigned int codepoint = std::stoul(hex, nullptr, 16);
				if (codepoint <= 0x7F) {
					string.push_back(static_cast<char>(codepoint));
				}
				else {
					// Convert Unicode codepoint to UTF-8
					if (codepoint <= 0x7FF) {
						// 2-byte UTF-8
						string.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
						string.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
					}
					else {
						// 3-byte UTF-8 (covers BMP)
						string.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
						string.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
						string.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
					}
				}
				break;
			}
			default:
				throw std::runtime_error(std::string("Invalid escape sequence: \\") + escaped);
			}
		}

	private:

#ifdef HAS_AVX2
		template<Container C>
		static inline size_t skipWhitespaceSIMD32(const C& input, size_t i) {
			const size_t size = input.size();

			static const __m256i ws_space = _mm256_set1_epi8(' ');
			static const __m256i ws_tab = _mm256_set1_epi8('\t');
			static const __m256i ws_cr = _mm256_set1_epi8('\r');
			static const __m256i ws_lf = _mm256_set1_epi8('\n');

			__m256i chunk;
			uint32_t mask;

			while (i + 32 <= size) {
				chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i]));

				mask = ~_mm256_movemask_epi8(_mm256_or_si256(
					_mm256_or_si256(_mm256_cmpeq_epi8(chunk, ws_space), _mm256_cmpeq_epi8(chunk, ws_tab)),
					_mm256_or_si256(_mm256_cmpeq_epi8(chunk, ws_cr), _mm256_cmpeq_epi8(chunk, ws_lf))));

				if (mask != 0) {
					size_t j = i + CTZ32(mask);
					return j;
				}

				i += 32;
			}

			for (;; ++i) {
				if (isWhitespace(input[i])) continue;
				return i;
			}
			return i;
		};
#endif

#ifdef HAS_SSE2
		template<Container C>
		static inline size_t skipWhitespaceSIMD16(const C& input, size_t i) {
			const size_t size = input.size();

			static const __m128i ws_space = _mm_set1_epi8(' ');
			static const __m128i ws_tab = _mm_set1_epi8('\t');
			static const __m128i ws_cr = _mm_set1_epi8('\r');
			static const __m128i ws_lf = _mm_set1_epi8('\n');

			__m128i chunk;
			uint16_t mask;

			while (i + 16 <= size) {
				chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));

				mask = ~_mm_movemask_epi8(_mm_or_si128(
					_mm_or_si128(_mm_cmpeq_epi8(chunk, ws_space), _mm_cmpeq_epi8(chunk, ws_tab)),
					_mm_or_si128(_mm_cmpeq_epi8(chunk, ws_cr), _mm_cmpeq_epi8(chunk, ws_lf))));

				if (mask != 0) {
					size_t j = i + CTZ16(mask);
					return j;
				}

				i += 16;
			}

			for (;; ++i) {
				if (isWhitespace(input[i])) continue;
				return i;
			}
			return i;
		};
#endif

		template<Container C>
		static inline size_t skipWhitespaceScalar(const C& input, size_t i) {
			for (;; ++i) {
				if (isWhitespace(input[i])) continue;
				return i;
			}
			return i;
		}


		template<Container C>
		static inline size_t skipWhitespace(const C& input, size_t i) {
#ifdef HAS_AVX2
			return skipWhitespaceSIMD32(input, i);
#elif defined(HAS_SSE2)
			return skipWhitespaceSIMD16(input, i);
#else 
			return skipWhitespaceScalar(input, i);
#endif
		}

		static inline bool isNumber(char c) {
			return (c >= '0' && c <= '9') || c == '-' || c == 'e' || c == 'E' || c == decimalSeparator;
		}

		template<Container C>
		static inline Value parseNumber(C& input, size_t& i) {
			std::string string;
			bool isFloat = false;

			for (;; ++i) {
				char c = input[i];
				if (!isNumber(c)) break;
				if (c == decimalSeparator || c == 'e' || c == 'E') isFloat = true;
				string.push_back(c);
			}

			if (isFloat) {
				return Value(std::stod(string));
			}
			else {
				return Value(std::stoi(string));
			}
		}

		template<Container C>
		static inline std::string parseString(C& input, size_t& i)
		{
			++i;
			size_t size = 0;

			size_t end = i;
			for (;; ++end, ++size) {
				if (input[end] == stringEnd) break;
				if (input[end] == escapedCharStart) ++end;
			}

			std::string string;
			string.reserve(size);

			char c;
			for (;; ++i)
			{
				c = input[i];
				if (c == stringEnd)
				{
					++i;
					return string;
				}
				else if (c == escapedCharStart)
					handleEscapedChar(input, ++i, string);
				else string.push_back(c);
			}
			throw std::runtime_error("Invalid string syntax");
		}

		template<Container C>
		static inline Value parseLiteral(C& input, size_t& i, const std::string& literal, Value value)
		{
			i += literal.size();
			return value;
		}

		template<Container C>
		static Value parseArray(C& input, size_t& i) {
			Value value = Value::array();
			auto& array = value.asArray();
			i = skipWhitespace(input, ++i);
			if (input[i] == endArray) { ++i; return value; }
			while (true) {				
				array.emplace_back(parseValue(input, i));
				i = skipWhitespace(input, i);
				if (input[i] == endArray) { ++i; return value; }
				i = skipWhitespace(input, ++i);
			}
		}

		template<Container C>
		static Value parseObject(C& input, size_t& i) {
			Value value = Value::object();
			auto& object = value.asObject();
			i = skipWhitespace(input, ++i);
			if (input[i] == endObject) { ++i; return value; }
			while (true) {
				std::string name = parseString(input, i);
				i = skipWhitespace(input, i);
				i = skipWhitespace(input, ++i);
				object[name] = parseValue(input, i);
				i = skipWhitespace(input, i);
				if (input[i] == endObject) { ++i; return value; }
				i = skipWhitespace(input, ++i);
			}
		}

		static inline bool isNumberStart(char c) {
			return (c >= '0' && c <= '9') || c == '-';
		}

		template<Container C>
		static Value parseValue(C& input, size_t& i) {
			char c = input[i];
			switch (c) {
			case beginObject: return parseObject(input, i);
			case beginArray: return parseArray(input, i);
			case stringStart: return Value(parseString(input, i));
			case 't': return parseLiteral(input, i, trueLiteral, Value(true));
			case 'f': return parseLiteral(input, i, falseLiteral, Value(false));
			case 'n': return parseLiteral(input, i, nullLiteral, Value(nullptr));
			default: parseNumber(input, i);
			}
		}

	public:
		template<Container C>
		static Value parse(C& input)
		{
			Value value;
			try {
				size_t i = 0;
				i = skipWhitespace(input, i);
				if (i < input.size()) value = parseValue(input, i);
			}
			catch (const std::exception& e) {
				throw std::runtime_error(std::string("JSON parsing failed: ") + e.what());
			}
			return value;
		}
	};
}