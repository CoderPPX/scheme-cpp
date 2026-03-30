#pragma once
#include <functional>
#include "scheme_types.hpp"

namespace Scheme {

using FuncType = std::function<Value(const ValueList &args, FramePtr env)>;

inline Value eval(Value expr, FramePtr env_, bool tail = false);

struct BuiltinProcedure : public Procedure {
public:
	FuncType cppFunc;
	std::string name;
	inline BuiltinProcedure(const std::string &name_, FuncType func) : cppFunc(func), name(name_) {}
	inline std::string str() override { return fmt::format("#<builtin procedure {}>", name); }
	inline std::string repr() override { return name; }
	inline ~BuiltinProcedure() override {}
	// Error: undefined reference to `Scheme::Procedure::~Procedure()
	// Notes: 即使析构函数是纯虚的，你仍然必须提供一个定义（哪怕是空的），否则链接器会报错。

public:
	template <typename Operator = double(double a, double b)>
	inline static Value number_reduce(const ValueList &args, Operator op) {
		if (args.size() == 0) {
			SCHEME_THROW("not enough parameters");
		}
		if (!args[0].isType<double>()) {
			SCHEME_THROW("expected parameters to be numbers");
		}
		double result = args[0].toType<double>();
		for (size_t i = 1; i < args.size(); ++i) {
			if (!args[i].isType<double>()) {
				SCHEME_THROW("expected parameters to be numbers");
			}
			result = op(result, args[i].toType<double>());
		}
		return result;
	}
	inline static Value builtin_add(const ValueList &args, FramePtr) {
		return number_reduce(args, [](double a, double b) { return a + b; });
	}
	inline static Value builtin_sub(const ValueList &args, FramePtr) {
		if (args.size() == 1) {
			return -args[0].toType<double>();
		}
		return number_reduce(args, [](double a, double b) { return a - b; });
	}
	inline static Value builtin_mul(const ValueList &args, FramePtr) {
		return number_reduce(args, [](double a, double b) { return a * b; });
	}
	inline static Value builtin_div(const ValueList &args, FramePtr) {
		return number_reduce(args, [](double a, double b) {
			if (b == 0.0) {
				SCHEME_THROW("division by zero");
			}
			return a / b;
		});
	}
	// 比较运算模板：检查是否满足单调性，例如 (< 1 2 3) 为真
	template <typename Compare>
	inline static Value number_compare(const ValueList &args, Compare comp) {
		if (args.size() < 2)
			return true; // 单个数字比较默认为真
		for (size_t i = 0; i < args.size() - 1; ++i) {
			if (!args[i].isNumber() || !args[i + 1].isNumber()) {
				SCHEME_THROW("comparisons expect numbers");
			}
			if (!comp(args[i].toType<double>(), args[i + 1].toType<double>())) {
				return false;
			}
		}
		return true;
	}
	inline static Value builtin_lt(const ValueList &args, FramePtr) {
		return number_compare(args, std::less<double>());
	}
	inline static Value builtin_le(const ValueList &args, FramePtr) {
		return number_compare(args, std::less_equal<double>());
	}
	inline static Value builtin_gt(const ValueList &args, FramePtr) {
		return number_compare(args, std::greater<double>());
	}
	inline static Value builtin_ge(const ValueList &args, FramePtr) {
		return number_compare(args, std::greater_equal<double>());
	}
	inline static Value builtin_eq(const ValueList &args, FramePtr) {
		return number_compare(args, std::equal_to<double>());
	}
	inline static Value builtin_not(const ValueList &args, FramePtr) {
		if (args.size() != 1) {
			SCHEME_THROW("not expects 1 argument");
		}
		return args[0].isFalse(); // Scheme 中只有 #f 是假，其余皆为真
	}
	// Pair and List
	inline static Value builtin_cons(const ValueList &args, FramePtr) {
		if (args.size() != 2)
			SCHEME_THROW("cons expects 2 arguments");
		return std::make_shared<Pair>(args[0], args[1]);
	}
	inline static Value builtin_car(const ValueList &args, FramePtr) {
		if (args.size() != 1 || !args[0].isPair())
			SCHEME_THROW("car expects a pair");
		return args[0].toPair()->car;
	}
	inline static Value builtin_cdr(const ValueList &args, FramePtr) {
		if (args.size() != 1 || !args[0].isPair())
			SCHEME_THROW("cdr expects a pair");
		return args[0].toPair()->cdr;
	}
	inline static Value builtin_list(const ValueList &args, FramePtr) {
		Value result = nil;
		// 从后往前构造 Pair 链表
		for (auto it = args.rbegin(); it != args.rend(); ++it) {
			result = std::make_shared<Pair>(*it, result);
		}
		return result;
	}
	// Type checks
	inline static Value builtin_is_null(const ValueList &args, FramePtr) {
		if (args.size() != 1)
			SCHEME_THROW("null? expects 1 argument");
		return args[0].isNil();
	}
	inline static Value builtin_is_pair(const ValueList &args, FramePtr) {
		return args.size() == 1 && args[0].isPair();
	}
	inline static Value builtin_is_number(const ValueList &args, FramePtr) {
		return args.size() == 1 && args[0].isNumber();
	}
	// Display
	inline static Value builtin_display(const ValueList &args, FramePtr) {
		for (const auto &arg : args) {
			fmt::print("{}", arg.str());
		}
		return nil;
	}
	inline static Value builtin_newline(const ValueList &args, FramePtr) {
		fmt::print("\n");
		return nil;
	}
};

inline const static std::vector<BuiltinProcedure> BUILTINS = {
	// 算术
	{"+", BuiltinProcedure::builtin_add},
	{"-", BuiltinProcedure::builtin_sub},
	{"*", BuiltinProcedure::builtin_mul},
	{"/", BuiltinProcedure::builtin_div},
	// 比较
	{"<", BuiltinProcedure::builtin_lt},
	{"<=", BuiltinProcedure::builtin_le},
	{">", BuiltinProcedure::builtin_gt},
	{">=", BuiltinProcedure::builtin_ge},
	{"=", BuiltinProcedure::builtin_eq},
	// 逻辑
	{"not", BuiltinProcedure::builtin_not},
	// 列表
	{"cons", BuiltinProcedure::builtin_cons},
	{"car", BuiltinProcedure::builtin_car},
	{"cdr", BuiltinProcedure::builtin_cdr},
	{"list", BuiltinProcedure::builtin_list},
	{"null?", BuiltinProcedure::builtin_is_null},
	{"pair?", BuiltinProcedure::builtin_is_pair},
	// 杂项
	{"display", BuiltinProcedure::builtin_display},
	{"newline", BuiltinProcedure::builtin_newline},
};

inline Frame::Frame() {
	for (const auto &proc : BUILTINS) {
		/*
		define(proc.name, std::make_shared<Procedure>(proc));
		error: invalid new-expression of abstract class type ‘Scheme::Procedure’
		119 |       ::new((void*)__p) _Tp(std::forward<_Args>(__args)...);
		*/
		define(proc.name, std::make_shared<BuiltinProcedure>(proc));
	}
}

struct LambdaProcedure : public Procedure {
	FramePtr env;
	ValueList formals, body;
	// formals: vector of string symbol, body: vector of body expressions
	inline LambdaProcedure(const ValueList &formals_, const ValueList &body_, FramePtr env_)
		: formals(formals_), body(body_), env(env_) {}
	inline std::string str() override {
		return fmt::format("(lambda ({}) {})", fmt::join(formals, " "), fmt::join(body, " "));
	}
	inline std::string repr() override { return "lambda"; } // TBD
	inline ~LambdaProcedure() override {}
};

inline Value do_quote_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		if (args.size() != 1) {
			SCHEME_THROW("badly formed expression");
		}
		return args[0];
	}
	SCHEME_THROW("frame already released");
}

// args[0] = formals的Pair形式, args[1:] = body的List形式
inline Value do_lambda_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		if (args.size() < 2 || !args[0].isList() || !args[1].isList()) {
			SCHEME_THROW("badly formed expression");
		}
		// Notes:
		// (lambda (x) (+ x 1)): args = List(List(x), List(+, x, 1))
		// (lambda (x) (+ x 1) (+ x 1)) List(List(x), List(+, x, 1), List(+, x, 1))
		// 错误: return std::make_shared<LambdaProcedure>(args[0].toList(), args[1].toList(), env_);
		// (lambda (x) (+ x 1)) 会解析为 (lambda (x) + x 1)
		return std::make_shared<LambdaProcedure>(
			args[0].toList(), std::vector<Value>(args.begin() + 1, args.end()), env_);
	}
	SCHEME_THROW("frame already released");
}

inline Value do_define_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		if (args.size() != 2) {
			SCHEME_THROW("badly formed expression");
		}
		Value signature = args[0];
		if (signature.isSymbol()) {
			env->define(signature.toType<std::string>(), eval(args[1], env_));
			return args[0];
		} else if (signature.isList() && signature.toPair()->car.isSymbol()) {
			auto pair = signature.toPair();
			env->define(pair->car.toString(), do_lambda_form({pair->cdr, args[1]}, env_));
			return pair->car;
		}
		SCHEME_THROW("non-symbol");
	}
	SCHEME_THROW("frame already released");
}

inline Value do_and_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		Value result = true;
		for (auto &arg : args) {
			result = eval(arg, env_);
			if (result.isFalse()) {
				return false;
			}
		}
		return result;
	}
	SCHEME_THROW("frame already released");
}

inline Value do_or_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		Value result = false;
		for (auto &arg : args) {
			result = eval(arg, env_);
			if (result.isTrue()) {
				return result;
			}
		}
		return false;
	}
	SCHEME_THROW("frame already released");
}

/* Notes: requires args.size() == 3
(if (> x 0) x (- x)) -> List(List(>, x, 0), x, List(-, x))
*/
inline Value do_if_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		if (args.size() != 3) {
			SCHEME_THROW("badly formed if expression");
		}
		if (eval(args[0], env_).isTrue()) {
			return eval(args[1], env_);
		} else {
			return eval(args[2], env_);
		}
	}
	SCHEME_THROW("frame already released");
}

/* Notes: requires args.size() >= 1
(cond ((> x 0) x) ((<= x 0) (- x))) ->
	List(List(List(>, x, 0), x), List(List(<=, x, 0), List(-, x)))
*/
inline Value do_cond_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		if (args.empty()) {
			SCHEME_THROW("empty cond expression");
		}
		for (auto &expr : args) {
			if (!expr.isList()) {
				SCHEME_THROW("badly formed cond expression");
			}
			auto expr_list = expr.toList();
			if (expr_list.size() != 2) {
				SCHEME_THROW("badly formed cond expression");
			}
			auto cond = expr_list[0];
			if ((cond.isSymbol() && cond.toString() == "else") || eval(cond, env_).isTrue()) {
				return eval(expr_list[1], env_);
			}
		}
	}
	SCHEME_THROW("frame already released");
}

static std::unordered_map<std::string, FuncType> SPECIAL_FORMS = {
	{"define", do_define_form}, {"quote", do_quote_form}, {"lambda", do_lambda_form},
	{"and", do_and_form},		{"or", do_or_form},		  {"if", do_if_form},
	{"cond", do_cond_form},
};

} // namespace Scheme