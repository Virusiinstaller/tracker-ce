#include "trackEq.h"

#include "fileManager.h"
#include "core/interfaceMenuPopup.h"
#include "core/songTimer.h"
#include "elapsedMillis.h"
#include "mtAudioEngine.h"
#include "mtPadBoard.h"
#include "mtPadsBacklight.h"
#include "mtSequencer.h"
#include "mtStructs.h"

#include <stdio.h>
#include <string.h>

cTrackEqEditor trackEqEditor;
static cTrackEqEditor* TE = &trackEqEditor;

// The EQ file is written shortly after the last edit, not on every jog-wheel click.
static constexpr uint32_t kSaveDelayMs = 400;
static elapsedMillis s_sinceEdit;
static bool s_saveDue = false;

static void functEncoder(int16_t value);
static void functSwitchModule(uint8_t button);
static void functActionButton(uint8_t button, uint8_t state);
static void functLeft();
static void functRight();
static void functUp();
static void functDown();
static void functDelete();
static void functPlayAction();
static void functMenu(uint8_t state);

static bool shiftHeld()
{
    return ModuleBase::buttons->isButtonPressed(interfaceButtonShift);
}

static void flushSave()
{
    if (s_saveDue)
    {
        s_saveDue = false;
        newFileManager.getTrackEqStore()->saveToWorkspace();
    }
}

//----------------------------------------------------------------------------------------------
void cTrackEqEditor::update()
{
    if (s_saveDue && s_sinceEdit > kSaveDelayMs)
    {
        flushSave();
    }
}

void cTrackEqEditor::start(uint32_t options)
{
    moduleRefresh = 1;

    track = (options < (uint32_t)trackeq::kEqTracks) ? (int)options : 0;
    if (band < 0 || band >= kBands)
    {
        band = 0;
    }
    s_saveDue = false;

    songTimer.hide();

    ip->setButtonObj(interfaceButtonPerformance, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonParams, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonFile, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonConfig, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonMaster, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSamplePlay, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleEdit, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleRec, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleLoad, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSong, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonPattern, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonGame, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonMiniMenu, functMenu);

    ip->setButtonObj(interfaceButtonPlay, buttonPress, functPlayAction);

    ip->setButtonObj(interfaceButtonLeft, buttonPress, functLeft);
    ip->setButtonObj(interfaceButtonRight, buttonPress, functRight);
    ip->setButtonObj(interfaceButtonUp, buttonPress, functUp);
    ip->setButtonObj(interfaceButtonDown, buttonPress, functDown);
    ip->setButtonObj(interfaceButtonDelete, buttonPress, functDelete);

    ip->setButtonObj(interfaceButton0, functActionButton);
    ip->setButtonObj(interfaceButton1, functActionButton);
    ip->setButtonObj(interfaceButton2, functActionButton);
    ip->setButtonObj(interfaceButton3, functActionButton);
    ip->setButtonObj(interfaceButton4, functActionButton);
    ip->setButtonObj(interfaceButton5, functActionButton);
    ip->setButtonObj(interfaceButton6, functActionButton);
    ip->setButtonObj(interfaceButton7, functActionButton);

    ip->setPotObj(interfacePot0, functEncoder, false);

    showScreen();
}

void cTrackEqEditor::stop()
{
    flushSave();
    moduleRefresh = 0;
    mtPadBoard.releaseAllInstruments();
    padsBacklight.clearGlobalPads(1, 1, 1);
    songTimer.show();
}

//----------------------------------------------------------------------------------------------
const trackeq::TrackEqParams& cTrackEqEditor::params() const
{
    return engine.getTrackEqSet().track[track];
}

void cTrackEqEditor::commit(const trackeq::TrackEqParams& p)
{
    engine.setTrackEq(track, p);
    s_saveDue = true;
    s_sinceEdit = 0;
}

void cTrackEqEditor::selectTrack(int newTrack)
{
    if (newTrack < 0 || newTrack >= trackeq::kEqTracks || newTrack == track)
    {
        return;
    }
    flushSave();
    track = newTrack;
    showScreen();
}

void cTrackEqEditor::selectBand(int newBand)
{
    if (newBand == band)
    {
        // pressing the selected band again switches it on or off
        trackeq::TrackEqParams p = params();
        p.band[band].on = !p.band[band].on;
        commit(p);
    }
    else
    {
        band = newBand;
    }
    refreshValues();
}

void cTrackEqEditor::selectParam(int newParam)
{
    param = newParam;
    refreshValues();
}

void cTrackEqEditor::changeValue(int steps, bool coarse)
{
    if (steps == 0)
    {
        return;
    }
    trackeq::TrackEqParams p = params();
    trackeq::EqBand& b = p.band[band];

    switch (param)
    {
        case 0:
            b.type = trackeq::eqStepType(b.type, steps > 0 ? 1 : -1);
            if (!trackeq::eqSlopeAllowed(band, b.type))
            {
                b.slope = 0; // no hidden 48 dB on a bell
            }
            break;
        case 1: b.freqHz = trackeq::eqStepFreq(b.freqHz, steps, coarse); break;
        case 2:
            if (trackeq::eqSlopeAllowed(band, b.type))
            {
                b.slope = b.slope ? 0 : 1; // 12 <-> 48 dB/oct
            }
            else if (trackeq::eqUsesGain(b.type))
            {
                b.gainDb10 = trackeq::eqStepGain(b.gainDb10, steps, coarse);
            }
            else
            {
                return;
            }
            break;
        default: b.q100 = trackeq::eqStepQ(b.q100, steps, coarse); break;
    }
    b.on = true; // an edited band is audible at once
    commit(p);
    refreshValues();
}

void cTrackEqEditor::resetBand()
{
    trackeq::TrackEqParams p = params();
    p.band[band] = trackeq::eqDefaultBand(band);
    commit(p);
    refreshValues();
}

//----------------------------------------------------------------------------------------------
void cTrackEqEditor::showBandColumn(int i)
{
    const trackeq::EqBand& b = params().band[i];
    snprintf(headerText[i], sizeof(headerText[i]), "Band %d", i + 1);
    trackeq::eqFormatBand(i, b, valueText[i]);

    const Theme& th = display.getTheme();
    const uint32_t* base = b.on ? th.activeLabelsColors : th.inactiveLabelsColors;
    display.setControlColors(bandHeader[i], i == band ? th.highlightedButtonLabelsColors : base);
    display.refreshControl(bandHeader[i]);
    for (int r = 0; r < 4; ++r)
    {
        const bool selected = (i == band && r == param);
        display.setControlColors(bandValue[i][r], selected ? th.highlightedButtonLabelsColors : base);
        display.refreshControl(bandValue[i][r]);
    }
}

void cTrackEqEditor::showBottomLabels()
{
    const trackeq::EqBand& sel = params().band[band];
    const Theme& th = display.getTheme();

    for (int i = 0; i < 8; ++i)
    {
        const char* t = "";
        const char* t2 = "";
        const uint32_t* colors = th.activeLabelsColors;

        if (i < kBands)
        {
            static const char* const kBandNames[4] = { "Band 1", "Band 2", "Band 3", "Band 4" };
            t = kBandNames[i];
            t2 = params().band[i].on ? "On" : "Off";
            if (i == band)
            {
                colors = th.highlightedButtonLabelsColors;
            }
        }
        else
        {
            static const char* const kNames[4] = { "Type", "Freq", "Gain", "Q" };
            const int p = i - kBands;
            t = kNames[p];
            if (p == 2 && trackeq::eqSlopeAllowed(band, sel.type))
            {
                t = "Slope";
            }
            if (p == param)
            {
                colors = th.highlightedButtonLabelsColors;
            }
        }
        display.setControlText(label[i], t);
        display.setControlText2(label[i], t2);
        display.setControlColors(label[i], colors);
        display.setControlValue(label[i], i < kBands ? 1 : 0);
        display.refreshControl(label[i]);
    }
}

void cTrackEqEditor::refreshValues()
{
    graphData.params = &params();
    graphData.selectedBand = (int8_t)band;
    display.refreshControl(graph);
    for (int i = 0; i < kBands; ++i)
    {
        showBandColumn(i);
    }
    showBottomLabels();
    display.refreshControl(bgLabel);
    display.synchronizeRefresh();
}

void cTrackEqEditor::showScreen()
{
    snprintf(titleText, sizeof(titleText), "Track %d EQ", track + 1);
    titleBar->show();
    titleBar->setTitle(titleText);

    for (int i = 0; i < kBands; ++i)
    {
        display.setControlShow(bandHeader[i]);
        for (int r = 0; r < 4; ++r)
        {
            display.setControlShow(bandValue[i][r]);
            display.setControlText(bandValue[i][r], valueText[i][r]);
        }
        display.setControlText(bandHeader[i], headerText[i]);
    }
    display.setControlShow(graph);
    display.setControlData(graph, &graphData);
    refreshValues();
}

//----------------------------------------------------------------------------------------------
static void functEncoder(int16_t value)
{
    TE->changeValue(value, shiftHeld());
}

static void functUp()
{
    TE->changeValue(1, shiftHeld());
}

static void functDown()
{
    TE->changeValue(-1, shiftHeld());
}

static void functLeft()
{
    TE->selectTrack(TE->track - 1);
}

static void functRight()
{
    TE->selectTrack(TE->track + 1);
}

static void functDelete()
{
    TE->resetBand();
}

static void functActionButton(uint8_t button, uint8_t state)
{
    if (state != buttonPress)
    {
        return;
    }
    if (button < 4)
    {
        TE->selectBand(button);
    }
    else if (button < 8)
    {
        TE->selectParam(button - 4);
    }
}

static void functSwitchModule(uint8_t button)
{
    TE->eventFunct(eventSwitchModule, TE, &button, 0);
}

static void functPlayAction()
{
    // transport keeps working while the EQ is open
    if (sequencer.getSeqState() == Sequencer::SEQ_STATE_STOP)
    {
        if (TE->buttons->isButtonPressed(interfaceButtonShift))
        {
            sequencer.playSong();
        }
        else
        {
            sequencer.playPattern();
        }
    }
    else
    {
        sequencer.stop();
    }
}

static void functMenu(uint8_t state)
{
    (void)state;
}
