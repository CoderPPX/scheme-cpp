#include "scheme_eval_apply.hpp"
using namespace Scheme;

void test1() {
	fmt::print("[ {} ]\n", __FUNCTION__);
	auto global = std::make_shared<Frame>();
	global->define("a", "221412421");
	global->define("b", true);
	global->define("c", 1.0);
	fmt::print("{}\n", eval(Value("a"), global));
	fmt::print("{}\n", eval(Value("b"), global));
	fmt::print("{}\n", eval(Value("c"), global));
	fmt::print("{}\n", eval(Value(1.22), global));
}

void test2() {
	fmt::print("[ {} ]\n", __FUNCTION__);
	auto global = std::make_shared<Frame>();
	fmt::print("{}\n", eval(Pair("define", Pair("a", Pair(1.0, nil))), global));
	fmt::print("{}\n", eval(Value("a"), global));
	fmt::print("{}\n", eval(Pair("quote", Pair("b", nil)), global));
}

void test3() {
	fmt::print("[ {} ]\n", __FUNCTION__);
	auto global = std::make_shared<Frame>();
	fmt::print("{}\n", eval(Pair("define", Pair("a", Pair(2.0, nil))), global));
	fmt::print("{}\n", eval(Value("a"), global));
	/*
	fmt::print("{}\n", eval(Pair("/", Pair("a", Pair("*", Pair("a", Pair(2.0, nil))))), global));
	Error: terminate called after throwing an instance of 'std::runtime_error'
	what():  Error: in function static Scheme::Value Scheme::BuiltinProcedure::number_reduce(const
	Scheme::ValueList&, Operator) [with Operator = Scheme::BuiltinProcedure::builtin_div(const
	Scheme::ValueList&, Scheme::FramePtr)::<lambda(double, double)>; Scheme::ValueList =
	std::vector<Scheme::Value>]: expected parameters to be numbers
	Notes: 上述表达式等价于(/ a * a 2) 应为(/ a (* a 2)) 嵌套Pair代表一次函数调用
	*/
	fmt::print("{}\n",
			   eval(Pair("/", Pair("a", Pair(Pair("*", Pair("a", Pair(2.0, nil))), nil))), global));
}

int main() {
	test1();
	test2();
	test3();
	return 0;
}