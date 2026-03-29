#pragma once
#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <stdexcept>
#include <unordered_map>
#include <fmt/format.h>

#define SCHEME_THROW(msg)                                                                          \
	throw std::runtime_error(fmt::format("Error: in function {}: {}", __PRETTY_FUNCTION__, (msg)));

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

struct NilType : public std::monostate {};
const static NilType nil{};

// Notes: 为什么使用variant
struct Value : public std::variant<double,		 // Number
								   bool,		 // Boolean
								   std::string,	 // Symbol
								   PairPtr,		 // Pair
								   ProcedurePtr, // Functions
								   NilType		 // Nil
								   > {
	// Notes: 这一句什么意思
	using variant::variant;
	// Notes: 为什么这样设计
	// 可能的问题: Python变量存的是引用还是值,C++呢
	/*
	Notes: Value(1.22)
	error: conversion from ‘double’ to ‘Scheme::Value’ is ambiguous
	8 |     global->define("c", 1.0);
	*/
	// Notes: 这个constructor会导致ambiguity
	// template <typename T>
	//     requires std::is_arithmetic_v<T> && (!std::is_same_v<T, bool>)
	// inline Value(T v) : variant(static_cast<double>(v)) {}
	inline Value(const Pair &v);
	template <typename T> inline bool isType() const { return std::holds_alternative<T>(*this); }
	template <typename T> inline T toType() const { return std::get<T>(*this); }
	inline bool isNil() const { return isType<NilType>(); };
	inline bool isFalse() const { return isType<bool>() && toType<bool>() == false; }
	inline bool isTrue() const { return !isFalse(); }
	inline bool isPair() const { return isType<PairPtr>(); }
	inline bool isList() const;
	inline bool isProcedure() const { return isType<ProcedurePtr>(); }
	inline bool isBoolean() const { return isType<bool>(); }
	inline bool isNumber() const { return isType<double>(); }
	inline bool isString() const {
		if (!isType<std::string>()) {
			return false;
		}
		const std::string &str = toType<std::string>();
		return str.starts_with('"') && str.ends_with('"');
	}
	inline bool isSymbol() const {
		return isType<std::string>() && !toType<std::string>().starts_with('"') &&
			   !toType<std::string>().ends_with('"');
	}
	inline bool isAtomic() const {
		return isBoolean() || isNumber() || isSymbol() || isNil() || isString();
	};
	// Conversion
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
		SCHEME_THROW("unknown type");
		return "undefined";
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
	inline std::string str() const {
		return cdr.isNil() ? fmt::format("({})", car.str())
						   : fmt::format("({} {})", car.str(), cdr.str());
	}
	inline std::string repr() const { return fmt::format("pair({}, {})", car.repr(), cdr.repr()); }
};

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
		values.emplace_back(v.toType<PairPtr>()->car);
		v = v.toType<PairPtr>()->cdr;
	}
	return values;
}
template <typename Functor> inline ValueList Value::mapToList(Functor func) const {
	ValueList values;
	Value v = *this;
	while (!v.isNil() && v.isPair()) {
		values.emplace_back(func(v.toType<PairPtr>()->car));
		v = v.toType<PairPtr>()->cdr;
	}
	return values;
}

inline bool Value::isList() const {
	Value x = *this;
	while (!x.isNil()) {
		if (!x.isPair()) {
			return false;
		}
		x = x.toType<PairPtr>()->cdr;
	}
	return true;
}

inline std::string Value::str() const {
	if (isNil()) {
		return "()";
	}
	if (isList()) {
		return toType<PairPtr>()->str();
	}
	if (isSymbol() || isString()) {
		return toType<std::string>();
	}
	if (isNumber()) {
		return std::to_string(toType<double>());
	}
	if (isBoolean()) {
		return toType<bool>() ? "#t" : "#f";
	}
	if (isProcedure()) {
		return "#<procedure>";
	}
	SCHEME_THROW("unknown type");
	return "undefined";
}

inline std::string Value::repr() const {
	if (isNil()) {
		return "nil";
	}
	if (isList()) {
		return toType<PairPtr>()->repr();
	}
	if (isSymbol() || isString()) {
		return toType<std::string>();
	}
	if (isNumber()) {
		return std::to_string(toType<double>());
	}
	if (isBoolean()) {
		return toType<bool>() ? "#t" : "#f";
	}
	if (isProcedure()) {
		return "#<procedure>";
	}
	SCHEME_THROW("unknown type");
	return "undefined";
}

inline size_t Value::size() const {
	size_t len = 0;
	Value v = *this;
	while (!v.isNil()) {
		++len;
		if (v.isType<PairPtr>()) {
			v = v.toType<PairPtr>()->cdr;
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
	inline Value lookUp(Value var) { return lookUp(var.toType<std::string>()); }
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
		// 	auto pv = vals.toType<PairPtr>();
		// 	auto pf = formals.toType<PairPtr>();
		// 	child->bindings[pf->car.toType<std::string>()] = pv->car;
		// 	vals = pv->cdr;
		// 	formals = pf->cdr;
		// }
		for (size_t i = 0; i < formals.size(); ++i) {
			child->bindings[formals[i].toString()] = vals[i];
		}
		return child;
	}
};

} // namespace Scheme
