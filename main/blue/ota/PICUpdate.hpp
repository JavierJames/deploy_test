#pragma once

#include <cstddef>
#include "OTAUpdate.hpp"

class PICUpdate : public OTAUpdate {
   public:
    void start(const ota_init_pkt_t& init) override;
    void processPkt(nus_pkt_t& pkt) override;
    void abort() override;
    void finish(bool success) override;

   private:
    bool isBootloaderReady();
};
