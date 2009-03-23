#include "tr.h"
#include "internal.h"

TrClosure *TrClosure_new(VM, TrBlock *b, OBJ self, OBJ class, TrClosure *parent) {
  TrClosure *cl = TR_ALLOC(TrClosure);
  cl->block = b;
  cl->upvals = TR_ALLOC_N(TrUpval, kv_size(b->upvals));
  cl->self = self;
  cl->class = class;
  cl->parent = parent;
  return cl;
}

OBJ TrProc_new(VM) {
  TrProc *p = TR_INIT_OBJ(Proc);
  p->closure = FRAME->closure;
  return (OBJ)p;
}

static OBJ TrProc_call(VM, OBJ self, int argc, OBJ argv[]) {
  TrClosure *cl = TR_CPROC(self)->closure;
  return TrVM_run2(vm, cl->block, cl->self, cl->class, argc, argv, cl);
}

void TrProc_init(VM) {
  OBJ c = TR_INIT_CLASS(Proc, Object);
  tr_def(c, "call", TrProc_call, -1);
  tr_metadef(c, "new", TrProc_new, 0);
}
