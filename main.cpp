#include <iostream>
#include "scheme.hpp"

int main() {
	std::string line;
	Scheme::Parser parser;
	Scheme::RegexLexer lexer;
	auto env = std::make_shared<Scheme::Frame>();
	fmt::print(">>> ");
	while (std::getline(std::cin, line)) {
		lexer.tokens.clear();
		parser.expressions.clear();
		try {
			lexer.parse_line(line);
			if (!lexer.tokens.empty()) {
				parser.parse_expressions(lexer.tokens);
				for (auto expr : parser.expressions) {
					fmt::print("{}\n", eval(expr, env));
				}
			}
			line.clear();
		} catch (const std::exception &err) {
			fmt::print("{}\n", err.what());
		}
		fmt::print(">>> ");
	}
	return 0;
}