#include "lua.h"

static evm_t * env = NULL;

static evm_val_t foreign_caller(evm_t *e, evm_val_t p, int argc, evm_val_t * v){
    if( !evm_is_foreign(&p) )
        return EVM_VAL_UNDEFINED;
    lua_State L;
    lua_copystate(&L, &p, argc, v);
    lua_CFunction func = (lua_CFunction)evm_2_intptr(&p);
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

LUA_API lua_State *lua_newstate (lua_Alloc f, void *args) {
    lua_set_allocf(f, args);
    env = upy_init(200);
    upy_foreign_caller_set(env, foreign_caller);
    return lua_new_state(env);
}


