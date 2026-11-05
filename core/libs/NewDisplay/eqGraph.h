#ifndef _EQ_GRAPH_H_
#define _EQ_GRAPH_H_

#include "displayControls.h"
#include "track_eq.h"

// Layout shared by the Track EQ and the Sample Editor EQ4 screens (800 x 480): under the graph
// there are eight 100 px columns, F1..F4 = the four bands, F5..F8 = Type / Freq / Gain|Slope / Q
// of the selected band; each column is a header row plus four 30 px rows.
constexpr int kEqGraphY = 34;
constexpr int kEqGraphH = 218;
constexpr int kEqColumnsY = 262;
constexpr int kEqRowH = 30;
constexpr int kEqColumnH = 5 * kEqRowH - 2;
constexpr int kEqColumnW = 100;

// Fills the five colours of the control: grid from the theme, green curve, red selection / "on" marks,
// grey markers of bands that are off.
void eqGraphColors(uint32_t (&colors)[5]);

// Data of the EQ response graph: which EQ to draw and which band / parameter is selected.
// With selectedParam >= 0 the control also draws the selection frames of the columns below.
struct strEqGraphData
{
    const trackeq::TrackEqParams* params = nullptr;
    int8_t selectedBand = 0;
    int8_t selectedParam = -1;
    bool paramColumns = true; // the F5..F8 columns exist (and get a selection frame)
};

// Frequency response (20 Hz .. 20 kHz, log axis; +-15 dB) of a track EQ with a marker per band.
// colors[0] = curve, colors[1] = grid, colors[2] = marker of a band that is off, colors[3] = marker of a
// band that is on, colors[4] = selection frames. The selected band's marker is larger.
class cEqGraph : public cDisplayControl
{
  public:
    cEqGraph(strControlProperties* properties);
    virtual ~cEqGraph();

    virtual void update();

    virtual void setStyle(uint32_t style);
    virtual void setText(const char* text);
    virtual void setValue(int value);
    virtual void setColors(const uint32_t* colors);
    virtual void setData(void* data);

  private:
    strEqGraphData* graph = nullptr;
};

// The whole EQ panel of the two EQ screens: graph + eight columns (F1..F4 bands, F5..F8 the
// selected band's parameters) with the red selection frames. Needs 29 display controls, or 21
// without the parameter columns (the Sample Editor shows the parameters on its bottom labels).
class cEqPanel
{
  public:
    void create(bool parameterColumns); // from initDisplayControls()
    void show();
    void hide();
    // band = selected band, param = selected parameter or -1 (then only the band is framed)
    void refresh(const trackeq::TrackEqParams& p, int band, int param);

    // texts of the selected band's parameters (F5..F8): name and value
    const char* paramName(int k) const { return names[k]; }
    const char* paramValue(int k) const { return values[k]; }
    // text of the band header ("Band n") for the bottom labels
    static const char* bandName(int i);

  private:
    hControl graph = -1;
    hControl head[trackeq::kEqBands];
    hControl row[trackeq::kEqBands][4];
    hControl nameLabel[4];
    hControl valueLabel[4];
    char headText[trackeq::kEqBands][10];
    char rowText[trackeq::kEqBands][4][16];
    char names[4][8];
    char values[4][16];
    strEqGraphData data;
    uint32_t colors[5];
    bool paramColumns = true;
};

#endif
