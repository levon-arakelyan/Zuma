#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Slider.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCommon.h"
#include "KillerBallDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace KillerBallDialogGraphics
{
    const char *const kTitle = "KILLER BALL";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    const float kMinIntervalSec = 2.0f;
    const float kMaxIntervalSec = 15.0f;
    const float kIntervalStep = 1.0f;
    const float kIntervalSpan = kMaxIntervalSec - kMinIntervalSec;

    const float kMinFlightSec = 2.0f;
    const float kMaxFlightSec = 10.0f;
    const float kFlightStep = 0.5f;
    const float kFlightSpan = kMaxFlightSec - kMinFlightSec;

    const int kSliderHeight = 36;
    const int kIntervalLabelTopOffset = 10;
    const int kIntervalSliderTopOffset = 35;
    const int kFlightLabelTopOffset = 85;
    const int kFlightSliderTopOffset = 110;
    const int kExtraPreferredHeight = 155;

    const int kIntervalSliderId = 0;
    const int kFlightSliderId = 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
float KillerBallDialog::SnapInterval(float theValue)
{
    float snapped = (float)floor(theValue / KillerBallDialogGraphics::kIntervalStep + 0.5f) *
                    KillerBallDialogGraphics::kIntervalStep;
    if (snapped < KillerBallDialogGraphics::kMinIntervalSec)
        snapped = KillerBallDialogGraphics::kMinIntervalSec;
    if (snapped > KillerBallDialogGraphics::kMaxIntervalSec)
        snapped = KillerBallDialogGraphics::kMaxIntervalSec;
    return snapped;
}

float KillerBallDialog::SnapFlight(float theValue)
{
    float snapped = (float)floor(theValue / KillerBallDialogGraphics::kFlightStep + 0.5f) *
                    KillerBallDialogGraphics::kFlightStep;
    if (snapped < KillerBallDialogGraphics::kMinFlightSec)
        snapped = KillerBallDialogGraphics::kMinFlightSec;
    if (snapped > KillerBallDialogGraphics::kMaxFlightSec)
        snapped = KillerBallDialogGraphics::kMaxFlightSec;
    return snapped;
}

double KillerBallDialog::IntervalToSlider(float theSec)
{
    return (theSec - KillerBallDialogGraphics::kMinIntervalSec) /
           KillerBallDialogGraphics::kIntervalSpan;
}

double KillerBallDialog::FlightToSlider(float theSec)
{
    return (theSec - KillerBallDialogGraphics::kMinFlightSec) /
           KillerBallDialogGraphics::kFlightSpan;
}

std::string KillerBallDialog::FormatSecLabel(const char *thePrefix, float theSec)
{
    int hundredths = (int)floor(theSec * 100.0f + 0.5f);
    if ((hundredths % 100) == 0)
        return Sexy::StrFormat("%s %d seconds", thePrefix, hundredths / 100);
    if ((hundredths % 50) == 0)
        return Sexy::StrFormat("%s %.1f seconds", thePrefix, (float)hundredths / 100.0f);
    return Sexy::StrFormat("%s %.2f seconds", thePrefix, (float)hundredths / 100.0f);
}

KillerBallDialog::KillerBallDialog(float initialIntervalSec, float initialFlightSec)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_KillerBall, true,
                   KillerBallDialogGraphics::kTitle,
                   KillerBallDialogGraphics::kEmptyLines,
                   KillerBallDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mIntervalSec = SnapInterval(initialIntervalSec);
    mFlightSec = SnapFlight(initialFlightSec);

    mIntervalSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                                 KillerBallDialogGraphics::kIntervalSliderId, this);
    mIntervalSlider->SetValue(IntervalToSlider(mIntervalSec));

    mFlightSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                               KillerBallDialogGraphics::kFlightSliderId, this);
    mFlightSlider->SetValue(FlightToSlider(mFlightSec));

    mYesButton->mLabel = KillerBallDialogGraphics::kApplyLabel;
    mNoButton->mLabel = KillerBallDialogGraphics::kCancelLabel;
}

KillerBallDialog::~KillerBallDialog()
{
    delete mIntervalSlider;
    delete mFlightSlider;
}

void KillerBallDialog::SetIntervalFromSlider(double theVal)
{
    float sec = KillerBallDialogGraphics::kMinIntervalSec +
                (float)theVal * KillerBallDialogGraphics::kIntervalSpan;
    mIntervalSec = SnapInterval(sec);

    double snappedVal = IntervalToSlider(mIntervalSec);
    if (fabs(snappedVal - theVal) > 0.0001)
        mIntervalSlider->SetValue(snappedVal);
}

void KillerBallDialog::SetFlightFromSlider(double theVal)
{
    float sec = KillerBallDialogGraphics::kMinFlightSec +
                (float)theVal * KillerBallDialogGraphics::kFlightSpan;
    mFlightSec = SnapFlight(sec);

    double snappedVal = FlightToSlider(mFlightSec);
    if (fabs(snappedVal - theVal) > 0.0001)
        mFlightSlider->SetValue(snappedVal);
}

void KillerBallDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop();
    int width = GetWidth();

    mIntervalSlider->Resize(left, top + KillerBallDialogGraphics::kIntervalSliderTopOffset,
                            width, KillerBallDialogGraphics::kSliderHeight);
    mFlightSlider->Resize(left, top + KillerBallDialogGraphics::kFlightSliderTopOffset,
                          width, KillerBallDialogGraphics::kSliderHeight);
}

int KillerBallDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + KillerBallDialogGraphics::kExtraPreferredHeight;
}

void KillerBallDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mIntervalSlider);
    theWidgetManager->AddWidget(mFlightSlider);
}

void KillerBallDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mIntervalSlider);
    theWidgetManager->RemoveWidget(mFlightSlider);
}

void KillerBallDialog::SliderVal(int theId, double theVal)
{
    if (theId == KillerBallDialogGraphics::kIntervalSliderId)
        SetIntervalFromSlider(theVal);
    else if (theId == KillerBallDialogGraphics::kFlightSliderId)
        SetFlightFromSlider(theVal);
    MarkDirty();
}

void KillerBallDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int intervalY = GetTop() - mY + KillerBallDialogGraphics::kIntervalLabelTopOffset +
                    FONT_DIALOG->GetAscent();
    int flightY = GetTop() - mY + KillerBallDialogGraphics::kFlightLabelTopOffset +
                  FONT_DIALOG->GetAscent();

    g->DrawString(FormatSecLabel("Next killer every:", mIntervalSec), labelX, intervalY);
    g->DrawString(FormatSecLabel("Flight time:", mFlightSec), labelX, flightY);
}
