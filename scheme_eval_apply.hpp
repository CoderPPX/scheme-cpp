#pragma once
#include "scheme_types.hpp"
#include "scheme_procedures.hpp"

namespace Scheme {

// Notes: 用Value还是vector<Value>代表args
inline Value apply(ProcedurePtr procedure_, const ValueList &args_, FramePtr env_);

inline Value eval(Value expr, FramePtr env_, bool tail) {
	if (auto env = env_.lock()) {
		// Notes: 什么是symbol? quoted? variable name?
		if (expr.isSymbol()) {
			return env->lookUp(expr.toSymbol().name);
		} else if (expr.isSelfEvaluating()) {
			return expr;
		}
		if (!expr.isList()) {
			SCHEME_THROW(fmt::format("malformed list: {}", expr.str()));
			return expr;
		}
		auto p = expr.toPair();
		auto car = p->car, cdr = p->cdr;
		if (car.isSymbol() && SPECIAL_FORMS.contains(car.toSymbol().name)) // special forms
		{
			// List(List("f", "x"), List("+", "x", 1)) -> ((f x) (+ x 1))
			return SPECIAL_FORMS[car.toSymbol().name](cdr.toList(), env_);
		} else {
			auto procedure = eval(car, env_);
			if (!procedure.isType<ProcedurePtr>()) {
				SCHEME_THROW(fmt::format("{} object is not callable", procedure.typeName()));
			}
			return apply(procedure.toType<ProcedurePtr>(),
						 cdr.mapToList([&](Value v) { return eval(v, env_); }), env_);
		}
	}
	SCHEME_THROW("frame already released");
}

inline Value evalAll(const ValueList &expr, FramePtr env_) {
	if (expr.empty()) {
		return nil;
	}
	for (size_t i = 0; i < expr.size() - 1; ++i) {
		eval(expr[i], env_);
	}
	return eval(expr.back(), env_);
}

inline Value apply(ProcedurePtr procedure, const ValueList &args_, FramePtr env_) {
	if (auto env = env_.lock()) {
		if (auto builtin = std::dynamic_pointer_cast<BuiltinProcedure>(procedure)) {
			return builtin->cppFunc(args_, env);
		} else if (auto lambda = std::dynamic_pointer_cast<LambdaProcedure>(procedure)) {
			/*
			fmt::print("{}\n", eval(List(List("lambda", List("x", "y"),
				List("+", "x", "y")), 1, 2), global)); 输出2
			*/
			return evalAll(lambda->body, env->makeChildFrame(lambda->formals, args_));
		}
		// else if (auto mu = std::dynamic_pointer_cast<MuProcedure>(procedure))
		// {}
	}
	SCHEME_THROW("frame already released");
}

} // namespace Scheme