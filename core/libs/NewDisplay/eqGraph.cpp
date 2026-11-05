#include "FT812.h"

#include "display.h"
#include "eqGraph.h"

#include <math.h>
#include <stdio.h>

namespace
{
constexpr float kRangeDb = 15.0f;
constexpr float kFreqMin = 20.0f;
constexpr float kFreqRatio = 1000.0f; // 20 Hz .. 20 kHz
constexpr int kCurvePoints = 160;
} // namespace

void eqGraphColors(uint32_t (&colors)[5])
{
    colors[0] = DISP_RGB(60, 220, 90);   // response curve: green
    colors[1] = display.getTheme().waveformColors[1];   // frame and grid
    colors[2] = DISP_RGB(120, 120, 120);   // marker of a band that is off
    colors[3] = DISP_RGB(235, 45, 45);   // marker of a band that is on: red
    colors[4] = DISP_RGB(235, 45, 45);   // selection frames: red
}

cEqGraph::cEqGraph(strControlProperties* properties)
  : cDisplayControl(controlIdEqGraph, 1)
{
    colorsCount = 5;
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
    API_LINE_WIDTH(24);
    API_BEGIN(LINE_STRIP);
    for (int i = 0; i < kCurvePoints; ++i)
    {
        const float hz = kFreqMin * powf(kFreqRatio, (float)i / (float)(kCurvePoints - 1));
        API_VERTEX2F(px + (i * pw) / (kCurvePoints - 1), yOf(trackeq::eqResponseDb(p, hz)));
    }
    API_END();

    // band markers: on = colors[3], off = colors[2]; the selected band is larger; numbered 1..4
    static const char* const kNumbers[trackeq::kEqBands] = { "1", "2", "3", "4" };
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
            const int mx = xOf((float)b.freqHz);
            const int my = yOf(b.on ? gain : 0.0f);
            API_COLOR(b.on ? colors[3] : colors[2]);
            API_POINT_SIZE((selected ? 7 : 5) * 16);
            API_BEGIN(FTPOINTS);
            API_VERTEX2F(mx, my);
            API_END();
            API_CMD_TEXT(mx, my - 13, fonts[0].handle, OPT_CENTER, kNumbers[i], 1);
        }
    }

    // selection frames of the columns below the graph: the selected band and, when a parameter is
    // selected, its column (F5..F8) and its row inside the band column
    {
        API_COLOR(colors[4]);
        API_LINE_WIDTH(24);
        auto frame = [](int x, int y, int w, int h) {
            API_BEGIN(LINE_STRIP);
            API_VERTEX2F(x, y);
            API_VERTEX2F(x + w, y);
            API_VERTEX2F(x + w, y + h);
            API_VERTEX2F(x, y + h);
            API_VERTEX2F(x, y);
            API_END();
        };
        frame(kEqColumnW * graph->selectedBand + 1, kEqColumnsY, kEqColumnW - 3, kEqColumnH);
        if (graph->selectedParam >= 0)
        {
            frame(kEqColumnW * (trackeq::kEqBands + graph->selectedParam) + 1,
                  kEqColumnsY,
                  kEqColumnW - 3,
                  kEqColumnH);
            frame(kEqColumnW * graph->selectedBand + 5,
                  kEqColumnsY + kEqRowH * (graph->selectedParam + 1) + 1,
                  kEqColumnW - 11,
                  kEqRowH - 3);
        }
    }
}

//--------------------------------------------------------------------------------
const char* cEqPanel::bandName(int i)
{
    static const char* const kNames[trackeq::kEqBands] = { "Band 1", "Band 2", "Band 3", "Band 4" };
    return kNames[i];
}

void cEqPanel::create()
{
    eqGraphColors(colors);
    const Theme& th = display.getTheme();

    strControlProperties prop;
    prop.colors = colors;
    prop.data = &data;
    prop.style = 0;
    prop.x = 2;
    prop.y = kEqGraphY;
    prop.w = 796;
    prop.h = kEqGraphH;
    graph = display.createControl<cEqGraph>(&prop);

    for (int b = 0; b < trackeq::kEqBands; ++b)
    {
        strControlProperties h;
        h.colors = th.activeLabelsColors;
        h.text = headText[b];
        h.style = (controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
        h.x = kEqColumnW * b + kEqColumnW / 2;
        h.y = kEqColumnsY;
        h.w = kEqColumnW - 6;
        h.h = kEqRowH;
        h.value = 0;
        head[b] = display.createControl<cLabel>(&h);

        for (int r = 0; r < 4; ++r)
        {
            strControlProperties t;
            t.colors = th.activeLabelsColors;
            t.text = rowText[b][r];
            t.style = (controlStyleCenterX | controlStyleCenterY | controlStyleFont1);
            t.x = kEqColumnW * b + kEqColumnW / 2;
            t.y = kEqColumnsY + kEqRowH * (r + 1);
            t.w = kEqColumnW - 6;
            t.h = kEqRowH;
            t.value = 0;
            row[b][r] = display.createControl<cLabel>(&t);
        }
    }

    for (int k = 0; k < 4; ++k)
    {
        strControlProperties n;
        n.colors = th.activeLabelsColors;
        n.text = names[k];
        n.style = (controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
        n.x = kEqColumnW * (trackeq::kEqBands + k) + kEqColumnW / 2;
        n.y = kEqColumnsY;
        n.w = kEqColumnW - 6;
        n.h = kEqRowH;
        n.value = 0;
        nameLabel[k] = display.createControl<cLabel>(&n);

        strControlProperties v;
        v.colors = th.activeLabelsColors;
        v.text = values[k];
        v.style = (controlStyleCenterX | controlStyleCenterY | controlStyleFont4);
        v.x = n.x;
        v.y = kEqColumnsY + kEqRowH;
        v.w = kEqColumnW - 6;
        v.h = 2 * kEqRowH;
        v.value = 0;
        valueLabel[k] = display.createControl<cLabel>(&v);
    }
}

void cEqPanel::show()
{
    display.setControlShow(graph);
    display.setControlData(graph, &data);
    for (int b = 0; b < trackeq::kEqBands; ++b)
    {
        display.setControlShow(head[b]);
        for (int r = 0; r < 4; ++r)
        {
            display.setControlShow(row[b][r]);
        }
    }
    for (int k = 0; k < 4; ++k)
    {
        display.setControlShow(nameLabel[k]);
        display.setControlShow(valueLabel[k]);
    }
}

void cEqPanel::hide()
{
    display.setControlHide(graph);
    display.refreshControl(graph);
    for (int b = 0; b < trackeq::kEqBands; ++b)
    {
        display.setControlHide(head[b]);
        display.refreshControl(head[b]);
        for (int r = 0; r < 4; ++r)
        {
            display.setControlHide(row[b][r]);
            display.refreshControl(row[b][r]);
        }
    }
    for (int k = 0; k < 4; ++k)
    {
        display.setControlHide(nameLabel[k]);
        display.refreshControl(nameLabel[k]);
        display.setControlHide(valueLabel[k]);
        display.refreshControl(valueLabel[k]);
    }
}

void cEqPanel::refresh(const trackeq::TrackEqParams& p, int band, int param)
{
    const Theme& th = display.getTheme();

    data.params = &p;
    data.selectedBand = (int8_t)band;
    data.selectedParam = (int8_t)param;
    display.refreshControl(graph);

    // band columns: a band that is off is greyed out
    for (int i = 0; i < trackeq::kEqBands; ++i)
    {
        const trackeq::EqBand& b = p.band[i];
        snprintf(headText[i], sizeof(headText[i]), "%s", bandName(i));
        trackeq::eqFormatBand(i, b, rowText[i]);
        const uint32_t* c = b.on ? th.activeLabelsColors : th.inactiveLabelsColors;
        display.setControlColors(head[i], c);
        display.refreshControl(head[i]);
        for (int r = 0; r < 4; ++r)
        {
            display.setControlColors(row[i][r], c);
            display.refreshControl(row[i][r]);
        }
    }

    // parameter columns of the selected band
    const trackeq::EqBand& sel = p.band[band];
    static const char* const kNames[4] = { "Type", "Freq", "Gain", "Q" };
    for (int k = 0; k < 4; ++k)
    {
        const bool slope = (k == 2 && trackeq::eqSlopeAllowed(band, sel.type));
        const bool unused = (k == 2 && !slope && !trackeq::eqUsesGain(sel.type));
        snprintf(names[k], sizeof(names[k]), "%s", slope ? "Slope" : kNames[k]);
        snprintf(values[k], sizeof(values[k]), "%s", rowText[band][k]);
        const uint32_t* c = (sel.on && !unused) ? th.activeLabelsColors : th.inactiveLabelsColors;
        display.setControlColors(nameLabel[k], c);
        display.setControlColors(valueLabel[k], c);
        display.refreshControl(nameLabel[k]);
        display.refreshControl(valueLabel[k]);
    }
}
