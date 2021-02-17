#include "regex_wrapper.h"
#include <stdio.h>
#include <stdbool.h>

static regex_t s_reg;
static bool is_pattern_set = false;

auto_error_t set_pattern(const char* pattern) {
	
	//Verifiy that this is possible
	if (is_pattern_set == true) {
		regfree(&s_reg);
	}
	int err = regcomp(&s_reg, pattern, REG_EXTENDED);
	if (err != 0) {
		return ERR_REGEX;
	}
	is_pattern_set = true;
	return ERR_NONE;
}

auto_error_t find_set_pattern(const char* str, size_t nmatch, regmatch_t* matches) {
	
	int err = regexec(&s_reg, str, nmatch, matches, 0);
	if (err != 0) {
		return ERR_REGEX;
	}

	return ERR_NONE;
}

auto_error_t find_pattern_compiled(regex_t* reg, const char* str, size_t nmatch, regmatch_t* matches) {
	int err = regexec(reg, str, nmatch, matches, 0);
	//CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");
	if (err != 0)
		return ERR_REGEX;

	return ERR_NONE;
}

auto_error_t find_pattern(const char* pattern, const char* str, size_t nmatch, regmatch_t* matches) {
	regex_t reg;
	
	//Print nothing here
	//Let the caller handle the error if it is fatal

	int err = regcomp(&reg, pattern, REG_EXTENDED);
	CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");

	err = regexec(&reg, str, nmatch, matches, 0);
	CHECK_COND_DO(err != 0, ERR_REGEX, "", regfree(&reg));

	regfree(&reg);

	return ERR_NONE;
}