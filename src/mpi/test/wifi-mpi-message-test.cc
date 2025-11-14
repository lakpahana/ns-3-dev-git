/*
 * Copyright (c) 2025
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/null-message-mpi-interface.h"
#include "ns3/test.h"
#include "ns3/wifi-mpi-messages.h"

#include <cstring>

using namespace ns3;

/**
 * \ingroup mpi-tests
 * \brief Test WiFi MPI message serialization and deserialization
 */
class WifiMpiMessageSerializationTest : public TestCase
{
  public:
    WifiMpiMessageSerializationTest();
    ~WifiMpiMessageSerializationTest() override;

  private:
    void DoRun() override;

    /**
     * Test WifiMpiMessageHeader serialization
     */
    void TestMessageHeader();

    /**
     * Test WifiTxRequestMessage serialization
     */
    void TestTxRequestMessage();

    /**
     * Test WifiRxEventMessage serialization
     */
    void TestRxEventMessage();

    /**
     * Test WifiMobilityUpdateMessage serialization
     */
    void TestMobilityUpdateMessage();

    /**
     * Test WifiRegistrationMessage serialization
     */
    void TestRegistrationMessage();
};

WifiMpiMessageSerializationTest::WifiMpiMessageSerializationTest()
    : TestCase("WiFi MPI Message Serialization")
{
}

WifiMpiMessageSerializationTest::~WifiMpiMessageSerializationTest()
{
}

void
WifiMpiMessageSerializationTest::TestMessageHeader()
{
    WifiMpiMessageHeader header;
    header.timeTs = 123456789;
    header.guaranteeTs = 987654321;
    header.nodeId = 42;
    header.devIfIndex = 3;
    header.msgType = MPI_MSG_WIFI_TX_REQUEST;

    // Serialize
    uint8_t buffer[100];
    uint32_t size = header.Serialize(buffer);

    NS_TEST_ASSERT_MSG_EQ(size,
                          WifiMpiMessageHeader::GetSerializedSize(),
                          "Serialized size mismatch");

    // Deserialize
    WifiMpiMessageHeader header2;
    uint32_t readSize = header2.Deserialize(buffer);

    NS_TEST_ASSERT_MSG_EQ(readSize, size, "Deserialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(header2.timeTs, header.timeTs, "timeTs mismatch");
    NS_TEST_ASSERT_MSG_EQ(header2.guaranteeTs, header.guaranteeTs, "guaranteeTs mismatch");
    NS_TEST_ASSERT_MSG_EQ(header2.nodeId, header.nodeId, "nodeId mismatch");
    NS_TEST_ASSERT_MSG_EQ(header2.devIfIndex, header.devIfIndex, "devIfIndex mismatch");
    NS_TEST_ASSERT_MSG_EQ(header2.msgType, header.msgType, "msgType mismatch");
}

void
WifiMpiMessageSerializationTest::TestTxRequestMessage()
{
    WifiTxRequestMessage msg;
    msg.header.timeTs = 100;
    msg.header.guaranteeTs = 200;
    msg.header.nodeId = 1;
    msg.header.devIfIndex = 0;
    msg.header.msgType = MPI_MSG_WIFI_TX_REQUEST;

    msg.senderNode = 1;
    msg.senderDev = 0;
    msg.ppduUid = 12345;
    msg.txPower = -20.0;
    msg.channelNumber = 36;
    msg.mcs = 7;
    msg.nss = 2;
    msg.guardInterval = 800;
    msg.channelWidth = 80;
    msg.preambleType = 3;

    // Create test PSDU data
    std::vector<uint8_t> testData = {0x01, 0x02, 0x03, 0x04, 0x05};
    msg.psduSize = testData.size();

    // Serialize
    uint8_t buffer[1000];
    uint32_t bufferSize = msg.GetSerializedSize() + testData.size();
    uint32_t size = msg.Serialize(buffer, bufferSize);

    // Copy PSDU data after the message
    std::memcpy(buffer + size, testData.data(), testData.size());
    size += testData.size();

    // Deserialize
    WifiTxRequestMessage msg2;
    uint32_t readSize = msg2.Deserialize(buffer, size);

    NS_TEST_ASSERT_MSG_EQ(msg2.header.msgType, msg.header.msgType, "msgType mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.senderNode, msg.senderNode, "senderNode mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.ppduUid, msg.ppduUid, "ppduUid mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.txPower, msg.txPower, "txPower mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.mcs, msg.mcs, "mcs mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.nss, msg.nss, "nss mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.psduSize, msg.psduSize, "psduSize mismatch");
}

void
WifiMpiMessageSerializationTest::TestRxEventMessage()
{
    WifiRxEventMessage msg;
    msg.header.timeTs = 500;
    msg.header.guaranteeTs = 600;
    msg.header.nodeId = 2;
    msg.header.devIfIndex = 0;
    msg.header.msgType = MPI_MSG_WIFI_RX_EVENT;

    msg.ppduUid = 67890;
    msg.rxPower = -65.5;
    msg.txDuration = 1000000; // 1ms in nanoseconds
    msg.mcs = 5;
    msg.nss = 1;
    msg.guardInterval = 400;
    msg.channelWidth = 20;
    msg.preambleType = 2;

    std::vector<uint8_t> testData = {0xAA, 0xBB, 0xCC};
    msg.psduSize = testData.size();

    // Serialize
    uint8_t buffer[1000];
    uint32_t bufferSize = msg.GetSerializedSize() + testData.size();
    uint32_t size = msg.Serialize(buffer, bufferSize);

    std::memcpy(buffer + size, testData.data(), testData.size());
    size += testData.size();

    // Deserialize
    WifiRxEventMessage msg2;
    uint32_t readSize = msg2.Deserialize(buffer, size);

    NS_TEST_ASSERT_MSG_EQ(msg2.ppduUid, msg.ppduUid, "ppduUid mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.rxPower, msg.rxPower, "rxPower mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.txDuration, msg.txDuration, "txDuration mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.mcs, msg.mcs, "mcs mismatch");
}

void
WifiMpiMessageSerializationTest::TestMobilityUpdateMessage()
{
    WifiMobilityUpdateMessage msg;
    msg.header.timeTs = 1000;
    msg.header.guaranteeTs = 1100;
    msg.header.nodeId = 5;
    msg.header.devIfIndex = 0;
    msg.header.msgType = MPI_MSG_WIFI_MOBILITY_UPDATE;

    msg.nodeId = 5;
    msg.posX = 100.5;
    msg.posY = 200.7;
    msg.posZ = 1.5;
    msg.velX = 10.0;
    msg.velY = 5.0;
    msg.velZ = 0.0;

    // Serialize
    uint8_t buffer[200];
    uint32_t size = msg.Serialize(buffer);

    NS_TEST_ASSERT_MSG_EQ(size,
                          WifiMobilityUpdateMessage::GetSerializedSize(),
                          "Serialized size mismatch");

    // Deserialize
    WifiMobilityUpdateMessage msg2;
    uint32_t readSize = msg2.Deserialize(buffer);

    NS_TEST_ASSERT_MSG_EQ(readSize, size, "Deserialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.nodeId, msg.nodeId, "nodeId mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.posX, msg.posX, "posX mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.posY, msg.posY, "posY mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.posZ, msg.posZ, "posZ mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.velX, msg.velX, "velX mismatch");
}

void
WifiMpiMessageSerializationTest::TestRegistrationMessage()
{
    WifiRegistrationMessage msg;
    msg.header.timeTs = 0;
    msg.header.guaranteeTs = 0;
    msg.header.nodeId = 0;
    msg.header.devIfIndex = 0;
    msg.header.msgType = MPI_MSG_WIFI_REGISTRATION;

    msg.nodeId = 10;
    msg.devIfIndex = 1;
    msg.rankId = 2;
    msg.channelNumber = 44;
    msg.rxSensitivity = -82.0;
    msg.rxGain = 0.0;
    msg.hasPropagationInfo = true;
    std::strcpy(msg.propagationLossModelTypeId, "ns3::FriisPropagationLossModel");
    std::strcpy(msg.propagationDelayModelTypeId, "ns3::ConstantSpeedPropagationDelayModel");

    // Serialize
    uint8_t buffer[500];
    uint32_t size = msg.Serialize(buffer);

    // Deserialize
    WifiRegistrationMessage msg2;
    uint32_t readSize = msg2.Deserialize(buffer);

    NS_TEST_ASSERT_MSG_EQ(msg2.nodeId, msg.nodeId, "nodeId mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.rankId, msg.rankId, "rankId mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.channelNumber, msg.channelNumber, "channelNumber mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.rxSensitivity, msg.rxSensitivity, "rxSensitivity mismatch");
    NS_TEST_ASSERT_MSG_EQ(msg2.hasPropagationInfo,
                          msg.hasPropagationInfo,
                          "hasPropagationInfo mismatch");
    NS_TEST_ASSERT_MSG_EQ(std::string(msg2.propagationLossModelTypeId),
                          std::string(msg.propagationLossModelTypeId),
                          "propagationLossModelTypeId mismatch");
}

void
WifiMpiMessageSerializationTest::DoRun()
{
    TestMessageHeader();
    TestTxRequestMessage();
    TestRxEventMessage();
    TestMobilityUpdateMessage();
    TestRegistrationMessage();
}

/**
 * \ingroup mpi-tests
 * \brief WiFi MPI message test suite
 */
class WifiMpiMessageTestSuite : public TestSuite
{
  public:
    WifiMpiMessageTestSuite();
};

WifiMpiMessageTestSuite::WifiMpiMessageTestSuite()
    : TestSuite("wifi-mpi-message", UNIT)
{
    AddTestCase(new WifiMpiMessageSerializationTest, TestCase::QUICK);
}

static WifiMpiMessageTestSuite g_wifiMpiMessageTestSuite;
