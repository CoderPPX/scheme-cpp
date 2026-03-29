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
};

inline const static std::vector<BuiltinProcedure> BUILTINS = {
	BuiltinProcedure("+", BuiltinProcedure::builtin_add),
	BuiltinProcedure("-", BuiltinProcedure::builtin_sub),
	BuiltinProcedure("*", BuiltinProcedure::builtin_mul),
	BuiltinProcedure("/", BuiltinProcedure::builtin_div),
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
	Value formals, body;
	inline LambdaProcedure(Value formals_, Value body_, FramePtr env_)
		: formals(formals_), body(body_), env(env_) {}
	inline std::string str() override { return Pair("lambda", Pair(formals, body)).str(); }
	inline std::string repr() override { return fmt::format("(lambda ({}) {})", formals, body); }
	inline ~LambdaProcedure() override {}
};

inline Value do_define_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		if (args.size() != 2) {
			SCHEME_THROW("badly formed expression");
		}
		Value signature = args[0];
		if (signature.isSymbol()) {
			env->define(signature.toType<std::string>(), eval(args[1], env_));
			return args[0];
		} else if (signature.isType<PairPtr>() && signature.toPair()->car.isSymbol()) {
			// TBD
		}
		SCHEME_THROW("non-symbol");
	}
	SCHEME_THROW("frame already released");
}

inline Value do_quote_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		if (args.size() != 1) {
			SCHEME_THROW("badly formed expression");
		}
		return args[0];
	}
	SCHEME_THROW("frame already released");
}

inline Value do_lambda_form(const ValueList &args, FramePtr env_) {
	if (auto env = env_.lock()) {
		if (args.size() != 2) {
			SCHEME_THROW("badly formed expression");
		}
		return std::make_shared<LambdaProcedure>(args[0], args[1], env_);
	}
	SCHEME_THROW("frame already released");
}

static std::unordered_map<std::string, FuncType> SPECIAL_FORMS = {
	{"define", do_define_form},
	{"quote", do_quote_form},
	{"lambda", do_lambda_form},
};

} // namespace Scheme