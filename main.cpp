#include <iostream>
#include "scheme.hpp"

int main() {
	std::string line;
	auto env = std::make_shared<Scheme::Frame>();
	fmt::print(">>> ");
	while (std::getline(std::cin, line)) {
		try {
			for (auto &expr : Scheme::parseLine(line)) {
				fmt::print("{}\n", eval(expr, env));
			}
			line.clear();
		} catch (const std::exception &err) {
			fmt::print("{}\n", err.what());
		}
		fmt::print(">>> ");
	}
	return 0;
}