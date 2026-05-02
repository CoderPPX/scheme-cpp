#pragma once
#include <fstream>
#include "scheme_parser.hpp"
#include "scheme_eval_apply.hpp"
#include "thirdparty/cpp-linenoise/linenoise.hpp"

namespace Scheme {

inline int REPL() {
	auto globalEnv = std::make_shared<Scheme::Frame>();
	std::vector<Token> tokens;
	std::vector<Value> expressions;
	std::string line, prompt = ">>> ";
	linenoise::SetHistoryMaxLen(200);
	while (!linenoise::Readline(prompt.c_str(), line)) {
		try {
			linenoise::AddHistory(line.c_str());
			if (tokenizeMultiLine(tokens, line)) {
				prompt = "... ";
			} else {
				prompt = ">>> ";
				parseTokens(tokens, expressions);
				for (auto &expr : expressions) {
					fmt::println("{}", eval(expr, globalEnv).repr());
				}
				tokens.clear();
				expressions.clear();
			}
		} catch (const SchemeExit &exitErr) {
			return exitErr.code;
		} catch (const std::runtime_error &err) {
			fmt::println(stderr, "In file <stdin>\n{}", err.what());
		}
		line.clear();
	}
	return 0;
}

inline int execFile(const std::string &filePath) {
	auto globalEnv = std::make_shared<Scheme::Frame>();
	std::fstream file(filePath, std::ios::in);
	if (!file) {
		fmt::println(stderr, "Error: Cannot open file {}", filePath);
		return 1;
	}
	std::string code{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
	std::vector<Value> expressions;
	try {
		expressions = parseLine(code);
	} catch (const std::runtime_error &err) {
		fmt::println(stderr, "In file {}\n{}", filePath, err.what());
		return 1;
	}
	for (auto &expr : expressions) {
		try {
			eval(expr, globalEnv);
		} catch (const SchemeExit &exitErr) {
			return exitErr.code;
		} catch (const std::runtime_error &err) {
			fmt::println(stderr, "In file {}\n{}", filePath, err.what());
			return 1;
		}
	}
	return 0;
}

} // namespace Scheme