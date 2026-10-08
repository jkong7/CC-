#pragma once

#include <CM.h>

namespace CM {
    Program parse_file(char* fileName);
    Program parse_string(const std::string &source, const std::string &name);
}
