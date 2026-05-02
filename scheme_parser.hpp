#pragma once
#include <regex>
#include <array>
#include <vector>
#include <cctype>
#include <cstdint>
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
		QUOTE = 6,
		BOOLEAN = 7,
		DOT = 8,
	};
	inline const static std::array<std::string, 9> TOKEN_NAMES = {
		"STRING", "BLANK", "LPAREN", "RPAREN", "NUMBER", "SYMBOL", "QUOTE", "BOOLEAN", "DOT",
	};

public:
	Type type;
	std::string value;
	inline Token() = default;
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

namespace SyntaxRule {

inline const std::string NUMERAL_STARTS = "0123456789+-.";
inline const std::string SYMBOL_CHARS = "abcdefghijklmnopqrstuvwxyz"
										"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
										"#!$%&*/:<=>?@^_~[],@`" +
										NUMERAL_STARTS;
inline const std::string STRING_DELIMS = "\"";
inline const std::string WHITESPACE = "\t\n\r ";
inline const std::string SINGLE_CHAR_TOKENS = "()'";
inline const std::string TOKEN_END = WHITESPACE + SINGLE_CHAR_TOKENS + STRING_DELIMS + ';';
inline const std::string DELIMITERS = SINGLE_CHAR_TOKENS + ".";

} // namespace SyntaxRule

inline bool isValidSymbol(const std::string &str) {
	if (str.empty())
		return false;
	for (auto c : str) {
		if (!SyntaxRule::SYMBOL_CHARS.contains(c)) {
			return false;
		}
	}
	return true;
}

// return value: (token string, end index)
inline std::pair<Token, size_t> nextToken(const std::string &str, size_t begin_idx) {
	for (size_t i = begin_idx; i < str.size();) {
		char c = str[i];
		if (c == ';') {
			i = str.find_first_of('\n', i);
			if (i == std::string::npos)
				return {Token(), std::string::npos};
			else
				return {Token(), i + 1};
		} else if (SyntaxRule::WHITESPACE.contains(c)) {
			++i;
		} else if (SyntaxRule::SINGLE_CHAR_TOKENS.contains(c)) {
			Token token;
			if (c == '(')
				token.type = Token::LPAREN;
			else if (c == ')')
				token.type = Token::RPAREN;
			else if (c == '\'')
				token.type = Token::QUOTE;
			return {token, i + 1};
		} else if (SyntaxRule::STRING_DELIMS.contains(c)) {
			i++;
			std::string s;
			while (i < str.size()) {
				if (str[i] == '\"') {
					return {Token(Token::STRING, s), i + 1};
				} else if (str[i] == '\\') {
					if (++i == str.size())
						SCHEME_THROW("string ended abruptly");
					switch (str[i++]) {
					case 'n':
						s += '\n';
						break;
					case 'r':
						s += '\r';
						break;
					case 't':
						s += '\t';
						break;
					case '0':
						s += '\0';
						break;
					case 'b':
						s += '\b';
						break;
					case '"':
						s += '"';
						break;
					default:
						SCHEME_THROW("unsupported escape character");
					}
				} else {
					s += str[i++];
				}
			}
			SCHEME_THROW("string ended abruptly");
		} else {
			size_t j = i;
			std::string s;
			for (; j < str.size() && !SyntaxRule::TOKEN_END.contains(str[j]); ++j) {
				s += str[j];
			}
			try {
				size_t pos;
				double v = std::stod(s, &pos);
				if (pos == s.size())
					return {Token(Token::NUMBER, s), j};
			} catch (...) {
			}
			if (s == "#t" || s == "true")
				return {Token(Token::BOOLEAN, "#t"), j};
			else if (s == "#f" || s == "false")
				return {Token(Token::BOOLEAN, "#f"), j};
			else if (s == ".")
				return {Token(Token::DOT, "."), j};
			else
				return {Token(Token::SYMBOL, s), j};
		}
	}
	return {Token(), std::string::npos};
}

inline std::vector<Token> tokenizeLine(const std::string &line) {
	std::vector<Token> tokens;
	auto [token, idx] = nextToken(line, 0);
	while (idx != std::string::npos) {
		tokens.emplace_back(token);
		std::tie(token, idx) = nextToken(line, idx);
	}
	return tokens;
}

// Stateful function
inline std::tuple<Token, size_t, bool> nextTokenMultiLine(const std::string &line,
														  size_t begin_idx) {
	static std::string multiLineString;
	static int unmatchedParen = 0, stringUnclosed = 0;
	if (stringUnclosed) {
		while (begin_idx < line.size()) {
			if (line[begin_idx] == '"') {
				stringUnclosed = 0;
				return {Token(Token::STRING, std::move(multiLineString)), begin_idx + 1, false};
			} else if (line[begin_idx] == '\\') {
				if (++begin_idx == line.size()) {
					SCHEME_THROW("missing escape character");
				}
				switch (line[begin_idx++]) {
				case 'n':
					multiLineString += '\n';
					break;
				case 'r':
					multiLineString += '\r';
					break;
				case 't':
					multiLineString += '\t';
					break;
				case '0':
					multiLineString += '\0';
					break;
				case 'b':
					multiLineString += '\b';
					break;
				case '"':
					multiLineString += '"';
					break;
				default:
					SCHEME_THROW("unsupported escape character");
				}
			} else {
				multiLineString += line[begin_idx++];
			}
		}
		return {Token(Token::STRING, ""), begin_idx + 1, true};
	}
	for (size_t i = begin_idx; i < line.size();) {
		char c = line[i];
		if (c == ';') {
			i = line.find_first_of('\n', i);
			return {Token(Token::BLANK, ""), i == std::string::npos ? line.size() : (i + 1), false};
		} else if (SyntaxRule::WHITESPACE.contains(c)) {
			++i;
		} else if (SyntaxRule::SINGLE_CHAR_TOKENS.contains(c)) {
			Token token;
			if (c == '(') {
				++unmatchedParen;
				token.type = Token::LPAREN;
			} else if (c == ')') {
				--unmatchedParen;
				token.type = Token::RPAREN;
			} else if (c == '\'') {
				token.type = Token::QUOTE;
			}
			if (unmatchedParen == 0) {
				return {token, i + 1, false};
			} else if (unmatchedParen > 0) {
				return {token, i + 1, true};
			} else {
				unmatchedParen = 0;
				SCHEME_THROW("unmatched right parenthesis");
			}
		} else if (SyntaxRule::STRING_DELIMS.contains(c)) {
			stringUnclosed = 1;
			/* Notes: 此处直接返回 */
			return nextTokenMultiLine(line, begin_idx + 1);
		} else {
			size_t j = i;
			std::string s;
			for (; j < line.size() && !SyntaxRule::TOKEN_END.contains(line[j]); ++j) {
				s += line[j];
			}
			try {
				size_t pos;
				double v = std::stod(s, &pos);
				if (pos == s.size())
					return {Token(Token::NUMBER, s), j, unmatchedParen};
			} catch (...) {
			}
			if (s == "#t" || s == "true")
				return {Token(Token::BOOLEAN, "#t"), j, unmatchedParen};
			else if (s == "#f" || s == "false")
				return {Token(Token::BOOLEAN, "#f"), j, unmatchedParen};
			else if (s == ".")
				return {Token(Token::DOT, "."), j, unmatchedParen};
			else
				return {Token(Token::SYMBOL, s), j, unmatchedParen};
		}
	}
	return {Token(), std::string::npos, true};
}

// return value: whether more line is needed
inline bool tokenizeMultiLine(std::vector<Token> &tokens, const std::string &line) {
	Token token;
	bool moreOnLine = false;
	for (size_t idx = 0; idx < line.size();) {
		std::tie(token, idx, moreOnLine) = nextTokenMultiLine(line, idx);
		if ((token.type != Token::STRING || !moreOnLine) && token.type != Token::BLANK) {
			tokens.push_back(token);
		}
	}
	return moreOnLine;
}

// return value: (expression, end index)
inline std::pair<Value, size_t> readExpr(const std::vector<Token> &tokens, size_t idx);
inline std::pair<Value, size_t> readTail(const std::vector<Token> &tokens, size_t idx) {
	PairPtr expr = nullptr;
	// std::shared_ptr<Value> end = nullptr;
	Value *end = nullptr;
	if (idx < tokens.size() && tokens[idx].type == Token::DOT) {
		SCHEME_THROW("ill-formed dotted list");
	}
	for (Value value; idx < tokens.size();) {
		const Token &token = tokens[idx];
		if (token.type == Token::RPAREN) {
			return {expr ? Value(expr) : nil, idx + 1};
		} else if (token.type == Token::DOT) {
			/* test cases
			'(1 . (2 . 3)) ;Value: (1 2 . 3)
			*/
			try {
				std::tie(*end, idx) = readExpr(tokens, idx + 1);
			} catch (const std::runtime_error &) {
				SCHEME_THROW("ill-formed dotted list");
			}
			return {expr, idx + 1};
		}
		std::tie(value, idx) = readExpr(tokens, idx);
		if (expr == nullptr) {
			expr = std::make_shared<Pair>(value, nil);
			// Notes: 这样是错的, 因为 make_shared 会创建一个副本
			// end = std::make_shared<Value>(expr->cdr);
			end = &expr->cdr;
		} else {
			auto cdr = std::make_shared<Pair>(value, nil);
			*end = cdr;
			// end = std::make_shared<Value>(cdr->cdr);
			end = &cdr->cdr;
		}
	}
	SCHEME_THROW("invalid expression");
}

inline std::pair<Value, size_t> readExpr(const std::vector<Token> &tokens, size_t idx) {
	if (idx >= tokens.size())
		SCHEME_THROW("index out of range");
	Value value;
	const Token &token = tokens[idx++];
	switch (token.type) {
	case Token::NUMBER:
		value = std::stod(token.value);
		break;
	case Token::STRING:
		value = token.value;
		break;
	case Token::SYMBOL:
		value = Symbol(token.value);
		break;
	case Token::BOOLEAN:
		value = bool(token.value == "#t");
		break;
	case Token::QUOTE:
		/* test cases:
		scm> (quote a)
		a
		scm> (quote (a b))
		(a b)
		scm> (quote a b)
		*/
		std::tie(value, idx) = readExpr(tokens, idx);
		// value = List("quote", value);
		value = List(Symbol("quote"), value);
		break;
	case Token::LPAREN:
		std::tie(value, idx) = readTail(tokens, idx);
		break;
	case Token::DOT:
		SCHEME_THROW("dot allowed only in list");
	case Token::RPAREN:
		SCHEME_THROW("unmatched right parenthesis");
	default:
		SCHEME_THROW("unsupported token type");
	}
	return {value, idx};
}

inline void parseTokens(const std::vector<Token> &tokens, std::vector<Value> &expressions) {
	for (size_t idx = 0; idx < tokens.size();) {
		Value value;
		std::tie(value, idx) = readExpr(tokens, idx);
		expressions.emplace_back(value);
	}
}

inline std::vector<Value> parseLine(const std::string &line) {
	std::vector<Value> expressions;
	auto tokens = tokenizeLine(line);
	for (size_t idx = 0; idx < tokens.size();) {
		Value value;
		std::tie(value, idx) = readExpr(tokens, idx);
		expressions.emplace_back(value);
	}
	return expressions;
}

}; // namespace Scheme

template <> struct fmt::formatter<Scheme::Token> {
	constexpr auto parse(format_parse_context &ctx) { return ctx.end(); }
	template <typename FormatContext>
	auto format(const Scheme::Token &val, FormatContext &ctx) const {
		return fmt::format_to(ctx.out(), "{}", val.str());
	}
};
