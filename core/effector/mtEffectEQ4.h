#ifndef EFFECTOR_MTEFFECTEQ4_H_
#define EFFECTOR_MTEFFECTEQ4_H_

#include "AudioStream.h"
#include "mtEffect.h"
#include "track_eq.h"

/*
 * EQ4: four-band parametric equalizer (same filters and band model as the per-track EQ).
 *
 * The bands are edited on the dedicated EQ4 screen of the Sample Editor (cSampleEditor), which
 * hands the whole set over with setBands(). Every band has a type (bell, shelves, cuts, notch),
 * frequency, gain, Q and an on/off flag; with all bands off the selection is copied unchanged.
 */
class mtEffectEQ4 : public mtEffect
{
  public:
    static constexpr int kBands = trackeq::kEqBands;

    mtEffectEQ4()
    {
        for (int b = 0; b < kBands; ++b)
        {
            bands.band[b] = trackeq::eqDefaultBand(b);
        }
    }

    void setBands(const trackeq::TrackEqParams& p) { bands = p; }
    const trackeq::TrackEqParams& getBands() const { return bands; }

    virtual void setParameter(float& value, int idx) override;
    virtual bool startProcess() override;
    virtual int32_t processBlock() override;
    virtual bool getProcessState() override;
    virtual uint32_t getExpectedProcessLength() override;

  private:
    static constexpr uint32_t kUpdateBlockLength = 8192;
    static constexpr uint32_t kChunk = 256;

    trackeq::TrackEqParams bands;

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
