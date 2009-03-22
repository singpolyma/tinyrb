#ifndef _TINYRB_H_
#define _TINYRB_H_

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include "config.h"
#include "vendor/kvec.h"
#include "vendor/khash.h"
#include "gc.h"

#define TR_MALLOC            GC_malloc
#define TR_CALLOC(m,n)       GC_MALLOC((m)*(n))
#define TR_REALLOC           GC_realloc
#define TR_FREE(S)           

#define TR_COBJECT(X)        ((TrObject*)TR_BOX(X))
#define TR_TYPE(X)           (TR_COBJECT(X)->type)
#define TR_IS_A(X,T)         (TR_TYPE(X) == TR_T_##T)
#define TR_CTYPE(X,T)        (tr_assert(TR_IS_A(X,T), "TypeError: expected " #T),(Tr##T*)(X))
#define TR_CCLASS(X)         (tr_assert(TR_IS_A(X,Class)||TR_IS_A(X,Module), "TypeError: expected Class"),(TrClass*)(X))
#define TR_CMODULE(X)        TR_CCLASS(X)
#define TR_CFIXNUM(X)        TR_CTYPE(X,Fixnum)
#define TR_CARRAY(X)         TR_CTYPE(X,Array)
#define TR_CHASH(X)          TR_CTYPE(X,Hash)
#define TR_CRANGE(X)         TR_CTYPE(X,Range)
#define TR_CPROC(X)          TR_CTYPE(X,Proc)
#define TR_CSTRING(X)        (tr_assert(TR_IS_A(X,String)||TR_IS_A(X,Symbol), "TypeError: expected String"),(TrString*)(X))
#define TR_CMETHOD(X)        ((TrMethod*)X)
#define TR_CBINDING(X)       TR_CTYPE(X,Binding)

#define TR_STR_PTR(S)        (TR_CSTRING(S)->ptr)
#define TR_STR_LEN(S)        (TR_CSTRING(S)->len)
#define TR_FIX2INT(F)        (TR_CFIXNUM(F)->value)
#define TR_ARRAY_PUSH(X,I)   kv_push(OBJ, ((TrArray*)(X))->kv, (I))
#define TR_ARRAY_AT(X,I)     kv_A((TR_CARRAY(X))->kv, (I))
#define TR_ARRAY_SIZE(X)     kv_size(TR_CARRAY(X)->kv)
#define TR_ARRAY_EACH(T,I,V,B) ({ \
    TrArray *__a##V = TR_CARRAY(T); \
    if (kv_size(__a##V->kv) != 0) { \
      size_t I; \
      for (I = 0; I < kv_size(__a##V->kv); I++) { \
        OBJ V = kv_A(__a##V->kv, I); \
        B \
      } \
    } \
  })
#define TR_KH_GET(KH,K) ({ \
  OBJ key = (K); \
  khash_t(OBJ) *kh = (KH); \
  khiter_t k = kh_get(OBJ, kh, key); \
  k == kh_end(kh) ? TR_NIL : kh_value(kh, k); \
})
#define TR_KH_SET(KH,K,V) ({ \
  OBJ key = (K); \
  khash_t(OBJ) *kh = (KH); \
  int ret; \
  khiter_t k = kh_put(OBJ, kh, key, &ret); \
  if (!ret) kh_del(OBJ, kh, k); \
  kh_value(kh, k) = (V); \
})
#define TR_KH_EACH(H,I,V,B) ({ \
    khiter_t __k##V; \
    for (__k##V = kh_begin(H); __k##V != kh_end(H); ++__k##V) \
      if (kh_exist((H), __k##V)) { \
        OBJ V = kh_value((H), __k##V); \
        B \
      } \
  })

#define TR_GETIVAR(O,N)      TR_KH_GET(TR_COBJECT(O)->ivars, N)
#define TR_SETIVAR(O,N,V)    TR_KH_SET(TR_COBJECT(O)->ivars, N, V)

#define VM                   struct TrVM *vm
#define FRAME                (&vm->frames[vm->cf])
#define PREV_FRAME           (&vm->frames[vm->cf-1])

#define TR_NIL               ((OBJ)0)
#define TR_FALSE             ((OBJ)1)
#define TR_TRUE              ((OBJ)2)
#define TR_TEST(X)           ((X) == TR_NIL || (X) == TR_FALSE ? 0 : 1)
#define TR_BOOL(X)           ((X) ? TR_TRUE : TR_FALSE)
#define TR_BOX(X)            ((X) < 3 ? vm->primitives[X] : (X))

#define TR_OBJECT_HEADER \
  TR_T type; \
  OBJ class; \
  khash_t(OBJ) *ivars
#define TR_INIT_OBJ(T) ({ \
  Tr##T *o = TR_ALLOC(Tr##T); \
  o->type  = TR_T_##T; \
  o->class = vm->classes[TR_T_##T]; \
  o->ivars = kh_init(OBJ); \
  o; \
})
#define TR_CLASS(T)          vm->classes[TR_T_##T]
#define TR_INIT_CLASS(T,S) \
  TR_CLASS(T) = TrObject_const_set(vm, vm->self, tr_intern(#T), \
                                   TrClass_new(vm, tr_intern(#T), TR_CLASS(S)))

#define tr_intern(S)         TrSymbol_new(vm, (S))
#define tr_raise(M,A...)     TrVM_raise(vm, tr_sprintf(vm, (M), ##A))
#define tr_raise_errno(M)    tr_raise("%s: %s", strerror(errno), (M))
#define tr_assert(X,M,A...)  ((X) ? (X) : TrVM_raise(vm, tr_sprintf(vm, (M), ##A)))
#define TR_RESCUE(B)         if (setjmp(FRAME->rescue_jmp)) { TrVM_rescue(vm); B }
#define tr_def(C,N,F,A)      TrModule_add_method(vm, (C), tr_intern(N), TrMethod_new(vm, (TrFunc *)(F), TR_NIL, (A)))
#define tr_metadef(O,N,F,A)  TrObject_add_singleton_method(vm, (O), tr_intern(N), TrMethod_new(vm, (TrFunc *)(F), TR_NIL, (A)))
#define tr_defclass(N)       TrObject_const_set(vm, vm->self, tr_intern(N), TrClass_new(vm, tr_intern(N)))
#define tr_defmodule(N)      TrObject_const_set(vm, vm->self, tr_intern(N), TrModule_new(vm, tr_intern(N)))
#define tr_send(R,MSG,A...)  ({ \
  OBJ r = TR_BOX(R); \
  TrMethod *m = TR_CMETHOD(TrObject_method(vm, r, (MSG))); \
  if (!m) tr_raise("Method not found: %s\n", TR_STR_PTR(MSG)); \
  FRAME->method = m; \
  m->func(vm, r, ##A); \
})
#define tr_send2(R,STR,A...) tr_send((R), tr_intern(STR), ##A)

typedef unsigned long OBJ;
typedef unsigned char u8;

KHASH_MAP_INIT_STR(str, OBJ);
KHASH_MAP_INIT_INT(OBJ, OBJ);

typedef enum {
  /*  0 */ TR_T_Object, TR_T_Module, TR_T_Class, TR_T_Method, TR_T_Binding,
  /*  5 */ TR_T_Symbol, TR_T_String, TR_T_Fixnum, TR_T_Range, TR_T_Proc,
  /*  9 */ TR_T_NilClass, TR_T_TrueClass, TR_T_FalseClass,
  /* 12 */ TR_T_Array, TR_T_Hash,
  /* 14 */ TR_T_Node,
  TR_T_MAX /* keep last */
} TR_T;

struct TrVM;
struct TrFrame;

typedef struct {
  u8 i, a, b, c;
} TrInst;

typedef struct {
  OBJ class;
  OBJ method;
  size_t miss;
} TrCallSite;

typedef struct TrBlock {
  /* static */
  kvec_t(OBJ) k; /* TODO rename to values ? */
  kvec_t(char *) strings;
  kvec_t(OBJ) locals;
  kvec_t(OBJ) upvals;
  kvec_t(TrInst) code;
  kvec_t(struct TrBlock *) blocks; /* TODO should not be pointers */
  size_t regc;
  size_t argc;
  size_t arg_splat;
  int inherit_scope:1;
  OBJ filename;
  size_t line;
  struct TrBlock *parent;
  /* dynamic */
  kvec_t(TrCallSite) sites;
} TrBlock;

typedef struct TrUpval {
  OBJ *value;
  OBJ closed; /* value when closed */
} TrUpval;

typedef struct TrClosure {
  TrBlock *block;
  TrUpval *upvals;
  struct TrFrame *frame;
} TrClosure;

typedef struct TrProc {
  TR_OBJECT_HEADER;
  TrClosure *closure;
} TrProc;

typedef OBJ (TrFunc)(VM, OBJ receiver, ...);
typedef struct {
  TR_OBJECT_HEADER;
  TrFunc *func;
  OBJ data;
  OBJ name;
  int arity;
} TrMethod;

typedef struct TrFrame {
  TrClosure *closure;
  TrMethod *method;  /* current called method */
  OBJ *stack;
  OBJ *upvals;
  OBJ self;
  OBJ class;
  OBJ filename;
  size_t line;
  TrInst *ip;
  jmp_buf rescue_jmp;
} TrFrame;

typedef struct {
  TR_OBJECT_HEADER;
  TrFrame *frame;
} TrBinding;

typedef struct TrVM {
  khash_t(str) *symbols;
  khash_t(OBJ) *globals;
  OBJ classes[TR_T_MAX];
  TrFrame frames[TR_MAX_FRAMES];
  size_t cf; /* current frame */
  khash_t(OBJ) *consts;
  OBJ self;
  OBJ primitives[3];
  int debug;
  OBJ exception;
} TrVM;

typedef struct {
  TR_OBJECT_HEADER;
} TrObject;

typedef struct {
  TR_OBJECT_HEADER;
  OBJ name;
  OBJ super;
  khash_t(OBJ) *methods;
  int meta:1;
} TrClass;
typedef TrClass TrModule;

typedef struct {
  TR_OBJECT_HEADER;
  char *ptr;
  size_t len;
  unsigned char interned:1;
} TrString;
typedef TrString TrSymbol;

typedef struct {
  TR_OBJECT_HEADER;
  int value;
} TrFixnum;

typedef struct {
  TR_OBJECT_HEADER;
  OBJ first;
  OBJ last;
  int exclusive;
} TrRange;

typedef struct {
  TR_OBJECT_HEADER;
  kvec_t(OBJ) kv;
} TrArray;

typedef struct {
  TR_OBJECT_HEADER;
  khash_t(OBJ) *kh;
} TrHash;

/* vm */
TrVM *TrVM_new();
OBJ TrVM_eval(VM, char *code, char *filename);
OBJ TrVM_load(VM, char *filename);
void TrVM_raise(VM, OBJ exception);
void TrVM_rescue(VM);
OBJ TrVM_run(VM, TrBlock *b, OBJ self, OBJ class, int argc, OBJ argv[]);
OBJ TrVM_run2(VM, TrBlock *b, OBJ self, OBJ class, int argc, OBJ argv[], TrClosure *cl);
void TrVM_destroy(TrVM *vm);

/* string */
OBJ TrSymbol_new(VM, const char *str);
OBJ TrString_new(VM, const char *str, size_t len);
OBJ TrString_new2(VM, const char *str);
OBJ TrString_new3(VM, size_t len);
OBJ tr_sprintf(VM, const char *fmt, ...);
void TrSymbol_init(VM);
void TrString_init(VM);

/* number */
OBJ TrFixnum_new(VM, int value);
OBJ TrFixnum_add(VM, OBJ self, OBJ other);
OBJ TrFixnum_sub(VM, OBJ self, OBJ other);
OBJ TrFixnum_lt(VM, OBJ self, OBJ other);
void TrFixnum_init(VM);

/* array */
OBJ TrArray_new(VM);
OBJ TrArray_new2(VM, int argc, ...);
OBJ TrArray_new3(VM, int argc, OBJ items[]);
void TrArray_init(VM);

/* hash */
OBJ TrHash_new(VM);
OBJ TrHash_new2(VM, size_t n, OBJ items[]);
void TrHash_init(VM);

/* range */
OBJ TrRange_new(VM, OBJ start, OBJ end, int exclusive);
void TrRange_init(VM);

/* proc */
TrClosure *TrClosure_new(VM, TrBlock *b);
void TrProc_init(VM);

/* object */
OBJ TrObject_new(VM);
OBJ TrObject_method(VM, OBJ self, OBJ name);
OBJ TrObject_const_set(VM, OBJ self, OBJ name, OBJ value);
OBJ TrObject_const_get(VM, OBJ self, OBJ name);
OBJ TrObject_add_singleton_method(VM, OBJ self, OBJ name, OBJ method);
void TrObject_preinit(VM);
void TrObject_init(VM);

/* module */
OBJ TrModule_new(VM, OBJ name);
OBJ TrModule_instance_method(VM, OBJ self, OBJ name);
OBJ TrModule_add_method(VM, OBJ self, OBJ name, OBJ method);
OBJ TrModule_include(VM, OBJ self, OBJ mod);
void TrModule_init(VM);

/* kernel */
void TrKernel_init(VM);

/* class */
OBJ TrClass_new(VM, OBJ name, OBJ super);
OBJ TrMetaClass_new(VM, OBJ super);
OBJ TrClass_allocate(VM, OBJ self);
void TrClass_init(VM);

/* method */
OBJ TrMethod_new(VM, TrFunc *func, OBJ data, int arity);
void TrMethod_init(VM);
void TrBinding_init(VM);

/* primitive */
void TrPrimitive_init(VM);

/* compiler */
TrBlock *TrBlock_compile(VM, char *code, char *fn, size_t lineno);
void TrBlock_dump(VM, TrBlock *b);

#endif /* _TINYRB_H_ */
