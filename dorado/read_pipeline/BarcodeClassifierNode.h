#pragma once
#include "BarcodeClassifierSelector.h"
#include "read_pipeline/ReadPipeline.h"
#include "utils/stats.h"
#include "utils/types.h"

#include <atomic>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dorado {

class BarcodeClassifierNode : public MessageSink {
public:
    BarcodeClassifierNode(int threads,
                          const std::vector<std::string>& kit_name,
                          bool barcode_both_ends,
                          bool no_trim);
    BarcodeClassifierNode(int threads);
    ~BarcodeClassifierNode();
    std::string get_name() const override { return "BarcodeClassifierNode"; }
    stats::NamedStats sample_stats() const override;
    void terminate(const FlushOptions& flush_options) override { terminate_impl(); }
    void restart() override;

private:
    void start_threads();

    size_t m_threads{1};
    std::atomic<size_t> m_active{0};
    std::vector<std::unique_ptr<std::thread>> m_workers;
    std::atomic<int> m_num_records{0};
    const BarcodingInfo m_default_barcoding_info{};
    demux::BarcodeClassifierSelector m_barcoder_selector{};

    const BarcodingInfo* get_barcoding_info(SimplexRead& read);

    void worker_thread(size_t tid);
    void barcode(BamPtr& read);
    void barcode(SimplexRead& read);

    BamPtr trim_barcode(BamPtr irecord, const demux::ScoreResults& res, int seqlen);
    void trim_barcode(SimplexRead& read, const demux::ScoreResults& res);
    void terminate_impl();
};

}  // namespace dorado
