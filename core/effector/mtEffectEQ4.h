#ifndef EFFECTOR_MTEFFECTEQ4_H_
#define EFFECTOR_MTEFFECTEQ4_H_

#include "AudioStream.h"
#include "mtEffect.h"
#include "track_eq.h"

/*
 * EQ4: four-band parametric equalizer (same filters as the per-track EQ).
 *
 * Band 1 = low shelf, bands 2 and 3 = bell, band 4 = high shelf. Every band has three parameters
 * (idx = band * 3 + n): frequency [Hz], gain [dB, +-15] and Q. A band with 0 dB gain does nothing;
 * with all four at 0 dB the selection is copied unchanged.
 */
class mtEffectEQ4 : public mtEffect
{
  public:
    static constexpr int kBands = 4;
    static constexpr int kParamsPerBand = 3;
    static constexpr int kParams = kBands * kParamsPerBand;

    virtual void setParameter(float& value, int idx) override;
    virtual bool startProcess() override;
    virtual int32_t processBlock() override;
    virtual bool getProcessState() override;
    virtual uint32_t getExpectedProcessLength() override;

  private:
    static constexpr uint32_t kUpdateBlockLength = 8192;
    static constexpr uint32_t kChunk = 256;

    float freq[kBands] = { 100.0f, 400.0f, 2500.0f, 8000.0f };
    float gain[kBands] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float q[kBands] = { 0.71f, 1.0f, 1.0f, 0.71f };

    struct ChannelParams
    {
        int16_t* srcAddr;
        int16_t* dstAddr;
        trackeq::TrackEq eq;
    };
    ChannelParams channelParams[kChannels];

    bool anyBandActive = false;
    uint32_t length = 0;
    uint32_t processedSamples = 0;
    bool state = false;
};

extern mtEffectEQ4 sampleEditorEQ4;

#endif /* EFFECTOR_MTEFFECTEQ4_H_ */
