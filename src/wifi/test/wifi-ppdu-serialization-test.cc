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

#include "ns3/test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-ppdu-serialization.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"

using namespace ns3;

/**
 * \ingroup wifi-test
 * \brief Test PPDU serialization and deserialization
 */
class WifiPpduSerializationTest : public TestCase
{
  public:
    WifiPpduSerializationTest();
    ~WifiPpduSerializationTest() override;

  private:
    void DoRun() override;

    /**
     * Test basic PPDU serialization
     */
    void TestBasicSerialization();

    /**
     * Test PSDU byte-level serialization
     */
    void TestPsduSerialization();

    /**
     * Test TxVector serialization
     */
    void TestTxVectorSerialization();

    /**
     * Test round-trip serialization with different MCS values
     */
    void TestRoundTripWithDifferentMcs();
};

WifiPpduSerializationTest::WifiPpduSerializationTest()
    : TestCase("WiFi PPDU Serialization")
{
}

WifiPpduSerializationTest::~WifiPpduSerializationTest()
{
}

void
WifiPpduSerializationTest::TestBasicSerialization()
{
    // Create a simple packet
    Ptr<Packet> packet = Create<Packet>(100);

    // Create MAC header
    WifiMacHeader header;
    header.SetType(WIFI_MAC_QOSDATA);
    header.SetQosAckPolicy(WifiMacHeader::NORMAL_ACK);

    // Create PSDU
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(packet, header);

    // Create TxVector
    WifiTxVector txVector;
    txVector.SetMode(WifiMode("HtMcs7"));
    txVector.SetChannelWidth(20);
    txVector.SetNss(1);
    txVector.SetGuardInterval(800);
    txVector.SetPreambleType(WIFI_PREAMBLE_HT_MF);

    // Create operating channel
    WifiPhyOperatingChannel channel;
    channel.SetDefault(20, WIFI_STANDARD_80211n, WIFI_PHY_BAND_2_4GHZ);

    // Create PPDU
    uint64_t uid = 12345;
    Ptr<WifiPpdu> ppdu = Create<WifiPpdu>(psdu, txVector, channel, uid);

    // Serialize
    SerializedPpdu serialized = WifiPpduSerialization::SerializePpdu(ppdu);

    NS_TEST_ASSERT_MSG_EQ(serialized.ppduUid, uid, "PPDU UID mismatch");
    NS_TEST_ASSERT_MSG_EQ(serialized.mcs, 7, "MCS mismatch");
    NS_TEST_ASSERT_MSG_EQ(serialized.nss, 1, "NSS mismatch");
    NS_TEST_ASSERT_MSG_EQ(serialized.channelWidth, 20, "Channel width mismatch");
    NS_TEST_ASSERT_MSG_EQ(serialized.guardInterval, 800, "Guard interval mismatch");
    NS_TEST_ASSERT_MSG_GT(serialized.psduBytes.size(), 0, "PSDU bytes should not be empty");
}

void
WifiPpduSerializationTest::TestPsduSerialization()
{
    // Create packet with specific data
    std::vector<uint8_t> testData = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    Ptr<Packet> packet = Create<Packet>(testData.data(), testData.size());

    // Create MAC header
    WifiMacHeader header;
    header.SetType(WIFI_MAC_QOSDATA);

    // Create PSDU
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(packet, header);

    // Serialize PSDU
    std::vector<uint8_t> serializedBytes = WifiPpduSerialization::SerializePsdu(psdu);

    NS_TEST_ASSERT_MSG_GT(serializedBytes.size(), 0, "Serialized PSDU should not be empty");

    // Deserialize PSDU
    Ptr<WifiPsdu> psdu2 = WifiPpduSerialization::DeserializePsdu(serializedBytes, 1, false);

    NS_TEST_ASSERT_MSG_NE(psdu2, nullptr, "Deserialized PSDU should not be null");
    NS_TEST_ASSERT_MSG_EQ(psdu2->GetSize(), psdu->GetSize(), "PSDU size mismatch after round-trip");

    // Verify packet content
    Ptr<const Packet> reconstructedPacket = psdu2->GetPacket();
    uint8_t buffer[1000];
    uint32_t copiedSize = reconstructedPacket->CopyData(buffer, reconstructedPacket->GetSize());

    // Check that at least the original payload size is preserved
    NS_TEST_ASSERT_MSG_GE(copiedSize, testData.size(), "Reconstructed packet too small");
}

void
WifiPpduSerializationTest::TestTxVectorSerialization()
{
    // Create TxVector with specific parameters
    WifiTxVector txVector;
    txVector.SetMode(WifiMode("VhtMcs8"));
    txVector.SetChannelWidth(80);
    txVector.SetNss(2);
    txVector.SetGuardInterval(400);
    txVector.SetPreambleType(WIFI_PREAMBLE_VHT_SU);

    // Serialize
    uint8_t buffer[100];
    uint32_t size = WifiPpduSerialization::SerializeTxVector(txVector, buffer);

    NS_TEST_ASSERT_MSG_EQ(size,
                          WifiPpduSerialization::GetTxVectorSerializedSize(),
                          "TxVector serialized size mismatch");

    // Deserialize
    WifiTxVector txVector2;
    uint32_t readSize = WifiPpduSerialization::DeserializeTxVector(buffer, txVector2);

    NS_TEST_ASSERT_MSG_EQ(readSize, size, "TxVector deserialized size mismatch");
    NS_TEST_ASSERT_MSG_EQ(txVector2.GetChannelWidth(),
                          txVector.GetChannelWidth(),
                          "Channel width mismatch");
    NS_TEST_ASSERT_MSG_EQ(txVector2.GetNss(), txVector.GetNss(), "NSS mismatch");
    NS_TEST_ASSERT_MSG_EQ(txVector2.GetGuardInterval(),
                          txVector.GetGuardInterval(),
                          "Guard interval mismatch");
    NS_TEST_ASSERT_MSG_EQ(txVector2.GetPreambleType(),
                          txVector.GetPreambleType(),
                          "Preamble type mismatch");
}

void
WifiPpduSerializationTest::TestRoundTripWithDifferentMcs()
{
    // Test with different MCS values
    std::vector<std::string> modes = {"HtMcs0", "HtMcs7", "VhtMcs5", "HeMcs9"};
    std::vector<WifiPreamble> preambles = {WIFI_PREAMBLE_HT_MF,
                                           WIFI_PREAMBLE_HT_MF,
                                           WIFI_PREAMBLE_VHT_SU,
                                           WIFI_PREAMBLE_HE_SU};

    for (size_t i = 0; i < modes.size(); ++i)
    {
        // Create packet
        Ptr<Packet> packet = Create<Packet>(50 + i * 10);

        // Create MAC header
        WifiMacHeader header;
        header.SetType(WIFI_MAC_QOSDATA);

        // Create PSDU
        Ptr<WifiPsdu> psdu = Create<WifiPsdu>(packet, header);

        // Create TxVector
        WifiTxVector txVector;
        txVector.SetMode(WifiMode(modes[i]));
        txVector.SetChannelWidth(20);
        txVector.SetNss(1);
        txVector.SetGuardInterval(800);
        txVector.SetPreambleType(preambles[i]);

        // Create operating channel
        WifiPhyOperatingChannel channel;
        channel.SetDefault(20, WIFI_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);

        // Create PPDU
        uint64_t uid = 1000 + i;
        Ptr<WifiPpdu> ppdu = Create<WifiPpdu>(psdu, txVector, channel, uid);

        // Serialize
        SerializedPpdu serialized = WifiPpduSerialization::SerializePpdu(ppdu);

        // Verify serialization
        NS_TEST_ASSERT_MSG_EQ(serialized.ppduUid, uid, "PPDU UID mismatch for mode " + modes[i]);

        // Test binary serialization
        uint8_t buffer[2000];
        uint32_t size = serialized.Serialize(buffer, 2000);
        NS_TEST_ASSERT_MSG_GT(size, 0, "Serialization failed for mode " + modes[i]);

        // Deserialize
        SerializedPpdu serialized2;
        uint32_t readSize = serialized2.Deserialize(buffer, size);

        NS_TEST_ASSERT_MSG_EQ(readSize, size, "Deserialization size mismatch for mode " + modes[i]);
        NS_TEST_ASSERT_MSG_EQ(serialized2.ppduUid,
                              serialized.ppduUid,
                              "PPDU UID mismatch after round-trip for mode " + modes[i]);
        NS_TEST_ASSERT_MSG_EQ(serialized2.mcs,
                              serialized.mcs,
                              "MCS mismatch after round-trip for mode " + modes[i]);
        NS_TEST_ASSERT_MSG_EQ(serialized2.psduBytes.size(),
                              serialized.psduBytes.size(),
                              "PSDU size mismatch after round-trip for mode " + modes[i]);

        // Verify byte-for-byte PSDU match
        bool bytesMatch = true;
        for (size_t j = 0; j < serialized.psduBytes.size(); ++j)
        {
            if (serialized.psduBytes[j] != serialized2.psduBytes[j])
            {
                bytesMatch = false;
                break;
            }
        }
        NS_TEST_ASSERT_MSG_EQ(bytesMatch,
                              true,
                              "PSDU bytes mismatch after round-trip for mode " + modes[i]);
    }
}

void
WifiPpduSerializationTest::DoRun()
{
    TestBasicSerialization();
    TestPsduSerialization();
    TestTxVectorSerialization();
    TestRoundTripWithDifferentMcs();
}

/**
 * \ingroup wifi-test
 * \brief WiFi PPDU serialization test suite
 */
class WifiPpduSerializationTestSuite : public TestSuite
{
  public:
    WifiPpduSerializationTestSuite();
};

WifiPpduSerializationTestSuite::WifiPpduSerializationTestSuite()
    : TestSuite("wifi-ppdu-serialization", UNIT)
{
    AddTestCase(new WifiPpduSerializationTest, TestCase::QUICK);
}

static WifiPpduSerializationTestSuite g_wifiPpduSerializationTestSuite;
