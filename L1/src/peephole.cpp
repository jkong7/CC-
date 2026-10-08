#include <peephole.h>

#include <regex>

namespace L1 {

  static std::string trim(const std::string &s) {
    size_t b = s.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t");
    return s.substr(b, e - b + 1);
  }

  static bool is_label(const std::string &line) {
    std::string t = trim(line);
    return !t.empty() && t.back() == ':';
  }

  static bool self_move(const std::string &line) {
    static const std::regex move(R"(movq (%r[a-z0-9]+), (%r[a-z0-9]+))");
    std::smatch m;
    std::string t = trim(line);
    return std::regex_match(t, m, move) && m[1] == m[2];
  }

  static bool overwritten_register(const std::string &line, std::string &reg, std::string &src) {
    static const std::regex move(R"(movq ([^,]+), (%r[a-z0-9]+))");
    std::smatch m;
    std::string t = trim(line);
    if (!std::regex_match(t, m, move)) return false;
    src = m[1];
    reg = m[2];
    return true;
  }

  static bool jump_to_next(const std::vector<std::string> &lines, size_t i) {
    std::string t = trim(lines[i]);
    if (t.rfind("jmp ", 0) != 0) return false;
    std::string target = t.substr(4) + ":";
    for (size_t k = i + 1; k < lines.size(); k++) {
      std::string next = trim(lines[k]);
      if (next.empty()) continue;
      if (!is_label(next)) return false;
      if (next == target) return true;
    }
    return false;
  }

  std::vector<std::string> peephole(const std::vector<std::string> &lines) {
    std::vector<std::string> current = lines;
    bool changed = true;
    while (changed) {
      changed = false;
      std::vector<std::string> next;
      for (size_t i = 0; i < current.size(); i++) {
        if (self_move(current[i]) || jump_to_next(current, i)) {
          changed = true;
          continue;
        }
        std::string reg, src, reg2, src2;
        if (i + 1 < current.size()
            && overwritten_register(current[i], reg, src)
            && overwritten_register(current[i + 1], reg2, src2)
            && reg == reg2
            && src2.find(reg) == std::string::npos) {
          changed = true;
          continue;
        }
        next.push_back(current[i]);
      }
      current = next;
    }
    return current;
  }

}
