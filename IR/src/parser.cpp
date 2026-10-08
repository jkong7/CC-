/*
 * SUGGESTION FROM THE CC TEAM:
 * double check the order of actions that are fired.
 * You can do this in (at least) two ways:
 * 1) by using gdb adding breakpoints to actions
 * 2) by adding printing statements in each action
 *
 * For 2), we suggest writing the code to make it straightforward to enable/disable all of them
 * (e.g., assuming shouldIPrint is a global variable
 *    if (shouldIPrint) std::cerr << "MY OUTPUT" << std::endl;
 * )
 */
#include <sched.h>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>
#include <iterator>
#include <unordered_map>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#include <IR.h>
#include <parser.h>
#include <helper.h>

static constexpr bool PARSER_DEBUG = false;

#define PARSER_PRINT(msg) \
  do { if (PARSER_DEBUG) std::cerr << msg << std::endl; } while (0)

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace IR {

  /*
   * Tokens parsed
   */
  std::vector<Item *> parsed_items;

  int64_t cur_int64_dims = 0;
  Type cur_type;

  Function* current_function = nullptr;
  BasicBlock* current_bb = nullptr;

  bool parsing_params = false;
  bool parsing_indexes = false;
  size_t index_begin = 0;
  size_t args_begin = 0;

  OP last_op;
  CallType last_call_type;

  /*
   * Grammar rules from now on.
   */

  // Keywords

  struct str_arrow : TAO_PEGTL_STRING( "<-" ) {};

  struct str_pars_left_paren  : TAO_PEGTL_STRING("(") {};
  struct str_pars_right_paren : TAO_PEGTL_STRING(")") {};

  struct str_args_left_paren  : TAO_PEGTL_STRING("(") {};
  struct str_args_right_paren : TAO_PEGTL_STRING(")") {};

  struct str_left_brace  : TAO_PEGTL_STRING("{") {};
  struct str_right_brace : TAO_PEGTL_STRING("}") {};

  struct str_type_left_bracket  : TAO_PEGTL_STRING("[") {};
  struct str_type_right_bracket : TAO_PEGTL_STRING("]") {};

  struct str_index_left_bracket  : TAO_PEGTL_STRING("[") {};
  struct str_index_right_bracket : TAO_PEGTL_STRING("]") {};

  struct str_comma : TAO_PEGTL_STRING(",") {};

  // Arith op keywords

  struct str_plus        : TAO_PEGTL_STRING( "+" ) {};
  struct str_minus       : TAO_PEGTL_STRING( "-" ) {};
  struct str_times       : TAO_PEGTL_STRING( "*" ) {};
  struct str_at          : TAO_PEGTL_STRING( "&" ) {};
  struct str_left_shift  : TAO_PEGTL_STRING( "<<" ) {};
  struct str_right_shift : TAO_PEGTL_STRING( ">>" ) {};

  // Comp op

  struct str_less_than         : TAO_PEGTL_STRING( "<" ) {};
  struct str_less_than_equal   : TAO_PEGTL_STRING( "<=" ) {};
  struct str_equal             : TAO_PEGTL_STRING( "=" ) {};
  struct str_greater_than      : TAO_PEGTL_STRING( ">" ) {};
  struct str_greater_than_equal: TAO_PEGTL_STRING( ">=" ) {};

  // length

  struct str_length : TAO_PEGTL_STRING( "length" ) {};

  // break

  struct str_break : TAO_PEGTL_STRING( "br" ) {};

  // Function keywords

  struct str_define       : TAO_PEGTL_STRING( "define" ) {};
  struct str_call         : TAO_PEGTL_STRING( "call" ) {};
  struct str_print        : TAO_PEGTL_STRING( "print" ) {};
  struct str_input        : TAO_PEGTL_STRING( "input" ) {};
  struct str_tuple_error  : TAO_PEGTL_STRING( "tuple-error" ) {};
  struct str_tensor_error : TAO_PEGTL_STRING( "tensor-error" ) {};
  struct str_return       : TAO_PEGTL_STRING( "return" ) {};

  // Type keywords

  struct str_void  : TAO_PEGTL_STRING( "void" ) {};
  struct str_int64 : TAO_PEGTL_STRING( "int64" ) {};
  struct str_tuple : TAO_PEGTL_STRING( "tuple" ) {};
  struct str_code  : TAO_PEGTL_STRING( "code" ) {};

  // Container keywords

  struct str_new   : TAO_PEGTL_STRING( "new" ) {};
  struct str_array : TAO_PEGTL_STRING( "Array" ) {};
  struct str_Tuple : TAO_PEGTL_STRING( "Tuple" ) {};

  struct op_rule :
    pegtl::sor<
      str_plus,
      str_minus,
      str_times,
      str_at,
      str_left_shift,
      str_right_shift,
      str_less_than_equal,
      str_less_than,
      str_equal,
      str_greater_than_equal,
      str_greater_than
    > {};

  struct name :
    pegtl::seq<
      pegtl::plus<
        pegtl::sor<
          pegtl::alpha,
          pegtl::one< '_' >
        >
      >,
      pegtl::star<
        pegtl::sor<
          pegtl::alpha,
          pegtl::one< '_' >,
          pegtl::digit
        >
      >
    > {};

  struct variable_rule :
    pegtl::seq<
      pegtl::one< '%' >,
      name
    > {};

  struct bb_label_rule :
    pegtl::seq<
      pegtl::one< ':' >,
      name
    > {};

  struct label_piece_rule :
    pegtl::seq<
      pegtl::one< ':' >,
      name
    > {};

  struct function_name_rule :
    pegtl::seq<
      pegtl::one< '@' >,
      name
    > {};

  struct function_name_piece_rule :
    pegtl::seq<
      pegtl::one< '@' >,
      name
    > {};

  struct number :
    pegtl::seq<
      pegtl::opt<
        pegtl::sor<
          pegtl::one< '-' >,
          pegtl::one< '+' >
        >
      >,
      pegtl::plus<
        pegtl::digit
      >
    > {};

  struct u_rule :
    pegtl::sor<
      variable_rule,
      function_name_piece_rule
    > {};

  struct t_rule :
    pegtl::sor<
      variable_rule,
      number
    > {};

  struct s_rule :
    pegtl::sor<
      t_rule,
      function_name_piece_rule
    > {};

  /*
   * Separators.
   */

  struct spaces :
    pegtl::star<
      pegtl::sor<
        pegtl::one< ' ' >,
        pegtl::one< '\t' >
      >
    > {};

  struct seps :
    pegtl::star<
      pegtl::seq<
        spaces,
        pegtl::eol
      >
    > {};

  struct comment :
    pegtl::disable<
      pegtl::seq< TAO_PEGTL_STRING("//"), pegtl::until< pegtl::eolf > >
    > {};

  struct seps_with_comments :
    pegtl::star<
      pegtl::seq<
        spaces,
        pegtl::sor<
          pegtl::eol,
          comment
        >
      >
    > {};

  // ---------------------------
  // CHANGES (necessary):
  //   - split args into non-empty (args1_rule) and optional (args0_rule)
  // ---------------------------
  struct args1_rule :
    pegtl::seq<
      t_rule,
      pegtl::star<
        spaces, str_comma, spaces, t_rule
      >
    > {};

  struct args0_rule :
    pegtl::opt< args1_rule > {};

  struct type_rule :
    pegtl::sor<
      pegtl::seq<
        str_int64,
        pegtl::star<
          str_type_left_bracket,
          str_type_right_bracket
        >
      >,
      str_tuple,
      str_code
    > {};

  struct vars_rule :
    pegtl::opt<
      pegtl::seq<
        type_rule,
        spaces,
        variable_rule,
        pegtl::star<
          spaces, str_comma, spaces, type_rule, spaces, variable_rule
        >
      >
    > {};

  struct callee_rule :
    pegtl::sor<
      u_rule,
      str_print,
      str_input,
      str_tuple_error,
      str_tensor_error
    > {};

  struct T_rule :
    pegtl::sor<
      type_rule,
      str_void
    > {};


  struct one_index_rule :
    pegtl::seq<
      str_index_left_bracket,
      t_rule,
      str_index_right_bracket
    > {};

  struct index_list_rule :
    pegtl::plus< one_index_rule > {};

  // Instruction rules

  struct Instruction_initialize_rule :
    pegtl::seq<
      type_rule,
      spaces,
      variable_rule
    > {};

  struct Instruction_assignment_rule :
    pegtl::seq<
      variable_rule,
      spaces,
      str_arrow,
      spaces,
      s_rule
    > {};

  struct Instruction_op_rule :
    pegtl::seq<
      variable_rule,
      spaces,
      str_arrow,
      spaces,
      t_rule,
      spaces,
      op_rule,
      spaces,
      t_rule
    > {};

  struct Instruction_index_load_rule :
    pegtl::seq<
      variable_rule,
      spaces,
      str_arrow,
      spaces,
      variable_rule,
      index_list_rule
    > {};

  struct Instruction_index_store_rule :
    pegtl::seq<
      variable_rule,
      index_list_rule,
      spaces,
      str_arrow,
      spaces,
      s_rule
    > {};

  struct Instruction_length_t_rule :
    pegtl::seq<
      variable_rule,
      spaces,
      str_arrow,
      spaces,
      str_length,
      spaces,
      variable_rule,
      spaces,
      t_rule
    > {};

  struct Instruction_length_rule :
    pegtl::seq<
      variable_rule,
      spaces,
      str_arrow,
      spaces,
      str_length,
      spaces,
      variable_rule
    > {};

  struct Instruction_call_rule :
    pegtl::seq<
      str_call,
      spaces,
      callee_rule,
      spaces,
      str_args_left_paren,
      spaces,
      args0_rule,
      spaces,
      str_args_right_paren
    > {};

  struct Instruction_call_assignment_rule :
    pegtl::seq<
      variable_rule,
      spaces,
      str_arrow,
      spaces,
      str_call,
      spaces,
      callee_rule,
      spaces,
      str_args_left_paren,
      spaces,
      args0_rule,
      spaces,
      str_args_right_paren
    > {};

  struct Instruction_new_array_rule :
    pegtl::seq<
      variable_rule,
      spaces,
      str_arrow,
      spaces,
      str_new,
      spaces,
      str_array,
      str_args_left_paren,
      args1_rule,
      str_args_right_paren
    > {};

  struct Instruction_new_tuple_rule :
    pegtl::seq<
      variable_rule,
      spaces,
      str_arrow,
      spaces,
      str_new,
      spaces,
      str_Tuple,
      spaces,
      str_args_left_paren,
      spaces,
      t_rule,
      spaces,
      str_args_right_paren
    > {};

  struct Instruction_break_uncond_rule :
    pegtl::seq<
      str_break,
      spaces,
      label_piece_rule
    > {};

  struct Instruction_break_cond_rule :
    pegtl::seq<
      str_break,
      spaces,
      t_rule,
      spaces,
      label_piece_rule,
      spaces,
      label_piece_rule
    > {};

  struct Instruction_return_rule :
    pegtl::seq<
      str_return
    > {};

  struct Instruction_return_t_rule :
    pegtl::seq<
      str_return,
      spaces,
      t_rule
    > {};

  struct Instruction_rule :
    pegtl::sor<
      pegtl::seq< pegtl::at< Instruction_initialize_rule >            , Instruction_initialize_rule            >,
      pegtl::seq< pegtl::at< Instruction_op_rule >                    , Instruction_op_rule                    >,
      pegtl::seq< pegtl::at< Instruction_index_load_rule >            , Instruction_index_load_rule            >,
      pegtl::seq< pegtl::at< Instruction_assignment_rule >            , Instruction_assignment_rule            >,
      pegtl::seq< pegtl::at< Instruction_index_store_rule >           , Instruction_index_store_rule           >,
      pegtl::seq< pegtl::at< Instruction_length_t_rule >              , Instruction_length_t_rule              >,
      pegtl::seq< pegtl::at< Instruction_length_rule >                , Instruction_length_rule                >,
      pegtl::seq< pegtl::at< Instruction_call_assignment_rule >       , Instruction_call_assignment_rule       >,
      pegtl::seq< pegtl::at< Instruction_call_rule >                  , Instruction_call_rule                  >,
      pegtl::seq< pegtl::at< Instruction_new_array_rule >             , Instruction_new_array_rule             >,
      pegtl::seq< pegtl::at< Instruction_new_tuple_rule >             , Instruction_new_tuple_rule             >
    > {};

  struct Terminator_instruction_rule :
    pegtl::sor<
      pegtl::seq< pegtl::at< Instruction_break_uncond_rule >          , Instruction_break_uncond_rule          >,
      pegtl::seq< pegtl::at< Instruction_break_cond_rule >            , Instruction_break_cond_rule            >,
      pegtl::seq< pegtl::at< Instruction_return_t_rule >              , Instruction_return_t_rule              >,
      pegtl::seq< pegtl::at< Instruction_return_rule >                , Instruction_return_rule                >
    > {};

  struct Instructions_rule :
    pegtl::star<
      pegtl::seq<
        seps_with_comments,
        pegtl::bol,
        spaces,
        Instruction_rule,
        spaces,
        pegtl::opt< comment >,
        seps_with_comments
      >
    > {};

  struct basic_block_rule :
    pegtl::seq<
      seps_with_comments,
      bol,
      spaces,
      bb_label_rule,
      Instructions_rule,
      seps_with_comments,
      pegtl::bol, spaces,
      Terminator_instruction_rule,
      spaces, pegtl::opt< comment >,
      seps_with_comments
    > {};

  struct Function_rule :
    pegtl::seq<
      seps_with_comments,
      pegtl::bol,
      spaces,
      str_define,
      spaces,
      T_rule,
      spaces,
      function_name_rule,
      spaces,
      str_pars_left_paren,
      spaces,
      vars_rule,
      spaces,
      str_pars_right_paren,
      seps_with_comments,
      spaces,
      str_left_brace,
      seps_with_comments,
      pegtl::plus< basic_block_rule >,
      seps_with_comments,
      spaces,
      str_right_brace
    > {};

  struct Program_rule :
    pegtl::seq<
      seps_with_comments,
      pegtl::plus< Function_rule >,
      seps_with_comments,
      pegtl::eof
    > {};

  struct grammar :
    pegtl::must< Program_rule >
    {};

  /*
   * Actions attached to grammar rules.
   */

  template< typename Rule >
  struct action : pegtl::nothing< Rule > {};

  // Function rule

  template<> struct action< str_define > {
    template<typename Input>
    static void apply(const Input&, Program& p) {
      current_function = new Function();
      p.functions.push_back(current_function);
      PARSER_PRINT("define");
    }
  };

  template<> struct action< str_type_right_bracket > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      cur_int64_dims++;
    }
  };

  template<> struct action< str_int64 > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      cur_int64_dims = 0;
      cur_type = Type::int64;
    }
  };

  template<> struct action< str_tuple > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      cur_int64_dims = 0;
      cur_type = Type::tuple;
    }
  };

  template<> struct action< str_code > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      cur_int64_dims = 0;
      cur_type = Type::code;
    }
  };


  template<> struct action< str_void > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      cur_int64_dims = 0;
      cur_type = Type::void_;
    }
  };

  template<> struct action< T_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      current_function->return_type = cur_type;
      current_function->dims = cur_int64_dims;
      cur_int64_dims = 0;
      PARSER_PRINT("T rule"); 
    }
  };

  template<> struct action< function_name_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      current_function->name = in.string();
      PARSER_PRINT("Function name rule"); 
    }
  };

  template<> struct action< str_pars_left_paren > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      parsing_params = true;
    }
  };

  template<> struct action< str_pars_right_paren > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      parsing_params = false;
    }
  };

  template<> struct action< str_args_left_paren > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      args_begin = parsed_items.size();
    }
  };

  template<> struct action< str_index_left_bracket > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      if (!parsing_indexes) {
        parsing_indexes = true;
        index_begin = parsed_items.size();   
      }
    }
  };


  // Basic block actions
  template<> struct action< bb_label_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      current_bb = new BasicBlock();
      current_bb->label_ = new Label(in.string());
      PARSER_PRINT("bb_label");
    }
  };

  template<> struct action< Terminator_instruction_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      current_function->basic_blocks.push_back(current_bb);
    }
  };

  // Single push actions

  // Push variable
  template<> struct action< variable_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto v = new Variable(in.string());
      parsed_items.push_back(v);

      if (parsing_params && current_function) {
        current_function->var_arguments.push_back(v);
        current_function->variable_types[v->var_] = {cur_type, cur_int64_dims}; 
        cur_int64_dims = 0;
      }
    }
  };

  // Push a number
  template<> struct action< number > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto n = new Number(std::stoll(in.string()));
      parsed_items.push_back(n);
    }
  };

  // Push a label
  template<> struct action< label_piece_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto l = new Label(in.string());
      parsed_items.push_back(l);
    }
  };

  // Push a function name piece
  template<> struct action< function_name_piece_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto f = new Func(in.string());
      parsed_items.push_back(f);
    }
  };

  // Push OP
  template<> struct action< op_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      last_op = op_from_string(in.string());
    }
  };

  // Push call type
  template<> struct action< u_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      last_call_type = CallType::ir;
    }
  };

  template<> struct action< str_print > {
    template<typename Input>
    static void apply(const Input&, Program&) { last_call_type = CallType::print; }
  };

  template<> struct action< str_input > {
    template<typename Input>
    static void apply(const Input&, Program&) { last_call_type = CallType::input; }
  };

  template<> struct action< str_tuple_error > {
    template<typename Input>
    static void apply(const Input&, Program&) { last_call_type = CallType::tuple_error; }
  };

  template<> struct action< str_tensor_error > {
    template<typename Input>
    static void apply(const Input&, Program&) { last_call_type = CallType::tensor_error; }
  };

  // Actions to build instruction nodes

  template<> struct action< Instruction_initialize_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      auto* var = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();
      current_function->variable_types[var->var_] = std::make_pair(cur_type, cur_int64_dims);
      cur_int64_dims = 0;

      PARSER_PRINT("Initialize instruction");
    }
  };

  template<> struct action< Instruction_assignment_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      Item* src = parsed_items.back(); parsed_items.pop_back();
      auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

      current_bb->instructions.push_back(new Instruction_assignment(dst, src));

      PARSER_PRINT("Assignment instruction");
    }
  };

  template<> struct action< Instruction_op_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      Item* rhs = parsed_items.back(); parsed_items.pop_back();
      Item* lhs = parsed_items.back(); parsed_items.pop_back();
      auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

      current_bb->instructions.push_back(new Instruction_op(dst, lhs, last_op, rhs));

      PARSER_PRINT("Op instruction");
    }
  };

  template<> struct action< Instruction_index_load_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {

      std::vector<Item*> idxs;
      if (!parsing_indexes) {
        index_begin = parsed_items.size();
      }
      for (size_t i = index_begin; i < parsed_items.size(); ++i) idxs.push_back(parsed_items[i]);
      parsed_items.resize(index_begin);

      parsing_indexes = false;
      index_begin = 0;

      auto* src = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();
      auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

      current_bb->instructions.push_back(
        new Instruction_index_load(dst, src, std::move(idxs))
      );

      PARSER_PRINT("Index load instruction");
    }
  };

  template<> struct action< Instruction_index_store_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {

      Item* src = parsed_items.back(); 
      parsed_items.pop_back();

      std::vector<Item*> idxs;
      if (!parsing_indexes) {
        index_begin = parsed_items.size();
      }
      for (size_t i = index_begin; i < parsed_items.size(); ++i) idxs.push_back(parsed_items[i]);
      parsed_items.resize(index_begin);

      parsing_indexes = false;
      index_begin = 0;

      auto* dst = static_cast<Variable*>(parsed_items.back()); 
      parsed_items.pop_back();

      current_bb->instructions.push_back(
        new Instruction_index_store(dst, std::move(idxs), src)
      );

      PARSER_PRINT("Index store instruction");
    }
  };

  template<> struct action< Instruction_length_t_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      Item* t = parsed_items.back(); parsed_items.pop_back();
      auto* src = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();
      auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

      current_bb->instructions.push_back(new Instruction_length_t(dst, src, t));

      PARSER_PRINT("Length t instruction ");
    }
  };

  template<> struct action< Instruction_length_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      auto* src = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();
      auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

      current_bb->instructions.push_back(new Instruction_length(dst, src));

      PARSER_PRINT("Length instruction");
    }
  };

  template<> struct action< Instruction_call_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      // args are parsed_items[callee (if ir calltype), args_begin..end)

      std::vector<Item*> args;
      for (size_t i = args_begin; i < parsed_items.size(); ++i) args.push_back(parsed_items[i]);
      parsed_items.resize(args_begin);

      Item* callee = nullptr;
      if (last_call_type == CallType::ir) {
        callee = parsed_items.back(); parsed_items.pop_back();
      }
      current_bb->instructions.push_back(new Instruction_call(last_call_type, callee, std::move(args)));
      PARSER_PRINT("Call instruction");
    }
  };

  template<> struct action< Instruction_call_assignment_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      std::vector<Item*> args;
      for (size_t i = args_begin; i < parsed_items.size(); ++i) args.push_back(parsed_items[i]);
      parsed_items.resize(args_begin);

      Item* callee = nullptr;
      if (last_call_type == CallType::ir) {
        callee = parsed_items.back(); parsed_items.pop_back();
      }
      auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

      current_bb->instructions.push_back(new Instruction_call_assignment(dst, last_call_type, callee, std::move(args)));
      PARSER_PRINT("Call assignment instruction");
    }
  };

  template<> struct action< Instruction_new_array_rule > {

    template<typename Input>
    static void apply0(const Input&, Program&) {
      args_begin = parsed_items.size();
    }

    template<typename Input>
    static void apply(const Input&, Program&) {
      std::vector<Item*> args;
      for (size_t i = args_begin; i < parsed_items.size(); ++i)
        args.push_back(parsed_items[i]);

      parsed_items.resize(args_begin);

      auto* dst = static_cast<Variable*>(parsed_items.back());
      parsed_items.pop_back();

      current_bb->instructions.push_back(
        new Instruction_new_array(dst, std::move(args))
      );

      PARSER_PRINT("New array instruction");
    }
  };

  template<> struct action< Instruction_new_tuple_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      Item* t = parsed_items.back(); parsed_items.pop_back();
      auto* dst = static_cast<Variable*>(parsed_items.back()); parsed_items.pop_back();

      current_bb->instructions.push_back(new Instruction_new_tuple(dst, t));

      PARSER_PRINT("New tuple instruction");
    }
  };

  template<> struct action< Instruction_break_uncond_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      auto* l = static_cast<Label*>(parsed_items.back()); parsed_items.pop_back();
      current_bb->instructions.push_back(new Instruction_break_uncond(l));
      current_bb->succ_labels.push_back(l->label_);

      PARSER_PRINT("Break uncond instruction");
    }
  };

  template<> struct action< Instruction_break_cond_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      auto* l2 = static_cast<Label*>(parsed_items.back()); parsed_items.pop_back();
      auto* l1 = static_cast<Label*>(parsed_items.back()); parsed_items.pop_back();
      Item* t = parsed_items.back(); parsed_items.pop_back();

      current_bb->instructions.push_back(new Instruction_break_cond(t, l1, l2));
      current_bb->succ_labels.push_back(l1->label_);
      current_bb->succ_labels.push_back(l2->label_);

      PARSER_PRINT("Break cond instruction");
    }
  };

  template<> struct action< Instruction_return_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      current_bb->instructions.push_back(new Instruction_return());
      PARSER_PRINT("Return instruction");
    }
  };

  template<> struct action< Instruction_return_t_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      Item* ret = parsed_items.back(); parsed_items.pop_back();

      current_bb->instructions.push_back(new Instruction_return_t(ret));
      PARSER_PRINT("Return t instruction");
    }
  };

  Program parse_file(char *fileName) {
    parsed_items.clear();        
    current_function = nullptr;
    current_bb = nullptr;
    parsing_params = false;
    args_begin = 0;
    cur_int64_dims = 0;

    /*
     * Check the grammar for some possible issues.
     */
    if (pegtl::analyze< grammar >() != 0) {
      std::cerr << "There are problems with the grammar" << std::endl;
      exit(1);
    }

    /*
     * Parse.
     */
    file_input<> fileInput(fileName);
    Program p;
    parse< grammar, action >(fileInput, p);

    return p;
  }
}