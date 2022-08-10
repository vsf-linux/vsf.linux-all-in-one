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
#ifndef EVM_H
#define EVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "upy.h"

enum Errcode
{
    ec_ok = 0,
    ec_err,
    ec_no_file,
    ec_file_name_len,
    ec_name,
    ec_type,
    ec_memory,
    ec_zero_division,
    ec_syntax,
    ec_index,
    ec_import,
    ec_attribute,
    ec_key,
    ec_value,
    ec_system,
    ec_assertion,
    ec_overflow,
    ec_arithmetic,
    ec_indent,
    ec_gc,
    ec_exit,
};

enum GC_TYPE
{
    GC_NONE,
    GC_TOKEN,
    GC_ROOT,
    GC_OBJECT,
    GC_NATIVE_OBJECT,
    GC_DICT,
    GC_CLASS,
    GC_SET,
    GC_FUNC,
    GC_CLOSURE_FUNC,
    GC_STATIC_FUNC,
    GC_LIST,
    GC_BUFFER,
    GC_BUFFER16,
    GC_BUFFER32,
    GC_BUFFER64,
    GC_BUFFER_FLOAT,
    GC_BUFFER_DOUBLE,
    GC_BUFFER_OBJECT,
    GC_STRING,
    GC_NUMBER,
    GC_BOOLEAN,
    GC_TUPLE,
    GC_MEMBER,
    GC_MEMBER_KEYS,
    GC_MEMBER_VALS,
    GC_STRING_OBJECT,
    GC_FROZEN_OBJECT,
};

#define EVM_API UPY_API

#define EVM_VAL_NULL        TAG_NONE
#define EVM_VAL_TRUE        VAL_TRUE
#define EVM_VAL_FALSE       VAL_FALSE
#define EVM_VAL_UNDEFINED   VAL_UNDEF
#define EVM_UNUSED          UNUSED

#define evm_malloc          malloc
#define evm_free            free
#define evm_print           printf

typedef upy_t evm_t;
typedef upy_val_t evm_val_t;
typedef uint16_t evm_hash_t;
typedef int evm_err_t;
typedef int evm_type_t;

typedef struct evm_builtin_t {
    const char * name;
    evm_val_t v;
} evm_builtin_t;

static inline int evm_type(evm_val_t *v) {
    return upy_type(*v);
}

static inline double evm_2_double(evm_val_t *v) {
    return upy_2_double(*v);
}

static inline int32_t evm_2_integer(evm_val_t *v) {
    return upy_2_int32(*v);
}

static inline intptr_t evm_2_intptr(evm_val_t *v) {
    return upy_2_pointer(*v);
}

static inline void *evm_2_object(evm_val_t *v) {
    return upy_2_pointer(*v);
}

static inline int evm_2_boolean(evm_val_t *v) {
    return evm_2_intptr(v);
}

static inline int evm_is_number(evm_val_t *v) {
    return upy_is_number(*v);
}

static inline int evm_is_integer(evm_val_t *v){
    return upy_is_integer(*v);
}

static inline int evm_is_heap_string(evm_val_t *v) {
    return upy_is_heap_string(*v);
}

static inline int evm_is_foreign_string(evm_val_t *v) {
    return upy_is_foreign_string(*v);
}

static inline int evm_is_string(evm_val_t *v) {
    return evm_is_heap_string(v) || evm_is_foreign_string(v);
}

static inline int evm_is_boolean(evm_val_t *v) {
    return upy_is_boolean(*v);
}

static inline int evm_is_buffer(evm_val_t *v) {
    return upy_is_buffer(*v);
}

static inline int evm_is_script(evm_val_t *v) {
    return upy_is_script(*v);
}

static inline int evm_is_native(evm_val_t *v) {
    return upy_is_native(*v);
}

static inline int evm_is_list(evm_val_t *v) {
    return upy_is_array(*v);
}

static inline int evm_is_foreign(evm_val_t *v) {
    return upy_is_foreign(*v);
}

static inline int evm_is_function(evm_val_t *v) {
    return evm_is_script(v) || evm_is_native(v);
}

static inline int evm_is_undefined(evm_val_t *v) {
    return *v == VAL_UNDEF;
}

static inline int evm_is_null(evm_val_t *v) {
    return upy_is_none(*v);
}

static inline int evm_is_object(evm_val_t *v) {
    return upy_is_object(*v) || upy_is_module(*v);
}

static inline int evm_is_true(evm_val_t *v) {
    if( evm_is_boolean(v) && (evm_2_boolean(v) == 1 ) )
        return 1;
    return 0;
}

static inline int evm_is_false(evm_val_t *v) {
    if( evm_is_boolean(v) && (evm_2_boolean(v) == 0 ) )
        return 1;
    return 0;
}

static inline const char *evm_2_string(evm_val_t *v) {
    return upy_2_string(*v);
}

static inline evm_val_t evm_mk_number(double d) {
    return double_2_val(d);
}

static inline evm_val_t evm_mk_foreign_string(const char *s) {
    return upy_mk_string(s);
}

static inline evm_val_t evm_mk_boolean(int v) {
    return upy_mk_boolean(v);
}

static inline evm_val_t evm_mk_native(upy_native_t v) {
    return upy_mk_native(v);
}

static inline evm_val_t evm_mk_null(void) {
    return upy_mk_none();
}

static inline evm_val_t evm_mk_undefined(void) {
    return VAL_UNDEF;
}


static inline evm_val_t evm_mk_foreign(void *v) {
    return upy_mk_foreign(v);
}


static inline evm_val_t evm_mk_true(void) {
    return EVM_VAL_TRUE;
}

static inline evm_val_t evm_mk_false(void) {
    return EVM_VAL_FALSE;
}

static inline void evm_set_boolean(evm_val_t *v, int b) {
    *v = upy_mk_boolean(b);
}

static inline void evm_set_number(evm_val_t *v, double d) {
    *v = upy_mk_number(d);
}

static inline void evm_set_foreign_string(evm_val_t *v, const char *s) {
    *v = upy_mk_string(s);
}

static inline void evm_set_native(evm_val_t *v, upy_native_t s) {
    *v = upy_mk_native(s);
}

static inline void evm_set_undefined(evm_val_t *v) {
    *v = VAL_UNDEF;
}

static inline void evm_set_null(evm_val_t *v) {
    *v = upy_mk_none();
}

static inline void evm_set_foreign(evm_val_t *v, void *s) {
    *v = upy_mk_foreign(s);
}

/*** 字符串常量操作函数 ***/
EVM_API evm_hash_t evm_str_insert(evm_t *e, const char *str, int alloc);
EVM_API const char *evm_string_get(evm_t * e, evm_hash_t key);

/*** 字符串对象操作函数 ***/
EVM_API evm_val_t *evm_heap_string_create(evm_t *e, const char *str, int len);
EVM_API char *evm_heap_string_addr(evm_val_t * v);
EVM_API int evm_string_len(evm_val_t * o);

/*** 字节数组对象操作函数 ***/
EVM_API evm_val_t *evm_buffer_create(evm_t *e, int len);
EVM_API uint8_t *evm_buffer_addr(evm_val_t *o);
EVM_API int evm_buffer_len(evm_val_t *o);

/*** 列表对象操作函数 ***/
EVM_API evm_val_t *evm_list_create(evm_t *e, evm_type_t type, int len);
EVM_API int evm_list_len(evm_val_t *o);
EVM_API evm_val_t evm_list_get(evm_t *e, evm_val_t *o, int index);
EVM_API void evm_list_set(evm_t *e, evm_val_t *o, int index, evm_val_t v);

/*** 对象操作函数 ***/
EVM_API evm_val_t *evm_object_create(evm_t *e, evm_type_t type);
EVM_API evm_val_t *evm_object_create_ext_data(evm_t *e, evm_type_t type, void *ext_data);
EVM_API void *evm_object_get_ext_data(evm_val_t *o);
EVM_API void evm_object_set_ext_data(evm_val_t *o, void *ext_data);

/*** 成员操作函数 ***/
EVM_API int evm_prop_len(evm_val_t * o);
EVM_API evm_err_t evm_prop_set(evm_t *e, evm_val_t *o, const char *key, evm_val_t v);
EVM_API evm_val_t evm_prop_get(evm_t *e, evm_val_t *o, const char* key);
EVM_API evm_val_t evm_prop_get_by_index(evm_t *e, evm_val_t *o, int index);
EVM_API void evm_prop_set_value_by_index(evm_t *e, evm_val_t *o, int index, evm_val_t v);

/*** 模块操作函数 ***/
EVM_API void evm_module_add(evm_t *e, const char* name, evm_val_t *v);
EVM_API evm_val_t evm_module_get(evm_t *e, const char* name);

EVM_API evm_val_t evm_run_callback(evm_t * e, evm_val_t * scope, evm_val_t *this, evm_val_t *args, int argc);
EVM_API void evm_set_err(evm_t * e, evm_err_t err, const char *arg);
EVM_API evm_val_t evm_run_eval(evm_t * e,  evm_val_t *object, char * string);
EVM_API evm_val_t evm_run_string(evm_t * e, const char * content);
EVM_API evm_err_t evm_native_add(evm_t * e, evm_builtin_t *n);

/*** 虚拟机栈操作 ***/
EVM_API void evm_push_null(evm_t * e);
EVM_API void evm_push_undefined(evm_t * e);
EVM_API void evm_push_number(evm_t * e, double v);
EVM_API void evm_push_integer(evm_t * e, int32_t v);
EVM_API void evm_push_boolean(evm_t * e, int v);
EVM_API evm_val_t *evm_push_buffer(evm_t * e, uint8_t * buf, int len);
EVM_API evm_val_t *evm_push_foreign_string(evm_t * e, const char * s);
EVM_API evm_val_t *evm_push_heap_string(evm_t * e, const char * s);
EVM_API evm_val_t *evm_push_value(evm_t *e, evm_val_t v);
EVM_API void evm_pop(evm_t *e);
EVM_API void evm_prop_push(evm_t * e, evm_val_t *o, const char *key, evm_val_t *v);

EVM_API void evm_lock(evm_t *e);
EVM_API void evm_unlock(evm_t *e);

static inline void evm_assert_fail (const char *assertion, const char *file, const char *function, const uint32_t line){
    evm_print ("AssertionError: '%s' failed at %s(%s):%lu.\n",
                       assertion,
                       file,
                       function,
                       (unsigned long) line);

    while(1);
}

#define EVM_ASSERT(x) \
  do \
  { \
    if (! (x) ) \
    { \
      evm_assert_fail (#x, __FILE__, __func__, __LINE__); \
    } \
  } while (0)

#endif

#ifdef __cplusplus
} /* extern "C" */

#endif

