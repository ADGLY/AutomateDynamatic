#include "error.h"
#include <pcreposix.h>
#include <stdint.h>

auto_error_t find_pattern(const char* pattern, const char* str, size_t nmatch, regmatch_t* matches);

auto_error_t set_pattern(const char* pattern, uint8_t reg_nb);
auto_error_t find_set_pattern(const char* str, size_t nmatch, regmatch_t* matches, uint8_t reg_nb);