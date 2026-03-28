#pragma once
#include <functional>
#include "scheme_types.hpp"

namespace Scheme {

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

} // namespace Scheme