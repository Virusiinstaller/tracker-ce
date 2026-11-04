#include "mtEffectEQ4.h"
#include "mtStructs.h"

#include <algorithm>
#include <math.h>
#include <string.h>

void mtEffectEQ4::setParameter(float& value, int idx)
{
    const int band = idx / kParamsPerBand;
    if (band < 0 || band >= kBands)
    {
        return;
    }
    switch (idx % kParamsPerBand)
    {
        case 0: freq[band] = value; break;
        case 1: gain[band] = value; break;
        default: q[band] = value; break;
    }
}

bool mtEffectEQ4::startProcess()
{
    length = confirmed[0].selection.length;

    static const uint8_t kTypes[kBands] = { trackeq::kEqLowShelf,
                                            trackeq::kEqPeak,
                                            trackeq::kEqPeak,
                                            trackeq::kEqHighShelf };

    trackeq::TrackEqParams params;
    anyBandActive = false;
    for (int b = 0; b < kBands; ++b)
    {
        trackeq::EqBand& band = params.band[b];
        band.type = kTypes[b];
        band.freqHz = (uint16_t)lroundf(freq[b]);
        band.gainDb10 = (int16_t)lroundf(gain[b] * 10.0f);
        band.q100 = (uint16_t)lroundf(q[b] * 100.0f);
        band.slope = 0;
        trackeq::eqSanitize(band);
        band.on = band.gainDb10 != 0;
        anyBandActive = anyBandActive || band.on;
    }

    for (int channel = 0; channel < kChannels; channel++)
    {
        channelParams[channel].srcAddr = confirmed[channel].selection.addr;
        channelParams[channel].dstAddr = processed[channel].selection.addr;
        channelParams[channel].eq.reset();
        channelParams[channel].eq.set(params);
    }

    processedSamples = 0;
    state = true;
    return true;
}

int32_t mtEffectEQ4::processBlock()
{
    if (!state)
    {
        return -1;
    }

    const uint32_t toProcess = length - processedSamples;
    const uint32_t blockLength = std::min<uint32_t>(toProcess, kUpdateBlockLength);

    for (int channel = 0; channel < kChannels; channel++)
    {
        ChannelParams& cp = channelParams[channel];

        if (!anyBandActive)
        {
            memcpy(cp.dstAddr, cp.srcAddr, blockLength * sizeof(int16_t));
            cp.dstAddr += blockLength;
            cp.srcAddr += blockLength;
            continue;
        }

        float x[kChunk];
        uint32_t done = 0;
        while (done < blockLength)
        {
            const uint32_t n = std::min<uint32_t>(kChunk, blockLength - done);
            for (uint32_t i = 0; i < n; ++i)
            {
                x[i] = (float)cp.srcAddr[i] * (1.0f / 32768.0f);
            }
            cp.eq.process(x, n);
            for (uint32_t i = 0; i < n; ++i)
            {
                float v = x[i] * 32768.0f;
                v = v > 32767.0f ? 32767.0f : (v < -32768.0f ? -32768.0f : v);
                cp.dstAddr[i] = (int16_t)lroundf(v);
            }
            cp.srcAddr += n;
            cp.dstAddr += n;
            done += n;
        }
    }

    if (blockLength < kUpdateBlockLength)
    {
        state = false;
    }

    processedSamples += blockLength;

    if (!state)
    {
        for (int channel = 0; channel < kChannels; channel++)
        {
            processed[channel].selection.length = processedSamples;
            processed[channel].area.length =
              confirmed[channel].area.length +
              (processed[channel].selection.length - confirmed[channel].selection.length);
        }
    }

    return blockLength;
}

bool mtEffectEQ4::getProcessState()
{
    return state;
}

uint32_t mtEffectEQ4::getExpectedProcessLength()
{
    return confirmed[0].selection.length;
}

mtEffectEQ4 sampleEditorEQ4;
