#pragma once

#include "BTC_OTAUpdate.hpp"

class BTC_ESPUpdate : public BTC_OTAUpdate {
   public:
    void start(const btc_ota_init_pkt_t& init) override;
    void processPkt(spp_pkt_t& pkt) override;
    void finish(bool success) override;
    void abort() override;

   private:
    bool isBootloaderReady();
};
