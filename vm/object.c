#include "tr.h"
#include "internal.h"

OBJ TrObject_new(VM) {
  return (OBJ) TR_INIT_CORE_OBJECT(Object);
}

int TrObject_type(VM, OBJ obj) {
  switch (obj) {
    case TR_NIL: return TR_T_NilClass;
    case TR_TRUE: return TR_T_TrueClass;
    case TR_FALSE: return TR_T_FalseClass;
  }
  if (TR_IS_FIX(obj)) return TR_T_Fixnum;
  return TR_COBJECT(obj)->type;
}

OBJ TrObject_method(VM, OBJ self, OBJ name) {
  return TrModule_instance_method(vm, TR_CLASS(self), name);
}

/* TODO respect namespace */
OBJ TrObject_const_get(VM, OBJ self, OBJ name) {
  khiter_t k = kh_get(OBJ, vm->consts, name);
  if (k != kh_end(vm->consts)) return kh_value(vm->consts, k);
  return TR_NIL;
}

OBJ TrObject_const_set(VM, OBJ self, OBJ name, OBJ value) {
  int ret;
  khiter_t k = kh_put(OBJ, vm->consts, name, &ret);
  if (!ret) kh_del(OBJ, vm->consts, k);
  kh_value(vm->consts, k) = value;
  return value;
}

OBJ TrObject_add_singleton_method(VM, OBJ self, OBJ name, OBJ method) {
  TrObject *o = TR_COBJECT(self);
  if (!TR_CCLASS(o->class)->meta)
    o->class = TrMetaClass_new(vm, o->class);
  assert(TR_CCLASS(o->class)->meta && "first class must be the metaclass");
  TrModule_add_method(vm, o->class, name, method);
  return method;
}

static OBJ TrObject_class(VM, OBJ self) {
  OBJ class = TR_CLASS(self);
  /* find the first non-metaclass */
  while (class && (!TR_IS_A(class, Class) || TR_CCLASS(class)->meta))
    class = TR_CCLASS(class)->super;
  assert(class && "classless object");
  return class;
}

static OBJ TrObject_object_id(VM, OBJ self) {
  return TR_INT2FIX((int)&self);
}

static OBJ TrObject_instance_eval(VM, OBJ self, OBJ code) {
  TrBlock *b = TrBlock_compile(vm, TR_STR_PTR(code), "<eval>", 0);
  return TrVM_run(vm, b, self, TR_COBJECT(self)->class, 0, 0);
}

static OBJ TrObject_inspect(VM, OBJ self) {
  const char *name;
  name = TR_STR_PTR(tr_send2(tr_send2(self, "class"), "name"));
  return tr_sprintf(vm, "#<%s:%p>", name, (void*)self);
}

void TrObject_preinit(VM) {
  TR_INIT_CORE_CLASS(Object, /* ignored */ Object);
}

void TrObject_init(VM) {
  OBJ c = TR_CORE_CLASS(Object);
  tr_def(c, "class", TrObject_class, 0);
  tr_def(c, "method", TrObject_method, 1);
  tr_def(c, "object_id", TrObject_object_id, 0);
  tr_def(c, "instance_eval", TrObject_instance_eval, 1);
  tr_def(c, "to_s", TrObject_inspect, 0);
  tr_def(c, "inspect", TrObject_inspect, 0);
}
