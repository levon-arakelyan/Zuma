#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Slider.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCommon.h"
#include "SonicDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace SonicDialogGraphics
{
    // Dialog chrome
    const char *const kTitle = "SONIC SPEED";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    // Speed range (slider steps of 0.5x)
    const float kMinMultiplier = 1.0f;
    const float kMaxMultiplier = 10.0f;
    const float kMultiplierStep = 0.5f;
    // slider 0..1 maps across (max - min) = 9.0
    const float kSliderSpan = kMaxMultiplier - kMinMultiplier;

    // Layout
    const int kSliderTopOffset = 40;
    const int kSliderHeight = 36;
    const int kLabelTopOffset = 10;

    // Dialog sizing
    const int kExtraPreferredHeight = 100;

    const int kSpeedSliderId = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
float SonicDialog::SnapMultiplier(float theValue)
{
    float snapped = (float)floor((theValue / SonicDialogGraphics::kMultiplierStep) + 0.5) *
                    SonicDialogGraphics::kMultiplierStep;
    if (snapped < SonicDialogGraphics::kMinMultiplier)
        snapped = SonicDialogGraphics::kMinMultiplier;
    if (snapped > SonicDialogGraphics::kMaxMultiplier)
        snapped = SonicDialogGraphics::kMaxMultiplier;
    return snapped;
}

double SonicDialog::MultiplierToSlider(float theMultiplier)
{
    return (theMultiplier - SonicDialogGraphics::kMinMultiplier) / SonicDialogGraphics::kSliderSpan;
}

SonicDialog::SonicDialog(float initialMultiplier)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_Sonic, true,
                   SonicDialogGraphics::kTitle,
                   SonicDialogGraphics::kEmptyLines,
                   SonicDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mMultiplier = SnapMultiplier(initialMultiplier);
    mSpeedSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                              SonicDialogGraphics::kSpeedSliderId, this);
    mSpeedSlider->SetValue(MultiplierToSlider(mMultiplier));

    mYesButton->mLabel = SonicDialogGraphics::kApplyLabel;
    mNoButton->mLabel = SonicDialogGraphics::kCancelLabel;
}

SonicDialog::~SonicDialog()
{
    delete mSpeedSlider;
}

void SonicDialog::SetMultiplierFromSlider(double theVal)
{
    float mult = SonicDialogGraphics::kMinMultiplier +
                 (float)theVal * SonicDialogGraphics::kSliderSpan;
    mMultiplier = SnapMultiplier(mult);

    double snappedVal = MultiplierToSlider(mMultiplier);
    if (fabs(snappedVal - theVal) > 0.0001)
        mSpeedSlider->SetValue(snappedVal);
}

void SonicDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop();
    int width = GetWidth();

    mSpeedSlider->Resize(left, top + SonicDialogGraphics::kSliderTopOffset,
                         width, SonicDialogGraphics::kSliderHeight);
}

int SonicDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + SonicDialogGraphics::kExtraPreferredHeight;
}

void SonicDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mSpeedSlider);
}

void SonicDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mSpeedSlider);
}

void SonicDialog::SliderVal(int theId, double theVal)
{
    if (theId == SonicDialogGraphics::kSpeedSliderId)
    {
        SetMultiplierFromSlider(theVal);
        MarkDirty();
    }
}

void SonicDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int labelY = GetTop() - mY + SonicDialogGraphics::kLabelTopOffset + FONT_DIALOG->GetAscent();
    g->DrawString(Sexy::StrFormat("Chain speed: %.1fx", mMultiplier), labelX, labelY);
}
