#include "SerialSatellite.h"
#include "CRSF.h"
#include "config.h"
#include "device.h"

#if defined(TARGET_RX)

extern RxConfig config;

static uint16_t crsfToSatellite(uint16_t crsfValue, uint8_t satelliteChannel, eSatelliteSystem protocol)
{
    static constexpr uint16_t SATELLITE_MIN_US = 903;
    static constexpr uint16_t SATELLITE_MAX_US = 2097;

    // Map the channel data
    const uint16_t us = constrain(CRSF_to_US(crsfValue), SATELLITE_MIN_US, SATELLITE_MAX_US);
    const float divisor = (protocol == DSM2_22MS) ? 1.166f
                                                  : 0.583f;

    uint16_t channelValue = roundf((us - SATELLITE_MIN_US) / divisor);

    // Encode the channel information
    channelValue |= satelliteChannel << ((protocol == DSM2_22MS) ? 10 : 11);

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    return __builtin_bswap16(channelValue);
#else
    return channelValue;
#endif // __BYTE_ORDER__
}

uint32_t SerialSatellite::sendRCFrame(bool frameAvailable, uint32_t *channelData)
{
    const eSatelliteSystem satelliteSystem = config.GetSatelliteSystem();
    const uint32_t callbackIntervalMs =
        ((satelliteSystem == DSMX_11MS) || (satelliteSystem == DSM2_11MS)) ? 11 : 22;

    if ((failsafe && config.GetFailsafeMode() == FAILSAFE_NO_PULSES) || (!sendPackets && connectionState != connected))
    {
        // Fade count does not overflow
        if (sendPackets && (fadeCount < UINT8_MAX))
        {
            ++fadeCount;
        }
        return callbackIntervalMs;
    }
    sendPackets = true;

    uint16_t outgoingPacket[SATELLITE_CHANNEL_DATA_LENGTH];
    for (uint8_t ii = 0; ii < SATELLITE_CHANNEL_DATA_LENGTH; ++ii)
    {
        // These channels are sent in every packet
        if (ii < SATELLITE_FIRST_RR_CHANNEL)
        {
            outgoingPacket[ii] = crsfToSatellite(channelData[ii],
                                                 crsfToSatelliteChannelMap[ii],
                                                 satelliteSystem);
        }
        // Round-robin channels
        else
        {
            outgoingPacket[ii] = crsfToSatellite(channelData[rr],
                                                 crsfToSatelliteChannelMap[rr],
                                                 satelliteSystem);

            // Update the round-robin index
            ++rr;
            if (rr >= SATELLITE_NUM_CHANNELS)
            {
                rr = SATELLITE_FIRST_RR_CHANNEL;
            }
        }
    }

    uint8_t satelliteSystemValue{};
    switch (satelliteSystem)
    {
    case DSMX_11MS:
        satelliteSystemValue = 0xB2;
        break;
    case DSMX_22MS:
        satelliteSystemValue = 0xA2;
        break;
    case DSM2_11MS:
        satelliteSystemValue = 0x12;
        break;
    default:
        satelliteSystemValue = 0x01;
    }

    // Transmit the fade count
    _outputPort->write(fadeCount);
    // Transmit the protocol in use
    _outputPort->write(satelliteSystemValue);
    // Transmit the channel data
    _outputPort->write(reinterpret_cast<uint8_t *>(outgoingPacket), sizeof(outgoingPacket)); // Channel data

    return callbackIntervalMs;
}

#endif // defined(TARGET_RX)
