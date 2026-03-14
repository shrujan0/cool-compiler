

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"
#include <set>
#include <vector>
#include <queue>


extern int semant_debug;
extern char *curr_filename;

typedef SymbolTable<Symbol, Symbol> ObjectEnv;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
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



ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {

    install_basic_classes();

    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ c = classes->nth(i);
        Symbol name = c->get_name();

        if (name == Object || name == Int || name == Bool || name == Str || name == IO || name == SELF_TYPE || name == No_class || name == No_type) {
            semant_error(c) << "Basic classes cannot be redefined: " << name << "." << endl;
            continue;
        }

        if (class_map.find(name) != class_map.end()) {
            semant_error(c) << "Class " << name << " is already defined." << endl;
            continue;
        }

        add_class(c);
    }

    if (class_map.find(Main) == class_map.end()) {
        semant_error() << "Class Main is not defined." << endl;
    }

    check_inheritance_cycle();

    if (errors() == 0) {
        build_feature_envs();
    }

    if (class_map.find(Main) != class_map.end()) {
        method_class* main_method = lookup_method(Main, main_meth);
        if (main_method == NULL) {
            semant_error(class_map[Main]) << "No 'main' method in class Main." << endl;
        } else {
            Formals formals = main_method->get_formals();
            int count = 0;
            for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
                count++;
            }
            if (count > 0) {
                semant_error(class_map[Main]) << "The main method in class Main should have no arguments." << endl;
            }
        }
    }

    if (errors() == 0) {
        type_check_classes();
    }
}

void ClassTable::add_class(Class_ c) {
    class_map[c->get_name()] = c;
}

Class_ ClassTable::get_class(Symbol name) {
    if (class_map.find(name) != class_map.end()) {
        return class_map[name];
    }
    return NULL;
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);
    add_class(Object_class);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
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
	       filename);  
    add_class(IO_class);

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);
    add_class(Int_class);
        
    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);
    add_class(Bool_class);
    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
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
	       filename);
    add_class(Str_class);
}

bool ClassTable::check_inheritance_cycle() {
    bool valid = true;

    // Check parent classes are defined and basic classes are not inherited from
    for (std::map<Symbol, Class_>::iterator it = class_map.begin(); it != class_map.end(); it++) {
        Symbol name = it->first;
        Class_ c = it->second;
        Symbol parent = c->get_parent();

        if (name == Object) continue;

        if (parent == Int || parent == Bool || parent == Str || parent == SELF_TYPE) {
            semant_error(c) << "Class " << name << " cannot inherit from class " << parent << "." << endl;
            valid = false;
            continue;
        }

        if (class_map.find(parent) == class_map.end()) {
            semant_error(c) << "Class " << name << " inherits from an undefined class " << parent << "." << endl;
            valid = false;
            continue;
        }
    }

    // Check if inheritance cycles exist
    for (std::map<Symbol, Class_>::iterator it = class_map.begin(); it != class_map.end(); ++it) {
        Symbol name = it->first;
        if (name == Object) continue;

        std::set<Symbol> visited;
        Symbol cur = name;

        while (cur != No_class && cur != Object) {
            if (visited.find(cur) != visited.end()) {
                semant_error(class_map[name]) << "Class " << name << " or its parent is involved in an inheritance cycle." << endl;
                valid = false;
                break;
            }
            visited.insert(cur);

            if (class_map.find(cur) == class_map.end()) break;
            cur = class_map[cur]->get_parent();
        }
    }

    return valid;
}

bool ClassTable::is_subclass(Symbol child, Symbol parent) {
    Symbol cur = child;

    while (cur != No_class) {
        if (cur == parent) return true;
        if (class_map.find(cur) == class_map.end()) return false;
        cur = class_map[cur]->get_parent();
    }
    return false;
}

Symbol ClassTable::lub(Symbol t1, Symbol t2, Symbol cur_class) {
    Symbol s1 = (t1 == SELF_TYPE) ? cur_class : t1;
    Symbol s2 = (t2 == SELF_TYPE) ? cur_class : t2;

    std::set<Symbol> parents;

    Symbol cur = s1;
    while (cur != No_class) {
        parents.insert(cur);
        if (class_map.find(cur) == class_map.end()) break;
        cur = class_map[cur]->get_parent();
    }

    cur = s2;
    while (cur != No_class) {
        if (parents.find(cur) != parents.end()) return cur;
        if (class_map.find(cur) == class_map.end()) break;
        cur = class_map[cur]->get_parent();
    }

    return Object;
}

void ClassTable::build_inheritance_order() {
    std::map<Symbol, std::vector<Symbol> > children;

    for (std::map<Symbol, Class_>::iterator it = class_map.begin(); it != class_map.end(); ++it) {
        Symbol name = it->first;
        Symbol parent = it->second->get_parent();
        if (name != Object) {
            children[parent].push_back(name);
        }
    }

    std::queue<Symbol> worklist;
    worklist.push(Object);

    while (!worklist.empty()) {
        Symbol cur = worklist.front();
        worklist.pop();
        inheritance_order.push_back(cur);

        std::map<Symbol, std::vector<Symbol> >::iterator cit = children.find(cur);
        if (cit != children.end()) {
            for (size_t i = 0; i < cit->second.size(); i++) {
                worklist.push(cit->second[i]);
            }
        }
    }
}

void ClassTable::gather_features(Class_ c) {
    Symbol name = c->get_name();
    Symbol parent = c->get_parent();

    if (parent != No_class && method_env.find(parent) != method_env.end()) {
        method_env[name] = method_env[parent];
    }
    if (parent != No_class && attr_env.find(parent) != attr_env.end()) {
        attr_env[name] = attr_env[parent];
    }

    Features features = c->get_features();
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        Feature f = features->nth(i);

        if (f->is_method()) {
            method_class* m = static_cast<method_class*>(f);
            Symbol mname = m->get_name();

            if (method_env[name].find(mname) != method_env[name].end()) {
                method_class* inherited = method_env[name][mname];

                if (m->get_return_type() != inherited->get_return_type()) {
                    semant_error(c) << "In redefined method "<<mname
                        << ", return type "<<m->get_return_type()
                        <<" is different from original return type "
                        <<inherited->get_return_type() << "." <<endl;
                }

                Formals new_formals = m->get_formals();
                Formals old_formals = inherited->get_formals();

                int new_count = 0, old_count = 0;
                for (int j = new_formals->first(); new_formals->more(j); j = new_formals->next(j))
                    new_count++;
                for (int j = old_formals->first(); old_formals->more(j); j = old_formals->next(j))
                    old_count++;

                if (new_count != old_count) {
                    semant_error(c) << "Number of formal paramenter different in the inherited method "
                        <<mname<<"."<<endl;
                } else {
                    int j_new = new_formals->first();
                    int j_old = old_formals->first();

                    while (new_formals->more(j_new)) {
                        Symbol new_type = new_formals->nth(j_new)->get_type_decl();
                        Symbol old_type = old_formals->nth(j_old)->get_type_decl();
                        if (new_type != old_type) {
                            semant_error(c) << "The redefined method "<<mname
                                <<" has parameter type "<<new_type<<" which is different from "
                                <<old_type<<"."<<endl;
                        }
                        j_new = new_formals->next(j_new);
                        j_old = old_formals->next(j_old);
                    }
                }
            }

            Formals formals = m->get_formals();
            std::set<Symbol> seen_formals;

            for (int j = formals->first(); formals->more(j); j = formals->next(j)) {
                Formal fm = formals->nth(j);
                Symbol fname = fm->get_name();
                Symbol ftype = fm->get_type_decl();

                if (fname == self) {
                    semant_error(c) << "'self' cannt be used as the name of a formal parameter" << endl;
                }
                if (ftype == SELF_TYPE) {
                    semant_error(c) << "Formal parameter " << fname
                        <<" cannot have SELF_TYPE as type" << endl;
                }

                if (seen_formals.find(fname) != seen_formals.end()) {
                    semant_error(c) << "Formal parameter " << fname
                        <<" is defined multiple timess" << endl;
                }

                seen_formals.insert(fname);
            }
            method_env[name][mname] = m;
            
        } else {
            attr_class* a = static_cast<attr_class*>(f);
            Symbol aname = a->get_name();

            if (aname == self) {
                semant_error(c) << "'self' cannot be the name of an attribute" << endl;
                continue;
            }

            if (parent != No_class && attr_env.find(parent) != attr_env.end()) {
                if (attr_env[parent].find(aname) != attr_env[parent].end()) {
                    semant_error(c) << "Attribute " << aname
                        << " is already defined in the parent class" << endl;
                    continue;
                }
            }

            if (attr_env[name].find(aname) != attr_env[name].end()) {
                semant_error(c) << "Attribute " << aname
                    << " is defined multiple times in the class" << endl;
                continue;
            }

            attr_env[name][aname] = a;
        }
    }
}

void ClassTable::build_feature_envs() {
    build_inheritance_order();

    for (size_t i = 0; i < inheritance_order.size(); i++) {
        Class_ c = class_map[inheritance_order[i]];
        gather_features(c);
    }
}

method_class* ClassTable::lookup_method(Symbol class_name, Symbol method_name) {
    if (method_env.find(class_name) == method_env.end()) return NULL;
    if (method_env[class_name].find(method_name) == method_env[class_name].end()) return NULL;
    return method_env[class_name][method_name];
}

std::map<Symbol, attr_class*>& ClassTable::get_attrs(Symbol class_name) {
    return attr_env[class_name];
}

std::map<Symbol, method_class*>& ClassTable::get_methods(Symbol class_name) {
    return method_env[class_name];
}

/////////////////////////////////////////////////////////////////////
//
// Type checking all of the classes
//
////////////////////////////////////////////////////////////////////

void ClassTable::type_check_classes() {
    for (size_t ci = 0; ci < inheritance_order.size(); ci++) {
        Symbol class_name = inheritance_order[ci];
        if (class_name == Object || class_name == IO || class_name == Int
            || class_name == Bool || class_name == Str)
            continue;
        
        Class_ c = class_map[class_name];
        
        //Object environment for each class
        SymbolTable<Symbol, Symbol>* obj_env = new SymbolTable<Symbol, Symbol>();
        obj_env->enterscope();

        obj_env->addid(self, new Symbol(SELF_TYPE));
        
        //Adding all the attributes to the object enivronment
        std::map<Symbol, attr_class*>& attrs = attr_env[class_name];
        for (std::map<Symbol, attr_class*>::iterator it = attrs.begin(); it != attrs.end(); ++it) {
            Symbol aname = it->first;
            attr_class* a = it->second;
            obj_env->addid(aname, new Symbol(a->get_type_decl()));
        }

        //Type checking features (methods and attributes)
        Features features = c->get_features();
        for (int i = features->first(); features->more(i); i = features->next(i)) {
            Feature f = features->nth(i);

            if (f->is_method()) {
                method_class* m = static_cast<method_class*>(f);

                obj_env->enterscope();

                //Adding formal parameters to the scope/object environment
                Formals formals = m->get_formals();
                for (int j = formals->first(); formals->more(j); j = formals->next(j)) {
                    Formal fm = formals->nth(j);
                    obj_env->addid(fm->get_name(), new Symbol(fm->get_type_decl()));
                }

                //Type check the body of the method
                Symbol body_type = m->get_expr()->type_check(this, c, obj_env);

                Symbol declared_return = m->get_return_type();

                if (declared_return != SELF_TYPE && get_class(declared_return) == NULL) {
                    semant_error(c) << "Undefined return type " << declared_return
                        <<" in method " << m->get_name() << "." << endl;
                }

                Symbol body_resolved = (body_type == SELF_TYPE) ? class_name : body_type;
                Symbol return_resolved = (declared_return == SELF_TYPE) ? class_name : declared_return;

                if (!is_subclass(body_resolved, return_resolved)) {
                    semant_error(c) << "Inferred return type " << body_type
                        << " of method " << m->get_name()
                        << " does not conform the declared return type "
                        << declared_return << "." << endl;
                }

                obj_env->exitscope();
            } else {
                attr_class* a = static_cast<attr_class*>(f);
                Expression init = a->get_init();

                Symbol init_type = init->type_check(this, c, obj_env);

                if (init_type == No_type) {
                    continue;
                }

                Symbol declared_type = a->get_type_decl();
                
                if (declared_type != SELF_TYPE && get_class(declared_type) == NULL) {
                    semant_error(c) << "Class " << declared_type << " of attribute"
                        << a->get_name() << " is undefined." << endl;
                }
                Symbol init_resolved = (init_type == SELF_TYPE) ? class_name : init_type;
                Symbol decl_resolved = (declared_type == SELF_TYPE) ? class_name : declared_type;

                if (!is_subclass(init_resolved, decl_resolved)) {
                    semant_error(c) << "Inferred type " << init_type
                        << " of initialisation of attribute " << a->get_name()
                        << " does not conform to declared type "
                        << declared_type << "." << endl;
                }
            }
        }

        obj_env->exitscope();
        delete obj_env;
    }
}

////////////////////////////////////////////////////////////////////
//
// Type checking for each expression type/kind
//
///////////////////////////////////////////////////////////////////

static Symbol resolve_type(Symbol t, Class_ cur_class) {
    if (t == SELF_TYPE) return cur_class->get_name();
    return t;
}

// type checking for assignment: id <- expr
Symbol assign_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    if (name == self) {
        ct->semant_error(cur_class) << "Cannot assign to 'self'." << endl;
        set_type(Object);
        return type;
    }

    Symbol expr_type = expr->type_check(v_ct, v_class, v_env);

    Symbol* id_type = env->lookup(name);
    if (id_type == NULL) {
        ct->semant_error(cur_class) << "Assignment to undeclared variable " << name
            << "." << endl;
        set_type(Object);
        return type;
    }

    Symbol id_resolved = resolve_type(*id_type, cur_class);
    Symbol expr_resolved = resolve_type(expr_type, cur_class);

    if (!ct->is_subclass(expr_resolved, id_resolved)) {
        ct->semant_error(cur_class) << "Type " << expr_type
            << " of the expression does not conform to declared type "
            << *id_type << " of the identifier " << name << "." << endl;
    }
    set_type(expr_type);
    return type;
}

// static dispatch: expr@type.method(args)
Symbol static_dispatch_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol expr_type = expr->type_check(v_ct, v_class, v_env);

    if (ct->get_class(type_name) == NULL) {
        ct->semant_error(cur_class) << "Static dispatch to undefined class " << type_name << "." << endl;
        set_type(Object);
        return type;
    }

    Symbol expr_resolved = resolve_type(expr_type, cur_class);

    if (!ct->is_subclass(expr_resolved, type_name)) {
        ct->semant_error(cur_class) << "Expression type " << expr_type
            << " does not conform to declared static dispatch type " << type_name << "." << endl;
    }

    method_class* m = ct->lookup_method(type_name, name);
    if (m == NULL) {
        ct->semant_error(cur_class) << "Dispatch to undefined method " << name << "." << endl;
        set_type(Object);
        return type;
    }

    Formals formals = m->get_formals();
    int fi = formals->first();
    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        Symbol actual_type = actual->nth(i)->type_check(v_ct, v_class, v_env);
        if (!formals->more(fi)) {
            ct->semant_error(cur_class) << "Method " << name << " called with too many arguments." << endl;
            break;
        }
        Symbol formal_type = formals->nth(fi)->get_type_decl();
        Symbol actual_resolved = resolve_type(actual_type, cur_class);

        if (!ct->is_subclass(actual_resolved, formal_type)) {
            ct->semant_error(cur_class) << "In call of method " << name
                << ", type " << actual_type << " of parameter "
                << formals->nth(fi)->get_name()
                << " does not conform to declared type " << formal_type << "." << endl;
        }
        fi = formals->next(fi);
    }
    
    if (formals->more(fi)) {
        ct->semant_error(cur_class) << "Method " << name << " called with too few arguments." << endl;
    }

    Symbol ret_type = m->get_return_type();
    if (ret_type == SELF_TYPE) {
        set_type(expr_type);
    } else {
        set_type(ret_type);
    }
    return type;
}

// dispatch: expr.name(args)
Symbol dispatch_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol expr_type = expr->type_check(v_ct, v_class, v_env);
    Symbol expr_resolved = resolve_type(expr_type, cur_class);

    if (ct->get_class(expr_resolved) == NULL) {
        ct->semant_error(cur_class) << "The class " << expr_resolved << " is not defined." << endl;
        set_type(Object);
        return type;
    }

    method_class* m = ct->lookup_method(expr_resolved, name);
    if (m == NULL) {
        ct->semant_error(cur_class) << "The method " << name << " is not defined in class "
            << expr_resolved << "." << endl;
        set_type(Object);
        return type;
    }

    Formals formals = m->get_formals();
    int fi = formals->first();

    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        Symbol actual_type = actual->nth(i)->type_check(v_ct, v_class, v_env);
        if (!formals->more(fi)) {
            ct->semant_error(cur_class) << "Method " << name << " is called with too many arguments." << endl;
            break;
        }
        Symbol formal_type = formals->nth(fi)->get_type_decl();
        Symbol actual_resolved = resolve_type(actual_type, cur_class);
        
        if (!ct->is_subclass(actual_resolved, formal_type)) {
            ct->semant_error(cur_class) << "In call of method " << name
                << ", type " << actual_type << " of parameter "
                << formals->nth(fi)->get_name()
                << " does not conform to declared type " << formal_type << "." << endl;
        }
        fi = formals->next(fi);
    }

    if (formals->more(fi)) {
        ct->semant_error(cur_class) << "Method " << name << " called with too few arguments." << endl;
    }

    Symbol ret_type = m->get_return_type();
    if (ret_type == SELF_TYPE) {
        set_type(expr_resolved);
    } else {
        set_type(ret_type);
    }
    return type;
}

// if pred then then_exp else else_exp fi
Symbol cond_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol pred_type = pred->type_check(v_ct, v_class, v_env);
    if (pred_type != Bool) {
        ct->semant_error(cur_class) << "Predicate of if expression is not of type Bool." << endl;
    }

    Symbol then_type = then_exp->type_check(v_ct, v_class, v_env);
    Symbol else_type = else_exp->type_check(v_ct, v_class, v_env);

    Symbol then_resolved = resolve_type(then_type, cur_class);
    Symbol else_resolved = resolve_type(else_type, cur_class);

    set_type(ct->lub(then_resolved, else_resolved, cur_class->get_name()));
    return type;
}

// while pred loop body pool
Symbol loop_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol pred_type = pred->type_check(v_ct, v_class, v_env);
    if (pred_type != Bool) {
        ct->semant_error(cur_class) << "the condition of while loop does not have type Bool." << endl;
    }

    body->type_check(v_ct, v_class, v_env);
    set_type(Object);
    return type;
}

// case expr of branches esac
Symbol typcase_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol expr_type = expr->type_check(v_ct, v_class, v_env);

    std::set<Symbol> branch_types_seen;
    Symbol result_type = No_type;

    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
        Case branch = cases->nth(i);
        Symbol branch_name = branch->get_name();
        Symbol branch_type_decl = branch->get_type_decl();

        if (branch_types_seen.find(branch_type_decl) != branch_types_seen.end()) {
            ct->semant_error(cur_class) << "Duplicate branch " << branch_type_decl
                << " in case statement." << endl;
        }
        branch_types_seen.insert(branch_type_decl);

        env->enterscope();
        env->addid(branch_name, new Symbol(branch_type_decl));

        Symbol branch_expr_type = branch->get_expr()->type_check(v_ct, v_class, v_env);
        env->exitscope();

        Symbol branch_resolved = resolve_type(branch_expr_type, cur_class);
        if (result_type == No_type) {
            result_type = branch_resolved;
        } else {
            result_type = ct->lub(result_type, branch_resolved, cur_class->get_name());
        }
    }

    set_type(result_type);
    return type;
}

// block: {expr1; expr2; ...exprn;}
Symbol block_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol last_expr_type = No_type;
    for (int i = body->first(); body->more(i); i = body->next(i)) {
        last_expr_type = body->nth(i)->type_check(v_ct, v_class, v_env);
    }
    set_type(last_expr_type);
    return type;
}

//let id: type [<- init] in body
Symbol let_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    if (identifier == self) {
        ct->semant_error(cur_class) << "'self' cannot be used in a let expression." << endl;
    }

    if (type_decl != SELF_TYPE && ct->get_class(type_decl) == NULL) {
        ct->semant_error(cur_class) << "Class " << type_decl << " of let identifier "
            << identifier << " is not defined." << endl;
    }

    Symbol init_type = init->type_check(v_ct, v_class, v_env);

    if (init_type != No_type) {
        Symbol init_resolved = resolve_type(init_type, cur_class);
        Symbol dec_resolved = resolve_type(type_decl, cur_class);

        if (!ct->is_subclass(init_resolved, dec_resolved)) {
            ct->semant_error(cur_class) << "Inferred type " << init_type
                << " of initialisation of " << identifier
                << " does not conform to the declare type "
                << type_decl << "." << endl;
        }
    }

    env->enterscope();
    env->addid(identifier, new Symbol(type_decl));
    Symbol body_type = body->type_check(v_ct, v_class, v_env);
    env->exitscope();

    set_type(body_type);
    return type;
}

// plus_expression: e1+e2
Symbol plus_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol t1 = e1->type_check(v_ct, v_class, v_env);
    Symbol t2 = e2->type_check(v_ct, v_class, v_env);

    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class) << "Operands not of type Int: " << t1 << " + " << t2 << endl;
    }

    set_type(Int);
    return type;
}

//sub_expression: e1 - e2
Symbol sub_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol t1 = e1->type_check(v_ct, v_class, v_env);
    Symbol t2 = e2->type_check(v_ct, v_class, v_env);

    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class) << "Operands not of type Int: " << t1 << " - " << t2 << endl;
    }

    set_type(Int);
    return type;
}

//multiply: e1 * e2
Symbol mul_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol t1 = e1->type_check(v_ct, v_class, v_env);
    Symbol t2 = e2->type_check(v_ct, v_class, v_env);

    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class) << "Operands not of type Int: " << t1 << " * " << t2 << endl;
    }

    set_type(Int);
    return type;
}

//divide: e1 / e2
Symbol divide_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol t1 = e1->type_check(v_ct, v_class, v_env);
    Symbol t2 = e2->type_check(v_ct, v_class, v_env);

    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class) << "Operands not of type Int: " << t1 << " / " << t2 << endl;
    }

    set_type(Int);
    return type;
}

// complement, neg_class: ~e1
Symbol neg_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol t1 = e1->type_check(v_ct, v_class, v_env);

    if (t1 != Int) {
        ct->semant_error(cur_class) << "Argument of '~' operator has type " << t1
            << " instead of Int." << endl;
    }

    set_type(Int);
    return type;
}

// lt: e1 < e2
Symbol lt_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol t1 = e1->type_check(v_ct, v_class, v_env);
    Symbol t2 = e2->type_check(v_ct, v_class, v_env);

    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class) << "Operands not of type Int: " << t1 << " < " << t2 << endl;
    }

    set_type(Bool);
    return type;
}

// eq: e1 = e2
Symbol eq_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol t1 = e1->type_check(v_ct, v_class, v_env);
    Symbol t2 = e2->type_check(v_ct, v_class, v_env);

    if ((t1 == Int || t1 == Bool || t1 == Str || t2 == Int || t2 == Bool || t2 == Str) && t1 != t2) {
        ct->semant_error(cur_class) << "Operands not of same basic type: " << t1 << " = " << t2 << endl;
    }

    set_type(Bool);
    return type;
}

// leq: e1 <= e2
Symbol leq_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol t1 = e1->type_check(v_ct, v_class, v_env);
    Symbol t2 = e2->type_check(v_ct, v_class, v_env);

    if (t1 != Int || t2 != Int) {
        ct->semant_error(cur_class) << "Operands not of type Int: " << t1 << " <= " << t2 << endl;
    }

    set_type(Bool);
    return type;
}

// complement, not: not e1
Symbol comp_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    Symbol t1 = e1->type_check(v_ct, v_class, v_env);

    if (t1 != Bool) {
        ct->semant_error(cur_class) << "Argument of 'not' operator has type " << t1
            << " instead of Bool." << endl;
    }

    set_type(Bool);
    return type;
}

Symbol int_const_class::type_check(void* v_ct, void* v_class, void* v_env) {
    set_type(Int);
    return type;
}

Symbol bool_const_class::type_check(void* v_ct, void* v_class, void* v_env) {
    set_type(Bool);
    return type;
}

Symbol string_const_class::type_check(void* v_ct, void* v_class, void* v_env) {
    set_type(Str);
    return type;
}

//new : new type_name
Symbol new__class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    if (type_name == SELF_TYPE) {
        set_type(SELF_TYPE);
        return type;
    }

    if (ct->get_class(type_name) == NULL) {
        ct->semant_error(cur_class) << "'new' is used with undefined class " << type_name << "." << endl;
        set_type(Object);
        return type;
    }

    set_type(type_name);
    return type;
}

//isvoid : isvoid expr
Symbol isvoid_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    e1->type_check(v_ct, v_class, v_env);
    set_type(Bool);
    return type;
}

//no_expr
Symbol no_expr_class::type_check(void* v_ct, void* v_class, void* v_env) {
    set_type(No_type);
    return type;
}

//object: identifier reference
Symbol object_class::type_check(void* v_ct, void* v_class, void* v_env) {
    ClassTable* ct = static_cast<ClassTable*>(v_ct);
    Class_ cur_class = static_cast<Class_>(v_class);
    ObjectEnv* env = static_cast<ObjectEnv*>(v_env);

    if (name == self) {
        set_type(SELF_TYPE);
        return type;
    }

    Symbol *found = env->lookup(name);
    if (found == NULL) {
        ct->semant_error(cur_class) << "Undeclared identifier " << name << "." <<endl;
        set_type(Object);
        return type;
    }

    set_type(*found);
    return type;
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 



/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    ClassTable *classtable = new ClassTable(classes);

    /* some semantic analysis code may go here */

    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}


