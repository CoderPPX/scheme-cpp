#pragma once
#include <regex>
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
		STRING = 0,
		BLANK = 1,
		LPAREN = 2,
		RPAREN = 3,
		NUMBER = 4,
		SYMBOL = 5,
	};
	inline const static std::array<std::string, 6> TOKEN_NAMES = {
		"STRING", "BLANK", "LPAREN", "RPAREN", "NUMBER", "SYMBOL",
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
	Token::Type currentType = Token::BLANK;

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

struct RegexLexer {
	std::vector<Token> tokens;
	/* Test cases:
	"" -> STRING()
	"\" -> ERROR
	"\t" -> STRING(    )
	"\\" -> STRING(\)
	""" -> ERROR
	*/
	inline static const std::regex tokenPattern{
		// R"((""|"(?:\\.|[^\\"])+")|)" // STRING
		R"(("(?:\\.|[^\\"])*")|)" // STRING
		R"((\s+|;.*)|)"			  // UNUSED
		R"((\()|)"				  // LPAREN
		R"((\))|)"				  // RPAREN
		// Notes: 最后的 ?! 防止以字符结尾
		R"(([+-]?(?:\d+\.?\d*|\.\d+)(?:[Ee][+-]?\d+)?)(?![A-Za-z_])|)" // NUMBER
		// Notes: 防止匹配失败的string跑到此处
		R"(([^\s\\"()]+)|)" // SYMBOL
		R"((.+))"			// UNMATCHED
	};
	inline void parse_line(const std::string &source) {
		std::sregex_iterator begin(source.begin(), source.end(), tokenPattern), end{};
		// Notes: 没有匹配到
		for (auto it = begin; it != end; ++it) {
			auto match = *it;
			// Notes: 1-based index
			// match[2] ignored
			if (match[7].matched) {
				SCHEME_THROW(fmt::format("unmatched token '{}'", match[7].str()));
			} else if (match[1].matched) {
				std::string str = match.str(1), trueStr;
				for (size_t i = 1; i < str.size() - 1; ++i) {
					if (char ch = str[i]; ch == '\\') {
						switch (str[++i]) {
						case '\\':
							ch = '\\';
							break;
						case '\"':
							ch = '\"';
							break;
						case 'n':
							ch = '\n';
							break;
						case 'r':
							ch = '\r';
							break;
						case 't':
							ch = '\t';
							break;
						case 'b':
							ch = '\b';
							break;
						case '0':
							ch = '\0';
							break;
						default:
							SCHEME_THROW("unsupported escape character");
						}
						trueStr += ch;
					} else {
						trueStr += ch;
					}
				}
				tokens.emplace_back(Token::STRING, trueStr);
			} else if (match[3].matched) {
				tokens.emplace_back(Token::LPAREN, "");
			} else if (match[4].matched) {
				tokens.emplace_back(Token::RPAREN, "");
			} else if (match[5].matched) {
				tokens.emplace_back(Token::NUMBER, match[5].str());
			} else if (match[6].matched) {
				tokens.emplace_back(Token::SYMBOL, match[6].str());
			}
		}
	}
};

struct Parser {
public:
	std::vector<Value> expressions;

public:
	inline void parse_expressions(const std::vector<Token> &tokens) {
		for (size_t pos = 0; pos < tokens.size();) {
			auto [value, next_pos] = parse(tokens, pos);
			pos = next_pos;
			expressions.emplace_back(value);
		}
	}

	inline std::pair<Value, size_t> parse(const std::vector<Token> &tokens, size_t pos = 0) {
		auto &token = tokens[pos++];
		switch (token.type) {
		case Token::STRING:
			return {token.value, pos};
		case Token::LPAREN: {
			ValueList list;
			while (pos < tokens.size() && tokens[pos].type != Token::RPAREN) {
				auto [value, new_pos] = parse(tokens, pos);
				pos = new_pos;
				list.emplace_back(value);
			}
			if (pos == tokens.size()) {
				SCHEME_THROW("missing )");
			}
			Value result = nil;
			for (auto it = list.rbegin(); it != list.rend(); ++it) {
				result = Pair(*it, result);
			}
			return {result, pos + 1};
		}
		case Token::RPAREN:
			SCHEME_THROW("unmatched right parenthesis")
		case Token::NUMBER:
			return {std::stod(token.value), pos};
		case Token::SYMBOL:
			if (token.value == "#t" || token.value == "#T") {
				return {true, pos};
			} else if (token.value == "#f" || token.value == "#F") {
				return {false, pos};
			}
			return {Symbol(token.value), pos};
		default:
			SCHEME_THROW("unknown token type");
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
