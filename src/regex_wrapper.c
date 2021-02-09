#include "regex_wrapper.h"
#include <stdio.h>

auto_error_t find_pattern_compiled(regex_t* reg, const char* str, size_t nmatch, regmatch_t* matches) {
	int err = regexec(reg, str, nmatch, matches, 0);
	//CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");
	if (err != 0)
		return ERR_REGEX;

	return ERR_NONE;
}

auto_error_t find_pattern(const char* pattern, const char* str, size_t nmatch, regmatch_t* matches) {
	regex_t reg;

	int err = regcomp(&reg, pattern, REG_EXTENDED);
	CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");

	err = regexec(&reg, str, nmatch, matches, 0);
	CHECK_COND_DO(err != 0, ERR_REGEX, "Regex compilation failed !", regfree(&reg));

	regfree(&reg);

	return ERR_NONE;
}