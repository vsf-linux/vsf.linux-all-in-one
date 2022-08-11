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
#ifndef UPY_H
#define UPY_H


#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <setjmp.h>

enum Bytecode
{
    op_nop = 0,
    op_get_local,
    op_get_var,
    op_get_number,
    op_get_none,
    op_get_true,
    op_get_false,
    op_get_string,
    op_set_local,
    op_set_var,
    op_add,
    op_sub,
    op_mul,
    op_div,
    op_mod,
    op_and,
    op_or,
    op_xor,
    op_positive,
    op_negative,
    op_not,
    op_invert,
    op_left_shift,
    op_right_shift,
    op_logic_and,
    op_logic_or,
    op_call,
    op_return,
    op_return_void,
    op_lt,
    op_gt,
    op_lteq,
    op_gteq,
    op_eq,
    op_neq,
    op_sdn,
    op_sup,
    op_pop,
    op_dup,
    op_prop_get,
    op_prop_set,
    op_prop_call,
    op_get_array,
    op_set_array,
    op_branch,
    op_branch_true,
    op_branch_false,
    op_jmp,
    op_jmp_true,
    op_jmp_false,
    op_dict,
    op_array,
    op_self,
    op_line,
    op_label,
    op_try,
    op_catch,
    op_throw,
    op_iter,
    op_next,
    op_import,
    op_stop,
};

enum UPY_TYPE
{
    TYPE_NUMBER = 0,
    TYPE_HEAP_STRING,
    TYPE_FOREIGN_STRING,
    TYPE_BOOLEAN,
    TYPE_FUNCTION,
    TYPE_NATIVE,
    TYPE_UNDEF,
    TYPE_NONE,
    TYPE_NAN,
    TYPE_CLASS,
    TYPE_MODULE,
    TYPE_OBJECT,
    TYPE_ARRAY,
    TYPE_BUFFER,
    TYPE_FOREIGN,
};

typedef uint64_t upy_val_t;

typedef union {
    upy_val_t   v;
    double      d;
} valnum_t;

#define UPY_API extern
#define UPY_ARRAY_EXPAND_SIZE   8
#define UNUSED(x) (void)x
#define UPY_STRING_POOL_SIZE    256
#define UPY_VAR_NAME_LEN        128
#define UPY_CODE_BUF_LEN        64
#define UPY_TRYBUF_LIMIT        50
#define UPY_STACK_FACTOR        0.8

#define upy_malloc      malloc
#define upy_free        free
#define upy_realloc     realloc

#define MAKE_TAG(s, t) \
  ((upy_val_t)(s) << 63 | (upy_val_t) 0x7ff0 <<48 | (upy_val_t)(t) <<48)

#define TAG_INFINITE        MAKE_TAG(0, TYPE_NUMBER)
#define TAG_STRING_H        MAKE_TAG(1, TYPE_HEAP_STRING)
#define TAG_STRING_F        MAKE_TAG(1, TYPE_FOREIGN_STRING)
#define TAG_BOOLEAN         MAKE_TAG(1, TYPE_BOOLEAN)
#define TAG_FUNC_SCRIPT     MAKE_TAG(1, TYPE_FUNCTION)
#define TAG_FUNC_NATIVE     MAKE_TAG(1, TYPE_NATIVE)
#define TAG_MODULE          MAKE_TAG(1, TYPE_MODULE)
#define TAG_FOREIGN         MAKE_TAG(1, TYPE_FOREIGN)

#define TAG_ARRAY           MAKE_TAG(1, TYPE_ARRAY)
#define TAG_NONE            MAKE_TAG(1, TYPE_NONE)

#define TAG_OBJECT          MAKE_TAG(1, TYPE_OBJECT)
#define TAG_FOREIGN         MAKE_TAG(1, TYPE_FOREIGN)
#define TAG_CLASS           MAKE_TAG(1, TYPE_CLASS)
#define TAG_BUFFER          MAKE_TAG(1, TYPE_BUFFER)
#define TAG_UNDEF           MAKE_TAG(1, TYPE_UNDEF)

#define NUM_MASK            MAKE_TAG(0, 0xF)
#define TAG_MASK            MAKE_TAG(1, 0xF)
#define VAL_MASK            (~MAKE_TAG(1, 0xF))

#define VAL_TRUE            (TAG_BOOLEAN | 1)
#define VAL_FALSE           (TAG_BOOLEAN)
#define VAL_UNDEF           (TAG_UNDEF)

#define double_2_val(d) (((valnum_t*)&(d))->v)

typedef struct upy_buffer_t{
    int size;
    uint8_t * buf;
} upy_buffer_t;

typedef struct upy_array_t{
    int size;
    int count;
    upy_val_t *vals;
} upy_array_t;

typedef upy_buffer_t upy_string_t;

typedef struct upy_member_t {
    upy_val_t name;
    upy_val_t val;
    struct upy_member_t *next;
} upy_member_t;

typedef struct upy_property_t {
    const char *name;
    struct upy_property_t *next;
} upy_property_t;


typedef struct upy_function_t {
    uint16_t nparams;
    uint16_t nvars;
    int code_size;
    uint8_t *code;
    struct upy_object_t *scope;
} upy_function_t;

typedef struct upy_object_t {
    uint8_t type;
    uint8_t mark;
    upy_member_t *member;
    union {
        upy_string_t *s;
        upy_buffer_t *b;
        upy_function_t *f;
        upy_array_t *a;
    } u;
    union {
        upy_property_t *locals;
        struct upy_object_t *parent;
    } p;
    void *user_data;
    struct upy_object_t *next;
} upy_object_t;

typedef struct upy_trybuf_t
{
    jmp_buf buf;
    upy_val_t scope;
    upy_val_t *sp;
    int pc;
} upy_trybuf_t;


typedef struct upy_t{
    char name[UPY_VAR_NAME_LEN];
    char *input;
    int input_len;
    int index;
    int row;
    int indent;
    double num;

    upy_val_t *sp;
    upy_val_t *sp_base;
    upy_val_t *sp_high;
    size_t stack_size;

    intptr_t *str_table;
    int str_table_count;

    upy_val_t scope;
    upy_val_t globals;
    void *gcroot;
    void *gcnext;

    int trybuf_count;
    upy_trybuf_t trybuf[UPY_TRYBUF_LIMIT];
    void *string_pool;
    int pause;

    void *foreign_caller;
} upy_t;

typedef upy_val_t (*upy_native_t)(upy_t *e, upy_val_t self, int c, upy_val_t *v);

static inline double upy_2_double(upy_val_t v) {
    return ((valnum_t)v).d;
}

static inline int32_t upy_2_int32(upy_val_t v) {
    return (int32_t)((valnum_t)v).d;
}

static inline uint32_t upy_2_uint32(upy_val_t v) {
    return (uint32_t)((valnum_t)v).d;
}

static inline void *upy_2_pointer(upy_val_t v) {
    return (void*)(v & VAL_MASK);
}

static inline int upy_2_boolean(upy_val_t v) {
    return (int)(v & VAL_MASK);
}

static inline int upy_is_number(upy_val_t v) {
    return (v & TAG_INFINITE) != TAG_INFINITE;
}

static inline int upy_is_integer(upy_val_t v) {
    if(!upy_is_number(v)) return 0;
    if(upy_2_double(v) - upy_2_int32(v) == 0) return 1;
    return 0;
}

static inline int upy_is_heap_string(upy_val_t v) {
    return (v & TAG_MASK) == TAG_STRING_H;
}

static inline int upy_is_foreign_string(upy_val_t v) {
    return (v & TAG_MASK) == TAG_STRING_F;
}

static inline int upy_is_string(upy_val_t v) {
    return upy_is_heap_string(v) || upy_is_foreign_string(v);
}

static inline int upy_is_boolean(upy_val_t v) {
    return (v & TAG_MASK) == TAG_BOOLEAN;
}

static inline int upy_is_script(upy_val_t v) {
    return (v & TAG_MASK) == TAG_FUNC_SCRIPT;
}

static inline int upy_is_native(upy_val_t v) {
    return (v & TAG_MASK) == TAG_FUNC_NATIVE;
}

static inline int upy_is_array(upy_val_t v) {
    return (v & TAG_MASK) == TAG_ARRAY;
}

static inline int upy_is_function(upy_val_t v) {
    return upy_is_script(v) || upy_is_native(v);
}

static inline int upy_is_buffer(upy_val_t v) {
    return (v & TAG_MASK) == TAG_BUFFER;
}

static inline int upy_is_none(upy_val_t v) {
    return (v & TAG_MASK) == TAG_NONE;
}

static inline int upy_is_object(upy_val_t v) {
    return (v & TAG_MASK) == TAG_OBJECT;
}

static inline int upy_is_module(upy_val_t v) {
    return (v & TAG_MASK) == TAG_MODULE;
}

static inline int upy_is_class(upy_val_t v) {
    return (v & TAG_MASK) == TAG_CLASS;
}

static inline int upy_is_foreign(upy_val_t v) {
    return (v & TAG_MASK) == TAG_FOREIGN;
}

static inline upy_val_t upy_mk_number(double d) {
    return double_2_val(d);
}

static inline upy_val_t upy_mk_string(const char *s) {
    return TAG_STRING_F | (uint64_t)s;
}

static inline upy_val_t upy_mk_boolean(int v) {
    return TAG_BOOLEAN | (!!v);
}

static inline upy_val_t upy_mk_native(upy_native_t n) {
    return TAG_FUNC_NATIVE | (uint64_t)n;
}

static inline upy_val_t upy_mk_foreign(void *v) {
    return TAG_FOREIGN | (uint64_t)v;
}

static inline upy_val_t upy_mk_undef(void) {
    return TAG_UNDEF;
}

static inline upy_val_t upy_mk_none(void) {
    return TAG_NONE;
}

static inline upy_val_t upy_mk_true(void) {
    return VAL_TRUE;
}

static inline upy_val_t upy_mk_false(void) {
    return VAL_FALSE;
}

/*** exception manipulation ***/
#if defined (WIN32) || defined (WIN64)
#define upy_setjmp __builtin_setjmp
#define upy_longjmp __builtin_longjmp
#define upy_try(e) __builtin_setjmp(upy_savetrypc(e, VAL_UNDEF, 0))
#else
#define upy_try(e) setjmp(upy_savetrypc(e, VAL_UNDEF, 0))
#define upy_setjmp setjmp
#define upy_longjmp longjmp
#endif
UPY_API void *upy_savetrypc(upy_t *e, upy_val_t scope, int pc);
UPY_API void upy_throw(upy_t *e, upy_val_t error);
UPY_API void upy_throw_str(upy_t *e, const char *str);
/*** end of exception manipulation **/

/*** basic value manipulation ***/
UPY_API int upy_type(upy_val_t v);
UPY_API upy_val_t upy_add(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_sub(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_mul(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_div(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_mod(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_and(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_or(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_xor(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_negative(upy_t *e, upy_val_t v);
UPY_API upy_val_t upy_not(upy_t *e, upy_val_t v);
UPY_API upy_val_t upy_invert(upy_t *e, upy_val_t v);
UPY_API upy_val_t upy_left_shift(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_right_shift(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_logic_and(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API upy_val_t upy_logic_or(upy_t *e, upy_val_t v1, upy_val_t v2);
UPY_API int upy_compare(upy_t *e, upy_val_t v1, upy_val_t v2);
/*** end of basic value manipulation ***/

/*** object manipulation ***/
UPY_API upy_val_t upy_module_create(upy_t *e);
UPY_API upy_val_t upy_object_create(upy_t *e);
UPY_API int upy_object_size(upy_val_t o);
UPY_API upy_val_t upy_object_duplicate(upy_t *e, upy_val_t o);
UPY_API upy_val_t upy_class_create(upy_t *e);
UPY_API upy_val_t upy_object_get(upy_t *e, upy_val_t o, upy_val_t name);
UPY_API void upy_object_set(upy_t *e, upy_val_t o, upy_val_t name, upy_val_t v);
UPY_API upy_val_t upy_object_delete(upy_t *e, upy_val_t o, upy_val_t name);
UPY_API upy_val_t upy_object_get_cs(upy_t *e, upy_val_t o, const char *name);
UPY_API void upy_object_set_cs(upy_t *e, upy_val_t o, const char *name, upy_val_t v);
UPY_API upy_val_t upy_object_delete_cs(upy_t *e, upy_val_t o, const char *s);
UPY_API void upy_object_set_user_data(upy_val_t o, void *data);
UPY_API void *upy_object_get_user_data(upy_val_t o);
UPY_API void upy_object_set_parent(upy_val_t o, upy_val_t parent);
UPY_API upy_val_t upy_object_get_parent(upy_val_t o);
/*** end of object manipulation ***/

/*** array manipulation ***/
UPY_API upy_val_t upy_array_create(upy_t *e);
UPY_API int upy_array_size(upy_val_t o);
UPY_API upy_val_t upy_array_get(upy_t *e, upy_val_t o, int index);
UPY_API upy_val_t upy_array_set(upy_t *e, upy_val_t o, int index, upy_val_t v);
UPY_API upy_val_t upy_array_delete(upy_t *e, upy_val_t o, int index);
/*** end of array manipulation ***/

/*** string object manipulation ***/
UPY_API upy_val_t upy_string_create(upy_t *e, const char *s);
UPY_API upy_val_t upy_lstring_create(upy_t *e, const char *s, int len);
UPY_API int upy_string_size(upy_val_t o);
UPY_API const char *upy_2_string(upy_val_t v);
UPY_API char *upy_2_heap_string(upy_val_t v);
/*** end of string object manipulation ***/

/*** buffer object manipulation ***/
UPY_API upy_val_t upy_buffer_create(upy_t *e, uint8_t *buf, int size);
UPY_API upy_val_t upy_buffer_create_empty(upy_t *e, int size);
UPY_API int upy_buffer_size(upy_val_t o);
UPY_API uint8_t* upy_2_buffer(upy_val_t o);
/*** end of buffer object manipulation ***/

/*** vm manipulation ***/
UPY_API upy_t *upy_init(size_t stack_size);
UPY_API void upy_deinit(upy_t *e);
UPY_API upy_val_t upy_parse(upy_t *e, const char *file);
UPY_API upy_val_t upy_parse_string(upy_t *e, char *content);
/*** end of vm manipulation ***/

/*** stack manipulation ***/
UPY_API void upy_push(upy_t *e, upy_val_t v);
UPY_API upy_val_t upy_pop(upy_t *e);
/*** end of stack manipulation ***/

/*** string pool manipulation ***/
UPY_API const char *upy_key_2_str(upy_t *e, uint16_t key);
UPY_API uint16_t upy_strpool_insert(upy_t *e, const char *str, int alloc);
/*** end of string pool manipulation ***/

/*** global values manipulation ***/
UPY_API upy_val_t upy_global_get(upy_t *e, upy_val_t name);
UPY_API void upy_global_set(upy_t *e, upy_val_t name, upy_val_t v);
UPY_API upy_val_t upy_global_delete(upy_t *e, upy_val_t name);
UPY_API upy_val_t upy_global_get_cs(upy_t *e, const char *name);
UPY_API void upy_global_set_cs(upy_t *e, const char *name, upy_val_t v);
UPY_API upy_val_t upy_global_delete_cs(upy_t *e, const char *name);
/*** end of global values manipulation ***/

/*** function call manipulation ***/
UPY_API upy_val_t upy_call(upy_t *e, upy_val_t script, upy_val_t self, int argc, upy_val_t *values);
void upy_foreign_caller_set(upy_t *e, upy_native_t caller);
/*** end of function call manipulation ***/
#ifdef __cplusplus
} /* extern "C" */
#endif


#endif
