#include "FT812.h"

#include "display.h"
#include "eqGraph.h"

#include <math.h>

namespace
{
constexpr float kRangeDb = 15.0f;
constexpr float kFreqMin = 20.0f;
constexpr float kFreqRatio = 1000.0f; // 20 Hz .. 20 kHz
constexpr int kCurvePoints = 160;
} // namespace

cEqGraph::cEqGraph(strControlProperties* properties)
  : cDisplayControl(controlIdEqGraph, 1)
{
    colorsCount = 4;
    colors = display.getTheme().waveformPointsColors;

    if (properties == nullptr)
    {
        return;
    }

    colors = (properties->colors == nullptr) ? colors : properties->colors;
    graph = (strEqGraphData*)properties->data;
    posX = properties->x;
    posY = properties->y;
    width = properties->w;
    height = properties->h;
    setStyle(properties->style);
}

cEqGraph::~cEqGraph() {}

void cEqGraph::setStyle(uint32_t style)
{
    this->style = style;
}

void cEqGraph::setText(const char* text) {}

void cEqGraph::setValue(int value)
{
    this->value = value;
}

void cEqGraph::setColors(const uint32_t* colors)
{
    this->colors = colors;
}

void cEqGraph::setData(void* data)
{
    graph = (strEqGraphData*)data;
}

void cEqGraph::update()
{
    const int px = posX + 6;
    const int pw = width - 12;
    const int py = posY + 6;
    const int ph = height - 12;

    auto yOf = [&](float db) -> int {
        if (db > kRangeDb)
        {
            db = kRangeDb;
        }
        else if (db < -kRangeDb)
        {
            db = -kRangeDb;
        }
        return py + (int)((kRangeDb - db) / (2.0f * kRangeDb) * (float)ph + 0.5f);
    };
    auto xOf = [&](float hz) -> int {
        return px + (int)(logf(hz / kFreqMin) / logf(kFreqRatio) * (float)pw + 0.5f);
    };

    // frame and grid
    API_COLOR(colors[1]);
    API_LINE_WIDTH(8);
    API_BEGIN(LINE_STRIP);
    API_VERTEX2F(posX, posY);
    API_VERTEX2F(posX + width, posY);
    API_VERTEX2F(posX + width, posY + height);
    API_VERTEX2F(posX, posY + height);
    API_VERTEX2F(posX, posY);
    API_END();

    API_BEGIN(LINES);
    static const float kVLines[] = { 100.0f, 1000.0f, 10000.0f };
    for (float hz : kVLines)
    {
        const int x = xOf(hz);
        API_VERTEX2F(x, py);
        API_VERTEX2F(x, py + ph);
    }
    static const float kHLines[] = { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f };
    for (float db : kHLines)
    {
        const int y = yOf(db);
        API_VERTEX2F(px, y);
        API_VERTEX2F(px + pw, y);
    }
    API_END();

    if (graph == nullptr || graph->params == nullptr)
    {
        return;
    }
    const trackeq::TrackEqParams& p = *graph->params;

    // response curve
    API_COLOR(colors[0]);
    API_LINE_WIDTH(16);
    API_BEGIN(LINE_STRIP);
    for (int i = 0; i < kCurvePoints; ++i)
    {
        const float hz = kFreqMin * powf(kFreqRatio, (float)i / (float)(kCurvePoints - 1));
        API_VERTEX2F(px + (i * pw) / (kCurvePoints - 1), yOf(trackeq::eqResponseDb(p, hz)));
    }
    API_END();

    // band markers: the selected band is bigger and in its own colour
    for (int pass = 0; pass < 2; ++pass)
    {
        for (int i = 0; i < trackeq::kEqBands; ++i)
        {
            const bool selected = (i == graph->selectedBand);
            if (selected != (pass == 1))
            {
                continue;
            }
            const trackeq::EqBand& b = p.band[i];
            const float gain = trackeq::eqUsesGain(b.type) ? (float)b.gainDb10 / 10.0f : 0.0f;
            API_COLOR(selected ? colors[3] : colors[2]);
            API_POINT_SIZE((selected ? 7 : 5) * 16);
            API_BEGIN(FTPOINTS);
            API_VERTEX2F(xOf((float)b.freqHz), yOf(b.on ? gain : 0.0f));
            API_END();
        }
    }
}
