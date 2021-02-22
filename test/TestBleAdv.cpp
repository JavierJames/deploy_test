#include <stdio.h>
#include <string.h>
#include "ble_adv_conf.hpp"
#include "gtest/gtest.h"

#include "HAL_TDD.hpp"

using ::testing::ElementsAreArray;
using ::testing::Return;

namespace {

class TestBleAdv : public ::testing::Test {
   protected:
    TestBleAdv(){};

    Kien::scan_response_t scanres;
    Kien::altbeacon_t beacon;

    virtual void SetUp() {
        Kien::ble_adv_scanres_default(scanres);
        Kien::ble_adv_altbeacon_default(beacon);
    }
    virtual void TearDown() {}
};

TEST_F(TestBleAdv, UpdateBatteryInfoInAdv_when_DifferentParamsGiven) {
    Kien::ble_adv_config_altbeacon_battery(beacon, 50, true);

    ASSERT_EQ(beacon.battery_info & 0x7F, 50);
    ASSERT_TRUE(beacon.battery_info & 0x80);

    Kien::ble_adv_config_altbeacon_battery(beacon, 22, false);

    ASSERT_EQ(beacon.battery_info & 0x7F, 22);
    ASSERT_FALSE(beacon.battery_info & 0x80);
}

TEST_F(TestBleAdv, UpdateCoCoInScanRes_when_NewValueGiven) {
    std::array<uint8_t, 3> coco = {0x12, 0x34, 0x56};
    Kien::ble_adv_config_scanres_coco(scanres, coco);

    EXPECT_THAT(std::vector<uint8_t>(scanres.coco, scanres.coco + 3),
                ElementsAreArray({0x12, 0x34, 0x56}));
}

TEST_F(TestBleAdv, TruncateNameInScanRes_when_SizeLargerThanMax) {
    char name[] = "kienSat_123456X";
    uint8_t nameLen = sizeof(name) - 1;  // Do not include '\0'
    Kien::ble_adv_config_scanres_name(scanres, name, nameLen);

    nameLen = sizeof(scanres.name_raw);  // Truncate length
    ASSERT_EQ(scanres.name_len - 1,
              nameLen);  // Beacon counts name of data type (1 byte)
    EXPECT_THAT(std::vector<char>(scanres.name_raw, scanres.name_raw + nameLen),
                ElementsAreArray({'k', 'i', 'e', 'n', 'S', 'a', 't', '_', '1',
                                  '2', '3', '4', '5', '6'}));
}

TEST_F(TestBleAdv, SaveEntireNameInScanRes_when_SizeSmallerThanMax) {
    char name[] = "kien_spk";
    uint8_t nameLen = sizeof(name) - 1;  // Do not include '\0'
    Kien::ble_adv_config_scanres_name(scanres, name, nameLen);

    ASSERT_EQ(scanres.name_len - 1,
              nameLen);  // Beacon counts name of data type (1 byte)
    EXPECT_THAT(std::vector<char>(scanres.name_raw, scanres.name_raw + nameLen),
                ElementsAreArray({'k', 'i', 'e', 'n', '_', 's', 'p', 'k'}));
}

TEST_F(TestBleAdv, DoNotModifyName_when_PointsToNull) {
    char name[] = "kien_spk";
    uint8_t nameLen = sizeof(name) - 1;  // Do not include '\0'
    Kien::ble_adv_config_scanres_name(scanres, name, nameLen);

    Kien::ble_adv_config_scanres_name(scanres, nullptr, nameLen);

    ASSERT_EQ(scanres.name_len - 1,
              nameLen);  // Beacon counts name of data type (1 byte)
    EXPECT_THAT(std::vector<char>(scanres.name_raw, scanres.name_raw + nameLen),
                ElementsAreArray({'k', 'i', 'e', 'n', '_', 's', 'p', 'k'}));
}

TEST_F(TestBleAdv, JoinNamePrefixAndId_when_UsingDefaultNameStruct) {
    const std::array<uint8_t, 3> id1 = {0x12, 0x34, 0x56};
    Kien::ble_adv_config_scanres_name(scanres, true, id1);
    EXPECT_THAT(std::vector<char>(scanres.name_raw,
                                  scanres.name_raw + sizeof(scanres.name_raw)),
                ElementsAreArray({'k', 'i', 'e', 'n', 'S', 'A', 'T', '_', '1',
                                  '2', '3', '4', '5', '6'}));

    std::array<uint8_t, 3> id2 = {0xA0, 0xB3, 0x7F};
    Kien::ble_adv_config_scanres_name(scanres, false, id2);
    EXPECT_THAT(std::vector<char>(scanres.name_raw,
                                  scanres.name_raw + sizeof(scanres.name_raw)),
                ElementsAreArray({'k', 'i', 'e', 'n', 'S', 'U', 'B', '_', 'A',
                                  '0', 'B', '3', '7', 'F'}));
}

TEST_F(TestBleAdv, ParsingAndConvertingIdFromName_when_RetrievingId) {
    const std::array<uint8_t, 3> id_expected = {0xA1, 0xB3, 0x7F};
    std::array<uint8_t, 3> id;
    Kien::ble_adv_config_scanres_name(scanres, true, id_expected);
    Kien::ble_adv_parse_id(scanres, id);

    EXPECT_EQ(id, id_expected);
}

TEST_F(TestBleAdv, UpdateFirmwareVersionInScanRes_when_NewVersionGiven) {
    std::array<char, 3> picver = {'0', '1', '2'};
    std::array<char, 3> nrfver = {'a', 'b', 'c'};
    Kien::ble_adv_config_scanres_ver(scanres, picver, nrfver);

    EXPECT_THAT(std::vector<char>(scanres.pic_ver, scanres.pic_ver + 4),
                ElementsAreArray({'p', '0', '1', '2'}));
    EXPECT_THAT(std::vector<char>(scanres.nrf_ver, scanres.nrf_ver + 4),
                ElementsAreArray({'n', 'a', 'b', 'c'}));
}

TEST_F(TestBleAdv, IdentifyAsKienBeacon_when_UUIDMatches) {
    EXPECT_TRUE(Kien::ble_adv_is_altbeacon((uint8_t*)&beacon, sizeof(beacon)));
}

TEST_F(TestBleAdv, IdentifyAsKienBeacon_when_UUIDDoesNotMatch) {
    memset(beacon.uuid, 0, sizeof(beacon.uuid));
    EXPECT_FALSE(Kien::ble_adv_is_altbeacon((uint8_t*)&beacon, sizeof(beacon)));

    Kien::altbeacon_t beacon_new;
    EXPECT_FALSE(
        Kien::ble_adv_is_altbeacon((uint8_t*)&beacon_new, sizeof(beacon_new)));
}

TEST_F(TestBleAdv, IdentifyAsUnknownBeacon_when_SizeDoesNotMatch) {
    EXPECT_FALSE(
        Kien::ble_adv_is_altbeacon((uint8_t*)&beacon, sizeof(beacon) - 1));
    EXPECT_FALSE(Kien::ble_adv_is_altbeacon((uint8_t*)&beacon, 0));
}

TEST_F(TestBleAdv, HandleError_when_PtrIsNull) {
    EXPECT_FALSE(Kien::ble_adv_is_altbeacon(nullptr, sizeof(beacon)));
}

}  // namespace
