#include <hl.h>

vclosure *hl_alloc_closure_void( hl_type *t, void *fvalue ) {
	vclosure *c = (vclosure*)hl_gc_alloc(sizeof(vclosure));
	c->t = t;
	c->fun = fvalue;
	c->hasValue = false;
	return c;
}

static hl_type *hl_get_closure_type( hl_type *t ) {
	hl_type_fun *ft = t->fun;
	if( ft->closure_type.kind != HFUN ) {
		if( ft->nargs == 0 ) hl_fatal("assert");
		ft->closure_type.kind = HFUN;
		ft->closure_type.p = &ft->closure;
		ft->closure.nargs = ft->nargs - 1;
		ft->closure.args = ft->closure.nargs ? ft->args + 1 : NULL;
		ft->closure.ret = ft->ret;
		ft->closure.parent = t;
	}
	return (hl_type*)&ft->closure_type;
}

vclosure *hl_alloc_closure_ptr( hl_type *fullt, void *fvalue, void *v ) {
	vclosure *c = (vclosure*)hl_gc_alloc(sizeof(vclosure));
	c->t = hl_get_closure_type(fullt);
	c->fun = fvalue;
	c->hasValue = 1;
	c->value = v;
	return c;
}

vclosure *hl_alloc_closure_wrapper( hl_type *t, void *fvalue, void *v ) {
	vclosure *c = (vclosure*)hl_gc_alloc(sizeof(vclosure));
	c->t = t;
	c->fun = fvalue;
	c->hasValue = 2;
	c->value = v;
	return c;
}

HL_PRIM vdynamic *hl_make_var_args( vclosure *c ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vdynamic *hl_no_closure( vdynamic *c ) {
	vclosure *cl = (vclosure*)c;
	if( !cl->hasValue ) return c;
	if( cl->hasValue == 2 ) hl_fatal("TODO"); // wrapper
	return (vdynamic*)hl_alloc_closure_void(cl->t->fun->parent,cl->fun);
}

HL_PRIM vdynamic* hl_get_closure_value( vdynamic *c ) {
	vclosure *cl = (vclosure*)c;
	if( cl->hasValue == 2 )
		return hl_get_closure_value((vdynamic*)cl->value);
	return (vdynamic*)cl->value;
}

void *hlc_dyn_call( void *fun, hl_type *t, vdynamic **args );

HL_PRIM vdynamic* hl_call_method( vdynamic *c, varray *args ) {
	vclosure *cl = (vclosure*)c;
	int i;
	vdynamic **vargs = (vdynamic**)(args + 1);
	if( cl->hasValue ) hl_error("Can't call closure with value");
	if( args->size != cl->t->fun->nargs || args->at->kind != HDYN ) hl_error("Invalid args");
	for(i=0;i<args->size;i++) {
		vdynamic *v = vargs[i];
		hl_type *t = cl->t->fun->args[i];
		if( v == NULL ) {
			if( hl_is_ptr(t) )
				continue;
			v = hl_alloc_dynamic(t);
			v->v.d = 0;
			vargs[i] = v;
		} else if( !hl_safe_cast(v->t,t) )
			hl_write_dyn(vargs + i, t, v);
	}
	return (vdynamic*)hlc_dyn_call(cl->fun,cl->t,vargs);
}

bool hl_fun_compare( vdynamic *a, vdynamic *b ) {
	vclosure *ca, *cb;
	if( a == b )
		return true;
	if( !a || !b )
		return false;
	if( a->t->kind != b->t->kind || a->t->kind != HFUN )
		return false;
	ca = (vclosure*)a;
	cb = (vclosure*)b;
	if( ca->fun != cb->fun )
		return false;
	if( ca->hasValue && ca->value != cb->value )
		return false;
	return true;
}