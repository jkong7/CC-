#pragma once 

#include <string_view> 
#include <stdexcept>

#include <LA.h> 

namespace LA {
    OP op_from_string(std::string_view s); 
    int64_t encode(int64_t n); 
    int64_t fold(OP op, int64_t lhs, int64_t rhs); 
    bool is_comparison(OP op); 
    const char* op_to_str(OP op); 
}
