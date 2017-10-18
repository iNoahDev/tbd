//
//  misc/recurse.h
//  tbd
//
//  Created by inoahdev on 8/25/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <cerrno>
#include <string>

#include "../utils/directory.h"
#include "../mach-o/file.h"

namespace recurse {
    enum class options : uint64_t {
        none,
        recurse_subdirectories = 1 << 0,
        print_warnings         = 1 << 1
    };

    inline bool operator==(const uint64_t &lhs, const options &rhs) noexcept { return lhs == static_cast<uint64_t>(rhs); }
    inline bool operator!=(const uint64_t &lhs, const options &rhs) noexcept { return lhs != static_cast<uint64_t>(rhs); }

    inline bool operator==(const options &lhs, const uint64_t &rhs) noexcept { return static_cast<uint64_t>(lhs) == rhs; }
    inline bool operator!=(const options &lhs, const uint64_t &rhs) noexcept { return static_cast<uint64_t>(lhs) == rhs; }

    inline uint64_t operator|(const uint64_t &lhs, const options &rhs) noexcept { return lhs | static_cast<uint64_t>(rhs); }
    inline void operator|=(uint64_t &lhs, const options &rhs) noexcept { lhs |= static_cast<uint64_t>(rhs); }

    inline options operator|(const options &lhs, const uint64_t &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) | rhs); }
    inline void operator|=(options &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) | rhs); }

    inline options operator|(const options &lhs, const options &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }
    inline void operator|=(options &lhs, const options &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }

    inline uint64_t operator&(const uint64_t &lhs, const options &rhs) noexcept { return lhs & static_cast<uint64_t>(rhs); }
    inline void operator&=(uint64_t &lhs, const options &rhs) noexcept { lhs &= static_cast<uint64_t>(rhs); }

    inline options operator&(const options &lhs, const uint64_t &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) & rhs); }
    inline void operator&=(options &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) & rhs); }

    inline options operator&(const options &lhs, const options &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }
    inline void operator&=(options &lhs, const options &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }

    enum class operation_result {
        ok,
        failed_to_open_directory
    };

    enum class macho_file_type {
        none,
        library,
        dynamic_library
    };

    macho::file::open_result _macho_open_file_as_filetype(const char *path, macho_file_type filetype, macho::file &macho_file);
    bool _macho_check_file_as_filetype(const char *path, macho_file_type filetype, macho::file::check_error &error);

    template <typename T>
    operation_result macho_files(const char *directory_path, macho_file_type filetype, options options, T &&callback) { // <void(std::string &, macho::file &)>
        auto directory = utils::directory();
        auto directory_open_result = directory.open(directory_path);

        switch (directory_open_result) {
            case utils::directory::open_result::ok:
                break;

            case utils::directory::open_result::failed_to_open_directory:
                return operation_result::failed_to_open_directory;
        }

        auto recursion_options = utils::directory::recursion_options::skip_current_directory | utils::directory::recursion_options::skip_parent_directory;
        if ((options & options::recurse_subdirectories) != options::none) {
            recursion_options |= utils::directory::recursion_options::recurse_subdirectories;
        }

        auto recursion_result = directory.recurse(utils::directory::recursion_filetypes::regular_file, recursion_options, [&](utils::directory &directory, std::string &path) {
            auto macho_file = macho::file();
            auto macho_file_open_result = _macho_open_file_as_filetype(path.data(), filetype, macho_file);;

            switch (macho_file_open_result) {
                case macho::file::open_result::ok:
                    callback(path, macho_file);
                    break;

                case macho::file::open_result::failed_to_open_stream:
                    if ((options & options::print_warnings) != options::none) {
                        fprintf(stderr, "Warning: Failed to open file (at path %s), failing with error: %s\n", path.data(), strerror(errno));
                    }

                    break;

                case macho::file::open_result::failed_to_allocate_memory:
                    if ((options & options::print_warnings) != options::none) {
                        fprintf(stderr, "Warning: Failed to allocate memory necessary to process file (at path %s)\n", path.data());
                    }

                    break;

                case macho::file::open_result::stream_seek_error:
                case macho::file::open_result::stream_read_error:
                case macho::file::open_result::zero_architectures:
                case macho::file::open_result::invalid_container:
                case macho::file::open_result::not_a_macho:
                case macho::file::open_result::not_a_library:
                case macho::file::open_result::not_a_dynamic_library:
                    break;
            }
        }, [&](utils::directory::recursion_warning warning, const void *data) {
            if ((options & options::print_warnings) != options::none) {
                switch (warning) {
                    case utils::directory::recursion_warning::failed_to_open_subdirectory: {
                        const auto &sub_directory = *(utils::directory *)data;
                        fprintf(stderr, "Warning: Failed to open sub-directory (at path %s), failing with error: %s\n", sub_directory.path.data(), strerror(errno));

                        break;
                    }

                    default:
                        break;
                }
            }
        });

        switch (recursion_result) {
            case utils::directory::recursion_result::ok:
            case utils::directory::recursion_result::directory_not_opened:
                break;
        }

        return operation_result::ok;
    }

    template <typename T>
    operation_result macho_file_paths(const char *directory_path, macho_file_type filetype, options options, T &&callback) { // <void(std::string &)>
        auto directory = utils::directory();
        auto directory_open_result = directory.open(directory_path);

        switch (directory_open_result) {
            case utils::directory::open_result::ok:
                break;

            case utils::directory::open_result::failed_to_open_directory:
                return operation_result::failed_to_open_directory;
        }

        auto recursion_options = utils::directory::recursion_options::skip_current_directory | utils::directory::recursion_options::skip_parent_directory;
        if ((options & options::recurse_subdirectories) != options::none) {
            recursion_options |= utils::directory::recursion_options::recurse_subdirectories;
        }

        auto recursion_result = directory.recurse(utils::directory::recursion_filetypes::regular_file, recursion_options, [&](utils::directory &directory, std::string &path) {
            auto check_error = macho::file::check_error::ok;
            auto is_valid_library = _macho_check_file_as_filetype(path.data(), filetype, check_error);

            switch (check_error) {
                case macho::file::check_error::ok:
                    break;

                case macho::file::check_error::failed_to_allocate_memory:
                    if ((options & options::print_warnings) != options::none) {
                        fprintf(stderr, "Warning: Failed to allocate memory necessary to process file (at path %s)", path.data());
                    }

                    break;

                case macho::file::check_error::failed_to_open_descriptor:
                    if ((options & options::print_warnings) != options::none) {
                        fprintf(stderr, "Warning: Failed to open file (at path %s), failing with error: %s\n", path.data(), strerror(errno));
                    }

                    break;

                case macho::file::check_error::failed_to_close_descriptor:
                case macho::file::check_error::failed_to_seek_descriptor:
                case macho::file::check_error::failed_to_read_descriptor:
                    break;
            }

            if (is_valid_library) {
                callback(path);
            }
        }, [&](utils::directory::recursion_warning warning, const void *data) {
            if ((options & options::print_warnings) != options::none) {
                switch (warning) {
                    case utils::directory::recursion_warning::failed_to_open_subdirectory: {
                        const auto &sub_directory = *(utils::directory *)data;
                        fprintf(stderr, "Warning: Failed to open sub-directory (at path %s), failing with error: %s\n", sub_directory.path.data(), strerror(errno));

                        break;
                    }
                }
            }
        });

        switch (recursion_result) {
            case utils::directory::recursion_result::ok:
            case utils::directory::recursion_result::directory_not_opened:
                break;
        }

        return operation_result::ok;
    }
}
