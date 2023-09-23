#include "SerialIO.h"

class SerialSatellite : public SerialIO
{
public:
    SerialSatellite(Stream &out, Stream &in)
        : SerialIO(&out, &in) {}

    virtual ~SerialSatellite() = default;

    void queueLinkStatisticsPacket() override {}
    void queueMSPFrameTransmission(uint8_t *data) override {}
    uint32_t sendRCFrame(bool frameAvailable, uint32_t *channelData) override;

private:
    static constexpr uint16_t SATELLITE_NUM_CHANNELS = 12;
    static constexpr uint8_t SATELLITE_FIRST_RR_CHANNEL = 4;
    static constexpr uint8_t SATELLITE_CHANNEL_DATA_LENGTH = 7;

    void processBytes(uint8_t *bytes, uint16_t size) override{};

    static constexpr uint8_t crsfToSatelliteChannelMap[12] = {
        // clang-format off
        // CRSF         // Satellite
        /*Channel 0*/   1,
        /*Channel 1*/   2,
        /*Channel 2*/   0,
        /*Channel 3*/   3,
        /*Channel 4*/   11,
        /*Channel 5*/   4,
        /*Channel 6*/   5,
        /*Channel 7*/   6,
        /*Channel 8*/   7,
        /*Channel 9*/   8,
        /*Channel 10*/  9,
        /*Channel 11*/  10,
        // clang-format on
    };

    uint8_t fadeCount{0};
    uint8_t rr{SATELLITE_FIRST_RR_CHANNEL};
    bool sendPackets{false};
};