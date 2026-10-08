#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include <CM.h>
#include <parser.h>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace CM {

  /*
   * Lexical rules.
   */

  struct line_comment :
    pegtl::seq< TAO_PEGTL_STRING("//"), pegtl::until< pegtl::eolf > > {};

  struct block_comment :
    pegtl::seq< TAO_PEGTL_STRING("/*"), pegtl::until< TAO_PEGTL_STRING("*/") > > {};

  struct ws :
    pegtl::star<
      pegtl::sor<
        pegtl::space,
        line_comment,
        block_comment
      >
    > {};

  template< typename R >
  struct tok :
    pegtl::seq< R, ws > {};

  template< char C >
  struct sym :
    tok< pegtl::one< C > > {};

  struct identifier_other :
    pegtl::sor<
      pegtl::alnum,
      pegtl::one< '_' >
    > {};

  template< typename Str >
  struct kw :
    tok< pegtl::seq< Str, pegtl::not_at< identifier_other > > > {};

  struct str_int      : TAO_PEGTL_STRING( "int" ) {};
  struct str_void     : TAO_PEGTL_STRING( "void" ) {};
  struct str_if       : TAO_PEGTL_STRING( "if" ) {};
  struct str_else     : TAO_PEGTL_STRING( "else" ) {};
  struct str_while    : TAO_PEGTL_STRING( "while" ) {};
  struct str_for      : TAO_PEGTL_STRING( "for" ) {};
  struct str_return   : TAO_PEGTL_STRING( "return" ) {};
  struct str_break    : TAO_PEGTL_STRING( "break" ) {};
  struct str_continue : TAO_PEGTL_STRING( "continue" ) {};
  struct str_new      : TAO_PEGTL_STRING( "new" ) {};
  struct str_true     : TAO_PEGTL_STRING( "true" ) {};
  struct str_do       : TAO_PEGTL_STRING( "do" ) {};
  struct str_false    : TAO_PEGTL_STRING( "false" ) {};

  struct kw_int      : kw< str_int > {};
  struct kw_void     : kw< str_void > {};
  struct kw_if       : kw< str_if > {};
  struct kw_else     : kw< str_else > {};
  struct kw_while    : kw< str_while > {};
  struct kw_for      : kw< str_for > {};
  struct kw_return   : kw< str_return > {};
  struct kw_break    : kw< str_break > {};
  struct kw_continue : kw< str_continue > {};
  struct kw_new      : kw< str_new > {};
  struct kw_true     : kw< str_true > {};
  struct kw_do       : kw< str_do > {};
  struct kw_false    : kw< str_false > {};

  struct keyword :
    pegtl::seq<
      pegtl::sor<
        str_int, str_void, str_if, str_else, str_while, str_for, str_return,
        str_break, str_continue, str_new, str_true, str_false, str_do
      >,
      pegtl::not_at< identifier_other >
    > {};

  struct identifier :
    pegtl::seq<
      pegtl::not_at< keyword >,
      pegtl::alpha,
      pegtl::star< identifier_other >
    > {};

  struct identifier_tok :
    tok< identifier > {};

  struct number :
    pegtl::plus< pegtl::digit > {};

  struct number_tok :
    tok< number > {};

  /*
   * Expressions.
   */

  struct expression;

  struct arguments :
    pegtl::opt< pegtl::list< expression, sym< ',' > > > {};

  struct call :
    pegtl::seq< identifier_tok, sym< '(' >, arguments, sym< ')' > > {};

  struct variable :
    pegtl::seq< identifier_tok > {};

  struct index_suffix :
    pegtl::seq< sym< '[' >, expression, sym< ']' > > {};

  struct new_array :
    pegtl::seq< kw_new, pegtl::must< kw_int, index_suffix >, pegtl::star< index_suffix > > {};

  struct true_literal :
    pegtl::seq< kw_true > {};

  struct false_literal :
    pegtl::seq< kw_false > {};

  struct parenthesized :
    pegtl::seq< sym< '(' >, expression, sym< ')' > > {};

  struct primary :
    pegtl::sor<
      number_tok,
      true_literal,
      false_literal,
      new_array,
      call,
      variable,
      parenthesized
    > {};

  struct postfix :
    pegtl::seq< primary, pegtl::star< index_suffix > > {};

  struct unary;

  struct negate :
    pegtl::seq< sym< '-' >, unary > {};

  struct logical_not :
    pegtl::seq< tok< pegtl::seq< pegtl::one< '!' >, pegtl::not_at< pegtl::one< '=' > > > >, unary > {};

  struct bitwise_not :
    pegtl::seq< sym< '~' >, unary > {};

  struct unary :
    pegtl::sor< negate, logical_not, bitwise_not, postfix > {};

  struct mul_op   : pegtl::one< '*', '/', '%' > {};
  struct add_op   : pegtl::one< '+', '-' > {};
  struct shift_op : pegtl::sor< TAO_PEGTL_STRING("<<"), TAO_PEGTL_STRING(">>") > {};
  struct rel_op   :
    pegtl::sor<
      TAO_PEGTL_STRING("<="),
      TAO_PEGTL_STRING(">="),
      pegtl::seq< pegtl::one< '<' >, pegtl::not_at< pegtl::one< '<' > > >,
      pegtl::seq< pegtl::one< '>' >, pegtl::not_at< pegtl::one< '>' > > >
    > {};
  struct eq_op    : pegtl::sor< TAO_PEGTL_STRING("=="), TAO_PEGTL_STRING("!=") > {};
  struct band_op  : pegtl::seq< pegtl::one< '&' >, pegtl::not_at< pegtl::one< '&', '=' > > > {};
  struct bxor_op  : pegtl::seq< pegtl::one< '^' >, pegtl::not_at< pegtl::one< '=' > > > {};
  struct bor_op   : pegtl::seq< pegtl::one< '|' >, pegtl::not_at< pegtl::one< '|', '=' > > > {};
  struct land_op  : TAO_PEGTL_STRING("&&") {};
  struct lor_op   : TAO_PEGTL_STRING("||") {};

  template< typename Op, typename Operand >
  struct left_assoc :
    pegtl::seq< Operand, pegtl::star< tok< Op >, Operand > > {};

  struct multiplicative : left_assoc< pegtl::seq< mul_op, pegtl::not_at< pegtl::one< '=' > > >, unary > {};
  struct additive       : left_assoc< pegtl::seq< add_op, pegtl::not_at< pegtl::one< '=', '+', '-' > > >, multiplicative > {};
  struct shift          : left_assoc< pegtl::seq< shift_op, pegtl::not_at< pegtl::one< '=' > > >, additive > {};
  struct relational     : left_assoc< rel_op, shift > {};
  struct equality       : left_assoc< eq_op, relational > {};
  struct bitwise_and    : left_assoc< band_op, equality > {};
  struct bitwise_xor    : left_assoc< bxor_op, bitwise_and > {};
  struct bitwise_or     : left_assoc< bor_op, bitwise_xor > {};
  struct logical_and    : left_assoc< land_op, bitwise_or > {};
  struct logical_or     : left_assoc< lor_op, logical_and > {};

  struct conditional :
    pegtl::seq< logical_or, pegtl::opt< sym< '?' >, expression, sym< ':' >, conditional > > {};

  struct expression :
    pegtl::seq< conditional > {};

  /*
   * Statements.
   */

  struct array_dim :
    pegtl::seq< sym< '[' >, sym< ']' > > {};

  struct type_spec :
    pegtl::seq<
      pegtl::sor< kw_int, kw_void >,
      pegtl::star< array_dim >
    > {};

  struct declarator :
    pegtl::seq< identifier_tok, pegtl::opt< sym< '=' >, expression > > {};

  struct declaration_body :
    pegtl::seq< type_spec, pegtl::list< declarator, sym< ',' > > > {};

  struct lvalue :
    pegtl::seq< identifier_tok, pegtl::star< index_suffix > > {};

  struct assign_op :
    pegtl::sor<
      pegtl::seq< pegtl::one< '=' >, pegtl::not_at< pegtl::one< '=' > > >,
      TAO_PEGTL_STRING("+="),
      TAO_PEGTL_STRING("-="),
      TAO_PEGTL_STRING("*="),
      TAO_PEGTL_STRING("&="),
      TAO_PEGTL_STRING("/="),
      TAO_PEGTL_STRING("%="),
      TAO_PEGTL_STRING("|="),
      TAO_PEGTL_STRING("^="),
      TAO_PEGTL_STRING("<<="),
      TAO_PEGTL_STRING(">>=")
    > {};

  struct incdec_op :
    pegtl::sor< TAO_PEGTL_STRING("++"), TAO_PEGTL_STRING("--") > {};

  struct assignment :
    pegtl::seq< lvalue, tok< assign_op >, expression > {};

  struct increment :
    pegtl::seq< lvalue, tok< incdec_op > > {};

  struct expression_statement :
    pegtl::seq< expression > {};

  struct simple_statement :
    pegtl::sor< assignment, increment, expression_statement > {};

  struct statement;

  struct block :
    pegtl::seq< sym< '{' >, pegtl::star< statement >, pegtl::must< sym< '}' > > > {};

  struct else_branch :
    pegtl::seq< kw_else, pegtl::must< statement > > {};

  struct if_statement :
    pegtl::seq< kw_if, pegtl::must< sym< '(' >, expression, sym< ')' >, statement >, pegtl::opt< else_branch > > {};

  struct while_statement :
    pegtl::seq< kw_while, pegtl::must< sym< '(' >, expression, sym< ')' >, statement > > {};

  struct for_init :
    pegtl::opt< pegtl::sor< declaration_body, simple_statement > > {};

  struct for_cond :
    pegtl::opt< expression > {};

  struct for_step :
    pegtl::opt< simple_statement > {};

  struct for_statement :
    pegtl::seq<
      kw_for,
      pegtl::must<
        sym< '(' >,
        for_init, sym< ';' >,
        for_cond, sym< ';' >,
        for_step, sym< ')' >,
        statement
      >
    > {};

  struct do_while_statement :
    pegtl::seq< kw_do, pegtl::must< statement, kw_while, sym< '(' >, expression, sym< ')' >, sym< ';' > > > {};

  struct break_statement :
    pegtl::seq< kw_break, pegtl::must< sym< ';' > > > {};

  struct continue_statement :
    pegtl::seq< kw_continue, pegtl::must< sym< ';' > > > {};

  struct return_statement :
    pegtl::seq< kw_return, pegtl::opt< expression >, pegtl::must< sym< ';' > > > {};

  struct declaration :
    pegtl::seq< declaration_body, pegtl::must< sym< ';' > > > {};

  struct statement :
    pegtl::sor<
      block,
      if_statement,
      while_statement,
      do_while_statement,
      for_statement,
      break_statement,
      continue_statement,
      return_statement,
      declaration,
      pegtl::seq< simple_statement, pegtl::must< sym< ';' > > >
    > {};

  /*
   * Functions.
   */

  struct parameter :
    pegtl::seq< type_spec, identifier_tok > {};

  struct parameters :
    pegtl::opt< pegtl::list< parameter, sym< ',' > > > {};

  struct function :
    pegtl::seq< type_spec, pegtl::must< identifier_tok, sym< '(' >, parameters, sym< ')' >, block > > {};

  struct grammar :
    pegtl::seq< ws, pegtl::star< function >, pegtl::must< pegtl::eof > > {};

  template< typename Rule >
  struct selector :
    parse_tree::selector<
      Rule,
      parse_tree::store_content::on<
        identifier,
        number,
        mul_op, add_op, shift_op, rel_op, eq_op, band_op, bxor_op, bor_op, land_op, lor_op,
        assign_op, incdec_op,
        kw_int, kw_void
      >,
      parse_tree::remove_content::on<
        function, parameters, parameter, type_spec, array_dim,
        block, if_statement, else_branch, while_statement, do_while_statement, for_statement,
        for_init, for_cond, for_step,
        break_statement, continue_statement, return_statement,
        declaration_body, declarator, assignment, increment, expression_statement, lvalue,
        call, arguments, variable, new_array, index_suffix,
        true_literal, false_literal, negate, logical_not, bitwise_not
      >,
      parse_tree::fold_one::on<
        postfix,
        multiplicative, additive, shift, relational, equality,
        bitwise_and, bitwise_xor, bitwise_or, logical_and, logical_or, conditional
      >
    > {};

  /*
   * Parse tree to AST.
   */

  using Node = parse_tree::node;

  static Position position_of(const Node &n) {
    auto p = n.begin();
    return Position{static_cast<int64_t>(p.line), static_cast<int64_t>(p.column)};
  }

  template< typename T >
  static T* at(T* node, const Node &n) {
    node->pos = position_of(n);
    return node;
  }

  static BinOp binop_from_string(const std::string &s) {
    if (s == "+")  return add;
    if (s == "-")  return sub;
    if (s == "*")  return mul;
    if (s == "&")  return band;
    if (s == "/")  return div;
    if (s == "%")  return mod;
    if (s == "|")  return bor;
    if (s == "^")  return bxor;
    if (s == "<<") return shl;
    if (s == ">>") return shr;
    if (s == "<")  return lt;
    if (s == "<=") return le;
    if (s == ">")  return gt;
    if (s == ">=") return ge;
    if (s == "==") return eq;
    if (s == "!=") return ne;
    if (s == "&&") return land;
    if (s == "||") return lor;
    throw std::invalid_argument("unknown operator " + s);
  }

  static Expression* build_expression(const Node &n);
  static Statement* build_statement(const Node &n);

  static Type build_type(const Node &n) {
    Type t = n.children[0]->is_type< kw_void >() ? void_type() : int_type();
    t.dims = static_cast<int64_t>(n.children.size()) - 1;
    return t;
  }

  static std::vector<Expression*> build_indexes(const Node &n, size_t from) {
    std::vector<Expression*> idxs;
    for (size_t k = from; k < n.children.size(); k++) {
      idxs.push_back(build_expression(*n.children[k]->children[0]));
    }
    return idxs;
  }

  static Expression* build_expression(const Node &n) {
    if (n.is_type< number >()) {
      return at(new Number(std::stoll(n.string())), n);
    }
    if (n.is_type< true_literal >()) {
      return at(new Number(1), n);
    }
    if (n.is_type< false_literal >()) {
      return at(new Number(0), n);
    }
    if (n.is_type< variable >()) {
      return at(new Variable(n.children[0]->string()), n);
    }
    if (n.is_type< call >()) {
      std::vector<Expression*> args;
      if (n.children.size() > 1) {
        for (auto &a : n.children[1]->children) args.push_back(build_expression(*a));
      }
      return at(new Call(n.children[0]->string(), std::move(args)), n);
    }
    if (n.is_type< new_array >()) {
      return at(new NewArray(build_indexes(n, 1)), n);
    }
    if (n.is_type< postfix >()) {
      Expression* base = build_expression(*n.children[0]);
      return at(new Index(base, build_indexes(n, 1)), n);
    }
    if (n.is_type< negate >()) {
      return at(new Unary(neg, build_expression(*n.children[0])), n);
    }
    if (n.is_type< logical_not >()) {
      return at(new Unary(lnot, build_expression(*n.children[0])), n);
    }
    if (n.is_type< bitwise_not >()) {
      return at(new Unary(bnot, build_expression(*n.children[0])), n);
    }
    if (n.is_type< conditional >()) {
      return at(new Conditional(build_expression(*n.children[0]), build_expression(*n.children[1]), build_expression(*n.children[2])), n);
    }
    if (n.children.size() >= 3) {
      Expression* lhs = build_expression(*n.children[0]);
      for (size_t k = 1; k + 1 < n.children.size(); k += 2) {
        Expression* rhs = build_expression(*n.children[k + 1]);
        auto* b = new Binary(binop_from_string(n.children[k]->string()), lhs, rhs);
        b->pos = position_of(*n.children[k]);
        lhs = b;
      }
      return lhs;
    }
    throw std::runtime_error("unexpected expression node " + std::string(n.type));
  }

  static Statement* build_simple(const Node &n) {
    if (n.is_type< assignment >() || n.is_type< increment >()) {
      const Node &lv = *n.children[0];
      auto* target = at(new Variable(lv.children[0]->string()), lv);
      auto idxs = build_indexes(lv, 1);
      std::string op = n.children[1]->string();
      if (n.is_type< increment >()) {
        auto* one = at(new Number(1), *n.children[1]);
        return at(new Assign(target, idxs, true, op == "++" ? add : sub, one), n);
      }
      Expression* value = build_expression(*n.children[2]);
      if (op == "=") {
        return at(new Assign(target, idxs, false, add, value), n);
      }
      return at(new Assign(target, idxs, true, binop_from_string(op.substr(0, op.size() - 1)), value), n);
    }
    if (n.is_type< expression_statement >()) {
      return at(new ExpressionStatement(build_expression(*n.children[0])), n);
    }
    if (n.is_type< declaration_body >()) {
      Type type = build_type(*n.children[0]);
      std::vector<std::pair<std::string, Expression*>> declarators;
      for (size_t k = 1; k < n.children.size(); k++) {
        const Node &d = *n.children[k];
        Expression* init = d.children.size() > 1 ? build_expression(*d.children[1]) : nullptr;
        declarators.emplace_back(d.children[0]->string(), init);
      }
      return at(new Declaration(type, std::move(declarators)), n);
    }
    return build_statement(n);
  }

  static Block* build_block(const Node &n) {
    std::vector<Statement*> stmts;
    for (auto &c : n.children) stmts.push_back(build_statement(*c));
    return at(new Block(std::move(stmts)), n);
  }

  static Statement* build_statement(const Node &n) {
    if (n.is_type< block >()) {
      return build_block(n);
    }
    if (n.is_type< if_statement >()) {
      Statement* else_branch_stmt = nullptr;
      if (n.children.size() > 2) {
        else_branch_stmt = build_statement(*n.children[2]->children[0]);
      }
      return at(new If(build_expression(*n.children[0]), build_statement(*n.children[1]), else_branch_stmt), n);
    }
    if (n.is_type< while_statement >()) {
      return at(new While(build_expression(*n.children[0]), build_statement(*n.children[1])), n);
    }
    if (n.is_type< for_statement >()) {
      const Node &init = *n.children[0];
      const Node &cond = *n.children[1];
      const Node &step = *n.children[2];
      Statement* init_stmt = init.children.empty() ? nullptr : build_simple(*init.children[0]);
      Expression* cond_expr = cond.children.empty() ? nullptr : build_expression(*cond.children[0]);
      Statement* step_stmt = step.children.empty() ? nullptr : build_simple(*step.children[0]);
      return at(new For(init_stmt, cond_expr, step_stmt, build_statement(*n.children[3])), n);
    }
    if (n.is_type< do_while_statement >()) {
      return at(new DoWhile(build_statement(*n.children[0]), build_expression(*n.children[1])), n);
    }
    if (n.is_type< break_statement >()) {
      return at(new Break(), n);
    }
    if (n.is_type< continue_statement >()) {
      return at(new Continue(), n);
    }
    if (n.is_type< return_statement >()) {
      Expression* value = n.children.empty() ? nullptr : build_expression(*n.children[0]);
      return at(new Return(value), n);
    }
    return build_simple(n);
  }

  static Function* build_function(const Node &n) {
    auto* f = new Function();
    f->return_type = build_type(*n.children[0]);
    f->name = n.children[1]->string();
    f->pos = position_of(n);
    for (auto &p : n.children[2]->children) {
      f->params.emplace_back(build_type(*p->children[0]), p->children[1]->string());
    }
    f->body = build_block(*n.children[3]);
    return f;
  }

  template< typename Input >
  static Program parse_input(Input &input) {
    if (pegtl::analyze< grammar >() != 0) {
      std::cerr << "There are problems with the grammar" << std::endl;
      exit(1);
    }

    std::unique_ptr<Node> root;
    try {
      root = parse_tree::parse< grammar, selector >(input);
    } catch (const parse_error &e) {
      const auto p = e.positions().front();
      std::cerr << p.source << ":" << p.line << ":" << p.column << ": error: syntax error" << std::endl;
      exit(1);
    }

    Program p;
    for (auto &f : root->children) {
      p.functions.push_back(build_function(*f));
    }
    return p;
  }

  Program parse_file(char *fileName) {
    file_input<> input(fileName);
    return parse_input(input);
  }

  Program parse_string(const std::string &source, const std::string &name) {
    memory_input<> input(source, name);
    return parse_input(input);
  }

}
