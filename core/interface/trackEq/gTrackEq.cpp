#include "trackEq/trackEq.h"

#include "mtStructs.h"

#include <stdio.h>

// Screen layout (800 x 480): title bar, EQ graph, four band columns, the eight bottom labels.
namespace
{
constexpr int kGraphY = 34;
constexpr int kGraphH = 218;
constexpr int kColumnsY = 262;
constexpr int kColumnW = 200;
constexpr int kRowH = 30;
} // namespace

void cTrackEqEditor::initDisplayControls()
{
    graphColors[0] = display.getTheme().waveformColors[0];
    graphColors[1] = display.getTheme().waveformColors[1];
    graphColors[2] = display.getTheme().waveformPointsColors[1];
    graphColors[3] = display.getTheme().waveformPointsColors[0];

    strControlProperties prop;

    prop.colors = graphColors;
    prop.data = &graphData;
    prop.style = controlStyleShow;
    prop.x = 2;
    prop.y = kGraphY;
    prop.w = 796;
    prop.h = kGraphH;
    graph = display.createControl<cEqGraph>(&prop);

    for (int b = 0; b < kBands; ++b)
    {
        strControlProperties head;
        head.colors = display.getTheme().activeLabelsColors;
        head.text = headerText[b];
        head.style = (controlStyleShow | controlStyleCenterX | controlStyleCenterY |
                      controlStyleFont3);
        head.x = kColumnW * b + kColumnW / 2;
        head.y = kColumnsY;
        head.w = kColumnW - 6;
        head.h = kRowH;
        head.value = 0;
        bandHeader[b] = display.createControl<cLabel>(&head);

        for (int r = 0; r < 4; ++r)
        {
            strControlProperties row;
            row.colors = display.getTheme().activeLabelsColors;
            row.text = valueText[b][r];
            row.style = (controlStyleShow | controlStyleCenterX | controlStyleCenterY |
                         controlStyleFont2);
            row.x = kColumnW * b + kColumnW / 2;
            row.y = kColumnsY + kRowH * (r + 1);
            row.w = kColumnW - 6;
            row.h = kRowH;
            row.value = 0;
            bandValue[b][r] = display.createControl<cLabel>(&row);
        }
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        prop.colors = display.getTheme().activeLabelsColors;
        prop.data = nullptr;
        prop.text = "";
        prop.style = (controlStyleShow | controlStyleCenterX | controlStyleCenterY |
                      controlStyleFont3);
        prop.x = (800 / 8) * i + (800 / 16);
        prop.y = 424;
        prop.w = 800 / 8 - 6;
        prop.h = 55;
        prop.value = 1;
        label[i] = display.createControl<cLabel>(&prop);
    }

    strControlProperties bg;
    bg.text = nullptr;
    bg.colors = nullptr;
    bg.value = 255;
    bg.style = controlStyleNoTransparency | controlStyleShow;
    bg.x = 0;
    bg.w = 800;
    bg.y = 425;
    bg.h = 54;
    bgLabel = display.createControl<cBgLabel>(&bg);
}
