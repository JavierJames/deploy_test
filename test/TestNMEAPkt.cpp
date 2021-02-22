#include <stdio.h>
#include <string.h>
#include "NMEAPkt.hpp"
#include "gtest/gtest.h"

#include "HAL_TDD.hpp"

using ::testing::ElementsAreArray;

namespace {

class TestNMEAPkt : public ::testing::Test {
   protected:
    NMEAPkt* pkt;
    virtual void SetUp() { pkt = new NMEAPkt; }

    virtual void TearDown() { delete pkt; }
};

ACTION_P(PassArg0ThruPtr, value) {
    strcpy(static_cast<char*>(arg0), value);
}

TEST_F(TestNMEAPkt, ParseAllFields_when_FullPkt1) {
    char raw[] = "$10,A,0.10,WPTNME*66";
    uint8_t chks = 0x66;
    NMEAPkt::NMEAPktPages page = NMEAPkt::NMEAPktPages::PICCOMMS_RESET;
    char* param;

    // Packet valid
    EXPECT_TRUE(pkt->parseArray(raw, sizeof(raw)));

    // Loop through params
    size_t param_len = pkt->nextParam(&param);
    ASSERT_EQ(param_len, 1);
    EXPECT_THAT(std::vector<uint8_t>(param, param + param_len),
                ElementsAreArray({'A'}));

    param_len = pkt->nextParam(&param);
    ASSERT_EQ(param_len, 4);
    EXPECT_THAT(std::vector<uint8_t>(param, param + param_len),
                ElementsAreArray({'0', '.', '1', '0'}));

    param_len = pkt->nextParam(&param);
    ASSERT_EQ(param_len, 6);
    EXPECT_THAT(std::vector<uint8_t>(param, param + param_len),
                ElementsAreArray({'W', 'P', 'T', 'N', 'M', 'E'}));

    param_len = pkt->nextParam(&param);
    ASSERT_EQ(param_len, 0);
    EXPECT_STREQ(param, NULL);

    // Validate rest of the fields
    EXPECT_EQ(pkt->chks, chks);
    EXPECT_EQ(pkt->page, page);
}

TEST_F(TestNMEAPkt, ParseAllFields_when_FullPkt2) {
    char raw[] = "$91,12,6a7*6B";
    uint8_t chks = 0x6B;
    NMEAPkt::NMEAPktPages page = NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC;
    char* param;

    // Packet valid
    EXPECT_TRUE(pkt->parseArray(raw, sizeof(raw)));

    // Loop through params
    size_t param_len = pkt->nextParam(&param);
    ASSERT_EQ(param_len, 2);
    EXPECT_THAT(std::vector<uint8_t>(param, param + param_len),
                ElementsAreArray({'1', '2'}));

    param_len = pkt->nextParam(&param);
    ASSERT_EQ(param_len, 3);
    EXPECT_THAT(std::vector<uint8_t>(param, param + param_len),
                ElementsAreArray({'6', 'a', '7'}));

    param_len = pkt->nextParam(&param);
    ASSERT_EQ(param_len, 0);
    EXPECT_STREQ(param, NULL);

    // Validate rest of the fields
    EXPECT_EQ(pkt->chks, chks);
    EXPECT_EQ(pkt->page, page);
}

TEST_F(TestNMEAPkt, ParsePkt_when_EmptyParameters) {
    char raw[] = "$42*06";
    uint8_t chks = 0x06;
    NMEAPkt::NMEAPktPages page =
        NMEAPkt::NMEAPktPages::PICCOMMS_DEFAULTLEADER_COCO;
    char* param;

    // Packet valid
    EXPECT_TRUE(pkt->parseArray(raw, sizeof(raw)));

    // No parameters found
    size_t param_len = pkt->nextParam(&param);
    ASSERT_EQ(param_len, 0);
    EXPECT_STREQ(param, NULL);

    // Validate rest of the fields
    EXPECT_EQ(pkt->chks, chks);
    EXPECT_EQ(pkt->page, page);
}

TEST_F(TestNMEAPkt, DoNothing_when_ChecksumNotValid) {
    char raw[] = "$91,12,6a7*00";

    // Checksum not valid
    EXPECT_FALSE(pkt->parseArray(raw, sizeof(raw)));
}

TEST_F(TestNMEAPkt, DoNothing_when_IncompletePkt) {
    char raw1[] = "$10,A32";
    EXPECT_FALSE(pkt->parseArray(raw1, sizeof(raw1)));

    char raw[] = "10,A*32";
    EXPECT_FALSE(pkt->parseArray(raw, sizeof(raw)));

    char raw3[] = "10,A32";
    EXPECT_FALSE(pkt->parseArray(raw3, sizeof(raw3)));
}

TEST_F(TestNMEAPkt, CreateRawPkt_when_FieldsGiven) {
    char params[] = "12,6a7";
    NMEAPkt* newpkt =
        new NMEAPkt(NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC, params);

    EXPECT_THAT(std::vector<uint8_t>(newpkt->raw, newpkt->raw + newpkt->rawLen),
                ElementsAreArray({'$', '9', '1', ',', '1', '2', ',', '6', 'a',
                                  '7', '*', '6', 'B', '\0'}));
    EXPECT_EQ(newpkt->chks, 0x6B);
    EXPECT_EQ(newpkt->page, NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC);
    EXPECT_EQ(newpkt->rawLen,
              (sizeof(params) - 1) + 8);  // do not count '\0' in params[]
    EXPECT_TRUE(newpkt->isRawBuffAvailable());
}

TEST_F(TestNMEAPkt, CreateRawPkt_when_EmptyFieldsGiven) {
    NMEAPkt* newpkt =
        new NMEAPkt(NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC, NULL);

    EXPECT_THAT(std::vector<uint8_t>(newpkt->raw, newpkt->raw + newpkt->rawLen),
                ElementsAreArray({'$', '9', '1', '*', '0', '8', '\0'}));
    EXPECT_EQ(newpkt->chks, 0x08);
    EXPECT_EQ(newpkt->page, NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC);
    EXPECT_EQ(newpkt->rawLen,
              7);  // do not count ',' cause there are not params
    EXPECT_TRUE(newpkt->isRawBuffAvailable());
}

TEST_F(TestNMEAPkt, CreateRawPkt_when_LengthEqualsToMax) {
    const uint8_t NON_PARAMS_NUM_CHARS =
        7;  // $ + 2 page nibbles + , + * + 2 checksum nibbles
    char maxLenParams[SERIAL_RAW_MAX_LEN - NON_PARAMS_NUM_CHARS];
    memset(maxLenParams, 'x', SERIAL_RAW_MAX_LEN - NON_PARAMS_NUM_CHARS);
    maxLenParams[SERIAL_RAW_MAX_LEN - NON_PARAMS_NUM_CHARS - 1] = '\0';
    NMEAPkt* newpkt =
        new NMEAPkt(NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC, maxLenParams);
    EXPECT_TRUE(newpkt->isRawBuffAvailable());
    EXPECT_EQ(newpkt->rawLen, SERIAL_RAW_MAX_LEN);
}

TEST_F(TestNMEAPkt, ReportPktNotValid_when_LengthBiggerThanExpected) {
    const uint8_t NON_PARAMS_NUM_CHARS =
        7;  // $ + 2 page nibbles + , + * + 2 checksum nibbles
    char longParams[SERIAL_RAW_MAX_LEN + 1 - NON_PARAMS_NUM_CHARS];
    memset(longParams, 'x', SERIAL_RAW_MAX_LEN + 1 - NON_PARAMS_NUM_CHARS);
    longParams[SERIAL_RAW_MAX_LEN + 1 - NON_PARAMS_NUM_CHARS - 1] = '\0';
    NMEAPkt* newpkt =
        new NMEAPkt(NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC, longParams);
    EXPECT_FALSE(newpkt->isRawBuffAvailable());

    char reallyLongParams[200];
    memset(reallyLongParams, 'a', 200);
    reallyLongParams[199] = '\0';
    NMEAPkt* newpkt1 =
        new NMEAPkt(NMEAPkt::NMEAPktPages::PICCOMMS_BAT_SOC, reallyLongParams);
    EXPECT_FALSE(newpkt1->isRawBuffAvailable());
}

}  // namespace
