#include "BarcodeDemuxer.h"

#include "htslib/bgzf.h"
#include "htslib/kroundup.h"
#include "htslib/sam.h"
#include "read_pipeline/ReadPipeline.h"
#include "utils/sequence_utils.h"

#include <indicators/progress_bar.hpp>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>
#include <unordered_set>

namespace dorado {

BarcodeDemuxer::BarcodeDemuxer(const std::string& output_dir,
                               size_t threads,
                               size_t num_reads,
                               bool write_fastq)
        : MessageSink(10000),
          m_output_dir(output_dir),
          m_threads(threads),
          m_num_reads_expected(num_reads),
          m_write_fastq(write_fastq) {
    std::filesystem::create_directory(m_output_dir);
    m_worker = std::make_unique<std::thread>(std::thread(&BarcodeDemuxer::worker_thread, this));
}

void BarcodeDemuxer::terminate_impl() {
    terminate_input_queue();
    if (m_worker->joinable()) {
        m_worker->join();
    }
}

BarcodeDemuxer::~BarcodeDemuxer() {
    terminate_impl();
    sam_hdr_destroy(m_header);
    for (auto& [k, f] : m_files) {
        hts_close(f);
    }
}

void BarcodeDemuxer::worker_thread() {
    Message message;
    while (m_work_queue.try_pop(message)) {
        auto aln = std::move(std::get<BamPtr>(message));
        write(aln.get());
    }
}

int BarcodeDemuxer::write(bam1_t* const record) {
    // track stats
    // Fetch the barcode name
    assert(m_header);
    std::string bc(bam_aux2Z(bam_aux_get(record, "BC")));
    auto res = m_files.find(bc);
    htsFile* file = nullptr;
    if (res != m_files.end()) {
        file = res->second;
    } else {
        std::string filename = bc + (m_write_fastq ? ".fastq" : ".bam");
        auto filepath = m_output_dir / filename;
        file = hts_open(filepath.c_str(), (m_write_fastq ? "wf" : "wb"));
        if (file->format.compression == bgzf) {
            auto res = bgzf_mt(file->fp.bgzf, m_threads, 128);
            if (res < 0) {
                throw std::runtime_error("Could not enable multi threading for BAM generation.");
            }
        }
        m_files[bc] = file;
        auto hts_res = sam_hdr_write(file, m_header);
        if (hts_res < 0) {
            throw std::runtime_error("Failed to write SAM header, error code " +
                                     std::to_string(hts_res));
        }
    }
    auto hts_res = sam_write1(file, m_header, record);
    if (hts_res < 0) {
        throw std::runtime_error("Failed to write SAM record, error code " +
                                 std::to_string(hts_res));
    }
    m_processed_reads++;
    return hts_res;
}

void BarcodeDemuxer::set_header(const sam_hdr_t* const header) {
    if (header) {
        // Avoid leaking memory if this is called twice.
        if (m_header) {
            sam_hdr_destroy(m_header);
        }
        m_header = sam_hdr_dup(header);
    }
}

stats::NamedStats BarcodeDemuxer::sample_stats() const {
    auto stats = stats::from_obj(m_work_queue);
    stats["demuxed_reads_written"] = m_processed_reads.load();
    return stats;
}

}  // namespace dorado
