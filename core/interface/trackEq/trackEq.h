#ifndef INTERFACE_TRACKEQ_H_
#define INTERFACE_TRACKEQ_H_

#include "core/modulesBase.h"
#include "track_eq.h"

// Per-track 4-band EQ editor (tracks 1-8). Opened from Pattern > More (page 3) and from the
// mixer (hold a track button + Insert).
//
//   F1..F4  select band 1..4 (press the selected band again: on / off)
//   F5..F8  select the parameter: Type, Frequency, Gain (Slope on the cut bands), Q
//   Jog     change the selected parameter (Shift = coarse)
//   Up/Down same as the jog wheel by one step
//   Left/Right previous / next track
//   Delete  reset the selected band to its factory layout (off)
class cTrackEqEditor : public ModuleBase
{
  public:
    virtual void update();
    virtual void start(uint32_t options);
    virtual void stop();

    virtual void initDisplayControls();
    virtual void destroyDisplayControls() {}

    cTrackEqEditor() {}
    ~cTrackEqEditor() {}

    void selectTrack(int track);
    void selectBand(int band);
    void selectParam(int param);
    void changeValue(int steps, bool coarse);
    void resetBand();
    void showScreen();
    void refreshValues();

    int track = 0;
    int band = 0;
    int param = 1; // 0 type, 1 frequency, 2 gain / slope, 3 Q

  private:
    void showBottomLabels();
    const trackeq::TrackEqParams& params() const;
    void commit(const trackeq::TrackEqParams& p);

    static constexpr int kBands = trackeq::kEqBands;

    char titleText[24];

    hControl label[8];
    hControl bgLabel;
    cEqPanel panel;
};

extern cTrackEqEditor trackEqEditor;

#endif /* INTERFACE_TRACKEQ_H_ */
