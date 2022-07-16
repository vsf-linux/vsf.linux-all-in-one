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

#include "upy.h"

enum TOKEN {
    TOK_VAR = 2,
    TOK_INDEX_SUFFIX,
    TOK_PROP_SUFFIX,
    TOK_CALL
};

typedef struct string_pool_t{
    int count;
    intptr_t *values;
    struct string_pool_t * next;
} string_pool_t;

#define SAVE int row = p->row; int index = p->index; int buf_index = compile_buf_index;
#define RESTORE p->row = row; p->index = index; compile_buf_index = buf_index;

static uint8_t * compile_buf;
static int compile_buf_size;
static int compile_buf_index;
static upy_val_t curr_scope;
static upy_val_t upy_run(upy_t *e, upy_object_t *script, upy_object_t *self, upy_val_t *values);

static void code_buf_init(void) {
    compile_buf = upy_malloc(UPY_CODE_BUF_LEN);
    compile_buf_size = UPY_CODE_BUF_LEN;
}

static void code_buf_upy_free(void) {
    upy_free(compile_buf);
}

static void code_buf_check_extend(int index) {
    if( index < compile_buf_size)
        return;
    uint8_t *old = compile_buf;
    compile_buf = upy_malloc((size_t)(compile_buf_size + UPY_CODE_BUF_LEN));
    assert(compile_buf);
    memcpy(compile_buf, old, (size_t)compile_buf_index);
    compile_buf_size += UPY_CODE_BUF_LEN;
    upy_free(old);
}

static upy_val_t mk_class(upy_object_t *o) {
    return TAG_CLASS | (uint64_t)o;
}

static upy_val_t mk_object(upy_object_t *o) {
    return TAG_OBJECT | (uint64_t)o;
}

static upy_val_t mk_heap_string(upy_object_t *o) {
    return TAG_STRING_H | (uint64_t)o;
}

static upy_val_t mk_buffer(upy_object_t *o) {
    return TAG_BUFFER | (uint64_t)o;
}

static upy_val_t mk_script(upy_object_t *o) {
    return TAG_FUNC_SCRIPT | (uint64_t)o;
}

static upy_val_t mk_module(upy_object_t *o) {
    return TAG_MODULE | (uint64_t)o;
}

const char *upy_2_string(upy_val_t v) {
    if( upy_is_foreign_string(v)){
        return upy_2_pointer(v);
    } else if( upy_is_heap_string(v)){
        upy_object_t *obj = upy_2_pointer(v);
        return (const char*)obj->u.s->buf;
    }
    return NULL;
}

char *upy_2_heap_string(upy_val_t v) {
    if( upy_is_heap_string(v)){
        upy_object_t *obj = upy_2_pointer(v);
        return (char*)obj->u.s->buf;
    }
    return NULL;
}

static void add_gc_object(upy_t *e, upy_object_t *obj) {
    if( e->gcroot == NULL ) {
        e->gcnext = e->gcroot = obj;
    } else {
        ((upy_object_t*)e->gcnext)->next = obj;
        e->gcnext = obj;
    }
}

static upy_object_t *object_create(upy_t *e, uint8_t type) {
    upy_object_t *obj = upy_malloc(sizeof(upy_object_t));
    assert(obj);
    memset(obj, 0, sizeof(upy_object_t));
    obj->type = type;
    add_gc_object(e, obj);
    return obj;
}

static upy_val_t function_create(upy_t *e) {
    upy_object_t *obj = object_create(e, TYPE_FUNCTION);
    obj->u.f = upy_malloc(sizeof (upy_function_t));
    assert(obj->u.a);
    memset(obj->u.a, 0, sizeof (upy_function_t));
    return mk_script(obj);
}

static upy_val_t module_create(upy_t *e) {
    upy_object_t *obj = object_create(e, TYPE_MODULE);
    obj->u.f = upy_malloc(sizeof (upy_function_t));
    assert(obj->u.a);
    memset(obj->u.a, 0, sizeof (upy_function_t));
    return mk_module(obj);
}

static int16_t get_int16(uint8_t * buf){
    union{
        int16_t i;
        char buf[2];
    }u;
    memcpy(u.buf, buf, 2);
    return u.i;
}

static uint16_t get_uint16(uint8_t * buf){
    union{
        uint16_t i;
        char buf[2];
    }u;
    memcpy(u.buf, buf, 2);
    return u.i;
}

static int32_t get_int32(uint8_t * buf){
    union{
        int32_t i;
        char buf[4];
    }u;
    memcpy(u.buf, buf, 4);
    return u.i;
}

static void set_int32(uint8_t * buf, int v){
    union{
        int i;
        char buf[4];
    }u;
    u.i = v;
    memcpy(buf, u.buf, 4);
}

static double get_double(uint8_t * buf){
    union{
        double i;
        char buf[8];
    }u;
    memcpy(u.buf, buf, 8);
    return u.i;
}

static int bc_add_8(uint8_t op)
{
    code_buf_check_extend(compile_buf_index + 1);
    compile_buf[compile_buf_index++] = op;
    return compile_buf_index - 1;
}

static int bc_add_16(uint16_t oprand)
{
    code_buf_check_extend(compile_buf_index + 2);
    union {
        uint16_t i;
        char c[2];
    } v;
    v.i = oprand;
    memcpy(compile_buf + compile_buf_index, v.c, 2);
    compile_buf_index += 2;
    return compile_buf_index - 2;
}

static int bc_add_32(int oprand)
{
    code_buf_check_extend(compile_buf_index + 4);
    union {
        int i;
        char c[4];
    } v;
    v.i = oprand;
    memcpy(compile_buf + compile_buf_index, v.c, 4);
    compile_buf_index += 4;
    return compile_buf_index - 4;
}

static int bc_add_double(double oprand)
{
    code_buf_check_extend(compile_buf_index + 8);
    union {
        double i;
        char c[8];
    } v;
    v.i = oprand;
    memcpy(compile_buf + compile_buf_index, v.c, 8);
    compile_buf_index += 8;
    return compile_buf_index - 8;
}

static void bc_set_32(int addr, int oprand)
{
    union {
        int i;
        char c[4];
    } v;
    v.i = oprand;
    memcpy(compile_buf + addr, v.c, 4);
}

static void clear_locals(upy_object_t *object) {
    if( !object->p.locals )
        return;
    upy_property_t *prop = object->p.locals;
    while(prop) {
        upy_property_t *local = prop;
        prop = prop->next;
        upy_free(local);
    }
    object->p.locals = NULL;
}

static void add_locals(upy_object_t *object, const char *name) {
    if( object->p.locals == NULL ) {
        object->p.locals = upy_malloc(sizeof (upy_property_t));
        assert(object->p.locals);
        object->p.locals->name = name;
        object->u.f->nvars++;
    } else {
        upy_property_t *next = object->p.locals;
        for(;;) {
            if( next->name == name ) {
                return;
            } else if( !next->next ) {
                next->next = upy_malloc(sizeof (upy_property_t));
                next->next->name = name;
                object->u.f->nvars += 1;
                return;
            }
            next = next->next;
        }
    }
}

static int find_locals(upy_object_t *object, const char *name, int *gen) {
    if( !object || !object->p.locals ) {
        return -1;
    }
    int pos = -1;
    upy_property_t *prop = object->p.locals;
    while(prop) {
        pos += 1;
        if( prop->name == name ) {
            return pos;
        }
        prop = prop->next;
    }
    (*gen)++;
    return find_locals(object->u.f->scope, name, gen);
}

static int parser_is_hex(int c)
{
    if( isdigit(c) )
        return 1;
    if( c == 'x' || c == 'X')
        return 1;
    if( c >= 'a' && c <= 'f')
        return 1;
    if( c >= 'A' && c <= 'F')
        return 1;
    return 0;
}

static int parser_token(upy_t *p, char *str)
{
    if( !memcmp(p->input + p->index, str, strlen(str)) ) {
        return 1;
    }
    return 0;
}

static int parser_token_eat(upy_t *p, char *str)
{
    if( !memcmp(p->input + p->index, str, strlen(str)) ) {
        p->index += strlen(str);
        return 1;
    }
    return 0;
}

static char parser_next_ch(upy_t *p, int index)
{
    return p->input[index];
}

int parser_Identifier(upy_t *p, char * name)
{
    char ch;
    int index;
    int len = 0;

    if(isdigit(parser_next_ch(p, p->index)))
        return 0;

    while(1) {
        index = p->index;
        ch = parser_next_ch(p, index);

        if( isalnum(ch) || '_' == ch || '$' == ch ) {
            p->index++;
            name[len] = ch;
            len++;
        } else {
            if(len)
            {
                name[len] = 0;
                return 1;
            }
            else
                return 0;
        }
    }
}

static int hex_to_char(char ch)
{
    int n;
    if (tolower(ch) > '9')
        n = (10 + tolower(ch) - 'a');
    else
        n = (tolower(ch) - '0');
    return n;
}

static int check_escape(char *input) {
    char ch = input[0];
    if( ch == '\\' ) {
        ch = input[1];
        switch(ch) {
        case 'b': return '\b';
        case 'x': return 'x';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'r': return '\r';
        case 'n': return '\n';
        case 't': return '\t';
        case '\'':return '\'';
        case '\\':return '\\';
        case '0':return '\0';
        case '"': return '"';
        default:
            return -1;
        }
    }
    return -1;
}

static int check_str_len(upy_t *p){
    uint8_t ch = 0;
    int len = 0;
    char *input = (char*)p->input + p->index;
    int doubleQuote = -1;
    if( input[0] == '"' ) {
        doubleQuote = 1;
    } else if( input[0] == '\'') {
        doubleQuote = 0;
    }
    char flag = 1;
    int escape_flag = 1;
    if( doubleQuote >= 0 ) {
        input++;
        do {
            if( input[0] == 0 )
                return -1;

            if( check_escape(input) == -1) {
                ch = (uint8_t)input[0];
                if( ch == '"' && doubleQuote == 0 ){
                    escape_flag = !escape_flag;
                } else if ( ch == '\'' && doubleQuote == 1) {
                    escape_flag = !escape_flag;
                }
                if( ch == '\\') {
                    input += 1;
                } else {
                    len += 1;
                    input++;
                }
            } else {
                ch = (uint8_t)input[0];
                input += 2;
                if( escape_flag == 1)
                    len+=1;
                else
                    len += 2;
            }

            if( ch == 0x0D || ch == 0x0A ){
                return -1;
            }

            if( doubleQuote == 1 && ch == '"')
                flag = 0;

            if( doubleQuote == 0 && ch == '\'')
                flag = 0;
        } while( flag );
        return len;
    }
    return -1;
}


char * parser_StringLiteral(upy_t *p)
{
    char ch;
    int escape;
    char flag = 1;
    int len = 0;
    len = check_str_len(p);
    if( len == -1 )
        return NULL;
    char *str = (char*)upy_malloc((size_t)len);
    if( !str ) {
        upy_throw(p, upy_mk_string("Insufficient memory"));
        return NULL;
    }
    memset(str, 0, (size_t)len);
    len = 0;
    char *input = p->input + p->index;
    int doubleQuote = -1;
    if( input[0] == '"' ) {
        doubleQuote = 1;
    } else if( input[0] == '\'') {
        doubleQuote = 0;
    }
    int escape_flag = 1;
    if( doubleQuote >= 0 ) {
        input++;
        p->index++;
        do {
            escape = check_escape(input);
            if( escape == -1 ) {
                ch = input[0];
                if( ch == '"' && doubleQuote == 0 ){
                    escape_flag = !escape_flag;
                } else if ( ch == '\'' && doubleQuote == 1) {
                    escape_flag = !escape_flag;
                }
                if( ch == '\\' && escape_flag) {
                    input++;
                    p->index++;
                } else {
                    str[len++] = (char)ch;
                    input++;
                    p->index++;
                }
            } else {
                if( escape_flag == 1) {
                    p->index += 2;
                    input += 2;
                    if( escape == 'x') {
                        if( !parser_is_hex(input[0]) || !parser_is_hex(input[1]) ) {
                            upy_free(str);
                            return NULL;
                        }
                        escape = hex_to_char(input[0]);
                        escape <<= 4;
                        escape |= hex_to_char(input[1]);
                        p->index += 2;
                        input += 2;
                    }
                    str[len++] = (char)escape;
                } else {
                    str[len++] = input[0];
                    p->index += 1;
                    input += 1;
                    str[len++] = input[0];
                    p->index += 1;
                    input += 1;
                }
            }

            if( ch == 0x0D || ch == 0x0A ){
                upy_free(str);
                return NULL;
            }

            if( doubleQuote == 1 && ch == '"')
                flag = 0;

            if( doubleQuote == 0 && ch == '\'')
                flag = 0;
        } while( flag );
        str[len - 1] = 0;
        return str;
    }
    return NULL;
}

static int hex_to_int(char s[])
{
    int i;
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))
        i = 2;
    else
        i = 0;
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i){
        if (tolower(s[i]) > '9')
            n = 16 * n + (10 + tolower(s[i]) - 'a');
        else
            n = 16 * n + (tolower(s[i]) - '0');
    }
    return n;
}

static int p_hexLiteral(upy_t *p)
{
    char ch;
    char *str = p->name;
    int len = 0;
    int index = p->index;
    if( parser_next_ch(p, index) == '0' && parser_next_ch(p, index + 1) == 'x' ){
        if( parser_is_hex(parser_next_ch(p, index)) ){
            do {
                index =  p->index;
                ch = parser_next_ch(p, index);
                if( !parser_is_hex(ch) ) break;
                str[len++] = ch;
                p->index++;
            } while(parser_is_hex(ch));
            str[len++] = 0;
            p->num = hex_to_int(str);
            return 1;
        }
    }
    return 0;
}

static int p_NumericLiteral(upy_t * p)
{
    char ch;
    char *str = p->name;
    int len = 0;
    int index = p->index;
    if( isdigit( parser_next_ch(p, index) ) ) {
        do {
            index =  p->index;
            ch = parser_next_ch(p, index);
            if( !isdigit(ch) ) break;
            str[len++] = ch;
            p->index++;
        } while(isdigit(ch));

        if( parser_token_eat(p, ".") ) {
            str[len++] = '.';
            do {
                index =  p->index;
                ch = parser_next_ch(p, index);
                if( !isdigit(ch) ) break;
                str[len++] = ch;
                p->index++;
            } while(isdigit(ch));
        }
        str[len++] = 0;
        p->num = atof(str);
        return 1;
    }
    return 0;
}

static upy_val_t p_file_input(upy_t *p);
static upy_val_t p_eval_input(upy_t *p);
static int p_NEWLINE(upy_t *p);
static int p_funcdef(upy_t *p);
static int p_parameters(upy_t *p);
static int p_typedargslist(upy_t *p);
static int p_tfpdef(upy_t *p);
static int p_stmt(upy_t *p);
static int p_simple_stmt(upy_t *p);
static int p_small_stmt(upy_t *p);
static int p_expr_stmt(upy_t *p);
static int p_testlist_star_expr(upy_t *p);
static int p_augassign(upy_t *p);
static int p_pass_stmt(upy_t *p);
static int p_flow_stmt(upy_t *p);
static int p_break_stmt(upy_t *p);
static int p_continue_stmt(upy_t *p);
static int p_return_stmt(upy_t *p);
static int p_raise_stmt(upy_t *p);
static int p_import_stmt(upy_t *p);
static int p_import_name(upy_t *p);
static int p_import_from(upy_t *p);
static int p_import_as_name(upy_t *p);
static int p_dotted_as_name(upy_t *p);
static int p_import_as_names(upy_t *p);
static int p_dotted_as_names(upy_t *p);
static int p_dotted_name(upy_t *p);
static int p_global_stmt(upy_t *p);
static int p_nonlocal_stmt(upy_t *p);
static int p_compound_stmt(upy_t *p);
static int p_if_stmt(upy_t *p);
static int p_while_stmt(upy_t *p);
static int p_for_stmt(upy_t *p);
static int p_try_stmt(upy_t *p);
static int p_except_clause(upy_t *p);
static int p_suite(upy_t *p);
static int p_test(upy_t *p);
static int p_test_nocond(upy_t *p);
static int p_or_test(upy_t *p);
static int p_and_test(upy_t *p);
static int p_not_test(upy_t *p);
static int p_comparison(upy_t *p);
static int p_comp_op(upy_t *p);
static int p_expr(upy_t *p);
static int p_xor_expr(upy_t *p);
static int p_and_expr(upy_t *p);
static int p_shift_expr(upy_t *p);
static int p_arith_expr(upy_t *p);
static int p_term(upy_t *p);
static int p_factor(upy_t *p);
static int p_atom_expr(upy_t *p);
static int p_atom(upy_t *p);
static int p_testlist_comp(upy_t *p);
static int p_trailer(upy_t *p, int tk);
static int p_subscriptlist(upy_t *p);
static int p_subscript(upy_t *p);
static int p_sliceop(upy_t *p);
static int p_exprlist(upy_t *p);
static int p_testlist(upy_t *p);
static int p_dictorsetmaker(upy_t *p);
static int p_arglist(upy_t *p);
static int p_argument(upy_t *p);
static int p_comp_iter(upy_t *p);
static int p_comp_for(upy_t *p);
static int p_comp_if(upy_t *p);
static int p_NUMBER(upy_t * p);
static int p_INDENT(upy_t *p);\
static int p_classdef(upy_t *p);
static int p_SPACES(upy_t *p);

static void add_line(upy_t *p){
    static int line = 0;
    if( p->row > line){
        bc_add_8(op_line);
        bc_add_32(line);
        line = p->row;
    }
}

static char next_ch(upy_t *p)
{
    return p->input[p->index++];
}

static char current_ch(upy_t *p)
{
    return p->input[p->index++];
}

static int parser_space(upy_t * p)
{
    char ch = current_ch(p);
    if(ch == ' '){
        p->index++;return 1;
    } else if(ch == '\t'){
        p->index++;return 1;
    }
    return 0;
}

static int parser_EOF(upy_t * p)
{
    if( p->index >= p->input_len )
        return 1;
    if( !current_ch(p) )
        return 1;
    return 0;
}

static int parser_Comment(upy_t * p)
{
    if( parser_token_eat(p, "'''") ) {
        do {
            if( parser_token_eat(p, "'''") ) {
                return 1;
            }
            char ch = next_ch(p);
            if(ch == '\n')
                p->row++;
        } while(1);
    } else
        return 0;
}

static int parser_LineComment(upy_t * p)
{
    if( parser_token_eat(p, "#") ) {
        while(parser_space(p));
        do {
            if( parser_token_eat(p, "\n") ) {
                p->row++;
                return 1;
            }
            next_ch(p);
        } while(1);
    } else
        return 0;
}

static upy_val_t p_file_input(upy_t *p)
{
    upy_val_t scope = curr_scope = module_create(p);
    while(1){
        int indent = p_INDENT(p);
        while(p_NEWLINE(p)){
            indent = p_INDENT(p);
        }
        if(indent != 0) {
            break;
        }
        if( p_stmt(p) )
            continue;
        break;
    }
    while(p_NEWLINE(p));
    bc_add_8(op_stop);

    upy_object_t *obj = upy_2_pointer(scope);
    obj->u.f->code = (uint8_t*)upy_malloc((size_t)compile_buf_index);
    assert(obj->u.f->code);
    obj->u.f->code_size = compile_buf_index;
    memcpy(obj->u.f->code, compile_buf, (size_t)compile_buf_index);
    clear_locals(obj);

    if( !parser_EOF(p) )
        return 0;

    return scope;
}

static upy_val_t p_eval_input(upy_t *p)
{
    if( !p_testlist(p) )
        return 0;
    while(p_NEWLINE(p));
    if( !parser_EOF(p) )
        return 0;
    return 1;
}

static const char *strpool(upy_t *p, char *name) {
    return upy_key_2_str(p, upy_strpool_insert(p, name, 1));
}

static int p_funcdef(upy_t *p)
{
    if( !parser_token_eat(p, "def") )
        return 0;
    upy_val_t scope = curr_scope;
    while(p_SPACES(p));
    if( !parser_Identifier(p, p->name) )
        return 0;
    curr_scope = function_create(p);
    upy_object_t *obj = upy_2_pointer(curr_scope);
    upy_object_set_cs(p, scope, strpool(p, p->name), curr_scope);
    int start_index = compile_buf_index;

    while(p_SPACES(p));
    p_parameters(p);
    while(p_SPACES(p));
    if( !parser_token_eat(p, ":") )
        return 0;
    while(p_SPACES(p));
    if( !p_suite(p) ) return 0;
    bc_add_8(op_return_void);
    curr_scope = scope;
    int len = compile_buf_index - start_index;
    obj->u.f->code = (uint8_t *)upy_malloc((size_t)len);
    obj->u.f->code_size = len;
    memcpy(obj->u.f->code, compile_buf + start_index, (size_t)len);
    compile_buf_index = start_index;
    clear_locals(obj);
    return 1;
}

static int p_parameters(upy_t *p)
{
    int argc = 0;
    if( !parser_token_eat(p, "(") )
        return 0;
    while(p_SPACES(p));
    p_typedargslist(p);
    while(p_SPACES(p));
    if( !parser_token_eat(p, ")") )
        return 0;
    return argc;
}

static int p_typedargslist(upy_t *p)
{
    if( p_tfpdef(p) ) {
        upy_object_t *obj = upy_2_pointer(curr_scope);
        add_locals(obj, strpool(p, p->name));
        obj->u.f->nparams++;
        while(1){
            while(p_SPACES(p));
            if(!parser_token_eat(p, ","))
                break;
            while(p_SPACES(p));
            if( !p_tfpdef(p) )
                return 0;
            add_locals(obj, strpool(p, p->name));
            obj->u.f->nparams++;
        }
    }
    return 1;
}

static int p_tfpdef(upy_t *p)
{
    if( !parser_Identifier(p, p->name) )
        return 0;
    while(p_SPACES(p));
    return 1;
}

static int p_stmt(upy_t *p)
{
    if( p_compound_stmt(p) ) return 1;
    if( p_simple_stmt(p) ) return 1;
    return 0;
}

static int p_simple_stmt(upy_t *p)
{
    if( !p_small_stmt(p) ) return 0;
    while(p_SPACES(p));
    while(1){
        if( !parser_token_eat(p, ";") ) break;
        while(p_SPACES(p));
        if( !p_small_stmt(p) ) break;
    }
    parser_token_eat(p, ";");
    while(p_SPACES(p));
    if( p_NEWLINE(p) == 1) {
        return 1;
    }
    return 0;
}

static int p_small_stmt(upy_t *p)
{
    if( p_pass_stmt(p) ) return 1;
    if( p_flow_stmt(p) ) return 1;
    if( p_import_stmt(p) ) return 1;
    if( p_global_stmt(p) ) return 1;
    if( p_nonlocal_stmt(p) ) return 1;
    int tk = p_expr_stmt(p);
    if( tk )
        bc_add_8(op_pop);
    return tk;
}

static int p_expr_stmt(upy_t *p)
{
    SAVE;
    if( !p_testlist_star_expr(p) )
        return 0;
    while(p_SPACES(p));
    int tk = p_augassign(p);
    if( tk ) {
        RESTORE;
        upy_object_t *obj = upy_2_pointer(curr_scope);
        int code_buf_index = compile_buf_index;
        int tk1 = p_testlist_star_expr(p);
        if( !tk )
            return 0;
        if( (tk1 & 0xFF) == TOK_PROP_SUFFIX )
            compile_buf_index -= 3;
        while(p_SPACES(p));

        tk = p_augassign(p);

        while(p_SPACES(p));
        if( !p_testlist_star_expr(p) )
            return 0;

        switch (tk1 & 0xFF) {
        case TOK_VAR: {
            uint16_t key = (tk1 >> 8) & 0xFFFF;
            const char *name = upy_key_2_str(p, key);
            if(obj->type == TYPE_MODULE || obj->type == TYPE_CLASS) {
                memmove(compile_buf + code_buf_index, compile_buf + code_buf_index + 3, (size_t)(compile_buf_index - code_buf_index - 3));
                compile_buf_index -= 3;
                bc_add_8(op_set_var);
                bc_add_16(key);
            } else {
                obj->u.f->nvars++;
                int gen, pos;
                pos = find_locals(obj, name, &gen);
                if( pos < 0 ) {
                    add_locals(obj, name);
                }
                pos = find_locals(obj, name, &gen);
                if( pos > 65535 ) {
                    upy_throw(p, upy_mk_string("Exceeds max size of local variable"));
                }
                memmove(compile_buf + code_buf_index, compile_buf + code_buf_index + 3, (size_t)(compile_buf_index - code_buf_index - 3));
                compile_buf_index -= 3;
                bc_add_8(op_set_local);
                bc_add_16((uint16_t)pos);
            }
        } break;
        case TOK_PROP_SUFFIX:{
            uint16_t key = (tk1 >> 8) & 0xFFFF;
            bc_add_8(op_prop_set);
            bc_add_16(key);
        }break;
        case TOK_INDEX_SUFFIX: break;
        }
    }
    return 1;
}

static int p_testlist_star_expr(upy_t *p)
{
    return p_test(p);
}

static int p_augassign(upy_t *p)
{
    if( parser_token_eat(p, "+=") ) { return 1;}
    if( parser_token_eat(p, "-=") ) { return 2;}
    if( parser_token_eat(p, "*=") ) { return 3;}
    if( parser_token_eat(p, "/=") ) { return 4;}
    if( parser_token_eat(p, "%=") ) { return 5;}
    if( parser_token_eat(p, "&=") ) { return 6;}
    if( parser_token_eat(p, "|=") ) { return 7;}
    if( parser_token_eat(p, "^=") ) { return 8;}
    if( parser_token_eat(p, "<<=") ) { return 9;}
    if( parser_token_eat(p, ">>=") ) { return 10;}
    if( parser_token_eat(p, "=") ) { return 11;}
    return 0;
}

static int p_pass_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "pass") ) return 0;
    while(p_SPACES(p));
    return 1;
}

static int p_flow_stmt(upy_t *p)
{
    if( p_break_stmt(p) ) return 1;
    if( p_continue_stmt(p) ) return 1;
    if( p_return_stmt(p) ) return 1;
    if( p_raise_stmt(p) ) return 1;
    return 0;
}

static int p_break_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "break") )
        return 0;
    return 1;
}

static int p_continue_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "continue") ) return 0;
    return 1;
}

static int p_return_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "return") ) return 0;
    while(p_SPACES(p));
    if( p_testlist(p) )
        bc_add_8(op_return);
    else
        bc_add_8(op_return_void);
    return 1;
}

static int p_raise_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "raise") ) return 0;
    while(p_SPACES(p));
    if( !p_test(p) )
        return 0;
    bc_add_8(op_throw);
    return 1;
}

static int p_import_stmt(upy_t *p)
{
    if( p_import_name(p) ) return 1;
    if( p_import_from(p) ) return 1;
    return 0;
}

static int p_import_name(upy_t *p)
{
    if( !parser_token_eat(p, "import") ) return 0;
    while(p_SPACES(p));
    if( !p_dotted_as_names(p) ) return 0;
    return 1;
}

static int p_import_from(upy_t *p)
{
    if( !parser_token_eat(p, "from") ) return 0;
    while(p_SPACES(p));
    if( !p_dotted_name(p) ) return 0;
    while(p_SPACES(p));
    if( !parser_token_eat(p, "import") ) return 0;
    while(p_SPACES(p));
    p_import_as_names(p);
    return 1;
}

static int p_import_as_name(upy_t *p)
{
    if( !parser_Identifier(p, p->name) ) return 0;
    while(p_SPACES(p));

    if( parser_token_eat(p, "as") ){
        while(p_SPACES(p));
        if( !parser_Identifier(p, p->name) ) return 0;
    }
    return 1;
}

static int p_dotted_as_name(upy_t *p)
{
    int tk = p_dotted_name(p);
    if( !tk ) return 0;
    while(p_SPACES(p));
    uint16_t key = upy_strpool_insert(p, p->name, 1);
    const char *name;
    if( tk == TOK_PROP_SUFFIX ) {
        if( parser_token_eat(p, "as") ){
            while(p_SPACES(p));
            if( !parser_Identifier(p, p->name) ) return 0;
            key = upy_strpool_insert(p, p->name, 1);
            goto p_dotted_as_name_end;
        }
        return 0;
    }
p_dotted_as_name_end:
    name = upy_key_2_str(p, key);
    upy_object_t *obj = upy_2_pointer(curr_scope);
    if(obj->type == TYPE_MODULE || obj->type == TYPE_CLASS) {
        bc_add_8(op_set_var);
        bc_add_16(key);
        bc_add_8(op_sdn);
    } else {
        obj->u.f->nvars++;
        int gen, pos;
        pos = find_locals(obj, name, &gen);
        if( pos < 0 ) {
            add_locals(obj, name);
        }
        pos = find_locals(obj, name, &gen);
        if( pos > 65535 ) {
            upy_throw(p, upy_mk_string("Exceeds max size of local variable"));
        }
        bc_add_8(op_set_local);
        bc_add_16((uint16_t)pos);
        bc_add_8(op_sdn);
    }
    return 1;
}

static int p_import_as_names(upy_t *p)
{
    if( !p_import_as_name(p) ) return 0;
    while(p_SPACES(p));
    while(1){
        if( parser_token_eat(p, ",") ){
            while(p_SPACES(p));
            if( !p_import_as_name(p) ) return 0;
        } else break;
    }
    parser_token_eat(p, ",");
    return 1;
}

static int p_dotted_as_names(upy_t *p)
{
    if( !p_dotted_as_name(p) ) return 0;
    while(p_SPACES(p));
    while(1){
        if( parser_token_eat(p, ",") ){
            while(p_SPACES(p));
            if( !p_dotted_as_name(p) ) return 0;
        } else break;
    }

    return 1;
}

static int p_dotted_name(upy_t *p)
{
    int tk = 1;
    if( !parser_Identifier(p, p->name) ) return 0;
    while(p_SPACES(p));
    uint16_t key = upy_strpool_insert(p, p->name, 1);
    bc_add_8(op_import);
    bc_add_16(key);
    while(1){
        if( parser_token_eat(p, ".") ){
            while(p_SPACES(p));
            if( !parser_Identifier(p, p->name) ) return 0;
            tk = TOK_PROP_SUFFIX;
        } else
            break;
    }
    return tk;
}

static int p_global_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "global") ) return 0;
    while(p_SPACES(p));
    if( !parser_Identifier(p, p->name) ) return 0;
    return 1;
}

static int p_nonlocal_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "nonlocal") ) return 0;
    while(p_SPACES(p));
    if( !parser_Identifier(p, p->name) ) return 0;
    return 1;
}

static int p_compound_stmt(upy_t *p)
{
    if( parser_Comment(p) ) return 1;
    if( parser_LineComment(p) ) return 1;
    if( p_if_stmt(p) ) return 1;
    if( p_while_stmt(p) ) return 1;
    if( p_for_stmt(p) ) return 1;
    if( p_try_stmt(p) ) return 1;
    if( p_funcdef(p) ) return 1;
    if( p_classdef(p) ) return 1;
    return 0;
}

static int p_classdef(upy_t *p)
{
    if(!parser_token_eat(p, "class")) return 0;
    while(p_SPACES(p));
    if(!parser_Identifier(p, p->name)) return 0;
    while(p_SPACES(p));
    upy_val_t scope = curr_scope;
    curr_scope = upy_class_create(p);
    const char *name = strpool(p, p->name);
    upy_object_set_cs(p, scope, name, curr_scope);
    upy_object_t *obj = upy_2_pointer(curr_scope);
    int start_index = compile_buf_index;
    if(parser_token_eat(p, "(")){
        while(p_SPACES(p));
        p_argument(p);
        if(!parser_token_eat(p, ")")) return 0;
        while(p_SPACES(p));
    }
    if(!parser_token_eat(p, ":")) return 0;
    while(p_SPACES(p));
    if( !p_suite(p) ) return 0;
    bc_add_8(op_stop);
    curr_scope = scope;
    int len = compile_buf_index - start_index;
    obj->u.f->code = (uint8_t *)upy_malloc((size_t)len);
    obj->u.f->code_size = len;
    memcpy(obj->u.f->code, compile_buf + start_index, (size_t)len);
    compile_buf_index = start_index;
    return 1;
}

static int p_if_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "if") ) return 0;
    while(p_SPACES(p));
    if( !p_test(p) ) return 0;
    while(p_SPACES(p));
    bc_add_8(op_jmp_false);
    int else_index = bc_add_32(0);

    if( !parser_token_eat(p, ":") ) return 0;
    while(p_SPACES(p));
    if( !p_suite(p) ) return 0;

    bc_add_8(op_jmp);
    int end_index = bc_add_32(0);

    bc_set_32(else_index, compile_buf_index);

    while(1) {
        if( !parser_token_eat(p, "elif") )
            break;
        while(p_SPACES(p));
        if( !p_test(p) ) return 0;
        while(p_SPACES(p));
        if( !parser_token_eat(p, ":") ) return 0;
        while(p_SPACES(p));
        if( !p_suite(p) ) return 0;
    }
    if(parser_token_eat(p, "else")){
        while(p_SPACES(p));
        if( !parser_token_eat(p, ":") ) return 0;
        while(p_SPACES(p));
        if( !p_suite(p) ) return 0;

    }
    bc_set_32(end_index, compile_buf_index);
    return 1;
}

static int p_while_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "while") ) return 0;
    while(p_SPACES(p));
    int head_index = compile_buf_index;

    if( !p_test(p) ) return 0;
    while(p_SPACES(p));
    if( !parser_token_eat(p, ":") ) return 0;
    while(p_SPACES(p));

    bc_add_8(op_jmp_false);
    int end_index = bc_add_32(0);

    if( !p_suite(p) ) return 0;

    bc_add_8(op_jmp);
    bc_add_32(head_index);

    bc_set_32(end_index, compile_buf_index);

    if(parser_token_eat(p, "else")){
        while(p_SPACES(p));
        if( !parser_token_eat(p, ":") ) return 0;
        while(p_SPACES(p));
        if( !p_suite(p) ) return 0;
    }
    return 1;
}

static int p_for_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "for") ) return 0;
    while(p_SPACES(p));
    SAVE;
    int tk1 = p_exprlist(p);
    if( !tk1 ) return 0;
    while(p_SPACES(p));
    compile_buf_index = buf_index;
    if( !parser_token_eat(p, "in") ) return 0;
    while(p_SPACES(p));
    if( !p_testlist(p) ) return 0;
    while(p_SPACES(p));
    if( !parser_token_eat(p, ":") ) return 0;

    bc_add_8(op_iter);
    int begin_addr = compile_buf_index;
    bc_add_8(op_next);
    int next_index = bc_add_32(0);

    upy_object_t *obj = upy_2_pointer(curr_scope);
    switch (tk1 & 0xFF) {
    case TOK_VAR: {
        obj->u.f->nvars++;
        uint16_t key = (tk1 >> 8) & 0xFFFF;
        const char *name = upy_key_2_str(p, key);
        int gen, pos;
        pos = find_locals(obj, name, &gen);
        if( pos < 0 ) {
            add_locals(obj, name);
        }
        pos = find_locals(obj, name, &gen);
        if( pos > 65535 ) {
            upy_throw(p, upy_mk_string("Exceeds max size of local variable"));
        }
        bc_add_8(op_set_local);
        bc_add_16((uint16_t)pos);
        bc_add_8(op_sdn);
    } break;
    case TOK_PROP_SUFFIX: break;
    case TOK_INDEX_SUFFIX: break;
    }

    while(p_SPACES(p));
    if( !p_suite(p) ) return 0;

    bc_add_8(op_sdn);
    bc_add_8(op_jmp);
    bc_add_32(begin_addr);

    bc_set_32(next_index, compile_buf_index);

    if(parser_token_eat(p, "else")){
        while(p_SPACES(p));
        if( !parser_token_eat(p, ":") ) return 0;
        while(p_SPACES(p));
        if( !p_suite(p) ) return 0;
    }

    return 1;
}

static int p_except_clause(upy_t *p)
{
    if( !parser_token_eat(p, "except") ) return 0;
    while(p_SPACES(p));
    if(p_test(p)){
        while(p_SPACES(p));
        if(parser_token_eat(p, "as")) {
            while(p_SPACES(p));
            if(!parser_Identifier(p, p->name)) return 0;
        }
    }
    return 1;
}

static int p_try_stmt(upy_t *p)
{
    if( !parser_token_eat(p, "try") ) return 0;
    while(p_SPACES(p));
    if( !parser_token_eat(p, ":") ) return 0;
    while(p_SPACES(p));
    bc_add_8(op_try);
    int try_index = bc_add_32(0);
    if( !p_suite(p) ) return 0;
    bc_add_8(op_jmp);
    int end_index = bc_add_32(0);
    while(p_SPACES(p));
    bc_set_32(try_index, compile_buf_index);
    if( parser_token(p, "except") ){
        int catch_index = -1;
        int count = 0;
        SAVE;
        while(1){
            if( !p_except_clause(p) ) break;
            while(p_SPACES(p));
            if( !parser_token_eat(p, ":") ) break;
            while(p_SPACES(p));
            if( !p_suite(p) ) break;
            while(p_SPACES(p));
            count++;
        }
        RESTORE;
        int *jmp_addr = upy_malloc(sizeof (int) * count);
        assert(jmp_addr);
        count = 0;
        while(1){
            if( !p_except_clause(p) ) break;
            while(p_SPACES(p));
            if( !parser_token_eat(p, ":") ) break;
            bc_add_8(op_catch);
            catch_index = bc_add_32(0);
            while(p_SPACES(p));
            if( !p_suite(p) ) break;
            while(p_SPACES(p));
            bc_add_8(op_jmp);
            jmp_addr[count++] = bc_add_32(0);
            bc_set_32(catch_index, compile_buf_index);
        }
        for(int i = 0; i < count; i++) {
            bc_set_32(jmp_addr[i], compile_buf_index);
        }
        upy_free(jmp_addr);
        if( parser_token_eat(p, "else") ) {
            while(p_SPACES(p));
            if( !parser_token_eat(p, ":") ) return 0;;
            while(p_SPACES(p));
            if( !p_suite(p) ) return 0;
            while(p_SPACES(p));
        }
        if( parser_token_eat(p, "finally") ) {
            while(p_SPACES(p));
            if( !parser_token_eat(p, ":") ) return 0;;
            while(p_SPACES(p));
            if( !p_suite(p) ) return 0;
            while(p_SPACES(p));
        }
    } else {
        if( parser_token_eat(p, "finally") ) {
            while(p_SPACES(p));
            if( !p_suite(p) ) return 0;
            while(p_SPACES(p));
        }
    }
    bc_set_32(end_index, compile_buf_index);
    return 1;
}

static int p_suite(upy_t *p)
{
    if(p_simple_stmt(p)) return 1;
    if( p_NEWLINE(p) ) {
        int indent = p_INDENT(p);
        if ( !p_stmt(p) )
            return 0;
        while(1){
            if( indent != p_INDENT(p) )
                break;
            if ( !p_stmt(p) ) break;
        }
        return 1;
    }
    return 0;
}

static int p_test(upy_t *p)
{
    return p_or_test(p);
}

static int p_test_nocond(upy_t *p)
{
    if( p_or_test(p) ) return 1;
    return 0;
}

static int p_or_test(upy_t *p)
{
    int tk = p_and_test(p);
    if(!tk ) return 0;
    while(1){
        while(p_SPACES(p));
        if(!parser_token_eat(p, "or")) break;
        while(p_SPACES(p));
        if(!p_and_test(p)) return 0;
        bc_add_8(op_logic_or);
    }
    return tk;
}

static int p_and_test(upy_t *p)
{
    int tk = p_not_test(p);
    if( tk == 0) return 0;
    while(1){
        while(p_SPACES(p));
        if(!parser_token_eat(p, "and")) break;
        while(p_SPACES(p));
        if(p_not_test(p) == 0) return 0;
        bc_add_8(op_logic_and);
    }
    return tk;
}

static int p_not_test(upy_t *p)
{
    int tk;
    if(parser_token_eat(p, "not")) {
        while(p_SPACES(p));
        tk = p_not_test(p);
        if(!tk) return 0;
        bc_add_8(op_not);
    } else {
        return p_comparison(p);
    }
    return tk;
}

static int p_comparison(upy_t *p)
{
    int tk = p_expr(p);
    if( tk == 0 ) return 0;
    while(1){
        int tk1 = p_comp_op(p);
        if( tk1 == 0) break;
        while(p_SPACES(p));
        if(!p_expr(p)) return 0;
        switch (tk1) {
        case 1: break;
        case 2: break;
        case 3: bc_add_8(op_eq);break;
        case 4: break;
        case 5: bc_add_8(op_neq);break;
        case 6: bc_add_8(op_gteq);break;
        case 7: bc_add_8(op_lteq);break;
        case 8: bc_add_8(op_lt);break;
        case 9: bc_add_8(op_gt);break;
        }
    }
    return tk;
}

static int p_comp_op(upy_t *p)
{
    if( parser_token_eat(p, "not") ){
        while(p_SPACES(p));
        if( parser_token_eat(p, "in") ){
            return 1;
        }
        return 0;
    }
    else if( parser_token_eat(p, "is") ){
        while(p_SPACES(p));
        if( parser_token_eat(p, "not") ){
            return 2;
        }
        return 0;
    }
    else if( parser_token_eat(p, "==") ) return 3;
    else if( parser_token_eat(p, "in") ) return 4;
    else if( parser_token_eat(p, "!=") ) return 5;
    else if( parser_token_eat(p, ">=") ) return 6;
    else if( parser_token_eat(p, "<=") ) return 7;
    else if( parser_token_eat(p, "<") ) return 8;
    else if( parser_token_eat(p, ">") ) return 9;
    return 0;
}

static int p_expr(upy_t *p)
{
    int tk = p_xor_expr(p);
    if( tk == 0) return 0;

    while(1){
        while(p_SPACES(p));
        if(!parser_token_eat(p, "|")) break;
        while(p_SPACES(p));
        if(!p_xor_expr(p)) {
            return 0;
        }
        bc_add_8(op_or);
    }
    return tk;
}

static int p_xor_expr(upy_t *p)
{
    int tk = p_and_expr(p);
    if( tk == 0) return 0;
    while(1){
        while(p_SPACES(p));
        if(!parser_token_eat(p, "^")) break;
        while(p_SPACES(p));
        if(!p_and_expr(p)) {
            return 0;
        }
        bc_add_8(op_xor);
    }
    return tk;
}

static int p_and_expr(upy_t *p)
{
    int tk = p_shift_expr(p);
    if( tk == 0) return 0;
    while(1){
        while(p_SPACES(p));
        if(!parser_token_eat(p, "&")) break;
        while(p_SPACES(p));
        if(!p_shift_expr(p)) {
            return 0;
        }
        bc_add_8(op_and);
    }
    return tk;
}

static int p_shift_expr(upy_t *p)
{
    int tk = p_arith_expr(p);
    if( tk == 0) return 0;
    while(1){
        while(p_SPACES(p));
        uint8_t tk;
        if( parser_token_eat(p, "<<") ) {
            tk = 0;
        }
        else if( parser_token_eat(p, ">>")) {
            tk = 1;
        }
        else
            break;
        while(p_SPACES(p));
        if(!p_arith_expr(p)) {
            return 0;
        }
        bc_add_8(op_left_shift + tk);
    }
    return tk;
}

static int p_arith_expr(upy_t *p)
{
    int tk = p_term(p);
    if( tk == 0) return 0;
    while(1){
        while(p_SPACES(p));
        uint8_t tk;
        if( parser_token_eat(p, "+") ){
            tk = 0;
        }
        else if( parser_token_eat(p, "-")){
            tk = 1;
        }
        else
            break;
        while(p_SPACES(p));
        if(!p_term(p)) {
            return 0;
        }
        bc_add_8(op_add + tk);
    }
    return tk;
}

static int p_term(upy_t *p)
{
    int tk = p_factor(p);
    if( tk == 0) return 0;
    while(1){
        uint8_t tk;
        if(parser_token_eat(p, "*")) {
            tk = 0;
        } else if(parser_token_eat(p, "/")) {
            tk = 1;
        } else if(parser_token_eat(p, "%")) {
            tk = 2;
        } else
            break;
        while(p_SPACES(p));
        if(!p_factor(p)) {
            return 0;
        }
        bc_add_8(op_mul + tk);
    }
    return tk;
}

static int p_factor(upy_t *p)
{
    int tk;
    if(parser_token_eat(p, "+")) {
        while(p_SPACES(p));
        tk = p_factor(p);
        if( tk ) {
            bc_add_8(op_positive);
            return tk;
        }
    } else if(parser_token_eat(p, "-")) {
        while(p_SPACES(p));
        tk = p_factor(p);
        if( tk ) {
            bc_add_8(op_negative);
            return tk;
        }
    } else if(parser_token_eat(p, "~")) {
        while(p_SPACES(p));
        tk = p_factor(p);
        if( tk ) {
            bc_add_8(op_invert);
            return tk;
        }
    } else {
        return p_atom_expr(p);
    }
    return 0;
}

static int p_atom_expr(upy_t *p)
{
    int token = p_atom(p);
    if(token == 0)  {return token;}
    while(1){
        while(p_SPACES(p));
        int local_token = p_trailer(p, token);
        if(local_token == 0) break;
        token = local_token;
    }
    return token;
}

static int p_atom(upy_t *p)
{
    if( parser_token_eat(p, "(") ){
        while(p_SPACES(p));
        if(!p_testlist_comp(p)) return 0;
        while(p_SPACES(p));
        if( parser_token_eat(p, ")") ) {
            while(p_SPACES(p));
            return 1;
        } else
            return 0;
    } else if( parser_token_eat(p, "[") ) {
        while(p_SPACES(p));
        int count = p_testlist_comp(p);
        if( count > 65535 )
            upy_throw(p, upy_mk_string("Exceeds max size of array"));
        while(p_SPACES(p));
        if( !parser_token_eat(p, "]") ) return 0;
        bc_add_8(op_array);
        bc_add_16((uint16_t)count);
        return 1;
    } else if( parser_token_eat(p, "{") ) {
        while(p_SPACES(p));
        int count = p_dictorsetmaker(p);
        if( count > 65535 )
            upy_throw(p, upy_mk_string("Exceeds max size of dict"));
        while(p_SPACES(p));
        if( !parser_token_eat(p, "}") ) return 0;
        bc_add_8(op_dict);
        bc_add_16((uint16_t)count);
        return 1;
    } else {
        if( p_NUMBER(p) ) {
            bc_add_8(op_get_number);
            bc_add_double(p->num);
            return 1;
        } else if( parser_token_eat(p, "None") ) {
            bc_add_8(op_get_none);
            return 1;
        } else if( parser_token_eat(p, "True") ) {
            bc_add_8(op_get_true);
            return 1;
        } else if( parser_token_eat(p, "False") ) {
            bc_add_8(op_get_false);
            return 1;
        } else if( parser_Identifier(p, p->name) ) {
            const char *name = strpool(p, p->name);
            uint16_t key = upy_strpool_insert(p, p->name, 0);
            int gen, pos;
            pos = find_locals(upy_2_pointer(curr_scope), name, &gen);
            if( pos != -1 ) {
                if( pos > 65535 )
                    upy_throw(p, upy_mk_string("Exceeds max size of local variable"));
                bc_add_8(op_get_local);
                bc_add_16((uint16_t)pos);
            } else {
                bc_add_8(op_get_var);
                bc_add_16(key);
            }
            return TOK_VAR | key << 8;
        } else {
            char *str = parser_StringLiteral(p);
            if( str ) {
                uint16_t key = upy_strpool_insert(p, str, 1);
                upy_free(str);
                bc_add_8(op_get_string);
                bc_add_16(key);
                return 1;
            }
        }
        return 0;
    }
}

static int p_testlist_comp(upy_t *p)
{
    int count = 0;
    if( !p_test(p) ) {
        return 0;
    }
    count++;
    while(p_SPACES(p));
    if( !p_comp_for(p) ) {
        while(1){
            if( !parser_token_eat(p, ",") ) break;
            while(p_SPACES(p));
            if( !p_test(p) ) break;
            count++;
        }
        parser_token_eat(p, ",");
    }
    return count;
}

static int p_trailer(upy_t *p, int tk)
{
    if( parser_token_eat(p, "(") ) {;
        while(p_SPACES(p));
        if( (tk & 0xFF) == TOK_PROP_SUFFIX ){
            bc_add_8(op_sup);
        }

        int count = p_arglist(p);
        if( count > 65536 )
            upy_throw(p, upy_mk_string("Exceeds max size of arguments"));
        while(p_SPACES(p));
        if( !parser_token_eat(p, ")") ) return 0;
        if( (tk & 0xFF) == TOK_PROP_SUFFIX ){
            bc_add_8(op_prop_call);
        } else
            bc_add_8(op_call);
        bc_add_16((uint16_t)count);
        return TOK_CALL;
    } else if( parser_token_eat(p, "[") ) {;
        while(p_SPACES(p));
        if( !p_subscriptlist(p) ) return 0;
        while(p_SPACES(p));
        if( !parser_token_eat(p, "]") ) return 0;
        bc_add_8(op_get_array);
        return TOK_INDEX_SUFFIX;
    } else if( parser_token_eat(p, ".") ) {
        while(p_SPACES(p));
        if( !parser_Identifier(p, p->name)) return 0;
        uint16_t key = upy_strpool_insert(p, p->name, 1);
        bc_add_8(op_prop_get);
        bc_add_16(key);
        return TOK_PROP_SUFFIX | (key << 8);
    } else
        return 0;
}

static int p_subscriptlist(upy_t *p)
{
    if(!p_subscript(p)) return 0;
    while(p_SPACES(p));
    while(1){
        if( !parser_token_eat(p, ",") ) break;;
        while(p_SPACES(p));
        if(!p_subscript(p)) return 0;
        while(p_SPACES(p));
    }
    parser_token_eat(p, ",") ;
    return 1;
}

static int p_subscript(upy_t *p)
{
    if(p_test(p)) while(p_SPACES(p));
    else{
        if( !parser_token_eat(p, ":") ) return 0;
    }

    if( !parser_token_eat(p, ":") ) return 1;
    while(p_SPACES(p));
    if(p_test(p)) while(p_SPACES(p));
    if(p_sliceop(p)) while(p_SPACES(p));
    return 1;
}

static int p_sliceop(upy_t *p)
{
    if( !parser_token_eat(p, ":") ) return 0;
    while(p_SPACES(p));
    p_test(p);
    return 1;
}

static int p_exprlist(upy_t *p)
{
    int tk = p_expr(p);
    if( !tk) return 0;
    while(1){
        while(p_SPACES(p));
        if(!parser_token_eat(p, ",")) break;
        while(p_SPACES(p));
        tk = p_expr(p);
        if(!tk)
            break;
    }
    parser_token_eat(p, ",");
    return tk;
}

static int p_testlist(upy_t *p)
{
    if( !p_test(p)) return 0;
    while(1){
        while(p_SPACES(p));
        if(!parser_token_eat(p, ",")) break;
        while(p_SPACES(p));
        if(!p_test(p)) break;
    }
    parser_token_eat(p, ",");
    return 1;
}

static int p_dictorsetmaker(upy_t *p)
{
    int count = 0;
    while(1){
        while(p_SPACES(p));
        if( !p_test(p) ) break;
        while(p_SPACES(p));
        if(!parser_token_eat(p, ":")) return 0;
        while(p_SPACES(p));
        if( !p_test(p) ) return 0;
        while(p_SPACES(p));
        parser_token_eat(p, ",");
        count++;
    }
    return count;
}

static int p_arglist(upy_t *p)
{
    int count = 0;
    if( !p_argument(p)) return 0;
    count++;
    while(1){
        while(p_SPACES(p));
        if(!parser_token_eat(p, ",")) break;;
        while(p_SPACES(p));
        if( !p_argument(p)) break;
        count++;
    }
    parser_token_eat(p, ",");
    return count;
}

static int p_argument(upy_t *p)
{
    if( p_test(p) ) {
        while(p_SPACES(p));
        if(parser_token_eat(p, "=")){
            while(p_SPACES(p));
            if( !p_test(p) ) return 0;
            return 1;
        }
        return 1;
    }
    return 0;
}

static int p_comp_iter(upy_t *p)
{
    if( p_comp_for(p)) return 1;
    if( p_comp_if(p)) return 1;
    return 0;
}

static int p_comp_for(upy_t *p)
{
    if(!parser_token_eat(p, "for")) return 0;
    while(p_SPACES(p));
    if(!p_exprlist(p)) return 0;
    while(p_SPACES(p));
    if(!parser_token_eat(p, "in")) return 0;
    while(p_SPACES(p));
    if(!p_or_test(p)) return 0;
    while(p_SPACES(p));
    p_comp_iter(p);
    return 1;
}

static int p_comp_if(upy_t *p)
{
    if(!parser_token_eat(p, "if")) return 0;
    while(p_SPACES(p));
    if(!p_test_nocond(p)) return 0;
    while(p_SPACES(p));
    p_comp_iter(p);
    return 1;
}

static int p_SPACES(upy_t *p)
{
    if( parser_token_eat(p, "\t") ) return 4;
    if( parser_token_eat(p, " ") ) return 1;
    return 0;
}

static int p_NEWLINE(upy_t *p)
{
    if( parser_token_eat(p, "\r") ){
        if( !parser_token_eat(p, "\n") ) return 0;
        add_line(p);
        p->row++;
        return 1;
    } else if( parser_token_eat(p, "\n") ){
        add_line(p);
        p->row++;
        return 1;
    } else if( parser_token_eat(p, "\f") ){
        return 1;
    } else
        return 0;
}

static int p_IMAG(upy_t *p)
{
    (void)p;
    return 0;
}

static int p_OCT(upy_t *p)
{
    (void)p;
    return 0;
}

static int p_BIN(upy_t *p)
{
    (void)p;
    return 0;
}

static int p_NUMBER(upy_t * p)
{
    int token;
    token = p_hexLiteral(p);
    if(token) return token;
    token = p_NumericLiteral(p);
    if(token) return token;
    token = p_IMAG(p);
    if(token) return token;
    token = p_OCT(p);
    if(token) return token;
    token = p_BIN(p);
    if(token) return token;;
    return 0;
}

static int p_INDENT(upy_t *p)
{
    int i = 0;
    while(1){
        int space = p_SPACES(p);
        if( space ) i+= space;
        else
            break;
    }
    return i;
}

int upy_type(upy_val_t v) {
    int type = (v) >> 48;
    if ((type & 0x7ff0) != 0x7ff0) {
        return 0;
    } else {
        return type & 0xf;
    }
}

static inline int branch(uint8_t *code, int size, int pc, uint16_t key)
{
    for(int i = pc; i < size; i++){
        switch(code[pc++]){
        case op_get_local:pc += 2;break;
        case op_get_var:pc += 2;break;
        case op_get_number:pc += 8;break;
        case op_get_none:break;
        case op_get_true:break;
        case op_get_false:break;
        case op_get_string:pc += 2;break;
        case op_set_local:pc += 2;break;
        case op_set_var:pc += 2;break;
        case op_add:break;
        case op_sub:break;
        case op_mul:break;
        case op_div:break;
        case op_mod:break;
        case op_and:break;
        case op_or:break;
        case op_xor:break;
        case op_negative:break;
        case op_not:break;
        case op_invert:break;
        case op_left_shift:break;
        case op_right_shift:break;
        case op_call:pc += 2;break;
        case op_return:break;
        case op_return_void:break;
        case op_lt:break;
        case op_gt:break;
        case op_lteq:break;
        case op_gteq:break;
        case op_eq:break;
        case op_neq:break;
        case op_sdn:break;
        case op_sup:break;
        case op_pop:break;
        case op_dup:break;
        case op_iter:break;
        case op_logic_and:break;
        case op_logic_or:break;
        case op_try:pc+=4;break;
        case op_catch:pc += 4;break;
        case op_throw:;break;
        case op_next:pc+=4;break;
        case op_prop_get:pc += 2;break;
        case op_prop_set:pc += 2;break;
        case op_prop_call:pc += 2;break;
        case op_get_array:break;
        case op_set_array:break;
        case op_branch:pc += 4;break;
        case op_branch_true:pc += 4;break;
        case op_branch_false:pc += 4;break;
        case op_jmp:pc += 4;break;
        case op_jmp_true:pc += 4;break;
        case op_jmp_false:pc += 4;break;
        case op_dict:pc += 2;break;
        case op_array:pc += 2;break;
        case op_import:pc += 2;break;
        case op_self:break;
        case op_line:pc += 4;break;
        case op_label:
            if( get_uint16(code + pc) == key )
                return pc + 2;
            break;
        case op_stop:break;
        }
    }
    return -1;
}

int upy_compare(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        double x = upy_2_double(v1);
        double y = upy_2_double(v2);
        return x < y ? -1 : x > y ? 1 : 0;
    } else if (upy_is_string(v1) && upy_is_string(v2)) {
        return strcmp(upy_2_string(v1), upy_2_string(v2));
    } else {
        upy_throw(e, upy_mk_string("unsupported operand type(s) for comparison"));
    }
    return 0;
}

upy_val_t upy_add(upy_t *e, upy_val_t v1, upy_val_t v2) {
    const char *sa = NULL;
    const char *sb = NULL;
    char *sab = NULL;
    char buf[32];
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number(upy_2_double(v1) + upy_2_double(v2));
    } else if( upy_is_string(v1) && upy_is_string(v2) ) {
        sa = upy_2_string(v1);
        sb = upy_2_string(v2);
        goto CONCAT;
    } else if( upy_is_number(v1) && upy_is_string(v2) ) {
        if( upy_is_integer(v1) )
            sprintf(buf, "%d", upy_2_int32(v1));
        else
            sprintf(buf, "%f", upy_2_double(v1));
        sa = buf;
        sb = upy_2_string(v2);
        goto CONCAT;
    } else if( upy_is_number(v2) && upy_is_string(v1) ) {
        if( upy_is_integer(v2) )
            sprintf(buf, "%d", upy_2_int32(v2));
        else
            sprintf(buf, "%f", upy_2_double(v2));
        sb = buf;
        sa = upy_2_string(v1);
        goto CONCAT;
    } else {
        upy_throw(e, upy_mk_string("unsupported operand type(s) for +"));
    }
CONCAT:
    sab = upy_malloc(strlen(sa) + strlen(sb) + 1);
    assert(sab);
    strcpy(sab, sa);
    strcat(sab, sb);
    return upy_string_create(e, sab);
}

upy_val_t upy_sub(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number(upy_2_double(v1) - upy_2_double(v2));
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for -"));
    return VAL_FALSE;
}

upy_val_t upy_mul(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number(upy_2_double(v1) * upy_2_double(v2));
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for *"));
    return VAL_FALSE;
}

upy_val_t upy_div(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number(upy_2_double(v1) / upy_2_double(v2));
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for /"));
    return VAL_FALSE;
}

upy_val_t upy_mod(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number( fmod(upy_2_double(v1), upy_2_double(v2)));
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for %"));
    return VAL_FALSE;
}

upy_val_t upy_and(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number( upy_2_int32(v1) & upy_2_int32(v2) );
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for &"));
    return VAL_FALSE;
}

upy_val_t upy_or(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number( upy_2_int32(v1) | upy_2_int32(v2) );
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for &"));
    return VAL_FALSE;
}

upy_val_t upy_xor(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number( upy_2_int32(v1) ^ upy_2_int32(v2) );
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for ^"));
    return VAL_FALSE;
}

upy_val_t upy_negative(upy_t *e, upy_val_t v) {
    if( upy_is_number(v) ) {
        return upy_mk_number( -upy_2_double(v) );
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for -"));
    return VAL_FALSE;
}

upy_val_t upy_not(upy_t *e, upy_val_t v) {
    if( upy_is_boolean(v) ) {
        return upy_mk_boolean( -upy_2_boolean(v) );
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for !"));
    return VAL_FALSE;
}

upy_val_t upy_invert(upy_t *e, upy_val_t v) {
    if( upy_is_number(v) ) {
        return upy_mk_number(~upy_2_uint32(v) );
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for ~"));
    return VAL_FALSE;
}

upy_val_t upy_left_shift(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number( upy_2_int32(v1) << upy_2_uint32(v2) );
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for <<"));
    return VAL_FALSE;
}

upy_val_t upy_right_shift(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) && upy_is_number(v2) ) {
        return upy_mk_number( upy_2_int32(v1) >> upy_2_uint32(v2) );
    }
    upy_throw(e, upy_mk_string("unsupported operand type(s) for %"));
    return VAL_FALSE;
}

upy_val_t upy_logic_and(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) ){
        if( upy_2_double(v1) != 0 ){
            return v2;
        }
        return v1;
    } else if( upy_is_boolean(v1) ){
        if( upy_2_boolean(v1) != 0 ){
            return v2;
        }
        return v1;
    } else if( upy_is_string(v1) ){
        if( upy_string_size(v1) != 0 ){
            return v2;
        }
        return v1;
    } else if( upy_is_none(v1) )
        return v1;
    return v2;
}

upy_val_t upy_logic_or(upy_t *e, upy_val_t v1, upy_val_t v2) {
    if( upy_is_number(v1) ){
        if( upy_2_double(v1) == 0 ){
            return v2;
        }
        return v1;
    } else if( upy_is_boolean(v1) ){
        if( upy_2_boolean(v1) == 0 ){
            return v2;
        }
        return v1;
    } else if( upy_is_string(v1) ){
        if( upy_string_size(v1) == 0 ){
            return v2;
        }
        return v1;
    }
    return v2;
}

upy_val_t upy_module_create(upy_t *e) {
    return mk_module(object_create(e, TYPE_MODULE));
}

upy_val_t upy_object_create(upy_t *e) {
    return mk_object(object_create(e, TYPE_OBJECT));
}

int upy_object_size(upy_val_t o) {
    int count = 0;
    switch (upy_type(o)) {
    case TYPE_OBJECT:
    case TYPE_MODULE:
    case TYPE_ARRAY:
    case TYPE_CLASS:
    case TYPE_BUFFER:
    case TYPE_FUNCTION:
    case TYPE_HEAP_STRING:{
        upy_object_t *obj = (upy_object_t *)upy_2_pointer(o);
        upy_member_t *next = obj->member;
        while(next) {
            count++;
            next = next->next;
        }
    }break;
    }
    return count;
}

upy_val_t upy_class_create(upy_t *e) {
    upy_object_t *obj = object_create(e, TYPE_CLASS);
    obj->u.f = upy_malloc(sizeof (upy_function_t));
    assert(obj->u.a);
    memset(obj->u.a, 0, sizeof (upy_function_t));
    return mk_class(obj);
}

upy_val_t upy_object_duplicate(upy_t *e, upy_val_t o) {
    switch (upy_type(o)) {
    case TYPE_OBJECT:
    case TYPE_MODULE:
    case TYPE_ARRAY:
    case TYPE_CLASS:
    case TYPE_BUFFER:
    case TYPE_FUNCTION:
    case TYPE_HEAP_STRING:{
        upy_val_t res = upy_object_create(e);
        upy_object_t *obj = (upy_object_t *)upy_2_pointer(o);
        upy_member_t *next = obj->member;
        while(next) {
            upy_object_set(e, res, next->name, next->val);
            next = next->next;
        }
        return res;
    }break;
    }
    return VAL_UNDEF;
}

upy_val_t upy_object_get(upy_t *e, upy_val_t o, upy_val_t name) {
    switch (upy_type(o)) {
    case TYPE_OBJECT:
    case TYPE_MODULE:
    case TYPE_ARRAY:
    case TYPE_CLASS:
    case TYPE_BUFFER:
    case TYPE_FUNCTION:
    case TYPE_HEAP_STRING:{
        upy_object_t *obj = (upy_object_t *)upy_2_pointer(o);
        upy_member_t *next = obj->member;
        while(next) {
            if( !upy_compare(e, next->name, name) ){
                return next->val;
            }
            next = next->next;
        }
    }break;
    }
    return VAL_UNDEF;
}

void upy_object_set(upy_t *e, upy_val_t o, upy_val_t name, upy_val_t v) {
    switch (upy_type(o)) {
    case TYPE_OBJECT:
    case TYPE_MODULE:
    case TYPE_ARRAY:
    case TYPE_CLASS:
    case TYPE_BUFFER:
    case TYPE_FUNCTION:
    case TYPE_HEAP_STRING:{
        upy_object_t *obj = (upy_object_t *)upy_2_pointer(o);
        if( obj->member == NULL ) {
            obj->member = upy_malloc(sizeof (upy_member_t));
            assert(obj->member);
            obj->member->val = v;
            obj->member->name = name;
            obj->member->next = NULL;
        } else {
            upy_member_t *next = obj->member;
            for(;;) {
                if( !upy_compare(e, next->name, name) ) {
                    next->val = v;
                    break;
                } else if( !next->next ) {
                    next->next = upy_malloc(sizeof (upy_member_t));
                    assert(next->next);
                    next->next->val = v;
                    next->next->name = name;
                    next->next->next = NULL;
                    break;
                }
                next = next->next;
            }
        }
    }break;
    }
}

upy_val_t upy_object_delete(upy_t *e, upy_val_t o, upy_val_t name) {
    switch (upy_type(o)) {
    case TYPE_OBJECT:
    case TYPE_MODULE:
    case TYPE_ARRAY:
    case TYPE_CLASS:
    case TYPE_BUFFER:
    case TYPE_FUNCTION:
    case TYPE_HEAP_STRING:{
        upy_object_t *obj = (upy_object_t *)upy_2_pointer(o);
        if( obj->member == NULL ) {
            return VAL_FALSE;
        } else {
            upy_member_t *next = obj->member;
            upy_member_t *prev = next;
            while(next) {
                if( !upy_compare(e, next->val, name) ){
                    if( next != prev ) {
                        upy_member_t *local_next = next->next;
                        upy_free(next);
                        prev->next = local_next;
                    } else {
                        upy_free(obj->member);
                        obj->member = NULL;
                    }
                    return VAL_TRUE;
                }
                prev = next;
                next = next->next;
            }
            return VAL_FALSE;
        }
    }break;
    }
    return VAL_FALSE;
}

upy_val_t upy_object_get_cs(upy_t *e, upy_val_t o, const char *name) {
    return upy_object_get(e, o, upy_mk_string(name));
}

void upy_object_set_cs(upy_t *e, upy_val_t o, const char *name, upy_val_t v) {
    upy_object_set(e, o, upy_mk_string(name), v);
}

upy_val_t upy_object_delete_cs(upy_t *e, upy_val_t o, const char *name) {
    return upy_object_delete(e, o, upy_mk_string(name));
}

void upy_object_set_user_data(upy_val_t o, void *data) {
    switch (upy_type(o)) {
    case TYPE_OBJECT:
    case TYPE_MODULE:
    case TYPE_ARRAY:
    case TYPE_CLASS:
    case TYPE_BUFFER:
    case TYPE_FUNCTION:
    case TYPE_HEAP_STRING:{
        upy_object_t *obj = (upy_object_t *)upy_2_pointer(o);
        obj->user_data = data;
    }break;
    }
}

void *upy_object_get_user_data(upy_val_t o){    
    switch (upy_type(o)) {
    case TYPE_OBJECT:
    case TYPE_MODULE:
    case TYPE_ARRAY:
    case TYPE_CLASS:
    case TYPE_BUFFER:
    case TYPE_FUNCTION:
    case TYPE_HEAP_STRING:{
        upy_object_t *obj = (upy_object_t *)upy_2_pointer(o);
        return obj->user_data;
    }break;
    }
    return NULL;
}

void upy_object_set_parent(upy_val_t o, upy_val_t parent) {
    if( !upy_is_object(parent) )
        return;

    switch (upy_type(o)) {
    case TYPE_OBJECT:
    case TYPE_MODULE:
    case TYPE_ARRAY:
    case TYPE_CLASS:
    case TYPE_BUFFER:
    case TYPE_FUNCTION:
    case TYPE_HEAP_STRING:{
        upy_object_t *obj = upy_2_pointer(o);
        obj->p.parent = upy_2_pointer(parent);
    }break;
    }
}

upy_val_t upy_object_get_parent(upy_val_t o) {
    switch (upy_type(o)) {
    case TYPE_OBJECT:
    case TYPE_MODULE:
    case TYPE_ARRAY:
    case TYPE_CLASS:
    case TYPE_BUFFER:
    case TYPE_FUNCTION:
    case TYPE_HEAP_STRING:{
        upy_object_t *obj = upy_2_pointer(o);
        if( obj->p.parent )
            return mk_object(obj->p.parent);
    }break;
    }
    return VAL_UNDEF;
}

upy_val_t upy_global_get(upy_t *e, upy_val_t name) {
    return upy_object_get(e, e->globals, name);
}

void upy_global_set(upy_t *e, upy_val_t name, upy_val_t v) {
    upy_object_set(e, e->globals, name, v);
}

upy_val_t upy_global_delete(upy_t *e, upy_val_t name) {
    return upy_object_delete(e, e->globals, name);
}

upy_val_t upy_global_get_cs(upy_t *e, const char *name) {
    return upy_object_get(e, e->globals, upy_mk_string(name));
}

void upy_global_set_cs(upy_t *e, const char *name, upy_val_t v) {
    upy_object_set(e, e->globals, upy_mk_string(name), v);
}

upy_val_t upy_global_delete_cs(upy_t *e, const char *name) {
    return upy_object_delete(e, e->globals, upy_mk_string(name));
}

upy_val_t upy_array_create(upy_t *e) {
    upy_object_t *obj = object_create(e, TYPE_ARRAY);
    obj->u.a = upy_malloc(sizeof (upy_array_t));
    assert(obj->u.a);
    memset(obj->u.a, 0, sizeof (upy_array_t));
    return mk_object(obj);
}

int upy_array_size(upy_val_t o) {
    if( !upy_is_array(o) )
        return 0;
    upy_object_t *obj = upy_2_pointer(o);
    return obj->u.a->size;
}

upy_val_t upy_array_get(upy_t *e, upy_val_t o, int index) {
    UNUSED(e);
    if( !upy_is_array(o) )
        return VAL_UNDEF;
    upy_object_t *obj = upy_2_pointer(o);
    if( !obj->u.a )
        return VAL_UNDEF;
    if( index >= obj->u.a->count )
        return VAL_UNDEF;
    return obj->u.a->vals[index];
}

upy_val_t upy_array_set(upy_t *e, upy_val_t o, int index, upy_val_t v) {
    UNUSED(e);
    int expand_size = UPY_ARRAY_EXPAND_SIZE;
    if( !upy_is_array(o) )
        return VAL_FALSE;
    upy_object_t *obj = upy_2_pointer(o);
    int size = 0;
    if( !obj->u.a ) {
        obj->u.a = upy_malloc(sizeof(upy_array_t));
        assert(obj->u.a);
        obj->u.a->vals = (upy_val_t*)upy_malloc(sizeof(upy_val_t) * (size_t)expand_size);
        assert(obj->u.a->vals);
        size = obj->u.a->size = expand_size;
    } else {
        size = obj->u.a->size;
    }
    expand_size = size;
    if( index >= size ) {
        expand_size += (index - size + 1) / UPY_ARRAY_EXPAND_SIZE * UPY_ARRAY_EXPAND_SIZE;
        if( (index - size + 1) % UPY_ARRAY_EXPAND_SIZE )
            expand_size += UPY_ARRAY_EXPAND_SIZE;
        upy_val_t *old = obj->u.a->vals;
        upy_val_t *new = (upy_val_t*)upy_malloc(sizeof(upy_val_t) * (size_t)expand_size);
        assert(new);
        for(int i = 0; i < size; i++) {
            new[i] = old[i];
        }
        upy_free(old);
        obj->u.a->vals = new;
        obj->u.a->size = expand_size;
    }

    if( index >= obj->u.a->count ){
        obj->u.a->count = index + 1;
    }

    obj->u.a->vals[index] = v;
    return VAL_TRUE;
}

upy_val_t upy_array_delete(upy_t *e, upy_val_t o, int index) {
    UNUSED(e);
    if( !upy_is_array(o) )
        return VAL_FALSE;
    upy_object_t *obj = upy_2_pointer(o);
    if( !obj->u.a )
        return VAL_FALSE;
    if( index >= obj->u.a->count ) {
        return VAL_FALSE;
    }
    if( index < obj->u.a->count - 1)
        memmove(obj->u.a->vals + index, obj->u.a->vals + 1 + index, (size_t)(obj->u.a->count - index) * sizeof(upy_val_t));
    else{
        obj->u.a->vals[index] = VAL_UNDEF;
    }
    obj->u.a->count--;
    return VAL_TRUE;
}

upy_val_t upy_string_create(upy_t *e, const char *s) {
    upy_object_t *obj = object_create(e, TYPE_HEAP_STRING);
    obj->u.s = upy_malloc(sizeof (upy_string_t));
    assert(obj->u.s);
    obj->u.s->size = (int)strlen(s);
    obj->u.s->buf = upy_malloc((size_t)obj->u.s->size + 1);
    assert(obj->u.s->buf);
    memcpy(obj->u.s->buf, s, (size_t)obj->u.s->size);
    obj->u.s->buf[obj->u.s->size] = 0;
    return mk_heap_string(obj);
}

upy_val_t upy_lstring_create(upy_t *e, const char *s, int n) {
    upy_object_t *obj = object_create(e, TYPE_HEAP_STRING);
    if( n >= (int)strlen(s) ) {
        n = (int)strlen(s);
    }
    obj->u.s = upy_malloc(sizeof (upy_string_t));
    assert(obj->u.s);
    obj->u.s->size = n;
    obj->u.s->buf = upy_malloc((size_t)obj->u.s->size + 1);
    assert(obj->u.s->buf);
    memcpy(obj->u.s->buf, s, (size_t)obj->u.s->size);
    obj->u.s->buf[obj->u.s->size] = 0;
    return mk_heap_string(obj);
}

int upy_string_size(upy_val_t o) {
    if( upy_is_heap_string(o) ) {
        upy_object_t *obj = (upy_object_t*)upy_2_pointer(o);
        return obj->u.s->size;
    } else if( upy_is_foreign_string(o) ) {
        const char *s = (const char *)upy_2_pointer(o);
        return (int)strlen(s);
    }
    return 0;
}

upy_val_t upy_buffer_create(upy_t *e, uint8_t *buf, int size) {
    upy_object_t *obj = object_create(e, TYPE_HEAP_STRING);
    obj->u.s->size = size;
    obj->u.s->buf = upy_malloc((size_t)obj->u.s->size);
    assert(obj->u.s->buf);
    memcpy(obj->u.s->buf, buf, (size_t)obj->u.s->size);
    return mk_buffer(obj);
}

upy_val_t upy_buffer_create_empty(upy_t *e, int size) {
    upy_object_t *obj = object_create(e, TYPE_HEAP_STRING);
    obj->u.s->size = size;
    obj->u.s->buf = upy_malloc((size_t)obj->u.s->size);
    assert(obj->u.s->buf);
    memset(obj->u.s->buf, 0, (size_t)obj->u.s->size);
    return mk_buffer(obj);
}

int upy_buffer_size(upy_val_t o) {
    if( upy_is_buffer(o) ) {
        upy_object_t *obj = (upy_object_t*)upy_2_pointer(o);
        return obj->u.b->size;
    }
    return 0;
}

uint8_t* upy_2_buffer(upy_val_t o){
    if( upy_is_buffer(o) ) {
        upy_object_t *obj = (upy_object_t*)upy_2_pointer(o);
        return obj->u.b->buf;
    }
    return NULL;
}

void *upy_savetrypc(upy_t *e, upy_val_t scope, int pc)
{
    if (e->trybuf_count >= UPY_TRYBUF_LIMIT)
        upy_throw(e, upy_mk_string("Stack overflow"));
    e->trybuf[e->trybuf_count].scope = scope;
    e->trybuf[e->trybuf_count].pc = pc;
    e->trybuf[e->trybuf_count].sp = e->sp;
    return e->trybuf[e->trybuf_count++].buf;
}

#define upy_trypc(e, scope, PC) upy_setjmp(upy_savetrypc(e, scope, PC))

static void droptrypc(upy_t *e)
{
    if (e->trybuf_count > 0) {
        --e->trybuf_count;
    }
}

void upy_throw(upy_t *e, upy_val_t error) {
    if (e->trybuf_count > 0) {
        --e->trybuf_count;
        e->sp = e->trybuf[e->trybuf_count].sp;
        upy_push(e, error);
        upy_longjmp(e->trybuf[e->trybuf_count].buf, 1);
    }
}

void upy_throw_str(upy_t *e, const char *str) {
    upy_throw(e, upy_string_create(e, str));
}

static upy_val_t upy_import(upy_t *e, upy_val_t self, const char *str) {
    upy_val_t fn = upy_global_get_cs(e, "import");
    upy_val_t arg = upy_string_create(e, str);
    return upy_call(e, fn, self, 1, &arg);
}

static inline void check_stack(upy_t *e) {
    if( e->sp >= e->sp_high ){
        upy_throw(e, upy_mk_string("Stack overflow"));
    }
}

static upy_val_t call(upy_t *e, upy_val_t script, upy_val_t self, int argc, upy_val_t *values) {
    check_stack(e);
    switch (upy_type(script)) {
    case TYPE_MODULE:
    case TYPE_FUNCTION: {
        upy_object_t *obj = upy_2_pointer(script);
        upy_val_t *sp = e->sp;
        e->sp += obj->u.f->nvars;
        upy_val_t res = upy_run(e, obj, upy_2_pointer(self), values + 1);
        e->sp = sp;
        return res;
    }break;
    case TYPE_NATIVE: {
        upy_native_t native = (upy_native_t)upy_2_pointer(script);
        upy_val_t *sp = e->sp;
        upy_val_t res = native(e, self, argc, values);
        e->sp = sp;
        return res;
    }break;
    case TYPE_CLASS: {
        upy_val_t *sp = e->sp;
        upy_val_t __init__ = upy_object_get_cs(e, script, "__init__");
        upy_val_t res = upy_object_duplicate(e, script);
        if( __init__ != VAL_UNDEF)
            upy_call(e, __init__, res, argc, values);
        e->sp = sp;
        return res;
    }break;
    case TYPE_FOREIGN:{
        if( !e->foreign_caller )
            upy_throw(e, upy_mk_string("No foreign caller"));
        upy_native_t native = (upy_native_t)e->foreign_caller;
        upy_val_t *sp = e->sp;
        upy_val_t res = native(e, script, argc, values);
        e->sp = sp;
        return res;
    } break;
    default:
        upy_throw(e, upy_mk_string("Object is not callable"));
        break;
    }
    return VAL_UNDEF;
}

static upy_val_t __next__(upy_t *e, upy_val_t self, int argc, upy_val_t *v) {
    UNUSED(argc);UNUSED(v);
    if( !upy_is_object(self) ) {
        return VAL_UNDEF;
    }
    upy_val_t iter = upy_object_get_cs(e, self, "__object__");
    if( upy_is_string(iter) ) {
        int len = upy_string_size(iter);
        const char *str = upy_2_string(iter);
        int count = upy_2_int32(upy_object_get_cs(e, self, "__count__"));
        if( count < len ) {
            upy_object_set_cs(e, self, "__count__", upy_mk_number(count + 1));
            return upy_lstring_create(e, str + count, 1);
        }
    } else if( upy_is_array(iter) ) {
        int len = upy_array_size(iter);
        int count = upy_2_int32(upy_object_get_cs(e, self, "__count__"));
        if( count < len ) {
            upy_object_set_cs(e, self, "__count__", upy_mk_number(count + 1));
            return upy_array_get(e, self, count);
        }
    }
    return VAL_UNDEF;
}

static upy_val_t iterator(upy_t *e, upy_val_t o) {
    upy_val_t res = upy_object_create(e);
    upy_object_set_cs(e, res, "__object__", o);
    upy_object_set_cs(e, res, "__next__", upy_mk_native(__next__));
    upy_object_set_cs(e, res, "__count__", upy_mk_number(0));
    upy_object_set_cs(e, res, "__finished__", upy_mk_boolean(0));
    return res;
}

static upy_val_t upy_run(upy_t *e, upy_object_t *script, upy_object_t *self, upy_val_t *values)
{
    uint8_t *code = script->u.f->code;
    int code_size = script->u.f->code_size;
    int opcode;
    int addr = 0;
    int pc = 0;
    int line = 1;
    uint16_t key;
    for(;;)
    {
        opcode = code[pc++];
        switch(opcode){
        case op_get_local: *(++e->sp) = values[get_int16(code + pc)];pc += 2;break;
        case op_get_var:{
            key = get_uint16(code + pc);
            const char *s = upy_key_2_str(e, key);
            pc += 2;
            upy_val_t res = upy_object_get_cs(e, mk_object(self), s);
            if( res == VAL_UNDEF ) {
                res = upy_object_get_cs(e, e->globals, s);
                if( res == VAL_UNDEF )
                    upy_throw(e, upy_mk_string("undefined variable"));
            }
            *(++e->sp) = res;
        } break;
        case op_get_number:*(++e->sp) = upy_mk_number(get_double(code + pc));pc += 8;break;
        case op_get_none:*(++e->sp) = upy_mk_none();break;
        case op_get_true:*(++e->sp) = upy_mk_true();break;
        case op_get_false:*(++e->sp) = upy_mk_false();break;
        case op_get_string:{
            key = get_uint16(code + pc);
            pc += 2;
            const char *s = upy_key_2_str(e, key);
            *(++e->sp) = upy_mk_string(s);
        }break;
        case op_set_local:values[get_int16(code + pc)] = *e->sp;pc += 2;break;;
        case op_set_var:{
            key = get_uint16(code + pc);
            const char *s = upy_key_2_str(e, key);
            pc += 2;
            upy_object_set_cs(e, mk_object(self), s, *e->sp);
        }break;
        case op_add:*(e->sp - 1) = upy_add(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_sub:
            *(e->sp - 1) = upy_sub(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_mul:*(e->sp - 1) = upy_mul(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_div:*(e->sp - 1) = upy_div(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_mod:*(e->sp - 1) = upy_mod(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_and:*(e->sp - 1) = upy_and(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_or:*(e->sp - 1) = upy_or(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_xor:*(e->sp - 1) = upy_xor(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_negative: *e->sp = upy_negative(e, *e->sp);break;
        case op_not:*e->sp = upy_not(e, *e->sp);break;
        case op_invert:*e->sp = upy_invert(e, *e->sp);break;
        case op_left_shift:*(e->sp - 1) = upy_left_shift(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_right_shift:*(e->sp - 1) = upy_right_shift(e, *(e->sp - 1), *e->sp);e->sp--;break;
        case op_logic_and:
            *(e->sp - 1) = upy_logic_and(e, *(e->sp - 1), *e->sp);
            e->sp--;
            break;
        case op_logic_or:
            *(e->sp - 1) = upy_logic_or(e, *(e->sp - 1), *e->sp);
            e->sp--;
            break;
        case op_call: {
            uint16_t argc = get_uint16(code + pc);
            pc += 2;
            *(e->sp - argc) = call(e, *(e->sp - argc), mk_object(self), argc, e->sp - argc + 1);
            e->sp -= argc;
        }break;
        case op_return_void:
            *e->sp = upy_mk_none();
            goto VM_END;
        case op_return:
            goto VM_END;
        case op_lt:
            *(e->sp - 1) = upy_mk_boolean(upy_compare(e, *(e->sp - 1), *e->sp) < 0);e->sp--;break;
        case op_gt:*(e->sp - 1) = upy_mk_boolean(upy_compare(e, *(e->sp - 1), *e->sp) > 0);e->sp--;break;
        case op_lteq:*(e->sp - 1) = upy_mk_boolean(upy_compare(e, *(e->sp - 1), *e->sp) <= 0);e->sp--;break;
        case op_gteq:*(e->sp - 1) = upy_mk_boolean(upy_compare(e, *(e->sp - 1), *e->sp) >= 0);e->sp--;break;
        case op_eq:*(e->sp - 1) = upy_mk_boolean(upy_compare(e, *(e->sp - 1), *e->sp) == 0);e->sp--;break;
        case op_neq:*(e->sp - 1) = upy_mk_boolean(upy_compare(e, *(e->sp - 1), *e->sp) != 0);e->sp--;break;
        case op_sdn: e->sp--;break;
        case op_sup: e->sp++;break;
        case op_pop: *(e->sp - 1) = *e->sp; e->sp--;break;
        case op_dup: *(e->sp + 1) = *e->sp; e->sp++;break;
        case op_prop_get: {
            key = get_uint16(code + pc);
            pc += 2;
            *(e->sp + 1) = *e->sp;
            upy_val_t obj = *e->sp;
            upy_val_t res = VAL_UNDEF;
            while(obj != VAL_UNDEF) {
                res = upy_object_get_cs(e, obj, upy_key_2_str(e, key));
                if( res != VAL_UNDEF ) {
                    *e->sp = res;
                    break;
                }
                obj = upy_object_get_parent(obj);
            }
            *e->sp = res;
        }break;
        case op_prop_set:
            key = get_uint16(code + pc);
            pc += 2;
            upy_object_set_cs(e, *(e->sp - 1), upy_key_2_str(e, key), *e->sp);
            e->sp--;
            break;
        case op_prop_call:{
            uint16_t argc = get_uint16(code + pc);
            pc += 2;
            upy_val_t self_obj = *(e->sp - argc);
            //module has no self argument in function
            if( upy_type(self_obj) == TYPE_MODULE )
                *(e->sp - argc - 1) = call(e, *(e->sp - argc - 1), self_obj, argc, e->sp - argc + 1);
            else
                *(e->sp - argc - 1) = call(e, *(e->sp - argc - 1), self_obj, argc + 1, e->sp - argc);
            e->sp -= argc + 1;
        }break;
        case op_get_array:
            if( upy_is_number(*e->sp) ) {
                *(e->sp - 1) = upy_array_get(e, *(e->sp - 1), upy_2_int32(*e->sp));
                e->sp--;
            } else {
                upy_throw(e, upy_mk_string("Object is not subscriptable"));
            }
            break;
        case op_set_array:
            if( upy_is_number(*e->sp - 1) ) {
                *(e->sp - 2) = upy_array_set(e, *(e->sp - 2), upy_2_int32(*e->sp - 1), *e->sp);
                e->sp -= 2;
            } else {
                upy_throw(e, upy_mk_string("Object is not subscriptable"));
            }
            break;
        case op_branch: {
            key = (uint16_t)get_int32(code + pc);
            addr = branch(code, code_size, 0, key);
            if( addr < 0 )
                upy_throw(e, upy_mk_string("Invalid jump address"));
            code[pc - 1] = op_jmp;
            set_int32(code + pc, addr);
            pc = addr;
        }break;
        case op_branch_true:
            key = (uint16_t)get_int32(code + pc);
            addr = branch(code, code_size, 0, key);
            if( addr < 0 )
                upy_throw(e, upy_mk_string("Invalid jump address"));
            code[pc - 1] = op_jmp_true;
            set_int32(code + pc, addr);
            if( upy_is_boolean(*e->sp) && upy_2_boolean(*e->sp) ){
                pc = addr;
            } else
                pc += 4;
            e->sp--;
            break;
        case op_branch_false:
            key = (uint16_t)get_int32(code + pc);
            addr = branch(code, code_size, 0, key);
            if( addr < 0 )
                upy_throw(e, upy_mk_string("Invalid jump address"));
            code[pc - 1] = op_jmp_false;
            set_int32(code + pc, addr);
            if( upy_is_boolean(*e->sp) && upy_2_boolean(*e->sp) == 0 ){
                pc = addr;
            } else
                pc += 4;
            e->sp--;
            break;
        case op_jmp: {
            pc = get_int32(code + pc);
        }break;
        case op_jmp_true:
            addr = get_int32(code + pc);
            if( upy_is_boolean(*e->sp) && upy_2_boolean(*e->sp) ){
                pc = addr;
            } else
                pc += 4;
            e->sp--;
            break;
        case op_jmp_false:
            addr = get_int32(code + pc);
            if( upy_is_boolean(*e->sp) && upy_2_boolean(*e->sp) ==0 ){
                pc = addr;
            } else
                pc += 4;
            e->sp--;
            break;
        case op_dict:pc += 2;break;
        case op_array:pc += 2;break;
        case op_self:*(++e->sp) = mk_object(self);break;
        case op_line:line = get_int32(code + pc); pc += 4;break;
        case op_label:pc += 2;break;
        case op_iter: *e->sp = iterator(e, *e->sp); break;
        case op_next:
            addr = get_int32(code + pc);
            *(e->sp + 2) = __next__(e, *e->sp, 0, NULL);
            e->sp += 2;
            if( *e->sp == VAL_UNDEF ) {
                e->sp -= 2;
                pc = addr;
            } else {
                pc += 4;
            }
            break;
        case op_try:
            addr = get_int32(code + pc);
            pc += 4;
            if (upy_trypc(e, mk_script(script), addr) ) {
                pc = e->trybuf[e->trybuf_count].pc;
            }
            break;
        case op_catch:{
            addr = get_int32(code + pc);
            pc += 4;
            if( upy_compare(e, *(e->sp - 1), *e->sp) != 0 ){
                e->sp--;
                pc = addr;
            } else {
                e->sp -= 2;
            }
        }break;
        case op_throw:
            upy_throw(e, *e->sp);
            break;
        case op_import: {
            key = get_uint16(code + pc);
            pc += 2;
            *(++e->sp) = upy_import(e, mk_object(self), upy_key_2_str(e, key));
        } break;
        case op_stop:
            goto VM_END;
        default:
            upy_throw(e, upy_mk_string("Invalid opcode"));
        }
    }
    VM_END:
    return *e->sp;
}

static void gc_mark_object(upy_t *e, upy_object_t *obj);

static void gc_mark_function(upy_t *e, upy_object_t *obj) {
    upy_function_t *func = obj->u.f;
    if( !func )
        return;
    if(func->scope) {
        gc_mark_object(e, func->scope);
    }
}

static void gc_mark_array(upy_t *e, upy_object_t *obj) {
    upy_array_t *arr = obj->u.a;
    if( !arr )
        return;
    for(int i = 0; i < arr->count; i++ ) {
        switch (upy_type(arr->vals[i])) {
        case TYPE_MODULE:
        case TYPE_FUNCTION:
        case TYPE_CLASS:
        case TYPE_OBJECT:
        case TYPE_BUFFER:
        case TYPE_HEAP_STRING:
            gc_mark_object(e, upy_2_pointer(arr->vals[i]));
            break;
        case TYPE_ARRAY:
            gc_mark_array(e, upy_2_pointer(arr->vals[i]));
            gc_mark_object(e, upy_2_pointer(arr->vals[i]));
            break;
        }
    }
}

static void gc_mark_object(upy_t *e, upy_object_t *obj) {
    if( !obj )
        return;
    if( obj->mark )
        return;
    obj->mark = 1;

    if( obj->p.parent )
        gc_mark_object(e, obj->p.parent);
    upy_member_t *next = obj->member;
    while(next) {
        switch (upy_type(next->name)) {
        case TYPE_HEAP_STRING:
            gc_mark_object(e, upy_2_pointer(next->name));
            break;
        }
        next = next->next;

        switch (upy_type(next->val)) {
        case TYPE_MODULE:
        case TYPE_FUNCTION:
            gc_mark_function(e, upy_2_pointer(next->val));
            gc_mark_object(e, upy_2_pointer(next->val));
            break;
        case TYPE_CLASS:
        case TYPE_OBJECT:
        case TYPE_BUFFER:
        case TYPE_HEAP_STRING:
            gc_mark_object(e, upy_2_pointer(next->val));
            break;
        case TYPE_ARRAY:
            gc_mark_array(e, upy_2_pointer(next->val));
            gc_mark_object(e, upy_2_pointer(next->val));
            break;
        }
        next = next->next;
    }
}

static void gc_mark_stack(upy_t *e) {
    for (upy_val_t *sp = e->sp_base; sp <= e->sp; sp++) {
        switch (upy_type(*sp)) {
        case TYPE_MODULE:
        case TYPE_FUNCTION:
        case TYPE_CLASS:
        case TYPE_OBJECT:
        case TYPE_BUFFER:
        case TYPE_HEAP_STRING:
            gc_mark_object(e, upy_2_pointer(*sp));
            break;
        case TYPE_ARRAY:
            gc_mark_array(e, upy_2_pointer(*sp));
            gc_mark_object(e, upy_2_pointer(*sp));
            break;
        }
    }
}

static void gc_free_object(upy_t *e, upy_object_t *obj) {
    UNUSED(e);
    switch (obj->type) {
    case TYPE_MODULE:
    case TYPE_FUNCTION:
    case TYPE_CLASS:
    case TYPE_OBJECT:
    case TYPE_BUFFER:
    case TYPE_HEAP_STRING:
    case TYPE_ARRAY:{
        upy_member_t *next = obj->member;
        while(next) {
            upy_member_t *local = next;
            next = next->next;
            upy_free(local);
        }
    }break;
    }

    switch (obj->type) {
    case TYPE_MODULE:
    case TYPE_FUNCTION:
        if( !obj->u.f )
            break;
        if( !obj->u.f->code )
            upy_free(obj->u.f->code);
        upy_free(obj->u.f);
        break;
    case TYPE_BUFFER:
    case TYPE_HEAP_STRING:
        if( !obj->u.s )
            break;
        if( !obj->u.s->buf )
            upy_free(obj->u.s->buf);
        upy_free(obj->u.s);
        break;
    case TYPE_ARRAY:{
        upy_array_t *arr = obj->u.a;
        if( !arr )
            break;
        if( arr->vals )
            upy_free(arr->vals);
        upy_free(arr);
    }break;
    }
    upy_free(obj);
}

void upy_gc(upy_t *e) {
    if( e->pause )
        return;

    upy_object_t **prevnextobj;
    upy_object_t *obj, *nextobj;

    gc_mark_object(e, upy_2_pointer(e->globals));
    gc_mark_object(e, upy_2_pointer(e->scope));
    gc_mark_stack(e);

    prevnextobj = (upy_object_t **)&e->gcroot;
    for (obj = e->gcroot; obj; obj = nextobj) {
        nextobj = obj->next;
        if (!obj->mark) {
            *prevnextobj = nextobj;
            gc_free_object(e, obj);
        } else {
            prevnextobj = &obj->next;
        }
    }
    obj = e->gcroot;
    while(obj) {
        if( obj->mark == 1)
            obj->mark = 0;
        e->gcnext = obj;
        obj = obj->next;
    }
}

void upy_push(upy_t *e, upy_val_t v) {
    *(++e->sp) = v;
}

upy_val_t upy_pop(upy_t *e) {
    upy_val_t res = *e->sp;
    e->sp--;
    return res;
}

upy_val_t upy_call(upy_t *e, upy_val_t script, upy_val_t self, int argc, upy_val_t *values) {
    upy_val_t *sp = e->sp;
    upy_push(e, script);
    upy_push(e, self);
    upy_val_t *args = e->sp + 1;
    for(int i = 0; i < argc; i++) {
        upy_push(e, values[i]);
    }
    upy_val_t res = call(e, script, self, argc, args);
    e->sp = sp;
    return res;
}

static uint32_t hash_computed(const void *key)
{
    const char *ptr = (const char *)key;
    uint32_t val = 0;
    uint32_t tmp;
    while (*ptr) {
        val = (val << 4) + (uint32_t)*ptr;
        tmp = val & 0xf0000000;
        if (tmp) {
            val = (val ^ (tmp >> 24)) ^ tmp;
        }
        ptr++;
    }
    return val;
}

static uint16_t htbl_key(uint16_t size, uint32_t hash, uint16_t i) {
    return (hash + i * (hash * 2 + 1)) % size;
}

static const char *strpool_copy(const char *str)
{
    size_t size = strlen(str) + 1;
    char *sym = (char *)upy_malloc(size);
    assert(sym);
    memcpy(sym, str, size);
    return sym;
}

uint16_t upy_strpool_insert(upy_t *e, const char *str, int alloc)
{
    uint16_t i;
    uint32_t hash;
    uint16_t pos, gen = 0;
    string_pool_t *tbl = e->string_pool;
    hash = hash_computed(str);
    while(tbl){
        for (i = 0; i < UPY_STRING_POOL_SIZE; i++) {
            pos = htbl_key(UPY_STRING_POOL_SIZE, hash, i);
            intptr_t v = tbl->values[pos];
            if( v == 0) {
                const char * res;
                if( alloc )
                    res = strpool_copy(str);
                else
                    res = str;
                tbl->values[pos] = (intptr_t)res;
                tbl->count++;
                return pos + gen * UPY_STRING_POOL_SIZE;
            } else if( v == (intptr_t)str || !strcmp( (const char *)v, str ) ) {
                return pos + gen * UPY_STRING_POOL_SIZE;
            }
        }
        if( tbl->next == NULL){
            tbl->next = (string_pool_t*)upy_malloc(sizeof(string_pool_t));
            if( !tbl->next ) {
                goto STR_INSERT_ERR;
            }
            memset(tbl->next, 0, sizeof(string_pool_t));
            tbl->next->values = (intptr_t*)upy_malloc(UPY_STRING_POOL_SIZE * sizeof(intptr_t));
            if( !tbl->next->values ) {
                goto STR_INSERT_ERR;
            }
            memset(tbl->next->values, 0, UPY_STRING_POOL_SIZE * sizeof(intptr_t));
        }
        tbl = tbl->next;
        gen++;
    }
STR_INSERT_ERR:
    upy_throw(e, upy_mk_string("Insufficient memory"));
    return 0xFFFF;
}

const char *upy_key_2_str(upy_t *e, uint16_t key) {
    uint16_t gen = key / UPY_STRING_POOL_SIZE;
    uint16_t index = key - gen * UPY_STRING_POOL_SIZE;
    string_pool_t *tbl = e->string_pool;
    while(gen-- > 0) tbl = tbl->next;
    return (const char *)(tbl->values[index]);
}

static upy_val_t native_show(upy_t *e, upy_val_t self, int c, upy_val_t *v) {
    UNUSED(self);UNUSED(c);UNUSED(v);
    printf("stack usage = %ld bytes\r\n", (intptr_t)e->sp - (intptr_t)e->sp_base);
    return upy_mk_none();
}

static upy_val_t native_print(upy_t *e, upy_val_t self, int c, upy_val_t *v) {
    UNUSED(e);UNUSED(self);
    for(int i = 0; i < c; i++){
        if(upy_is_boolean(v[i])) {
            if(!upy_2_boolean(v[i]))
                printf("False");
            else
                printf("True");
        } else if(upy_is_none(v[i])){
            printf("None");
        }
        else if(upy_is_number(v[i]))
            if(upy_is_integer(v[i]))
                printf("%d", upy_2_int32(v[i]));
            else
                printf("%lf", upy_2_double(v[i]));
        else if(upy_is_string(v[i]))
            printf("%s", upy_2_string(v[i]));
        else if(upy_is_script(v[i]))
            printf("function");
        else if(upy_is_class(v[i]))
            printf("class");
        else if(upy_is_object(v[i]))
            printf("object");
        else if(upy_is_boolean(v[i])) {
            if( upy_2_boolean(v[i]))
                printf("True");
            else{
                printf("False");
            }
        } else if( upy_is_none(v[i])) {
            printf("None");
        } else if( upy_is_foreign(v[i])) {
            printf("foreign function");
        }
        printf(" ");
        v++;
    }
    printf("\r\n");
    return upy_mk_none();
}

static upy_val_t native_import(upy_t *e, upy_val_t self, int c, upy_val_t *v) {
    UNUSED(self);UNUSED(c);
    const char *path = upy_2_string(v[0]);
    upy_val_t module = upy_global_get_cs(e, "__modules__");
    module = upy_object_get_cs(e, module, path);
    if( module != VAL_UNDEF )
        return module;

    sprintf(e->name, "%s.py", path);
    upy_val_t res = upy_parse(e, e->name);
    if( res == VAL_UNDEF )
        upy_throw_str(e, "Module not found");
    upy_call(e, res, res, 0, NULL);
    return res;
}

static void native_init(upy_t *e) {
    upy_global_set_cs(e, "print", upy_mk_native(native_print));
    upy_global_set_cs(e, "show", upy_mk_native(native_show));
    upy_global_set_cs(e, "import", upy_mk_native(native_import));
}

upy_t *upy_init(size_t stack_size)
{
    upy_t *e = upy_malloc(sizeof (upy_t));
    memset(e, 0, sizeof (upy_t));
    e->sp = e->sp_base = (upy_val_t *)upy_malloc(stack_size * sizeof (upy_val_t));
    assert(e->sp_base);
    e->stack_size = stack_size;
    e->sp_high = e->sp_base + (int)(stack_size * UPY_STACK_FACTOR);

    e->string_pool = (string_pool_t*)upy_malloc(sizeof(string_pool_t));
    assert(e->string_pool);
    memset(e->string_pool, 0, sizeof(string_pool_t));
    ((string_pool_t*)e->string_pool)->values = (intptr_t*)upy_malloc(UPY_STRING_POOL_SIZE * sizeof(intptr_t));
    assert(((string_pool_t*)e->string_pool)->values);
    memset(((string_pool_t*)e->string_pool)->values, 0, UPY_STRING_POOL_SIZE * sizeof(intptr_t));

    e->globals = upy_object_create(e);
    native_init(e);
    upy_global_set_cs(e, "__modules__", upy_object_create(e));
    return e;
}

void upy_deinit(upy_t *e)
{
    upy_free(e->sp_base);
}

static void class_init(upy_t *e, upy_val_t o) {
    if( !upy_is_module(o) )
        return;

    upy_object_t *obj = upy_2_pointer(o);
    upy_member_t *next = obj->member;
    while(next) {
        if( upy_is_class(next->val) ){
            upy_val_t *sp = e->sp;
            upy_run(e, upy_2_pointer(next->val), upy_2_pointer(next->val), NULL);
            e->sp = sp;
        }
        next = next->next;
    }
}

upy_val_t upy_parse(upy_t *e, const char *file)
{
    FILE *fd = fopen(file, "r");
    if( !fd )
        return VAL_UNDEF;
    fseek (fd , 0 , SEEK_END);
    size_t size = (size_t)ftell (fd);
    rewind (fd);

    char *content = upy_malloc(size + 1);
    assert(content);
    fread (content, 1, size, fd);
    content[size] = 0;

    upy_val_t res = upy_parse_string(e, content);
    upy_free(content);
    return res;
}

upy_val_t upy_parse_string(upy_t *e, char *content)
{
    if( upy_try(e) ) {
        upy_val_t error = upy_pop(e);
        if( upy_is_string(error) ){
            printf("SyntaxError: %s\r\n", upy_2_string(error));
        }
        code_buf_upy_free();
        return VAL_UNDEF;
    }
    e->input = (char*)content;
    e->input_len = (int)strlen(content);
    e->row = 1;
    e->index = 0;
    compile_buf_index = 0;
    code_buf_init();
    upy_val_t *sp = e->sp;
    upy_val_t res = p_file_input(e);
    code_buf_upy_free();
    class_init(e,res);
    e->sp = sp;
    droptrypc(e);
    e->scope = res;
    return res;
}

void upy_foreign_caller_set(upy_t *e, upy_native_t caller) {
    e->foreign_caller = (void*)caller;
}

