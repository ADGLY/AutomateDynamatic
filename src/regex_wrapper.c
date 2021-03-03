#include "regex_wrapper.h"
#include <stdio.h>
#include <stdbool.h>

static regex_t s_reg1;
static regex_t  s_reg2;
static bool is_first_pattern_set = false;
static bool is_snd_pattern_set = false;

auto_error_t set_pattern(const char* pattern, uint8_t reg_nb) {
	if (reg_nb == 1 && is_first_pattern_set == true) {
		regfree(&s_reg1);
	}
    if (reg_nb == 2 && is_snd_pattern_set == true) {
        regfree(&s_reg2);
    }
    int err;
    if(reg_nb == 1) {
        err = regcomp(&s_reg1, pattern, REG_EXTENDED);
    }
    else if(reg_nb == 2) {
        err = regcomp(&s_reg2, pattern, REG_EXTENDED);
    }
	if (err != 0) {
		return ERR_REGEX;
	}
	if(reg_nb == 1) {
        is_first_pattern_set = true;
	}
	if(reg_nb == 2) {
	    is_snd_pattern_set = true;
	}
	return ERR_NONE;
}

auto_error_t find_set_pattern(const char* str, size_t nmatch, regmatch_t* matches, uint8_t reg_nb) {
	int err;
    if(reg_nb == 1) {
        err = regexec(&s_reg1, str, nmatch, matches, 0);
    }
    if(reg_nb == 2) {
        err = regexec(&s_reg2, str, nmatch, matches, 0);
    }
	if (err != 0) {
		return ERR_REGEX;
	}

	return ERR_NONE;
}

auto_error_t find_pattern(const char* pattern, const char* str, size_t nmatch, regmatch_t* matches) {
	regex_t reg;
	
	//Print nothing here
	//Let the caller handle the error if it is fatal

	int err = regcomp(&reg, pattern, REG_EXTENDED);
	CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !")

	err = regexec(&reg, str, nmatch, matches, 0);
	CHECK_COND_DO(err != 0, ERR_REGEX, "", regfree(&reg))

	regfree(&reg);

	return ERR_NONE;
}

void free_regs() {
   if(is_first_pattern_set) {
        regfree(&s_reg1);
        is_first_pattern_set = false;
    }
    if(is_snd_pattern_set) {
        regfree(&s_reg2);
        is_first_pattern_set = false;
    }
}