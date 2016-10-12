struct ssaModule;
struct ssaProcedure;
struct ssaBlock;
struct ssaValue;
struct ssaInstr;

enum ssaDebugInfoKind {
	ssaDebugInfo_Invalid,

	ssaDebugInfo_CompileUnit,
	ssaDebugInfo_File,
	ssaDebugInfo_Proc,
	ssaDebugInfo_AllProcs,

	ssaDebugInfo_Count,
};

struct ssaDebugInfo {
	ssaDebugInfoKind kind;
	i32 id;

	union {
		struct {
			AstFile *file;
			String   producer;
			ssaDebugInfo *all_procs;
		} CompileUnit;
		struct {
			AstFile *file;
			String filename;
			String directory;
		} File;
		struct {
			Entity *      entity;
			String        name;
			ssaDebugInfo *file;
			TokenPos      pos;
		} Proc;
		struct {
			Array<ssaDebugInfo *> procs;
		} AllProcs;
	};
};

struct ssaModule {
	CheckerInfo * info;
	BaseTypeSizes sizes;
	gbArena       arena;
	gbArena       tmp_arena;
	gbAllocator   allocator;
	gbAllocator   tmp_allocator;
	b32 generate_debug_info;

	u32 stmt_state_flags;

	// String source_filename;
	String layout;
	// String triple;

	Map<ssaValue *>     values;     // Key: Entity *
	Map<ssaValue *>     members;    // Key: String
	Map<String>         type_names; // Key: Type *
	Map<ssaDebugInfo *> debug_info; // Key: Unique pointer
	i32                 global_string_index;
	i32                 global_array_index; // For ConstantSlice

	Array<ssaValue *> procs; // NOTE(bill): Procedures to generate
};

// NOTE(bill): For more info, see https://en.wikipedia.org/wiki/Dominator_(graph_theory)
struct ssaDomNode {
	ssaBlock *idom; // Parent
	Array<ssaBlock *> children;
	i32 pre, post; // Ordering in tree
};


struct ssaBlock {
	i32           index;
	String        label;
	ssaProcedure *parent;
	AstNode *     node; // Can be NULL
	Scope *       scope;
	isize         scope_index;
	ssaDomNode    dom;
	i32           gaps;

	Array<ssaValue *> instrs;
	Array<ssaValue *> locals;

	Array<ssaBlock *> preds;
	Array<ssaBlock *> succs;
};

struct ssaTargetList {
	ssaTargetList *prev;
	ssaBlock *     break_;
	ssaBlock *     continue_;
	ssaBlock *     fallthrough_;
};

enum ssaDeferExitKind {
	ssaDeferExit_Default,
	ssaDeferExit_Return,
	ssaDeferExit_Branch,
};
enum ssaDeferKind {
	ssaDefer_Node,
	ssaDefer_Instr,
};

struct ssaDefer {
	ssaDeferKind kind;
	isize scope_index;
	ssaBlock *block;
	union {
		AstNode *stmt;
		// NOTE(bill): `instr` will be copied every time to create a new one
		ssaValue *instr;
	};
};

struct ssaProcedure {
	ssaProcedure *parent;
	Array<ssaProcedure *> children;

	Entity *      entity;
	ssaModule *   module;
	String        name;
	Type *        type;
	AstNode *     type_expr;
	AstNode *     body;
	u64           tags;

	isize               scope_index;
	Array<ssaDefer>     defer_stmts;
	Array<ssaBlock *>   blocks;
	ssaBlock *          decl_block;
	ssaBlock *          entry_block;
	ssaBlock *          curr_block;
	ssaTargetList *     target_list;
	Array<ssaValue *>   referrers;
};

#define SSA_STARTUP_RUNTIME_PROC_NAME  "__$startup_runtime"
#define SSA_TYPE_INFO_DATA_NAME        "__$type_info_data"
#define SSA_TYPE_INFO_DATA_MEMBER_NAME "__$type_info_data_member"


#define SSA_INSTR_KINDS \
	SSA_INSTR_KIND(Invalid), \
	SSA_INSTR_KIND(Comment), \
	SSA_INSTR_KIND(Local), \
	SSA_INSTR_KIND(ZeroInit), \
	SSA_INSTR_KIND(Store), \
	SSA_INSTR_KIND(Load), \
	SSA_INSTR_KIND(GetElementPtr), \
	SSA_INSTR_KIND(ExtractValue), \
	SSA_INSTR_KIND(InsertValue), \
	SSA_INSTR_KIND(Conv), \
	SSA_INSTR_KIND(Br), \
	SSA_INSTR_KIND(Ret), \
	SSA_INSTR_KIND(Select), \
	SSA_INSTR_KIND(Phi), \
	SSA_INSTR_KIND(Unreachable), \
	SSA_INSTR_KIND(BinaryOp), \
	SSA_INSTR_KIND(Call), \
	SSA_INSTR_KIND(NoOp), \
	SSA_INSTR_KIND(ExtractElement), \
	SSA_INSTR_KIND(InsertElement), \
	SSA_INSTR_KIND(ShuffleVector), \
	SSA_INSTR_KIND(StartupRuntime), \
	SSA_INSTR_KIND(Count),

enum ssaInstrKind {
#define SSA_INSTR_KIND(x) GB_JOIN2(ssaInstr_, x)
	SSA_INSTR_KINDS
#undef SSA_INSTR_KIND
};

String const ssa_instr_strings[] = {
#define SSA_INSTR_KIND(x) {cast(u8 *)#x, gb_size_of(#x)-1}
	SSA_INSTR_KINDS
#undef SSA_INSTR_KIND
};

#define SSA_CONV_KINDS \
	SSA_CONV_KIND(Invalid), \
	SSA_CONV_KIND(trunc), \
	SSA_CONV_KIND(zext), \
	SSA_CONV_KIND(fptrunc), \
	SSA_CONV_KIND(fpext), \
	SSA_CONV_KIND(fptoui), \
	SSA_CONV_KIND(fptosi), \
	SSA_CONV_KIND(uitofp), \
	SSA_CONV_KIND(sitofp), \
	SSA_CONV_KIND(ptrtoint), \
	SSA_CONV_KIND(inttoptr), \
	SSA_CONV_KIND(bitcast), \
	SSA_CONV_KIND(Count)

enum ssaConvKind {
#define SSA_CONV_KIND(x) GB_JOIN2(ssaConv_, x)
	SSA_CONV_KINDS
#undef SSA_CONV_KIND
};

String const ssa_conv_strings[] = {
#define SSA_CONV_KIND(x) {cast(u8 *)#x, gb_size_of(#x)-1}
	SSA_CONV_KINDS
#undef SSA_CONV_KIND
};

struct ssaInstr {
	ssaInstrKind kind;

	ssaBlock *parent;
	Type *type;

	union {
		struct {
			String text;
		} Comment;
		struct {
			Entity *          entity;
			Type *            type;
			b32               zero_initialized;
			Array<ssaValue *> referrers;
		} Local;
		struct {
			ssaValue *address;
		} ZeroInit;
		struct {
			ssaValue *address;
			ssaValue *value;
		} Store;
		struct {
			Type *type;
			ssaValue *address;
		} Load;
		struct {
			ssaValue *address;
			Type *    result_type;
			Type *    elem_type;
			ssaValue *indices[2];
			isize     index_count;
			b32       inbounds;
		} GetElementPtr;
		struct {
			ssaValue *address;
			Type *    result_type;
			Type *    elem_type;
			i32       index;
		} ExtractValue;
		struct {
			ssaValue *value;
			ssaValue *elem;
			i32       index;
		} InsertValue;
		struct {
			ssaConvKind kind;
			ssaValue *value;
			Type *from, *to;
		} Conv;
		struct {
			ssaValue *cond;
			ssaBlock *true_block;
			ssaBlock *false_block;
		} Br;
		struct { ssaValue *value; } Ret;
		struct {} Unreachable;
		struct {
			ssaValue *cond;
			ssaValue *true_value;
			ssaValue *false_value;
		} Select;
		struct {
			Array<ssaValue *> edges;
			Type *type;
		} Phi;
		struct {
			Type *type;
			TokenKind op;
			ssaValue *left, *right;
		} BinaryOp;
		struct {
			Type *type; // return type
			ssaValue *value;
			ssaValue **args;
			isize arg_count;
		} Call;
		struct {
			ssaValue *vector;
			ssaValue *index;
		} ExtractElement;
		struct {
			ssaValue *vector;
			ssaValue *elem;
			ssaValue *index;
		} InsertElement;
		struct {
			ssaValue *vector;
			i32 *indices;
			isize index_count;
			Type *type;
		} ShuffleVector;

		struct {} StartupRuntime;
	};
};


enum ssaValueKind {
	ssaValue_Invalid,

	ssaValue_Constant,
	ssaValue_ConstantSlice,
	ssaValue_Nil,
	ssaValue_TypeName,
	ssaValue_Global,
	ssaValue_Param,

	ssaValue_Proc,
	ssaValue_Block,
	ssaValue_Instr,

	ssaValue_Count,
};

struct ssaValue {
	ssaValueKind kind;
	i32 index;
	union {
		struct {
			Type *     type;
			ExactValue value;
		} Constant;
		struct {
			Type *type;
			ssaValue *backing_array;
			i64 count;
		} ConstantSlice;
		struct {
			Type *type;
		} Nil;
		struct {
			String name;
			Type * type;
		} TypeName;
		struct {
			b32 is_constant;
			b32 is_private;
			b32 is_thread_local;
			Entity *  entity;
			Type *    type;
			ssaValue *value;
			Array<ssaValue *> referrers;
		} Global;
		struct {
			ssaProcedure *parent;
			Entity *entity;
			Type *  type;
			Array<ssaValue *> referrers;
		} Param;
		ssaProcedure Proc;
		ssaBlock     Block;
		ssaInstr     Instr;
	};
};

gb_global ssaValue *v_zero    = NULL;
gb_global ssaValue *v_one     = NULL;
gb_global ssaValue *v_zero32  = NULL;
gb_global ssaValue *v_one32   = NULL;
gb_global ssaValue *v_two32   = NULL;
gb_global ssaValue *v_false   = NULL;
gb_global ssaValue *v_true    = NULL;

struct ssaAddr {
	ssaValue *addr;
	AstNode *expr; // NOTE(bill): Just for testing - probably remove later

	b32 is_vector;
	ssaValue *index;
};

ssaAddr ssa_make_addr(ssaValue *addr, AstNode *expr) {
	ssaAddr v = {addr, expr, false, NULL};
	return v;
}
ssaAddr ssa_make_addr_vector(ssaValue *addr, ssaValue *index, AstNode *expr) {
	ssaAddr v = {addr, expr, true, index};
	return v;
}


ssaValue *ssa_make_value_global(gbAllocator a, Entity *e, ssaValue *value);


void ssa_module_add_value(ssaModule *m, Entity *e, ssaValue *v) {
	map_set(&m->values, hash_pointer(e), v);
}

ssaDebugInfo *ssa_alloc_debug_info(gbAllocator a, ssaDebugInfoKind kind) {
	ssaDebugInfo *di = gb_alloc_item(a, ssaDebugInfo);
	di->kind = kind;
	return di;
}


ssaDefer ssa_add_defer_node(ssaProcedure *proc, isize scope_index, AstNode *stmt) {
	ssaDefer d = {ssaDefer_Node};
	d.scope_index = scope_index;
	d.block = proc->curr_block;
	d.stmt = stmt;
	array_add(&proc->defer_stmts, d);
	return d;
}


ssaDefer ssa_add_defer_instr(ssaProcedure *proc, isize scope_index, ssaValue *instr) {
	ssaDefer d = {ssaDefer_Instr};
	d.scope_index = proc->scope_index;
	d.block = proc->curr_block;
	d.instr = instr; // NOTE(bill): It will make a copy everytime it is called
	array_add(&proc->defer_stmts, d);
	return d;
}

void ssa_init_module(ssaModule *m, Checker *c) {
	// TODO(bill): Determine a decent size for the arena
	isize token_count = c->parser->total_token_count;
	isize arena_size = 4 * token_count * gb_size_of(ssaValue);
	gb_arena_init_from_allocator(&m->arena, gb_heap_allocator(), arena_size);
	gb_arena_init_from_allocator(&m->tmp_arena, gb_heap_allocator(), arena_size);
	m->allocator     = gb_arena_allocator(&m->arena);
	m->tmp_allocator = gb_arena_allocator(&m->tmp_arena);
	m->info = &c->info;
	m->sizes = c->sizes;

	map_init(&m->values,     gb_heap_allocator());
	map_init(&m->members,    gb_heap_allocator());
	map_init(&m->debug_info, gb_heap_allocator());
	map_init(&m->type_names, gb_heap_allocator());
	array_init(&m->procs,  gb_heap_allocator());

	// Default states
	m->stmt_state_flags = 0;
	m->stmt_state_flags |= StmtStateFlag_bounds_check;

	{
		// Add type info data
		{
			String name = make_string(SSA_TYPE_INFO_DATA_NAME);
			isize count = c->info.type_info_map.entries.count;
			Entity *e = make_entity_variable(m->allocator, NULL, make_token_ident(name), make_type_array(m->allocator, t_type_info, count));
			ssaValue *g = ssa_make_value_global(m->allocator, e, NULL);
			g->Global.is_private  = true;
			ssa_module_add_value(m, e, g);
			map_set(&m->members, hash_string(name), g);
		}

		// Type info member buffer
		{
			// NOTE(bill): Removes need for heap allocation by making it global memory
			isize count = 0;

			for_array(entry_index, m->info->type_info_map.entries) {
				auto *entry = &m->info->type_info_map.entries[entry_index];
				Type *t = cast(Type *)cast(uintptr)entry->key.key;

				switch (t->kind) {
				case Type_Record:
					switch (t->Record.kind) {
					case TypeRecord_Struct:
					case TypeRecord_RawUnion:
						count += t->Record.field_count;
					}
					break;
				case Type_Tuple:
					count += t->Tuple.variable_count;
					break;
				}
			}

			String name = make_string(SSA_TYPE_INFO_DATA_MEMBER_NAME);
			Entity *e = make_entity_variable(m->allocator, NULL, make_token_ident(name),
			                                 make_type_array(m->allocator, t_type_info_member, count));
			ssaValue *g = ssa_make_value_global(m->allocator, e, NULL);
			ssa_module_add_value(m, e, g);
			map_set(&m->members, hash_string(name), g);
		}
	}

	{
		ssaDebugInfo *di = ssa_alloc_debug_info(m->allocator, ssaDebugInfo_CompileUnit);
		di->CompileUnit.file = m->info->files.entries[0].value; // Zeroth is the init file
		di->CompileUnit.producer = make_string("odin");

		map_set(&m->debug_info, hash_pointer(m), di);
	}
}

void ssa_destroy_module(ssaModule *m) {
	map_destroy(&m->values);
	map_destroy(&m->members);
	map_destroy(&m->type_names);
	map_destroy(&m->debug_info);
	array_free(&m->procs);
	gb_arena_free(&m->arena);
}


Type *ssa_type(ssaValue *value);
Type *ssa_instr_type(ssaInstr *instr) {
	switch (instr->kind) {
	case ssaInstr_Local:
		return instr->Local.type;
	case ssaInstr_Load:
		return instr->Load.type;
	case ssaInstr_GetElementPtr:
		return instr->GetElementPtr.result_type;
	case ssaInstr_Phi:
		return instr->Phi.type;
	case ssaInstr_ExtractValue:
		return instr->ExtractValue.result_type;
	case ssaInstr_InsertValue:
		return ssa_type(instr->InsertValue.value);
	case ssaInstr_BinaryOp:
		return instr->BinaryOp.type;
	case ssaInstr_Conv:
		return instr->Conv.to;
	case ssaInstr_Select:
		return ssa_type(instr->Select.true_value);
	case ssaInstr_Call: {
		Type *pt = base_type(instr->Call.type);
		if (pt != NULL) {
			if (pt->kind == Type_Tuple && pt->Tuple.variable_count == 1) {
				return pt->Tuple.variables[0]->type;
			}
			return pt;
		}
		return NULL;
	} break;
	case ssaInstr_ExtractElement: {
		Type *vt = ssa_type(instr->ExtractElement.vector);
		Type *bt = base_vector_type(vt);
		GB_ASSERT(!is_type_vector(bt));
		return bt;
	} break;
	case ssaInstr_InsertElement:
		return ssa_type(instr->InsertElement.vector);
	case ssaInstr_ShuffleVector:
		return instr->ShuffleVector.type;
	}
	return NULL;
}

Type *ssa_type(ssaValue *value) {
	switch (value->kind) {
	case ssaValue_Constant:
		return value->Constant.type;
	case ssaValue_ConstantSlice:
		return value->ConstantSlice.type;
	case ssaValue_Nil:
		return value->Nil.type;
	case ssaValue_TypeName:
		return value->TypeName.type;
	case ssaValue_Global:
		return value->Global.type;
	case ssaValue_Param:
		return value->Param.type;
	case ssaValue_Proc:
		return value->Proc.type;
	case ssaValue_Instr:
		return ssa_instr_type(&value->Instr);
	}
	return NULL;
}

void ssa_set_instr_parent(ssaValue *instr, ssaBlock *parent) {
	if (instr->kind == ssaValue_Instr) {
		instr->Instr.parent = parent;
	}
}

Array<ssaValue *> *ssa_value_referrers(ssaValue *v) {
	switch (v->kind) {
	case ssaValue_Global:
		return &v->Global.referrers;
	case ssaValue_Param:
		return &v->Param.referrers;
	case ssaValue_Proc: {
		if (v->Proc.parent != NULL) {
			return &v->Proc.referrers;
		}
		return NULL;
	}
	case ssaValue_Instr: {
		ssaInstr *i = &v->Instr;
		switch (i->kind) {
		case ssaInstr_Local:
			return &i->Local.referrers;
		}
	} break;
	}

	return NULL;
}


void ssa_add_operands(Array<ssaValue *> *ops, ssaInstr *i) {
	switch (i->kind) {
	case ssaInstr_Comment:
		break;
	case ssaInstr_Local:
		break;
	case ssaInstr_ZeroInit:
		array_add(ops, i->ZeroInit.address);
		break;
	case ssaInstr_Store:
		array_add(ops, i->Store.address);
		array_add(ops, i->Store.value);
		break;
	case ssaInstr_Load:
		array_add(ops, i->Load.address);
		break;
	case ssaInstr_GetElementPtr:
		array_add(ops, i->GetElementPtr.address);
		for (isize index = 0; index < i->GetElementPtr.index_count; index++) {
			array_add(ops, i->GetElementPtr.indices[index]);
		}
		break;
	case ssaInstr_ExtractValue:
		array_add(ops, i->ExtractValue.address);
		break;
	case ssaInstr_InsertValue:
		array_add(ops, i->InsertValue.value);
		break;
	case ssaInstr_Conv:
		array_add(ops, i->Conv.value);
		break;
	case ssaInstr_Br:
		if (i->Br.cond) {
			array_add(ops, i->Br.cond);
		}
		break;
	case ssaInstr_Ret:
		if (i->Ret.value) {
			array_add(ops, i->Ret.value);
		}
		break;
	case ssaInstr_Select:
		array_add(ops, i->Select.cond);
		break;
	case ssaInstr_Phi:
		for_array(j, i->Phi.edges) {
			array_add(ops, i->Phi.edges[j]);
		}
		break;
	case ssaInstr_Unreachable: break;
	case ssaInstr_BinaryOp:
		array_add(ops, i->BinaryOp.left);
		array_add(ops, i->BinaryOp.right);
		break;
	case ssaInstr_Call:
		array_add(ops, i->Call.value);
		for (isize j = 0; j < i->Call.arg_count; j++) {
			array_add(ops, i->Call.args[j]);
		}
		break;
	case ssaInstr_NoOp: break;
	case ssaInstr_ExtractElement:
		array_add(ops, i->ExtractElement.vector);
		array_add(ops, i->ExtractElement.index);
		break;
	case ssaInstr_InsertElement:
		array_add(ops, i->InsertElement.vector);
		array_add(ops, i->InsertElement.elem);
		array_add(ops, i->InsertElement.index);
		break;
	case ssaInstr_ShuffleVector:
		array_add(ops, i->ShuffleVector.vector);
		break;
	case ssaInstr_StartupRuntime:
		break;
	}
}





ssaDebugInfo *ssa_add_debug_info_file(ssaProcedure *proc, AstFile *file) {
	if (!proc->module->generate_debug_info) {
		return NULL;
	}

	GB_ASSERT(file != NULL);
	ssaDebugInfo *di = ssa_alloc_debug_info(proc->module->allocator, ssaDebugInfo_File);
	di->File.file = file;

	String filename = file->tokenizer.fullpath;
	String directory = filename;
	isize slash_index = 0;
	for (isize i = filename.len-1; i >= 0; i--) {
		if (filename.text[i] == '\\' ||
		    filename.text[i] == '/') {
			break;
		}
		slash_index = i;
	}
	directory.len = slash_index-1;
	filename.text = filename.text + slash_index;
	filename.len -= slash_index;


	di->File.filename = filename;
	di->File.directory = directory;

	map_set(&proc->module->debug_info, hash_pointer(file), di);
	return di;
}


ssaDebugInfo *ssa_add_debug_info_proc(ssaProcedure *proc, Entity *entity, String name, ssaDebugInfo *file) {
	if (!proc->module->generate_debug_info) {
		return NULL;
	}

	GB_ASSERT(entity != NULL);
	ssaDebugInfo *di = ssa_alloc_debug_info(proc->module->allocator, ssaDebugInfo_Proc);
	di->Proc.entity = entity;
	di->Proc.name = name;
	di->Proc.file = file;
	di->Proc.pos = entity->token.pos;

	map_set(&proc->module->debug_info, hash_pointer(entity), di);
	return di;
}




ssaValue *ssa_build_expr(ssaProcedure *proc, AstNode *expr);
ssaValue *ssa_build_single_expr(ssaProcedure *proc, AstNode *expr, TypeAndValue *tv);
ssaAddr   ssa_build_addr(ssaProcedure *proc, AstNode *expr);
ssaValue *ssa_emit_conv(ssaProcedure *proc, ssaValue *value, Type *a_type, b32 is_argument = false);
ssaValue *ssa_emit_transmute(ssaProcedure *proc, ssaValue *value, Type *a_type);
void      ssa_build_proc(ssaValue *value, ssaProcedure *parent);




ssaValue *ssa_alloc_value(gbAllocator a, ssaValueKind kind) {
	ssaValue *v = gb_alloc_item(a, ssaValue);
	v->kind = kind;
	return v;
}

ssaValue *ssa_alloc_instr(ssaProcedure *proc, ssaInstrKind kind) {
	ssaValue *v = ssa_alloc_value(proc->module->allocator, ssaValue_Instr);
	v->Instr.kind = kind;
	return v;
}

ssaValue *ssa_make_value_type_name(gbAllocator a, String name, Type *type) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_TypeName);
	v->TypeName.name = name;
	v->TypeName.type = type;
	return v;
}

ssaValue *ssa_make_value_global(gbAllocator a, Entity *e, ssaValue *value) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_Global);
	v->Global.entity = e;
	v->Global.type = make_type_pointer(a, e->type);
	v->Global.value = value;
	array_init(&v->Global.referrers, gb_heap_allocator()); // TODO(bill): Replace heap allocator here
	return v;
}
ssaValue *ssa_make_value_param(gbAllocator a, ssaProcedure *parent, Entity *e) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_Param);
	v->Param.parent = parent;
	v->Param.entity = e;
	v->Param.type   = e->type;
	array_init(&v->Param.referrers, gb_heap_allocator()); // TODO(bill): Replace heap allocator here
	return v;
}
ssaValue *ssa_make_value_nil(gbAllocator a, Type *type) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_Nil);
	v->Nil.type = type;
	return v;
}



ssaValue *ssa_make_instr_local(ssaProcedure *p, Entity *e, b32 zero_initialized) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Local);
	ssaInstr *i = &v->Instr;
	i->Local.entity = e;
	i->Local.type = make_type_pointer(p->module->allocator, e->type);
	i->Local.zero_initialized = zero_initialized;
	array_init(&i->Local.referrers, gb_heap_allocator()); // TODO(bill): Replace heap allocator here
	ssa_module_add_value(p->module, e, v);
	return v;
}


ssaValue *ssa_make_instr_store(ssaProcedure *p, ssaValue *address, ssaValue *value) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Store);
	ssaInstr *i = &v->Instr;
	i->Store.address = address;
	i->Store.value = value;
	return v;
}

ssaValue *ssa_make_instr_zero_init(ssaProcedure *p, ssaValue *address) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_ZeroInit);
	ssaInstr *i = &v->Instr;
	i->ZeroInit.address = address;
	return v;
}

ssaValue *ssa_make_instr_load(ssaProcedure *p, ssaValue *address) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Load);
	ssaInstr *i = &v->Instr;
	i->Load.address = address;
	i->Load.type = type_deref(ssa_type(address));
	return v;
}

ssaValue *ssa_make_instr_get_element_ptr(ssaProcedure *p, ssaValue *address,
                                         ssaValue *index0, ssaValue *index1, isize index_count,
                                         b32 inbounds) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_GetElementPtr);
	ssaInstr *i = &v->Instr;
	i->GetElementPtr.address = address;
	i->GetElementPtr.indices[0]   = index0;
	i->GetElementPtr.indices[1]   = index1;
	i->GetElementPtr.index_count  = index_count;
	i->GetElementPtr.elem_type    = ssa_type(address);
	i->GetElementPtr.inbounds     = inbounds;
	GB_ASSERT_MSG(is_type_pointer(ssa_type(address)),
	              "%s", type_to_string(ssa_type(address)));
	return v;
}

ssaValue *ssa_make_instr_extract_value(ssaProcedure *p, ssaValue *address, i32 index, Type *result_type) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_ExtractValue);
	ssaInstr *i = &v->Instr;
	i->ExtractValue.address = address;
	i->ExtractValue.index = index;
	i->ExtractValue.result_type = result_type;
	Type *et = ssa_type(address);
	i->ExtractValue.elem_type = et;
	// GB_ASSERT(et->kind == Type_Struct || et->kind == Type_Array || et->kind == Type_Tuple);
	return v;
}
ssaValue *ssa_make_instr_insert_value(ssaProcedure *p, ssaValue *value, ssaValue *elem, i32 index) {
	Type *t = ssa_type(value);
	GB_ASSERT(is_type_array(t) || is_type_struct(t));
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_InsertValue);
	v->Instr.InsertValue.value = value;
	v->Instr.InsertValue.elem   = elem;
	v->Instr.InsertValue.index  = index;
	return v;
}


ssaValue *ssa_make_instr_binary_op(ssaProcedure *p, TokenKind op, ssaValue *left, ssaValue *right, Type *type) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_BinaryOp);
	ssaInstr *i = &v->Instr;
	i->BinaryOp.op = op;
	i->BinaryOp.left = left;
	i->BinaryOp.right = right;
	i->BinaryOp.type = type;
	return v;
}

ssaValue *ssa_make_instr_br(ssaProcedure *p, ssaValue *cond, ssaBlock *true_block, ssaBlock *false_block) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Br);
	ssaInstr *i = &v->Instr;
	i->Br.cond = cond;
	i->Br.true_block = true_block;
	i->Br.false_block = false_block;
	return v;
}

ssaValue *ssa_make_instr_phi(ssaProcedure *p, Array<ssaValue *> edges, Type *type) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Phi);
	ssaInstr *i = &v->Instr;
	i->Phi.edges = edges;
	i->Phi.type = type;
	return v;
}

ssaValue *ssa_make_instr_unreachable(ssaProcedure *p) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Unreachable);
	return v;
}

ssaValue *ssa_make_instr_ret(ssaProcedure *p, ssaValue *value) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Ret);
	v->Instr.Ret.value = value;
	return v;
}

ssaValue *ssa_make_instr_select(ssaProcedure *p, ssaValue *cond, ssaValue *t, ssaValue *f) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Select);
	v->Instr.Select.cond = cond;
	v->Instr.Select.true_value = t;
	v->Instr.Select.false_value = f;
	return v;
}

ssaValue *ssa_make_instr_call(ssaProcedure *p, ssaValue *value, ssaValue **args, isize arg_count, Type *result_type) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Call);
	v->Instr.Call.value = value;
	v->Instr.Call.args = args;
	v->Instr.Call.arg_count = arg_count;
	v->Instr.Call.type = result_type;
	return v;
}

ssaValue *ssa_make_instr_conv(ssaProcedure *p, ssaConvKind kind, ssaValue *value, Type *from, Type *to) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Conv);
	v->Instr.Conv.kind = kind;
	v->Instr.Conv.value = value;
	v->Instr.Conv.from = from;
	v->Instr.Conv.to = to;
	return v;
}

ssaValue *ssa_make_instr_extract_element(ssaProcedure *p, ssaValue *vector, ssaValue *index) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_ExtractElement);
	v->Instr.ExtractElement.vector = vector;
	v->Instr.ExtractElement.index = index;
	return v;
}

ssaValue *ssa_make_instr_insert_element(ssaProcedure *p, ssaValue *vector, ssaValue *elem, ssaValue *index) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_InsertElement);
	v->Instr.InsertElement.vector = vector;
	v->Instr.InsertElement.elem   = elem;
	v->Instr.InsertElement.index  = index;
	return v;
}

ssaValue *ssa_make_instr_shuffle_vector(ssaProcedure *p, ssaValue *vector, i32 *indices, isize index_count) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_ShuffleVector);
	v->Instr.ShuffleVector.vector      = vector;
	v->Instr.ShuffleVector.indices     = indices;
	v->Instr.ShuffleVector.index_count = index_count;

	Type *vt = base_type(ssa_type(vector));
	v->Instr.ShuffleVector.type = make_type_vector(p->module->allocator, vt->Vector.elem, index_count);

	return v;
}

ssaValue *ssa_make_instr_no_op(ssaProcedure *p) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_NoOp);
	return v;
}


ssaValue *ssa_make_instr_comment(ssaProcedure *p, String text) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Comment);
	v->Instr.Comment.text = text;
	return v;
}



ssaValue *ssa_make_value_constant(gbAllocator a, Type *type, ExactValue value) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_Constant);
	v->Constant.type  = type;
	v->Constant.value = value;
	return v;
}


ssaValue *ssa_make_value_constant_slice(gbAllocator a, Type *type, ssaValue *backing_array, i64 count) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_ConstantSlice);
	v->ConstantSlice.type = type;
	v->ConstantSlice.backing_array = backing_array;
	v->ConstantSlice.count = count;
	return v;
}

ssaValue *ssa_make_const_int(gbAllocator a, i64 i) {
	return ssa_make_value_constant(a, t_int, make_exact_value_integer(i));
}
ssaValue *ssa_make_const_i32(gbAllocator a, i64 i) {
	return ssa_make_value_constant(a, t_i32, make_exact_value_integer(i));
}
ssaValue *ssa_make_const_i64(gbAllocator a, i64 i) {
	return ssa_make_value_constant(a, t_i64, make_exact_value_integer(i));
}
ssaValue *ssa_make_const_bool(gbAllocator a, b32 b) {
	return ssa_make_value_constant(a, t_bool, make_exact_value_bool(b != 0));
}

ssaValue *ssa_add_module_constant(ssaModule *m, Type *type, ExactValue value) {
	if (is_type_slice(type)) {
		ast_node(cl, CompoundLit, value.value_compound);
		gbAllocator a = m->allocator;

		isize count = cl->elems.count;
		if (count > 0) {
			Type *elem = base_type(type)->Slice.elem;
			Type *t = make_type_array(a, elem, count);
			ssaValue *backing_array = ssa_add_module_constant(m, t, value);


			isize max_len = 7+8+1;
			u8 *str = cast(u8 *)gb_alloc_array(a, u8, max_len);
			isize len = gb_snprintf(cast(char *)str, max_len, "__csba$%x", m->global_array_index);
			m->global_array_index++;

			String name = make_string(str, len-1);

			Entity *e = make_entity_constant(a, NULL, make_token_ident(name), t, value);
			ssaValue *g = ssa_make_value_global(a, e, backing_array);
			ssa_module_add_value(m, e, g);
			map_set(&m->members, hash_string(name), g);

			return ssa_make_value_constant_slice(a, type, g, count);
		} else {
			return ssa_make_value_constant_slice(a, type, NULL, 0);
		}
	}

	return ssa_make_value_constant(m->allocator, type, value);
}


ssaValue *ssa_make_value_procedure(gbAllocator a, ssaModule *m, Entity *entity, Type *type, AstNode *type_expr, AstNode *body, String name) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_Proc);
	v->Proc.module = m;
	v->Proc.entity = entity;
	v->Proc.type   = type;
	v->Proc.type_expr = type_expr;
	v->Proc.body   = body;
	v->Proc.name   = name;
	array_init(&v->Proc.referrers, gb_heap_allocator(), 0); // TODO(bill): replace heap allocator
	return v;
}

ssaValue *ssa_make_value_block(ssaProcedure *proc, AstNode *node, Scope *scope, String label) {
	ssaValue *v = ssa_alloc_value(proc->module->allocator, ssaValue_Block);
	v->Block.label  = label;
	v->Block.node   = node;
	v->Block.scope  = scope;
	v->Block.parent = proc;

	array_init(&v->Block.instrs, gb_heap_allocator());
	array_init(&v->Block.locals, gb_heap_allocator());

	array_init(&v->Block.preds,  gb_heap_allocator());
	array_init(&v->Block.succs,  gb_heap_allocator());

	return v;
}

b32 ssa_is_blank_ident(AstNode *node) {
	if (node->kind == AstNode_Ident) {
		ast_node(i, Ident, node);
		return is_blank_ident(i->string);
	}
	return false;
}


ssaInstr *ssa_get_last_instr(ssaBlock *block) {
	if (block != NULL) {
		isize len = block->instrs.count;
		if (len > 0) {
			ssaValue *v = block->instrs[len-1];
			GB_ASSERT(v->kind == ssaValue_Instr);
			return &v->Instr;
		}
	}
	return NULL;

}

b32 ssa_is_instr_terminating(ssaInstr *i) {
	if (i != NULL) {
		switch (i->kind) {
		case ssaInstr_Ret:
		case ssaInstr_Unreachable:
			return true;
		}
	}

	return false;
}

ssaValue *ssa_emit(ssaProcedure *proc, ssaValue *instr) {
	GB_ASSERT(instr->kind == ssaValue_Instr);
	ssaBlock *b = proc->curr_block;
	instr->Instr.parent = b;
	if (b != NULL) {
		ssaInstr *i = ssa_get_last_instr(b);
		if (!ssa_is_instr_terminating(i)) {
			array_add(&b->instrs, instr);
		}
	}
	return instr;
}
ssaValue *ssa_emit_store(ssaProcedure *p, ssaValue *address, ssaValue *value) {
	return ssa_emit(p, ssa_make_instr_store(p, address, value));
}
ssaValue *ssa_emit_load(ssaProcedure *p, ssaValue *address) {
	return ssa_emit(p, ssa_make_instr_load(p, address));
}
ssaValue *ssa_emit_select(ssaProcedure *p, ssaValue *cond, ssaValue *t, ssaValue *f) {
	return ssa_emit(p, ssa_make_instr_select(p, cond, t, f));
}

ssaValue *ssa_emit_zero_init(ssaProcedure *p, ssaValue *address)  {
	return ssa_emit(p, ssa_make_instr_zero_init(p, address));
}

ssaValue *ssa_emit_comment(ssaProcedure *p, String text) {
	return ssa_emit(p, ssa_make_instr_comment(p, text));
}


ssaValue *ssa_add_local(ssaProcedure *proc, Entity *e, b32 zero_initialized = true) {
	ssaBlock *b = proc->decl_block; // all variables must be in the first block
	ssaValue *instr = ssa_make_instr_local(proc, e, zero_initialized);
	instr->Instr.parent = b;
	array_add(&b->instrs, instr);
	array_add(&b->locals, instr);

	// if (zero_initialized) {
		ssa_emit_zero_init(proc, instr);
	// }

	return instr;
}

ssaValue *ssa_add_local_for_identifier(ssaProcedure *proc, AstNode *name, b32 zero_initialized) {
	Entity **found = map_get(&proc->module->info->definitions, hash_pointer(name));
	if (found) {
		Entity *e = *found;
		ssa_emit_comment(proc, e->token.string);
		return ssa_add_local(proc, e, zero_initialized);
	}
	return NULL;
}

ssaValue *ssa_add_local_generated(ssaProcedure *proc, Type *type) {
	GB_ASSERT(type != NULL);

	Scope *scope = NULL;
	if (proc->curr_block) {
		scope = proc->curr_block->scope;
	}
	Entity *e = make_entity_variable(proc->module->allocator,
	                                 scope,
	                                 empty_token,
	                                 type);
	return ssa_add_local(proc, e, true);
}

ssaValue *ssa_add_param(ssaProcedure *proc, Entity *e) {
	ssaValue *v = ssa_make_value_param(proc->module->allocator, proc, e);
	ssaValue *l = ssa_add_local(proc, e);
	ssa_emit_store(proc, l, v);
	return v;
}


ssaValue *ssa_emit_call(ssaProcedure *p, ssaValue *value, ssaValue **args, isize arg_count) {
	Type *pt = base_type(ssa_type(value));
	GB_ASSERT(pt->kind == Type_Proc);
	Type *results = pt->Proc.results;
	return ssa_emit(p, ssa_make_instr_call(p, value, args, arg_count, results));
}

ssaValue *ssa_emit_global_call(ssaProcedure *proc, char *name_, ssaValue **args, isize arg_count) {
	String name = make_string(name_);
	ssaValue **found = map_get(&proc->module->members, hash_string(name));
	GB_ASSERT_MSG(found != NULL, "%.*s", LIT(name));
	ssaValue *gp = *found;
	return ssa_emit_call(proc, gp, args, arg_count);
}


Type *ssa_addr_type(ssaAddr lval) {
	if (lval.addr != NULL) {
		Type *t = ssa_type(lval.addr);
		GB_ASSERT(is_type_pointer(t));
		return type_deref(t);
	}
	return NULL;
}

ssaBlock *ssa_add_block(ssaProcedure *proc, AstNode *node, char *label) {
	Scope *scope = NULL;
	if (node != NULL) {
		Scope **found = map_get(&proc->module->info->scopes, hash_pointer(node));
		if (found) {
			scope = *found;
		} else {
			GB_PANIC("Block scope not found for %.*s", LIT(ast_node_strings[node->kind]));
		}
	}

	ssaValue *value = ssa_make_value_block(proc, node, scope, make_string(label));
	ssaBlock *block = &value->Block;
	array_add(&proc->blocks, block);
	return block;
}

void ssa_block_replace_pred(ssaBlock *b, ssaBlock *from, ssaBlock *to) {
	for_array(i, b->preds) {
		ssaBlock *pred = b->preds[i];
		if (pred == from) {
			b->preds[i] = to;
		}
	}
}

void ssa_block_replace_succ(ssaBlock *b, ssaBlock *from, ssaBlock *to) {
	for_array(i, b->succs) {
		ssaBlock *succ = b->succs[i];
		if (succ == from) {
			b->succs[i] = to;
		}
	}
}

b32 ssa_block_has_phi(ssaBlock *b) {
	return b->instrs[0]->Instr.kind == ssaInstr_Phi;
}








void ssa_build_stmt(ssaProcedure *proc, AstNode *s);
void ssa_emit_no_op(ssaProcedure *proc);
void ssa_emit_jump(ssaProcedure *proc, ssaBlock *block);

void ssa_build_defer_stmt(ssaProcedure *proc, ssaDefer d) {
	ssaBlock *b = ssa_add_block(proc, NULL, "defer");
	// NOTE(bill): The prev block may defer injection before it's terminator
	ssaInstr *last_instr = ssa_get_last_instr(proc->curr_block);
	if (last_instr == NULL || !ssa_is_instr_terminating(last_instr)) {
		ssa_emit_jump(proc, b);
	}
	proc->curr_block = b;
	ssa_emit_comment(proc, make_string("defer"));
	if (d.kind == ssaDefer_Node) {
		ssa_build_stmt(proc, d.stmt);
	} else if (d.kind == ssaDefer_Instr) {
		// NOTE(bill): Need to make a new copy
		ssaValue *instr = cast(ssaValue *)gb_alloc_copy(proc->module->allocator, d.instr, gb_size_of(ssaValue));
		ssa_emit(proc, instr);
	}
}

void ssa_emit_defer_stmts(ssaProcedure *proc, ssaDeferExitKind kind, ssaBlock *block) {
	isize count = proc->defer_stmts.count;
	isize i = count;
	while (i --> 0) {
		ssaDefer d = proc->defer_stmts[i];
		if (kind == ssaDeferExit_Default) {
			if (proc->scope_index == d.scope_index &&
			    d.scope_index > 1) {
				ssa_build_defer_stmt(proc, d);
				array_pop(&proc->defer_stmts);
				continue;
			} else {
				break;
			}
		} else if (kind == ssaDeferExit_Return) {
			ssa_build_defer_stmt(proc, d);
		} else if (kind == ssaDeferExit_Branch) {
			GB_ASSERT(block != NULL);
			isize lower_limit = block->scope_index+1;
			if (lower_limit < d.scope_index) {
				ssa_build_defer_stmt(proc, d);
			}
		}
	}
}

void ssa_open_scope(ssaProcedure *proc) {
	proc->scope_index++;
}


void ssa_close_scope(ssaProcedure *proc, ssaDeferExitKind kind, ssaBlock *block) {
	ssa_emit_defer_stmts(proc, kind, block);
	GB_ASSERT(proc->scope_index > 0);
	proc->scope_index--;
}

void ssa_add_edge(ssaBlock *from, ssaBlock *to) {
	array_add(&from->succs, to);
	array_add(&to->preds, from);
}



void ssa_emit_unreachable(ssaProcedure *proc) {
	ssa_emit(proc, ssa_make_instr_unreachable(proc));
}

void ssa_emit_ret(ssaProcedure *proc, ssaValue *v) {
	ssa_emit_defer_stmts(proc, ssaDeferExit_Return, NULL);
	ssa_emit(proc, ssa_make_instr_ret(proc, v));
}

void ssa_emit_jump(ssaProcedure *proc, ssaBlock *target_block) {
	ssaBlock *b = proc->curr_block;
	if (b != NULL) {
		ssa_emit(proc, ssa_make_instr_br(proc, NULL, target_block, NULL));
		ssa_add_edge(b, target_block);
		proc->curr_block = NULL;
	}
}

void ssa_emit_if(ssaProcedure *proc, ssaValue *cond, ssaBlock *true_block, ssaBlock *false_block) {
	ssaBlock *b = proc->curr_block;
	if (b != NULL) {
		ssaValue *br = ssa_make_instr_br(proc, cond, true_block, false_block);
		ssa_emit(proc, br);
		ssa_add_edge(b, true_block);
		ssa_add_edge(b, false_block);
		proc->curr_block = NULL;
	}
}

void ssa_emit_no_op(ssaProcedure *proc) {
	ssa_emit(proc, ssa_make_instr_no_op(proc));
}



ssaValue *ssa_lvalue_store(ssaProcedure *proc, ssaAddr lval, ssaValue *value) {
	if (lval.addr != NULL) {
		if (lval.is_vector) {
			ssaValue *v = ssa_emit_load(proc, lval.addr);
			Type *elem_type = base_type(ssa_type(v))->Vector.elem;
			ssaValue *elem = ssa_emit_conv(proc, value, elem_type);
			ssaValue *out = ssa_emit(proc, ssa_make_instr_insert_element(proc, v, elem, lval.index));
			return ssa_emit_store(proc, lval.addr, out);
		} else {
			ssaValue *v = ssa_emit_conv(proc, value, ssa_addr_type(lval));
			return ssa_emit_store(proc, lval.addr, v);
		}
	}
	return NULL;
}
ssaValue *ssa_lvalue_load(ssaProcedure *proc, ssaAddr lval) {
	if (lval.addr != NULL) {
		if (lval.is_vector) {
			ssaValue *v = ssa_emit_load(proc, lval.addr);
			return ssa_emit(proc, ssa_make_instr_extract_element(proc, v, lval.index));
		}
		// NOTE(bill): Imported procedures don't require a load as they are pointers
		Type *t = base_type(ssa_type(lval.addr));
		if (t->kind == Type_Proc) {
			return lval.addr;
		}
		return ssa_emit_load(proc, lval.addr);
	}
	GB_PANIC("Illegal lvalue load");
	return NULL;
}




void ssa_begin_procedure_body(ssaProcedure *proc) {
	array_init(&proc->blocks,      gb_heap_allocator());
	array_init(&proc->defer_stmts, gb_heap_allocator());
	array_init(&proc->children,    gb_heap_allocator());

	proc->decl_block  = ssa_add_block(proc, proc->type_expr, "decls");
	proc->entry_block = ssa_add_block(proc, proc->type_expr, "entry");
	proc->curr_block  = proc->entry_block;

	if (proc->type->Proc.params != NULL) {
		auto *params = &proc->type->Proc.params->Tuple;
		for (isize i = 0; i < params->variable_count; i++) {
			Entity *e = params->variables[i];
			ssa_add_param(proc, e);
		}
	}
}


void ssa_optimize_blocks(ssaProcedure *proc);
void ssa_build_referrers(ssaProcedure *proc);
void ssa_build_dom_tree (ssaProcedure *proc);
void ssa_opt_mem2reg    (ssaProcedure *proc);

void ssa_end_procedure_body(ssaProcedure *proc) {
	if (proc->type->Proc.result_count == 0) {
		ssa_emit_ret(proc, NULL);
	}

	if (proc->curr_block->instrs.count == 0) {
		ssa_emit_unreachable(proc);
	}

	proc->curr_block = proc->decl_block;
	ssa_emit_jump(proc, proc->entry_block);

	ssa_optimize_blocks(proc);
	ssa_build_referrers(proc);
	ssa_build_dom_tree(proc);

	// TODO(bill): mem2reg optimization
	// [ ] Local never loaded? Eliminate
	// [ ] Local never stored? Replace all loads with `Nil`
	// [ ] Local stored once?  Replace loads with dominating store
	// [ ] Convert to phi nodes
	ssa_opt_mem2reg(proc);


// Number registers
	i32 reg_index = 0;
	for_array(i, proc->blocks) {
		ssaBlock *b = proc->blocks[i];
		b->index = i;
		for_array(j, b->instrs) {
			ssaValue *value = b->instrs[j];
			GB_ASSERT(value->kind == ssaValue_Instr);
			ssaInstr *instr = &value->Instr;
			if (ssa_instr_type(instr) == NULL) { // NOTE(bill): Ignore non-returning instructions
				continue;
			}
			value->index = reg_index;
			reg_index++;
		}
	}
}

void ssa_push_target_list(ssaProcedure *proc, ssaBlock *break_, ssaBlock *continue_, ssaBlock *fallthrough_) {
	ssaTargetList *tl = gb_alloc_item(proc->module->allocator, ssaTargetList);
	tl->prev          = proc->target_list;
	tl->break_        = break_;
	tl->continue_     = continue_;
	tl->fallthrough_  = fallthrough_;
	proc->target_list = tl;
}

void ssa_pop_target_list(ssaProcedure *proc) {
	proc->target_list = proc->target_list->prev;
}


ssaValue *ssa_emit_ptr_offset(ssaProcedure *proc, ssaValue *ptr, ssaValue *offset) {
	ssaValue *gep = NULL;
	offset = ssa_emit_conv(proc, offset, t_int);
	gep = ssa_make_instr_get_element_ptr(proc, ptr, offset, NULL, 1, false);
	gep->Instr.GetElementPtr.result_type = ssa_type(ptr);
	return ssa_emit(proc, gep);
}

ssaValue *ssa_emit_arith(ssaProcedure *proc, TokenKind op, ssaValue *left, ssaValue *right, Type *type) {
	Type *t_left = ssa_type(left);
	Type *t_right = ssa_type(right);

	if (op == Token_Add) {
		if (is_type_pointer(t_left)) {
			ssaValue *ptr = ssa_emit_conv(proc, left, type);
			ssaValue *offset = right;
			return ssa_emit_ptr_offset(proc, ptr, offset);
		} else if (is_type_pointer(ssa_type(right))) {
			ssaValue *ptr = ssa_emit_conv(proc, right, type);
			ssaValue *offset = left;
			return ssa_emit_ptr_offset(proc, ptr, offset);
		}
	} else if (op == Token_Sub) {
		if (is_type_pointer(t_left) && is_type_integer(t_right)) {
			// ptr - int
			ssaValue *ptr = ssa_emit_conv(proc, left, type);
			ssaValue *offset = right;
			return ssa_emit_ptr_offset(proc, ptr, offset);
		} else if (is_type_pointer(t_left) && is_type_pointer(t_right)) {
			GB_ASSERT(is_type_integer(type));
			Type *ptr_type = t_left;
			ssaModule *m = proc->module;
			ssaValue *x = ssa_emit_conv(proc, left, type);
			ssaValue *y = ssa_emit_conv(proc, right, type);
			ssaValue *diff = ssa_emit_arith(proc, op, x, y, type);
			ssaValue *elem_size = ssa_make_const_int(m->allocator, type_size_of(m->sizes, m->allocator, ptr_type));
			return ssa_emit_arith(proc, Token_Quo, diff, elem_size, type);
		}
	}


	switch (op) {
	case Token_AndNot: {
		// NOTE(bill): x &~ y == x & (~y) == x & (y ~ -1)
		// NOTE(bill): "not" `x` == `x` "xor" `-1`
		ssaValue *neg = ssa_add_module_constant(proc->module, type, make_exact_value_integer(-1));
		op = Token_Xor;
		right = ssa_emit_arith(proc, op, right, neg, type);
		GB_ASSERT(right->Instr.kind == ssaInstr_BinaryOp);
		right->Instr.BinaryOp.type = type;
		op = Token_And;
	} /* fallthrough */
	case Token_Add:
	case Token_Sub:
	case Token_Mul:
	case Token_Quo:
	case Token_Mod:
	case Token_And:
	case Token_Or:
	case Token_Xor:
		left  = ssa_emit_conv(proc, left, type);
		right = ssa_emit_conv(proc, right, type);
		break;
	}

	return ssa_emit(proc, ssa_make_instr_binary_op(proc, op, left, right, type));
}

ssaValue *ssa_emit_comp(ssaProcedure *proc, TokenKind op_kind, ssaValue *left, ssaValue *right) {
	Type *a = base_type(ssa_type(left));
	Type *b = base_type(ssa_type(right));

	if (are_types_identical(a, b)) {
		// NOTE(bill): No need for a conversion
	} else if (left->kind == ssaValue_Constant || left->kind == ssaValue_Nil) {
		left = ssa_emit_conv(proc, left, ssa_type(right));
	} else if (right->kind == ssaValue_Constant || right->kind == ssaValue_Nil) {
		right = ssa_emit_conv(proc, right, ssa_type(left));
	}

	Type *result = t_bool;
	if (is_type_vector(a)) {
		result = make_type_vector(proc->module->allocator, t_bool, a->Vector.count);
	}
	return ssa_emit(proc, ssa_make_instr_binary_op(proc, op_kind, left, right, result));
}



ssaValue *ssa_emit_zero_gep(ssaProcedure *proc, ssaValue *s) {
	ssaValue *gep = NULL;
	// NOTE(bill): For some weird legacy reason in LLVM, structure elements must be accessed as an i32
	gep = ssa_make_instr_get_element_ptr(proc, s, NULL, NULL, 0, true);
	gep->Instr.GetElementPtr.result_type = ssa_type(s);
	return ssa_emit(proc, gep);
}

ssaValue *ssa_emit_struct_gep(ssaProcedure *proc, ssaValue *s, ssaValue *index, Type *result_type) {
	ssaValue *gep = NULL;
	// NOTE(bill): For some weird legacy reason in LLVM, structure elements must be accessed as an i32
	index = ssa_emit_conv(proc, index, t_i32);
	gep = ssa_make_instr_get_element_ptr(proc, s, v_zero, index, 2, true);
	gep->Instr.GetElementPtr.result_type = result_type;

	return ssa_emit(proc, gep);
}

ssaValue *ssa_emit_struct_gep(ssaProcedure *proc, ssaValue *s, i32 index, Type *result_type) {
	ssaValue *i = ssa_make_const_i32(proc->module->allocator, index);
	return ssa_emit_struct_gep(proc, s, i, result_type);
}


ssaValue *ssa_emit_struct_ev(ssaProcedure *proc, ssaValue *s, i32 index, Type *result_type) {
	// NOTE(bill): For some weird legacy reason in LLVM, structure elements must be accessed as an i32
	return ssa_emit(proc, ssa_make_instr_extract_value(proc, s, index, result_type));
}


ssaValue *ssa_emit_deep_field_gep(ssaProcedure *proc, Type *type, ssaValue *e, Selection sel) {
	GB_ASSERT(sel.index.count > 0);

	for_array(i, sel.index) {
		isize index = sel.index[i];
		if (is_type_pointer(type)) {
			type = type_deref(type);
			e = ssa_emit_load(proc, e);
			e = ssa_emit_ptr_offset(proc, e, v_zero);
		}
		type = base_type(type);


		if (is_type_raw_union(type)) {
			type = type->Record.fields[index]->type;
			e = ssa_emit_conv(proc, e, make_type_pointer(proc->module->allocator, type));
		} else if (type->kind == Type_Record) {
			type = type->Record.fields[index]->type;
			e = ssa_emit_struct_gep(proc, e, index, make_type_pointer(proc->module->allocator, type));
		} else if (type->kind == Type_Basic) {
			switch (type->Basic.kind) {
			case Basic_any: {
				if (index == 0) {
					type = t_type_info_ptr;
				} else if (index == 1) {
					type = t_rawptr;
				}
				e = ssa_emit_struct_gep(proc, e, index, make_type_pointer(proc->module->allocator, type));
			} break;

			case Basic_string:
				e = ssa_emit_struct_gep(proc, e, index, make_type_pointer(proc->module->allocator, sel.entity->type));
				break;

			default:
				GB_PANIC("un-gep-able type");
				break;
			}
		} else if (type->kind == Type_Slice) {
			e = ssa_emit_struct_gep(proc, e, index, make_type_pointer(proc->module->allocator, sel.entity->type));
		} else {
			GB_PANIC("un-gep-able type");
		}
	}

	return e;
}


ssaValue *ssa_emit_deep_field_ev(ssaProcedure *proc, Type *type, ssaValue *e, Selection sel) {
	GB_ASSERT(sel.index.count > 0);

	for_array(i, sel.index) {
		isize index = sel.index[i];
		if (is_type_pointer(type)) {
			type = type_deref(type);
			e = ssa_emit_load(proc, e);
			e = ssa_emit_ptr_offset(proc, e, v_zero);
		}
		type = base_type(type);


		if (is_type_raw_union(type)) {
			type = type->Record.fields[index]->type;
			e = ssa_emit_conv(proc, e, make_type_pointer(proc->module->allocator, type));
		} else if (type->kind == Type_Record) {
			type = type->Record.fields[index]->type;
			e = ssa_emit_struct_ev(proc, e, index, type);
		} else if (type->kind == Type_Basic) {
			switch (type->Basic.kind) {
			case Basic_any: {
				if (index == 0) {
					type = t_type_info_ptr;
				} else if (index == 1) {
					type = t_rawptr;
				}
				e = ssa_emit_struct_ev(proc, e, index, type);
			} break;

			case Basic_string:
				e = ssa_emit_struct_ev(proc, e, index, sel.entity->type);
				break;

			default:
				GB_PANIC("un-ev-able type");
				break;
			}
		} else if (type->kind == Type_Slice) {
			e = ssa_emit_struct_gep(proc, e, index, make_type_pointer(proc->module->allocator, sel.entity->type));
		} else {
			GB_PANIC("un-ev-able type");
		}
	}

	return e;
}



isize ssa_type_info_index(CheckerInfo *info, Type *type) {
	type = default_type(type);

	isize entry_index = -1;
	HashKey key = hash_pointer(type);
	auto *found_entry_index = map_get(&info->type_info_map, key);
	if (found_entry_index) {
		entry_index = *found_entry_index;
	}
	if (entry_index < 0) {
		// NOTE(bill): Do manual search
		// TODO(bill): This is O(n) and can be very slow
		for_array(i, info->type_info_map.entries){
			auto *e = &info->type_info_map.entries[i];
			Type *prev_type = cast(Type *)cast(uintptr)e->key.key;
			if (are_types_identical(prev_type, type)) {
				entry_index = e->value;
				// NOTE(bill): Add it to the search map
				map_set(&info->type_info_map, key, entry_index);
				break;
			}
		}
	}
	if (entry_index < 0) {
		compiler_error("Type_Info for `%s` could not be found", type_to_string(type));
	}
	return entry_index;
}

ssaValue *ssa_type_info(ssaProcedure *proc, Type *type) {
	ssaValue **found = map_get(&proc->module->members, hash_string(make_string(SSA_TYPE_INFO_DATA_NAME)));
	GB_ASSERT(found != NULL);
	ssaValue *type_info_data = *found;

	CheckerInfo *info = proc->module->info;
	isize entry_index = ssa_type_info_index(info, type);
	return ssa_emit_struct_gep(proc, type_info_data, entry_index, t_type_info_ptr);
}







ssaValue *ssa_array_elem(ssaProcedure *proc, ssaValue *array) {
	Type *t = type_deref(ssa_type(array));
	GB_ASSERT(t->kind == Type_Array);
	Type *result_type = make_type_pointer(proc->module->allocator, t->Array.elem);
	return ssa_emit_struct_gep(proc, array, v_zero32, result_type);
}
ssaValue *ssa_array_len(ssaProcedure *proc, ssaValue *array) {
	Type *t = ssa_type(array);
	GB_ASSERT(t->kind == Type_Array);
	return ssa_make_const_int(proc->module->allocator, t->Array.count);
}
ssaValue *ssa_array_cap(ssaProcedure *proc, ssaValue *array) {
	return ssa_array_len(proc, array);
}

ssaValue *ssa_slice_elem(ssaProcedure *proc, ssaValue *slice) {
	Type *t = ssa_type(slice);
	GB_ASSERT(t->kind == Type_Slice);

	Type *result_type = make_type_pointer(proc->module->allocator, t->Slice.elem);
	return ssa_emit_struct_ev(proc, slice, 0, result_type);
}
ssaValue *ssa_slice_len(ssaProcedure *proc, ssaValue *slice) {
	Type *t = ssa_type(slice);
	GB_ASSERT(t->kind == Type_Slice);
	return ssa_emit_struct_ev(proc, slice, 1, t_int);
}
ssaValue *ssa_slice_cap(ssaProcedure *proc, ssaValue *slice) {
	Type *t = ssa_type(slice);
	GB_ASSERT(t->kind == Type_Slice);
	return ssa_emit_struct_ev(proc, slice, 2, t_int);
}

ssaValue *ssa_string_elem(ssaProcedure *proc, ssaValue *string) {
	Type *t = ssa_type(string);
	GB_ASSERT(t->kind == Type_Basic && t->Basic.kind == Basic_string);
	Type *t_u8_ptr = make_type_pointer(proc->module->allocator, t_u8);
	return ssa_emit_struct_ev(proc, string, 0, t_u8_ptr);
}
ssaValue *ssa_string_len(ssaProcedure *proc, ssaValue *string) {
	Type *t = ssa_type(string);
	GB_ASSERT(t->kind == Type_Basic && t->Basic.kind == Basic_string);
	return ssa_emit_struct_ev(proc, string, 1, t_int);
}



ssaValue *ssa_add_local_slice(ssaProcedure *proc, Type *slice_type, ssaValue *base, ssaValue *low, ssaValue *high, ssaValue *max) {
	// TODO(bill): array bounds checking for slice creation
	// TODO(bill): check that low < high <= max
	gbAllocator a = proc->module->allocator;
	Type *bt = base_type(ssa_type(base));

	if (low == NULL) {
		low = v_zero;
	}
	if (high == NULL) {
		switch (bt->kind) {
		case Type_Array:   high = ssa_array_len(proc, base); break;
		case Type_Slice:   high = ssa_slice_len(proc, base); break;
		case Type_Pointer: high = v_one;                     break;
		}
	}
	if (max == NULL) {
		switch (bt->kind) {
		case Type_Array:   max = ssa_array_cap(proc, base); break;
		case Type_Slice:   max = ssa_slice_cap(proc, base); break;
		case Type_Pointer: max = high;                      break;
		}
	}
	GB_ASSERT(max != NULL);

	ssaValue *len = ssa_emit_arith(proc, Token_Sub, high, low, t_int);
	ssaValue *cap = ssa_emit_arith(proc, Token_Sub, max,  low, t_int);

	ssaValue *elem = NULL;
	switch (bt->kind) {
	case Type_Array:   elem = ssa_array_elem(proc, base); break;
	case Type_Slice:   elem = ssa_slice_elem(proc, base); break;
	case Type_Pointer: elem = ssa_emit_load(proc, base);  break;
	}

	elem = ssa_emit_ptr_offset(proc, elem, low);

	ssaValue *slice = ssa_add_local_generated(proc, slice_type);

	ssaValue *gep = NULL;
	gep = ssa_emit_struct_gep(proc, slice, v_zero32, ssa_type(elem));
	ssa_emit_store(proc, gep, elem);

	gep = ssa_emit_struct_gep(proc, slice, v_one32, t_int);
	ssa_emit_store(proc, gep, len);

	gep = ssa_emit_struct_gep(proc, slice, v_two32, t_int);
	ssa_emit_store(proc, gep, cap);

	return slice;
}


ssaValue *ssa_add_global_string_array(ssaModule *m, String string) {
	gbAllocator a = m->allocator;

	isize max_len = 6+8+1;
	u8 *str = cast(u8 *)gb_alloc_array(a, u8, max_len);
	isize len = gb_snprintf(cast(char *)str, max_len, "__str$%x", m->global_string_index);
	m->global_string_index++;

	String name = make_string(str, len-1);
	Token token = {Token_String};
	token.string = name;
	Type *type = make_type_array(a, t_u8, string.len);
	ExactValue ev = make_exact_value_string(string);
	Entity *entity = make_entity_constant(a, NULL, token, type, ev);
	ssaValue *g = ssa_make_value_global(a, entity, ssa_add_module_constant(m, type, ev));
	g->Global.is_private  = true;
	// g->Global.is_constant = true;

	ssa_module_add_value(m, entity, g);
	map_set(&m->members, hash_string(name), g);

	return g;
}

ssaValue *ssa_emit_string(ssaProcedure *proc, ssaValue *elem, ssaValue *len) {
	Type *t_u8_ptr = ssa_type(elem);
	GB_ASSERT(t_u8_ptr->kind == Type_Pointer);

	GB_ASSERT(is_type_u8(t_u8_ptr->Pointer.elem));

	ssaValue *str = ssa_add_local_generated(proc, t_string);
	ssaValue *str_elem = ssa_emit_struct_gep(proc, str, v_zero32, t_u8_ptr);
	ssaValue *str_len = ssa_emit_struct_gep(proc, str, v_one32, t_int);
	ssa_emit_store(proc, str_elem, elem);
	ssa_emit_store(proc, str_len, len);
	return ssa_emit_load(proc, str);
}


ssaValue *ssa_emit_global_string(ssaProcedure *proc, String str) {
	ssaValue *global_array = ssa_add_global_string_array(proc->module, str);
	ssaValue *elem = ssa_array_elem(proc, global_array);
	ssaValue *len =  ssa_make_const_int(proc->module->allocator, str.len);
	return ssa_emit_string(proc, elem, len);
}


String lookup_polymorphic_field(CheckerInfo *info, Type *dst, Type *src) {
	Type *prev_src = src;
	// Type *prev_dst = dst;
	src = base_type(type_deref(src));
	// dst = base_type(type_deref(dst));
	b32 src_is_ptr = src != prev_src;
	// b32 dst_is_ptr = dst != prev_dst;

	GB_ASSERT(is_type_struct(src));
	for (isize i = 0; i < src->Record.field_count; i++) {
		Entity *f = src->Record.fields[i];
		if (f->kind == Entity_Variable && f->Variable.anonymous) {
			if (are_types_identical(dst, f->type)) {
				return f->token.string;
			}
			if (src_is_ptr && is_type_pointer(dst)) {
				if (are_types_identical(type_deref(dst), f->type)) {
					return f->token.string;
				}
			}
			String name = lookup_polymorphic_field(info, dst, f->type);
			if (name.len > 0) {
				return name;
			}
		}
	}
	return make_string("");
}


ssaValue *ssa_emit_conv(ssaProcedure *proc, ssaValue *value, Type *t, b32 is_argument) {
	Type *src_type = ssa_type(value);
	if (are_types_identical(t, src_type)) {
		return value;
	}


	Type *src = get_enum_base_type(base_type(src_type));
	Type *dst = get_enum_base_type(base_type(t));

	if (value->kind == ssaValue_Constant) {
		if (is_type_any(dst)) {
			ssaValue *default_value = ssa_add_local_generated(proc, default_type(src_type));
			ssa_emit_store(proc, default_value, value);
			return ssa_emit_conv(proc, ssa_emit_load(proc, default_value), t_any, is_argument);
		} else if (dst->kind == Type_Basic) {
			ExactValue ev = value->Constant.value;
			if (is_type_float(dst)) {
				ev = exact_value_to_float(ev);
			} else if (is_type_string(dst)) {
				// Handled elsewhere
			} else if (is_type_integer(dst)) {
				ev = exact_value_to_integer(ev);
			} else if (is_type_pointer(dst)) {
				// IMPORTANT NOTE(bill): LLVM doesn't support pointer constants expect `null`
				ssaValue *i = ssa_add_module_constant(proc->module, t_uint, ev);
				return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_inttoptr, i, t_uint, dst));
			}
			return ssa_add_module_constant(proc->module, t, ev);
		}
	}

	if (are_types_identical(src, dst)) {
		return value;
	}

	if (is_type_maybe(dst)) {
		gbAllocator a = proc->module->allocator;
		Type *elem = base_type(dst)->Maybe.elem;
		ssaValue *maybe = ssa_add_local_generated(proc, dst);
		ssaValue *val = ssa_emit_struct_gep(proc, maybe, v_zero32, make_type_pointer(a, elem));
		ssaValue *set = ssa_emit_struct_gep(proc, maybe, v_one32,  make_type_pointer(a, t_bool));
		ssa_emit_store(proc, val, value);
		ssa_emit_store(proc, set, v_true);
		return ssa_emit_load(proc, maybe);
	}

	// integer -> integer
	if (is_type_integer(src) && is_type_integer(dst)) {
		GB_ASSERT(src->kind == Type_Basic &&
		          dst->kind == Type_Basic);
		i64 sz = type_size_of(proc->module->sizes, proc->module->allocator, src);
		i64 dz = type_size_of(proc->module->sizes, proc->module->allocator, dst);
		if (sz == dz) {
			// NOTE(bill): In LLVM, all integers are signed and rely upon 2's compliment
			return value;
		}

		ssaConvKind kind = ssaConv_trunc;
		if (dz >= sz) {
			kind = ssaConv_zext;
		}
		return ssa_emit(proc, ssa_make_instr_conv(proc, kind, value, src, dst));
	}

	// boolean -> integer
	if (is_type_boolean(src) && is_type_integer(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_zext, value, src, dst));
	}

	// integer -> boolean
	if (is_type_integer(src) && is_type_boolean(dst)) {
		return ssa_emit_comp(proc, Token_NotEq, value, v_zero);
	}


	// float -> float
	if (is_type_float(src) && is_type_float(dst)) {
		i64 sz = basic_type_sizes[src->Basic.kind];
		i64 dz = basic_type_sizes[dst->Basic.kind];
		ssaConvKind kind = ssaConv_fptrunc;
		if (dz >= sz) {
			kind = ssaConv_fpext;
		}
		return ssa_emit(proc, ssa_make_instr_conv(proc, kind, value, src, dst));
	}

	// float <-> integer
	if (is_type_float(src) && is_type_integer(dst)) {
		ssaConvKind kind = ssaConv_fptosi;
		if (is_type_unsigned(dst)) {
			kind = ssaConv_fptoui;
		}
		return ssa_emit(proc, ssa_make_instr_conv(proc, kind, value, src, dst));
	}
	if (is_type_integer(src) && is_type_float(dst)) {
		ssaConvKind kind = ssaConv_sitofp;
		if (is_type_unsigned(src)) {
			kind = ssaConv_uitofp;
		}
		return ssa_emit(proc, ssa_make_instr_conv(proc, kind, value, src, dst));
	}

	// Pointer <-> int
	if (is_type_pointer(src) && is_type_int_or_uint(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_ptrtoint, value, src, dst));
	}
	if (is_type_int_or_uint(src) && is_type_pointer(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_inttoptr, value, src, dst));
	}

	if (is_type_union(dst)) {
		for (isize i = 0; i < dst->Record.field_count; i++) {
			Entity *f = dst->Record.fields[i];
			if (are_types_identical(f->type, src_type)) {
				ssa_emit_comment(proc, make_string("union - child to parent"));
				gbAllocator allocator = proc->module->allocator;
				ssaValue *parent = ssa_add_local_generated(proc, t);
				ssaValue *tag = ssa_make_const_int(allocator, i);
				ssa_emit_store(proc, ssa_emit_struct_gep(proc, parent, v_one32, t_int), tag);

				ssaValue *data = ssa_emit_conv(proc, parent, t_rawptr);

				Type *tag_type = src_type;
				Type *tag_type_ptr = make_type_pointer(allocator, tag_type);
				ssaValue *underlying = ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, data, t_rawptr, tag_type_ptr));
				ssa_emit_store(proc, underlying, value);

				return ssa_emit_load(proc, parent);
			}
		}
	}

	// NOTE(bill): This has to be done beofre `Pointer <-> Pointer` as it's
	// subtype polymorphism casting
	if (true || is_argument) {
		Type *sb = base_type(type_deref(src));
		b32 src_is_ptr = src != sb;
		if (is_type_struct(sb)) {
			String field_name = lookup_polymorphic_field(proc->module->info, t, src);
			// gb_printf("field_name: %.*s\n", LIT(field_name));
			if (field_name.len > 0) {
				// NOTE(bill): It can be casted
				Selection sel = lookup_field(proc->module->allocator, sb, field_name, false);
				if (sel.entity != NULL) {
					ssa_emit_comment(proc, make_string("cast - polymorphism"));
					if (src_is_ptr) {
						value = ssa_emit_load(proc, value);
					}
					return ssa_emit_deep_field_ev(proc, sb, value, sel);
				}
			}
		}
	}



	// Pointer <-> Pointer
	if (is_type_pointer(src) && is_type_pointer(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}



	// proc <-> proc
	if (is_type_proc(src) && is_type_proc(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}

	// pointer -> proc
	if (is_type_pointer(src) && is_type_proc(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}
	// proc -> pointer
	if (is_type_proc(src) && is_type_pointer(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}



	// []byte/[]u8 <-> string
	if (is_type_u8_slice(src) && is_type_string(dst)) {
		ssaValue *elem = ssa_slice_elem(proc, value);
		ssaValue *len  = ssa_slice_len(proc, value);
		return ssa_emit_string(proc, elem, len);
	}
	if (is_type_string(src) && is_type_u8_slice(dst)) {
		ssaValue *elem = ssa_string_elem(proc, value);
		ssaValue *elem_ptr = ssa_add_local_generated(proc, ssa_type(elem));
		ssa_emit_store(proc, elem_ptr, elem);

		ssaValue *len  = ssa_string_len(proc, value);
		ssaValue *slice = ssa_add_local_slice(proc, dst, elem_ptr, v_zero, len, len);
		return ssa_emit_load(proc, slice);
	}

	if (is_type_vector(dst)) {
		Type *dst_elem = dst->Vector.elem;
		value = ssa_emit_conv(proc, value, dst_elem);
		ssaValue *v = ssa_add_local_generated(proc, t);
		v = ssa_emit_load(proc, v);
		v = ssa_emit(proc, ssa_make_instr_insert_element(proc, v, value, v_zero32));
		// NOTE(bill): Broadcast lowest value to all values
		isize index_count = dst->Vector.count;
		i32 *indices = gb_alloc_array(proc->module->allocator, i32, index_count);
		for (isize i = 0; i < index_count; i++) {
			indices[i] = 0;
		}

		v = ssa_emit(proc, ssa_make_instr_shuffle_vector(proc, v, indices, index_count));
		return v;
	}

	if (is_type_any(dst)) {
		ssaValue *result = ssa_add_local_generated(proc, t_any);

		if (is_type_untyped_nil(src)) {
			return ssa_emit_load(proc, result);
		}

		ssaValue *data = NULL;
		if (value->kind == ssaValue_Instr &&
		    value->Instr.kind == ssaInstr_Load) {
			// NOTE(bill): Addressable value
			data = value->Instr.Load.address;
		} else {
			// NOTE(bill): Non-addressable value
			data = ssa_add_local_generated(proc, src_type);
			ssa_emit_store(proc, data, value);
		}
		GB_ASSERT(is_type_pointer(ssa_type(data)));
		GB_ASSERT(is_type_typed(src_type));
		data = ssa_emit_conv(proc, data, t_rawptr);


		ssaValue *ti = ssa_type_info(proc, src_type);

		ssaValue *gep0 = ssa_emit_struct_gep(proc, result, v_zero32, make_type_pointer(proc->module->allocator, t_type_info_ptr));
		ssaValue *gep1 = ssa_emit_struct_gep(proc, result, v_one32,  make_type_pointer(proc->module->allocator, t_rawptr));
		ssa_emit_store(proc, gep0, ti);
		ssa_emit_store(proc, gep1, data);

		return ssa_emit_load(proc, result);
	}

	if (is_type_untyped_nil(src) && type_has_nil(dst)) {
		return ssa_make_value_nil(proc->module->allocator, t);
	}


	gb_printf_err("Not Identical %s != %s\n", type_to_string(src_type), type_to_string(t));
	gb_printf_err("Not Identical %s != %s\n", type_to_string(src), type_to_string(dst));


	GB_PANIC("Invalid type conversion: `%s` to `%s`", type_to_string(src_type), type_to_string(t));

	return NULL;
}


ssaValue *ssa_emit_transmute(ssaProcedure *proc, ssaValue *value, Type *t) {
	Type *src_type = ssa_type(value);
	if (are_types_identical(t, src_type)) {
		return value;
	}

	Type *src = base_type(src_type);
	Type *dst = base_type(t);
	if (are_types_identical(t, src_type)) {
		return value;
	}

	i64 sz = type_size_of(proc->module->sizes, proc->module->allocator, src);
	i64 dz = type_size_of(proc->module->sizes, proc->module->allocator, dst);

	if (sz == dz) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}


	GB_PANIC("Invalid transmute conversion: `%s` to `%s`", type_to_string(src_type), type_to_string(t));

	return NULL;
}

ssaValue *ssa_emit_down_cast(ssaProcedure *proc, ssaValue *value, Type *t) {
	GB_ASSERT(is_type_pointer(ssa_type(value)));
	gbAllocator allocator = proc->module->allocator;

	String field_name = check_down_cast_name(t, type_deref(ssa_type(value)));
	GB_ASSERT(field_name.len > 0);
	Selection sel = lookup_field(proc->module->allocator, t, field_name, false);
	Type *t_u8_ptr = make_type_pointer(allocator, t_u8);
	ssaValue *bytes = ssa_emit_conv(proc, value, t_u8_ptr);

	i64 offset_ = type_offset_of_from_selection(proc->module->sizes, allocator, type_deref(t), sel);
	ssaValue *offset = ssa_make_const_int(allocator, -offset_);
	ssaValue *head = ssa_emit_ptr_offset(proc, bytes, offset);
	return ssa_emit_conv(proc, head, t);
}

void ssa_build_cond(ssaProcedure *proc, AstNode *cond, ssaBlock *true_block, ssaBlock *false_block);

ssaValue *ssa_emit_logical_binary_expr(ssaProcedure *proc, AstNode *expr) {
	ast_node(be, BinaryExpr, expr);
#if 0
	ssaBlock *true_   = ssa_add_block(proc, NULL, "logical.cmp.true");
	ssaBlock *false_  = ssa_add_block(proc, NULL, "logical.cmp.false");
	ssaBlock *done  = ssa_add_block(proc, NULL, "logical.cmp.done");

	ssaValue *result = ssa_add_local_generated(proc, t_bool);
	ssa_build_cond(proc, expr, true_, false_);

	proc->curr_block = true_;
	ssa_emit_store(proc, result, v_true);
	ssa_emit_jump(proc, done);

	proc->curr_block = false_;
	ssa_emit_store(proc, result, v_false);
	ssa_emit_jump(proc, done);

	proc->curr_block = done;

	return ssa_emit_load(proc, result);
#else
	ssaBlock *rhs = ssa_add_block(proc, NULL, "logical.cmp.rhs");
	ssaBlock *done = ssa_add_block(proc, NULL, "logical.cmp.done");

	Type *type = type_of_expr(proc->module->info, expr);
	type = default_type(type);

	ssaValue *short_circuit = NULL;
	if (be->op.kind == Token_CmpAnd) {
		ssa_build_cond(proc, be->left, rhs, done);
		short_circuit = v_false;
	} else if (be->op.kind == Token_CmpOr) {
		ssa_build_cond(proc, be->left, done, rhs);
		short_circuit = v_true;
	}

	if (rhs->preds.count == 0) {
		proc->curr_block = done;
		return short_circuit;
	}

	if (done->preds.count == 0) {
		proc->curr_block = rhs;
		return ssa_build_expr(proc, be->right);
	}

	Array<ssaValue *> edges = {};
	array_init(&edges, proc->module->allocator, done->preds.count+1);
	for_array(i, done->preds) {
		array_add(&edges, short_circuit);
	}

	proc->curr_block = rhs;
	array_add(&edges, ssa_build_expr(proc, be->right));
	ssa_emit_jump(proc, done);
	proc->curr_block = done;

	return ssa_emit(proc, ssa_make_instr_phi(proc, edges, type));
#endif
}


void ssa_array_bounds_check(ssaProcedure *proc, Token token, ssaValue *index, ssaValue *len) {
	if ((proc->module->stmt_state_flags & StmtStateFlag_no_bounds_check) != 0) {
		return;
	}

	gbAllocator a = proc->module->allocator;
	ssaValue **args = gb_alloc_array(a, ssaValue *, 5);
	args[0] = ssa_emit_global_string(proc, token.pos.file);
	args[1] = ssa_make_const_int(a, token.pos.line);
	args[2] = ssa_make_const_int(a, token.pos.column);
	args[3] = ssa_emit_conv(proc, index, t_int);
	args[4] = ssa_emit_conv(proc, len, t_int);

	ssa_emit_global_call(proc, "__bounds_check_error", args, 5);
}

void ssa_slice_bounds_check(ssaProcedure *proc, Token token, ssaValue *low, ssaValue *high, ssaValue *max, b32 is_substring) {
	if ((proc->module->stmt_state_flags & StmtStateFlag_no_bounds_check) != 0) {
		return;
	}

	gbAllocator a = proc->module->allocator;
	ssaValue **args = gb_alloc_array(a, ssaValue *, 6);
	args[0] = ssa_emit_global_string(proc, token.pos.file);
	args[1] = ssa_make_const_int(a, token.pos.line);
	args[2] = ssa_make_const_int(a, token.pos.column);
	args[3] = ssa_emit_conv(proc, low, t_int);
	args[4] = ssa_emit_conv(proc, high, t_int);
	args[5] = ssa_emit_conv(proc, max, t_int);

	if (!is_substring) {
		ssa_emit_global_call(proc, "__slice_expr_error", args, 6);
	} else {
		ssa_emit_global_call(proc, "__substring_expr_error", args, 5);
	}
}

ssaValue *ssa_find_global_variable(ssaProcedure *proc, String name) {
	ssaValue *value = *map_get(&proc->module->members, hash_string(name));
	return value;
}

ssaValue *ssa_emit_implicit_value(ssaProcedure *proc, Entity *e) {
	String name = e->token.string;
	ssaValue *g = NULL;
	if (name == "context") {
		g = ssa_emit_load(proc, ssa_find_global_variable(proc, make_string("__context")));
	}
	GB_ASSERT(g != NULL);
	return g;
}


ssaValue *ssa_build_single_expr(ssaProcedure *proc, AstNode *expr, TypeAndValue *tv) {
	switch (expr->kind) {
	case_ast_node(bl, BasicLit, expr);
		GB_PANIC("Non-constant basic literal");
	case_end;

	case_ast_node(i, Ident, expr);
		Entity *e = *map_get(&proc->module->info->uses, hash_pointer(expr));
		if (e->kind == Entity_Builtin) {
			Token token = ast_node_token(expr);
			GB_PANIC("TODO(bill): ssa_build_single_expr Entity_Builtin `%.*s`\n"
			         "\t at %.*s(%td:%td)", LIT(builtin_procs[e->Builtin.id].name),
			         LIT(token.pos.file), token.pos.line, token.pos.column);
			return NULL;
		} else if (e->kind == Entity_Nil) {
			return ssa_make_value_nil(proc->module->allocator, tv->type);
		} else if (e->kind == Entity_ImplicitValue) {
			return ssa_emit_implicit_value(proc, e);
		}

		auto *found = map_get(&proc->module->values, hash_pointer(e));
		if (found) {
			ssaValue *v = *found;
			if (v->kind == ssaValue_Proc) {
				return v;
			}
			return ssa_emit_load(proc, v);
		}
		return NULL;
	case_end;

	case_ast_node(pe, ParenExpr, expr);
		return ssa_build_single_expr(proc, unparen_expr(expr), tv);
	case_end;

	case_ast_node(de, DerefExpr, expr);
		return ssa_lvalue_load(proc, ssa_build_addr(proc, expr));
	case_end;

	case_ast_node(se, SelectorExpr, expr);
		return ssa_lvalue_load(proc, ssa_build_addr(proc, expr));
	case_end;

	case_ast_node(ue, UnaryExpr, expr);
		switch (ue->op.kind) {
		case Token_Pointer:
			return ssa_emit_zero_gep(proc, ssa_build_addr(proc, ue->expr).addr); // Make a copy of the pointer

		case Token_Maybe:
			return ssa_emit_conv(proc, ssa_build_expr(proc, ue->expr), type_of_expr(proc->module->info, expr));

		case Token_Add:
			return ssa_build_expr(proc, ue->expr);

		case Token_Sub: // NOTE(bill): -`x` == 0 - `x`
			return ssa_emit_arith(proc, ue->op.kind, v_zero, ssa_build_expr(proc, ue->expr), tv->type);

		case Token_Not:   // Boolean not
		case Token_Xor: { // Bitwise not
			// NOTE(bill): "not" `x` == `x` "xor" `-1`
			ssaValue *left = ssa_build_expr(proc, ue->expr);
			ssaValue *right = ssa_add_module_constant(proc->module, tv->type, make_exact_value_integer(-1));
			return ssa_emit_arith(proc, ue->op.kind,
			                      left, right,
			                      tv->type);
		} break;
		}
	case_end;

	case_ast_node(be, BinaryExpr, expr);
		switch (be->op.kind) {
		case Token_Add:
		case Token_Sub:
		case Token_Mul:
		case Token_Quo:
		case Token_Mod:
		case Token_And:
		case Token_Or:
		case Token_Xor:
		case Token_AndNot:
		case Token_Shl:
		case Token_Shr:
			return ssa_emit_arith(proc, be->op.kind,
			                      ssa_build_expr(proc, be->left),
			                      ssa_build_expr(proc, be->right),
			                      tv->type);


		case Token_CmpEq:
		case Token_NotEq:
		case Token_Lt:
		case Token_LtEq:
		case Token_Gt:
		case Token_GtEq: {
			ssaValue *left  = ssa_build_expr(proc, be->left);
			ssaValue *right = ssa_build_expr(proc, be->right);

			ssaValue *cmp = ssa_emit_comp(proc, be->op.kind, left, right);
			return ssa_emit_conv(proc, cmp, default_type(tv->type));
		} break;

		case Token_CmpAnd:
		case Token_CmpOr:
			return ssa_emit_logical_binary_expr(proc, expr);

		case Token_as:
			ssa_emit_comment(proc, make_string("cast - as"));
			return ssa_emit_conv(proc, ssa_build_expr(proc, be->left), tv->type);

		case Token_transmute:
			ssa_emit_comment(proc, make_string("cast - transmute"));
			return ssa_emit_transmute(proc, ssa_build_expr(proc, be->left), tv->type);

		case Token_down_cast:
			ssa_emit_comment(proc, make_string("cast - down_cast"));
			return ssa_emit_down_cast(proc, ssa_build_expr(proc, be->left), tv->type);

		default:
			GB_PANIC("Invalid binary expression");
			break;
		}
	case_end;

	case_ast_node(pl, ProcLit, expr);
		// NOTE(bill): Generate a new name
		// parent$count
		isize name_len = proc->name.len + 1 + 8 + 1;
		u8 *name_text = gb_alloc_array(proc->module->allocator, u8, name_len);
		name_len = gb_snprintf(cast(char *)name_text, name_len, "%.*s$%d", LIT(proc->name), cast(i32)proc->children.count);
		String name = make_string(name_text, name_len-1);

		Type *type = type_of_expr(proc->module->info, expr);
		ssaValue *value = ssa_make_value_procedure(proc->module->allocator,
		                                           proc->module, NULL, type, pl->type, pl->body, name);

		value->Proc.tags = pl->tags;

		array_add(&proc->children, &value->Proc);
		ssa_build_proc(value, proc);

		return value;
	case_end;


	case_ast_node(cl, CompoundLit, expr);
		ssa_emit_comment(proc, make_string("CompoundLit"));
		Type *type = type_of_expr(proc->module->info, expr);
		Type *bt = base_type(type);
		ssaValue *v = ssa_add_local_generated(proc, type);

		Type *et = NULL;
		switch (bt->kind) {
		case Type_Vector: et = bt->Vector.elem; break;
		case Type_Array:  et = bt->Array.elem;  break;
		case Type_Slice:  et = bt->Slice.elem;  break;
		}

		auto is_elem_const = [](ssaModule *m, AstNode *elem) -> b32 {
			if (elem->kind == AstNode_FieldValue) {
				elem = elem->FieldValue.value;
			}
			TypeAndValue *tav = type_and_value_of_expression(m->info, elem);
			GB_ASSERT(tav != NULL);
			return tav->value.kind != ExactValue_Invalid;
		};

		switch (bt->kind) {
		default: GB_PANIC("Unknown CompoundLit type: %s", type_to_string(type)); break;

		case Type_Vector: {
			ssaValue *result = ssa_add_module_constant(proc->module, type, make_exact_value_compound(expr));
			for_array(index, cl->elems) {
				AstNode *elem = cl->elems[index];
				if (is_elem_const(proc->module, elem)) {
					continue;
				}
				ssaValue *field_elem = ssa_build_expr(proc, elem);
				Type *t = ssa_type(field_elem);
				GB_ASSERT(t->kind != Type_Tuple);
				ssaValue *ev = ssa_emit_conv(proc, field_elem, et);
				ssaValue *i = ssa_make_const_int(proc->module->allocator, index);
				result = ssa_emit(proc, ssa_make_instr_insert_element(proc, result, ev, i));
			}

			if (cl->elems.count == 1 && bt->Vector.count > 1) {
				isize index_count = bt->Vector.count;
				i32 *indices = gb_alloc_array(proc->module->allocator, i32, index_count);
				for (isize i = 0; i < index_count; i++) {
					indices[i] = 0;
				}
				ssaValue *sv = ssa_emit(proc, ssa_make_instr_shuffle_vector(proc, result, indices, index_count));
				ssa_emit_store(proc, v, sv);
				return ssa_emit_load(proc, v);
			}
			return result;
		} break;

		case Type_Record: {
			GB_ASSERT(is_type_struct(bt));
			auto *st = &bt->Record;
			if (cl->elems.count > 0) {
				ssa_emit_store(proc, v, ssa_add_module_constant(proc->module, type, make_exact_value_compound(expr)));
				for_array(field_index, cl->elems) {
					AstNode *elem = cl->elems[field_index];
					if (is_elem_const(proc->module, elem)) {
						continue;
					}

					ssaValue *field_expr = NULL;
					Entity *field = NULL;
					isize index = field_index;

					if (elem->kind == AstNode_FieldValue) {
						ast_node(fv, FieldValue, elem);
						Selection sel = lookup_field(proc->module->allocator, bt, fv->field->Ident.string, false);
						index = sel.index[0];
						field_expr = ssa_build_expr(proc, fv->value);
					} else {
						TypeAndValue *tav = type_and_value_of_expression(proc->module->info, elem);
						Selection sel = lookup_field(proc->module->allocator, bt, st->fields_in_src_order[field_index]->token.string, false);
						index = sel.index[0];
						field_expr = ssa_build_expr(proc, elem);
					}

					GB_ASSERT(ssa_type(field_expr)->kind != Type_Tuple);

					field = st->fields[index];

					Type *ft = field->type;
					ssaValue *fv = ssa_emit_conv(proc, field_expr, ft);
					ssaValue *gep = ssa_emit_struct_gep(proc, v, index, ft);
					ssa_emit_store(proc, gep, fv);
				}
			}
		} break;
		case Type_Array: {
			if (cl->elems.count > 0) {
				ssa_emit_store(proc, v, ssa_add_module_constant(proc->module, type, make_exact_value_compound(expr)));
				for_array(i, cl->elems) {
					AstNode *elem = cl->elems[i];
					if (is_elem_const(proc->module, elem)) {
						continue;
					}
					ssaValue *field_expr = ssa_build_expr(proc, elem);
					Type *t = ssa_type(field_expr);
					GB_ASSERT(t->kind != Type_Tuple);
					ssaValue *ev = ssa_emit_conv(proc, field_expr, et);
					ssaValue *gep = ssa_emit_struct_gep(proc, v, i, et);
					ssa_emit_store(proc, gep, ev);
				}
			}
		} break;
		case Type_Slice: {
			if (cl->elems.count > 0) {
				Type *elem_type = bt->Slice.elem;
				Type *elem_ptr_type = make_type_pointer(proc->module->allocator, elem_type);
				Type *elem_ptr_ptr_type = make_type_pointer(proc->module->allocator, elem_ptr_type);
				Type *t_int_ptr = make_type_pointer(proc->module->allocator, t_int);
				ssaValue *slice = ssa_add_module_constant(proc->module, type, make_exact_value_compound(expr));
				GB_ASSERT(slice->kind == ssaValue_ConstantSlice);

				ssaValue *data = ssa_emit_struct_gep(proc, slice->ConstantSlice.backing_array, v_zero32, elem_ptr_type);

				for_array(i, cl->elems) {
					AstNode *elem = cl->elems[i];
					if (is_elem_const(proc->module,elem)) {
						continue;
					}

					ssaValue *field_expr = ssa_build_expr(proc, elem);
					Type *t = ssa_type(field_expr);
					GB_ASSERT(t->kind != Type_Tuple);
					ssaValue *ev = ssa_emit_conv(proc, field_expr, elem_type);
					ssaValue *offset = ssa_emit_ptr_offset(proc, data, ssa_make_const_int(proc->module->allocator, i));
					ssa_emit_store(proc, offset, ev);
				}

				ssaValue *gep0 = ssa_emit_struct_gep(proc, v, v_zero32, elem_ptr_ptr_type);
				ssaValue *gep1 = ssa_emit_struct_gep(proc, v, v_one32, t_int_ptr);
				ssaValue *gep2 = ssa_emit_struct_gep(proc, v, v_two32, t_int_ptr);

				ssa_emit_store(proc, gep0, data);
				ssa_emit_store(proc, gep1, ssa_make_const_int(proc->module->allocator, slice->ConstantSlice.count));
				ssa_emit_store(proc, gep2, ssa_make_const_int(proc->module->allocator, slice->ConstantSlice.count));
			}
		} break;
		}

		return ssa_emit_load(proc, v);
	case_end;


	case_ast_node(ce, CallExpr, expr);
		AstNode *p = unparen_expr(ce->proc);
		if (p->kind == AstNode_Ident) {
			Entity **found = map_get(&proc->module->info->uses, hash_pointer(p));
			if (found && (*found)->kind == Entity_Builtin) {
				Entity *e = *found;
				switch (e->Builtin.id) {
				case BuiltinProc_type_info: {
					Type *t = default_type(type_of_expr(proc->module->info, ce->args[0]));
					return ssa_type_info(proc, t);
				} break;
				case BuiltinProc_type_info_of_val: {
					Type *t = default_type(type_of_expr(proc->module->info, ce->args[0]));
					return ssa_type_info(proc, t);
				} break;

				case BuiltinProc_new: {
					ssa_emit_comment(proc, make_string("new"));
					// new :: proc(Type) -> ^Type
					gbAllocator allocator = proc->module->allocator;

					Type *type = type_of_expr(proc->module->info, ce->args[0]);
					Type *ptr_type = make_type_pointer(allocator, type);

					i64 s = type_size_of(proc->module->sizes, allocator, type);
					i64 a = type_align_of(proc->module->sizes, allocator, type);

					ssaValue **args = gb_alloc_array(allocator, ssaValue *, 2);
					args[0] = ssa_make_const_int(allocator, s);
					args[1] = ssa_make_const_int(allocator, a);
					ssaValue *call = ssa_emit_global_call(proc, "alloc_align", args, 2);
					ssaValue *v = ssa_emit_conv(proc, call, ptr_type);
					return v;
				} break;

				case BuiltinProc_new_slice: {
					ssa_emit_comment(proc, make_string("new_slice"));
					// new_slice :: proc(Type, len: int[, cap: int]) -> ^Type
					gbAllocator allocator = proc->module->allocator;

					Type *type = type_of_expr(proc->module->info, ce->args[0]);
					Type *ptr_type = make_type_pointer(allocator, type);
					Type *slice_type = make_type_slice(allocator, type);

					i64 s = type_size_of(proc->module->sizes, allocator, type);
					i64 a = type_align_of(proc->module->sizes, allocator, type);

					ssaValue *elem_size  = ssa_make_const_int(allocator, s);
					ssaValue *elem_align = ssa_make_const_int(allocator, a);

					ssaValue *len =ssa_emit_conv(proc, ssa_build_expr(proc, ce->args[1]), t_int);
					ssaValue *cap = len;
					if (ce->args.count == 3) {
						cap = ssa_emit_conv(proc, ssa_build_expr(proc, ce->args[2]), t_int);
					}

					ssa_slice_bounds_check(proc, ast_node_token(ce->args[1]), v_zero, len, cap, false);

					ssaValue *slice_size = ssa_emit_arith(proc, Token_Mul, elem_size, cap, t_int);

					ssaValue **args = gb_alloc_array(allocator, ssaValue *, 2);
					args[0] = slice_size;
					args[1] = elem_align;
					ssaValue *call = ssa_emit_global_call(proc, "alloc_align", args, 2);

					ssaValue *ptr = ssa_emit_conv(proc, call, ptr_type, true);
					ssaValue *slice = ssa_add_local_generated(proc, slice_type);

					ssaValue *gep0 = ssa_emit_struct_gep(proc, slice, v_zero32, ptr_type);
					ssaValue *gep1 = ssa_emit_struct_gep(proc, slice, v_one32, t_int);
					ssaValue *gep2 = ssa_emit_struct_gep(proc, slice, v_two32, t_int);
					ssa_emit_store(proc, gep0, ptr);
					ssa_emit_store(proc, gep1, len);
					ssa_emit_store(proc, gep2, cap);
					return ssa_emit_load(proc, slice);
				} break;

				case BuiltinProc_assert: {
					ssa_emit_comment(proc, make_string("assert"));
					ssaValue *cond = ssa_build_expr(proc, ce->args[0]);
					GB_ASSERT(is_type_boolean(ssa_type(cond)));

					cond = ssa_emit_comp(proc, Token_CmpEq, cond, v_false);
					ssaBlock *err  = ssa_add_block(proc, NULL, "builtin.assert.err");
					ssaBlock *done = ssa_add_block(proc, NULL, "builtin.assert.done");

					ssa_emit_if(proc, cond, err, done);
					proc->curr_block = err;

					// TODO(bill): Cleanup allocations here
					Token token = ast_node_token(ce->args[0]);
					TokenPos pos = token.pos;
					gbString expr = expr_to_string(ce->args[0]);
					defer (gb_string_free(expr));
					isize expr_len = gb_string_length(expr);
					String expr_str = {};
					expr_str.text = cast(u8 *)gb_alloc_copy_align(proc->module->allocator, expr, expr_len, 1);
					expr_str.len = expr_len;

					ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, 4);
					args[0] = ssa_emit_global_string(proc, pos.file);
					args[1] = ssa_make_const_int(proc->module->allocator, pos.line);
					args[2] = ssa_make_const_int(proc->module->allocator, pos.column);
					args[3] = ssa_emit_global_string(proc, expr_str);
					ssa_emit_global_call(proc, "__assert", args, 4);

					ssa_emit_jump(proc, done);
					proc->curr_block = done;

					return NULL;
				} break;

				case BuiltinProc_panic: {
					ssa_emit_comment(proc, make_string("panic"));
					ssaValue *msg = ssa_build_expr(proc, ce->args[0]);
					GB_ASSERT(is_type_string(ssa_type(msg)));

					Token token = ast_node_token(ce->args[0]);
					TokenPos pos = token.pos;

					ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, 4);
					args[0] = ssa_emit_global_string(proc, pos.file);
					args[1] = ssa_make_const_int(proc->module->allocator, pos.line);
					args[2] = ssa_make_const_int(proc->module->allocator, pos.column);
					args[3] = msg;
					ssa_emit_global_call(proc, "__assert", args, 4);

					return NULL;
				} break;


				case BuiltinProc_copy: {
					ssa_emit_comment(proc, make_string("copy"));
					// copy :: proc(dst, src: []Type) -> int
					AstNode *dst_node = ce->args[0];
					AstNode *src_node = ce->args[1];
					ssaValue *dst_slice = ssa_build_expr(proc, dst_node);
					ssaValue *src_slice = ssa_build_expr(proc, src_node);
					Type *slice_type = base_type(ssa_type(dst_slice));
					GB_ASSERT(slice_type->kind == Type_Slice);
					Type *elem_type = slice_type->Slice.elem;
					i64 size_of_elem = type_size_of(proc->module->sizes, proc->module->allocator, elem_type);


					ssaValue *dst = ssa_emit_conv(proc, ssa_slice_elem(proc, dst_slice), t_rawptr, true);
					ssaValue *src = ssa_emit_conv(proc, ssa_slice_elem(proc, src_slice), t_rawptr, true);

					ssaValue *len_dst = ssa_slice_len(proc, dst_slice);
					ssaValue *len_src = ssa_slice_len(proc, src_slice);

					ssaValue *cond = ssa_emit_comp(proc, Token_Lt, len_dst, len_src);
					ssaValue *len = ssa_emit_select(proc, cond, len_dst, len_src);

					ssaValue *elem_size = ssa_make_const_int(proc->module->allocator, size_of_elem);
					ssaValue *byte_count = ssa_emit_arith(proc, Token_Mul, len, elem_size, t_int);

					ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, 3);
					args[0] = dst;
					args[1] = src;
					args[2] = byte_count;

					ssa_emit_global_call(proc, "__mem_copy", args, 3);

					return len;
				} break;
				case BuiltinProc_append: {
					ssa_emit_comment(proc, make_string("append"));
					// append :: proc(s: ^[]Type, item: Type) -> bool
					AstNode *sptr_node = ce->args[0];
					AstNode *item_node = ce->args[1];
					ssaValue *slice_ptr = ssa_build_expr(proc, sptr_node);
					ssaValue *slice = ssa_emit_load(proc, slice_ptr);

					ssaValue *elem = ssa_slice_elem(proc, slice);
					ssaValue *len  = ssa_slice_len(proc,  slice);
					ssaValue *cap  = ssa_slice_cap(proc,  slice);

					Type *elem_type = type_deref(ssa_type(elem));

					ssaValue *item_value = ssa_build_expr(proc, item_node);
					item_value = ssa_emit_conv(proc, item_value, elem_type, true);

					ssaValue *item = ssa_add_local_generated(proc, elem_type);
					ssa_emit_store(proc, item, item_value);


					// NOTE(bill): Check if can append is possible
					ssaValue *cond = ssa_emit_comp(proc, Token_Lt, len, cap);
					ssaBlock *able = ssa_add_block(proc, NULL, "builtin.append.able");
					ssaBlock *done = ssa_add_block(proc, NULL, "builtin.append.done");

					ssa_emit_if(proc, cond, able, done);
					proc->curr_block = able;

					// Add new slice item
					i64 item_size = type_size_of(proc->module->sizes, proc->module->allocator, elem_type);
					ssaValue *byte_count = ssa_make_const_int(proc->module->allocator, item_size);

					ssaValue *offset = ssa_emit_ptr_offset(proc, elem, len);
					offset = ssa_emit_conv(proc, offset, t_rawptr, true);

					item = ssa_emit_ptr_offset(proc, item, v_zero);
					item = ssa_emit_conv(proc, item, t_rawptr, true);

					ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, 3);
					args[0] = offset;
					args[1] = item;
					args[2] = byte_count;

					ssa_emit_global_call(proc, "__mem_copy", args, 3);

					// Increment slice length
					ssaValue *new_len = ssa_emit_arith(proc, Token_Add, len, v_one, t_int);
					ssaValue *gep = ssa_emit_struct_gep(proc, slice_ptr, v_one32, t_int);
					ssa_emit_store(proc, gep, new_len);

					ssa_emit_jump(proc, done);
					proc->curr_block = done;

					return ssa_emit_conv(proc, cond, t_bool, true);
				} break;

				case BuiltinProc_swizzle: {
					ssa_emit_comment(proc, make_string("swizzle"));
					ssaValue *vector = ssa_build_expr(proc, ce->args[0]);
					isize index_count = ce->args.count-1;
					if (index_count == 0) {
						return vector;
					}

					i32 *indices = gb_alloc_array(proc->module->allocator, i32, index_count);
					isize index = 0;
					for_array(i, ce->args) {
						if (i == 0) continue;
						TypeAndValue *tv = type_and_value_of_expression(proc->module->info, ce->args[i]);
						GB_ASSERT(is_type_integer(tv->type));
						GB_ASSERT(tv->value.kind == ExactValue_Integer);
						indices[index++] = cast(i32)tv->value.value_integer;
					}

					return ssa_emit(proc, ssa_make_instr_shuffle_vector(proc, vector, indices, index_count));

				} break;

#if 0
				case BuiltinProc_ptr_offset: {
					ssa_emit_comment(proc, make_string("ptr_offset"));
					ssaValue *ptr = ssa_build_expr(proc, ce->args[0]);
					ssaValue *offset = ssa_build_expr(proc, ce->args[1]);
					return ssa_emit_ptr_offset(proc, ptr, offset);
				} break;

				case BuiltinProc_ptr_sub: {
					ssa_emit_comment(proc, make_string("ptr_sub"));
					ssaValue *ptr_a = ssa_build_expr(proc, ce->args[0]);
					ssaValue *ptr_b = ssa_build_expr(proc, ce->args[1]);
					Type *ptr_type = base_type(ssa_type(ptr_a));
					GB_ASSERT(ptr_type->kind == Type_Pointer);
					isize elem_size = type_size_of(proc->module->sizes, proc->module->allocator, ptr_type->Pointer.elem);

					ssaValue *v = ssa_emit_arith(proc, Token_Sub, ptr_a, ptr_b, t_int);
					if (elem_size > 1) {
						ssaValue *ez = ssa_make_const_int(proc->module->allocator, elem_size);
						v = ssa_emit_arith(proc, Token_Quo, v, ez, t_int);
					}

					return v;
				} break;
#endif

				case BuiltinProc_slice_ptr: {
					ssa_emit_comment(proc, make_string("slice_ptr"));
					ssaValue *ptr = ssa_build_expr(proc, ce->args[0]);
					ssaValue *len = ssa_build_expr(proc, ce->args[1]);
					ssaValue *cap = len;

					len = ssa_emit_conv(proc, len, t_int, true);

					if (ce->args.count == 3) {
						cap = ssa_build_expr(proc, ce->args[2]);
						cap = ssa_emit_conv(proc, cap, t_int, true);
					}


					Type *slice_type = make_type_slice(proc->module->allocator, type_deref(ssa_type(ptr)));
					ssaValue *slice = ssa_add_local_generated(proc, slice_type);
					ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_zero32, ssa_type(ptr)), ptr);
					ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_one32,  t_int),         len);
					ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_two32,  t_int),         cap);
					return ssa_emit_load(proc, slice);
				} break;

				case BuiltinProc_min: {
					ssa_emit_comment(proc, make_string("min"));
					ssaValue *x = ssa_build_expr(proc, ce->args[0]);
					ssaValue *y = ssa_build_expr(proc, ce->args[1]);
					Type *t = base_type(ssa_type(x));
					ssaValue *cond = ssa_emit_comp(proc, Token_Lt, x, y);
					return ssa_emit_select(proc, cond, x, y);
				} break;

				case BuiltinProc_max: {
					ssa_emit_comment(proc, make_string("max"));
					ssaValue *x = ssa_build_expr(proc, ce->args[0]);
					ssaValue *y = ssa_build_expr(proc, ce->args[1]);
					Type *t = base_type(ssa_type(x));
					ssaValue *cond = ssa_emit_comp(proc, Token_Gt, x, y);
					return ssa_emit_select(proc, cond, x, y);
				} break;

				case BuiltinProc_abs: {
					ssa_emit_comment(proc, make_string("abs"));

					ssaValue *x = ssa_build_expr(proc, ce->args[0]);
					Type *t = ssa_type(x);

					ssaValue *neg_x = ssa_emit_arith(proc, Token_Sub, v_zero, x, t);
					ssaValue *cond = ssa_emit_comp(proc, Token_Lt, x, v_zero);
					return ssa_emit_select(proc, cond, neg_x, x);
				} break;

				case BuiltinProc_enum_to_string: {
					ssa_emit_comment(proc, make_string("enum_to_string"));
					ssaValue *x = ssa_build_expr(proc, ce->args[0]);
					Type *t = ssa_type(x);
					ssaValue *ti = ssa_type_info(proc, t);


					ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, 2);
					args[0] = ti;
					args[1] = ssa_emit_conv(proc, x, t_i64);
					return ssa_emit_global_call(proc, "__enum_to_string", args, 2);
				} break;
				}
			}
		}


		// NOTE(bill): Regular call
		ssaValue *value = ssa_build_expr(proc, ce->proc);
		Type *proc_type_ = base_type(ssa_type(value));
		GB_ASSERT(proc_type_->kind == Type_Proc);
		auto *type = &proc_type_->Proc;

		isize arg_index = 0;

		isize arg_count = 0;
		for_array(i, ce->args) {
			AstNode *a = ce->args[i];
			Type *at = base_type(type_of_expr(proc->module->info, a));
			if (at->kind == Type_Tuple) {
				arg_count += at->Tuple.variable_count;
			} else {
				arg_count++;
			}
		}
		ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, arg_count);
		b32 variadic = proc_type_->Proc.variadic;
		b32 vari_expand = ce->ellipsis.pos.line != 0;

		for_array(i, ce->args) {
			ssaValue *a = ssa_build_expr(proc, ce->args[i]);
			Type *at = ssa_type(a);
			if (at->kind == Type_Tuple) {
				for (isize i = 0; i < at->Tuple.variable_count; i++) {
					Entity *e = at->Tuple.variables[i];
					ssaValue *v = ssa_emit_struct_ev(proc, a, i, e->type);
					args[arg_index++] = v;
				}
			} else {
				args[arg_index++] = a;
			}
		}

		auto *pt = &type->params->Tuple;

		if (variadic) {
			isize i = 0;
			for (; i < type->param_count-1; i++) {
				args[i] = ssa_emit_conv(proc, args[i], pt->variables[i]->type, true);
			}
			if (!vari_expand) {
				Type *variadic_type = pt->variables[i]->type;
				GB_ASSERT(is_type_slice(variadic_type));
				variadic_type = base_type(variadic_type)->Slice.elem;
				for (; i < arg_count; i++) {
					args[i] = ssa_emit_conv(proc, args[i], variadic_type, true);
				}
			}
		} else {
			for (isize i = 0; i < arg_count; i++) {
				args[i] = ssa_emit_conv(proc, args[i], pt->variables[i]->type, true);
			}
		}

		if (variadic && !vari_expand) {
			ssa_emit_comment(proc, make_string("variadic call argument generation"));
			gbAllocator allocator = proc->module->allocator;
			Type *slice_type = pt->variables[type->param_count-1]->type;
			Type *elem_type  = base_type(slice_type)->Slice.elem;
			Type *elem_ptr_type = make_type_pointer(allocator, elem_type);
			ssaValue *slice = ssa_add_local_generated(proc, slice_type);
			isize slice_len = arg_count+1 - type->param_count;

			if (slice_len > 0) {
				ssaValue *base_array = ssa_add_local_generated(proc, make_type_array(allocator, elem_type, slice_len));

				for (isize i = type->param_count-1, j = 0; i < arg_count; i++, j++) {
					ssaValue *addr = ssa_emit_struct_gep(proc, base_array, j, elem_type);
					ssa_emit_store(proc, addr, args[i]);
				}

				ssaValue *base_elem  = ssa_emit_struct_gep(proc, base_array, v_zero32, elem_ptr_type);
				ssaValue *slice_elem = ssa_emit_struct_gep(proc, slice,      v_zero32, elem_ptr_type);
				ssa_emit_store(proc, slice_elem, base_elem);
				ssaValue *len = ssa_make_const_int(allocator, slice_len);
				ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_one32, t_int), len);
				ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_two32, t_int), len);
			}


			arg_count = type->param_count;
			args[arg_count-1] = ssa_emit_load(proc, slice);
		}


		return ssa_emit_call(proc, value, args, arg_count);
	case_end;

	case_ast_node(de, DemaybeExpr, expr);
		return ssa_emit_load(proc, ssa_build_addr(proc, expr).addr);
	case_end;

	case_ast_node(se, SliceExpr, expr);
		return ssa_emit_load(proc, ssa_build_addr(proc, expr).addr);
	case_end;

	case_ast_node(ie, IndexExpr, expr);
		return ssa_emit_load(proc, ssa_build_addr(proc, expr).addr);
	case_end;
	}

	GB_PANIC("Unexpected expression: %.*s", LIT(ast_node_strings[expr->kind]));
	return NULL;
}


ssaValue *ssa_build_expr(ssaProcedure *proc, AstNode *expr) {
	expr = unparen_expr(expr);

	TypeAndValue *tv = map_get(&proc->module->info->types, hash_pointer(expr));
	GB_ASSERT_NOT_NULL(tv);

	if (tv->value.kind != ExactValue_Invalid) {
		if (tv->value.kind == ExactValue_String) {
			ssa_emit_comment(proc, make_string("Emit string constant"));

			// TODO(bill): Optimize by not allocating everytime
			if (tv->value.value_string.len > 0) {
				return ssa_emit_global_string(proc, tv->value.value_string);
			} else {
				ssaValue *null_string = ssa_add_local_generated(proc, t_string);
				return ssa_emit_load(proc, null_string);
			}
		}

		return ssa_add_module_constant(proc->module, tv->type, tv->value);
	}

	ssaValue *value = NULL;
	if (tv->mode == Addressing_Variable) {
		ssaAddr addr = ssa_build_addr(proc, expr);
		value = ssa_lvalue_load(proc, addr);
	} else {
		value = ssa_build_single_expr(proc, expr, tv);
	}

	return value;
}

ssaValue *ssa_add_using_variable(ssaProcedure *proc, Entity *e) {
	GB_ASSERT(e->kind == Entity_Variable && e->Variable.anonymous);
	String name = e->token.string;
	Entity *parent = e->using_parent;
	Selection sel = lookup_field(proc->module->allocator, parent->type, name, false);
	GB_ASSERT(sel.entity != NULL);
	ssaValue **pv = map_get(&proc->module->values, hash_pointer(parent));
	ssaValue *v = NULL;
	if (pv != NULL) {
		v = *pv;
	} else {
		v = ssa_build_addr(proc, e->using_expr).addr;
	}
	GB_ASSERT(v != NULL);
	ssaValue *var = ssa_emit_deep_field_gep(proc, parent->type, v, sel);
	map_set(&proc->module->values, hash_pointer(e), var);
	return var;
}

ssaAddr ssa_build_addr(ssaProcedure *proc, AstNode *expr) {
	switch (expr->kind) {
	case_ast_node(i, Ident, expr);
		if (ssa_is_blank_ident(expr)) {
			ssaAddr val = {};
			return val;
		}

		Entity *e = entity_of_ident(proc->module->info, expr);
		TypeAndValue *tv = map_get(&proc->module->info->types, hash_pointer(expr));
		// GB_ASSERT(tv == NULL || tv->mode == Addressing_Variable);


		if (e->kind == Entity_Constant) {
			if (base_type(e->type) == t_string) {
				// HACK TODO(bill): This is lazy but it works
				String str = e->Constant.value.value_string;
				ssaValue *global_array = ssa_add_global_string_array(proc->module, str);
				ssaValue *elem = ssa_array_elem(proc, global_array);
				ssaValue *len =  ssa_make_const_int(proc->module->allocator, str.len);
				ssaValue *v = ssa_add_local_generated(proc, e->type);
				ssaValue *str_elem = ssa_emit_struct_gep(proc, v, v_zero32, ssa_type(elem));
				ssaValue *str_len = ssa_emit_struct_gep(proc, v, v_one32, t_int);
				ssa_emit_store(proc, str_elem, elem);
				ssa_emit_store(proc, str_len, len);
				return ssa_make_addr(v, expr);
			}
		}

		ssaValue *v = NULL;
		ssaValue **found = map_get(&proc->module->values, hash_pointer(e));
		if (found) {
			v = *found;
		} else if (e->kind == Entity_Variable && e->Variable.anonymous) {
			v = ssa_add_using_variable(proc, e);
		} else if (e->kind == Entity_ImplicitValue) {
			ssaValue *g = ssa_emit_implicit_value(proc, e);
			// NOTE(bill): Create a copy as it's by value
			v = ssa_add_local_generated(proc, ssa_type(g));
			ssa_emit_store(proc, v, g);
		}

		if (v == NULL) {
			GB_PANIC("Unknown value: %s, entity: %p %.*s\n", expr_to_string(expr), e, LIT(entity_strings[e->kind]));
		}

		return ssa_make_addr(v, expr);
	case_end;

	case_ast_node(pe, ParenExpr, expr);
		return ssa_build_addr(proc, unparen_expr(expr));
	case_end;

	case_ast_node(se, SelectorExpr, expr);
		ssa_emit_comment(proc, make_string("SelectorExpr"));
		String selector = unparen_expr(se->selector)->Ident.string;
		Type *type = base_type(type_of_expr(proc->module->info, se->expr));

		if (type == t_invalid) {
			// NOTE(bill): Imports
			Entity *imp = entity_of_ident(proc->module->info, se->expr);
			if (imp != NULL) {
				GB_ASSERT(imp->kind == Entity_ImportName);
			}
			return ssa_build_addr(proc, unparen_expr(se->selector));
		} else {
			Selection sel = lookup_field(proc->module->allocator, type, selector, false);
			GB_ASSERT(sel.entity != NULL);

			ssaValue *a = ssa_build_addr(proc, se->expr).addr;
			a = ssa_emit_deep_field_gep(proc, type, a, sel);
			return ssa_make_addr(a, expr);
		}
	case_end;

	case_ast_node(ue, UnaryExpr, expr);
		switch (ue->op.kind) {
		case Token_Pointer: {
			return ssa_build_addr(proc, ue->expr);
		}
		default:
			GB_PANIC("Invalid unary expression for ssa_build_addr");
		}
	case_end;

	case_ast_node(be, BinaryExpr, expr);
		switch (be->op.kind) {
		case Token_as: {
			ssa_emit_comment(proc, make_string("Cast - as"));
			// NOTE(bill): Needed for dereference of pointer conversion
			Type *type = type_of_expr(proc->module->info, expr);
			ssaValue *v = ssa_add_local_generated(proc, type);
			ssa_emit_store(proc, v, ssa_emit_conv(proc, ssa_build_expr(proc, be->left), type));
			return ssa_make_addr(v, expr);
		}
		case Token_transmute: {
			ssa_emit_comment(proc, make_string("Cast - transmute"));
			// NOTE(bill): Needed for dereference of pointer conversion
			Type *type = type_of_expr(proc->module->info, expr);
			ssaValue *v = ssa_add_local_generated(proc, type);
			ssa_emit_store(proc, v, ssa_emit_transmute(proc, ssa_build_expr(proc, be->left), type));
			return ssa_make_addr(v, expr);
		}
		default:
			GB_PANIC("Invalid binary expression for ssa_build_addr: %.*s\n", LIT(be->op.string));
			break;
		}
	case_end;

	case_ast_node(ie, IndexExpr, expr);
		ssa_emit_comment(proc, make_string("IndexExpr"));
		Type *t = base_type(type_of_expr(proc->module->info, ie->expr));
		gbAllocator a = proc->module->allocator;


		b32 deref = is_type_pointer(t);
		t = type_deref(t);

		ssaValue *using_addr = NULL;
		if (!is_type_indexable(t)) {
			// Using index expression
			Entity *using_field = find_using_index_expr(t);
			if (using_field != NULL) {
				Selection sel = lookup_field(a, t, using_field->token.string, false);
				ssaValue *e = ssa_build_addr(proc, ie->expr).addr;
				using_addr = ssa_emit_deep_field_gep(proc, t, e, sel);

				t = using_field->type;
			}
		}


		switch (t->kind) {
		case Type_Vector: {
			ssaValue *vector = NULL;
			if (using_addr != NULL) {
				vector = using_addr;
			} else {
				vector = ssa_build_addr(proc, ie->expr).addr;
				if (deref) {
					vector = ssa_emit_load(proc, vector);
				}
			}
			ssaValue *index = ssa_emit_conv(proc, ssa_build_expr(proc, ie->index), t_int);
			ssaValue *len = ssa_make_const_int(a, t->Vector.count);
			ssa_array_bounds_check(proc, ast_node_token(ie->index), index, len);
			return ssa_make_addr_vector(vector, index, expr);
		} break;

		case Type_Array: {
			ssaValue *array = NULL;
			if (using_addr != NULL) {
				array = using_addr;
			} else {
				array = ssa_build_addr(proc, ie->expr).addr;
				if (deref) {
					array = ssa_emit_load(proc, array);
				}
			}
			Type *et = make_type_pointer(proc->module->allocator, t->Array.elem);
			ssaValue *index = ssa_emit_conv(proc, ssa_build_expr(proc, ie->index), t_int);
			ssaValue *elem = ssa_emit_struct_gep(proc, array, index, et);
			ssaValue *len = ssa_make_const_int(a, t->Vector.count);
			ssa_array_bounds_check(proc, ast_node_token(ie->index), index, len);
			return ssa_make_addr(elem, expr);
		} break;

		case Type_Slice: {
			ssaValue *slice = NULL;
			if (using_addr != NULL) {
				slice = ssa_emit_load(proc, using_addr);
			} else {
				slice = ssa_build_expr(proc, ie->expr);
				if (deref) {
					slice = ssa_emit_load(proc, slice);
				}
			}
			ssaValue *elem = ssa_slice_elem(proc, slice);
			ssaValue *len = ssa_slice_len(proc, slice);
			ssaValue *index = ssa_emit_conv(proc, ssa_build_expr(proc, ie->index), t_int);
			ssa_array_bounds_check(proc, ast_node_token(ie->index), index, len);
			ssaValue *v = ssa_emit_ptr_offset(proc, elem, index);
			return ssa_make_addr(v, expr);

		} break;

		case Type_Basic: { // Basic_string
			TypeAndValue *tv = map_get(&proc->module->info->types, hash_pointer(ie->expr));
			ssaValue *elem = NULL;
			ssaValue *len = NULL;
			if (tv != NULL && tv->mode == Addressing_Constant) {
				ssaValue *array = ssa_add_global_string_array(proc->module, tv->value.value_string);
				elem = ssa_array_elem(proc, array);
				len = ssa_make_const_int(a, tv->value.value_string.len);
			} else {
				ssaValue *str = NULL;
				if (using_addr != NULL) {
					str = ssa_emit_load(proc, using_addr);
				} else {
					str = ssa_build_expr(proc, ie->expr);
					if (deref) {
						str = ssa_emit_load(proc, str);
					}
				}
				elem = ssa_string_elem(proc, str);
				len = ssa_string_len(proc, str);
			}

			ssaValue *index = ssa_emit_conv(proc, ssa_build_expr(proc, ie->index), t_int);
			ssa_array_bounds_check(proc, ast_node_token(ie->index), index, len);

			ssaValue *v = ssa_emit_ptr_offset(proc, elem, index);
			return ssa_make_addr(v, expr);
		} break;
		}
	case_end;

	case_ast_node(se, SliceExpr, expr);
		ssa_emit_comment(proc, make_string("SliceExpr"));
		gbAllocator a = proc->module->allocator;
		ssaValue *low  = v_zero;
		ssaValue *high = NULL;
		ssaValue *max  = NULL;

		if (se->low  != NULL)    low  = ssa_build_expr(proc, se->low);
		if (se->high != NULL)    high = ssa_build_expr(proc, se->high);
		if (se->triple_indexed)  max  = ssa_build_expr(proc, se->max);
		ssaValue *addr = ssa_build_addr(proc, se->expr).addr;
		ssaValue *base = ssa_emit_load(proc, addr);
		Type *type = base_type(ssa_type(base));

		if (is_type_pointer(type)) {
			type = type_deref(type);
			addr = base;
			base = ssa_emit_load(proc, base);
		}

		// TODO(bill): Cleanup like mad!

		switch (type->kind) {
		case Type_Slice: {
			Type *slice_type = type;

			if (high == NULL) high = ssa_slice_len(proc, base);
			if (max == NULL)  max  = ssa_slice_cap(proc, base);
			GB_ASSERT(max != NULL);

			ssa_slice_bounds_check(proc, se->open, low, high, max, false);

			ssaValue *elem = ssa_slice_elem(proc, base);
			ssaValue *len  = ssa_emit_arith(proc, Token_Sub, high, low, t_int);
			ssaValue *cap  = ssa_emit_arith(proc, Token_Sub, max,  low, t_int);
			ssaValue *slice = ssa_add_local_generated(proc, slice_type);

			ssaValue *gep0 = ssa_emit_struct_gep(proc, slice, v_zero32, ssa_type(elem));
			ssaValue *gep1 = ssa_emit_struct_gep(proc, slice, v_one32, t_int);
			ssaValue *gep2 = ssa_emit_struct_gep(proc, slice, v_two32, t_int);
			ssa_emit_store(proc, gep0, elem);
			ssa_emit_store(proc, gep1, len);
			ssa_emit_store(proc, gep2, cap);

			return ssa_make_addr(slice, expr);
		}

		case Type_Array: {
			Type *slice_type = make_type_slice(a, type->Array.elem);

			if (high == NULL) high = ssa_array_len(proc, base);
			if (max == NULL)  max  = ssa_array_cap(proc, base);
			GB_ASSERT(max != NULL);

			ssa_slice_bounds_check(proc, se->open, low, high, max, false);

			ssaValue *elem = ssa_array_elem(proc, addr);
			ssaValue *len  = ssa_emit_arith(proc, Token_Sub, high, low, t_int);
			ssaValue *cap  = ssa_emit_arith(proc, Token_Sub, max,  low, t_int);
			ssaValue *slice = ssa_add_local_generated(proc, slice_type);

			ssaValue *gep0 = ssa_emit_struct_gep(proc, slice, v_zero32, ssa_type(elem));
			ssaValue *gep1 = ssa_emit_struct_gep(proc, slice, v_one32, t_int);
			ssaValue *gep2 = ssa_emit_struct_gep(proc, slice, v_two32, t_int);
			ssa_emit_store(proc, gep0, elem);
			ssa_emit_store(proc, gep1, len);
			ssa_emit_store(proc, gep2, cap);

			return ssa_make_addr(slice, expr);
		}

		case Type_Basic: {
			GB_ASSERT(type == t_string);
			if (high == NULL) {
				high = ssa_string_len(proc, base);
			}

			ssa_slice_bounds_check(proc, se->open, low, high, high, true);

			ssaValue *elem, *len;
			len = ssa_emit_arith(proc, Token_Sub, high, low, t_int);

			elem = ssa_string_elem(proc, base);
			elem = ssa_emit_ptr_offset(proc, elem, low);

			ssaValue *str = ssa_add_local_generated(proc, t_string);
			ssaValue *gep0 = ssa_emit_struct_gep(proc, str, v_zero32, ssa_type(elem));
			ssaValue *gep1 = ssa_emit_struct_gep(proc, str, v_one32, t_int);
			ssa_emit_store(proc, gep0, elem);
			ssa_emit_store(proc, gep1, len);

			return ssa_make_addr(str, expr);
		} break;
		}

		GB_PANIC("Unknown slicable type");
	case_end;

	case_ast_node(de, DerefExpr, expr);
		ssaValue *e = ssa_build_expr(proc, de->expr);
		// TODO(bill): Is a ptr copy needed?
		ssaValue *gep = ssa_emit_zero_gep(proc, e);
		return ssa_make_addr(gep, expr);
	case_end;

	case_ast_node(de, DemaybeExpr, expr);
		ssa_emit_comment(proc, make_string("DemaybeExpr"));
		ssaValue *maybe = ssa_build_expr(proc, de->expr);
		Type *t = default_type(type_of_expr(proc->module->info, expr));
		GB_ASSERT(is_type_tuple(t));

		Type *elem = ssa_type(maybe);
		GB_ASSERT(is_type_maybe(elem));
		elem = base_type(elem)->Maybe.elem;

		ssaValue *result = ssa_add_local_generated(proc, t);
		ssaValue *gep0 = ssa_emit_struct_gep(proc, result, v_zero32, make_type_pointer(proc->module->allocator, elem));
		ssaValue *gep1 = ssa_emit_struct_gep(proc, result, v_one32,  make_type_pointer(proc->module->allocator, t_bool));
		ssa_emit_store(proc, gep0, ssa_emit_struct_ev(proc, maybe, 0, elem));
		ssa_emit_store(proc, gep1, ssa_emit_struct_ev(proc, maybe, 1, t_bool));

		return ssa_make_addr(result, expr);
	case_end;

	case_ast_node(ce, CallExpr, expr);
		ssaValue *e = ssa_build_expr(proc, expr);
		ssaValue *v = ssa_add_local_generated(proc, ssa_type(e));
		ssa_emit_store(proc, v, e);
		return ssa_make_addr(v, expr);
	case_end;
	}

	TokenPos token_pos = ast_node_token(expr).pos;
	GB_PANIC("Unexpected address expression\n"
	         "\tAstNode: %.*s @ "
	         "%.*s(%td:%td)\n",
	         LIT(ast_node_strings[expr->kind]),
	         LIT(token_pos.file), token_pos.line, token_pos.column);


	return ssa_make_addr(NULL, NULL);
}

void ssa_build_assign_op(ssaProcedure *proc, ssaAddr lhs, ssaValue *value, TokenKind op) {
	ssaValue *old_value = ssa_lvalue_load(proc, lhs);
	Type *type = ssa_type(old_value);

	ssaValue *change = value;
	if (is_type_pointer(type) && is_type_integer(ssa_type(value))) {
		change = ssa_emit_conv(proc, value, default_type(ssa_type(value)));
	} else {
		change = ssa_emit_conv(proc, value, type);
	}
	ssaValue *new_value = ssa_emit_arith(proc, op, old_value, change, type);
	ssa_lvalue_store(proc, lhs, new_value);
}

void ssa_build_cond(ssaProcedure *proc, AstNode *cond, ssaBlock *true_block, ssaBlock *false_block) {
	switch (cond->kind) {
	case_ast_node(pe, ParenExpr, cond);
		ssa_build_cond(proc, pe->expr, true_block, false_block);
		return;
	case_end;

	case_ast_node(ue, UnaryExpr, cond);
		if (ue->op.kind == Token_Not) {
			ssa_build_cond(proc, ue->expr, false_block, true_block);
			return;
		}
	case_end;

	case_ast_node(be, BinaryExpr, cond);
		if (be->op.kind == Token_CmpAnd) {
			ssaBlock *block = ssa_add_block(proc, NULL, "cmp.and");
			ssa_build_cond(proc, be->left, block, false_block);
			proc->curr_block = block;
			ssa_build_cond(proc, be->right, true_block, false_block);
			return;
		} else if (be->op.kind == Token_CmpOr) {
			ssaBlock *block = ssa_add_block(proc, NULL, "cmp.or");
			ssa_build_cond(proc, be->left, true_block, block);
			proc->curr_block = block;
			ssa_build_cond(proc, be->right, true_block, false_block);
			return;
		}
	case_end;
	}

	ssaValue *expr = ssa_build_expr(proc, cond);
	expr = ssa_emit_conv(proc, expr, t_bool);
	ssa_emit_if(proc, expr, true_block, false_block);
}

void ssa_gen_global_type_name(ssaModule *m, Entity *e, String name);
void ssa_mangle_sub_type_name(ssaModule *m, Entity *field, String parent) {
	if (field->kind != Entity_TypeName) {
		return;
	}
	auto *tn = &field->type->Named;
	String cn = field->token.string;

	isize len = parent.len + 1 + cn.len;
	String child = {NULL, len};
	child.text = gb_alloc_array(m->allocator, u8, len);

	isize i = 0;
	gb_memcopy(child.text+i, parent.text, parent.len);
	i += parent.len;
	child.text[i++] = '.';
	gb_memcopy(child.text+i, cn.text, cn.len);

	map_set(&m->type_names, hash_pointer(field->type), child);
	ssa_gen_global_type_name(m, field, child);
}

void ssa_gen_global_type_name(ssaModule *m, Entity *e, String name) {
	ssaValue *t = ssa_make_value_type_name(m->allocator, name, e->type);
	ssa_module_add_value(m, e, t);
	map_set(&m->members, hash_string(name), t);

	Type *bt = base_type(e->type);
	if (bt->kind == Type_Record) {
		auto *s = &bt->Record;
		for (isize j = 0; j < s->other_field_count; j++) {
			ssa_mangle_sub_type_name(m, s->other_fields[j], name);
		}
	}

	if (is_type_union(bt)) {
		auto *s = &bt->Record;
		// NOTE(bill): Zeroth entry is null (for `match type` stmts)
		for (isize j = 1; j < s->field_count; j++) {
			ssa_mangle_sub_type_name(m, s->fields[j], name);
		}
	}
}



void ssa_build_stmt_list(ssaProcedure *proc, AstNodeArray stmts) {
	for_array(i, stmts) {
		ssa_build_stmt(proc, stmts[i]);
	}
}

void ssa_build_stmt(ssaProcedure *proc, AstNode *node) {
	u32 prev_stmt_state_flags = proc->module->stmt_state_flags;
	defer (proc->module->stmt_state_flags = prev_stmt_state_flags);

	if (node->stmt_state_flags != 0) {
		u32 in = node->stmt_state_flags;
		u32 out = proc->module->stmt_state_flags;
		defer (proc->module->stmt_state_flags = out);

		if (in & StmtStateFlag_bounds_check) {
			out |= StmtStateFlag_bounds_check;
			out &= ~StmtStateFlag_no_bounds_check;
		} else if (in & StmtStateFlag_no_bounds_check) {
			out |= StmtStateFlag_no_bounds_check;
			out &= ~StmtStateFlag_bounds_check;
		}
	}


	switch (node->kind) {
	case_ast_node(bs, EmptyStmt, node);
	case_end;

	case_ast_node(us, UsingStmt, node);
		AstNode *decl = unparen_expr(us->node);
		if (decl->kind == AstNode_VarDecl) {
			ssa_build_stmt(proc, decl);
		}
	case_end;

	case_ast_node(vd, VarDecl, node);
		ssaModule *m = proc->module;
		gbTempArenaMemory tmp = gb_temp_arena_memory_begin(&m->tmp_arena);
		defer (gb_temp_arena_memory_end(tmp));

		if (vd->values.count == 0) { // declared and zero-initialized
			for_array(i, vd->names) {
				AstNode *name = vd->names[i];
				if (!ssa_is_blank_ident(name)) {
					ssa_add_local_for_identifier(proc, name, true);
				}
			}
		} else { // Tuple(s)
			Array<ssaAddr>  lvals;
			Array<ssaValue *> inits;
			array_init(&lvals, m->tmp_allocator, vd->names.count);
			array_init(&inits, m->tmp_allocator, vd->names.count);

			for_array(i, vd->names) {
				AstNode *name = vd->names[i];
				ssaAddr lval = ssa_make_addr(NULL, NULL);
				if (!ssa_is_blank_ident(name)) {
					ssa_add_local_for_identifier(proc, name, false);
					lval = ssa_build_addr(proc, name);
				}

				array_add(&lvals, lval);
			}

			for_array(i, vd->values) {
				ssaValue *init = ssa_build_expr(proc, vd->values[i]);
				Type *t = ssa_type(init);
				if (t->kind == Type_Tuple) {
					for (isize i = 0; i < t->Tuple.variable_count; i++) {
						Entity *e = t->Tuple.variables[i];
						ssaValue *v = ssa_emit_struct_ev(proc, init, i, e->type);
						array_add(&inits, v);
					}
				} else {
					array_add(&inits, init);
				}
			}


			for_array(i, inits) {
				ssaValue *v = ssa_emit_conv(proc, inits[i], ssa_addr_type(lvals[i]));
				ssa_lvalue_store(proc, lvals[i], v);
			}
		}
	case_end;

	case_ast_node(pd, ProcDecl, node);
		if (pd->body != NULL) {
			auto *info = proc->module->info;

			Entity **found = map_get(&info->definitions, hash_pointer(pd->name));
			GB_ASSERT_MSG(found != NULL, "Unable to find: %.*s", LIT(pd->name->Ident.string));
			Entity *e = *found;

			// NOTE(bill): Generate a new name
			// parent.name-guid
			String original_name = pd->name->Ident.string;
			String pd_name = original_name;
			if (pd->link_name.len > 0) {
				pd_name = pd->link_name;
			}

			isize name_len = proc->name.len + 1 + pd_name.len + 1 + 10 + 1;
			u8 *name_text = gb_alloc_array(proc->module->allocator, u8, name_len);
			i32 guid = cast(i32)proc->children.count;
			name_len = gb_snprintf(cast(char *)name_text, name_len, "%.*s.%.*s-%d", LIT(proc->name), LIT(pd_name), guid);
			String name = make_string(name_text, name_len-1);


			ssaValue *value = ssa_make_value_procedure(proc->module->allocator,
			                                           proc->module, e, e->type, pd->type, pd->body, name);

			value->Proc.tags = pd->tags;
			value->Proc.parent = proc;

			ssa_module_add_value(proc->module, e, value);
			array_add(&proc->children, &value->Proc);
			array_add(&proc->module->procs, value);
		} else {
			auto *info = proc->module->info;

			Entity **found = map_get(&info->definitions, hash_pointer(pd->name));
			GB_ASSERT_MSG(found != NULL, "Unable to find: %.*s", LIT(pd->name->Ident.string));
			Entity *e = *found;

			// FFI - Foreign function interace
			String original_name = pd->name->Ident.string;
			String name = original_name;
			if (pd->foreign_name.len > 0) {
				name = pd->foreign_name;
			}

			ssaValue *value = ssa_make_value_procedure(proc->module->allocator,
			                                           proc->module, e, e->type, pd->type, pd->body, name);

			value->Proc.tags = pd->tags;

			ssa_module_add_value(proc->module, e, value);
			ssa_build_proc(value, proc);

			if (value->Proc.tags & ProcTag_foreign) {
				HashKey key = hash_string(name);
				auto *prev_value = map_get(&proc->module->members, key);
				if (prev_value == NULL) {
					// NOTE(bill): Don't do mutliple declarations in the IR
					map_set(&proc->module->members, key, value);
				}
			} else {
				array_add(&proc->children, &value->Proc);
			}
		}
	case_end;

	case_ast_node(td, TypeDecl, node);

		// NOTE(bill): Generate a new name
		// parent_proc.name-guid
		String td_name = td->name->Ident.string;
		isize name_len = proc->name.len + 1 + td_name.len + 1 + 10 + 1;
		u8 *name_text = gb_alloc_array(proc->module->allocator, u8, name_len);
		i32 guid = cast(i32)proc->module->members.entries.count;
		name_len = gb_snprintf(cast(char *)name_text, name_len, "%.*s.%.*s-%d", LIT(proc->name), LIT(td_name), guid);
		String name = make_string(name_text, name_len-1);

		Entity **found = map_get(&proc->module->info->definitions, hash_pointer(td->name));
		GB_ASSERT(found != NULL);
		Entity *e = *found;
		ssaValue *value = ssa_make_value_type_name(proc->module->allocator,
		                                           name, e->type);
		map_set(&proc->module->type_names, hash_pointer(e->type), name);
		ssa_gen_global_type_name(proc->module, e, name);
	case_end;

	case_ast_node(ids, IncDecStmt, node);
		ssa_emit_comment(proc, make_string("IncDecStmt"));
		TokenKind op = ids->op.kind;
		if (op == Token_Increment) {
			op = Token_Add;
		} else if (op == Token_Decrement) {
			op = Token_Sub;
		}
		ssaAddr lval = ssa_build_addr(proc, ids->expr);
		ssaValue *one = ssa_emit_conv(proc, v_one, ssa_addr_type(lval));
		ssa_build_assign_op(proc, lval, one, op);

	case_end;

	case_ast_node(as, AssignStmt, node);
		ssa_emit_comment(proc, make_string("AssignStmt"));

		ssaModule *m = proc->module;
		gbTempArenaMemory tmp = gb_temp_arena_memory_begin(&m->tmp_arena);
		defer (gb_temp_arena_memory_end(tmp));

		switch (as->op.kind) {
		case Token_Eq: {
			Array<ssaAddr> lvals;
			array_init(&lvals, m->tmp_allocator);

			for_array(i, as->lhs) {
				AstNode *lhs = as->lhs[i];
				ssaAddr lval = {};
				if (!ssa_is_blank_ident(lhs)) {
					lval = ssa_build_addr(proc, lhs);
				}
				array_add(&lvals, lval);
			}

			if (as->lhs.count == as->rhs.count) {
				if (as->lhs.count == 1) {
					AstNode *rhs = as->rhs[0];
					ssaValue *init = ssa_build_expr(proc, rhs);
					ssa_lvalue_store(proc, lvals[0], init);
				} else {
					Array<ssaValue *> inits;
					array_init(&inits, m->tmp_allocator, lvals.count);

					for_array(i, as->rhs) {
						ssaValue *init = ssa_build_expr(proc, as->rhs[i]);
						array_add(&inits, init);
					}

					for_array(i, inits) {
						ssa_lvalue_store(proc, lvals[i], inits[i]);
					}
				}
			} else {
				Array<ssaValue *> inits;
				array_init(&inits, m->tmp_allocator, lvals.count);

				for_array(i, as->rhs) {
					ssaValue *init = ssa_build_expr(proc, as->rhs[i]);
					Type *t = ssa_type(init);
					// TODO(bill): refactor for code reuse as this is repeated a bit
					if (t->kind == Type_Tuple) {
						for (isize i = 0; i < t->Tuple.variable_count; i++) {
							Entity *e = t->Tuple.variables[i];
							ssaValue *v = ssa_emit_struct_ev(proc, init, i, e->type);
							array_add(&inits, v);
						}
					} else {
						array_add(&inits, init);
					}
				}

				for_array(i, inits) {
					ssa_lvalue_store(proc, lvals[i], inits[i]);
				}
			}

		} break;

		default: {
			// NOTE(bill): Only 1 += 1 is allowed, no tuples
			// +=, -=, etc
			i32 op = cast(i32)as->op.kind;
			op += Token_Add - Token_AddEq; // Convert += to +
			ssaAddr lhs = ssa_build_addr(proc, as->lhs[0]);
			ssaValue *value = ssa_build_expr(proc, as->rhs[0]);
			ssa_build_assign_op(proc, lhs, value, cast(TokenKind)op);
		} break;
		}
	case_end;

	case_ast_node(es, ExprStmt, node);
		// NOTE(bill): No need to use return value
		ssa_build_expr(proc, es->expr);
	case_end;

	case_ast_node(bs, BlockStmt, node);
		ssa_open_scope(proc);
		ssa_build_stmt_list(proc, bs->stmts);
		ssa_close_scope(proc, ssaDeferExit_Default, NULL);
	case_end;

	case_ast_node(ds, DeferStmt, node);
		ssa_emit_comment(proc, make_string("DeferStmt"));
		isize scope_index = proc->scope_index;
		if (ds->stmt->kind == AstNode_BlockStmt) {
			scope_index--;
		}
		ssa_add_defer_node(proc, scope_index, ds->stmt);
	case_end;

	case_ast_node(rs, ReturnStmt, node);
		ssa_emit_comment(proc, make_string("ReturnStmt"));
		ssaValue *v = NULL;
		auto *return_type_tuple  = &proc->type->Proc.results->Tuple;
		isize return_count = proc->type->Proc.result_count;
		if (return_count == 0) {
			// No return values
		} else if (return_count == 1) {
			Entity *e = return_type_tuple->variables[0];
			v = ssa_emit_conv(proc, ssa_build_expr(proc, rs->results[0]), e->type);
		} else {
			gbTempArenaMemory tmp = gb_temp_arena_memory_begin(&proc->module->tmp_arena);
			defer (gb_temp_arena_memory_end(tmp));

			Array<ssaValue *> results;
			array_init(&results, proc->module->tmp_allocator, return_count);

			for_array(res_index, rs->results) {
				ssaValue *res = ssa_build_expr(proc, rs->results[res_index]);
				Type *t = ssa_type(res);
				if (t->kind == Type_Tuple) {
					for (isize i = 0; i < t->Tuple.variable_count; i++) {
						Entity *e = t->Tuple.variables[i];
						ssaValue *v = ssa_emit_struct_ev(proc, res, i, e->type);
						array_add(&results, v);
					}
				} else {
					array_add(&results, res);
				}
			}

			Type *ret_type = proc->type->Proc.results;
			v = ssa_add_local_generated(proc, ret_type);
			for_array(i, results) {
				Entity *e = return_type_tuple->variables[i];
				ssaValue *res = ssa_emit_conv(proc, results[i], e->type);
				ssaValue *field = ssa_emit_struct_gep(proc, v, i, make_type_pointer(proc->module->allocator, e->type));
				ssa_emit_store(proc, field, res);
			}

			v = ssa_emit_load(proc, v);

		}
		ssa_emit_ret(proc, v);

	case_end;

	case_ast_node(is, IfStmt, node);
		ssa_emit_comment(proc, make_string("IfStmt"));
		if (is->init != NULL) {
			ssaBlock *init = ssa_add_block(proc, node, "if.init");
			ssa_emit_jump(proc, init);
			proc->curr_block = init;
			ssa_build_stmt(proc, is->init);
		}
		ssaBlock *then = ssa_add_block(proc, node, "if.then");
		ssaBlock *done = ssa_add_block(proc, node, "if.done"); // NOTE(bill): Append later
		ssaBlock *else_ = done;
		if (is->else_stmt != NULL) {
			else_ = ssa_add_block(proc, is->else_stmt, "if.else");
		}

		ssa_build_cond(proc, is->cond, then, else_);
		proc->curr_block = then;

		ssa_open_scope(proc);
		ssa_build_stmt(proc, is->body);
		ssa_close_scope(proc, ssaDeferExit_Default, NULL);

		ssa_emit_jump(proc, done);

		if (is->else_stmt != NULL) {
			proc->curr_block = else_;

			ssa_open_scope(proc);
			ssa_build_stmt(proc, is->else_stmt);
			ssa_close_scope(proc, ssaDeferExit_Default, NULL);

			ssa_emit_jump(proc, done);
		}
		proc->curr_block = done;
	case_end;

	case_ast_node(fs, ForStmt, node);
		ssa_emit_comment(proc, make_string("ForStmt"));
		if (fs->init != NULL) {
			ssaBlock *init = ssa_add_block(proc, node, "for.init");
			ssa_emit_jump(proc, init);
			proc->curr_block = init;
			ssa_build_stmt(proc, fs->init);
		}
		ssaBlock *body = ssa_add_block(proc, node, "for.body");
		ssaBlock *done = ssa_add_block(proc, node, "for.done"); // NOTE(bill): Append later

		ssaBlock *loop = body;

		if (fs->cond != NULL) {
			loop = ssa_add_block(proc, node, "for.loop");
		}
		ssaBlock *cont = loop;
		if (fs->post != NULL) {
			cont = ssa_add_block(proc, node, "for.post");

		}
		ssa_emit_jump(proc, loop);
		proc->curr_block = loop;
		if (loop != body) {
			ssa_build_cond(proc, fs->cond, body, done);
			proc->curr_block = body;
		}

		ssa_push_target_list(proc, done, cont, NULL);

		ssa_open_scope(proc);
		ssa_build_stmt(proc, fs->body);
		ssa_close_scope(proc, ssaDeferExit_Default, NULL);

		ssa_pop_target_list(proc);
		ssa_emit_jump(proc, cont);

		if (fs->post != NULL) {
			proc->curr_block = cont;
			ssa_build_stmt(proc, fs->post);
			ssa_emit_jump(proc, loop);
		}


		proc->curr_block = done;

	case_end;

	case_ast_node(ms, MatchStmt, node);
		ssa_emit_comment(proc, make_string("MatchStmt"));
		if (ms->init != NULL) {
			ssa_build_stmt(proc, ms->init);
		}
		ssaValue *tag = v_true;
		if (ms->tag != NULL) {
			tag = ssa_build_expr(proc, ms->tag);
		}
		ssaBlock *done = ssa_add_block(proc, node, "match.done"); // NOTE(bill): Append later

		ast_node(body, BlockStmt, ms->body);

		AstNodeArray default_stmts = {};
		ssaBlock *default_fall = NULL;
		ssaBlock *default_block = NULL;

		ssaBlock *fall = NULL;
		b32 append_fall = false;

		isize case_count = body->stmts.count;
		for_array(i, body->stmts) {
			AstNode *clause = body->stmts[i];
			ssaBlock *body = fall;

			ast_node(cc, CaseClause, clause);

			if (body == NULL) {
				if (cc->list.count == 0) {
					body = ssa_add_block(proc, clause, "match.dflt.body");
				} else {
					body = ssa_add_block(proc, clause, "match.case.body");
				}
			}
			if (append_fall && body == fall) {
				append_fall = false;
			}

			fall = done;
			if (i+1 < case_count) {
				append_fall = true;
				fall = ssa_add_block(proc, clause, "match.fall.body");
			}

			if (cc->list.count == 0) {
				// default case
				default_stmts = cc->stmts;
				default_fall  = fall;
				default_block = body;
				continue;
			}

			ssaBlock *next_cond = NULL;
			for_array(j, cc->list) {
				AstNode *expr = cc->list[j];
				next_cond = ssa_add_block(proc, clause, "match.case.next");

				ssaValue *cond = ssa_emit_comp(proc, Token_CmpEq, tag, ssa_build_expr(proc, expr));
				ssa_emit_if(proc, cond, body, next_cond);
				proc->curr_block = next_cond;
			}
			proc->curr_block = body;

			ssa_push_target_list(proc, done, NULL, fall);
			ssa_open_scope(proc);
			ssa_build_stmt_list(proc, cc->stmts);
			ssa_close_scope(proc, ssaDeferExit_Default, body);
			ssa_pop_target_list(proc);

			ssa_emit_jump(proc, done);
			proc->curr_block = next_cond;
		}

		if (default_block != NULL) {
			ssa_emit_jump(proc, default_block);
			proc->curr_block = default_block;

			ssa_push_target_list(proc, done, NULL, default_fall);
			ssa_open_scope(proc);
			ssa_build_stmt_list(proc, default_stmts);
			ssa_close_scope(proc, ssaDeferExit_Default, default_block);
			ssa_pop_target_list(proc);
		}

		ssa_emit_jump(proc, done);
		proc->curr_block = done;
	case_end;


	case_ast_node(ms, TypeMatchStmt, node);
		ssa_emit_comment(proc, make_string("TypeMatchStmt"));
		gbAllocator allocator = proc->module->allocator;

		ssaValue *parent = ssa_build_expr(proc, ms->tag);
		Type *union_type = type_deref(ssa_type(parent));
		GB_ASSERT(is_type_union(union_type));

		ssa_emit_comment(proc, make_string("get union's tag"));
		ssaValue *tag_index = ssa_emit_struct_gep(proc, parent, v_one32, make_type_pointer(allocator, t_int));
		tag_index = ssa_emit_load(proc, tag_index);

		ssaValue *data = ssa_emit_conv(proc, parent, t_rawptr);

		ssaBlock *start_block = ssa_add_block(proc, node, "type-match.case.first");
		ssa_emit_jump(proc, start_block);
		proc->curr_block = start_block;

		ssaBlock *done = ssa_add_block(proc, node, "type-match.done"); // NOTE(bill): Append later

		ast_node(body, BlockStmt, ms->body);


		String tag_var_name = ms->var->Ident.string;

		AstNodeArray default_stmts = {};
		ssaBlock *default_block = NULL;

		isize case_count = body->stmts.count;
		for_array(i, body->stmts) {
			AstNode *clause = body->stmts[i];
			ast_node(cc, CaseClause, clause);

			if (cc->list.count == 0) {
				// default case
				default_stmts = cc->stmts;
				default_block = ssa_add_block(proc, clause, "type-match.dflt.body");
				continue;
			}


			ssaBlock *body = ssa_add_block(proc, clause, "type-match.case.body");

			Scope *scope = *map_get(&proc->module->info->scopes, hash_pointer(clause));
			Entity *tag_var_entity = current_scope_lookup_entity(scope, tag_var_name);
			GB_ASSERT_MSG(tag_var_entity != NULL, "%.*s", LIT(tag_var_name));
			ssaValue *tag_var = ssa_add_local(proc, tag_var_entity);
			ssaValue *data_ptr = ssa_emit_conv(proc, data, tag_var_entity->type);
			ssa_emit_store(proc, tag_var, data_ptr);



			Type *bt = type_deref(tag_var_entity->type);
			ssaValue *index = NULL;
			Type *ut = base_type(union_type);
			GB_ASSERT(ut->Record.kind == TypeRecord_Union);
			for (isize field_index = 1; field_index < ut->Record.field_count; field_index++) {
				Entity *f = base_type(union_type)->Record.fields[field_index];
				if (are_types_identical(f->type, bt)) {
					index = ssa_make_const_int(allocator, field_index);
					break;
				}
			}
			GB_ASSERT(index != NULL);

			ssaBlock *next_cond = ssa_add_block(proc, clause, "type-match.case.next");
			ssaValue *cond = ssa_emit_comp(proc, Token_CmpEq, tag_index, index);
			ssa_emit_if(proc, cond, body, next_cond);
			proc->curr_block = next_cond;

			proc->curr_block = body;

			ssa_push_target_list(proc, done, NULL, NULL);
			ssa_open_scope(proc);
			ssa_build_stmt_list(proc, cc->stmts);
			ssa_close_scope(proc, ssaDeferExit_Default, body);
			ssa_pop_target_list(proc);

			ssa_emit_jump(proc, done);
			proc->curr_block = next_cond;
		}

		if (default_block != NULL) {
			ssa_emit_jump(proc, default_block);
			proc->curr_block = default_block;

			ssa_push_target_list(proc, done, NULL, NULL);
			ssa_open_scope(proc);
			ssa_build_stmt_list(proc, default_stmts);
			ssa_close_scope(proc, ssaDeferExit_Default, default_block);
			ssa_pop_target_list(proc);
		}

		ssa_emit_jump(proc, done);
		proc->curr_block = done;
	case_end;

	case_ast_node(bs, BranchStmt, node);
		ssaBlock *block = NULL;
		switch (bs->token.kind) {
		case Token_break:
			for (ssaTargetList *t = proc->target_list; t != NULL && block == NULL; t = t->prev) {
				block = t->break_;
			}
			break;
		case Token_continue:
			for (ssaTargetList *t = proc->target_list; t != NULL && block == NULL; t = t->prev) {
				block = t->continue_;
			}
			break;
		case Token_fallthrough:
			for (ssaTargetList *t = proc->target_list; t != NULL && block == NULL; t = t->prev) {
				block = t->fallthrough_;
			}
			break;
		}
		// TODO(bill): Handle fallthrough scope exit correctly
		// if (block != NULL && bs->token.kind != Token_fallthrough) {
		if (block != NULL) {
			ssa_emit_defer_stmts(proc, ssaDeferExit_Branch, block);
		}
		switch (bs->token.kind) {
		case Token_break:       ssa_emit_comment(proc, make_string("break"));       break;
		case Token_continue:    ssa_emit_comment(proc, make_string("continue"));    break;
		case Token_fallthrough: ssa_emit_comment(proc, make_string("fallthrough")); break;
		}
		ssa_emit_jump(proc, block);
		ssa_emit_unreachable(proc);
	case_end;



	case_ast_node(pa, PushAllocator, node);
		ssa_emit_comment(proc, make_string("PushAllocator"));
		ssa_open_scope(proc);
		defer (ssa_close_scope(proc, ssaDeferExit_Default, NULL));

		ssaValue *context_ptr = ssa_find_global_variable(proc, make_string("__context"));
		ssaValue *prev_context = ssa_add_local_generated(proc, t_context);
		ssa_emit_store(proc, prev_context, ssa_emit_load(proc, context_ptr));

		ssa_add_defer_instr(proc, proc->scope_index, ssa_make_instr_store(proc, context_ptr, ssa_emit_load(proc, prev_context)));

		ssaValue *gep = ssa_emit_struct_gep(proc, context_ptr, 1, t_allocator_ptr);
		ssa_emit_store(proc, gep, ssa_build_expr(proc, pa->expr));

		ssa_build_stmt(proc, pa->body);

	case_end;


	case_ast_node(pa, PushContext, node);
		ssa_emit_comment(proc, make_string("PushContext"));
		ssa_open_scope(proc);
		defer (ssa_close_scope(proc, ssaDeferExit_Default, NULL));

		ssaValue *context_ptr = ssa_find_global_variable(proc, make_string("__context"));
		ssaValue *prev_context = ssa_add_local_generated(proc, t_context);
		ssa_emit_store(proc, prev_context, ssa_emit_load(proc, context_ptr));

		ssa_add_defer_instr(proc, proc->scope_index, ssa_make_instr_store(proc, context_ptr, ssa_emit_load(proc, prev_context)));

		ssa_emit_store(proc, context_ptr, ssa_build_expr(proc, pa->expr));

		ssa_build_stmt(proc, pa->body);
	case_end;


	}
}


void ssa_emit_startup_runtime(ssaProcedure *proc) {
	GB_ASSERT(proc->parent == NULL && proc->name == "main");

	ssa_emit(proc, ssa_alloc_instr(proc, ssaInstr_StartupRuntime));
}

void ssa_insert_code_before_proc(ssaProcedure* proc, ssaProcedure *parent) {
	if (parent == NULL) {
		if (proc->name == "main") {
			ssa_emit_startup_runtime(proc);
		}
	}
}

void ssa_build_proc(ssaValue *value, ssaProcedure *parent) {
	ssaProcedure *proc = &value->Proc;

	proc->parent = parent;

	if (proc->entity != NULL) {
		ssaModule *m = proc->module;
		CheckerInfo *info = m->info;
		Entity *e = proc->entity;
		String filename = e->token.pos.file;
		AstFile **found = map_get(&info->files, hash_string(filename));
		GB_ASSERT(found != NULL);
		AstFile *f = *found;
		ssaDebugInfo *di_file = NULL;

		ssaDebugInfo **di_file_found = map_get(&m->debug_info, hash_pointer(f));
		if (di_file_found) {
			di_file = *di_file_found;
			GB_ASSERT(di_file->kind == ssaDebugInfo_File);
		} else {
			di_file = ssa_add_debug_info_file(proc, f);
		}

		ssa_add_debug_info_proc(proc, e, proc->name, di_file);
	}

	if (proc->body != NULL) {
		u32 prev_stmt_state_flags = proc->module->stmt_state_flags;
		defer (proc->module->stmt_state_flags = prev_stmt_state_flags);

		if (proc->tags != 0) {
			u32 in = proc->tags;
			u32 out = proc->module->stmt_state_flags;
			defer (proc->module->stmt_state_flags = out);

			if (in & ProcTag_bounds_check) {
				out |= StmtStateFlag_bounds_check;
				out &= ~StmtStateFlag_no_bounds_check;
			} else if (in & ProcTag_no_bounds_check) {
				out |= StmtStateFlag_no_bounds_check;
				out &= ~StmtStateFlag_bounds_check;
			}
		}


		ssa_begin_procedure_body(proc);
		ssa_insert_code_before_proc(proc, parent);
		ssa_build_stmt(proc, proc->body);
		ssa_end_procedure_body(proc);
	}
}






b32 ssa_is_instr_jump(ssaValue *v) {
	if (v->kind != ssaValue_Instr) {
		return false;
	}
	ssaInstr *i = &v->Instr;
	if (i->kind != ssaInstr_Br) {
		return false;
	}

	return i->Br.false_block == NULL;
}

Array<ssaValue *> ssa_get_block_phi_nodes(ssaBlock *b) {
	Array<ssaValue *> phis = {};
	for_array(i, b->instrs) {
		ssaInstr *instr = &b->instrs[i]->Instr;
		if (instr->kind != ssaInstr_Phi) {
			phis = b->instrs;
			phis.count = i;
			return phis;
		}
	}
	return phis;
}

void ssa_remove_pred(ssaBlock *b, ssaBlock *p) {
	auto phis = ssa_get_block_phi_nodes(b);
	isize i = 0;
	for_array(j, b->preds) {
		ssaBlock *pred = b->preds[j];
		if (pred != p) {
			b->preds[i] = b->preds[j];
			for_array(k, phis) {
				auto *phi = &phis[k]->Instr.Phi;
				phi->edges[i] = phi->edges[j];
			}
			i++;
		}
	}
	b->preds.count = i;
	for_array(k, phis) {
		auto *phi = &phis[k]->Instr.Phi;
		phi->edges.count = i;
	}

}

void ssa_remove_dead_blocks(ssaProcedure *proc) {
	isize j = 0;
	for_array(i, proc->blocks) {
		ssaBlock *b = proc->blocks[i];
		if (b == NULL) {
			continue;
		}
		// NOTE(bill): Swap order
		b->index = j;
		proc->blocks[j++] = b;
	}
	proc->blocks.count = j;
}

void ssa_mark_reachable(ssaBlock *b) {
	isize const WHITE =  0;
	isize const BLACK = -1;
	b->index = BLACK;
	for_array(i, b->succs) {
		ssaBlock *succ = b->succs[i];
		if (succ->index == WHITE) {
			ssa_mark_reachable(succ);
		}
	}
}

void ssa_remove_unreachable_blocks(ssaProcedure *proc) {
	isize const WHITE =  0;
	isize const BLACK = -1;
	for_array(i, proc->blocks) {
		proc->blocks[i]->index = WHITE;
	}

	ssa_mark_reachable(proc->blocks[0]);

	for_array(i, proc->blocks) {
		ssaBlock *b = proc->blocks[i];
		if (b->index == WHITE) {
			for_array(j, b->succs) {
				ssaBlock *c = b->succs[j];
				if (c->index == BLACK) {
					ssa_remove_pred(c, b);
				}
			}
			// NOTE(bill): Mark as empty but don't actually free it
			// As it's been allocated with an arena
			proc->blocks[i] = NULL;
		}
	}
	ssa_remove_dead_blocks(proc);
}

b32 ssa_opt_block_fusion(ssaProcedure *proc, ssaBlock *a) {
	if (a->succs.count != 1) {
		return false;
	}
	ssaBlock *b = a->succs[0];
	if (b->preds.count != 1) {
		return false;
	}

	if (ssa_block_has_phi(b)) {
		return false;
	}

	array_pop(&a->instrs); // Remove branch at end
	for_array(i, b->instrs) {
		array_add(&a->instrs, b->instrs[i]);
		ssa_set_instr_parent(b->instrs[i], a);
	}

	array_clear(&a->succs);
	for_array(i, b->succs) {
		array_add(&a->succs, b->succs[i]);
	}

	// Fix preds links
	for_array(i, b->succs) {
		ssa_block_replace_pred(b->succs[i], b, a);
	}

	proc->blocks[b->index] = NULL;
	return true;
}

void ssa_optimize_blocks(ssaProcedure *proc) {
	ssa_remove_unreachable_blocks(proc);

#if 1
	b32 changed = true;
	while (changed) {
		changed = false;
		for_array(i, proc->blocks) {
			ssaBlock *b = proc->blocks[i];
			if (b == NULL) {
				continue;
			}
			GB_ASSERT(b->index == i);

			if (ssa_opt_block_fusion(proc, b)) {
				changed = true;
			}
			// TODO(bill): other simple block optimizations
		}
	}
#endif

	ssa_remove_dead_blocks(proc);
}
void ssa_build_referrers(ssaProcedure *proc) {
	gbTempArenaMemory tmp = gb_temp_arena_memory_begin(&proc->module->tmp_arena);
	defer (gb_temp_arena_memory_end(tmp));

	Array<ssaValue *> ops = {}; // NOTE(bill): Act as a buffer
	array_init(&ops, proc->module->tmp_allocator, 64); // HACK(bill): This _could_ overflow the temp arena
	for_array(i, proc->blocks) {
		ssaBlock *b = proc->blocks[i];
		for_array(j, b->instrs) {
			ssaValue *instr = b->instrs[j];
			array_clear(&ops);
			ssa_add_operands(&ops, &instr->Instr);
			for_array(k, ops) {
				ssaValue *op = ops[k];
				if (op == NULL) {
					continue;
				}
				auto *refs = ssa_value_referrers(op);
				if (refs != NULL) {
					array_add(refs, instr);
				}
			}
		}
	}
}







// State of Lengauer-Tarjan algorithm
// Based on this paper: http://jgaa.info/accepted/2006/GeorgiadisTarjanWerneck2006.10.1.pdf
struct ssaLTState {
	isize count;
	// NOTE(bill): These are arrays
	ssaBlock **sdom;     // Semidominator
	ssaBlock **parent;   // Parent in DFS traversal of CFG
	ssaBlock **ancestor;
};

// §2.2 - bottom of page
void ssa_lt_link(ssaLTState *lt, ssaBlock *p, ssaBlock *q) {
	lt->ancestor[q->index] = p;
}

i32 ssa_lt_depth_first_search(ssaLTState *lt, ssaBlock *p, i32 i, ssaBlock **preorder) {
	preorder[i] = p;
	p->dom.pre = i++;
	lt->sdom[p->index] = p;
	ssa_lt_link(lt, NULL, p);
	for_array(index, p->succs) {
		ssaBlock *q = p->succs[index];
		if (lt->sdom[q->index] == NULL) {
			lt->parent[q->index] = p;
			i = ssa_lt_depth_first_search(lt, q, i, preorder);
		}
	}
	return i;
}

ssaBlock *ssa_lt_eval(ssaLTState *lt, ssaBlock *v) {
	ssaBlock *u = v;
	for (;
	     lt->ancestor[v->index] != NULL;
	     v = lt->ancestor[v->index]) {
		if (lt->sdom[v->index]->dom.pre < lt->sdom[u->index]->dom.pre) {
			u = v;
		}
	}
	return u;
}

void ssa_number_dom_tree(ssaBlock *v, i32 pre, i32 post, i32 *pre_out, i32 *post_out) {
	v->dom.pre = pre++;
	for_array(i, v->dom.children) {
		ssaBlock *child = v->dom.children[i];
		i32 new_pre = 0, new_post = 0;
		ssa_number_dom_tree(child, pre, post, &new_pre, &new_post);
		pre = new_pre;
		post = new_post;
	}
	v->dom.post = post++;
	*pre_out  = pre;
	*post_out = post;
}


// NOTE(bill): Requires `ssa_optimize_blocks` to be called before this
void ssa_build_dom_tree(ssaProcedure *proc) {
	// Based on this paper: http://jgaa.info/accepted/2006/GeorgiadisTarjanWerneck2006.10.1.pdf

	gbTempArenaMemory tmp = gb_temp_arena_memory_begin(&proc->module->tmp_arena);
	defer (gb_temp_arena_memory_end(tmp));

	isize n = proc->blocks.count;
	ssaBlock **buf = gb_alloc_array(proc->module->tmp_allocator, ssaBlock *, 5*n);

	ssaLTState lt = {};
	lt.count    = n;
	lt.sdom     = &buf[0*n];
	lt.parent   = &buf[1*n];
	lt.ancestor = &buf[2*n];

	ssaBlock **preorder = &buf[3*n];
	ssaBlock **buckets  = &buf[4*n];
	ssaBlock *root = proc->blocks[0];

	// Step 1 - number vertices
	i32 pre_num = ssa_lt_depth_first_search(&lt, root, 0, preorder);
	gb_memcopy(buckets, preorder, n*gb_size_of(preorder[0]));

	for (i32 i = n-1; i > 0; i--) {
		ssaBlock *w = preorder[i];

		// Step 3 - Implicitly define idom for nodes
		for (ssaBlock *v = buckets[i]; v != w; v = buckets[v->dom.pre]) {
			ssaBlock *u = ssa_lt_eval(&lt, v);
			if (lt.sdom[u->index]->dom.pre < i) {
				v->dom.idom = u;
			} else {
				v->dom.idom = w;
			}
		}

		// Step 2 - Compute all sdoms
		lt.sdom[w->index] = lt.parent[w->index];
		for_array(pred_index, w->preds) {
			ssaBlock *v = w->preds[pred_index];
			ssaBlock *u = ssa_lt_eval(&lt, v);
			if (lt.sdom[u->index]->dom.pre < lt.sdom[w->index]->dom.pre) {
				lt.sdom[w->index] = lt.sdom[u->index];
			}
		}

		ssa_lt_link(&lt, lt.parent[w->index], w);

		if (lt.parent[w->index] == lt.sdom[w->index]) {
			w->dom.idom = lt.parent[w->index];
		} else {
			buckets[i] = buckets[lt.sdom[w->index]->dom.pre];
			buckets[lt.sdom[w->index]->dom.pre] = w;
		}
	}

	// The rest of Step 3
	for (ssaBlock *v = buckets[0]; v != root; v = buckets[v->dom.pre]) {
		v->dom.idom = root;
	}

	// Step 4 - Explicitly define idom for nodes (in preorder)
	for (isize i = 1; i < n; i++) {
		ssaBlock *w = preorder[i];
		if (w == root) {
			w->dom.idom = NULL;
		} else {
			// Weird tree relationships here!

			if (w->dom.idom != lt.sdom[w->index]) {
				w->dom.idom = w->dom.idom->dom.idom;
			}

			// Calculate children relation as inverse of idom
			auto *children = &w->dom.idom->dom.children;
			if (children->data == NULL) {
				// TODO(bill): Is this good enough for memory allocations?
				array_init(children, gb_heap_allocator());
			}
			array_add(children, w);
		}
	}

	i32 pre = 0;
	i32 pos = 0;
	ssa_number_dom_tree(root, 0, 0, &pre, &pos);
}




void ssa_opt_mem2reg(ssaProcedure *proc) {
	// TODO(bill):
}
