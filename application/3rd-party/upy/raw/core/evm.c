/*
 * This file is part of the Micropy project.
 *
 * Apache License
 *
 * Copyright (c) 2022 Zhe Wang zhewang_edmund@outlook.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "evm.h"

evm_hash_t evm_str_insert(evm_t *e, const char *str, int alloc) {
    return upy_strpool_insert(e, str, 1);
}

const char *evm_string_get(evm_t * e, evm_hash_t key) {
    return upy_key_2_str(e, key);
}

evm_val_t *evm_heap_string_create(evm_t *e, const char *str, int len) {
    *(++e->sp) = upy_lstring_create(e, str, len);
    return e->sp;
}

char *evm_heap_string_addr(evm_val_t * v) {
    return upy_2_heap_string(*v);
}

int evm_string_len(evm_val_t * o) {
    return upy_string_size(*o);
}

evm_val_t *evm_buffer_create(evm_t *e, int len) {
    *(++e->sp) = upy_buffer_create_empty(e, len);
    return e->sp;
}

uint8_t *evm_buffer_addr(evm_val_t *o) {
    return upy_2_buffer(*o);
}

int evm_buffer_len(evm_val_t *o) {
    return upy_buffer_size(*o);
}

evm_val_t *evm_list_create(evm_t *e, evm_type_t type, int len) {
    *(++e->sp) = upy_array_create(e);
    upy_array_set(e, *e->sp, len - 1, VAL_UNDEF);
    return e->sp;
}

int evm_list_len(evm_val_t *o) {
    return upy_array_size(*o);
}

evm_val_t evm_list_get(evm_t *e, evm_val_t *o, int index) {
    return upy_array_get(e, *o, index);
}

void evm_list_set(evm_t *e, evm_val_t *o, int index, evm_val_t v) {
    upy_array_set(e, *o, index, v);
}

evm_val_t *evm_object_create(evm_t *e, evm_type_t type) {
    if( type == GC_ROOT)
        *(++e->sp) = upy_module_create(e);
    else
        *(++e->sp) = upy_object_create(e);
    return e->sp;
}

evm_val_t *evm_object_create_ext_data(evm_t *e, evm_type_t type, void* ext_data) {
    *(++e->sp) = upy_object_create(e);
    upy_object_set_user_data(*e->sp, ext_data);
    return e->sp;
}

void *evm_object_get_ext_data(evm_val_t *o) {
    return upy_object_get_user_data(*o);
}

void evm_object_set_ext_data(evm_val_t *o, void *ext_data) {
    upy_object_set_user_data(*o, ext_data);
}

int evm_prop_len(evm_val_t * o) {
    return upy_object_size(*o);
}

void evm_module_add(evm_t *e, const char* name, evm_val_t *v) {
    upy_val_t res = upy_global_get_cs(e, "__modules__");
    upy_object_set_cs(e, res, name, *v);
}

evm_val_t evm_module_get(evm_t *e, const char* name) {
    upy_val_t res = upy_global_get_cs(e, "__modules__");
    res = upy_object_get_cs(e, res, name);
    return res;
}

void evm_push_null(evm_t * e) {
    upy_push(e, upy_mk_none());
}

void evm_push_undefined(evm_t * e) {
    upy_push(e, VAL_UNDEF);
}

void evm_push_number(evm_t * e, double v) {
    upy_push(e, upy_mk_number(v));
}

void evm_push_integer(evm_t * e, int32_t v) {
    upy_push(e, upy_mk_number(v));
}

void evm_push_boolean(evm_t * e, int v) {
    upy_push(e, upy_mk_boolean(v));
}

evm_val_t *evm_push_buffer(evm_t * e, uint8_t * buf, int len) {
    *(++e->sp) = upy_buffer_create(e, buf, len);
    return e->sp;
}

evm_val_t *evm_push_foreign_string(evm_t * e, const char * s) {
    upy_push(e, upy_mk_string(s));
}

evm_val_t *evm_push_heap_string(evm_t * e, const char * s) {
    *(++e->sp) = upy_string_create(e, s);
    return e->sp;
}

evm_val_t *evm_push_value(evm_t *e, evm_val_t v) {
    upy_push(e, v);
}

void evm_pop(evm_t *e) {
    upy_pop(e);
}

void evm_prop_push(evm_t * e, evm_val_t *o, const char *key, evm_val_t *v) {
    upy_object_set_cs(e, *o, key, *v);
    upy_pop(e);
}

evm_val_t evm_run_callback(evm_t * e, evm_val_t * scope, evm_val_t *this, evm_val_t *args, int argc) {
    return upy_call(e, *scope, *this, argc, args);
}

void evm_set_err(evm_t * e, evm_err_t err, const char *arg) {
    upy_throw_str(e, arg);
}

evm_val_t evm_run_eval(evm_t * e,  evm_val_t *object, char * string) {
    evm_val_t res = upy_parse_string(e, string);
    return upy_call(e, res, *object, 0, NULL);
}

evm_val_t evm_run_string(evm_t * e, const char * content) {
    evm_val_t res = upy_parse_string(e, content);
    return upy_call(e, res, res, 0, NULL);
}

evm_err_t evm_native_add(evm_t * e, evm_builtin_t *n) {
    if( n == NULL) return ec_ok;
    uint32_t i = 0;
    while(1){
        if(n[i].name == NULL)
            break;
        upy_global_set_cs(e, n[i].name, n[i].v);
        i++;
    }
    return ec_ok;
}

evm_err_t evm_prop_set(evm_t *e, evm_val_t *o, const char *key, evm_val_t v) {
    upy_object_set_cs(e, *o, key, v);
    return ec_ok;
}

evm_val_t evm_prop_get(evm_t *e, evm_val_t *o, const char* key) {
    return upy_object_get_cs(e, *o, key);
}

evm_val_t evm_prop_get_by_index(evm_t *e, evm_val_t *o, int index) {
    if( !evm_is_object(o) )
        return VAL_UNDEF;
    upy_object_t *obj = evm_2_object(o);
    upy_member_t *next = obj->member;
    int i = 0;
    while(next) {
        if( i == index )
            return next->val;
        next = next->next;
        i++;
    }
    return VAL_UNDEF;
}

void evm_prop_set_value_by_index(evm_t *e, evm_val_t *o, int index, evm_val_t v) {
    if( !evm_is_object(o) )
        return;
    upy_object_t *obj = evm_2_object(o);
    upy_member_t *next = obj->member;
    int i = 0;
    while(next) {
        if( i == index ) {
            next->val = v;
            return;
        }
        next = next->next;
        i++;
    }
}

void evm_lock(evm_t *e) {

}

void evm_unlock(evm_t *e) {

}

