#include "scheme_parser.hpp"
#include "scheme_eval_apply.hpp"
using namespace Scheme;

void test_pair() {
	Pair p1(1.0, nil);
	Pair p2(1.0, 2.0);
	Pair p3(1.0, Pair(2.0, nil));
	Pair p4(Pair(1.0, nil), nil);
	Pair p5(Pair(1.0, Pair(2.0, nil)), Pair(1.0, nil));
	Pair p6(nil, nil);
	Value list = List(1, 2, 3, 4, List(1, 2, 3, List(0, "dfs")), 7, "fdfhdsj");
	fmt::print("{}\n{}\n{}\n{}\n{}\n{}\n{}\n", p1, p2, p3, p4, p5, p6, list);
}

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
	fmt::print("{}\n", Value(Pair(1.0, nil)).isList());
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
	Value definition = List("define", List("f", "x"), List("+", "x", 1));
	fmt::print("{}\n", eval(definition, global));
	fmt::print("{}\n", eval("f", global));
	fmt::print("{}\n", eval(List("f", "a"), global));
	/*
	fmt::print("{}\n", eval(Pair("/", Pair("a", Pair("*", Pair("a", Pair(2.0, nil))))), global));
	Error: terminate called after throwing an instance of 'std::runtime_error'
	what():  Error: in function static Scheme::Value Scheme::BuiltinProcedure::number_reduce(const
	Scheme::ValueList&, Operator) [with Operator = Scheme::BuiltinProcedure::builtin_div(const
	Scheme::ValueList&, Scheme::FramePtr)::<lambda(double, double)>; Scheme::ValueList =
	std::vector<Scheme::Value>]: expected parameters to be numbers
	Notes: 上述表达式等价于(/ a * a 2) 应为(/ a (* a 2)) 嵌套Pair代表一次函数调用
	*/
}

void test4() {
	fmt::print("---- {}: Integrated Builtin Test ----\n", __FUNCTION__);
	// 自动调用构造函数，注入 +, -, *, /, <, >, =, cons, car, cdr, list, null?, pair?
	auto global = std::make_shared<Frame>();
	// 1. 测试算术嵌套与变量绑定: (+ 10 (* 5 2)) -> 20
	eval(Pair("define", Pair("x", Pair(10.0, nil))), global);
	auto expr1 = Pair("+", Pair("x", Pair(Pair("*", Pair(5.0, Pair(2.0, nil))), nil)));
	fmt::print("Arithmetic (+ x (* 5 2)): {} (Expected 20)\n", eval(expr1, global).str());
	// 2. 测试比较运算: (< 1 5 10) -> #t
	auto expr2 = Pair("<", Pair(1.0, Pair(5.0, Pair(10, nil))));
	fmt::print("Comparison (< 1 5 10): {}\n", eval(expr2, global).str());
	// 3. 测试列表构造与访问: (car (cdr (list 1 2 3))) -> 2
	// 结构: (car (cdr (list 1.0 2.0 3.0)))
	auto list_const = Pair("list", Pair(1.0, Pair(2.0, Pair(3.0, nil))));
	auto expr3 = Pair("car", Pair(Pair("cdr", Pair(list_const, nil)), nil));
	fmt::print("List (car (cdr (list 1 2 3))): {} (Expected 2)\n", eval(expr3, global).str());
	// 4. 测试逻辑与类型检查: (and (number? x) (not (null? (list 1))))
	// 注：假设你还没写 and，我们用嵌套来实现逻辑
	// (not (null? (list 1))) -> #t
	auto expr4 = Pair("not", Pair(Pair("null?", Pair(Pair("list", Pair(1.0, nil)), nil)), nil));
	fmt::print("Logic (not (null? (list 1))): {}\n", eval(expr4, global).str());
	// 5. 测试 cons 构造非规范列表 (Dotted Pair): (cons 1 2) -> (1 . 2)
	auto expr5 = Pair("cons", Pair(1.0, Pair(2.0, nil)));
	fmt::print("Cons (cons 1 2): {}\n", eval(expr5, global).str());
	fmt::print("{}\n", eval(List("or", R"("2dsfga")", 1, 2, 3, R"("ads")", false), global));
	fmt::print("{}\n", eval(List("and", R"("alhhfga")", 1, false, 3, R"("ashjd")"), global));
}

void test_branch() {
	auto global = std::make_shared<Frame>();
	fmt::print("{}\n", eval(List("define", "x", -1), global));
	fmt::print("{}\n", eval(List("if", List(">", "x", 0), "x", List("-", "x")), global));
	fmt::print("{}\n", eval(List("cond", List(List(">", "x", 0), "x", "x"),
								 List("else", List("-", "x"), List("-", "x"))),
							global));
	fmt::print("{}\n", eval(List("switch", "x", List(1, R"("dsjfshj")"),
								 List(-1, R"("aabcx")", R"("11222xxc")")),
							global));
}
#include <iostream>
int main() {
	// test_pair();
	// test1();
	// test2();
	// test3();
	// test4();
	// test_branch();
	std::string line;
	Scheme::RegexLexer lexer;
	fmt::print(">>> ");
	while (std::getline(std::cin, line)) {
		lexer.tokens.clear();
		lexer.parse_line(line);
		fmt::print("{}\n", fmt::join(lexer.tokens, "\n"));
		fmt::print(">>> ");
	}
	return 0;
}