#include "lauxlib.h"

extern evm_val_t _lua_cclosure_exec(evm_t *e, evm_val_t p, int argc, evm_val_t *v);
extern int _lua_type(evm_val_t *v);

//todo
LUALIB_API void luaL_checkversion_ (lua_State *L, lua_Number ver, size_t sz) {

}

LUALIB_API int luaL_getmetafield (lua_State *L, int obj, const char *event) {
    if (!lua_getmetatable(L, obj))  /* no metatable? */
        return LUA_TNIL;
    else {
        int tt;
        lua_pushstring(L, event);
        tt = lua_rawget(L, -2);
        if (tt == LUA_TNIL)  /* is metafield nil? */
            lua_pop(L, 2);  /* remove metatable and metafield */
        else
            lua_remove(L, -2);  /* remove only metatable */
        return tt;  /* return metafield type */
    }
}
//todo
LUALIB_API int luaL_callmeta (lua_State *L, int obj, const char *event) {
    return LUA_OK;
}

LUALIB_API const char *luaL_tolstring (lua_State *L, int idx, size_t *len) {
{
    switch (lua_type(L, idx)) {
        case LUA_TNUMBER: {
            if (lua_isinteger(L, idx))
            lua_pushfstring(L, "%I", (LUAI_UACINT)lua_tointeger(L, idx));
            else
            lua_pushfstring(L, "%f", (LUAI_UACNUMBER)lua_tonumber(L, idx));
            break;
            }
        case LUA_TSTRING:
            lua_pushvalue(L, idx);
            break;
        case LUA_TBOOLEAN:
            lua_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
            break;
        case LUA_TNIL:
            lua_pushliteral(L, "nil");
            break;
        default: {
            evm_set_err(L->e, ec_type, "try to convert invalid type to string");
            lua_error(L);
            break;
            }
        }
    }
    return lua_tolstring(L, -1, len);
}

LUALIB_API int luaL_argerror (lua_State *L, int arg, const char *extramsg) {
    evm_set_err(L->e, arg, extramsg);
    EVM_ASSERT(0);
    return 0; 
}

LUALIB_API const char *luaL_checklstring (lua_State *L, int arg, size_t *len) {
    return lua_tolstring(L, arg, len);
}

LUALIB_API const char *luaL_optlstring (lua_State *L, int arg,
                                        const char *def, size_t *len) {
  if (lua_isnoneornil(L, arg)) {
    if (len)
      *len = (def ? strlen(def) : 0);
    return def;
  }
  else 
    return luaL_checklstring(L, arg, len);
}

LUALIB_API lua_Number luaL_checknumber (lua_State *L, int arg) {
    int isnum;
    lua_Number d = lua_tonumberx(L, arg, &isnum);
    if (!isnum)
        evm_set_err(L->e, ec_type, "Invalid type of arguments");
    return d;
}

LUALIB_API lua_Number luaL_optnumber (lua_State *L, int arg, lua_Number def) {
    if( arg > L->argc ) {
        return def;
    }
    return luaL_opt(L, luaL_checknumber, arg, def);
}

LUALIB_API lua_Integer luaL_checkinteger (lua_State *L, int arg) {
    int isnum;
    lua_Integer d = lua_tointegerx(L, arg, &isnum);
    if (!isnum) {
        evm_set_err(L->e, ec_type, "Invalid type of arguments");
    }
    return d;
}

LUALIB_API lua_Integer luaL_optinteger (lua_State *L, int arg, lua_Integer def) {
    if( arg > L->argc ) {
        return def;
    }
    return luaL_opt(L, luaL_checkinteger, arg, def);
}

LUALIB_API void luaL_checkstack (lua_State *L, int space, const char *msg) {
  if (!lua_checkstack(L, space)) {
    if (msg)
      luaL_error(L, "stack overflow (%s)", msg);
    else
      luaL_error(L, "stack overflow");
  }
}

LUALIB_API void luaL_checktype (lua_State *L, int arg, int t) {
    if( lua_type(L, arg) != t ) {
        lua_pushstring(L, "invalid type");
        lua_error(L);
    }
}

LUALIB_API void luaL_checkany (lua_State *L, int arg) {
  if (lua_type(L, arg) == LUA_TNONE)
    luaL_argerror(L, arg, "value expected");
}

LUALIB_API int luaL_newmetatable (lua_State *L, const char *tname) {
    if (luaL_getmetatable(L, tname) != LUA_TNIL)  /* name already in use? */
        return 0;  /* leave previous value on top, but return 0 */
    lua_pop(L, 1);
    lua_createtable(L, 0, 2);  /* create metatable */
    lua_pushstring(L, tname);
    lua_setfield(L, -2, "__name");  /* metatable.__name = tname */
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, tname);  /* registry.name = metatable */
    return 1;
}

LUALIB_API void luaL_setmetatable (lua_State *L, const char *tname) {
    luaL_getmetatable(L, tname);
    lua_setmetatable(L, -2);
}
//todo
LUALIB_API void *luaL_testudata (lua_State *L, int ud, const char *tname) {
    void *p = lua_touserdata(L, ud);
    if (p != NULL) {  /* value is a userdata? */
        if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
            luaL_getmetatable(L, tname);  /* get correct metatable */
            if (!lua_rawequal(L, -1, -2))  /* not the same? */
                p = NULL;  /* value is a userdata with wrong metatable */
            lua_pop(L, 2);  /* remove both metatables */
            return p;
        }
    }
    return NULL;  /* value is not a userdata with a metatable */
}

LUALIB_API void *luaL_checkudata (lua_State *L, int ud, const char *tname) {
    void *p = luaL_testudata(L, ud, tname);
    return p;
}
//todo
LUALIB_API void luaL_where (lua_State *L, int level) {

}
//todo
LUALIB_API int luaL_error (lua_State *L, const char *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    luaL_where(L, 1);
    lua_pushvfstring(L, fmt, argp);
    va_end(argp);
    lua_concat(L, 2);
    return lua_error(L);
}
//todo
LUALIB_API int luaL_checkoption (lua_State *L, int arg, const char *def,
                                 const char *const lst[]) {
    return LUA_OK;                                  
}

LUALIB_API int luaL_fileresult (lua_State *L, int stat, const char *fname) {
  int en = errno;  /* calls to Lua API may change this value */
  if (stat) {
    lua_pushboolean(L, 1);
    return 1;
  }
  else {
    lua_pushnil(L);
    if (fname)
      lua_pushfstring(L, "%s: %s", fname, strerror(en));
    else
      lua_pushstring(L, strerror(en));
    lua_pushinteger(L, en);
    return 3;
  }
}
//todo
LUALIB_API int luaL_execresult (lua_State *L, int stat) {
    return LUA_OK;
}

#define freelist	0

LUALIB_API int luaL_ref (lua_State *L, int t) {
    int ref;
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);  /* remove from stack */
        return LUA_REFNIL;  /* 'nil' has a unique fixed reference */
    }
    t = lua_absindex(L, t);
    lua_rawgeti(L, t, freelist);  /* get first free element */
    ref = (int)lua_tointeger(L, -1);  /* ref = t[freelist] */
    lua_pop(L, 1);  /* remove it from stack */
    if (ref != 0) {  /* any free element? */
        lua_rawgeti(L, t, ref);  /* remove it from list */
        lua_rawseti(L, t, freelist);  /* (t[freelist] = t[ref]) */
    }
    else  /* no free elements */
        ref = (int)lua_rawlen(L, t) + 1;  /* get a new reference */
    lua_rawseti(L, t, ref);
    return ref;
}

LUALIB_API void luaL_unref (lua_State *L, int t, int ref) {
    if (ref >= 0) {
        t = lua_absindex(L, t);
        lua_rawgeti(L, t, freelist);
        lua_rawseti(L, t, ref);  /* t[ref] = t[freelist] */
        lua_pushinteger(L, ref);
        lua_rawseti(L, t, freelist);  /* t[freelist] = ref */
    }
}
//todo
LUALIB_API int luaL_loadfilex (lua_State *L, const char *filename,
                                             const char *mode) {
    return LUA_OK;                                       
}

typedef struct LoadS {
  const char *s;
  size_t size;
} LoadS;

static const char *getS (lua_State *L, void *ud, size_t *size) {
  LoadS *ls = (LoadS *)ud;
  (void)L;  /* not used */
  if (ls->size == 0) return NULL;
  *size = ls->size;
  ls->size = 0;
  return ls->s;
}

LUALIB_API int luaL_loadbufferx (lua_State *L, const char *buff, size_t size,
                                 const char *name, const char *mode) {
    LoadS ls;
    ls.s = buff;
    ls.size = size;
    return lua_load(L, getS, &ls, name, mode);
}

LUALIB_API int luaL_dostring (lua_State *L, const char *s) {
    upy_val_t res = upy_parse_string(L->e, s);
    upy_call(L->e, res, res, 0, NULL);
    return LUA_OK;
}

LUALIB_API int luaL_loadstring (lua_State *L, const char *s) {
  return luaL_loadbuffer(L, s, strlen(s), s);
}
//todo
LUALIB_API lua_State *luaL_newstate (void) {
    return LUA_OK;
}

LUALIB_API lua_Integer luaL_len (lua_State *L, int idx) {
    lua_Integer l;
    int isnum;
    lua_len(L, idx);
    l = lua_tointegerx(L, -1, &isnum);
    if (!isnum)
    luaL_error(L, "object length is not an integer");
    lua_pop(L, 1);  /* remove object */
    return l;
}
//todo
LUALIB_API const char *luaL_gsub (lua_State *L, const char *s, const char *p,
                                                               const char *r) {
    return NULL;
}

LUALIB_API void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -nup);
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_setfield(L, -(nup + 2), l->name);
  }
  lua_pop(L, nup);  /* remove upvalues */
}

LUALIB_API int luaL_getsubtable (lua_State *L, int idx, const char *fname) {
  if (lua_getfield(L, idx, fname) == LUA_TTABLE)
    return 1;  /* table already there */
  else {
    lua_pop(L, 1);  /* remove previous result */
    idx = lua_absindex(L, idx);
    lua_newtable(L);
    lua_pushvalue(L, -1);  /* copy to be left at top */
    lua_setfield(L, idx, fname);  /* assign new table to field */
    return 0;  /* false, because did not find table there */
  }
}
//todo
LUALIB_API void luaL_traceback (lua_State *L, lua_State *L1,
                                const char *msg, int level) {

}

static evm_val_t _luaL_native_caller(evm_t * e, evm_val_t * p, int argc, evm_val_t * v){
    if( !evm_is_foreign(p) )
        return EVM_VAL_UNDEFINED;
    lua_State L;
    lua_copystate(&L, p, argc, v);
    lua_CFunction func = (lua_CFunction)evm_2_object(p);
    int nresults = func(&L);
    if( nresults == 1) {
        return *e->sp;
    } else if( nresults > 1) {
        evm_val_t *ret = evm_list_create(e, GC_TUPLE, nresults);
        if( ret == NULL ) {
            e->sp -= nresults - 1;
            evm_set_undefined(e->sp);
        } else {
            e->sp -= nresults;
            for(int i = 0; i < nresults; i++)
                evm_list_set(e, ret, i, *(e->sp + i));
            *e->sp = *ret;
        }
        return *e->sp;
    } else {
        return EVM_VAL_UNDEFINED;
    }
}

LUALIB_API void luaL_requiref (lua_State *L, const char *modname,
                               lua_CFunction openf, int glb) {
    evm_t *e = L->e;
    evm_val_t module = evm_module_get(e, modname);
    if( module != VAL_UNDEF ) {
        evm_push_value(e, module);
        return;
    }
    lua_pushcfunction(L, openf);
    lua_pushstring(L, modname);
    lua_call(L, 1, 1);
    evm_module_add(e, modname, e->sp);
}

LUALIB_API void luaL_buffinit (lua_State *L, luaL_Buffer *B) {
    lua_lock(L);
    B->L = L;
    B->v = EVM_VAL_UNDEFINED;
    B->n = 0;
    B->size = 0;
    lua_unlock(L);
}

LUALIB_API char *luaL_prepbuffsize (luaL_Buffer *B, size_t sz) {
    evm_t *e = B->L->e;
    if (sz > 0) {  /* avoid 'memcpy' when 's' can be NULL */
        if( B->v == EVM_VAL_UNDEFINED ) {
            B->v = *evm_buffer_create(e, sz);
            B->size = evm_buffer_len(&B->v);
            char * old_addr = (char *)evm_buffer_addr(&B->v);
            evm_pop(e);
            B->b = old_addr;
            return old_addr;
        } else {
            if( B->size - B->n < sz ) {
                uint32_t len = evm_buffer_len(&B->v);
                evm_val_t old_v = B->v;
                B->v = *evm_buffer_create(e, sz + len);
                char * old_addr = (char*)evm_buffer_addr(&old_v);
                char * new_addr = (char*)evm_buffer_addr(&B->v);
                memcpy(new_addr, old_addr, len);
                B->size = evm_buffer_len(&B->v);
                evm_pop(e);
                B->b = new_addr;
                return new_addr + B->n;
            } else {
                return (char*)evm_buffer_addr(&B->v);
            }
        }
    }
    return NULL;
}

LUALIB_API void luaL_addlstring (luaL_Buffer *B, const char *s, size_t l) {
    evm_t *e = B->L->e;
    if (l > 0) {  /* avoid 'memcpy' when 's' can be NULL */
        if( B->v == EVM_VAL_UNDEFINED ) {
            B->v = *evm_buffer_create(e, l);
            char * old_addr = (char*)evm_buffer_addr(&B->v);
            memcpy(old_addr, s, l);
            evm_pop(e);
        } else {
            uint32_t len = evm_buffer_len(&B->v);
            evm_val_t old_v = B->v;
            B->v = *evm_buffer_create(e, l + len);
            void * old_addr = evm_buffer_addr(&old_v);
            void * new_addr = evm_buffer_addr(&B->v);
            memcpy(new_addr, old_addr, len);
            memcpy(new_addr + len, s, l);
            evm_pop(e);
        }
    }
}

LUALIB_API void luaL_addstring (luaL_Buffer *B, const char *s) {
    lua_lock(B->L);
    luaL_addlstring(B, s, strlen(s));
    lua_unlock(B->L);
}

LUALIB_API void luaL_addvalue (luaL_Buffer *B) {
    lua_State *L = B->L;
    size_t l;
    const char *s = lua_tolstring(L, -1, &l);
    luaL_addlstring(B, s, l);
    lua_remove(L, -1);  /* remove value */
}

LUALIB_API void luaL_pushresult (luaL_Buffer *B) {
    evm_push_value(B->L->e, B->v);
}

LUALIB_API void luaL_pushresultsize (luaL_Buffer *B, size_t sz) {
  luaL_pushresult(B);
  const char * str = (char*)evm_buffer_addr(&B->v);
  lua_pushlstring(B->L, str, sz + evm_buffer_len(&B->v));
}

LUALIB_API char *luaL_buffinitsize (lua_State *L, luaL_Buffer *B, size_t sz) {
    lua_lock(L);
    evm_val_t * buffer = evm_buffer_create(L->e, sz);
    B->L = L;
    B->v = *buffer;
    lua_pop(L, 1);
    lua_unlock(L);
    return (char*)evm_buffer_addr(buffer);
}

