#include <stdio.h>
#include <string.h>
#include "PICUpdate.hpp"
#include "comms/OTAPkt.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "HAL_TDD.hpp"
static HAL& hal = getHAL();

#include "globals.hpp"
#include "support/BluemanMock.hpp"
#include "support/PicmanMock.hpp"

using ::testing::_;
using ::testing::An;
using ::testing::Matcher;
using ::testing::Mock;
using ::testing::Return;

namespace {

MATCHER_P(SerialPktCmp, item, "") {
    if (item.rawLen != arg.rawLen) {
        return false;
    } else {
        return !memcmp(item.raw, arg.raw, item.rawLen);
    }
}

MATCHER_P(NusPktCmp, item, "") {
    if (item.len != arg.len) {
        return false;
    } else {
        return !memcmp(item.data, arg.data, item.len);
    }
}

class TestOTAUpdate : public ::testing::Test {
   protected:
    PICUpdate* ota;
    nus_pkt_t app_in_pkt;
    nus_pkt_t app_crc_pkt;
    OTAPkt ota_pkt;
    PicmanMock& pic = static_cast<PicmanMock&>(getPicman());
    BluemanMock& blue = static_cast<BluemanMock&>(getBlueman());

    virtual void SetUp() { ota = new PICUpdate(); }

    virtual void TearDown() {
        // Mock::VerifyAndClearExpectations(&pic);
        // Mock::VerifyAndClear(&pic);
        Mock::AllowLeak(&pic);
        // Mock::VerifyAndClearExpectations(&blue);
        // Mock::VerifyAndClear(&blue);
        Mock::AllowLeak(&blue);
        delete ota;
    }
};

TEST_F(TestOTAUpdate, ForwardPktToPic_when_NewPktReceivedFromApp) {
    uint8_t data[] = {1, 2, 3, 4, 5};
    memcpy(app_in_pkt.data, data, sizeof(data));
    app_in_pkt.len = sizeof(data);
    ota_pkt.encode(data + 1, sizeof(data) - 1);

    // EXPECT_CALL(pic, send(An<OTAPkt&>()));
    EXPECT_CALL(
        pic,
        send(Matcher<OTAPkt&>(SerialPktCmp(static_cast<OTAPkt&>(ota_pkt)))));

    ota->processPkt(app_in_pkt);
}

TEST_F(TestOTAUpdate, DoNothing_when_PacketSizeOutOfRange) {}

TEST_F(TestOTAUpdate, RespondWithCRC_when_NPktsReceived) {
    // Receive 10 packets
    uint8_t data[] = {BLE_NUS_CMD_OTA_CHUNK, 1, 2, 3, 4, 5};
    memcpy(app_in_pkt.data, data, sizeof(data));
    app_in_pkt.len = sizeof(data);
    ota_pkt.encode(data + 1, sizeof(data) - 1);

    // Forward to PIC (n-1)
    EXPECT_CALL(pic, send(An<OTAPkt&>())).Times(OTA_BURST_NUM_PKTS - 1);
    for (int i = 0; i < OTA_BURST_NUM_PKTS - 1; i++) {
        ota->processPkt(app_in_pkt);
    }

    // On packet N, respond with CRC (0xBCF4) and forward to PIC
    uint8_t crc[] = {BLE_NUS_CMD_OTA_CHUNK_ACK, 0xF4, 0xBC};  // LSB
    memcpy(app_crc_pkt.data, crc, sizeof(crc));
    app_crc_pkt.len = sizeof(crc);
    EXPECT_CALL(blue, send(NusPktCmp(app_crc_pkt)));
    EXPECT_CALL(pic, send(An<OTAPkt&>()));
    ota->processPkt(app_in_pkt);
}

TEST_F(TestOTAUpdate, RespondWithCRC_when_TwoPktBurstReceived) {
    // Receive 10 packets
    uint8_t data[] = {BLE_NUS_CMD_OTA_CHUNK, 1, 2, 3, 4, 5};
    memcpy(app_in_pkt.data, data, sizeof(data));
    app_in_pkt.len = sizeof(data);
    ota_pkt.encode(data + 1, sizeof(data) - 1);

    // Forward to PIC (n-1)
    EXPECT_CALL(pic, send(An<OTAPkt&>())).Times(OTA_BURST_NUM_PKTS - 1);
    for (int i = 0; i < OTA_BURST_NUM_PKTS - 1; i++) {
        ota->processPkt(app_in_pkt);
    }

    // On packet N, respond with CRC (0xBCF4) and forward to PIC
    uint8_t crc[] = {BLE_NUS_CMD_OTA_CHUNK_ACK, 0xF4, 0xBC};  // LSB
    memcpy(app_crc_pkt.data, crc, sizeof(crc));
    app_crc_pkt.len = sizeof(crc);
    EXPECT_CALL(blue, send(NusPktCmp(app_crc_pkt)));
    EXPECT_CALL(pic, send(An<OTAPkt&>()));
    ota->processPkt(app_in_pkt);

    // Forward to PIC (n-1)
    EXPECT_CALL(pic, send(An<OTAPkt&>())).Times(OTA_BURST_NUM_PKTS - 1);
    for (int i = 0; i < OTA_BURST_NUM_PKTS - 1; i++) {
        ota->processPkt(app_in_pkt);
    }

    // On packet N, respond with CRC (0x2AEA) and forward to PIC
    crc[1] = 0xEA;
    crc[2] = 0x2A;  // LSB
    memcpy(app_crc_pkt.data, crc, sizeof(crc));
    app_crc_pkt.len = sizeof(crc);
    EXPECT_CALL(blue, send(NusPktCmp(app_crc_pkt)));
    EXPECT_CALL(pic, send(An<OTAPkt&>()));
    ota->processPkt(app_in_pkt);
}

}  // namespace
