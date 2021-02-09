#include "error.h"
#include <pcreposix.h>

auto_error_t find_pattern_compiled(regex_t* reg, const char* str, size_t nmatch, regmatch_t* matches);
auto_error_t find_pattern(const char* pattern, const char* str, size_t nmatch, regmatch_t* matches);