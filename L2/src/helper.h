#pragma once 

#include <string_view> 
#include <stdexcept>
#include <unordered_map> 
#include <unordered_set> 
#include <vector> 

#include <L2.h> 

namespace L2 {

    inline const std::unordered_set<std::string> GPregisters = {
    "r10", "r11", "r12", "r13", "r14", "r15",
    "r8", "r9", "rax", "rbp", "rbx", "rcx",
    "rdi", "rdx", "rsi"
    };

    inline const std::unordered_set<std::string> GPregisters_without_rcx = {
    "r10", "r11", "r12", "r13", "r14", "r15",
    "r8", "r9", "rax", "rbp", "rbx",
    "rdi", "rdx", "rsi"
    };

    inline const std::vector<std::string> colorOrder = {
    "r10", "r11", "r8", "r9", "rax", "rcx", "rdx", "rsi", "rdi",
    "rbx", "rbp", "r12", "r13", "r14", "r15"
    };

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

    std::unordered_set<std::string> set_difference (const std::unordered_set<std::string> A, const std::unordered_set<std::string> B);
    std::unordered_set<std::string> set_union (const std::unordered_set<std::string> A, const std::unordered_set<std::string> B);

    void add_edges_to_graph(std::unordered_map<std::string, std::unordered_set<std::string>>& graph, const std::unordered_set<std::string>& A, const std::unordered_set<std::string>& B);

    int comp(int64_t lhs, int64_t rhs, CMP op); 
}
