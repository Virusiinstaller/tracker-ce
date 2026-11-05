#include "trackEq/trackEq.h"

#include "mtStructs.h"

#include <stdio.h>

// Screen layout (800 x 480): title bar, EQ panel (graph + eight columns, see eqGraph.h), eight bottom labels.

void cTrackEqEditor::initDisplayControls()
{
    panel.create(); // graph, band columns, parameter columns

    strControlProperties prop;

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
