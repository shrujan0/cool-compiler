
//**************************************************************
//
// Code generator SKELETON
//
// Read the comments carefully. Make sure to
//    initialize the base class tags in
//       `CgenClassTable::CgenClassTable'
//
//    Add the label for the dispatch tables to
//       `IntEntry::code_def'
//       `StringEntry::code_def'
//       `BoolConst::code_def'
//
//    Add code to emit everyting else that is needed
//       in `CgenClassTable::code'
//
//
// The files as provided will produce code to begin the code
// segments, declare globals, and emit constants.  You must
// fill in the rest.
//
//**************************************************************

#include "cgen.h"
#include "cgen_gc.h"

extern void emit_string_constant(ostream& str, char *s);
extern int cgen_debug;

//
// Three symbols from the semantic analyzer (semant.cc) are used.
// If e : No_type, then no code is generated for e.
// Special code is generated for new SELF_TYPE.
// The name "self" also generates code different from other references.
//
//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
Symbol 
       arg,
       arg2,
       Bool,
       concat,
       cool_abort,
       copy,
       Int,
       in_int,
       in_string,
       IO,
       length,
       Main,
       main_meth,
       No_class,
       No_type,
       Object,
       out_int,
       out_string,
       prim_slot,
       self,
       SELF_TYPE,
       Str,
       str_field,
       substr,
       type_name,
       val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
  arg         = idtable.add_string("arg");
  arg2        = idtable.add_string("arg2");
  Bool        = idtable.add_string("Bool");
  concat      = idtable.add_string("concat");
  cool_abort  = idtable.add_string("abort");
  copy        = idtable.add_string("copy");
  Int         = idtable.add_string("Int");
  in_int      = idtable.add_string("in_int");
  in_string   = idtable.add_string("in_string");
  IO          = idtable.add_string("IO");
  length      = idtable.add_string("length");
  Main        = idtable.add_string("Main");
  main_meth   = idtable.add_string("main");
//   _no_class is a symbol that can't be the name of any 
//   user-defined class.
  No_class    = idtable.add_string("_no_class");
  No_type     = idtable.add_string("_no_type");
  Object      = idtable.add_string("Object");
  out_int     = idtable.add_string("out_int");
  out_string  = idtable.add_string("out_string");
  prim_slot   = idtable.add_string("_prim_slot");
  self        = idtable.add_string("self");
  SELF_TYPE   = idtable.add_string("SELF_TYPE");
  Str         = idtable.add_string("String");
  str_field   = idtable.add_string("_str_field");
  substr      = idtable.add_string("substr");
  type_name   = idtable.add_string("type_name");
  val         = idtable.add_string("_val");
}

static char *gc_init_names[] =
  { "_NoGC_Init", "_GenGC_Init", "_ScnGC_Init" };
static char *gc_collect_names[] =
  { "_NoGC_Collect", "_GenGC_Collect", "_ScnGC_Collect" };


//  BoolConst is a class that implements code generation for operations
//  on the two booleans, which are given global names here.
BoolConst falsebool(FALSE);
BoolConst truebool(TRUE);

enum VarType { VAR_ATTR, VAR_FORMAL, VAR_LOCAL };

struct VarLocation {
  VarType type;
  int offset;
  VarLocation(VarType t, int o) : type(t), offset(o) {}
};

//Global symbol table, maps variable names to locations
SymbolTable<Symbol, VarLocation> var_env;
CgenNodeP current_class = NULL;

CgenClassTable *codegen_classtable = NULL;

int next_local_offset = -1;

int cgen_label_count = 0;
int get_next_label() {
  return cgen_label_count++;
}

struct CaseBranchTemp {
  int tag;
  int max_child;
  Symbol type;
  Symbol name;
  Expression expr;

  CaseBranchTemp(int t, int m, Symbol ty, Symbol n, Expression e)
    : tag(t), max_child(m), type(ty), name(n), expr(e) {}
};

//*********************************************************
//
// Define method for code generation
//
// This is the method called by the compiler driver
// `cgtest.cc'. cgen takes an `ostream' to which the assembly will be
// emmitted, and it passes this and the class list of the
// code generator tree to the constructor for `CgenClassTable'.
// That constructor performs all of the work of the code
// generator.
//
//*********************************************************

void program_class::cgen(ostream &os) 
{
  // spim wants comments to start with '#'
  os << "# start of generated code\n";

  initialize_constants();
  codegen_classtable = new CgenClassTable(classes,os);

  os << "\n# end of generated code\n";
}


//////////////////////////////////////////////////////////////////////////////
//
//  emit_* procedures
//
//  emit_X  writes code for operation "X" to the output stream.
//  There is an emit_X for each opcode X, as well as emit_ functions
//  for generating names according to the naming conventions (see emit.h)
//  and calls to support functions defined in the trap handler.
//
//  Register names and addresses are passed as strings.  See `emit.h'
//  for symbolic names you can use to refer to the strings.
//
//////////////////////////////////////////////////////////////////////////////

static void emit_load(char *dest_reg, int offset, char *source_reg, ostream& s)
{
  s << LW << dest_reg << " " << offset * WORD_SIZE << "(" << source_reg << ")" 
    << endl;
}

static void emit_store(char *source_reg, int offset, char *dest_reg, ostream& s)
{
  s << SW << source_reg << " " << offset * WORD_SIZE << "(" << dest_reg << ")"
      << endl;
}

static void emit_load_imm(char *dest_reg, int val, ostream& s)
{ s << LI << dest_reg << " " << val << endl; }

static void emit_load_address(char *dest_reg, char *address, ostream& s)
{ s << LA << dest_reg << " " << address << endl; }

static void emit_partial_load_address(char *dest_reg, ostream& s)
{ s << LA << dest_reg << " "; }

static void emit_load_bool(char *dest, const BoolConst& b, ostream& s)
{
  emit_partial_load_address(dest,s);
  b.code_ref(s);
  s << endl;
}

static void emit_load_string(char *dest, StringEntry *str, ostream& s)
{
  emit_partial_load_address(dest,s);
  str->code_ref(s);
  s << endl;
}

static void emit_load_int(char *dest, IntEntry *i, ostream& s)
{
  emit_partial_load_address(dest,s);
  i->code_ref(s);
  s << endl;
}

static void emit_move(char *dest_reg, char *source_reg, ostream& s)
{ s << MOVE << dest_reg << " " << source_reg << endl; }

static void emit_neg(char *dest, char *src1, ostream& s)
{ s << NEG << dest << " " << src1 << endl; }

static void emit_add(char *dest, char *src1, char *src2, ostream& s)
{ s << ADD << dest << " " << src1 << " " << src2 << endl; }

static void emit_addu(char *dest, char *src1, char *src2, ostream& s)
{ s << ADDU << dest << " " << src1 << " " << src2 << endl; }

static void emit_addiu(char *dest, char *src1, int imm, ostream& s)
{ s << ADDIU << dest << " " << src1 << " " << imm << endl; }

static void emit_div(char *dest, char *src1, char *src2, ostream& s)
{ s << DIV << dest << " " << src1 << " " << src2 << endl; }

static void emit_mul(char *dest, char *src1, char *src2, ostream& s)
{ s << MUL << dest << " " << src1 << " " << src2 << endl; }

static void emit_sub(char *dest, char *src1, char *src2, ostream& s)
{ s << SUB << dest << " " << src1 << " " << src2 << endl; }

static void emit_sll(char *dest, char *src1, int num, ostream& s)
{ s << SLL << dest << " " << src1 << " " << num << endl; }

static void emit_jalr(char *dest, ostream& s)
{ s << JALR << "\t" << dest << endl; }

static void emit_jal(char *address,ostream &s)
{ s << JAL << address << endl; }

static void emit_return(ostream& s)
{ s << RET << endl; }

static void emit_gc_assign(ostream& s)
{ s << JAL << "_GenGC_Assign" << endl; }

static void emit_disptable_ref(Symbol sym, ostream& s)
{  s << sym << DISPTAB_SUFFIX; }

static void emit_init_ref(Symbol sym, ostream& s)
{ s << sym << CLASSINIT_SUFFIX; }

static void emit_label_ref(int l, ostream &s)
{ s << "label" << l; }

static void emit_protobj_ref(Symbol sym, ostream& s)
{ s << sym << PROTOBJ_SUFFIX; }

static void emit_method_ref(Symbol classname, Symbol methodname, ostream& s)
{ s << classname << METHOD_SEP << methodname; }

static void emit_label_def(int l, ostream &s)
{
  emit_label_ref(l,s);
  s << ":" << endl;
}

static void emit_beqz(char *source, int label, ostream &s)
{
  s << BEQZ << source << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_beq(char *src1, char *src2, int label, ostream &s)
{
  s << BEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bne(char *src1, char *src2, int label, ostream &s)
{
  s << BNE << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bleq(char *src1, char *src2, int label, ostream &s)
{
  s << BLEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blt(char *src1, char *src2, int label, ostream &s)
{
  s << BLT << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blti(char *src1, int imm, int label, ostream &s)
{
  s << BLT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bgti(char *src1, int imm, int label, ostream &s)
{
  s << BGT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_branch(int l, ostream& s)
{
  s << BRANCH;
  emit_label_ref(l,s);
  s << endl;
}

//
// Push a register on the stack. The stack grows towards smaller addresses.
//
static void emit_push(char *reg, ostream& str)
{
  emit_store(reg,0,SP,str);
  emit_addiu(SP,SP,-4,str);
}

//
// Fetch the integer value in an Int object.
// Emits code to fetch the integer value of the Integer object pointed
// to by register source into the register dest
//
static void emit_fetch_int(char *dest, char *source, ostream& s)
{ emit_load(dest, DEFAULT_OBJFIELDS, source, s); }

//
// Emits code to store the integer value contained in register source
// into the Integer object pointed to by dest.
//
static void emit_store_int(char *source, char *dest, ostream& s)
{ emit_store(source, DEFAULT_OBJFIELDS, dest, s); }


static void emit_test_collector(ostream &s)
{
  emit_push(ACC, s);
  emit_move(ACC, SP, s); // stack end
  emit_move(A1, ZERO, s); // allocate nothing
  s << JAL << gc_collect_names[cgen_Memmgr] << endl;
  emit_addiu(SP,SP,4,s);
  emit_load(ACC,0,SP,s);
}

static void emit_gc_check(char *source, ostream &s)
{
  if (source != (char*)A1) emit_move(A1, source, s);
  s << JAL << "_gc_check" << endl;
}


///////////////////////////////////////////////////////////////////////////////
//
// coding strings, ints, and booleans
//
// Cool has three kinds of constants: strings, ints, and booleans.
// This section defines code generation for each type.
//
// All string constants are listed in the global "stringtable" and have
// type StringEntry.  StringEntry methods are defined both for String
// constant definitions and references.
//
// All integer constants are listed in the global "inttable" and have
// type IntEntry.  IntEntry methods are defined for Int
// constant definitions and references.
//
// Since there are only two Bool values, there is no need for a table.
// The two booleans are represented by instances of the class BoolConst,
// which defines the definition and reference methods for Bools.
//
///////////////////////////////////////////////////////////////////////////////

//
// Strings
//
void StringEntry::code_ref(ostream& s)
{
  s << STRCONST_PREFIX << index;
}

//
// Emit code for a constant String.
// You should fill in the code naming the dispatch table.
//

void StringEntry::code_def(ostream& s, int stringclasstag)
{
  IntEntryP lensym = inttable.add_int(len);

  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s  << LABEL                                             // label
      << WORD << stringclasstag << endl                                 // tag
      << WORD << (DEFAULT_OBJFIELDS + STRING_SLOTS + (len+4)/4) << endl // size
      << WORD;


 /***** Add dispatch information for class String ******/
    emit_disptable_ref(Str, s);
      s << endl;                                              // dispatch table
      s << WORD;  lensym->code_ref(s);  s << endl;            // string length
  emit_string_constant(s,str);                                // ascii string
  s << ALIGN;                                                 // align to word
}

//
// StrTable::code_string
// Generate a string object definition for every string constant in the 
// stringtable.
//
void StrTable::code_string_table(ostream& s, int stringclasstag)
{  
  for (List<StringEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s,stringclasstag);
}

//
// Ints
//
void IntEntry::code_ref(ostream &s)
{
  s << INTCONST_PREFIX << index;
}

//
// Emit code for a constant Integer.
// You should fill in the code naming the dispatch table.
//

void IntEntry::code_def(ostream &s, int intclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL                                // label
      << WORD << intclasstag << endl                      // class tag
      << WORD << (DEFAULT_OBJFIELDS + INT_SLOTS) << endl  // object size
      << WORD; 

 /***** Add dispatch information for class Int ******/
      emit_disptable_ref(Int, s);
      s << endl;                                          // dispatch table
      s << WORD << str << endl;                           // integer value
}


//
// IntTable::code_string_table
// Generate an Int object definition for every Int constant in the
// inttable.
//
void IntTable::code_string_table(ostream &s, int intclasstag)
{
  for (List<IntEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s,intclasstag);
}


//
// Bools
//
BoolConst::BoolConst(int i) : val(i) { assert(i == 0 || i == 1); }

void BoolConst::code_ref(ostream& s) const
{
  s << BOOLCONST_PREFIX << val;
}
  
//
// Emit code for a constant Bool.
// You should fill in the code naming the dispatch table.
//

void BoolConst::code_def(ostream& s, int boolclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL                                  // label
      << WORD << boolclasstag << endl                       // class tag
      << WORD << (DEFAULT_OBJFIELDS + BOOL_SLOTS) << endl   // object size
      << WORD;

 /***** Add dispatch information for class Bool ******/
      emit_disptable_ref(Bool, s);
      s << endl;                                            // dispatch table
      s << WORD << val << endl;                             // value (0 or 1)
}

//////////////////////////////////////////////////////////////////////////////
//
//  CgenClassTable methods
//
//////////////////////////////////////////////////////////////////////////////

void CgenClassTable::assign_tags(CgenNodeP node)
{
  node->set_tag(next_class_tag);
  class_nodes.push_back(node);
  next_class_tag++;

  if (node->get_name() == Str) stringclasstag = node->get_tag();
  if (node->get_name() == Int) intclasstag = node->get_tag();
  if (node->get_name() == Bool) boolclasstag = node->get_tag();

  node->build_attr_table();
  node->build_method_table();

  for (List<CgenNode> *l = node->get_children(); l; l = l->tl()) {
    assign_tags(l->hd());
  }

  node->set_max_child_tag(next_class_tag - 1);
}

//***************************************************
//
//  Emit code to start the .data segment and to
//  declare the global names.
//
//***************************************************

void CgenClassTable::code_global_data()
{
  Symbol main    = idtable.lookup_string(MAINNAME);
  Symbol string  = idtable.lookup_string(STRINGNAME);
  Symbol integer = idtable.lookup_string(INTNAME);
  Symbol boolc   = idtable.lookup_string(BOOLNAME);

  str << "\t.data\n" << ALIGN;
  //
  // The following global names must be defined first.
  //
  str << GLOBAL << CLASSNAMETAB << endl;
  str << GLOBAL; emit_protobj_ref(main,str);    str << endl;
  str << GLOBAL; emit_protobj_ref(integer,str); str << endl;
  str << GLOBAL; emit_protobj_ref(string,str);  str << endl;
  str << GLOBAL; falsebool.code_ref(str);  str << endl;
  str << GLOBAL; truebool.code_ref(str);   str << endl;
  str << GLOBAL << INTTAG << endl;
  str << GLOBAL << BOOLTAG << endl;
  str << GLOBAL << STRINGTAG << endl;

  //
  // We also need to know the tag of the Int, String, and Bool classes
  // during code generation.
  //
  str << INTTAG << LABEL
      << WORD << intclasstag << endl;
  str << BOOLTAG << LABEL 
      << WORD << boolclasstag << endl;
  str << STRINGTAG << LABEL 
      << WORD << stringclasstag << endl;    
}


//***************************************************
//
//  Emit code to start the .text segment and to
//  declare the global names.
//
//***************************************************

void CgenClassTable::code_global_text()
{
  str << GLOBAL << HEAP_START << endl
      << HEAP_START << LABEL 
      << WORD << 0 << endl
      << "\t.text" << endl
      << GLOBAL;
  emit_init_ref(idtable.add_string("Main"), str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Int"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("String"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Bool"),str);
  str << endl << GLOBAL;
  emit_method_ref(idtable.add_string("Main"), idtable.add_string("main"), str);
  str << endl;
}

void CgenClassTable::code_bools(int boolclasstag)
{
  falsebool.code_def(str,boolclasstag);
  truebool.code_def(str,boolclasstag);
}

void CgenClassTable::code_select_gc()
{
  //
  // Generate GC choice constants (pointers to GC functions)
  //
  str << GLOBAL << "_MemMgr_INITIALIZER" << endl;
  str << "_MemMgr_INITIALIZER:" << endl;
  str << WORD << gc_init_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_COLLECTOR" << endl;
  str << "_MemMgr_COLLECTOR:" << endl;
  str << WORD << gc_collect_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_TEST" << endl;
  str << "_MemMgr_TEST:" << endl;
  str << WORD << (cgen_Memmgr_Test == GC_TEST) << endl;
}


//********************************************************
//
// Emit code to reserve space for and initialize all of
// the constants.  Class names should have been added to
// the string table (in the supplied code, is is done
// during the construction of the inheritance graph), and
// code for emitting string constants as a side effect adds
// the string's length to the integer table.  The constants
// are emmitted by running through the stringtable and inttable
// and producing code for each entry.
//
//********************************************************

void CgenClassTable::code_constants()
{
  //
  // Add constants that are required by the code generator.
  //
  stringtable.add_string("");
  inttable.add_string("0");

  stringtable.code_string_table(str,stringclasstag);
  inttable.code_string_table(str,intclasstag);
  code_bools(boolclasstag);
}

void CgenClassTable::code_class_name_tab()
{
  str << CLASSNAMETAB << LABEL;
  for (size_t i = 0; i < class_nodes.size(); i++) {
    str << WORD;
    StringEntry *entry = stringtable.lookup_string(
      class_nodes[i]->get_name()->get_string());
    entry->code_ref(str);
    str << endl;
  }
}

void CgenClassTable::code_class_obj_tab()
{
  str << CLASSOBJTAB << LABEL;
  for (size_t i = 0; i < class_nodes.size(); i++) {
    str << WORD;
    emit_protobj_ref(class_nodes[i]->get_name(), str);
    str << endl;

    str << WORD;
    emit_init_ref(class_nodes[i]->get_name(), str);
    str << endl;
  }
}

void CgenClassTable::code_dispatch_tables()
{
  for (size_t i = 0; i < class_nodes.size(); i++) {
    CgenNodeP nd = class_nodes[i];

    str << nd->get_name() << DISPTAB_SUFFIX << LABEL;

    std::vector<std::pair<Symbol, Symbol> >& methods = nd->get_method_table();
    for (size_t j = 0; j < methods.size(); j++) {
      str << WORD;
      emit_method_ref(methods[j].first, methods[j].second, str);
      str << endl;
    }
  }
}

void CgenClassTable::code_prototype_objects()
{
  for (size_t i = 0; i < class_nodes.size(); i++) {
    CgenNodeP nd = class_nodes[i];
    std::vector<std::pair<Symbol, Symbol> >& attrs = nd->get_attr_table();

    str << WORD << "-1" << endl;

    emit_protobj_ref(nd->get_name(), str);
    str << LABEL;

    str << WORD << nd->get_tag() << endl;
    str << WORD << (DEFAULT_OBJFIELDS + attrs.size()) << endl;
    str << WORD;
    emit_disptable_ref(nd->get_name(), str);
    str << endl;

    for (size_t j = 0; j < attrs.size(); j++) {
      Symbol type = attrs[j].second;
      if (type == Str) {
        str << WORD;
        stringtable.lookup_string("")->code_ref(str);
        str << endl;
      } else if (type == Int) {
        str << WORD;
        inttable.lookup_string("0")->code_ref(str);
        str << endl;
      } else if (type == Bool) {
        str << WORD;
        falsebool.code_ref(str);
        str << endl;
      } else {
        str << WORD << "0" << endl;
      }
    }
  }
}

void CgenClassTable::code_class_inits()
{
  for (size_t i = 0; i < class_nodes.size(); i++) {
    CgenNodeP nd = class_nodes[i];
    Symbol class_name = nd->get_name();

    //ClassName_init
    emit_init_ref(class_name, str);
    str << LABEL;
    
    //
    emit_push(FP, str);
    emit_push(SELF, str);
    emit_push(RA, str);

    emit_addiu(FP, SP, 4, str);

    emit_move(SELF, ACC, str);

    if (class_name != Object) {
      str << JAL;
      emit_init_ref(nd->get_parentnd()->get_name(), str);
      str << endl;
    }

    int parent_attr_count = 0;
    if (class_name != Object) {
      parent_attr_count = nd->get_parentnd()->get_attr_table().size();
    }

    std::vector<std::pair<Symbol, Symbol> >& attrs = nd->get_attr_table();

    for (int fi = nd->features->first(); nd->features->more(fi); fi = nd->features->next(fi)) {
      Feature f = nd->features->nth(fi);
      if (f->is_method()) continue;
      attr_class *a = static_cast<attr_class*>(f);

      int attr_offset = -1;
      for (size_t k = 0; k < attrs.size(); k++) {
        if (attrs[k].first == a->name) {
          attr_offset = DEFAULT_OBJFIELDS + k;
          break;
        }
      }

      if (!a->init->is_empty_expr()) {
        a->init->code(str);
        emit_store(ACC, attr_offset, SELF, str);
      }
    }

    emit_move(ACC, SELF, str);

    emit_load(RA, 0, FP, str);
    emit_load(SELF, 1, FP, str);
    emit_load(FP, 2, FP, str);

    emit_addiu(SP, SP, 12, str);
    emit_return(str);
  }
}

void CgenClassTable::code_class_methods()
{
  for (size_t i = 0; i < class_nodes.size(); i++) {
    CgenNodeP nd = class_nodes[i];

    if (nd->basic()) continue;

    Features features = nd->features;
    for (int fi = features->first(); features->more(fi); fi = features->next(fi)) {
      Feature f = features->nth(fi);
      if (!f->is_method()) continue;
      method_class *m = static_cast<method_class*>(f);

      emit_method_ref(nd->get_name(), m->name, str);
      str << LABEL;

      emit_push(FP, str);
      emit_push(SELF, str);
      emit_push(RA, str);
      emit_addiu(FP, SP, 4, str);
      emit_move(SELF, ACC, str);

      //environment setup
      current_class = nd;
      var_env.enterscope();
      next_local_offset = -1;

      //add attributes to environment
      std::vector<std::pair<Symbol, Symbol> >& attrs = nd->get_attr_table();
      for (size_t k = 0; k < attrs.size(); k++) {
        var_env.addid(attrs[k].first, new VarLocation(VAR_ATTR, DEFAULT_OBJFIELDS + k));
      }

      //add formals to environment
      int num_formals = 0;
      for (int fi2 = m->formals->first(); m->formals->more(fi2); fi2 = m->formals->next(fi2)) {
        num_formals++;
      }

      int formal_idx = 0;
      for (int fi2 = m->formals->first(); m->formals->more(fi2); fi2 = m->formals->next(fi2)) {
        formal_class *f = static_cast<formal_class*>(m->formals->nth(fi2));
        int offset = 2 + num_formals - formal_idx;
        var_env.addid(f->name, new VarLocation(VAR_FORMAL, offset));
        formal_idx++;
      }

      m->expr->code(str);

      var_env.exitscope();

      emit_load(SELF, 1, FP, str);
      emit_load(RA, 0, FP, str);
      emit_load(FP, 2, FP, str);

      emit_addiu(SP, SP, (3 + num_formals) * WORD_SIZE, str);
      emit_return(str);
    }
  } 
}

CgenClassTable::CgenClassTable(Classes classes, ostream& s) : nds(NULL) , str(s)
{
   codegen_classtable = this;
   stringclasstag = 0 /* Change to your String class tag here */;
   intclasstag =    0 /* Change to your Int class tag here */;
   boolclasstag =   0 /* Change to your Bool class tag here */;
   next_class_tag = 0;
   label_count = 0;

   enterscope();
   if (cgen_debug) cout << "Building CgenClassTable" << endl;
   install_basic_classes();
   install_classes(classes);
   build_inheritance_tree();

   assign_tags(root());

   code();
   exitscope();
}

void CgenClassTable::install_basic_classes()
{

// The tree package uses these globals to annotate the classes built below.
  //curr_lineno  = 0;
  Symbol filename = stringtable.add_string("<basic class>");

//
// A few special class names are installed in the lookup table but not
// the class list.  Thus, these classes exist, but are not part of the
// inheritance hierarchy.
// No_class serves as the parent of Object and the other special classes.
// SELF_TYPE is the self class; it cannot be redefined or inherited.
// prim_slot is a class known to the code generator.
//
  addid(No_class,
	new CgenNode(class_(No_class,No_class,nil_Features(),filename),
			    Basic,this));
  addid(SELF_TYPE,
	new CgenNode(class_(SELF_TYPE,No_class,nil_Features(),filename),
			    Basic,this));
  addid(prim_slot,
	new CgenNode(class_(prim_slot,No_class,nil_Features(),filename),
			    Basic,this));

// 
// The Object class has no parent class. Its methods are
//        cool_abort() : Object    aborts the program
//        type_name() : Str        returns a string representation of class name
//        copy() : SELF_TYPE       returns a copy of the object
//
// There is no need for method bodies in the basic classes---these
// are already built in to the runtime system.
//
  install_class(
   new CgenNode(
    class_(Object, 
	   No_class,
	   append_Features(
           append_Features(
           single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
           single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
           single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	   filename),
    Basic,this));

// 
// The IO class inherits from Object. Its methods are
//        out_string(Str) : SELF_TYPE          writes a string to the output
//        out_int(Int) : SELF_TYPE               "    an int    "  "     "
//        in_string() : Str                    reads a string from the input
//        in_int() : Int                         "   an int     "  "     "
//
   install_class(
    new CgenNode(
     class_(IO, 
            Object,
            append_Features(
            append_Features(
            append_Features(
            single_Features(method(out_string, single_Formals(formal(arg, Str)),
                        SELF_TYPE, no_expr())),
            single_Features(method(out_int, single_Formals(formal(arg, Int)),
                        SELF_TYPE, no_expr()))),
            single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
            single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	   filename),	    
    Basic,this));

//
// The Int class has no methods and only a single attribute, the
// "val" for the integer. 
//
   install_class(
    new CgenNode(
     class_(Int, 
	    Object,
            single_Features(attr(val, prim_slot, no_expr())),
	    filename),
     Basic,this));

//
// Bool also has only the "val" slot.
//
    install_class(
     new CgenNode(
      class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename),
      Basic,this));

//
// The class Str has a number of slots and operations:
//       val                                  ???
//       str_field                            the string itself
//       length() : Int                       length of the string
//       concat(arg: Str) : Str               string concatenation
//       substr(arg: Int, arg2: Int): Str     substring
//       
   install_class(
    new CgenNode(
      class_(Str, 
	     Object,
             append_Features(
             append_Features(
             append_Features(
             append_Features(
             single_Features(attr(val, Int, no_expr())),
            single_Features(attr(str_field, prim_slot, no_expr()))),
            single_Features(method(length, nil_Formals(), Int, no_expr()))),
            single_Features(method(concat, 
				   single_Formals(formal(arg, Str)),
				   Str, 
				   no_expr()))),
	    single_Features(method(substr, 
				   append_Formals(single_Formals(formal(arg, Int)), 
						  single_Formals(formal(arg2, Int))),
				   Str, 
				   no_expr()))),
	     filename),
        Basic,this));

}

// CgenClassTable::install_class
// CgenClassTable::install_classes
//
// install_classes enters a list of classes in the symbol table.
//
void CgenClassTable::install_class(CgenNodeP nd)
{
  Symbol name = nd->get_name();

  if (probe(name))
    {
      return;
    }

  // The class name is legal, so add it to the list of classes
  // and the symbol table.
  nds = new List<CgenNode>(nd,nds);
  addid(name,nd);
}

void CgenClassTable::install_classes(Classes cs)
{
  for(int i = cs->first(); cs->more(i); i = cs->next(i))
    install_class(new CgenNode(cs->nth(i),NotBasic,this));
}

//
// CgenClassTable::build_inheritance_tree
//
void CgenClassTable::build_inheritance_tree()
{
  for(List<CgenNode> *l = nds; l; l = l->tl())
      set_relations(l->hd());
}

//
// CgenClassTable::set_relations
//
// Takes a CgenNode and locates its, and its parent's, inheritance nodes
// via the class table.  Parent and child pointers are added as appropriate.
//
void CgenClassTable::set_relations(CgenNodeP nd)
{
  CgenNode *parent_node = probe(nd->get_parent());
  nd->set_parentnd(parent_node);
  parent_node->add_child(nd);
}

void CgenNode::add_child(CgenNodeP n)
{
  children = new List<CgenNode>(n,children);
}

void CgenNode::set_parentnd(CgenNodeP p)
{
  assert(parentnd == NULL);
  assert(p != NULL);
  parentnd = p;
}



void CgenClassTable::code()
{
  if (cgen_debug) cout << "coding global data" << endl;
  code_global_data();

  if (cgen_debug) cout << "choosing gc" << endl;
  code_select_gc();

  if (cgen_debug) cout << "coding constants" << endl;
  code_constants();

  code_class_name_tab();

  code_class_obj_tab();

  code_dispatch_tables();

  code_prototype_objects();

  if (cgen_debug) cout << "coding global text" << endl;
  code_global_text();

  code_class_inits();
  code_class_methods();
}


CgenNodeP CgenClassTable::root()
{
   return probe(Object);
}


///////////////////////////////////////////////////////////////////////
//
// CgenNode methods
//
///////////////////////////////////////////////////////////////////////

CgenNode::CgenNode(Class_ nd, Basicness bstatus, CgenClassTableP ct) :
   class__class((const class__class &) *nd),
   parentnd(NULL),
   children(NULL),
   basic_status(bstatus)
{ 
   stringtable.add_string(name->get_string());          // Add class name to string table
}

void CgenNode::build_attr_table()
{
  if (get_name() != Object) {
    attr_table = parentnd->get_attr_table();
  }

  for (int i = features->first(); features->more(i); i = features->next(i)) {
    Feature f = features->nth(i);
    if (!f->is_method()) {
      attr_class *a = static_cast<attr_class*>(f);
      attr_table.push_back(std::make_pair(a->name, a->type_decl));
    }
  }
}

void CgenNode::build_method_table()
{
  if (get_name() != Object) {
    method_table = parentnd->get_method_table();
  }

  for (int i = features->first(); features->more(i); i = features->next(i)) {
    Feature f = features->nth(i);
    if (f->is_method()) {
      method_class *m = static_cast<method_class*>(f);
      bool overridden = false;
      for (size_t j = 0; j < method_table.size(); j++) {
        if (method_table[j].second == m->name) {
          method_table[j].first = get_name();
          overridden = true;
          break;
        }
      }
      if (!overridden) {
        method_table.push_back(std::make_pair(get_name(), m->name));
      }
    }
  }
}


//******************************************************************
//
//   Fill in the following methods to produce code for the
//   appropriate expression.  You may add or remove parameters
//   as you wish, but if you do, remember to change the parameters
//   of the declarations in `cool-tree.h'  Sample code for
//   constant integers, strings, and booleans are provided.
//
//*****************************************************************

void assign_class::code(ostream &s) {
  expr->code(s);
  VarLocation *loc = var_env.lookup(name);
  if (loc == NULL) return;
  if (loc->type == VAR_ATTR) {
    emit_store(ACC, loc->offset, SELF, s);
  } else if (loc->type == VAR_FORMAL || loc->type == VAR_LOCAL) {
    emit_store(ACC, loc->offset, FP, s);
  }
}

void static_dispatch_class::code(ostream &s) {
  int arg_count = 0;
  for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
    actual->nth(i)->code(s);
    emit_push(ACC, s);
    arg_count++;
  }

  expr->code(s);

  int not_void_label = get_next_label();
  emit_bne(ACC, ZERO, not_void_label, s);
  emit_load_address(ACC, "str_const0", s);
  emit_load_imm(T1, 1, s);
  emit_jal("_dispatch_abort", s);

  emit_label_def(not_void_label, s);

  emit_partial_load_address(T1, s);
  emit_disptable_ref(type_name, s);
  s << endl;

  CgenNodeP disp_node = codegen_classtable->probe(type_name);
  int method_offset = -1;
  std::vector<std::pair<Symbol, Symbol> >& methods = disp_node->get_method_table();

  for (size_t j = 0; j < methods.size(); j++) {
    if (methods[j].second == name) {
      method_offset = j;
      break;
    }
  }
  assert(method_offset != -1);

  emit_load(T1, method_offset, T1, s);
  emit_jalr(T1, s);
}

void dispatch_class::code(ostream &s) {
  int arg_count = 0;
  for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
    actual->nth(i)->code(s);
    emit_push(ACC, s);
    arg_count++;
  }

  expr->code(s);

  int not_void_label = get_next_label();
  emit_bne(ACC, ZERO, not_void_label, s);
  emit_load_address(ACC, "str_const0", s);
  emit_load_imm(T1, 1, s);
  emit_jal("_dispatch_abort", s);

  emit_label_def(not_void_label, s);

  emit_load(T1, 2, ACC, s);

  Symbol disp_type = expr->get_type();
  if (disp_type == SELF_TYPE) {
    disp_type = current_class->get_name();
  }

  CgenNodeP disp_node = codegen_classtable->probe(disp_type);
  int method_offset = -1;
  std::vector<std::pair<Symbol, Symbol> >& methods = disp_node->get_method_table();

  for (size_t j = 0; j < methods.size(); j++) {
    if (methods[j].second == name) {
      method_offset = j;
      break;
    }
  }
  assert(method_offset != -1);

  emit_load(T1, method_offset, T1, s);
  emit_jalr(T1, s);
}

void cond_class::code(ostream &s) {
  pred->code(s);
  emit_fetch_int(T1, ACC, s);

  int false_label = get_next_label();
  int end_label = get_next_label();

  emit_beqz(T1, false_label, s);

  then_exp->code(s);
  emit_branch(end_label, s);

  emit_label_def(false_label, s);
  else_exp->code(s);

  emit_label_def(end_label, s);
}

void loop_class::code(ostream &s) {
  int loop_label = get_next_label();
  int end_label = get_next_label();

  emit_label_def(loop_label, s);

  pred->code(s);
  emit_fetch_int(T1, ACC, s);
  emit_beqz(T1, end_label, s);

  body->code(s);
  emit_branch(loop_label, s);

  emit_label_def(end_label, s);
  emit_move(ACC, ZERO, s);
}

void typcase_class::code(ostream &s) {
  expr->code(s);

  int not_void_label = get_next_label();
  emit_bne(ACC, ZERO, not_void_label, s);

  emit_load_address(ACC, "str_const0", s);
  emit_load_imm(T1, 1, s);
  emit_jal("_case_abort2", s);

  emit_label_def(not_void_label, s);

  emit_load(T2, 0, ACC, s);

  int end_label = get_next_label();

  //get all the case branches information
  std::vector<CaseBranchTemp> branches;
  for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
    branch_class* b = static_cast<branch_class*>(cases->nth(i));
    CgenNodeP node_class = codegen_classtable->probe(b->type_decl);
  
    branches.push_back(CaseBranchTemp(
      node_class->get_tag(),
      node_class->get_max_child_tag(),
      b->type_decl,
      b->name,
      b->expr
    ));
  }

  //sort the tags in descending order, to match more specific matches first
  for (size_t i = 0; i < branches.size(); i++) {
    for (size_t j = i + 1; j < branches.size(); j++) {
      if (branches[j].tag > branches[i].tag) {
        CaseBranchTemp temp = branches[i];
        branches[i] = branches[j];
        branches[j] = temp;
      }
    }
  }

  for (size_t i = 0; i < branches.size(); i++) {
    CaseBranchTemp b = branches[i];
    int next_branch_label = get_next_label();

    emit_blti(T2, b.tag, next_branch_label, s);
    emit_bgti(T2, b.max_child, next_branch_label, s);

    emit_push(ACC, s);

    var_env.enterscope();
    var_env.addid(b.name, new VarLocation(VAR_LOCAL, next_local_offset));
    int prev_offset = next_local_offset;
    next_local_offset--;

    b.expr->code(s);

    var_env.exitscope();
    next_local_offset = prev_offset;

    emit_addiu(SP, SP, 4, s);

    emit_branch(end_label, s);

    emit_label_def(next_branch_label, s);
  }

  emit_jal("_case_abort", s);
  emit_label_def(end_label, s);
}

void block_class::code(ostream &s) {
  for (int i = body->first(); body->more(i); i = body->next(i)) {
    body->nth(i)->code(s);
  }
}

void let_class::code(ostream &s) {
  init->code(s);

  if (init->is_empty_expr()) {
    if (type_decl == Str) {
      emit_load_string(ACC, stringtable.lookup_string(""), s);
    } else if (type_decl == Int) {
      emit_load_int(ACC, inttable.lookup_string("0"), s);
    } else if (type_decl == Bool) {
      emit_load_bool(ACC, falsebool, s);
    } else {
      emit_move(ACC, ZERO, s);
    }
  }

  emit_push(ACC, s);

  var_env.enterscope();
  var_env.addid(identifier, new VarLocation(VAR_LOCAL, next_local_offset));
  int prev_offset = next_local_offset;
  next_local_offset--;

  body->code(s);

  var_env.exitscope();
  next_local_offset = prev_offset;
  emit_addiu(SP, SP, 4, s);
}

void plus_class::code(ostream &s) {
  //evalute the two expressions, e1 is pushed to stack
  e1->code(s);
  emit_push(ACC, s);
  e2->code(s);

  //create new object, load e1 result to T1, pop stack
  emit_jal("Object.copy", s);
  emit_load(T1, 1, SP, s);
  emit_addiu(SP, SP, 4, s);

  //get integer and store in T1 and T2
  emit_fetch_int(T1, T1, s);
  emit_fetch_int(T2, ACC, s);

  //execute add operation and store in address pointed by ACC
  emit_add(T3, T1, T2, s);
  emit_store_int(T3, ACC, s);
}

void sub_class::code(ostream &s) {
  e1->code(s);
  emit_push(ACC, s);
  e2->code(s);

  emit_jal("Object.copy", s);
  emit_load(T1, 1, SP, s);
  emit_addiu(SP, SP, 4, s);

  emit_fetch_int(T1, T1, s);
  emit_fetch_int(T2, ACC, s);

  emit_sub(T3, T1, T2, s);
  emit_store_int(T3, ACC, s);
}

void mul_class::code(ostream &s) {
  e1->code(s);
  emit_push(ACC, s);
  e2->code(s);

  emit_jal("Object.copy", s);
  emit_load(T1, 1, SP, s);
  emit_addiu(SP, SP, 4, s);

  emit_fetch_int(T1, T1, s);
  emit_fetch_int(T2, ACC, s);
  emit_mul(T3, T1, T2, s);
  emit_store_int(T3, ACC, s);
}

void divide_class::code(ostream &s) {
  e1->code(s);
  emit_push(ACC, s);
  e2->code(s);

  emit_jal("Object.copy", s);
  emit_load(T1, 1, SP, s);
  emit_addiu(SP, SP, 4, s);

  emit_fetch_int(T1, T1, s);
  emit_fetch_int(T2, ACC, s);
  emit_div(T3, T1, T2, s);
  emit_store_int(T3, ACC, s);
}

void neg_class::code(ostream &s) {
  e1->code(s);
  emit_jal("Object.copy", s);

  emit_fetch_int(T1, ACC, s);
  emit_neg(T1, T1, s);
  emit_store_int(T1, ACC, s);
}

void lt_class::code(ostream &s) {
  e1->code(s);
  emit_push(ACC, s);
  e2->code(s);

  emit_load(T1, 1, SP, s);
  emit_addiu(SP, SP, 4, s);

  emit_fetch_int(T1, T1, s);
  emit_fetch_int(T2, ACC, s);

  emit_load_bool(ACC, truebool, s);
  int label = get_next_label();
  emit_blt(T1, T2, label, s);

  emit_load_bool(ACC, falsebool, s);
  emit_label_def(label, s);
}

void eq_class::code(ostream &s) {
  e1->code(s);
  emit_push(ACC, s);
  e2->code(s);

  emit_move(T2, ACC, s);
  emit_load(T1, 1, SP, s);
  emit_addiu(SP, SP, 4, s);

  emit_load_bool(ACC, truebool, s);
  emit_load_bool(A1, falsebool, s);

  int label = get_next_label();
  emit_beq(T1, T2, label, s);

  emit_jal("equality_test", s);

  emit_label_def(label, s);
}

void leq_class::code(ostream &s) {
  e1->code(s);
  emit_push(ACC, s);
  e2->code(s);

  emit_load(T1, 1, SP, s);
  emit_addiu(SP, SP, 4, s);

  emit_fetch_int(T1, T1, s);
  emit_fetch_int(T2, ACC, s);

  emit_load_bool(ACC, truebool, s);
  int label = get_next_label();
  emit_bleq(T1, T2, label, s);

  emit_load_bool(ACC, falsebool, s);
  emit_label_def(label, s);
}

void comp_class::code(ostream &s) {
  e1->code(s);
  emit_fetch_int(T1, ACC, s);

  emit_load_bool(ACC, truebool, s);
  int label = get_next_label();
  emit_beqz(T1, label, s);

  emit_load_bool(ACC, falsebool, s);
  emit_label_def(label, s);
}

void int_const_class::code(ostream& s)  
{
  //
  // Need to be sure we have an IntEntry *, not an arbitrary Symbol
  //
  emit_load_int(ACC,inttable.lookup_string(token->get_string()),s);
}

void string_const_class::code(ostream& s)
{
  emit_load_string(ACC,stringtable.lookup_string(token->get_string()),s);
}

void bool_const_class::code(ostream& s)
{
  emit_load_bool(ACC, BoolConst(val), s);
}

void new__class::code(ostream &s) {
  if  (type_name == SELF_TYPE) {
    emit_load_address(T1, CLASSOBJTAB, s);
    emit_load(T2, 0, SELF, s);
    emit_sll(T2, T2, 3, s);
    emit_addu(T1, T1, T2, s);

    emit_push(T1, s);
    emit_load(ACC, 0, T1, s);
    emit_jal("Object.copy", s);

    emit_load(T1, 1, SP, s);
    emit_addiu(SP, SP, 4, s);

    emit_load(T1, 1, T1, s);
    emit_jalr(T1, s);
  } else {
    emit_partial_load_address(ACC, s);
    emit_protobj_ref(type_name, s);
    s << endl;
    emit_jal("Object.copy", s);

    s << JAL;
    emit_init_ref(type_name, s);
    s << endl;
  }
}

void isvoid_class::code(ostream &s) {
  e1->code(s);
  emit_move(T1, ACC, s);

  emit_load_bool(ACC, truebool, s);
  int label = get_next_label();
  emit_beqz(T1, label, s);

  emit_load_bool(ACC, falsebool, s);
  emit_label_def(label, s);
}

void no_expr_class::code(ostream &s) {
  emit_move(ACC, ZERO, s);
}

void object_class::code(ostream &s) {
  if (name == self) {
    emit_move(ACC, SELF, s);
    return;
  }
  
  VarLocation *loc = var_env.lookup(name);
  if (loc == NULL) {
    return;
  }

  if (loc->type == VAR_ATTR) {
    emit_load(ACC, loc->offset, SELF, s);
  } else if (loc->type == VAR_FORMAL || loc->type == VAR_LOCAL) {
    emit_load(ACC, loc->offset, FP, s);
  }
}


