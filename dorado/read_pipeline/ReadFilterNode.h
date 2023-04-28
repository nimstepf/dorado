#pragma once

#include "ReadPipeline.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace dorado {

/// Class to filter reads based on some criteria.
/// Currently only supports one baked in type of
/// filtering based on qscore.
/// TODO: Should be replaced with more general
/// mechanism to define and pass in arbitrary filters
/// which are applied to each read.
class ReadFilterNode : public MessageSink {
public:
    ReadFilterNode(MessageSink& sink,
                   size_t min_qscore,
                   size_t num_worker_threads = 1,
                   size_t max_reads = 1000);
    ~ReadFilterNode();

private:
    MessageSink& m_sink;
    void worker_thread();

    // Async worker for writing.
    std::vector<std::unique_ptr<std::thread>> m_workers;
    std::atomic<size_t> m_active_threads;

    size_t m_min_qscore;
    std::atomic<size_t> m_num_reads_filtered;
};

}  // namespace dorado