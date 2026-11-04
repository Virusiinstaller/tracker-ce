#include "effect_trackEq.h"
#include "config.h"

void AudioEffectTrackEq::setParams(const trackeq::TrackEqParams& params)
{
    seq = seq + 1;   // odd: write in progress
    __asm volatile("" ::: "memory");
    pending = params;
    __asm volatile("" ::: "memory");
    seq = seq + 1;   // even: complete
}

AUDIO_CORE_RAM void AudioEffectTrackEq::update(void)
{
    // Take over new parameters when a complete set is waiting.
    const uint32_t s1 = seq;
    if (s1 != appliedSeq && (s1 & 1u) == 0)
    {
        const trackeq::TrackEqParams copy = pending;
        if (seq == s1)
        {
            appliedSeq = s1;
            eq.set(copy);
        }
    }

    if (!eq.active())
    {
        audio_block_t* block = receiveReadOnly(0);
        if (block)
        {
            transmit(block);
            release(block);
        }
        return;
    }

    audio_block_t* block = receiveWritable(0);
    if (!block)
    {
        return;
    }

    float x[AUDIO_BLOCK_SAMPLES];
    constexpr float kToFloat = 1.0f / 32768.0f;
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
    {
        x[i] = float(block->data[i]) * kToFloat;
    }
    eq.process(x, AUDIO_BLOCK_SAMPLES);
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
    {
        float v = x[i] * 32768.0f;
        if (v > 32767.0f)
        {
            v = 32767.0f;
        }
        else if (v < -32768.0f)
        {
            v = -32768.0f;
        }
        block->data[i] = int16_t(v < 0.0f ? v - 0.5f : v + 0.5f);
    }
    transmit(block);
    release(block);
}
