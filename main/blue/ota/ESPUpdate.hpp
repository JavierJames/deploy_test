#pragma once

#include "OTAUpdate.hpp"

class ESPUpdate : public OTAUpdate {
   public:
    void start(const ota_init_pkt_t& init) override;
    void processPkt(nus_pkt_t& pkt) override;
    void finish(bool success) override;
    void abort() override;

   private:
    bool isBootloaderReady();
};
