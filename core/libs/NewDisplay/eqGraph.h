#ifndef _EQ_GRAPH_H_
#define _EQ_GRAPH_H_

#include "displayControls.h"
#include "track_eq.h"

// Data of the EQ response graph: which EQ to draw and which band is selected.
struct strEqGraphData
{
    const trackeq::TrackEqParams* params = nullptr;
    int8_t selectedBand = 0;
};

// Frequency response (20 Hz .. 20 kHz, log axis; +-15 dB) of a track EQ with a marker per band.
// colors[0] = curve, colors[1] = grid, colors[2] = band marker, colors[3] = selected band marker.
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

#endif
