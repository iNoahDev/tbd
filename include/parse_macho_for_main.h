//
//  include/parse_macho_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef PARSE_MACHO_FOR_MAIN_H
#define PARSE_MACHO_FOR_MAIN_H

#include "tbd_for_main.h"

/*
 * magic_in should be atleast 4 bytes large.
 */

enum parse_macho_for_main_options {
    O_PARSE_MACHO_FOR_MAIN_VERIFY_WRITE_PATH = 1 << 0
};

struct parse_macho_for_main_args {
    int fd;
    void *magic_in;

    uint64_t *magic_in_size_in;
    uint64_t *retained_info_in;

    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    const char *dir_path;
    uint64_t dir_path_length;

    const char *name;
    uint64_t name_length;

    bool ignore_non_macho_error;
    bool print_paths;

    uint64_t options;
};

enum parse_macho_for_main_result {
    E_PARSE_MACHO_FOR_MAIN_OK,
    E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO,
    E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR
};

enum parse_macho_for_main_result
parse_macho_file_for_main(struct parse_macho_for_main_args args);

enum parse_macho_for_main_result
parse_macho_file_for_main_while_recursing(
	struct parse_macho_for_main_args args);

#endif /* PARSE_MACHO_FOR_MAIN_H */
