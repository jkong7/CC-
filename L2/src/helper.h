#pragma once 

#include <string_view> 
#include <stdexcept>

#include <L2.h> 

namespace L2 {

    AOP aop_from_string(std::string_view s);
    SOP sop_from_string(std::string_view s);
    CMP cmp_from_string(std::string_view s);

    std::string string_from_aop(AOP op);
    std::string string_from_sop(SOP op);
    std::string string_from_cmp(CMP op);

    std::string assembly_from_aop(AOP op); 
    std::string assembly_from_inc_dec(IncDec op); 
    std::string assembly_from_sop(SOP op); 
    std::string assembly_from_register(RegisterID id); 
    std::string eightBitReg_assembly_from_register(RegisterID ID);
    std::string indirect_call_reg_assembly_from_register(RegisterID id);
    std::string string_from_register(RegisterID id); 
    std::string assembly_from_cmp(CMP cmp, bool flip);
    std::string jump_assembly_from_cmp(CMP cmp, bool flip); 

    int comp(int64_t lhs, int64_t rhs, CMP op); 
}
