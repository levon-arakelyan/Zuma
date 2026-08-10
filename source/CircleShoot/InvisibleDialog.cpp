#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Slider.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCommon.h"
#include "InvisibleDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace InvisibleDialogGraphics
{
    const char *const kTitle = "INVISIBLE";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    const float kMinSec = 0.5f;
    const float kMaxSec = 10.0f;
    const float kSecStep = 0.25f;
    const float kSliderSpan = kMaxSec - kMinSec;

    const int kMinPercent = 4;
    const int kMaxPercent = 100;
    const int kPercentStep = 2;
    const float kPercentSpan = (float)(kMaxPercent - kMinPercent);

    const int kSliderHeight = 36;
    const int kDurationLabelTopOffset = 10;
    const int kDurationSliderTopOffset = 35;
    const int kIntervalLabelTopOffset = 85;
    const int kIntervalSliderTopOffset = 110;
    const int kPercentLabelTopOffset = 160;
    const int kPercentSliderTopOffset = 185;
    const int kExtraPreferredHeight = 230;

    const int kDurationSliderId = 0;
    const int kIntervalSliderId = 1;
    const int kPercentSliderId = 2;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
float InvisibleDialog::SnapSec(float theValue)
{
    float snapped = (float)floor(theValue / InvisibleDialogGraphics::kSecStep + 0.5f) *
                    InvisibleDialogGraphics::kSecStep;
    if (snapped < InvisibleDialogGraphics::kMinSec)
        snapped = InvisibleDialogGraphics::kMinSec;
    if (snapped > InvisibleDialogGraphics::kMaxSec)
        snapped = InvisibleDialogGraphics::kMaxSec;
    return snapped;
}

double InvisibleDialog::SecToSlider(float theSec)
{
    return (theSec - InvisibleDialogGraphics::kMinSec) /
           InvisibleDialogGraphics::kSliderSpan;
}

int InvisibleDialog::SnapPercent(float theValue)
{
    int snapped = (int)floor(theValue / InvisibleDialogGraphics::kPercentStep + 0.5f) *
                  InvisibleDialogGraphics::kPercentStep;
    if (snapped < InvisibleDialogGraphics::kMinPercent)
        snapped = InvisibleDialogGraphics::kMinPercent;
    if (snapped > InvisibleDialogGraphics::kMaxPercent)
        snapped = InvisibleDialogGraphics::kMaxPercent;
    return snapped;
}

double InvisibleDialog::PercentToSlider(int thePercent)
{
    return (thePercent - InvisibleDialogGraphics::kMinPercent) /
           InvisibleDialogGraphics::kPercentSpan;
}

std::string InvisibleDialog::FormatSecLabel(const char *thePrefix, float theSec)
{
    int hundredths = (int)floor(theSec * 100.0f + 0.5f);
    if ((hundredths % 100) == 0)
        return Sexy::StrFormat("%s %d seconds", thePrefix, hundredths / 100);
    if ((hundredths % 50) == 0)
        return Sexy::StrFormat("%s %.1f seconds", thePrefix, (float)hundredths / 100.0f);
    return Sexy::StrFormat("%s %.2f seconds", thePrefix, (float)hundredths / 100.0f);
}

InvisibleDialog::InvisibleDialog(float initialDurationSec, float initialIntervalSec, int initialPercent)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_Invisible, true,
                   InvisibleDialogGraphics::kTitle,
                   InvisibleDialogGraphics::kEmptyLines,
                   InvisibleDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mDurationSec = SnapSec(initialDurationSec);
    mIntervalSec = SnapSec(initialIntervalSec);
    mPercent = SnapPercent((float)initialPercent);

    mDurationSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                                 InvisibleDialogGraphics::kDurationSliderId, this);
    mDurationSlider->SetValue(SecToSlider(mDurationSec));

    mIntervalSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                                 InvisibleDialogGraphics::kIntervalSliderId, this);
    mIntervalSlider->SetValue(SecToSlider(mIntervalSec));

    mPercentSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                                InvisibleDialogGraphics::kPercentSliderId, this);
    mPercentSlider->SetValue(PercentToSlider(mPercent));

    mYesButton->mLabel = InvisibleDialogGraphics::kApplyLabel;
    mNoButton->mLabel = InvisibleDialogGraphics::kCancelLabel;
}

InvisibleDialog::~InvisibleDialog()
{
    delete mDurationSlider;
    delete mIntervalSlider;
    delete mPercentSlider;
}

void InvisibleDialog::SetDurationFromSlider(double theVal)
{
    float sec = InvisibleDialogGraphics::kMinSec +
                (float)theVal * InvisibleDialogGraphics::kSliderSpan;
    mDurationSec = SnapSec(sec);

    double snappedVal = SecToSlider(mDurationSec);
    if (fabs(snappedVal - theVal) > 0.0001)
        mDurationSlider->SetValue(snappedVal);
}

void InvisibleDialog::SetIntervalFromSlider(double theVal)
{
    float sec = InvisibleDialogGraphics::kMinSec +
                (float)theVal * InvisibleDialogGraphics::kSliderSpan;
    mIntervalSec = SnapSec(sec);

    double snappedVal = SecToSlider(mIntervalSec);
    if (fabs(snappedVal - theVal) > 0.0001)
        mIntervalSlider->SetValue(snappedVal);
}

void InvisibleDialog::SetPercentFromSlider(double theVal)
{
    float percent = InvisibleDialogGraphics::kMinPercent +
                    (float)theVal * InvisibleDialogGraphics::kPercentSpan;
    mPercent = SnapPercent(percent);

    double snappedVal = PercentToSlider(mPercent);
    if (fabs(snappedVal - theVal) > 0.0001)
        mPercentSlider->SetValue(snappedVal);
}

void InvisibleDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop();
    int width = GetWidth();

    mDurationSlider->Resize(left, top + InvisibleDialogGraphics::kDurationSliderTopOffset,
                            width, InvisibleDialogGraphics::kSliderHeight);
    mIntervalSlider->Resize(left, top + InvisibleDialogGraphics::kIntervalSliderTopOffset,
                            width, InvisibleDialogGraphics::kSliderHeight);
    mPercentSlider->Resize(left, top + InvisibleDialogGraphics::kPercentSliderTopOffset,
                           width, InvisibleDialogGraphics::kSliderHeight);
}

int InvisibleDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + InvisibleDialogGraphics::kExtraPreferredHeight;
}

void InvisibleDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mDurationSlider);
    theWidgetManager->AddWidget(mIntervalSlider);
    theWidgetManager->AddWidget(mPercentSlider);
}

void InvisibleDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mDurationSlider);
    theWidgetManager->RemoveWidget(mIntervalSlider);
    theWidgetManager->RemoveWidget(mPercentSlider);
}

void InvisibleDialog::SliderVal(int theId, double theVal)
{
    if (theId == InvisibleDialogGraphics::kDurationSliderId)
        SetDurationFromSlider(theVal);
    else if (theId == InvisibleDialogGraphics::kIntervalSliderId)
        SetIntervalFromSlider(theVal);
    else if (theId == InvisibleDialogGraphics::kPercentSliderId)
        SetPercentFromSlider(theVal);
    MarkDirty();
}

void InvisibleDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int durationY = GetTop() - mY + InvisibleDialogGraphics::kDurationLabelTopOffset +
                    FONT_DIALOG->GetAscent();
    int intervalY = GetTop() - mY + InvisibleDialogGraphics::kIntervalLabelTopOffset +
                    FONT_DIALOG->GetAscent();
    int percentY = GetTop() - mY + InvisibleDialogGraphics::kPercentLabelTopOffset +
                   FONT_DIALOG->GetAscent();

    g->DrawString(FormatSecLabel("Stay invisible for: ", mDurationSec), labelX, durationY);
    g->DrawString(FormatSecLabel("New wave every: ", mIntervalSec), labelX, intervalY);
    g->DrawString(Sexy::StrFormat("Invisible balls: %d%%", mPercent), labelX, percentY);
}
