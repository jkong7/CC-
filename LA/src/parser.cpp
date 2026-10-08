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

#include <LA.h>
#include <parser.h>
#include <helper.h>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace LA {

  /*
   * Tokens parsed
   */
  std::vector<Item *> parsed_items;

  int64_t cur_int64_dims = 0;
  Type cur_type;

  Function* current_function = nullptr;

  bool parsing_params = false;
  bool parsing_indexes = false;
  size_t index_begin = 0;
  size_t args_begin = 0;

  OP last_op;
  CallType last_call_type;

  template< typename Input >
  static void add_instruction(const Input& in, Instruction* i) {
    i->line_ = static_cast<int64_t>(in.position().line);
    current_function->instructions.push_back(i);
  }

  static std::vector<Item*> pop_from(size_t begin) {
    std::vector<Item*> items(parsed_items.begin() + begin, parsed_items.end());
    parsed_items.resize(begin);
    return items;
  }

  template< typename T >
  static T* pop_item() {
    auto* item = static_cast<T*>(parsed_items.back());
    parsed_items.pop_back();
    return item;
  }

  /*
   * Grammar rules from now on.
   */

  struct identifier_char :
    pegtl::sor<
      pegtl::alpha,
      pegtl::one< '_' >,
      pegtl::digit
    > {};

  template< typename Str >
  struct keyword :
    pegtl::seq<
      Str,
      pegtl::not_at< identifier_char >
    > {};

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

  struct str_plus        : TAO_PEGTL_STRING( "+" ) {};
  struct str_minus       : TAO_PEGTL_STRING( "-" ) {};
  struct str_times       : TAO_PEGTL_STRING( "*" ) {};
  struct str_at          : TAO_PEGTL_STRING( "&" ) {};
  struct str_left_shift  : TAO_PEGTL_STRING( "<<" ) {};
  struct str_right_shift : TAO_PEGTL_STRING( ">>" ) {};

  struct str_less_than         : TAO_PEGTL_STRING( "<" ) {};
  struct str_less_than_equal   : TAO_PEGTL_STRING( "<=" ) {};
  struct str_equal             : TAO_PEGTL_STRING( "=" ) {};
  struct str_greater_than      : TAO_PEGTL_STRING( ">" ) {};
  struct str_greater_than_equal: TAO_PEGTL_STRING( ">=" ) {};

  struct str_length       : keyword< TAO_PEGTL_STRING( "length" ) > {};
  struct str_break        : keyword< TAO_PEGTL_STRING( "br" ) > {};
  struct str_print        : keyword< TAO_PEGTL_STRING( "print" ) > {};
  struct str_input        : keyword< TAO_PEGTL_STRING( "input" ) > {};
  struct str_tuple_error  : keyword< TAO_PEGTL_STRING( "tuple-error" ) > {};
  struct str_tensor_error : keyword< TAO_PEGTL_STRING( "tensor-error" ) > {};
  struct str_return       : keyword< TAO_PEGTL_STRING( "return" ) > {};

  struct str_void  : keyword< TAO_PEGTL_STRING( "void" ) > {};
  struct str_int64 : keyword< TAO_PEGTL_STRING( "int64" ) > {};
  struct str_tuple : keyword< TAO_PEGTL_STRING( "tuple" ) > {};
  struct str_code  : keyword< TAO_PEGTL_STRING( "code" ) > {};

  struct str_new   : keyword< TAO_PEGTL_STRING( "new" ) > {};
  struct str_array : keyword< TAO_PEGTL_STRING( "Array" ) > {};
  struct str_Tuple : keyword< TAO_PEGTL_STRING( "Tuple" ) > {};

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
        identifier_char
      >
    > {};

  struct name_rule :
    name {};

  struct function_name_rule :
    name {};

  struct label_rule :
    pegtl::seq<
      pegtl::one< ':' >,
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

  struct t_rule :
    pegtl::sor<
      name_rule,
      number
    > {};

  struct s_rule :
    t_rule {};

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

  struct param_rule :
    pegtl::seq<
      type_rule,
      spaces,
      name_rule
    > {};

  struct params_rule :
    pegtl::opt<
      pegtl::seq<
        param_rule,
        pegtl::star<
          spaces, str_comma, spaces, param_rule
        >
      >
    > {};

  struct callee_rule :
    pegtl::sor<
      str_print,
      str_input,
      str_tuple_error,
      str_tensor_error,
      name_rule
    > {};

  struct T_rule :
    pegtl::sor<
      type_rule,
      str_void
    > {};

  struct one_index_rule :
    pegtl::seq<
      str_index_left_bracket,
      spaces,
      t_rule,
      spaces,
      str_index_right_bracket
    > {};

  struct index_list_rule :
    pegtl::plus< one_index_rule > {};

  struct call_rule :
    pegtl::seq<
      callee_rule,
      spaces,
      str_args_left_paren,
      spaces,
      args0_rule,
      spaces,
      str_args_right_paren
    > {};

  // Instruction rules

  struct Instruction_declare_rule :
    pegtl::seq<
      type_rule,
      spaces,
      name_rule
    > {};

  struct Instruction_assignment_rule :
    pegtl::seq<
      name_rule,
      spaces,
      str_arrow,
      spaces,
      s_rule
    > {};

  struct Instruction_op_rule :
    pegtl::seq<
      name_rule,
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
      name_rule,
      spaces,
      str_arrow,
      spaces,
      name_rule,
      index_list_rule
    > {};

  struct Instruction_index_store_rule :
    pegtl::seq<
      name_rule,
      index_list_rule,
      spaces,
      str_arrow,
      spaces,
      s_rule
    > {};

  struct Instruction_length_t_rule :
    pegtl::seq<
      name_rule,
      spaces,
      str_arrow,
      spaces,
      str_length,
      spaces,
      name_rule,
      spaces,
      t_rule
    > {};

  struct Instruction_length_rule :
    pegtl::seq<
      name_rule,
      spaces,
      str_arrow,
      spaces,
      str_length,
      spaces,
      name_rule
    > {};

  struct Instruction_call_rule :
    pegtl::seq< call_rule > {};

  struct Instruction_call_assignment_rule :
    pegtl::seq<
      name_rule,
      spaces,
      str_arrow,
      spaces,
      call_rule
    > {};

  struct Instruction_new_array_rule :
    pegtl::seq<
      name_rule,
      spaces,
      str_arrow,
      spaces,
      str_new,
      spaces,
      str_array,
      spaces,
      str_args_left_paren,
      spaces,
      args1_rule,
      spaces,
      str_args_right_paren
    > {};

  struct Instruction_new_tuple_rule :
    pegtl::seq<
      name_rule,
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

  struct Instruction_label_rule :
    pegtl::seq< label_rule > {};

  struct Instruction_break_uncond_rule :
    pegtl::seq<
      str_break,
      spaces,
      label_rule
    > {};

  struct Instruction_break_cond_rule :
    pegtl::seq<
      str_break,
      spaces,
      t_rule,
      spaces,
      label_rule,
      spaces,
      label_rule
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
      pegtl::seq< pegtl::at< Instruction_break_cond_rule >            , Instruction_break_cond_rule            >,
      pegtl::seq< pegtl::at< Instruction_break_uncond_rule >          , Instruction_break_uncond_rule          >,
      pegtl::seq< pegtl::at< Instruction_return_t_rule >              , Instruction_return_t_rule              >,
      pegtl::seq< pegtl::at< Instruction_return_rule >                , Instruction_return_rule                >,
      pegtl::seq< pegtl::at< Instruction_label_rule >                 , Instruction_label_rule                 >,
      pegtl::seq< pegtl::at< Instruction_declare_rule >               , Instruction_declare_rule               >,
      pegtl::seq< pegtl::at< Instruction_new_array_rule >             , Instruction_new_array_rule             >,
      pegtl::seq< pegtl::at< Instruction_new_tuple_rule >             , Instruction_new_tuple_rule             >,
      pegtl::seq< pegtl::at< Instruction_length_t_rule >              , Instruction_length_t_rule              >,
      pegtl::seq< pegtl::at< Instruction_length_rule >                , Instruction_length_rule                >,
      pegtl::seq< pegtl::at< Instruction_op_rule >                    , Instruction_op_rule                    >,
      pegtl::seq< pegtl::at< Instruction_index_load_rule >            , Instruction_index_load_rule            >,
      pegtl::seq< pegtl::at< Instruction_call_assignment_rule >       , Instruction_call_assignment_rule       >,
      pegtl::seq< pegtl::at< Instruction_assignment_rule >            , Instruction_assignment_rule            >,
      pegtl::seq< pegtl::at< Instruction_index_store_rule >           , Instruction_index_store_rule           >,
      pegtl::seq< pegtl::at< Instruction_call_rule >                  , Instruction_call_rule                  >
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

  struct Function_rule :
    pegtl::seq<
      seps_with_comments,
      pegtl::bol,
      spaces,
      T_rule,
      spaces,
      function_name_rule,
      spaces,
      str_pars_left_paren,
      spaces,
      params_rule,
      spaces,
      str_pars_right_paren,
      seps_with_comments,
      spaces,
      str_left_brace,
      seps_with_comments,
      Instructions_rule,
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

  // Types

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

  // Function rule

  template<> struct action< T_rule > {
    template<typename Input>
    static void apply(const Input&, Program& p) {
      current_function = new Function();
      current_function->return_type = {cur_type, cur_int64_dims};
      p.functions.push_back(current_function);
      cur_int64_dims = 0;
    }
  };

  template<> struct action< function_name_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      current_function->name = in.string();
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

  template<> struct action< param_rule > {
    template<typename Input>
    static void apply(const Input&, Program&) {
      auto* v = pop_item<Name>();
      current_function->params.push_back(v);
      current_function->variable_types[v->name_] = {cur_type, cur_int64_dims};
      cur_int64_dims = 0;
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

  // Single push actions

  template<> struct action< name_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      parsed_items.push_back(new Name(in.string()));
    }
  };

  template<> struct action< number > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      parsed_items.push_back(new Number(std::stoll(in.string())));
    }
  };

  template<> struct action< label_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      parsed_items.push_back(new Label(in.string()));
    }
  };

  template<> struct action< op_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      last_op = op_from_string(in.string());
    }
  };

  template<> struct action< callee_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      std::string s = in.string();
      if (s == "print")             last_call_type = CallType::print;
      else if (s == "input")        last_call_type = CallType::input;
      else if (s == "tuple-error")  last_call_type = CallType::tuple_error;
      else if (s == "tensor-error") last_call_type = CallType::tensor_error;
      else                          last_call_type = CallType::la;
    }
  };

  // Actions to build instruction nodes

  template<> struct action< Instruction_declare_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto* var = pop_item<Name>();
      VarType type = {cur_type, cur_int64_dims};
      current_function->variable_types[var->name_] = type;
      cur_int64_dims = 0;
      add_instruction(in, new Instruction_declare(type, var));
    }
  };

  template<> struct action< Instruction_assignment_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      Item* src = pop_item<Item>();
      auto* dst = pop_item<Name>();
      add_instruction(in, new Instruction_assignment(dst, src));
    }
  };

  template<> struct action< Instruction_op_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      Item* rhs = pop_item<Item>();
      Item* lhs = pop_item<Item>();
      auto* dst = pop_item<Name>();
      add_instruction(in, new Instruction_op(dst, lhs, last_op, rhs));
    }
  };

  template<> struct action< Instruction_index_load_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto idxs = pop_from(index_begin);
      parsing_indexes = false;
      auto* src = pop_item<Name>();
      auto* dst = pop_item<Name>();
      add_instruction(in, new Instruction_index_load(dst, src, std::move(idxs)));
    }
  };

  template<> struct action< Instruction_index_store_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      Item* src = pop_item<Item>();
      auto idxs = pop_from(index_begin);
      parsing_indexes = false;
      auto* dst = pop_item<Name>();
      add_instruction(in, new Instruction_index_store(dst, std::move(idxs), src));
    }
  };

  template<> struct action< Instruction_length_t_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      Item* t = pop_item<Item>();
      auto* src = pop_item<Name>();
      auto* dst = pop_item<Name>();
      add_instruction(in, new Instruction_length_t(dst, src, t));
    }
  };

  template<> struct action< Instruction_length_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto* src = pop_item<Name>();
      auto* dst = pop_item<Name>();
      add_instruction(in, new Instruction_length(dst, src));
    }
  };

  template<> struct action< Instruction_call_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto args = pop_from(args_begin);
      Item* callee = nullptr;
      if (last_call_type == CallType::la) {
        callee = pop_item<Item>();
      }
      add_instruction(in, new Instruction_call(last_call_type, callee, std::move(args)));
    }
  };

  template<> struct action< Instruction_call_assignment_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto args = pop_from(args_begin);
      Item* callee = nullptr;
      if (last_call_type == CallType::la) {
        callee = pop_item<Item>();
      }
      auto* dst = pop_item<Name>();
      add_instruction(in, new Instruction_call_assignment(dst, last_call_type, callee, std::move(args)));
    }
  };

  template<> struct action< Instruction_new_array_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto args = pop_from(args_begin);
      auto* dst = pop_item<Name>();
      add_instruction(in, new Instruction_new_array(dst, std::move(args)));
    }
  };

  template<> struct action< Instruction_new_tuple_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      Item* t = pop_item<Item>();
      auto* dst = pop_item<Name>();
      add_instruction(in, new Instruction_new_tuple(dst, t));
    }
  };

  template<> struct action< Instruction_label_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto* l = pop_item<Label>();
      add_instruction(in, new Instruction_label(l));
    }
  };

  template<> struct action< Instruction_break_uncond_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto* l = pop_item<Label>();
      add_instruction(in, new Instruction_break_uncond(l));
    }
  };

  template<> struct action< Instruction_break_cond_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      auto* l2 = pop_item<Label>();
      auto* l1 = pop_item<Label>();
      Item* t = pop_item<Item>();
      add_instruction(in, new Instruction_break_cond(t, l1, l2));
    }
  };

  template<> struct action< Instruction_return_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      add_instruction(in, new Instruction_return());
    }
  };

  template<> struct action< Instruction_return_t_rule > {
    template<typename Input>
    static void apply(const Input& in, Program&) {
      Item* ret = pop_item<Item>();
      add_instruction(in, new Instruction_return_t(ret));
    }
  };

  Program parse_file(char *fileName) {
    parsed_items.clear();
    current_function = nullptr;
    parsing_params = false;
    parsing_indexes = false;
    args_begin = 0;
    index_begin = 0;
    cur_int64_dims = 0;

    if (pegtl::analyze< grammar >() != 0) {
      std::cerr << "There are problems with the grammar" << std::endl;
      exit(1);
    }

    file_input<> fileInput(fileName);
    Program p;
    parse< grammar, action >(fileInput, p);

    return p;
  }

}
