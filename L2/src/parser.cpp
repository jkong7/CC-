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
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#include <L2.h>
#include <parser.h>
#include <helper.h> 

namespace pegtl = TAO_PEGTL_NAMESPACE;

using namespace pegtl;

namespace L2 {

  /* 
   * Tokens parsed
   */ 
  std::vector<Item *> parsed_items; 

  AOP last_aop; 
  SOP last_sop; 
  CMP last_cmp; 

  /* 
   * Grammar rules from now on.
   */


  struct name:
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

  

  // Keywords

  struct str_arrow : TAO_PEGTL_STRING( "<-" ) {};

  // Arith op keywords 

  struct str_plus_equal : TAO_PEGTL_STRING( "+=" ) {};
  struct str_minus_equal : TAO_PEGTL_STRING( "-=" ) {};
  struct str_times_equal : TAO_PEGTL_STRING( "*=" ) {};
  struct str_and_equal : TAO_PEGTL_STRING( "&=" ) {};

  // Shift op keywords 

  struct str_left_shift : TAO_PEGTL_STRING( "<<=" ) {};
  struct str_right_shift : TAO_PEGTL_STRING( ">>=" ) {};

  // Comp op 

  struct str_less_than : TAO_PEGTL_STRING( "<" ) {}; 
  struct str_less_than_equal : TAO_PEGTL_STRING( "<=" ) {}; 
  struct str_equal : TAO_PEGTL_STRING( "=" ) {}; 


  struct str_mem : TAO_PEGTL_STRING( "mem" ) {};
  struct str_cjump : TAO_PEGTL_STRING( "cjump" ) {}; 
  struct str_goto : TAO_PEGTL_STRING( "goto" ) {};
  struct str_return : TAO_PEGTL_STRING( "return" ) {};
  struct str_call : TAO_PEGTL_STRING( "call" ) {};
  struct str_print : TAO_PEGTL_STRING( "print" ) {};
  struct str_input : TAO_PEGTL_STRING( "input" ) {};
  struct str_allocate : TAO_PEGTL_STRING( "allocate" ) {};
  struct str_tuple_error : TAO_PEGTL_STRING( "tuple-error" ) {};
  struct str_tensor_error : TAO_PEGTL_STRING( "tensor-error" ) {};

  struct str_increment : TAO_PEGTL_STRING( "++" ) {};
  struct str_decrement : TAO_PEGTL_STRING( "--" ) {}; 

  // Stack keyword 

  struct str_stack_arg : TAO_PEGTL_STRING( "stack-arg" ) {}; 

  // Register keywords 

  struct str_rdi : TAO_PEGTL_STRING( "rdi" ) {};
  struct str_rsi : TAO_PEGTL_STRING( "rsi" ) {};
  struct str_rdx : TAO_PEGTL_STRING( "rdx" ) {};
  struct str_rcx : TAO_PEGTL_STRING( "rcx" ) {};
  struct str_r8 : TAO_PEGTL_STRING( "r8" ) {};
  struct str_r9 : TAO_PEGTL_STRING( "r9" ) {};

  struct str_rax : TAO_PEGTL_STRING( "rax" ) {};

  struct str_rsp : TAO_PEGTL_STRING( "rsp" ) {};

  struct register_rdi_rule:
      str_rdi {};
    
  struct register_rsi_rule: 
      str_rsi {}; 

  struct register_rdx_rule: 
      str_rdx {}; 

  struct register_rcx_rule: 
      str_rcx {}; 
  
  struct register_r8_rule: 
      str_r8 {}; 

  struct register_r9_rule: 
      str_r9 {}; 

  struct register_rax_rule: 
      str_rax {}; 

  struct register_rsp_rule: 
      str_rsp {}; 

  struct variable_rule:
    pegtl::seq<
      pegtl::one< '%' >, 
      name
    > {}; 

  struct sx_rule:
    pegtl::sor<
      register_rcx_rule,
      variable_rule
    > {};


  struct a_register_rule:
    pegtl::sor<
      register_rdi_rule, 
      register_rsi_rule, 
      register_rdx_rule, 
      sx_rule, 
      register_r8_rule, 
      register_r9_rule
    > {};


  struct w_register_rule:
    pegtl::sor<
      a_register_rule, 
      register_rax_rule
    > {};
  

  struct x_register_rule: 
    pegtl::sor<
      w_register_rule,
      register_rsp_rule
    > {}; 

  struct number:
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
    >{};

  struct t_rule: 
    pegtl::sor<
      x_register_rule, 
      number
    > {};
  
  struct label:
    pegtl::seq<
      pegtl::one<':'>,
      name
    > {};
  
  struct label_piece:
    pegtl::seq<
      pegtl::one<':'>,
      name
    > {};


  struct function_name_rule:
    pegtl::seq<
      pegtl::one<'@'>,
      name
    > {};

  struct function_name_piece_rule:
    pegtl::seq<
      pegtl::one<'@'>,
      name
    > {};

  struct s_rule: 
    pegtl::sor<
      t_rule, 
      label_piece, 
      function_name_piece_rule
    > {};
  
  struct u_rule: 
    pegtl::sor<
      w_register_rule, 
      function_name_piece_rule
    > {};
  
  struct aop_rule:
    pegtl::sor<
      str_plus_equal, 
      str_minus_equal, 
      str_times_equal, 
      str_and_equal
    > {}; 
  
  
  struct sop_rule:
    pegtl::sor<
      str_left_shift, 
      str_right_shift
    > {};

  struct cmp_rule:
    pegtl::sor<
      str_less_than_equal,
      str_less_than,  
      str_equal
    > {}; 




  struct argument_number:
    number {}; 

  struct comment: 
    pegtl::disable< 
      TAO_PEGTL_STRING( "//" ), 
      pegtl::until< pegtl::eolf > 
    > {};

  /*
   * Separators.
   */

  struct spaces :
    pegtl::star< 
      pegtl::sor<
        pegtl::one< ' ' >,
        pegtl::one< '\t'>
      >
    > { };

  struct seps : 
    pegtl::star<
      pegtl::seq<
        spaces,
        pegtl::eol
      >
    > { };
  struct seps_with_comments : 
    pegtl::star< 
      pegtl::seq<
        spaces,
        pegtl::sor<
          pegtl::eol,
          comment
        >
      >
    > { };

  // Instruction rules 

  struct Instruction_assignment_rule:
    pegtl::seq<
      w_register_rule, 
      spaces, 
      str_arrow, 
      spaces, 
      s_rule
    > {};

  struct Instruction_memory_load_rule:
    pegtl::seq<
      w_register_rule, 
      spaces, 
      str_arrow, 
      spaces, 
      str_mem, 
      spaces, 
      x_register_rule, 
      spaces, 
      number
    > {};

  struct Instruction_memory_store_rule:
    pegtl::seq<
      str_mem, 
      spaces, 
      x_register_rule, 
      spaces, 
      number,
      spaces, 
      str_arrow, 
      spaces, 
      s_rule
    > {};

  struct Instruction_stack_arg_assignment_rule: 
    pegtl::seq<
      w_register_rule, 
      spaces, 
      str_arrow, 
      spaces, 
      str_stack_arg, 
      spaces, 
      number
    > {}; 

  struct Instruction_aop_rule:
    pegtl::seq<
      w_register_rule, 
      spaces, 
      aop_rule, 
      spaces,
      t_rule
    > {}; 

  struct Instruction_sop_rule: 
    pegtl::seq<
      w_register_rule, 
      spaces, 
      sop_rule, 
      spaces,
      pegtl::sor<
        sx_rule, 
        number
      >
    > {};
  
  
  struct Instruction_mem_arith_rule:
    pegtl::seq<
      str_mem, 
      spaces, 
      x_register_rule, 
      spaces, 
      number, 
      spaces, 
      aop_rule,
      spaces,
      t_rule
    > {}; 


  struct Instruction_reg_arith_rule:
    pegtl::seq<
      w_register_rule, 
      spaces, 
      aop_rule,
      spaces, 
      str_mem, 
      spaces, 
      x_register_rule, 
      spaces, 
      number
    >  {};


  struct Instruction_assignment_cmp_rule:
    pegtl::seq<
      w_register_rule,
      spaces,
      str_arrow, 
      spaces,
      t_rule, 
      spaces,
      cmp_rule,
      spaces,
      t_rule
    > {}; 
  
  struct Instruction_cjump_rule:
    pegtl::seq<
      str_cjump, 
      spaces,
      t_rule, 
      spaces,
      cmp_rule,
      spaces,
      t_rule, 
      spaces, 
      label_piece
    > {}; 

  struct Instruction_label_rule:
    pegtl::seq<
      label_piece
    > {};

  struct Instruction_goto_rule:
    pegtl::seq<
      str_goto, 
      spaces, 
      label_piece
    > {};

  struct Instruction_return_rule:
    pegtl::seq<
      str_return
    > { };
  
  struct Instruction_l1_call_rule:
    pegtl::seq<
      str_call, 
      spaces, 
      u_rule, 
      spaces, 
      number
    > {}; 

  struct Instruction_print_call_rule:
    pegtl::seq<
      str_call, 
      spaces, 
      str_print, 
      spaces, 
      pegtl::one< '1' >
    > {}; 

  struct Instruction_input_call_rule: 
    pegtl::seq<
      str_call, 
      spaces,
      str_input, 
      spaces, 
      pegtl::one< '0' >
    > {}; 


  struct Instruction_allocate_call_rule: 
    pegtl::seq<
      str_call, 
      spaces, 
      str_allocate, 
      spaces, 
      pegtl::one< '2' >
    > {};
  
  struct Instruction_tuple_error_call_rule:
    pegtl::seq<
      str_call, 
      spaces, 
      str_tuple_error, 
      spaces, 
      pegtl::one< '3' >
    > {}; 
  
  struct Instruction_tensor_error_call_rule: 
    pegtl::seq<
      str_call, 
      spaces, 
      str_tensor_error, 
      spaces, 
      number
    > {};

  struct Instruction_register_increment_rule:
    pegtl::seq<
      w_register_rule, 
      spaces, 
      str_increment 
    > {}; 

  struct Instruction_register_decrement_rule:
    pegtl::seq<
      w_register_rule, 
      spaces, 
      str_decrement
    > {}; 

  struct Instruction_lea_rule: 
    pegtl::seq<
      w_register_rule, 
      spaces, 
      pegtl::one< '@' >,
      spaces, 
      w_register_rule, 
      spaces, 
      w_register_rule, 
      spaces,
      number
    > {}; 
  



struct Instruction_rule:
  pegtl::sor<
    pegtl::seq< pegtl::at< Instruction_return_rule >              , Instruction_return_rule               >,
    pegtl::seq< pegtl::at< Instruction_assignment_cmp_rule >      , Instruction_assignment_cmp_rule       >,
    pegtl::seq< pegtl::at< Instruction_assignment_rule >          , Instruction_assignment_rule           >,

    pegtl::seq< pegtl::at< Instruction_stack_arg_assignment_rule > , Instruction_stack_arg_assignment_rule>,

    pegtl::seq< pegtl::at< Instruction_cjump_rule >               , Instruction_cjump_rule                >,
    pegtl::seq< pegtl::at< Instruction_goto_rule >                , Instruction_goto_rule                 >,

    pegtl::seq< pegtl::at< Instruction_label_rule >               , Instruction_label_rule                >,

    pegtl::seq< pegtl::at< Instruction_memory_load_rule >         , Instruction_memory_load_rule          >,
    pegtl::seq< pegtl::at< Instruction_memory_store_rule >        , Instruction_memory_store_rule         >,

    pegtl::seq< pegtl::at< Instruction_mem_arith_rule >           , Instruction_mem_arith_rule            >,
    pegtl::seq< pegtl::at< Instruction_reg_arith_rule >           , Instruction_reg_arith_rule            >,

    pegtl::seq< pegtl::at< Instruction_aop_rule >                 , Instruction_aop_rule                  >,
    pegtl::seq< pegtl::at< Instruction_sop_rule >                 , Instruction_sop_rule                  >,

    pegtl::seq< pegtl::at< Instruction_lea_rule >                 , Instruction_lea_rule                  >,
    pegtl::seq< pegtl::at< Instruction_register_increment_rule >  , Instruction_register_increment_rule   >,
    pegtl::seq< pegtl::at< Instruction_register_decrement_rule >  , Instruction_register_decrement_rule   >,

    pegtl::seq< pegtl::at< Instruction_print_call_rule >          , Instruction_print_call_rule           >,
    pegtl::seq< pegtl::at< Instruction_input_call_rule >          , Instruction_input_call_rule           >,
    pegtl::seq< pegtl::at< Instruction_allocate_call_rule >       , Instruction_allocate_call_rule        >,
    pegtl::seq< pegtl::at< Instruction_tuple_error_call_rule >    , Instruction_tuple_error_call_rule     >,
    pegtl::seq< pegtl::at< Instruction_tensor_error_call_rule >   , Instruction_tensor_error_call_rule    >,
    pegtl::seq< pegtl::at< Instruction_l1_call_rule >             , Instruction_l1_call_rule              >,

    pegtl::seq< pegtl::at< comment >                              , comment                               >
  > {};


  struct Instructions_rule:
    pegtl::plus<
      pegtl::seq<
        seps,
        pegtl::bol,
        spaces,
        Instruction_rule,
        seps
      >
    > { };

  struct Function_rule:
    pegtl::seq<
      pegtl::seq<spaces, pegtl::one< '(' >>,
      seps_with_comments,
      pegtl::seq<spaces, function_name_rule>,
      seps_with_comments,
      pegtl::seq<spaces, argument_number>,
      seps_with_comments,
      Instructions_rule,
      seps_with_comments,
      pegtl::seq<spaces, pegtl::one< ')' >>
    > {};

  struct Functions_rule:
    pegtl::plus<
      seps_with_comments,
      Function_rule,
      seps_with_comments
    > {};

  struct entry_point_rule:
    pegtl::seq<
      seps_with_comments,
      pegtl::seq<spaces, pegtl::one< '(' >>,
      seps_with_comments,
      function_name_rule,
      seps_with_comments,
      Functions_rule,
      seps_with_comments,
      pegtl::seq<spaces, pegtl::one< ')' >>,
      seps
    > { };

  struct grammar : 
    pegtl::must< 
      entry_point_rule
    > {};

  /* 
   * Actions attached to grammar rules.
   */

  template< typename Rule >
  struct action : pegtl::nothing< Rule > {};


  // Function rule 
  template<> struct action < function_name_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
      std::cout << "FUNCTION PARSING" << std::endl; 
      if (p.entryPointLabel.empty()){
        p.entryPointLabel = in.string();
      } else {
        auto newF = new Function();
        newF->name = in.string();
        p.functions.push_back(newF);
      }
    }
  };
    
  template<> struct action < argument_number > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "ARG NUMBER PARSING" << std::endl; 

      auto currentF = p.functions.back();
      currentF->arguments = std::stoll(in.string());
    }
  };
  
  // Single push actions 

  // Push variable
  template<> struct action<variable_rule> {
    template<typename Input>
    static void apply(const Input& in, Program& p) { 
      std::cout << "variable" << in.string() << std::endl; 
      auto v = new Variable(in.string()); 
      parsed_items.push_back(v); }
  };


  // Single register pushes 

  template<> struct action<register_rax_rule> {
    template<typename Input>
    static void apply(const Input&, Program&) { 
      std::cout << "rax" << std::endl; 
      parsed_items.push_back(new Register(RegisterID::rax)); }
  };

  template<> struct action<register_rdi_rule> {
    template<typename Input>
    static void apply(const Input&, Program&) { 
      std::cout << "rdi" << std::endl; 
      parsed_items.push_back(new Register(RegisterID::rdi)); }
  };

  template<> struct action<register_rsi_rule> {
    template<typename Input>
    static void apply(const Input&, Program&) { parsed_items.push_back(new Register(RegisterID::rsi)); }
  };

  template<> struct action<register_rdx_rule> {
    template<typename Input>
    static void apply(const Input&, Program&) { 
      std::cout << "rdx" << std::endl; 
      parsed_items.push_back(new Register(RegisterID::rdx)); }
  };

  template<> struct action<register_rcx_rule> {
    template<typename Input>
    static void apply(const Input&, Program&) { parsed_items.push_back(new Register(RegisterID::rcx)); }
  };

  template<> struct action<register_r8_rule> {
    template<typename Input>
    static void apply(const Input&, Program&) { parsed_items.push_back(new Register(RegisterID::r8)); }
  };

  template<> struct action<register_r9_rule> {
    template<typename Input>
    static void apply(const Input&, Program&) { parsed_items.push_back(new Register(RegisterID::r9)); }
  };

  template<> struct action<register_rsp_rule> {
    template<typename Input>
    static void apply(const Input&, Program&) { parsed_items.push_back(new Register(RegisterID::rsp)); }
  };


  // Push a number 
  template<> struct action < number > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      auto n = new Number(static_cast<uint64_t>(std::stoull(in.string())));
      std::cout << in.string() << std::endl; 
      parsed_items.push_back(n);
    }
  };

  // Push a label 
  template<> struct action < label_piece > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      auto l = new Label(in.string());
      std::cout << in.string() << std::endl; 
      parsed_items.push_back(l);
    }
  };

  // Push a function name 
  template<> struct action < function_name_piece_rule > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      auto f = new Func(in.string());
      std::cout << in.string() << std::endl; 
      parsed_items.push_back(f);
    }
  };

  // Push arith ops 
  template<> struct action < aop_rule > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      last_aop = aop_from_string(in.string()); 
    }
  };

  // Push shifting ops 
  template<> struct action < sop_rule > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      last_sop = sop_from_string(in.string()); 
      std::cout << in.string() << std::endl; 
    }
  };

  // Push comp ops 

  template<> struct action < cmp_rule > {
    template< typename Input >
    static void apply (const Input &in, Program &p) {
      last_cmp = cmp_from_string(in.string()); 
      std::cout << in.string() << std::endl; 
    }
  };

  // Instruction actions: create instruction ast nodes 

  template<> struct action < Instruction_assignment_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "ASSIGNMENT PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last two tokens parsed.
       */
      auto src = parsed_items.back();
      parsed_items.pop_back();
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_assignment(dst, src);

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };

  

  template<> struct action < Instruction_memory_load_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "MEMORY LOAD PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last three tokens parsed.
       */

      auto num = parsed_items.back(); 
      parsed_items.pop_back(); 
      auto src = parsed_items.back();
      parsed_items.pop_back();
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      auto mem = new Memory(src, static_cast<Number*> (num));

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_assignment(dst, mem);

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };

  template<> struct action < Instruction_memory_store_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "MEMORY STORE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last three tokens parsed.
       */

      auto src = parsed_items.back(); 
      parsed_items.pop_back(); 
      auto num = parsed_items.back();
      parsed_items.pop_back();
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      auto mem = new Memory(dst, static_cast<Number*> (num));

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_assignment(mem, src);

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  template<> struct action < Instruction_stack_arg_assignment_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "Stack arg assignment PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last two tokens parsed.
       */

      auto offset = parsed_items.back(); 
      parsed_items.pop_back(); 
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      auto stackarg = new StackArg(static_cast<Number*>(offset)); 

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_stack_arg_assignment(dst, stackarg);

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  template<> struct action < Instruction_aop_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "AOP RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last two tokens parsed.
       */
 
      auto rhs = parsed_items.back();
      parsed_items.pop_back();
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_aop(dst, last_aop, rhs);

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };

  template<> struct action < Instruction_sop_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "SOP RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last two tokens parsed.
       */
 
      auto rhs = parsed_items.back();
      parsed_items.pop_back();
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_sop(dst, last_sop, rhs);

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  template<> struct action < Instruction_mem_arith_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "MEM ARITH PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last three tokens parsed.
       */
      auto src = parsed_items.back();
      parsed_items.pop_back();

      auto num = parsed_items.back();
      parsed_items.pop_back();

      auto dst = parsed_items.back();
      parsed_items.pop_back();


      auto mem = new Memory(dst, static_cast<Number*> (num));


      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_mem_aop(mem, last_aop, src);

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  template<> struct action < Instruction_reg_arith_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "REG ARITH PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last three tokens parsed.
       */
      auto num = parsed_items.back();
      parsed_items.pop_back();

      auto src = parsed_items.back();
      parsed_items.pop_back();

      auto dst = parsed_items.back();
      parsed_items.pop_back();


      auto mem = new Memory(src, static_cast<Number*> (num));


      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_mem_aop(dst, last_aop, mem);

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  template<> struct action < Instruction_assignment_cmp_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "CMP ASSIGNMENT RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last two tokens parsed.
       */
      
      
      auto rhs = parsed_items.back();
      parsed_items.pop_back();
      auto lhs = parsed_items.back();
      parsed_items.pop_back(); 
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_cmp_assignment(dst, lhs, last_cmp, rhs);

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };



  template<> struct action < Instruction_cjump_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "CJUMP RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last three tokens parsed.
       */
      
      
      auto label = parsed_items.back();
      parsed_items.pop_back();
      auto rhs = parsed_items.back();
      parsed_items.pop_back(); 
      auto lhs = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_cjump(lhs, last_cmp, rhs, static_cast<Label*>(label));

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  template<> struct action < Instruction_label_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "label instruction RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last three tokens parsed.
       */
      
      
      auto label = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_label(static_cast<Label*>(label));

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };

  template<> struct action < Instruction_goto_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "goto instruction RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last token parsed.
       */
      
      
      auto label = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_goto(static_cast<Label*>(label));

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };

  template<> struct action < Instruction_return_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "RETURN PARSING" << std::endl; 

      auto currentF = p.functions.back();
      auto i = new Instruction_ret();
      currentF->instructions.push_back(i);
    }
  };

  template<> struct action < Instruction_l1_call_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "L1 call instruction RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last two tokens parsed.
       */
      
      
      auto nArgs = parsed_items.back();
      parsed_items.pop_back();
      auto callee = parsed_items.back(); 
      parsed_items.pop_back(); 

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_call(CallType::l1, callee, static_cast<Number*>(nArgs)); 

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  template<> struct action < Instruction_print_call_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "print instruction RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_call(CallType::print, nullptr, new Number(1)); 

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


    template<> struct action < Instruction_input_call_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "input instruction RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_call(CallType::input, nullptr, new Number(0)); 

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };

  template<> struct action < Instruction_allocate_call_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "allocate instruction RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_call(CallType::allocate, nullptr, new Number(2)); 

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };

  template<> struct action < Instruction_tuple_error_call_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "tuple error instruction RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_call(CallType::tuple_error, nullptr, new Number(3)); 

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  template<> struct action < Instruction_tensor_error_call_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "tensor error instruction RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
      * Fetch the last token
      */
      auto number = parsed_items.back(); 
      parsed_items.pop_back(); 

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_call(CallType::tensor_error, nullptr, static_cast<Number*>(number)); 

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  template<> struct action < Instruction_register_increment_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "register increment RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last three tokens parsed.
       */
      
      
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_reg_inc_dec(dst, IncDec::increment); 

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };

  template<> struct action < Instruction_register_decrement_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "register decrement RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last three tokens parsed.
       */
      
      
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_reg_inc_dec(dst, IncDec::decrement); 

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };



  template<> struct action < Instruction_lea_rule > {
    template< typename Input >
	  static void apply( const Input & in, Program & p){
              std::cout << "lea RULE PARSING" << std::endl; 


      /* 
       * Fetch the current function.
       */ 
      auto currentF = p.functions.back();

      /*
       * Fetch the last four tokens parsed.
       */
      auto number = parsed_items.back();
      parsed_items.pop_back();
      auto rhs = parsed_items.back();
      parsed_items.pop_back(); 
      auto lhs = parsed_items.back();
      parsed_items.pop_back();
      
      auto dst = parsed_items.back();
      parsed_items.pop_back();

      /* 
       * Create the instruction.
       */ 
      auto i = new Instruction_lea(dst, lhs, rhs, static_cast<Number*>(number)); 

      /* 
       * Add the just-created instruction to the current function.
       */ 
      currentF->instructions.push_back(i);
    }
  };


  Program parse_file (char *fileName){

    /* 
     * Check the grammar for some possible issues.
     */
    if (pegtl::analyze< grammar >() != 0){
      std::cerr << "There are problems with the grammar" << std::endl;
      exit(1);
    }

    /*
     * Parse.
     */
    file_input< > fileInput(fileName);
    Program p;
    parse< grammar, action >(fileInput, p);

    return p;
  }
}
