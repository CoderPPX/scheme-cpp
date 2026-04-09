#pragma once
#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <fmt/format.h>
#include <fmt/ranges.h>

#ifndef SCHEME_ENABLE_STACKTRACE
#define SCHEME_ENABLE_STACKTRACE
#endif

#ifdef SCHEME_ENABLE_STACKTRACE
#include <boost/stacktrace.hpp>
#define SCHEME_THROW(msg)                                                                          \
	do {                                                                                           \
		auto st = boost::stacktrace::stacktrace();                                                 \
		std::string trace = "Traceback (most recent call last):\n";                                \
		for (std::size_t i = 0; i < st.size(); ++i) {                                              \
			trace += fmt::format("\t#{} {}\n", i, st[i].name());                                   \
		}                                                                                          \
		trace += fmt::format("Error: {}", msg);                                                    \
		throw std::runtime_error(trace);                                                           \
	} while (0);
#else
#define SCHEME_THROW(msg)                                                                          \
	throw std::runtime_error(                                                                      \
		fmt::format("Error: in function \"{}\": {}", __PRETTY_FUNCTION__, (msg)));
#endif

namespace Scheme {

struct Pair;
struct Frame;
struct Value;
struct Procedure;
struct BuiltinProcedure;
struct LambdaProcedure;
struct MuProcedure;

using PairPtr = std::shared_ptr<Pair>;
using FramePtr = std::weak_ptr<Frame>;
using ProcedurePtr = std::shared_ptr<Procedure>;

struct Undefined {
	Undefined() = default;
	// Notes: 必须要有 const 标记否则 variant 无法比较
	inline bool operator==(const Undefined &) const { return true; }
};
struct NilType : public std::monostate {};
const static NilType nil{};

// Notes: 防止和 string 混淆
struct Symbol {
	std::string name;
	inline Symbol() = default;
	template <typename T> explicit Symbol(const T &str_) : name(str_) {}
	inline Symbol(const Symbol &other_) : name(other_.name) {}
	inline Symbol(Symbol &&other_) : name(std::move(other_.name)) {}
	inline bool operator==(const Symbol &other_) const { return name == other_.name; }
	inline Symbol &operator=(const Symbol &other_) {
		name = other_.name;
		return *this;
	}
	inline Symbol &operator=(Symbol &&other_) {
		name = std::move(other_.name);
		return *this;
	}
};

// Notes: 为什么使用variant
struct Value : public std::variant<double,		 // Number
								   bool,		 // Boolean
								   std::string,	 // String
								   Symbol,		 // Symbol
								   PairPtr,		 // Pair
								   ProcedurePtr, // Functions
								   NilType,		 // Nil
								   Undefined	 // Undefined
								   > {
	// Notes: 这一句什么意思
	using variant::variant;
	// Notes: 为什么这样设计
	// 可能的问题: Python 变量存的是引用还是值 C++ 呢
	/*
	Notes: Value(1.22)
	error: conversion from ‘double’ to ‘Scheme::Value’ is ambiguous
	8 |     global->define("c", 1.0);
	*/
	// Notes: 这个constructor会导致ambiguity
	template <typename T>
		requires std::is_integral_v<T> && (!std::is_same_v<T, bool>)
	inline Value(T v) : variant(static_cast<double>(v)) {}
	inline Value(float v) : variant(static_cast<double>(v)) {}
	inline Value(const Pair &v);
	inline Value &operator=(const Pair &v) {
		*this = std::make_shared<Pair>(v);
		return *this;
	};
	template <typename T> inline bool isType() const { return std::holds_alternative<T>(*this); }
	template <typename T> inline T toType() const {
		if (isType<T>()) {
			return std::get<T>(*this);
		}
		// DEBUG ONLY
		SCHEME_THROW(
			fmt::format("invalid type conversion from {} to {}", typeName(), typeid(T).name()));
	}
	inline bool isNil() const { return isType<NilType>(); };
	inline bool isFalse() const { return isType<bool>() && toType<bool>() == false; }
	inline bool isTrue() const { return !isFalse(); }
	// Notes: nil 不能是Pair否则 toList 的逻辑会出错
	inline bool isPair() const { return isType<PairPtr>(); }
	inline bool isList() const;
	inline bool isProcedure() const { return isType<ProcedurePtr>(); }
	inline bool isBoolean() const { return isType<bool>(); }
	inline bool isNumber() const { return isType<double>(); }
	inline bool isString() const { return isType<std::string>(); }
	inline bool isSymbol() const { return isType<Symbol>(); }
	inline bool isUndefined() const { return isType<Undefined>(); }
	inline bool isAtomic() const {
		return isBoolean() || isNumber() || isSymbol() || isNil() || isString();
	};
	// Conversion
	inline Symbol toSymbol() const { return toType<Symbol>(); }
	inline std::string toString() const { return toType<std::string>(); }
	inline PairPtr toPair() const;
	inline std::vector<Value> toList() const;
	template <typename Functor = Value(Value)>
	inline std::vector<Value> mapToList(Functor func) const;
	// May be wrong
	inline bool isSelfEvaluating() const {
		return (isAtomic() && !isSymbol()); // or expr is None
	}
	//
	inline std::string typeName() const {
		if (isNil()) {
			return "Nil";
		}
		if (isList() || isPair()) {
			return "Pair";
		}
		if (isSymbol()) {
			return "Symbol";
		}
		if (isString()) {
			return "String";
		}
		if (isNumber()) {
			return "Number";
		}
		if (isBoolean()) {
			return "Boolean";
		}
		if (isProcedure()) {
			return "Procedure";
		}
		if (isUndefined()) {
			return "Undefined";
		}
		SCHEME_THROW("unknown type");
	}
	inline std::string str() const;
	inline std::string repr() const;
	//
	inline void validateProcedure() {
		if (!isProcedure()) {
			SCHEME_THROW(fmt::format("{} is not callable: {}", typeName(), repr()));
		}
	}
	//
	inline size_t size() const;
};
using ValueList = std::vector<Value>;

struct Procedure {
	virtual std::string str() = 0;
	virtual std::string repr() = 0;
	virtual ~Procedure() = 0;
};
inline Procedure::~Procedure() {}

// Notes: 如果在namespace内include会导致嵌套namespace
struct Pair {
	Value car, cdr;
	inline Pair() = default;
	inline Pair(Value a, Value b) : car(std::move(a)), cdr(std::move(b)) {}
	// Notes: Pair如何转换为string
	// 非规范列表是什么
	inline std::string str() const {
		/* test cases:
		Pair(1, nil) -> (1)
		Pair(1, 2) -> (1 . 2)
		Pair(1, Pair(2, nil)) -> (1 2)
		Pair(Pair(1, nil), nil) -> ((1))
		Pair(Pair(1, Pair(2, nil)), Pair(1, nil)) -> ((1 2) 1)
		Pair(nil, nil) -> (())
		Pair(1, Pair(1, 2)) -> (1 (1 . 2))
		Pair(1, Pair(2, Pair(3, 4))) -> (1 2 3 . 4)
		*/
		Value rest = cdr;
		std::string s = '(' + car.str();
		while (rest.isPair()) {
			s += (' ' + rest.toPair()->car.str());
			rest = rest.toPair()->cdr;
		}
		if (!rest.isNil()) {
			s += " . " + rest.str();
		}
		return s + ')';
	}
	inline std::string repr() const { return fmt::format("pair({}, {})", car.repr(), cdr.repr()); }
};

// Notes: auxiliary class
inline Value List() { return nil; }
template <typename T1, typename... Ts> inline Value List(T1 v1, Ts... vs) {
	return Pair(v1, List(vs...));
}

} // namespace Scheme

template <> struct fmt::formatter<Scheme::Pair> {
	constexpr auto parse(format_parse_context &ctx) { return ctx.end(); }
	template <typename FormatContext>
	auto format(const Scheme::Pair &val, FormatContext &ctx) const {
		return fmt::format_to(ctx.out(), "{}", val.str());
	}
};
template <> struct fmt::formatter<Scheme::Value> {
	constexpr auto parse(format_parse_context &ctx) { return ctx.end(); }
	template <typename FormatContext>
	auto format(const Scheme::Value &val, FormatContext &ctx) const {
		return fmt::format_to(ctx.out(), "{}", val.str());
	}
};

namespace Scheme {

// Notes: 后置实现,解决循环引用的问题
inline Value::Value(const Pair &p) : variant(std::make_shared<Pair>(p)) {}

inline PairPtr Value::toPair() const { return toType<PairPtr>(); }
inline ValueList Value::toList() const {
	ValueList values;
	Value v = *this;
	while (!v.isNil() && v.isPair()) {
		values.emplace_back(v.toPair()->car);
		v = v.toPair()->cdr;
	}
	return values;
}
template <typename Functor> inline ValueList Value::mapToList(Functor func) const {
	ValueList values;
	Value v = *this;
	while (!v.isNil() && v.isPair()) {
		values.emplace_back(func(v.toPair()->car));
		v = v.toPair()->cdr;
	}
	return values;
}

inline bool Value::isList() const {
	Value x = *this;
	while (!x.isNil()) {
		if (!x.isPair()) {
			return false;
		}
		x = x.toPair()->cdr;
	}
	return true;
}

std::string Value::str() const {
	if (isNil()) {
		return "()";
	}
	if (isList() || isPair()) {
		return toPair()->str();
	}
	if (isSymbol()) {
		return toSymbol().name;
	}
	if (isString()) {
		return toString();
	}
	if (isNumber()) {
		return std::to_string(toType<double>());
	}
	if (isBoolean()) {
		return toType<bool>() ? "#t" : "#f";
	}
	if (isProcedure()) {
		return toType<ProcedurePtr>()->str();
	}
	if (isUndefined()) {
		return "<undefined>";
	}
	SCHEME_THROW("unknown type");
}

inline std::string Value::repr() const {
	if (isNil()) {
		return "nil";
	}
	if (isList() || isPair()) {
		return toPair()->repr();
	}
	if (isSymbol()) {
		return toSymbol().name;
	}
	if (isString()) {
		return fmt::format(R"("{}")", toType<std::string>());
	}
	if (isNumber()) {
		return std::to_string(toType<double>());
	}
	if (isBoolean()) {
		return toType<bool>() ? "#t" : "#f";
	}
	if (isProcedure()) {
		return toType<ProcedurePtr>()->repr();
	}
	if (isUndefined()) {
		return "<undefined>";
	}
	SCHEME_THROW("unknown type");
}

inline size_t Value::size() const {
	size_t len = 0;
	Value v = *this;
	while (!v.isNil()) {
		++len;
		if (v.isPair()) {
			v = v.toPair()->cdr;
		} else {
			break;
		}
	}
	return len;
}

struct Frame : public std::enable_shared_from_this<Frame> {
	// Notes: 为什么使用weak_ptr
	FramePtr parent;
	std::unordered_map<std::string, Value> bindings;
	// Create a global frame
	inline Frame();
	inline Frame(FramePtr parent_) : parent(parent_) {}
	inline void define(const std::string &symbol, Value v) { bindings[symbol] = v; }
	inline Value lookUp(Value var) { return lookUp(var.toSymbol().name); }
	inline Value lookUp(const std::string &name) {
		if (bindings.contains(name)) {
			return bindings[name];
		}
		if (auto p = parent.lock()) {
			return p->lookUp(name);
		}
		SCHEME_THROW("undefined variable: " + name);
	}
	std::shared_ptr<Frame> makeChildFrame(const ValueList &formals, const ValueList &vals) {
		if (formals.size() != vals.size()) {
			SCHEME_THROW("incorrect number of arguments to function call");
		}
		/* Notes: 如果标记为const则报错
		error: no matching function for call to ‘construct_at(Scheme::Frame*&,
		std::weak_ptr<const Scheme::Frame>)’ 115 | std::construct_at(__p,
		std::forward<_Args>(__args)...);
		*/
		auto child = std::make_shared<Frame>(weak_from_this());
		// while (!formals.isNil() && !vals.isNil()) {
		// 	auto pv = vals.toPair();
		// 	auto pf = formals.toPair();
		// 	child->bindings[pf->car.toType<std::string>()] = pv->car;
		// 	vals = pv->cdr;
		// 	formals = pf->cdr;
		// }
		for (size_t i = 0; i < formals.size(); ++i) {
			child->bindings[formals[i].toSymbol().name] = vals[i];
		}
		return child;
	}
};

} // namespace Scheme
