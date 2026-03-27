#pragma once
#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <fmt/format.h>

namespace Scheme {

struct Pair;
struct Frame;
struct Value;
struct Procedure;

using PairPtr = std::shared_ptr<Pair>;
using FramePtr = std::shared_ptr<Frame>;
using ProcedurePtr = std::shared_ptr<Procedure>;

struct NilType : public std::monostate {};
const static NilType nil{};

// Notes: 为什么使用variant
struct Value : public std::variant<
                   double,       // Number
                   bool,         // Boolean
                   std::string,  // Symbol
                   PairPtr,      // Pair
                   ProcedurePtr, // Functions
                   NilType       // Nil
                   > {
    // Notes: 这一句什么意思
    using variant::variant;
    // Notes: 为什么这样设计
    // 可能的问题: Python变量存的是引用还是值,C++呢
    template <typename T>
        requires std::is_arithmetic_v<T> && (!std::is_same_v<T, bool>)
    inline Value(T v) : variant(static_cast<double>(v)) {}
    inline Value(const Pair &v);
    template <typename T> inline bool isInstance() const {
        return std::holds_alternative<T>(*this);
    }
    template <typename T> inline T asType() const {
        return std::get<T>(*this);
    }
    inline bool isNil() const {
        return isInstance<NilType>();
    };
    inline bool isFalse() const {
        return isInstance<bool>() && asType<bool>() == false;
    }
    inline bool isTrue() const {
        return !isFalse();
    }
    inline bool isPair() const {
        return isInstance<PairPtr>();
    }
    inline bool isList() const;
    inline bool isProcedure() const {
        return isInstance<ProcedurePtr>();
    }
    inline bool isBoolean() const {
        return isInstance<bool>();
    }
    inline bool isNumber() const {
        return isInstance<double>();
    }
    inline bool isString() const {
        if (!isInstance<std::string>()) {
            return false;
        }
        const std::string &str = asType<std::string>();
        return str.starts_with('"') && str.ends_with('"');
    }
    inline bool isSymbol() const {
        return isInstance<std::string>() &&
               !asType<std::string>().starts_with('"');
    }
    inline bool isAtomic() const {
        return isBoolean() || isNumber() || isSymbol() || isNil() || isString();
    };
    // May be wrong
    inline bool isSelfEvaluating() const {
        return (isAtomic() && !isSymbol()); // or expr is None
    }
    //
    inline std::string typeName() const {
        if (isNil()) {
            return "nil";
        }
        if (isList()) {
            return "pair";
        }
        if (isSymbol()) {
            return "symbol";
        }
        if (isNumber()) {
            return "number";
        }
        if (isBoolean()) {
            return "boolean";
        }
        if (isProcedure()) {
            return "procedure";
        }
        throw std::runtime_error("unknown type");
        return "undefined";
    }
    inline std::string str() const;
    inline std::string repr() const;
    //
    inline void validateProcedure() {
        if (!isProcedure()) {
            throw std::runtime_error(
                fmt::format("{} is not callable: {}", typeName(), repr()));
        }
    }
};

struct Pair {
    Value car, cdr;
    Pair() = default;
    inline Pair(Value a, Value b) : car(std::move(a)), cdr(std::move(b)) {}
    inline std::string str() const {
        return cdr.isNil() ? fmt::format("({})", car.str())
                           : fmt::format("({} {})", car.str(), cdr.str());
    }
    inline std::string repr() const {
        return fmt::format("pair({}, {})", car.repr(), cdr.repr());
    }
};
// Notes: 后置实现,解决循环引用的问题
inline Value::Value(const Pair &p) : variant(std::make_shared<Pair>(p)) {}
inline bool Value::isList() const {
    Value x = *this;
    while (!x.isNil()) {
        if (!x.isPair()) {
            return false;
        }
        x = x.asType<PairPtr>()->cdr;
    }
    return true;
}
inline std::string Value::str() const {
    if (isNil()) {
        return "()";
    }
    if (isList()) {
        return asType<PairPtr>()->str();
    }
    if (isString()) {
        return fmt::format("\"{}\"", asType<std::string>());
    }
    if (isSymbol()) {
        return asType<std::string>();
    }
    if (isNumber()) {
        return std::to_string(asType<double>());
    }
    if (isBoolean()) {
        return asType<bool>() ? "#t" : "#f";
    }
    if (isProcedure()) {
        return "#<procedure>";
    }
    throw std::runtime_error("unknown type");
    return "undefined";
}
inline std::string Value::repr() const {
    if (isNil()) {
        return "nil";
    }
    if (isList()) {
        return asType<PairPtr>()->repr();
    }
    if (isSymbol() || isString()) {
        return asType<std::string>();
    }
    if (isNumber()) {
        return std::to_string(asType<double>());
    }
    if (isBoolean()) {
        return asType<bool>() ? "#t" : "#f";
    }
    if (isProcedure()) {
        return "#<procedure>";
    }
    throw std::runtime_error("unknown type");
    return "undefined";
}

struct Frame {
    FramePtr parent;
    std::unordered_map<std::string, Value> bindings;
};

struct Procedure {
    virtual Value apply(const std::vector<Value> &args) = 0;
    virtual ~Procedure() = 0;
};

} // namespace Scheme

template <> struct fmt::formatter<Scheme::Pair> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.end();
    }
    template <typename FormatContext>
    auto format(const Scheme::Pair &val, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", val.str());
    }
};

template <> struct fmt::formatter<Scheme::Value> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.end();
    }
    template <typename FormatContext>
    auto format(const Scheme::Value &val, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", val.str());
    }
};
