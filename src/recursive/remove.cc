//
//  src/recursive/remove.cc
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cstdio>
#include <unistd.h>

#include "../utils/path.h"
#include "remove.h"

namespace recursive::remove {
    result perform(char *path) {
        return perform(path, path);
    }

    result perform(char *path, char *begin) {
        auto end = utils::string::find_end_of_null_terminated_string(path);
        auto path_component_end = end;

        do {
            auto path_component_end_elmt = *path_component_end;
            *path_component_end = '\0';

            if (access(path, F_OK) != 0) {
                return result::ok;
            }

            auto next_path_component_end = utils::path::find_last_slash_in_front_of_pattern(begin, path_component_end);
            if (next_path_component_end == path_component_end) {
                if (::remove(path) != 0) {
                    return result::failed_to_remove_directory;
                }

                *path_component_end = path_component_end_elmt;
                break;
            }

            if (::remove(path) != 0) {
                return result::failed_to_remove_subdirectories;
            }

            *path_component_end = path_component_end_elmt;
            path_component_end = next_path_component_end;
        } while (true);

        return result::ok;
    }
}
