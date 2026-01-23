#include <helper.h>

namespace L2 {
  std::string assembly_from_register(RegisterID id) {
    switch (id) {
      case RegisterID::rax: return "%rax";
      case RegisterID::rbx: return "%rbx";
      case RegisterID::rcx: return "%rcx";
      case RegisterID::rdx: return "%rdx";
      case RegisterID::rsi: return "%rsi";
      case RegisterID::rdi: return "%rdi";
      case RegisterID::rbp: return "%rbp";
      case RegisterID::rsp: return "%rsp";
      case RegisterID::r8:  return "%r8";
      case RegisterID::r9:  return "%r9";
      case RegisterID::r10: return "%r10";
      case RegisterID::r11: return "%r11";
      case RegisterID::r12: return "%r12";
      case RegisterID::r13: return "%r13";
      case RegisterID::r14: return "%r14";
      case RegisterID::r15: return "%r15";
      default:
        throw std::runtime_error("invalid register");
    }
  }


  AOP aop_from_string(std::string_view s) {
    return s == "+="  ? AOP::plus_equal:
            s == "-="  ? AOP::minus_equal :
            s == "*=" ? AOP::times_equal :
            s == "&=" ? AOP::and_equal :
            throw std::runtime_error("bad AOP");
        }

  SOP sop_from_string(std::string_view s) {
    return s == "<<=" ? SOP::left_shift:
           s == ">>=" ? SOP::right_shift:
           throw std::runtime_error("bad SOP");
  }

  CMP cmp_from_string(std::string_view s) {
    return s == "<" ? CMP::less_than:
           s == "<=" ? CMP::less_than_equal: 
           s == "=" ? CMP::equal:
    throw std::runtime_error("bad CMP");
  }

  std::string string_from_aop(AOP op) {
    switch (op) {
      case AOP::plus_equal:  return "+=";
      case AOP::minus_equal: return "-=";
      case AOP::times_equal: return "*=";
      case AOP::and_equal:   return "&=";
      default:
        throw std::runtime_error("bad AOP");
    }
  }

  std::string assembly_from_aop(AOP op) {
    switch (op) {
      case AOP::plus_equal:  return "addq";
      case AOP::minus_equal: return "subq";
      case AOP::times_equal: return "imulq";
      case AOP::and_equal:   return "andq";
      default:
        throw std::runtime_error("bad AOP");
    }
  }

  std::string string_from_sop(SOP op) {
    switch (op) {
      case SOP::left_shift:  return "<<=";
      case SOP::right_shift: return ">>=";
      default:
        throw std::runtime_error("bad SOP");
    }
  }

  std::string string_from_cmp(CMP cmp) {
    switch (cmp) {
      case CMP::less_than:        return "<";
      case CMP::less_than_equal:  return "<=";
      case CMP::equal:            return "=";
      default:
        throw std::runtime_error("bad CMP");
    }
  }

  std::string assembly_from_cmp(CMP cmp, bool flip) {
    switch (cmp) {
      case CMP::less_than:        return flip ? "setg" : "setl";
      case CMP::less_than_equal:  return flip ? "setge" : "setle";
      case CMP::equal:            return "sete";
      default:
        throw std::runtime_error("bad CMP");
    }
  }

  std::string jump_assembly_from_cmp(CMP cmp, bool flip) {
    switch (cmp) {
      case CMP::less_than:        return flip ? "jg" : "jl";
      case CMP::less_than_equal:  return flip ? "jge" : "jle";
      case CMP::equal:            return "je";
      default:
        throw std::runtime_error("bad CMP");
    }
  }

  std::string assembly_from_inc_dec(IncDec op) {
    switch (op) {
      case increment:             return "inc"; 
      case decrement:             return "dec"; 
      default: 
        throw std::runtime_error("bad INCDEC");
    }
  }

  std::string assembly_from_sop(SOP op) {
    switch (op) {
      case left_shift:            return "salq";
      case right_shift:           return "sarq"; 
      default: 
        throw std::runtime_error("bad SOP"); 
    }
  }

  std::string eightBitReg_assembly_from_register(RegisterID ID) {
    switch (ID) {
      case rax: return "%al";
      case rbx: return "%bl";
      case rcx: return "%cl";
      case rdx: return "%dl";

      case rdi: return "%dil";
      case rsi: return "%sil";
      case rbp: return "%bpl";
      case rsp: return "%spl";

      case r8:  return "%r8b";
      case r9:  return "%r9b";
      case r10: return "%r10b";
      case r11: return "%r11b";
      case r12: return "%r12b";
      case r13: return "%r13b";
      case r14: return "%r14b";
      case r15: return "%r15b";
    }

  return "";
  }


  std::string indirect_call_reg_assembly_from_register(RegisterID id) {
    switch (id) {
      case RegisterID::rax: return "*%rax";
      case RegisterID::rbx: return "*%rbx";
      case RegisterID::rcx: return "*%rcx";
      case RegisterID::rdx: return "*%rdx";
      case RegisterID::rsi: return "*%rsi";
      case RegisterID::rdi: return "*%rdi";
      case RegisterID::rbp: return "*%rbp";
      case RegisterID::rsp: return "*%rsp";
      case RegisterID::r8:  return "*%r8";
      case RegisterID::r9:  return "*%r9";
      case RegisterID::r10: return "*%r10";
      case RegisterID::r11: return "*%r11";
      case RegisterID::r12: return "*%r12";
      case RegisterID::r13: return "*%r13";
      case RegisterID::r14: return "*%r14";
      case RegisterID::r15: return "*%r15";
      default:
        throw std::runtime_error("invalid register");
    }
  }


  int comp(int64_t lhs, int64_t rhs, CMP op) {
    switch (op) {
      case less_than: return lhs < rhs; 
      case less_than_equal: return lhs <= rhs; 
      case equal: return lhs == rhs; 
    }
    return 0; 
  }



    std::string string_from_register(RegisterID id) {
    switch (id) {
      case RegisterID::rax: return "rax";
      case RegisterID::rbx: return "rbx";
      case RegisterID::rcx: return "rcx";
      case RegisterID::rdx: return "rdx";
      case RegisterID::rsi: return "rsi";
      case RegisterID::rdi: return "rdi";
      case RegisterID::rbp: return "rbp";
      case RegisterID::rsp: return "rsp";
      case RegisterID::r8:  return "r8";
      case RegisterID::r9:  return "r9";
      case RegisterID::r10: return "r10";
      case RegisterID::r11: return "r11";
      case RegisterID::r12: return "r12";
      case RegisterID::r13: return "r13";
      case RegisterID::r14: return "r14";
      case RegisterID::r15: return "r15";
      default:
        throw std::runtime_error("invalid register");
    }
  }

  std::unordered_set<std::string> set_difference(const std::unordered_set<std::string> A, const std::unordered_set<std::string> B) {
    std::unordered_set<std::string> res; 
    for (const auto& s: A) {
      if (B.find(s) == B.end()) {
        res.insert(s);
      }
    }
    return res; 
  }

  std::unordered_set<std::string> set_union(const std::unordered_set<std::string> A, const std::unordered_set<std::string> B) {
    std::unordered_set<std::string> res = A; 
    for (const auto& s: B) {
      res.insert(s);
    }
    return res; 
  }

  void add_edges_to_graph(std::unordered_map<std::string, std::unordered_set<std::string>>& graph, const std::unordered_set<std::string>& A, const std::unordered_set<std::string>&B) {
    for (const auto& v1: A) {
      for (const auto& v2: B) {
        if (v1 != v2) {
          graph[v1].insert(v2); 
          graph[v2].insert(v1);
        }
      } 
    }
  }
}
