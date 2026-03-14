#ifndef SEMANT_H_
#define SEMANT_H_

#include <map>
#include <vector>
#include <assert.h>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class method_class;
class attr_class;

// struct MethodSig {
//   Symbol return_type;
//   std::vector<Symbol> param_types;
//   std::vector<Symbol> param_names;
// }

class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();

  std::map<Symbol, Class_> class_map;
  std::map<Symbol, std::map<Symbol, method_class*> > method_env;
  std::map<Symbol, std::map<Symbol, attr_class*> > attr_env;
  std::vector<Symbol> inheritance_order;
  void build_inheritance_order();
  void build_feature_envs();
  void gather_features(Class_ c);
  void type_check_classes();

  ostream& error_stream;

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }

  void add_class(Class_ c);
  Class_ get_class(Symbol name);
  bool check_inheritance_cycle();
  bool is_subclass(Symbol child, Symbol parent);

  method_class* lookup_method(Symbol class_name, Symbol method_name);
  std::map<Symbol, attr_class*>& get_attrs(Symbol class_name);
  std::map<Symbol, method_class*>& get_methods(Symbol class_name);
  std::map<Symbol, Class_>& get_class_map() { return class_map;}
  // finds the least common ancestor of two classes
  Symbol lub(Symbol s1, Symbol s2, Symbol cur_class);

  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
};

#endif
