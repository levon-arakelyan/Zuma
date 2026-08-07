#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Slider.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCommon.h"
#include "MovingHoleDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace MovingHoleDialogGraphics
{
    const char *const kTitle = "MOVING HOLE";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    // Speed range: 0 = Steady (no move), 100 = Ultra fast (~0.1s old interval)
    const int kMinSpeed = 0;
    const int kMaxSpeed = 100;
    const int kSpeedStep = 1;
    const float kSliderSpan = (float)(kMaxSpeed - kMinSpeed);

    const int kSliderTopOffset = 40;
    const int kSliderHeight = 36;
    const int kLabelTopOffset = 10;
    const int kExtraPreferredHeight = 100;
    const int kSpeedSliderId = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int MovingHoleDialog::SnapSpeed(float theValue)
{
    int snapped = (int)floor(theValue / MovingHoleDialogGraphics::kSpeedStep + 0.5f) *
                  MovingHoleDialogGraphics::kSpeedStep;
    if (snapped < MovingHoleDialogGraphics::kMinSpeed)
        snapped = MovingHoleDialogGraphics::kMinSpeed;
    if (snapped > MovingHoleDialogGraphics::kMaxSpeed)
        snapped = MovingHoleDialogGraphics::kMaxSpeed;
    return snapped;
}

double MovingHoleDialog::SpeedToSlider(int theSpeed)
{
    return (theSpeed - MovingHoleDialogGraphics::kMinSpeed) /
           MovingHoleDialogGraphics::kSliderSpan;
}

const char *MovingHoleDialog::SpeedLabel(int theSpeed)
{
    if (theSpeed <= 0)
        return "Steady";
    if (theSpeed >= 100)
        return "Ultra fast";
    if (theSpeed <= 25)
        return "Slow";
    if (theSpeed <= 50)
        return "Medium";
    if (theSpeed <= 75)
        return "Fast";
    return "Very fast";
}

MovingHoleDialog::MovingHoleDialog(int initialSpeed)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_MovingHole, true,
                   MovingHoleDialogGraphics::kTitle,
                   MovingHoleDialogGraphics::kEmptyLines,
                   MovingHoleDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mSpeed = SnapSpeed((float)initialSpeed);
    mSpeedSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                              MovingHoleDialogGraphics::kSpeedSliderId, this);
    mSpeedSlider->SetValue(SpeedToSlider(mSpeed));

    mYesButton->mLabel = MovingHoleDialogGraphics::kApplyLabel;
    mNoButton->mLabel = MovingHoleDialogGraphics::kCancelLabel;
}

MovingHoleDialog::~MovingHoleDialog()
{
    delete mSpeedSlider;
}

void MovingHoleDialog::SetSpeedFromSlider(double theVal)
{
    float speed = MovingHoleDialogGraphics::kMinSpeed +
                  (float)theVal * MovingHoleDialogGraphics::kSliderSpan;
    mSpeed = SnapSpeed(speed);

    double snappedVal = SpeedToSlider(mSpeed);
    if (fabs(snappedVal - theVal) > 0.0001)
        mSpeedSlider->SetValue(snappedVal);
}

void MovingHoleDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop();
    int width = GetWidth();

    mSpeedSlider->Resize(left, top + MovingHoleDialogGraphics::kSliderTopOffset,
                         width, MovingHoleDialogGraphics::kSliderHeight);
}

int MovingHoleDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + MovingHoleDialogGraphics::kExtraPreferredHeight;
}

void MovingHoleDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mSpeedSlider);
}

void MovingHoleDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mSpeedSlider);
}

void MovingHoleDialog::SliderVal(int theId, double theVal)
{
    if (theId == MovingHoleDialogGraphics::kSpeedSliderId)
    {
        SetSpeedFromSlider(theVal);
        MarkDirty();
    }
}

void MovingHoleDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int labelY = GetTop() - mY + MovingHoleDialogGraphics::kLabelTopOffset + FONT_DIALOG->GetAscent();
    g->DrawString(Sexy::StrFormat("Hole speed: %s (%d)", SpeedLabel(mSpeed), mSpeed),
                  labelX, labelY);
}
