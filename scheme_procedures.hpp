#pragma once
#include <functional>
#include "scheme_types.hpp"

namespace Scheme {

inline Value eval(Value expr, FramePtr env_, bool tail = false);

struct BuiltinProcedure : public Procedure {
    using FuncType =
        std::function<Value(const std::vector<Value> &args, FramePtr env)>;
    FuncType cppFunc;
    std::string name;
    inline BuiltinProcedure(FuncType func, const std::string &name_)
        : cppFunc(func), name(name_) {}
    inline std::string str() override {
        return fmt::format("#<builtin procedure {}>", name);
    }
    inline std::string repr() override {
        return name;
    }
};

struct LambdaProcedure : public Procedure {
    FramePtr env;
    Value formals, body;
    inline LambdaProcedure(Value formals_, Value body_, FramePtr env_)
        : formals(formals_), body(body_), env(env_) {}
    inline std::string str() override {
        return Pair("lambda", Pair(formals, body)).str();
    }
    inline std::string repr() override {
        return fmt::format("(lambda ({}) {})", formals, body);
    }
};

inline Value do_define_form(const std::vector<Value> &args, FramePtr env_) {
    if (auto env = env_.lock()) {
        if (args.size() != 2) {
            SCHEME_THROW("badly formed expression");
        }
        Value signature = args[0];
        if (signature.isSymbol()) {
            env->define(signature.toType<std::string>(), eval(args[1], env_));
            return args[0];
        } else if (
            signature.isType<PairPtr>() && signature.toPair()->car.isSymbol()) {
            // TBD
        }
        SCHEME_THROW("non-symbol");
    }
    SCHEME_THROW("frame already released");
}

inline Value do_quote_form(const std::vector<Value> &args, FramePtr env_) {
    if (auto env = env_.lock()) {
        if (args.size() != 1) {
            SCHEME_THROW("badly formed expression");
        }
        return args[0];
    }
    SCHEME_THROW("frame already released");
}

static std::unordered_map<std::string, BuiltinProcedure::FuncType>
    SPECIAL_FORMS = {
        {"define", do_define_form},
};

} // namespace Scheme