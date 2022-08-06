#include "lua.h"

static lua_Alloc _g_alloc = NULL;
static lua_State *_g_L = NULL;

#define ispseudo(i)		((i) <= LUA_REGISTRYINDEX)
static evm_val_t _g_nil = EVM_VAL_NULL;
/**
 * @brief lua取栈索引位置和EVM取栈索引位置的换算
 */
static evm_val_t *index2addr(lua_State *L, int idx) {
    if( idx == LUA_REGISTRYINDEX ) {
        return L->registry;
    }
    if( idx > 0) {
        return L->v + idx - 1;
    } else if( !ispseudo(idx) ) {
		return L->e->sp + (idx + 1);
	} else {
		return &_g_nil;
	}
}
/**
 * @brief evm数据类型换算对应的lua数据类型
 */
int _lua_type(evm_val_t *v) {
    switch( evm_type(v)) {
        case TYPE_BOOLEAN: return LUA_TBOOLEAN;
        case TYPE_NUMBER: return LUA_TNUMBER;
        case TYPE_BUFFER: return LUA_TSTRING;
        case TYPE_FOREIGN_STRING: return LUA_TSTRING;
        case TYPE_HEAP_STRING: return LUA_TSTRING;
        case TYPE_OBJECT: return LUA_TTABLE;
        case TYPE_FUNCTION: return LUA_TFUNCTION;
        case TYPE_FOREIGN: return LUA_TFUNCTION;
        case TYPE_UNDEF: return LUA_TNIL;
        case TYPE_NONE: return LUA_TNIL;
    }
    return LUA_TNONE;
}

LUA_API void lua_lock (lua_State *L) {
    evm_lock(L->e);
}

LUA_API void lua_unlock (lua_State *L) {
    evm_lock(L->e);
}

LUA_API int lua_absindex (lua_State *L, int idx) {
    if( idx > 0 || ispseudo(idx) )
        return idx;
    return (int)(idx + (intptr_t)L->e->sp);
}

LUA_API int lua_gettop (lua_State *L) {
  return L->e->sp - L->v + 1;
}

LUA_API void lua_settop (lua_State *L, int idx) {
    lua_lock(L);
    if( idx < 0 ) {
        L->e->sp += idx+1;
    }
    lua_unlock(L);
}

LUA_API void lua_pushvalue (lua_State *L, int idx) {
    lua_lock(L);
    evm_val_t *offset = index2addr(L, idx);
    ++L->e->sp;
    *L->e->sp = *offset;
    lua_unlock(L);
}

static void reverse (lua_State *L, evm_val_t *from, evm_val_t *to) {
  for (; from < to; from++, to--) {
    evm_val_t temp;
    temp = *from;
    *from = *to;
    *to = temp;
  }
}

LUA_API void lua_rotate (lua_State *L, int idx, int n) {
    evm_val_t *t = L->e->sp;
    evm_val_t *p = index2addr(L, idx);
    evm_val_t *m;
    m = (n >= 0 ? t - n : p - n - 1);

    reverse(L, p, m);  /* reverse the prefix with length 'n' */
    reverse(L, m + 1, t);  /* reverse the suffix */
    reverse(L, p, t);  /* reverse the entire segment */
}

LUA_API void lua_copy (lua_State *L, int fromidx, int toidx) {
    lua_lock(L);
    evm_val_t * from_offset = index2addr(L, fromidx);
    evm_val_t * to_offset = index2addr(L, toidx);
    *(to_offset) = *(from_offset);
    lua_unlock(L);
}

LUA_API int lua_checkstack (lua_State *L, int n) {
  if( ((L->e->sp - L->e->sp_base) + n) < (L->e->stack_size / sizeof(evm_val_t)) ){
      return 1;
  }
  return 0;
}
//todo
LUA_API void lua_xmove (lua_State *from, lua_State *to, int n) {
    EVM_ASSERT(0);
}

LUA_API int lua_isnumber (lua_State *L, int idx) {
    evm_val_t * offset = index2addr(L, idx);
    if( evm_is_number( offset ) ) 
        return 1;
    return 0;
}

LUA_API int lua_isstring (lua_State *L, int idx) {
    evm_val_t * offset = index2addr(L, idx);
    if( evm_is_string( offset ) ) 
        return 1;
    return 0;
}

LUA_API int lua_iscfunction (lua_State *L, int idx) {
    evm_val_t * offset = index2addr(L, idx);
    if( evm_is_native( offset ) || evm_is_foreign( offset ) )
        return 1;
    return 0;
}

LUA_API int lua_isinteger(lua_State *L, int idx) {
    evm_val_t * offset = index2addr(L, idx);
    if( evm_is_integer( offset ) ) 
        return 1;
    return 0;
}

LUA_API int lua_isuserdata (lua_State *L, int idx) {
    evm_val_t * v = index2addr(L, idx);
    if( evm_is_object(v) && evm_object_get_ext_data(v) != NULL ) {
        return 1;
    }
    return 0;
}

LUA_API int lua_type (lua_State *L, int idx) {
    if( idx > L->argc )
        return LUA_TNONE;
    evm_val_t * offset = index2addr(L, idx);
    return _lua_type(offset);
}

LUA_API const char *lua_typename (lua_State *L, int t) {
  EVM_UNUSED(L);
  switch (t) {
    case LUA_TNONE: return "no value";
    case LUA_TNIL:  return "nil";
    case LUA_TBOOLEAN: return "boolean";
    case LUA_TNUMBER: return "number";
    case LUA_TSTRING: return "string";
    case LUA_TTABLE: return "table";
    case LUA_TFUNCTION: return "function";
    case LUA_TUSERDATA: return "userdata";
    default:
      evm_set_err(L->e, ec_type, "invalid tag");
      lua_error(L);
  }
  return NULL;
}

LUA_API lua_Number lua_tonumberx (lua_State *L, int idx, int *pisnum) {
    evm_val_t * offset = index2addr(L, idx);
    if( !evm_is_number(offset) ) {
        if( pisnum ) *pisnum = 0;
        return 0;
    }
    if( pisnum ) *pisnum = 1;
    return (lua_Number)evm_2_double(offset);
}

LUA_API lua_Integer lua_tointegerx (lua_State *L, int idx, int *pisnum) {
    evm_val_t * offset = index2addr(L, idx);
    if( !evm_is_integer(offset) ) {
        if( pisnum ) *pisnum = 0;
        return 0;
    }
    if( pisnum ) *pisnum = 1;
    return evm_2_integer(offset);
}

LUA_API int lua_toboolean (lua_State *L, int idx) {
    evm_val_t * offset = index2addr(L, idx);
    return evm_2_boolean(offset);
}

LUA_API const char *lua_tolstring (lua_State *L, int idx, size_t *len) {
    evm_val_t * offset = index2addr(L, idx);
    if( evm_is_string(offset) ){
        const char *s = evm_2_string(offset);
        if( len ) *len = strlen(s);
        return s;
    } else if( evm_is_buffer(offset) ) {
        const char *s = (const char *)evm_buffer_addr(offset);
        if( len ) *len = strlen(s);
        return s;
    } else {
        if (len != NULL)
            *len = 0;
        return NULL;
    }
}

LUA_API int _evm_lua_table_len(lua_State *L, evm_val_t *v) {
    int len = evm_prop_len(v);
    return len;
}

LUA_API size_t lua_rawlen (lua_State *L, int idx) {
    evm_val_t * v = index2addr(L, idx);
    if( evm_is_string(v) ) {
        return evm_string_len(v);
    } else if( evm_is_object(v) ) {
        return _evm_lua_table_len(L, v);
    }
    return 0;
}

LUA_API lua_CFunction lua_tocfunction (lua_State *L, int idx) {
    evm_val_t * offset = index2addr(L, idx);
    if( evm_is_foreign(offset) ) {
        return (lua_CFunction)evm_2_intptr(offset);
    }
    return NULL;
}

LUA_API void *lua_touserdata (lua_State *L, int idx) {
    evm_val_t * v = index2addr(L, idx);
    if( evm_is_object(v) && evm_object_get_ext_data(v) != NULL ){
        return (void*)evm_object_get_ext_data(v);
    }
    return NULL;
}
//todo
LUA_API lua_State *lua_tothread (lua_State *L, int idx) {
    EVM_ASSERT(0);
    return NULL;
}

LUA_API const void *lua_topointer (lua_State *L, int idx) {
    evm_val_t * offset = index2addr(L, idx);
    return (const void*)evm_2_object(offset);
}
/* first operand at top - 2, second at top - 1; result go to top - 2 */
LUA_API void lua_arith (lua_State *L, int op) {
    if( !evm_is_number(L->e->sp) || !evm_is_number(L->e->sp - 1) ) {
        L->e->sp -= 1;
        evm_set_undefined(L->e->sp);
        evm_set_err(L->e, ec_arithmetic, "Value must be number type");
        return;
    }

    if( evm_is_integer(L->e->sp) && evm_is_integer(L->e->sp - 1) ) {
        lua_Integer v1 = (lua_Integer)evm_2_integer(L->e->sp - 1);
        lua_Integer v2 = (lua_Integer)evm_2_integer(L->e->sp - 1);
        lua_Integer res = 0;

        switch(op) {
            case LUA_OPADD: res = v1 + v2;break;
            case LUA_OPSUB: res = v1 - v2;break;
            case LUA_OPMUL: res = v1 * v2;break;
            case LUA_OPMOD: res = v1 % v2;break;
            case LUA_OPPOW: 
            case LUA_OPDIV: res = v1 / v2;break;
            case LUA_OPIDIV:
            case LUA_OPBAND: res = v1 & v2;break;
            case LUA_OPBOR:  res = v1 | v2;break;
            case LUA_OPBXOR: res = v1 ^ v2;break;
            case LUA_OPSHL:  res = v1 << v2;break;
            case LUA_OPSHR:  res = v1 >> v2;break;
        }
        L->e->sp--;
        evm_set_number(L->e->sp, res);
    } else {
        double v1 = evm_2_double(L->e->sp - 1);
        double v2 = evm_2_double(L->e->sp - 1);
        double res = 0;
        switch(op) {
            case LUA_OPADD: res = v1 + v2;break;
            case LUA_OPSUB: res = v1 - v2;break;
            case LUA_OPMUL: res = v1 * v2;break;
            case LUA_OPDIV: res = v1 / v2;break;
        }
        L->e->sp--;
        evm_set_number(L->e->sp, res);
    }
}

LUA_API int lua_rawequal (lua_State *L, int index1, int index2) {
    evm_val_t * v1 = index2addr(L, index1);
    evm_val_t * v2 = index2addr(L, index2);
    return upy_compare(L->e, *v1, *v2) == 0;
}

LUA_API int lua_compare (lua_State *L, int index1, int index2, int op) {
    evm_val_t * v1 = index2addr(L, index1);
    evm_val_t * v2 = index2addr(L, index2);
    switch (op) {
      case LUA_OPEQ: return (*v1 == *v2);
      case LUA_OPLT: {
          if( evm_is_integer(v1) && evm_is_integer(v2) ) {
              return evm_2_intptr(v1) < evm_2_intptr(v2);
          } else if( evm_is_number(v1) && evm_is_number(v2) ) {
              return evm_2_double(v1) < evm_2_double(v2);
          } else {
              evm_set_err(L->e, ec_type, "invalid type");return 0;
          }
      }break;
      case LUA_OPLE: {
          if( evm_is_integer(v1) && evm_is_integer(v2) ) {
              return evm_2_intptr(v1) <= evm_2_intptr(v2);
          } else if( evm_is_number(v1) && evm_is_number(v2) ) {
              return evm_2_double(v1) <= evm_2_double(v2);
          } else {
              evm_set_err(L->e, ec_type, "invalid type");return 0;
          }
      }break;
      default: evm_set_err(L->e, ec_type, "invalid option");return 0;
    }
}

LUA_API void lua_pushnil(lua_State *L) {
    evm_push_null(L->e);
}

LUA_API void lua_pushnumber(lua_State *L, lua_Number n) {
    evm_push_number(L->e, (uint32_t)n);
}

LUA_API void lua_pushinteger(lua_State *L, lua_Integer n) {
    evm_push_integer(L->e, n);
}

LUA_API const char *lua_pushlstring(lua_State *L, const char *s, size_t len) {
    evm_val_t * ret = evm_heap_string_create(L->e, s, (int)len);
    return (const char *)evm_buffer_addr(ret);
}

LUA_API const char *lua_pushstring(lua_State *L, const char *s) {
    evm_val_t * ret = evm_push_heap_string(L->e, s);
    return (const char *)evm_buffer_addr(ret);
}

static void pushstr (lua_State *L, const char *str, size_t l) {
    if( l == 0 ) {
        evm_heap_string_create(L->e, "", (int)l);
        return;
    }
    if( strlen(str) > l ) {
        evm_val_t *o = evm_heap_string_create(L->e, "", (int)l);
        memcpy(evm_heap_string_addr(o), str, l);
    } else {
        evm_heap_string_create(L->e, str, (int)l);
    }
}

static const char *luaO_pushvfstring (lua_State * L, const char *fmt, va_list argp) {
  int n = 0;
  for (;;) {
    const char *e = strchr(fmt, '%');
    if (e == NULL) break;
    pushstr(L, fmt, e - fmt);
    switch (*(e+1)) {
      case 's': {  /* zero-terminated string */
        const char *s = va_arg(argp, char *);
        if (s == NULL) s = "(null)";
        pushstr(L, s, strlen(s));
        break;
      }
      case 'c': {  /* an 'int' as a character */
        char buff =  (char)va_arg(argp, int);
        pushstr(L, &buff, 1);
        break;
      }
      case 'd': {  /* an 'int' */
        char local_buf[32];
        sprintf(local_buf, "%i", (int)va_arg(argp, int));
        lua_pushstring(L, local_buf);
        break;
      }
      case 'I': {  /* a 'lua_Integer' */
        char local_buf[32];
        sprintf(local_buf, "%i", (lua_Integer)va_arg(argp, int));
        lua_pushstring(L, local_buf);
        break;
      }
      case 'f': {  /* a 'lua_Number' */
        char local_buf[32];
        sprintf(local_buf, "%f", (lua_Number)va_arg(argp, lua_Number));
        lua_pushstring(L, local_buf);
        break;
      }
      case '%': {
        pushstr(L, "%", 1);
        break;
      }
      default: {
        evm_print("invalid option '%%%c' to 'lua_pushfstring'", *(e + 1));
        EVM_ASSERT(0);
      }
    }
    n += 2;
    fmt = e+2;
  }
  pushstr(L, fmt, strlen(fmt));
  if (n > 0) lua_concat(L, n + 1);
  return evm_heap_string_addr(L->e->sp);
}

LUA_API const char *lua_pushvfstring (lua_State *L, const char *fmt,
                                      va_list argp) {
  const char *ret;
  lua_lock(L);
  ret = luaO_pushvfstring(L, fmt, argp);
  lua_unlock(L);
  return ret;
}

LUA_API const char *lua_pushfstring (lua_State *L, const char *fmt, ...) {
  const char *ret;
  va_list argp;
  lua_lock(L);
  va_start(argp, fmt);
  ret = luaO_pushvfstring(L, fmt, argp);
  va_end(argp);
  lua_unlock(L);
  return ret;
}

LUA_API void lua_pushcclosure (lua_State *L, lua_CFunction fn, int n) {
    if( n == 0 ) {
        evm_push_value(L->e, evm_mk_foreign((void*)fn));
    } else {
        evm_val_t *ret = evm_list_create(L->e, GC_LIST, (n + 1));
        if( ret ) {
            evm_list_set(L->e, ret, 0, evm_mk_foreign((void*)fn));
            for(int i = 0; i < n; i++)
                evm_list_set(L->e, ret, i + 1, *(L->e->sp - n + i + 1));
            L->e->sp -= n;
        } else {
            evm_push_null(L->e);
        }
    }
}

LUA_API void lua_pushboolean(lua_State *L, int b) {
    evm_push_boolean(L->e, b);
}

LUA_API void lua_pushlightuserdata (lua_State *L, void *p) {
    lua_lock(L);
    evm_push_value(L->e, evm_mk_foreign(p));
    lua_unlock(L);
}

LUA_API int lua_getglobal (lua_State *L, const char *name) {
    lua_lock(L);

    evm_val_t * global = L->global;
    if( !evm_is_object(global) )
        return LUA_TNONE;

    evm_val_t res = evm_prop_get(L->e, global, name);
    evm_push_value(L->e, res);
    lua_unlock(L);
    return ec_ok;
}

LUA_API int lua_gettable (lua_State *L, int idx) {
    evm_val_t * offset = index2addr(L, idx);
    if( evm_is_foreign(L->e->sp) ) {
        char const * unique_address = (const char*)evm_2_object(L->e->sp);
        evm_val_t ret = evm_prop_get(L->e, offset, unique_address);
        *L->e->sp = ret;
    } else if( evm_is_string(L->e->sp) ) {
        const char * key = evm_2_string(L->e->sp);
        evm_val_t ret = evm_prop_get(L->e, offset, key);
        *L->e->sp = ret;
    } else {
        evm_set_null(L->e->sp);
        return LUA_TNONE;
    }
    return _lua_type(L->e->sp);
}

LUA_API int lua_getfield (lua_State *L, int idx, const char *k) {
    evm_val_t * v = index2addr(L, idx);
    if( !evm_is_object(v) ) {
        evm_push_undefined(L->e);
        return LUA_TNIL;
    } 
    evm_val_t ret = evm_prop_get(L->e, v, k);
    evm_push_value(L->e, ret);
    return _lua_type(&ret);
}

LUA_API int lua_geti (lua_State *L, int idx, lua_Integer n) {
    evm_val_t *v = index2addr(L, idx);
    if( !evm_is_object(v) ) {
        evm_push_value(L->e, EVM_VAL_UNDEFINED);
        return LUA_TNONE;
    }
    evm_val_t ret = evm_prop_get_by_index(L->e, v, n - 1);
    evm_push_value(L->e, ret);
    return _lua_type(L->e->sp);
}

LUA_API int lua_rawget (lua_State *L, int idx) {
    return lua_gettable(L, idx);
}

LUA_API int lua_rawgeti (lua_State *L, int idx, lua_Integer n) {
    n = n - 1;
    if( n == 0 ) {
        evm_push_null(L->e);
        return LUA_TNIL;
    }

    evm_val_t * v = index2addr(L, idx);
    if( !evm_is_object(v) ) {
        evm_push_null(L->e);
        return LUA_TNONE;
    }

    if( n >= evm_prop_len(v) ){
        evm_push_null(L->e);
        return LUA_TNONE;
    }
    evm_val_t ret = evm_prop_get_by_index(L->e, v, n);
    *L->e->sp = ret;
    return _lua_type(&ret);
}
//todo
LUA_API int lua_rawgetp (lua_State *L, int idx, const void *p) {
    return lua_getfield(L, idx, p);
}

LUA_API void lua_newlibtable (lua_State *L, int narray, int nrec) {
    evm_val_t *table = evm_object_create(L->e, GC_ROOT);
    if( table == NULL ) {
        evm_push_value(L->e, EVM_VAL_UNDEFINED);
        return;
    }
}

LUA_API void lua_createtable (lua_State *L, int narray, int nrec) {
    evm_val_t *table = evm_object_create(L->e, GC_OBJECT);
    if( table == NULL ) {
        evm_push_value(L->e, EVM_VAL_UNDEFINED);
        return;
    }
}

LUA_API void *lua_newuserdata (lua_State *L, size_t size) {
    lua_lock(L);
    void * userdata = evm_malloc(size);
    if( userdata == NULL ) {
        evm_push_null(L->e);
        return NULL;
    }

    evm_val_t *o = evm_object_create(L->e, GC_OBJECT);
    if( o == NULL ) {
        evm_push_null(L->e);
        return NULL;
    }
    evm_object_set_ext_data(o, userdata);
    lua_unlock(L);
    return userdata;
}

LUA_API int lua_getmetatable (lua_State *L, int objindex) {
    evm_val_t * v = index2addr(L, objindex);
    if( !evm_is_object(v) )
        return 1;

    evm_val_t res = upy_object_get_parent(*v);
    evm_push_value(L->e, res);
    return _lua_type(&res);
}
//todo
LUA_API int lua_getuservalue (lua_State *L, int idx) {
    return LUA_OK;
}
//todo
LUA_API void lua_setglobal (lua_State *L, const char *name) {
    evm_prop_push(L->e, L->global, name, L->e->sp);
}
//todo
LUA_API void lua_settable (lua_State *L, int idx) {
    evm_val_t * v = index2addr(L, idx);
    if( !evm_is_object(v) ) {
        L->e->sp -= 2;
        return;
    } 
    evm_val_t *key = L->e->sp - 1;
    evm_val_t *value = L->e->sp;
    if( !evm_is_string(v) ) {
        L->e->sp -= 2;
        return;
    }
    evm_prop_set(L->e, v, evm_2_string(key), *value);
    L->e->sp -= 2;
}

LUA_API void lua_setfield (lua_State *L, int idx, const char *k) {
    evm_val_t * offset = index2addr(L, idx);
    if( evm_is_object(offset) ) {
        evm_t * e = L->e;
        evm_prop_set(e, offset, k, *e->sp);
    }
    lua_pop(L, 1);
}
//todo
LUA_API void lua_seti (lua_State *L, int idx, lua_Integer n) {

}

LUA_API void lua_rawset (lua_State *L, int idx) {
    evm_t *e = L->e;
    evm_val_t * v = index2addr(L, idx);
    if( !evm_is_object(v) && !evm_is_list(v) )
        return;

    evm_val_t *key = e->sp - 1;
    evm_val_t *val = e->sp;

    if( evm_is_integer(key) ) {
        if( evm_is_object(v) )
            evm_prop_set_value_by_index(L->e, v, evm_2_integer(key) - 1, *val);
        else
            evm_list_set(e, v, evm_2_integer(key) - 1, *val);
    } else if( evm_is_string(key) ){
        if( evm_is_object(v) )
            evm_prop_set(L->e, v, evm_2_string(key), *val);
    }
    L->e->sp -= 2;
}

LUA_API void lua_rawseti (lua_State *L, int idx, lua_Integer n) {
    if( n == 0 )
        return;
    n = n - 1;
    evm_t *e = L->e;
    evm_val_t * v = index2addr(L, idx);
    if( !evm_is_object(v) && !evm_is_list(v) )
        return;
    if( evm_is_object(v) )
        evm_prop_set_value_by_index(e, v, n, *e->sp);
    else
        evm_list_set(e, v, n, *e->sp);
    e->sp--;
}
//todo
LUA_API void lua_rawsetp (lua_State *L, int idx, const void *p) {

}

LUA_API int lua_setmetatable (lua_State *L, int objindex) {
    evm_val_t * v = index2addr(L, objindex);
    if( !evm_is_object(v) )
        return 1;

    upy_object_set_parent(*v, *L->e->sp);
    L->e->sp--;
    return LUA_OK;
}

LUA_API void lua_setuservalue (lua_State *L, int idx) {
    evm_val_t * v = index2addr(L, idx);
    if( !evm_is_object(v) ) {
        L->e->sp--;
        return;
    }

    if( !evm_is_foreign(L->e->sp) ) {
        L->e->sp--;
        return;
    }

    evm_object_set_ext_data(v, evm_2_object(L->e->sp));
    L->e->sp--;
}

evm_val_t _lua_cclosure_exec(evm_t *e, evm_val_t p, int argc, evm_val_t *v) {
    lua_CFunction func = (lua_CFunction)evm_2_object(&p);
    lua_State L;
    memcpy(&L, _g_L, sizeof (lua_State));
    L.e = e;
    L.p = &p;
    L.v = v;
    L.argc = argc;
    func(&L);
    return EVM_VAL_UNDEFINED;
}

LUA_API void lua_callk (lua_State *L, int nargs, int nresults,
                        lua_KContext ctx, lua_KFunction k) {
    lua_pcallk(L, nargs, nresults, 0, ctx, k);
}

LUA_API int lua_pcallk (lua_State *L, int nargs, int nresults, int errfunc,
                        lua_KContext ctx, lua_KFunction k) {
    evm_t *e = L->e;
    evm_val_t *invoke = e->sp - nargs;
    if( evm_is_script(invoke) ) {
        *invoke = evm_run_callback(e, invoke, &e->scope, invoke + 1, nargs);
        e->sp -= nargs + 1;
        for(int i = 0; i < nargs; i++) {
            evm_set_undefined(invoke + i + 1);
        }
        return LUA_OK;
    }

    if( evm_is_list(invoke) && evm_list_len(invoke) > 0 ) {
        int len = evm_list_len(invoke);
        evm_val_t args[len];
        for(uint32_t i = 0; i < len - 1; i++) {
            args[i] = evm_list_get(e, invoke, i + 1);
        }

        _lua_cclosure_exec(e, evm_list_get(e, invoke, 0), len - 1, args);
        if( nresults == 1) {
            *invoke = *e->sp;
            e->sp = invoke;
        } else if( nresults > 1) {
            e->sp = invoke - 1;
        } else {
            e->sp = invoke - 1;
        }
        return LUA_OK;
    }

    if( !evm_is_foreign(invoke) ) {
        evm_set_err(e, ec_type, "Object is not callable");
        L->e->sp -= nargs;
        L->e->sp -= 2;
        return LUA_ERRERR;
    }

    _lua_cclosure_exec(e, *invoke, nargs, invoke + 1);
    if( nresults == 1) {
        *invoke = *e->sp;
        e->sp = invoke;
    } else if( nresults > 1) {
        e->sp = invoke - 1;
    } else {
        e->sp = invoke - 1;
    }
    for(int i = 0; i < nargs; i++) {
        evm_set_undefined(invoke + i + 1);
    }
    return LUA_OK;
}

LUA_API int lua_load (lua_State *L, lua_Reader reader, void *data,
                      const char *chunkname, const char *mode) {
    int status = LUA_OK;
    lua_lock(L);
    if (!chunkname) chunkname = "?";
    int function_size = strlen("function()\r\n");
    int end_size = strlen("\r\nend");
    int size = strlen(chunkname) + function_size + end_size + 1;
    char *buf = evm_malloc(size);
    if( buf == NULL )
        return LUA_ERRMEM;
    memcpy(buf, "function()\r\n", function_size);
    memcpy(buf + function_size, chunkname, strlen(chunkname));
    memcpy(buf + function_size + strlen(chunkname), "\r\nend", end_size);
    *L->e->sp = evm_run_eval(L->e, &L->e->scope, buf);
    lua_unlock(L);
    return status;
}
//todo
LUA_API int lua_dump (lua_State *L, lua_Writer writer, void *data, int strip) {
    return LUA_OK;
}
//todo
LUA_API int lua_gc (lua_State *L, int what, int data) {
    return LUA_OK;
}

LUA_API int lua_error (lua_State *L) {
    if( evm_is_string(L->e->sp) ){
        evm_set_err(L->e, ec_type, evm_2_string(L->e->sp));
    }
    EVM_ASSERT(0);
    return 0;
}

LUA_API int lua_next (lua_State *L, int idx) {
    evm_t *e = L->e;
    evm_val_t * v = index2addr(L, idx);
    if( !evm_is_object(v) ) {
        goto lua_next_err;
    }
    upy_object_t *obj = evm_2_object(v);
    evm_val_t *key = v + 1;
    if( evm_is_null(key) ) {
        if( obj->member ) {
            evm_pop(L->e);
            evm_push_value(e, obj->member->name);
            evm_push_value(e, obj->member->val);
            return 1;
        } else {
            goto lua_next_err;
        }
    } else if( !evm_is_string(key) ) {
        goto lua_next_err;
    }

    upy_member_t *next = obj->member;
    while(next) {
        if( upy_compare(e, next->name, *key) != 0 ) {
            next = next->next;
            continue;
        }
        evm_pop(L->e);
        evm_push_value(e, obj->member->name);
        evm_push_value(e, obj->member->val);
        return 1;
    }
lua_next_err:
    evm_pop(L->e);
    return 0;
}

LUA_API void lua_concat (lua_State *L, int n) {
    EVM_ASSERT(n >= 2);
    lua_lock(L);
    evm_val_t * base_op = L->e->sp - (n - 1);
    evm_val_t * op = L->e->sp - (n - 1);
    evm_val_t * res;
    for(int i = 0; i < (n - 1); i++){
        char *s1, *s2;
        int len1, len2;
        if( evm_is_foreign_string(op) || evm_is_heap_string(op)){
            s1 = (char*)evm_2_string(op);
            len1 = strlen(s1);
        } else {
            evm_set_err(L->e, ec_type, "try to concat non-string type");
            lua_error(L);
        }

        if( evm_is_foreign_string(op + 1) || evm_is_heap_string(op + 1)){
            s2 = (char*)evm_2_string(op + 1);
            len2 = strlen(s2);
        } else {
            evm_set_err(L->e, ec_type, "try to concat non-string type");
            lua_error(L);
        }

        res = evm_heap_string_create(L->e, "", len1 + len2 + 1);
        char * buf = evm_heap_string_addr(res);
        memcpy(buf, s1, len1);
        memcpy(buf + len1, s2, len2);
        *(op + 1) = *res;
        op++;
    }
    *base_op = *res;
    L->e->sp = base_op;
    lua_unlock(L);
}

LUA_API void lua_len (lua_State *L, int idx) {
    evm_t *e = L->e;
    evm_val_t *v = index2addr(L, idx);
    if( evm_is_object(v) ) {
        int len = evm_prop_len(v);
        evm_push_number(e, len);
        return;
    }

    if( evm_is_string(v) ) {
        evm_push_number(e, evm_string_len(v));
        return;
    }
    evm_set_err(L->e, ec_type, "attempt to get length of invalid type");
    lua_error(L);
}

LUA_API size_t lua_stringtonumber (lua_State *L, const char *s) {
  lua_Number value = (lua_Number)atof(s);
  lua_pushnumber(L, value);
  return strlen(s) + 1;
}

LUA_API lua_Alloc lua_getallocf (lua_State *L, void **ud) {
    return _g_alloc;
}

LUA_API void lua_setallocf (lua_State *L, lua_Alloc f, void *ud) {

}

LUA_API const char *(lua_getupvalue) (lua_State *L, int funcindex, int n) {
    return NULL;
}

LUA_API const char *(lua_setupvalue) (lua_State *L, int funcindex, int n) {
    return NULL;
}

static evm_val_t evm_lua_type(evm_t * e, evm_val_t * p, int argc, evm_val_t * v)
{
    switch( evm_type(v)) {
        case TYPE_BOOLEAN: return evm_mk_foreign_string("boolean");
        case TYPE_NUMBER: return evm_mk_foreign_string("number");
        case TYPE_NATIVE: return evm_mk_foreign_string("function");
        case TYPE_BUFFER: return evm_mk_foreign_string("table");
        case TYPE_FOREIGN_STRING: return evm_mk_foreign_string("string");
        case TYPE_HEAP_STRING: return evm_mk_foreign_string("string");
        case TYPE_OBJECT: return evm_mk_foreign_string("table");
        case TYPE_FUNCTION: return evm_mk_foreign_string("function");
        case TYPE_FOREIGN: return evm_mk_foreign_string("function");
        case TYPE_UNDEF: return evm_mk_foreign_string("nil");
        case TYPE_NONE: return evm_mk_foreign_string("nil");
    }
    return evm_mk_foreign_string("nil");
}

LUA_API lua_CFunction lua_atpanic (lua_State *L, lua_CFunction panicf){
    return NULL;
}

LUA_API const lua_Number *lua_version (lua_State *L) {
  static const lua_Number version = LUA_VERSION_NUM;
  return &version;
}

static void * vm_malloc(int size)
{
    void *p = NULL;
    if( _g_alloc ) {
        p = _g_alloc(NULL, NULL, 0, size);
    } else {
        p = malloc(size);
    }

    if( p != NULL ) {
        memset(p, 0, size);
    }
    return p;
}

static void vm_free(void * mem)
{
    if( _g_alloc ) {
        _g_alloc(NULL, mem, 0, 0);
    } else {
        free(mem);
    }
}

LUA_API void lua_set_allocf(lua_Alloc f, void *args) {
    _g_alloc = f;
}

LUA_API lua_State * lua_new_state(evm_t *env) {
    lua_State *L = evm_malloc(sizeof (lua_State));
    if( L == NULL ) 
        return NULL;
    L->e = env;

    L->registry = evm_object_create(env, GC_OBJECT);
    evm_prop_set(env, L->registry, "1", EVM_VAL_UNDEFINED);
    L->global = evm_object_create(env, GC_OBJECT);
    evm_prop_set(env, L->registry, "2", *L->global);
    _g_L = L;
    return L;
}

LUA_API void lua_copystate(lua_State *L, evm_val_t *p, int argc, evm_val_t *v) {
    memcpy(L, _g_L, sizeof (lua_State));
    L->e = _g_L->e;
    L->p = p;
    L->v = v;
    L->argc = argc;
}



