#include "values.h"
#include "util/vec.h"
#include "env.h"
#include "eval.h"
#include "ast.h"
#include "native.h"

namespace basil {
  static map<string, u64> symbol_table;
  static vector<string> symbol_array;

  u64 symbol_value(const string& symbol) {
    static u64 count = 0;
    auto it = symbol_table.find(symbol);
    if (it == symbol_table.end()) {
      symbol_table.put(symbol, count++);
      symbol_array.push(symbol);
      return count - 1;
    }
    else return it->second;
  }
  
  const string& symbol_for(u64 value) {
    return symbol_array[value];
  }

  Value::Value(): Value(VOID) {}

  Value::Value(const Type* type):
    _type(type) {}

  Value::Value(i64 i, const Type* type):
    _type(type) {
    is_bool() ? _data.b = i : _data.i = i;
  }

  Value::Value(const string& s, const Type* type):
    _type(type) {
    if (type == SYMBOL) _data.u = symbol_value(s);
		else if (type == STRING) _data.rc = new StringValue(s);
  }

  Value::Value(const Type* type_value, const Type* type):
    _type(type) {
    _data.t = type_value;
  }

  Value::Value(ListValue* l):
    _type(find<ListType>(l->head().type())) {
    _data.rc = l;
  }

  Value::Value(SumValue* s, const Type* type):
    _type(type) {
    _data.rc = s;
  }

  Value::Value(ProductValue* p) {
    vector<const Type*> ts;
    for (const Value& v : *p) ts.push(v.type());
    _type = find<ProductType>(ts);
    _data.rc = p;
  }

  Value::Value(FunctionValue* f):
    _type(find<FunctionType>(find<TypeVariable>(), find<TypeVariable>())) {
    _data.rc = f;
  }

  Value::Value(AliasValue* a):
    _type(ALIAS) {
    _data.rc = a;
  }

  Value::Value(MacroValue* m):
    _type(find<MacroType>(m->arity())) {
    _data.rc = m;
  }

	Value::Value(ASTNode* n):
		_type(find<RuntimeType>(n->type())) {
		_data.rc = n;
	}

  Value::~Value() {
    if (_type->kind() & GC_KIND_FLAG) _data.rc->dec();
  }
  
  Value::Value(const Value& other):
    _type(other._type), _loc(other._loc) {
    _data.u = other._data.u; // copy over raw data
    if (_type->kind() & GC_KIND_FLAG) _data.rc->inc();
  }

  Value& Value::operator=(const Value& other) {
		if (other.type()->kind() & GC_KIND_FLAG) other._data.rc->inc();
    if (_type->kind() & GC_KIND_FLAG) _data.rc->dec();
    _type = other._type;
    _loc = other._loc;
    _data.u = other._data.u; // copy over raw data
    return *this;
  }

  bool Value::is_int() const {
    return _type == INT;
  }

  i64 Value::get_int() const {
    return _data.i;
  }

  i64& Value::get_int() {
    return _data.i;
  }

  bool Value::is_symbol() const {
    return _type == SYMBOL;
  }

  u64 Value::get_symbol() const {
    return _data.u;
  }

  u64& Value::get_symbol() {
    return _data.u;
  }

	bool Value::is_string() const {
		return _type == STRING;
	}

	const string& Value::get_string() const {
		return ((const StringValue*)_data.rc)->value();
	}

	string& Value::get_string() {
		return ((StringValue*)_data.rc)->value();
	}

  bool Value::is_void() const {
    return _type == VOID;
  }

  bool Value::is_error() const {
    return _type == ERROR;
  }

  bool Value::is_type() const {
    return _type == TYPE;
  }

  const Type* Value::get_type() const {
    return _data.t;
  }

  const Type*& Value::get_type() {
    return _data.t;
  }

  bool Value::is_bool() const {
    return _type == BOOL;
  }

  bool Value::get_bool() const {
    return _data.b;
  }

  bool& Value::get_bool() {
    return _data.b;
  }

  bool Value::is_list() const {
    return _type->kind() == KIND_LIST;
  }

  const ListValue& Value::get_list() const {
    return *(const ListValue*)_data.rc;
  }

  ListValue& Value::get_list() {
    return *(ListValue*)_data.rc;
  }

  bool Value::is_sum() const {
    return _type->kind() == KIND_SUM;
  }

  const SumValue& Value::get_sum() const {
    return *(const SumValue*)_data.rc;
  }

  SumValue& Value::get_sum() {
    return *(SumValue*)_data.rc;
  }

  bool Value::is_product() const {
    return _type->kind() == KIND_PRODUCT;
  }

  const ProductValue& Value::get_product() const {
    return *(const ProductValue*)_data.rc;
  }

  ProductValue& Value::get_product() {
    return *(ProductValue*)_data.rc;
  }

  bool Value::is_function() const {
    return _type->kind() == KIND_FUNCTION;
  }

  const FunctionValue& Value::get_function() const {
    return *(const FunctionValue*)_data.rc;
  }

  FunctionValue& Value::get_function() {
    return *(FunctionValue*)_data.rc;
  } 

  bool Value::is_alias() const {
    return _type->kind() == KIND_ALIAS;
  }

  const AliasValue& Value::get_alias() const {
    return *(const AliasValue*)_data.rc;
  }

  AliasValue& Value::get_alias() {
    return *(AliasValue*)_data.rc;
  } 

  bool Value::is_macro() const {
    return _type->kind() == KIND_MACRO;
  }

  const MacroValue& Value::get_macro() const {
    return *(const MacroValue*)_data.rc;
  }

  MacroValue& Value::get_macro() {
    return *(MacroValue*)_data.rc;
  } 

	bool Value::is_runtime() const {
		return _type->kind() == KIND_RUNTIME;
	}

	const ASTNode* Value::get_runtime() const {
		return (ASTNode*)_data.rc;
	}
	
	ASTNode* Value::get_runtime() {
		return (ASTNode*)_data.rc;
	}

  const Type* Value::type() const {
    return _type;
  }

  void Value::format(stream& io) const {
    if (is_void()) write(io, "()");
    else if (is_error()) write(io, "error");
    else if (is_int()) write(io, get_int());
    else if (is_symbol()) write(io, symbol_for(get_symbol()));
		else if (is_string()) write(io, '"', get_string(), '"');
    else if (is_type()) write(io, get_type());
    else if (is_bool()) write(io, get_bool());
    else if (is_list()) {
      bool first = true;
      write(io, "(");
      const Value* ptr = this;
      while (ptr->is_list()) {
        write(io, first ? "" : " ", ptr->get_list().head());
        ptr = &ptr->get_list().tail();
        first = false;
      }
      write(io, ")");
    }
    else if (is_sum()) write(io, get_sum().value());
    else if (is_product()) {
      bool first = true;
      write(io, "(");
      for (const Value& v : get_product()) {
        write(io, first ? "" : ", ", v);
        first = false;
      }
      write(io, ")");
    }
    else if (is_function()) write(io, "<#procedure>");
    else if (is_alias()) write(io, "<#alias>");
    else if (is_macro()) write(io, "<#macro>");
		else if (is_runtime()) 
			write(io, "<#runtime ", ((const RuntimeType*)_type)->base(), ">");
  }

  u64 Value::hash() const {
    if (is_void()) return 11103515024943898793ul;
    else if (is_error()) return 14933118315469276343ul;
    else if (is_int()) return ::hash(get_int()) ^ 6909969109598810741ul;
    else if (is_symbol()) return ::hash(get_symbol()) ^ 1899430078708870091ul;
		else if (is_string()) return ::hash(get_string()) ^ 1276873522146073541ul;
    else if (is_type()) return get_type()->hash();
    else if (is_bool()) 
      return get_bool() ? 9269586835432337327ul
        : 18442604092978916717ul;
    else if (is_list()) {
      u64 h = 9572917161082946201ul;
      Value ptr = *this;
      while (ptr.is_list()) {
        h ^= ptr.get_list().head().hash();
        ptr = ptr.get_list().tail();
      }
      return h;
    }
    else if (is_sum()) {
      return get_sum().value().hash() ^ 7458465441398727979ul;
    }
    else if (is_product()) {
      u64 h = 16629385277682082909ul;
      for (const Value& v : get_product()) h ^= v.hash();
      return h;
    }
    else if (is_function()) {
      u64 h = 10916307465547805281ul;
      if (get_function().is_builtin())
        h ^= ::hash(get_function().get_builtin());
      else {
        h ^= get_function().body().hash();
        for (u64 arg : get_function().args())
          h ^= ::hash(arg);
      }
      return h;
    }
    else if (is_alias()) return 6860110315984869641ul;
    else if (is_macro()) {
      u64 h = 16414641732770006573ul;
      if (get_macro().is_builtin())
        h ^= ::hash(get_macro().get_builtin());
      else {
        h ^= get_macro().body().hash();
        for (u64 arg : get_macro().args())
          h ^= ::hash(arg);
      }
      return h;
    }
		else if (is_runtime()) {
			return _type->hash() ^ ::hash(_data.rc);
		}
    return 0;
  }
  
  bool Value::operator==(const Value& other) const {
		if (type() != other.type()) return false;
    else if (is_int()) return get_int() == other.get_int();
    else if (is_symbol()) return get_symbol() == other.get_symbol();
    else if (is_type()) return get_type() == other.get_type();
    else if (is_bool()) return get_bool() == other.get_bool();
		else if (is_string()) return get_string() == other.get_string();
    else if (is_list()) {
      const Value* l = this, *o = &other;
      while (l->is_list() && o->is_list()) {
				if (l->get_list().head() != o->get_list().head()) return false;
        l = &l->get_list().tail(), o = &o->get_list().tail();
      }
      return l->is_void() && o->is_void();
    }
    else if (is_function()) {
      if (get_function().is_builtin())
        return get_function().get_builtin() == 
          other.get_function().get_builtin();
      else {
        if (other.get_function().arity() != get_function().arity())
          return false;
        for (u32 i = 0; i < get_function().arity(); i ++) {
          if (other.get_function().args()[i] !=
              get_function().args()[i]) return false;
        }
        return get_function().body() == other.get_function().body();
      }
    }
    else if (is_macro()) {
      if (get_macro().is_builtin())
        return get_macro().get_builtin() == 
          other.get_macro().get_builtin();
      else {
        if (other.get_macro().arity() != get_macro().arity())
          return false;
        for (u32 i = 0; i < get_macro().arity(); i ++) {
          if (other.get_macro().args()[i] !=
              get_macro().args()[i]) return false;
        }
        return get_macro().body() == other.get_macro().body();
      }
    }
		else if (is_runtime()) {
			return _data.rc == other._data.rc;
		}
    return type() == other.type();
  }

  Value Value::clone() const {
    if (is_list())
      return Value(new ListValue(get_list().head().clone(),
        get_list().tail().clone()));
		else if (is_string())
			return Value(get_string(), STRING);
    else if (is_sum())
      return Value(new SumValue(get_sum().value()), type());
    else if (is_product()) {
      vector<Value> values;
      for (const Value& v : get_product()) values.push(v);
      return Value(new ProductValue(values));
    }
    else if (is_function()) {
      if (get_function().is_builtin()) {
        return Value(new FunctionValue(get_function().get_env()->clone(),
          get_function().get_builtin(), get_function().arity()));
      }
      else {
        return Value(new FunctionValue(get_function().get_env()->clone(),
          get_function().args(), get_function().body().clone()));
      }
    }
    else if (is_alias())
      return Value(new AliasValue(get_alias().value()));
    else if (is_macro()) {
      if (get_macro().is_builtin()) {
        return Value(new MacroValue(get_macro().get_env()->clone(),
          get_macro().get_builtin(), get_macro().arity()));
      }
      else {
        return Value(new MacroValue(get_macro().get_env()->clone(),
          get_macro().args(), get_macro().body().clone()));
      }
    }
		else if (is_runtime()) {
			// todo: ast cloning
		}
    return *this;
  }

  bool Value::operator!=(const Value& other) const {
    return !(*this == other);
  }

  void Value::set_location(SourceLocation loc) {
    _loc = loc;
  }

  SourceLocation Value::loc() const {
    return _loc;
  }

	StringValue::StringValue(const string& value):
		_value(value) {}

	string& StringValue::value() {
		return _value;
	}

	const string& StringValue::value() const {
		return _value;
	}

  ListValue::ListValue(const Value& head, const Value& tail) : 
    _head(head), _tail(tail) {}

  Value& ListValue::head() {
    return _head;
  }
  
  const Value& ListValue::head() const {
    return _head;
  }

  Value& ListValue::tail() {
    return _tail;
  }
  
  const Value& ListValue::tail() const {
    return _tail;
  }

  SumValue::SumValue(const Value& value):
    _value(value) {}

  Value& SumValue::value() {
    return _value;
  }

  const Value& SumValue::value() const {
    return _value;
  }

  ProductValue::ProductValue(const vector<Value>& values):
    _values(values) {}

  u32 ProductValue::size() const {
    return _values.size();
  }

  Value& ProductValue::operator[](u32 i) {
    return _values[i];
  }

  const Value& ProductValue::operator[](u32 i) const {
    return _values[i];
  }

  const Value* ProductValue::begin() const {
    return _values.begin();
  }

  const Value* ProductValue::end() const {
    return _values.end();
  }
  
  Value* ProductValue::begin() {
    return _values.begin();
  }
  
  Value* ProductValue::end() {
    return _values.end();
  }

	const u64 KEYWORD_ARG_BIT = 1ul << 63;
	const u64 ARG_NAME_MASK = ~KEYWORD_ARG_BIT;

  FunctionValue::FunctionValue(ref<Env> env, const vector<u64>& args,
    const Value& code, i64 name):
    _name(name), _code(code), _builtin(nullptr), 
		_env(env), _args(args), _insts(nullptr), _calls(nullptr) {}

  FunctionValue::FunctionValue(ref<Env> env, BuiltinFn builtin, 
    u64 arity, i64 name):
    _name(name), _builtin(builtin), _env(env), 
		_builtin_arity(arity), _insts(nullptr), _calls(nullptr) {}

	FunctionValue::~FunctionValue() {
		if (_insts) {
			for (auto& p : *_insts) p.second->dec();
			delete _insts;
		}
		if (_calls) {
			for (auto& p : *_calls) ((FunctionValue*)p)->dec();
			delete _calls;
		}
	}

	FunctionValue::FunctionValue(const FunctionValue& other):
		_name(other._name), _code(other._code.clone()), 
		_builtin(other._builtin), _env(other._env), _args(other._args),
		_insts(other._insts ? 
			new map<const Type*, ASTNode*>(*other._insts) 
			: nullptr),
		_calls(other._calls ?
			new set<const FunctionValue*>(*other._calls) : nullptr) {
		if (_insts) for (auto& p : *_insts) p.second->inc();
		if (_calls) for (auto p : *_calls) ((FunctionValue*)p)->inc();
	}
	
	FunctionValue& FunctionValue::operator=(const FunctionValue& other) {
		if (this != &other) {
			if (_insts) {
				for (auto& p : *_insts) p.second->dec();
				delete _insts;
			}
			if (_calls) {
				for (auto& p : *_calls) ((FunctionValue*)p)->dec();
				delete _calls;
			}
			_name = other._name;
			_code = other._code.clone();
			_builtin = other._builtin;
			_env = other._env;
			_args = other._args;
			_insts = other._insts ?
				new map<const Type*, ASTNode*>(*other._insts)
				: nullptr;
			_calls = other._calls ?
				new set<const FunctionValue*>(*other._calls) : nullptr;
			if (_insts) for (auto& p : *_insts) p.second->inc();
			if (_calls) for (auto& p : *_calls) ((FunctionValue*)p)->inc();
		}
		return *this;
	}

  const vector<u64>& FunctionValue::args() const {
    return _args;
  }

  bool FunctionValue::is_builtin() const {
    return _builtin;
  }

  BuiltinFn FunctionValue::get_builtin() const {
    return _builtin;
  }

  ref<Env> FunctionValue::get_env() {
    return _env;
  }

  const ref<Env> FunctionValue::get_env() const {
    return _env;
  }

	i64 FunctionValue::name() const {
		return _name;
	}

	bool FunctionValue::found_calls() const {
		return _calls;
	}

	bool FunctionValue::recursive() const {
		return _calls && _calls->find(this) != _calls->end();
	}

	void FunctionValue::add_call(const FunctionValue* other) {
		if (!_calls) _calls = new set<const FunctionValue*>();
		if (other != this && other->_calls) 
			for (const FunctionValue* f : *other->_calls) {
			_calls->insert(f);
			((FunctionValue*)f)->inc(); // evil
		}
		_calls->insert(other);
		((FunctionValue*)other)->inc();
	}

	ASTNode* FunctionValue::instantiation(const Type* type) const {
		if (_insts) {
			auto it = _insts->find(type);
			if (it != _insts->end()) return it->second;
		}
		return nullptr;
	}

	void FunctionValue::instantiate(const Type* type, ASTNode* body) {
		if (!_insts) _insts = new map<const Type*, ASTNode*>();
		auto it = _insts->find(type);
		if (it == _insts->end()) {
			_insts->put(type, body);
		}
		else {
			it->second->dec();
			it->second = body;
		}
		body->inc();
	}

  u64 FunctionValue::arity() const {
    return _builtin ? _builtin_arity : _args.size();
  }

  const Value& FunctionValue::body() const {
    return _code;
  }

  AliasValue::AliasValue(const Value& value):
    _value(value) {}

  Value& AliasValue::value() {
    return _value;
  }

  const Value& AliasValue::value() const {
    return _value;
  }

  MacroValue::MacroValue(ref<Env> env, const vector<u64>& args,
    const Value& code):
    _code(code), _builtin(nullptr), _env(env), _args(args) {}

  MacroValue::MacroValue(ref<Env> env, BuiltinMacro builtin, 
    u64 arity):
    _builtin(builtin), _env(env), _builtin_arity(arity) {}

  const vector<u64>& MacroValue::args() const {
    return _args;
  }

  bool MacroValue::is_builtin() const {
    return _builtin;
  }

  BuiltinMacro MacroValue::get_builtin() const {
    return _builtin;
  }

  ref<Env> MacroValue::get_env() {
    return _env;
  }

  const ref<Env> MacroValue::get_env() const {
    return _env;
  }

  u64 MacroValue::arity() const {
    return _builtin ? _builtin_arity : _args.size();
  }

  const Value& MacroValue::body() const {
    return _code;
  }
	
	vector<Value> to_vector(const Value& list) {
    vector<Value> values;
    const Value* v = &list;
    while (v->is_list()) {
      values.push(v->get_list().head());
      v = &v->get_list().tail();
    }
    return values;
  }

	Value lower(const Value& v) {
		if (v.is_runtime()) return v;
		else if (v.is_void()) return new ASTVoid(v.loc());
		else if (v.is_int()) 
			return new ASTInt(v.loc(), v.get_int());
		else if (v.is_symbol()) 
			return new ASTSymbol(v.loc(), v.get_symbol());
		else if (v.is_string())
			return new ASTString(v.loc(), v.get_string());
		else if (v.is_bool()) 
			return new ASTBool(v.loc(), v.get_bool());
		else if (v.is_list()) {
			vector<Value> vals = to_vector(v);
			ASTNode* acc = new ASTVoid(v.loc());
			for (i64 i = vals.size() - 1; i >= 0; i --) {
				Value l = lower(vals[i]);
				acc = new ASTCons(v.loc(), l.get_runtime(), acc);
			}
			return acc;
		}
		else if (v.is_error()) 
			return new ASTSingleton(ERROR);
		else {
			err(v.loc(), "Couldn't lower value '", v, "'.");
			return error();
		}
	}

  Value binary_arithmetic(const Value& lhs, const Value& rhs, 
		i64(*op)(i64, i64)) {
    if (!lhs.is_int() && !lhs.is_error()) {
      err(lhs.loc(), "Expected integer value in arithmetic expression, found '",
          lhs.type(), "'.");
      return error();
    }
    if (!rhs.is_int() && !lhs.is_error()) {
      err(rhs.loc(), "Expected integer value in arithmetic expression, found '",
          rhs.type(), "'.");
      return error();
    }
    if (lhs.is_error() || rhs.is_error()) return error();

    return Value(op(lhs.get_int(), rhs.get_int()));
  }

	bool is_runtime_binary(const Value& lhs, const Value& rhs) {
		return lhs.is_runtime() || rhs.is_runtime();
	}

	Value lower(ASTMathOp op, const Value& lhs, const Value& rhs) {
		return new ASTBinaryMath(lhs.loc(), op, lower(lhs).get_runtime(),
			lower(rhs).get_runtime());
	}
  
  Value add(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_ADD, lhs, rhs);
    return binary_arithmetic(lhs, rhs, [](i64 a, i64 b) -> i64 { return a + b; });
  }

  Value sub(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_SUB, lhs, rhs);
    return binary_arithmetic(lhs, rhs, [](i64 a, i64 b) -> i64 { return a - b; });
  }

  Value mul(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_MUL, lhs, rhs);
    return binary_arithmetic(lhs, rhs, [](i64 a, i64 b) -> i64 { return a * b; });
  }

  Value div(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_DIV, lhs, rhs);
    return binary_arithmetic(lhs, rhs, [](i64 a, i64 b) -> i64 { return a / b; });
  }

  Value rem(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_REM, lhs, rhs);
    return binary_arithmetic(lhs, rhs, [](i64 a, i64 b) -> i64 { return a % b; });
  }

  Value binary_logic(const Value& lhs, const Value& rhs, bool(*op)(bool, bool)) {
    if (!lhs.is_bool() && !lhs.is_error()) {
      err(lhs.loc(), "Expected boolean value in logical expression, found '",
          lhs.type(), "'.");
      return error();
    }
    if (!rhs.is_bool() && !lhs.is_error()) {
      err(rhs.loc(), "Expected boolean value in logical expression, found '",
          rhs.type(), "'.");
      return error();
    }
    if (lhs.is_error() || rhs.is_error()) return error();

    return Value(op(lhs.get_bool(), rhs.get_bool()), BOOL);
  }

	Value lower(ASTLogicOp op, const Value& lhs, const Value& rhs) {
		return new ASTBinaryLogic(lhs.loc(), op, lower(lhs).get_runtime(),
			lower(rhs).get_runtime());
	}

  Value logical_and(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_AND, lhs, rhs);
    return binary_logic(lhs, rhs, [](bool a, bool b) -> bool { return a && b; });
  }
  
  Value logical_or(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_OR, lhs, rhs);
    return binary_logic(lhs, rhs, [](bool a, bool b) -> bool { return a || b; });
  }
  
  Value logical_xor(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_XOR, lhs, rhs);
    return binary_logic(lhs, rhs, [](bool a, bool b) -> bool { return a ^ b; });
  }

  Value logical_not(const Value& v) {
		if (v.is_runtime()) return new ASTNot(v.loc(), lower(v).get_runtime());

    if (!v.is_bool() && !v.is_error()) {
      err(v.loc(), "Expected boolean value in logical expression, found '",
        v.type(), "'.");
      return error();
    }
    if (v.is_error()) return error();

    return Value(!v.get_bool(), BOOL);
  }

	Value lower(ASTEqualOp op, const Value& lhs, const Value& rhs) {
		return new ASTBinaryEqual(lhs.loc(), op, lower(lhs).get_runtime(),
			lower(rhs).get_runtime());
	}

  Value equal(const Value& lhs, const Value& rhs) {
    if (lhs.is_error() || rhs.is_error()) return error();
		if (is_runtime_binary(lhs, rhs)) return lower(AST_EQUAL, lhs, rhs);
    return Value(lhs == rhs, BOOL);
  }
  
  Value inequal(const Value& lhs, const Value& rhs) {
    if (lhs.is_error() || rhs.is_error()) return error();
		if (is_runtime_binary(lhs, rhs)) return lower(AST_INEQUAL, lhs, rhs);
    return Value(!equal(lhs, rhs).get_bool(), BOOL);
  }

  Value binary_relation(const Value& lhs, const Value& rhs, bool(*int_op)(i64, i64), bool(*string_op)(string, string)) {
    if (!lhs.is_int() && !lhs.is_string() && !lhs.is_error()) {
      err(lhs.loc(), "Expected integer or string value in relational expression, found '",
          lhs.type(), "'.");
      return error();
    }
    if (!rhs.is_int() && !rhs.is_string() && !lhs.is_error()) {
      err(rhs.loc(), "Expected integer or string value in relational expression, found '",
          rhs.type(), "'.");
      return error();
    }
    if ((lhs.is_int() && !rhs.is_int()) || (lhs.is_string() && !rhs.is_string())) {
      err(rhs.loc(), "Invalid parameters to relational expression: '", lhs.type(), "' and '", rhs.type(), "'.");
      return error();
    }
    if (lhs.is_error() || rhs.is_error()) return error();

    if (lhs.is_string()) return Value(string_op(lhs.get_string(), rhs.get_string()), BOOL);
    return Value(int_op(lhs.get_int(), rhs.get_int()), BOOL);
  }

	Value lower(ASTRelOp op, const Value& lhs, const Value& rhs) {
		return new ASTBinaryRel(lhs.loc(), op, lower(lhs).get_runtime(),
			lower(rhs).get_runtime());
	}

  Value less(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_LESS, lhs, rhs);
    return binary_relation(lhs, rhs, [](i64 a, i64 b) -> bool { return a < b; }, [](string a, string b) -> bool { return a < b; });
  }

  Value greater(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_GREATER, lhs, rhs);
    return binary_relation(lhs, rhs, [](i64 a, i64 b) -> bool { return a > b; }, [](string a, string b) -> bool { return a > b; });
  }

  Value less_equal(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_LESS_EQUAL, lhs, rhs);
    return binary_relation(lhs, rhs, [](i64 a, i64 b) -> bool { return a <= b; }, [](string a, string b) -> bool { return a <= b; });
  }

  Value greater_equal(const Value& lhs, const Value& rhs) {
		if (is_runtime_binary(lhs, rhs)) return lower(AST_GREATER_EQUAL, lhs, rhs);
    return binary_relation(lhs, rhs, [](i64 a, i64 b) -> bool { return a >= b; }, [](string a, string b) -> bool { return a >= b; });
  }

  Value head(const Value& v) {
		if (v.is_runtime()) return new ASTHead(v.loc(), lower(v).get_runtime());
    if (!v.is_list() && !v.is_error()) {
      err(v.loc(), "Can only get head of value of list type, given '",
          v.type(), "'.");
      return error();
    }
    if (v.is_error()) return error();

    return v.get_list().head();
  }

  Value tail(const Value& v) {
		if (v.is_runtime()) return new ASTTail(v.loc(), lower(v).get_runtime());
    if (!v.is_list() && !v.is_error()) {
      err(v.loc(), "Can only get tail of value of list type, given '",
          v.type(), "'.");
      return error();
    }
    if (v.is_error()) return error();

    return v.get_list().tail();
  }

  Value cons(const Value& head, const Value& tail) {
		if (head.is_runtime() || tail.is_runtime()) {
			return new ASTCons(head.loc(), lower(head).get_runtime(), 
				lower(tail).get_runtime());
		}
    if (!tail.is_list() && !tail.is_void() && !tail.is_error()) {
      err(tail.loc(), "Tail of cons cell must be a list or void, given '",
          tail.type(), "'.");
      return error();
    }
    if (head.is_error() || tail.is_error()) return error();
    
    return Value(new ListValue(head, tail));
  }

  Value empty() {
    return Value(VOID);
  }

  Value list_of(const Value& element) {
    if (element.is_error()) return error();
    return cons(element, empty());
  }

  Value list_of(const vector<Value>& elements) {
    Value l = empty();
    for (i64 i = i64(elements.size()) - 1; i >= 0; i --) {
      l = cons(elements[i], l);
    }
    return l;
  }

	Value is_empty(const Value& list) {
		if (list.is_runtime()) 
			return new ASTIsEmpty(list.loc(), lower(list).get_runtime());
    if (!list.is_list() && !list.is_void() && !list.is_error()) {
      err(list.loc(), "Can only get tail of value of list type, given '",
          list.type(), "'.");
      return error();
    }
		return Value(list.is_void(), BOOL);
	}

  Value error() {
    return Value(ERROR);
  }

  Value length(const Value& val) {
		if (val.is_error()) return error();

    if (val.is_runtime())
      return new ASTLength(val.loc(), lower(val).get_runtime());
    
    if (!val.is_string() && !val.is_list()) {
      err(val.loc(), "Expected string or list, given '", val.type(), "'.");
      return error();
    }

		if (val.is_string()) return Value(i64(val.get_string().size()));
		else return Value(i64(to_vector(val).size()));
  }

  Value char_at(const Value& str, const Value& idx) {
    if (str.is_runtime() || idx.is_runtime()) {
      vector<ASTNode*> args;
      Value s = lower(str), i = lower(idx);
      args.push(s.get_runtime());
      args.push(i.get_runtime());
      vector<const Type*> arg_types;
		  arg_types.push(STRING);
      arg_types.push(INT);
      return new ASTNativeCall(str.loc(), "_char_at", INT, args, arg_types);
    }
    if (!str.is_string()) {
      err(str.loc(), "Expected string, given '", str.type(), "'.");
      return error();
    }
    if (!idx.is_int()) {
      err(idx.loc(), "Expected integer to index string, given '", str.type(), "'.");
      return error();
    }
    return Value(i64(str.get_string()[idx.get_int()]));
  }

  Value type_of(const Value& v) {
    return Value(v.type(), TYPE);
  }

	ASTNode* instantiate(SourceLocation loc, FunctionValue& fn, 
		const Type* args_type) {
		ref<Env> new_env = fn.get_env()->clone();
		new_env->make_runtime();
		u32 j = 0;
		vector<u64> new_args;
		for (u32 i = 0; i < fn.arity(); i ++) {
			if (!(fn.args()[i] & KEYWORD_ARG_BIT)) {
				auto it = new_env->find(symbol_for(fn.args()[i] & ARG_NAME_MASK));
				const Type* argt = ((const ProductType*)args_type)->member(j);
				it->value = new ASTSingleton(argt);
				j ++;
				new_args.push(fn.args()[i]);
			}
		}
		Value cloned = fn.body().clone();
		Value v = eval(new_env, cloned);
		if (v.is_error()) return nullptr;
		if (!v.is_runtime()) v = lower(v);
		ASTNode* result = new ASTFunction(loc, new_env, args_type, 
			new_args, v.get_runtime(), fn.name());
		fn.instantiate(args_type, result);
		return result;
	}

	void find_calls(FunctionValue& fn, ref<Env> env, const Value& term,
		set<const FunctionValue*>& visited) {
		if (!term.is_list()) return;
		Value h = head(term);
		if (h.is_symbol()) {
			Def* def = env->find(symbol_for(h.get_symbol()));
			if (def && def->value.is_function()) {
				FunctionValue* f = &def->value.get_function();
				if (visited.find(f) == visited.end()) {
					visited.insert(f);
					if (f != &fn) find_calls(*f, f->get_env(), f->body(), visited);
					fn.add_call(f);
				}
			}
		}
		if (!introduces_env(term)) {
			const Value* v = &term;
			while (v->is_list()) {
				find_calls(fn, env, v->get_list().head(), visited);
				v = &v->get_list().tail();
			}
		}
	}

  Value call(ref<Env> env, Value& function, const Value& arg) {
		if (function.is_runtime()) {
      u32 argc = arg.get_product().size();
			vector<const Type*> argts;
			vector<Value> lowered_args;
			for (u32 i = 0; i < argc; i ++) {
				if (arg.get_product()[i].is_function()) {
					vector<const Type*> inner_argts;
					for (u32 j = 0; j < arg.get_product()[i].get_function().arity(); j ++)
						inner_argts.push(find<TypeVariable>());
					argts.push(find<FunctionType>(
						find<ProductType>(inner_argts), find<TypeVariable>()));
					lowered_args.push(arg.get_product()[i]); // we'll lower this later
				}
				else {
					Value lowered = lower(arg.get_product()[i]);
					argts.push(((const RuntimeType*)lowered.type())->base());
					lowered_args.push(lowered);
				}
			}
			const Type* argt = find<ProductType>(argts);
			vector<ASTNode*> arg_nodes;
			for (u32 i = 0; i < lowered_args.size(); i ++) {
				if (lowered_args[i].is_function()) {
					const Type* t = ((const ProductType*)argt)->member(i);
					if (!t->concrete() || t->kind() != KIND_FUNCTION) {
						err(lowered_args[i].loc(), "Could not deduce type for function ",
							"parameter, resolved to '", t, "'.");
						return error();
					}
					const Type* fnarg = ((const FunctionType*)t)->arg();
					FunctionValue& fn = lowered_args[i].get_function();
					ASTNode* argbody = fn.instantiation(fnarg);
					if (!argbody) {
						fn.instantiate(fnarg, new ASTIncompleteFn(lowered_args[i].loc(),
							fnarg, fn.name()));
						argbody = instantiate(lowered_args[i].loc(), fn, fnarg);
					}
					if (!argbody) return error();
					arg_nodes.push(argbody);
				}
				else arg_nodes.push(lowered_args[i].get_runtime());
			}
			return new ASTCall(function.loc(), function.get_runtime(), arg_nodes);
		}

    if (!function.is_function() && !function.is_error()) {
      err(function.loc(), "Called value is not a procedure.");
      return error();
    }
    if (!arg.is_product() && !arg.is_error()) {
      err(arg.loc(), "Arguments not provided as a product.");
      return error();
    }
    if (function.is_error() || arg.is_error()) return error();

    FunctionValue& fn = function.get_function();
    if (fn.is_builtin()) {
      return fn.get_builtin()(env, arg);
    }
    else {
      ref<Env> env = fn.get_env();
      u32 argc = arg.get_product().size(), arity = fn.args().size();
      if (argc != arity) {
        err(function.loc(), "Procedure requires ", arity, " arguments, ",
          argc, " provided.");
        return error();
      }

			bool runtime_call = false;
			for (u32 i = 0; i < argc; i ++) {
				if (arg.get_product()[i].is_runtime()) runtime_call = true;
			}
			if (!fn.found_calls()) {
				set<const FunctionValue*> visited;
				find_calls(fn, env, fn.body(), visited);
			}
			if (fn.recursive()) runtime_call = true;
			
			if (runtime_call) {
				vector<const Type*> argts;
				vector<Value> lowered_args;
				for (u32 i = 0; i < argc; i ++) {
					if (fn.args()[i] & KEYWORD_ARG_BIT) {
						// keyword arg
						if (!arg.get_product()[i].is_symbol() ||
								arg.get_product()[i].get_symbol() != 
									(fn.args()[i] & ARG_NAME_MASK)) {
							err(arg.get_product()[i].loc(), "Expected keyword '",
								symbol_for(fn.args()[i] & ARG_NAME_MASK), "'.");
							return error();
						}
					}
					else {
						if (arg.get_product()[i].is_function()) {
							vector<const Type*> inner_argts;
							for (u32 j = 0; j < arg.get_product()[i].get_function().arity(); j ++)
								inner_argts.push(find<TypeVariable>());
							argts.push(find<FunctionType>(
								find<ProductType>(inner_argts), find<TypeVariable>()));
							lowered_args.push(arg.get_product()[i]); // we'll lower this later
						}
						else {
							Value lowered = lower(arg.get_product()[i]);
							argts.push(((const RuntimeType*)lowered.type())->base());
							lowered_args.push(lowered);
						}
					}
				}
				const Type* argt = find<ProductType>(argts);
				ASTNode* body = fn.instantiation(argt);
				if (!body) {
					fn.instantiate(argt, new ASTIncompleteFn(function.loc(), argt, fn.name()));
					body = instantiate(function.loc(), fn, argt);
				} 
				if (!body) return error();
				vector<ASTNode*> arg_nodes;
				for (u32 i = 0; i < lowered_args.size(); i ++) {
					if (lowered_args[i].is_function()) {
						const Type* t = ((const ProductType*)argt)->member(i);
						if (t->kind() != KIND_FUNCTION || 
							!((const FunctionType*)t)->arg()->concrete()) {
							err(lowered_args[i].loc(), "Could not deduce type for function ",
								"parameter, resolved to '", t, "'.");
							return error();
						}
						const Type* fnarg = ((const FunctionType*)t)->arg();
						while (fnarg->kind() == KIND_TYPEVAR)
							fnarg = ((const TypeVariable*)fnarg)->actual();
						FunctionValue& fn = lowered_args[i].get_function();
						ASTNode* argbody = fn.instantiation(fnarg);
						if (!argbody) {
							fn.instantiate(fnarg, new ASTIncompleteFn(lowered_args[i].loc(),
								fnarg, fn.name()));
							argbody = instantiate(lowered_args[i].loc(), fn, fnarg);
						}
						if (!argbody) return error();
						arg_nodes.push(argbody);
					}
					else arg_nodes.push(lowered_args[i].get_runtime());
				}
				return new ASTCall(function.loc(), body, arg_nodes);
			}

      for (u32 i = 0; i < arity; i ++) {
				if (fn.args()[i] & KEYWORD_ARG_BIT) {
					// keyword arg
					if (!arg.get_product()[i].is_symbol() ||
							arg.get_product()[i].get_symbol() != 
								(fn.args()[i] & ARG_NAME_MASK)) {
						err(arg.get_product()[i].loc(), "Expected keyword '",
							symbol_for(fn.args()[i] & ARG_NAME_MASK), "'.");
						return error();
					}
				}
        else {
					const string& argname = symbol_for(fn.args()[i] & ARG_NAME_MASK);
        	env->find(argname)->value = arg.get_product()[i];
				}
      }
      return eval(env, fn.body());
    }
  }

	Value display(const Value& arg) {
		return new ASTDisplay(arg.loc(), lower(arg).get_runtime());
	}

	Value assign(ref<Env> env, const Value &dest, const Value& src) {
    if (!dest.is_symbol()) { 
      err(dest.loc(), "Invalid destination in assignment '", dest, "'.");
      return error();  
    }

		Def* def = env->find(symbol_for(dest.get_symbol()));
		if (!def) {
			err(dest.loc(), "Undefined variable '", 
				symbol_for(dest.get_symbol()), "'.");
			return error();
		}
		Value lowered = src;
		if (!lowered.is_runtime()) lowered = lower(src);
		if (def->value.is_runtime())
			return new ASTAssign(dest.loc(), env, 
				dest.get_symbol(), lowered.get_runtime());
		else {
			def->value = lower(def->value);
			return new ASTDefine(dest.loc(), env,
				dest.get_symbol(), lowered.get_runtime());
		}
	}
}

template<>
u64 hash(const basil::Value& t) {
  return t.hash();
}

void write(stream& io, const basil::Value& t) {
  t.format(io);
}