#pragma once
#include <typeinfo>
#include "scheme_types.hpp"
#include "scheme_procedures.hpp"

namespace Scheme {
inline Value eval(Value expr, FramePtr env_, bool tail = false) {
    if (auto env = env_.lock()) {
        // Notes: 什么是symbol? quoted? variable name?
        if (expr.isSymbol()) {
            return env->lookUp(expr);
        } else if (expr.isSelfEvaluating()) {
            return expr;
        }
        if (!expr.isList()) {
            throw std::runtime_error(
                fmt::format("malformed list: {}", expr.str()));
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
    throw std::runtime_error("in function eval: frame already released");
}

inline Value apply(Value procedure_, Value args_, FramePtr env_) {
    if (auto env = env_.lock()) {
        procedure_.validateProcedure();
        auto procedure = procedure_.toType<ProcedurePtr>();
        if (typeid(procedure.get()) == typeid(BuiltinProcedure *)) {
        }
    }
    throw std::runtime_error("in function apply: frame already released");
}

} // namespace Scheme