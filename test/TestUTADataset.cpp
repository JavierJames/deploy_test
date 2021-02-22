#include <stdio.h>
#include <string.h>

#include "globals.hpp"
#include "uta/dataset.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAreArray;
using ::testing::Matcher;

SysInfo _sys(NULL);

SysInfo& getSysInfo() {
    return _sys;
}

namespace {

MATCHER_P(DatasetCmp, item, "") {
    return (item.tof == arg.tof && item.room_id == arg.room_id &&
            item.spk_rx_id == arg.spk_rx_id &&
            item.spk_tx_id == arg.spk_tx_id && item.rssi == arg.rssi);
}

class TestUTADataset : public ::testing::Test {
   protected:
    TestUTADataset(){};

    Kien::altbeacon_t beacon;
    Kien::scan_response_t scanres;
    Kien::ds_uta_t ds;
 
    std::array<uint8_t, 3> my_id;
    Kien::ds_uta_t ds_sample;

    virtual void SetUp() {
        my_id = {0xA1, 0xB2, 0xC3};
        ds_sample = ds_sample = {.tof = 0x1000,
                                      .room_id = 0x2D,
                                      .spk_rx_id = my_id,
                                      .spk_tx_id = {0x01, 0x02, 0x03},
                                      .rssi = -67};

        Kien::ble_adv_altbeacon_default(beacon);
        getSysInfo().setSpkId(my_id);
    }
    virtual void TearDown() {}
};

TEST_F(TestUTADataset, KienUTADataExtracted_when_BeaconIsParsed) {
    beacon.room_id = ds_sample.room_id;
    beacon.tof = ds_sample.tof;
    Kien::ds_parse_beacon(ds, beacon, ds_sample.rssi);

    memcpy(scanres.name_id, "010203", 6);
    Kien::ds_parse_scanres(ds, scanres);

    EXPECT_THAT(ds, DatasetCmp(ds_sample));
}

TEST_F(TestUTADataset, WriteDatasetToString_when_Serializing) {
    char s[25];
    uint8_t sLen = ds_serialize(ds_sample, s, sizeof(s));

    EXPECT_THAT(std::vector<char>(s, s + sLen),
                ElementsAreArray({'A', '1', 'B', '2', 'C', '3', ',', '0',
                                  '1', '0', '2', '0', '3', ',', '2', 'D',
                                  ',', '1', '0', '0', '0', ',', 'B', 'D'}));
    EXPECT_EQ(sLen, sizeof(s) - 1);
}

TEST_F(TestUTADataset, FailSafe_when_InputNotValid) {
    ASSERT_DEATH(ds_serialize(ds_sample, nullptr, 100),
                 ".* Assertion .data. failed.");

    char data[18];
    ASSERT_DEATH(ds_serialize(ds_sample, data, 5), ".* Assertion .* failed.");
    ASSERT_DEATH(ds_serialize(ds_sample, data, 0), ".* Assertion .* failed.");
}

}  // namespace
