#pragma once
#include <array>
#include <vector>
#include <cctype>
#include <cstdint>
#include <fmt/format.h>
#include "scheme_types.hpp"

namespace Scheme {

struct Token {
public:
	enum Type : uint32_t {
		NUMBER = 0,
		STRING = 1,
		SYMBOL = 2,
		BOOLEAN = 3,
		LPAREN = 4,
		RPAREN = 5,
		BLANK = 6,
		QUOTE = 7,
	};
	inline const static std::array<std::string, 8> TOKEN_NAMES = {
		"NUMBER", "STRING", "SYMBOL", "BOOLEAN", "LPAREN", "RPAREN", "BLANK", "QUOTE",
	};

public:
	Type type;
	std::string value;
	inline Token(Type type_, const std::string &value_) : type(type_), value(value_) {}
	inline std::string str() const { return fmt::format("{}({})", TOKEN_NAMES[type], value); }
};

struct Lexer {
public:
	std::vector<Token> tokens;
	Token::Type current_type = Token::BLANK;

public:
	inline static bool ignoring(char c) { return c == ' ' || c == '\t' || c == '\n'; }
	// Notes: 忽略\n
	inline void parse_line(const std::string &source) {
		for (size_t i = 0; i < source.size();) {
			while (i < source.size() && ignoring(source[i])) {
				i++;
			}
			if (i >= source.size())
				break;
			char ch = source[i];
			if (ch == '\"') {
				i++; // Notes: 跳过开头的双引号
				std::string str;
				bool escape = false;
				for (; i < source.size();) {
					if (escape) {
						switch (source[i++]) {
						case 't':
							str += '\t';
							break;
						case 'n':
							str += '\n';
							break;
						case 'b':
							str += '\b';
							break;
						case 'r':
							str += '\r';
							break;
						case '\\':
							str += '\\';
							break;
						case '0':
							str += '\0';
							break;
						case '\"':
							str += '\"';
							break;
						default:
							SCHEME_THROW("unsupported escape character");
						}
						escape = false;
						continue;
					}
					if (source[i] == '\"') {
						break;
					} else if (source[i] == '\\') {
						escape = true;
						i++;
					} else {
						str += source[i++];
					}
				}
				if (i >= source.size()) {
					SCHEME_THROW("could not find end of string");
				}
				tokens.emplace_back(Token::STRING, str);
				i++;
			} else if (ch == ';') {
				return;
			} else if (ch == '(') {
				tokens.emplace_back(Token::LPAREN, "");
				i++;
			} else if (ch == ')') {
				tokens.emplace_back(Token::RPAREN, "");
				i++;
			} else if (ch == '\'') {
				tokens.emplace_back(Token::QUOTE, "");
				i++;
			} else {
				std::string str;
				bool could_be_number = false;
				if (isdigit(ch) || ch == '.' || ch == '+' || ch == '-') {
					could_be_number = true;
				}
				while (i < source.size()) {
					ch = source[i];
					if (ignoring(ch) || ch == '(' || ch == ')' || ch == ';' || ch == '\"' ||
						ch == '\'') {
						break;
					}
					str += ch;
					i++;
				}
				if (str.empty()) {
					continue;
				}
				if (could_be_number) {
					bool is_number = true;
					bool has_digit = false;
					bool has_point = false;
					bool has_exponent = false;
					for (size_t j = 0; j < str.size() && is_number; ++j) {
						char c = str[j];
						if (isdigit(c)) {
							has_digit = true;
						} else if (c == '+' || c == '-') {
							if (j != 0 && (j == 0 || !(str[j - 1] == 'e' || str[j - 1] == 'E'))) {
								is_number = false;
							}
						} else if (c == '.') {
							if (has_point || has_exponent) {
								is_number = false;
							}
							has_point = true;
						} else if (c == 'e' || c == 'E') {
							if (has_exponent || !has_digit) {
								is_number = false;
							}
							has_exponent = true;
							has_digit = false;
						} else {
							is_number = false;
						}
					}
					if (is_number && has_digit) {
						tokens.emplace_back(Token::NUMBER, str);
						continue;
					}
				}
				tokens.emplace_back(Token::SYMBOL, str);
			}
		}
	}
};


}; // namespace Scheme

template <> struct fmt::formatter<Scheme::Token> {
	constexpr auto parse(format_parse_context &ctx) { return ctx.end(); }
	template <typename FormatContext>
	auto format(const Scheme::Token &val, FormatContext &ctx) const {
		return fmt::format_to(ctx.out(), "{}", val.str());
	}
};
