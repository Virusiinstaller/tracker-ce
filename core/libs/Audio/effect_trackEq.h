#ifndef effect_trackEq_h_
#define effect_trackEq_h_

#include "AudioStream.h"
#include "track_eq.h"

// Audio-library wrapper of the per-track EQ: one mono channel in, one out.
// Parameters are handed over from the main thread with a sequence counter, so
// the audio interrupt never reads a half-written set. With no active band the
// block is passed through untouched (bit-identical to a build without the EQ).
class AudioEffectTrackEq : public AudioStream
{
  public:
    AudioEffectTrackEq()
      : AudioStream(1, inputQueueArray)
    {
    }
    virtual void update(void);

    // Main thread. Cheap; takes effect at the next audio block.
    void setParams(const trackeq::TrackEqParams& params);

  private:
    audio_block_t* inputQueueArray[1];
    trackeq::TrackEq eq;
    trackeq::TrackEqParams pending;
    volatile uint32_t seq = 0;   // odd while `pending` is being written
    uint32_t appliedSeq = 0;
};

#endif
