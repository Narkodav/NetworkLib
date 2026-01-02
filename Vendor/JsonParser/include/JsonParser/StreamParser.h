#pragma once
#include <stdint.h>
#include <array>
#include <stdexcept>
#include <memory>
#include <string>
#include <algorithm>
#include <fstream>

#include "Concepts.h"

namespace Json
{
	class Value;

	class StreamParser
	{
	public:
		static inline const uint8_t beginArray = '[';
		static inline const uint8_t endArray = ']';
		static inline const uint8_t beginObject = '{';
		static inline const uint8_t endObject = '}';
		static inline const uint8_t nameSeparator = ':';
		static inline const uint8_t valueSeparator = ',';

		static inline const std::array<uint8_t, 4> whitespaceCharacters = { ' ', '\t', '\r', '\n' };
		static inline const uint8_t commentStart = '/';
		static inline const uint8_t lineComment = '/';
		static inline const uint8_t blockCommentStart = '*';
		static inline const uint8_t blockCommentEnd = '*';

		static inline const std::string nullLiteral = "null";
		static inline const std::string trueLiteral = "true";
		static inline const std::string falseLiteral = "false";
		static inline const uint8_t decimalSeparator = '.';
		static inline const std::array<uint8_t, 12> numberStartCharacters =
		{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '-' };

		static inline const uint8_t stringStart = '\"';
		static inline const uint8_t stringEnd = '\"';
		static inline const uint8_t escapedCharStart = '\\';

	private:
		template<Stream S>
		static inline void handleEscapedChar(S& input, char& currentChar, std::string& string)
		{
			if (!input.get(currentChar)) throw std::runtime_error("Unterminated escape sequence");
			switch (currentChar) {
			case '"':  string.push_back('"'); break;
			case '\\': string.push_back('\\'); break;
			case '/':  string.push_back('/'); break;
			case 'b':  string.push_back('\b'); break;
			case 'f':  string.push_back('\f'); break;
			case 'n':  string.push_back('\n'); break;
			case 'r':  string.push_back('\r'); break;
			case 't':  string.push_back('\t'); break;
			case 'u': {
				std::string hex(4, '\0');
				for (int i = 0; i < 4; ++i) {
					if (!input.get(hex[i])) throw std::runtime_error("Invalid unicode escape");
				}
				unsigned int codepoint = std::stoul(hex, nullptr, 16);
				if (codepoint <= 0x7F) {
					string.push_back(static_cast<char>(codepoint));
				} else if (codepoint <= 0x7FF) {
					string.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
					string.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
				} else {
					string.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
					string.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
					string.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
				}
				break;
			}
			default:
				throw std::runtime_error(std::string("Invalid escape sequence: \\") + currentChar);
			}
		}

		template<Stream S>
		static inline void skipComment(S& input, char& currentChar) {
			if (!input.get(currentChar)) throw std::runtime_error("Invalid comment syntax");
			if (currentChar == lineComment) {
				while (input.get(currentChar) && currentChar != '\n');
				{
					input.get(currentChar);
					return;
				}
			} else if (currentChar == blockCommentStart) {
				char prev = currentChar;
				while (input.get(currentChar)) {
					if (prev == blockCommentEnd && currentChar == commentStart)
					{ 
						input.get(currentChar);
						return;
					}
					prev = currentChar;
				}
				throw std::runtime_error("Endless block comment");
			}
			throw std::runtime_error("Invalid comment syntax");
		}

		template<Stream S>
		static inline void skipWhitespace(S& input, char& currentChar) {
			while (true) {
				if (currentChar == ' ' || currentChar == '\t' || currentChar == '\r' || currentChar == '\n') {
					if (!input.get(currentChar)) return;
					continue;
				} else if (currentChar == commentStart) {
					skipComment(input, currentChar);
					continue;
				}
				return;
			}
		}

		template<Stream S>
		static inline Value parseNumber(S& input, char& currentChar) {
			std::string string;
			bool isFloat = false;
			
			do {
				string.push_back(currentChar);
				if (currentChar == decimalSeparator || currentChar == 'e' || currentChar == 'E') isFloat = true;
				if (!input.get(currentChar)) break;
			} while ((currentChar >= '0' && currentChar <= '9') || currentChar == '+' || currentChar == '-' || 
					 currentChar == 'e' || currentChar == 'E' || currentChar == decimalSeparator);
			
			if (isFloat) {
				return Value(std::stod(string));
			} else {
				return Value(std::stoi(string));
			}
		}

		template<Stream S>
		static inline std::string parseString(S& input, char& currentChar) {
			std::string string;
			while (input.get(currentChar)) {
				if (currentChar == stringEnd) {
					input.get(currentChar);
					return string;
				} else if (currentChar == escapedCharStart) {
					handleEscapedChar(input, currentChar, string);
				} else {
					string.push_back(currentChar);
				}
			}
			throw std::runtime_error("Invalid string syntax");
		}

		template<Stream S>
		static inline Value parseLiteral(S& input, char& currentChar, const std::string& literal, Value value) {
			for (size_t i = 1; i < literal.size(); ++i) {
				if (!input.get(currentChar) || currentChar != literal[i]) {
					throw std::runtime_error("Invalid " + literal + " literal");
				}
			}
			input.get(currentChar);
			return value;
		}

		template<Stream S>
		static Value parseArray(S& input, char& currentChar) {
			Value value = Value::array();
			auto& array = value.asArray();
			while (true) {
				input.get(currentChar);
				skipWhitespace(input, currentChar);
				if (currentChar == endArray) {
					input.get(currentChar);
					return value;
				}
				array.emplace_back(parseValue(input, currentChar));
				skipWhitespace(input, currentChar);
				if (currentChar == endArray) {
					input.get(currentChar);
					return value;
				} else if (currentChar == valueSeparator) {
					continue;
				} else {
					throw std::runtime_error("Invalid array syntax");
				}
			}
		}

		template<Stream S>
		static Value parseObject(S& input, char& currentChar) {
			Value value = Value::object();
			auto& object = value.asObject();
			while (true) {
				input.get(currentChar);
				skipWhitespace(input, currentChar);
				if (currentChar == endObject) {
					input.get(currentChar);
					return value;
				}
				if (currentChar != stringStart) throw std::runtime_error("Expected string key");
				std::string key = parseString(input, currentChar);
				skipWhitespace(input, currentChar);
				if (currentChar != nameSeparator) throw std::runtime_error("Expected ':'");
				input.get(currentChar);
				skipWhitespace(input, currentChar);
				object[key] = parseValue(input, currentChar);
				skipWhitespace(input, currentChar);
				if (currentChar == endObject) {
					input.get(currentChar);
					return value;
				} else if (currentChar == valueSeparator) {
					continue;
				} else {
					throw std::runtime_error("Invalid object syntax");
				}
			}
		}

		template<Stream S>
		static Value parseValue(S& input, char& currentChar) {
			switch (currentChar) {
			case beginObject: return parseObject(input, currentChar);
			case beginArray: return parseArray(input, currentChar);
			case stringStart: return Value(parseString(input, currentChar));
			case 't': return parseLiteral(input, currentChar, trueLiteral, Value(true));
			case 'f': return parseLiteral(input, currentChar, falseLiteral, Value(false));
			case 'n': return parseLiteral(input, currentChar, nullLiteral, Value(nullptr));
			default:
				if ((currentChar >= '0' && currentChar <= '9') || currentChar == '+' || currentChar == '-') {
					return parseNumber(input, currentChar);
				}
				throw std::runtime_error(std::string("Invalid value: ") + currentChar);
			}
		}

	public:
		template<Stream S>
		static std::vector<Value> parse(S& input) {
			std::vector<Value> document;
			char currentChar;
			try {
				while (input.get(currentChar)) {
					skipWhitespace(input, currentChar);
					if (input.eof()) break;
					document.push_back(parseValue(input, currentChar));
				}
			} catch (const std::exception& e) {
				throw std::runtime_error(std::string("JSON parsing failed: ") + e.what());
			}
			return document;
		}
	};
}