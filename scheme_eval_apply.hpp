#pragma once
#include <typeinfo>
#include "scheme_types.hpp"
#include "scheme_procedures.hpp"

namespace Scheme {
inline Value eval(Value expr, FramePtr env_, bool tail) {
    if (auto env = env_.lock()) {
        // Notes: 什么是symbol? quoted? variable name?
        if (expr.isSymbol()) {
            return env->lookUp(expr);
        } else if (expr.isSelfEvaluating()) {
            return expr;
        }
        if (!expr.isList()) {
            SCHEME_THROW(fmt::format("malformed list: {}", expr.str()));
            return expr;
        }
        auto p = expr.toType<PairPtr>();
        auto car = p->car, cdr = p->cdr;
        if (car.isSymbol() && false) // special forms
        {
            // TBD
        } else {
            auto procedure = eval(car, env_);
        }
    }
    SCHEME_THROW("frame already released");
}

inline Value evalAll(Value expr_, FramePtr env_) {
    if (expr_.isNil()) {
        return nil;
    }
    while (!expr_.toType<PairPtr>()->cdr.isNil()) {
        eval(expr_.toType<PairPtr>()->car, env_);
        expr_ = expr_.toType<PairPtr>()->cdr;
    }
    return eval(expr_.toType<PairPtr>()->car, env_);
}

inline Value apply(Value procedure_, Value args_, FramePtr env_) {
    if (auto env = env_.lock()) {
        procedure_.validateProcedure();
        auto procedure = procedure_.toType<ProcedurePtr>();
        if (auto builtin =
                std::dynamic_pointer_cast<BuiltinProcedure>(procedure)) {
            return builtin->cppFunc(args_.toList(), env);
        } else if (
            auto lambda =
                std::dynamic_pointer_cast<LambdaProcedure>(procedure)) {
            return evalAll(
                lambda->body, env->makeChildFrame(lambda->formals, args_));
        }
        // else if (auto mu = std::dynamic_pointer_cast<MuProcedure>(procedure))
        // {}
    }
    SCHEME_THROW("frame already released");
}

} // namespace Scheme