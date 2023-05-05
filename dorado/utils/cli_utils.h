// Add some utilities for CLI.

#include "Version.h"

#include <argparse.hpp>
#include <htslib/sam.h>
#include <stdio.h>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace dorado {

namespace utils {

// Determine the thread allocation for writer and aligner threads
// in dorado aligner.
inline std::pair<int, int> aligner_writer_thread_allocation(int available_threads,
                                                            float writer_thread_fraction) {
    // clamping because we need at least 1 thread for alignment and for writing.
    int writer_threads =
            std::clamp(static_cast<int>(std::floor(writer_thread_fraction * available_threads)), 1,
                       available_threads - 1);
    int aligner_threads = std::clamp(available_threads - writer_threads, 1, available_threads - 1);
    return std::make_pair(aligner_threads, writer_threads);
}

inline bool is_fd_tty(FILE* fd) {
#ifdef _WIN32
    return _isatty(_fileno(fd));
#else
    return isatty(fileno(fd));
#endif
}

inline void add_pg_hdr(sam_hdr_t* hdr, const std::vector<std::string>& args) {
    sam_hdr_add_lines(hdr, "@HD\tVN:1.6\tSO:unknown", 0);

    std::stringstream pg;
    pg << "@PG\tID:basecaller\tPN:dorado\tVN:" << DORADO_VERSION << "\tCL:dorado";
    for (const auto& arg : args) {
        pg << " " << arg;
    }
    pg << std::endl;
    sam_hdr_add_lines(hdr, pg.str().c_str(), 0);
}

inline argparse::ArgumentParser parse_internal_options(
        const std::vector<std::string>& unused_args) {
    auto args = unused_args;
    const std::string prog_name = std::string("internal_args");
    argparse::ArgumentParser private_parser(prog_name);
    private_parser.add_argument("--skip-model-compatibility-check")
            .help("(WARNING: For expert users only) Skip model and data compatibility checks.")
            .default_value(false)
            .implicit_value(true);
    args.insert(args.begin(), prog_name);
    private_parser.parse_args(args);

    return private_parser;
}

}  // namespace utils

}  // namespace dorado
