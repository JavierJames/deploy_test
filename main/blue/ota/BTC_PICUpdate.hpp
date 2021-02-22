#pragma once

#include <cstddef>
#include "BTC_OTAUpdate.hpp"

class BTC_PICUpdate : public BTC_OTAUpdate {
   public:
    void start(const btc_ota_init_pkt_t& init) override;
    void processPkt(spp_pkt_t& pkt) override;
    void abort() override;
    void finish(bool success) override;

   private:
    bool isBootloaderReady();
};
