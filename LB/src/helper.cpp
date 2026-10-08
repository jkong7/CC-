#include <helper.h> 

namespace LB {

  OP op_from_string(std::string_view s) {
    if (s == "+")  return OP::plus;
    if (s == "-")  return OP::minus;
    if (s == "*")  return OP::times;
    if (s == "&")  return OP::at;
    if (s == "<<") return OP::left_shift;
    if (s == ">>") return OP::right_shift;
    if (s == "<")  return OP::less_than;
    if (s == "<=") return OP::less_than_equal;
    if (s == "=")  return OP::equal;
    if (s == ">=") return OP::greater_than_equal;
    if (s == ">")  return OP::greater_than;
    throw std::invalid_argument("unknown operator: " + std::string(s));
  }

  int64_t encode(int64_t n) {
    return static_cast<int64_t>((static_cast<uint64_t>(n) << 1) | 1); 
  }

  int64_t fold(OP op, int64_t lhs, int64_t rhs) {
    uint64_t a = static_cast<uint64_t>(lhs); 
    uint64_t b = static_cast<uint64_t>(rhs); 
    switch (op) {
      case OP::plus:                return static_cast<int64_t>(a + b);
      case OP::minus:               return static_cast<int64_t>(a - b);
      case OP::times:               return static_cast<int64_t>(a * b);
      case OP::at:                  return lhs & rhs;
      case OP::left_shift:          return static_cast<int64_t>(a << (b & 63));
      case OP::right_shift:         return lhs >> (b & 63);
      case OP::less_than:           return lhs < rhs;
      case OP::less_than_equal:     return lhs <= rhs;
      case OP::equal:               return lhs == rhs;
      case OP::greater_than_equal:  return lhs >= rhs;
      case OP::greater_than:        return lhs > rhs;
    }
    return 0; 
  }

  bool is_comparison(OP op) {
    return op == OP::less_than || op == OP::less_than_equal || op == OP::equal
        || op == OP::greater_than_equal || op == OP::greater_than; 
  }

  const char* op_to_str(OP op) {
    switch (op) {
      case OP::plus:                return "+";
      case OP::minus:               return "-";
      case OP::times:               return "*";
      case OP::at:                  return "&";
      case OP::left_shift:          return "<<";
      case OP::right_shift:         return ">>";
      case OP::less_than:           return "<";
      case OP::less_than_equal:     return "<=";
      case OP::equal:               return "=";
      case OP::greater_than_equal:  return ">=";
      case OP::greater_than:        return ">";
    }
    return "<?>";
  }

}
