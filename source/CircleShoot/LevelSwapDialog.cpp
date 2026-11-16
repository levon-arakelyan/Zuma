#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Slider.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCommon.h"
#include "LevelSwapDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace LevelSwapDialogGraphics
{
    const char *const kTitle = "LEVEL SWAP";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    const int kMinSec = 1;
    const int kMaxSec = 60;
    const int kStep = 1;
    const float kSpan = (float)(kMaxSec - kMinSec);

    const int kSliderTopOffset = 40;
    const int kSliderHeight = 36;
    const int kLabelTopOffset = 10;
    const int kExtraPreferredHeight = 100;
    const int kSliderId = 0;
}

int LevelSwapDialog::SnapInterval(float theValue)
{
    int snapped = (int)floor(theValue / LevelSwapDialogGraphics::kStep + 0.5f) *
                  LevelSwapDialogGraphics::kStep;
    if (snapped < LevelSwapDialogGraphics::kMinSec)
        snapped = LevelSwapDialogGraphics::kMinSec;
    if (snapped > LevelSwapDialogGraphics::kMaxSec)
        snapped = LevelSwapDialogGraphics::kMaxSec;
    return snapped;
}

double LevelSwapDialog::IntervalToSlider(int theSec)
{
    return (theSec - LevelSwapDialogGraphics::kMinSec) / LevelSwapDialogGraphics::kSpan;
}

LevelSwapDialog::LevelSwapDialog(int initialSec)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_LevelSwap, true,
                   LevelSwapDialogGraphics::kTitle,
                   LevelSwapDialogGraphics::kEmptyLines,
                   LevelSwapDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mIntervalSec = SnapInterval((float)initialSec);
    mIntervalSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                                 LevelSwapDialogGraphics::kSliderId, this);
    mIntervalSlider->SetValue(IntervalToSlider(mIntervalSec));

    mYesButton->mLabel = LevelSwapDialogGraphics::kApplyLabel;
    mNoButton->mLabel = LevelSwapDialogGraphics::kCancelLabel;
}

LevelSwapDialog::~LevelSwapDialog()
{
    delete mIntervalSlider;
}

void LevelSwapDialog::SetIntervalFromSlider(double theVal)
{
    float sec = LevelSwapDialogGraphics::kMinSec + (float)theVal * LevelSwapDialogGraphics::kSpan;
    mIntervalSec = SnapInterval(sec);

    double snappedVal = IntervalToSlider(mIntervalSec);
    if (fabs(snappedVal - theVal) > 0.0001)
        mIntervalSlider->SetValue(snappedVal);
}

void LevelSwapDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop();
    int width = GetWidth();

    mIntervalSlider->Resize(left, top + LevelSwapDialogGraphics::kSliderTopOffset,
                            width, LevelSwapDialogGraphics::kSliderHeight);
}

int LevelSwapDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + LevelSwapDialogGraphics::kExtraPreferredHeight;
}

void LevelSwapDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mIntervalSlider);
}

void LevelSwapDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mIntervalSlider);
}

void LevelSwapDialog::SliderVal(int theId, double theVal)
{
    if (theId == LevelSwapDialogGraphics::kSliderId)
    {
        SetIntervalFromSlider(theVal);
        MarkDirty();
    }
}

void LevelSwapDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int labelY = GetTop() - mY + LevelSwapDialogGraphics::kLabelTopOffset + FONT_DIALOG->GetAscent();
    g->DrawString(Sexy::StrFormat("Swap every %d s", mIntervalSec), labelX, labelY);
}
