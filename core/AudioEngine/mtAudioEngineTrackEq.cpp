#include "mtAudioEngine.h"

void audioEngine::setTrackEq(int track, const trackeq::TrackEqParams& params)
{
    if (track < 0 || track >= kVoices)
    {
        return;
    }
    trackeq::TrackEqParams p = params;
    for (int i = 0; i < trackeq::kEqBands; ++i)
    {
        trackeq::eqSanitize(p.band[i]);
        if (!trackeq::eqSlopeAllowed(i, p.band[i].type))
        {
            p.band[i].slope = 0;
        }
    }
    trackEqSet.track[track] = p;
#ifndef PLATFORM_IMX
    instrumentPlayer[track].getAudioVoice()->getTrackEq()->setParams(p);
#else
    for (int channel = 0; channel < AudioVoice::VoiceChannels::kChannelsCount; channel++)
    {
    instrumentPlayer[track]
          .getAudioVoice()->getTrackEq((AudioVoice::VoiceChannels)channel)
          ->setParams(p);
    }
#endif
}

void audioEngine::setTrackEqSet(const trackeq::TrackEqSet& set)
{
    for (int track = 0; track < kVoices; ++track)
    {
        setTrackEq(track, set.track[track]);
    }
}
