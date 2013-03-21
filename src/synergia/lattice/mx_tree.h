#ifndef MX_PARSING_TREE_H
#define MX_PARSING_TREE_H

#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/variant.hpp>

#include "madx.h"
#include "mx_expr.h"

namespace synergia 
{
  typedef bool(*logic_op_t)(double, double);

  enum mx_statement_type 
    { MX_NULL
    , MX_COMMAND
    , MX_IF
    , MX_WHILE
    , MX_MACRO };

  enum mx_attr_type
    { MX_ATTR_NULL
    , MX_ATTR_STRING
    , MX_ATTR_NUMBER
    , MX_ATTR_PREDEFINED
    , MX_ATTR_ARRAY
    , MX_ATTR_LAZY_NUMBER
    , MX_ATTR_LAZY_ARRAY };

  enum mx_cmd_type
    { MX_CMD_VARIABLE
    , MX_CMD_ELEMENT
    , MX_CMD_EXECUTABLE
    , MX_CMD_ELEMENT_REF };

  enum mx_keyword_type
    { MX_KW_NONE
    , MX_KW_ELEMENT
    , MX_KW_COMMAND
    , MX_KW_ELEMENT_REF
    , MX_KW_PARTICLE 
    , MX_KW_MP_TYPE };

  struct mx_keyword;

  class mx_attr;
  class mx_command;
  class mx_logic;
  class mx_if_block;
  class mx_if;
  class mx_while;
  class mx_statement;
  class mx_tree;

  namespace detail
  {
    template<typename T>
    void print(T const & t) { t.print(); }
  }
}

struct synergia::mx_keyword
{
  mx_keyword() : name(), tag() { }
  mx_keyword(std::string const & k, mx_keyword_type t) : name(k), tag(t) { }

  std::string name;
  mx_keyword_type tag;
};

class synergia::mx_logic
{
public:
  mx_logic(bool p = true) 
    : lhs(0.0), rhs(0.0), op(), pre(p), use_preset(true) { }
  mx_logic(mx_expr const & l, mx_expr const & r, logic_op_t o)
    : lhs(l), rhs(r), op(o), pre(true), use_preset(false) { }

  void set(mx_expr const & l, logic_op_t o, mx_expr const & r);
  bool evaluate(MadX const & mx) const;

private:
  mx_expr lhs;
  mx_expr rhs;
  logic_op_t op;
  bool pre;
  bool use_preset;
};

// statement could be a command, if- or while-
class synergia::mx_statement
{
public:
  mx_statement()
    : value(), type(MX_NULL)
  { }
  mx_statement(mx_command const & st)
    : value(st), type(MX_COMMAND)
  { }
  mx_statement(mx_if const & st)
    : value(st), type(MX_IF)
  { }
  mx_statement(mx_while const & st)
    : value(st), type(MX_WHILE)
  { }

  void assign(mx_command const & st);
  void assign(mx_if const & st);
  void assign(mx_while const & st);

  bool interpret(MadX & mx);
  void print() const;

private:
  boost::any value;
  mx_statement_type type;
};

typedef std::vector<synergia::mx_statement> mx_statements_t;

// vector or statements
class synergia::mx_tree
{
public:
  mx_tree() 
    : statements() 
  { }
  mx_tree(mx_statements_t const & sts)
    : statements(sts)
  { }

  void push(mx_statement const & st);
  bool interpret(MadX & mx);
  void print() const;

private:
  mx_statements_t statements;
};

// attributes for commands
class synergia::mx_attr
{
public:
  mx_attr()
    : type_(MX_ATTR_NULL), name_(), value_() { }

  // modifier
  void set_attr(std::string const & name, boost::any const & val);
  void set_lazy_attr(std::string const & name, boost::any const & val);

  // accessor
  mx_attr_type type() const { return type_; }
  std::string  name() const { return name_; }
  boost::any   value() const { return value_; }

private:
  mx_attr_type type_;
  std::string  name_;
  boost::any   value_;
};

// basic commands
typedef std::map<std::string, synergia::mx_attr> attr_map_t;
typedef std::vector<synergia::mx_attr>           attrs_t;

class synergia::mx_command
{
public:
  mx_command()
    : type_(MX_CMD_VARIABLE), labeled_(false), label_(), keyed_(false), keyword_(), attrs_()
  { }

  void set_label(std::string const & label);
  void set_keyword(mx_keyword const & keyword);
  void set_keyword(std::string const & keyword, mx_cmd_type tag);
  void ins_attr(mx_attr const & attr);

  bool has_label() const { return labeled_; }

  bool interpret(MadX & mx);
  void execute(MadX & mx);
  void print() const;

private:
  mx_cmd_type  type_;
  bool         labeled_;
  std::string  label_;
  bool         keyed_;
  std::string  keyword_;
  attrs_t      attrs_;
};

// element block for building an if-elseif-else control statement
// an if-block contains an optional logic expression and a statement block
class synergia::mx_if_block
{
public:
  mx_if_block()
    : logic_expr(), block(), valid_(false)
  { }
  mx_if_block(mx_logic const & logic, mx_tree const & block) 
    : logic_expr(logic), block(block), valid_(true) 
  { }

  bool valid() const { return valid_; }
  bool evaluate_logic(MadX const & mx) const;
  bool interpret_block(MadX & mx);

  void print_logic() const;
  void print_block() const;

private:
  mx_logic logic_expr;
  mx_tree block;
  bool valid_;
};

typedef std::vector<synergia::mx_if_block> mx_if_block_v;

// if-(elseif-elseif-...)-else
class synergia::mx_if
{
public:
  mx_if()
    : if_(), elseif_(), else_() 
  { }

  void assign_if    (mx_logic const & logic, mx_tree const & block);
  void assign_elseif(mx_logic const & logic, mx_tree const & block);
  void assign_else  (mx_tree const & block);
  bool interpret(MadX & mx);
  void print() const;

private:
  mx_if_block   if_;
  mx_if_block_v elseif_;
  mx_if_block   else_;
};

// while
class synergia::mx_while
{
public:
  mx_while()
    : while_()
  { }

  void assign(mx_logic const & logic, mx_tree const & block);
  bool interpret(MadX & mx);
  void print() const;

private:
  mx_if_block while_;
};

#endif
