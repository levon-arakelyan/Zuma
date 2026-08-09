#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Slider.h>
#include <SexyAppFramework/Checkbox.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCheckbox.h"
#include "CircleCommon.h"
#include "MaxPowerDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace MaxPowerDialogGraphics
{
    const char *const kTitle = "MAX POWER";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";
    const char *const kQuietSoundsLabel = "Do not play power-up sounds";

    const int kMinPercent = 0;
    const int kMaxPercent = 100;
    const int kPercentStep = 1;
    const float kSliderSpan = (float)(kMaxPercent - kMinPercent);

    const int kSliderTopOffset = 40;
    const int kSliderHeight = 36;
    const int kLabelTopOffset = 10;
    const int kCheckboxTopOffset = 90;
    const int kExtraPreferredHeight = 140;

    const int kPercentSliderId = 0;
    const int kQuietSoundsCheckboxId = 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int MaxPowerDialog::SnapPercent(float theValue)
{
    int snapped = (int)floor(theValue / MaxPowerDialogGraphics::kPercentStep + 0.5f) *
                  MaxPowerDialogGraphics::kPercentStep;
    if (snapped < MaxPowerDialogGraphics::kMinPercent)
        snapped = MaxPowerDialogGraphics::kMinPercent;
    if (snapped > MaxPowerDialogGraphics::kMaxPercent)
        snapped = MaxPowerDialogGraphics::kMaxPercent;
    return snapped;
}

double MaxPowerDialog::PercentToSlider(int thePercent)
{
    return (thePercent - MaxPowerDialogGraphics::kMinPercent) /
           MaxPowerDialogGraphics::kSliderSpan;
}

MaxPowerDialog::MaxPowerDialog(int initialPercent, bool quietSounds)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_MaxPower, true,
                   MaxPowerDialogGraphics::kTitle,
                   MaxPowerDialogGraphics::kEmptyLines,
                   MaxPowerDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mPercent = SnapPercent((float)initialPercent);

    mPercentSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                                MaxPowerDialogGraphics::kPercentSliderId, this);
    mPercentSlider->SetValue(PercentToSlider(mPercent));

    // NULL listener: toggle is local UI only; Apply reads IsChecked().
    mQuietSoundsCheckbox = MakeCheckbox(MaxPowerDialogGraphics::kQuietSoundsCheckboxId, NULL);
    mQuietSoundsCheckbox->mChecked = quietSounds;

    mYesButton->mLabel = MaxPowerDialogGraphics::kApplyLabel;
    mNoButton->mLabel = MaxPowerDialogGraphics::kCancelLabel;
}

MaxPowerDialog::~MaxPowerDialog()
{
    delete mPercentSlider;
    delete mQuietSoundsCheckbox;
}

bool MaxPowerDialog::GetQuietSounds() const
{
    return mQuietSoundsCheckbox != NULL && mQuietSoundsCheckbox->IsChecked();
}

void MaxPowerDialog::SetPercentFromSlider(double theVal)
{
    float percent = MaxPowerDialogGraphics::kMinPercent +
                    (float)theVal * MaxPowerDialogGraphics::kSliderSpan;
    mPercent = SnapPercent(percent);

    double snappedVal = PercentToSlider(mPercent);
    if (fabs(snappedVal - theVal) > 0.0001)
        mPercentSlider->SetValue(snappedVal);
}

void MaxPowerDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop();
    int width = GetWidth();

    mPercentSlider->Resize(left, top + MaxPowerDialogGraphics::kSliderTopOffset,
                           width, MaxPowerDialogGraphics::kSliderHeight);
    mQuietSoundsCheckbox->Resize(left, top + MaxPowerDialogGraphics::kCheckboxTopOffset,
                                 mQuietSoundsCheckbox->mWidth, mQuietSoundsCheckbox->mHeight);
}

int MaxPowerDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + MaxPowerDialogGraphics::kExtraPreferredHeight;
}

void MaxPowerDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mPercentSlider);
    theWidgetManager->AddWidget(mQuietSoundsCheckbox);
}

void MaxPowerDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mPercentSlider);
    theWidgetManager->RemoveWidget(mQuietSoundsCheckbox);
}

void MaxPowerDialog::SliderVal(int theId, double theVal)
{
    if (theId == MaxPowerDialogGraphics::kPercentSliderId)
    {
        SetPercentFromSlider(theVal);
        MarkDirty();
    }
}

void MaxPowerDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int labelY = GetTop() - mY + MaxPowerDialogGraphics::kLabelTopOffset + FONT_DIALOG->GetAscent();
    g->DrawString(Sexy::StrFormat("Powered balls: %d%%", mPercent), labelX, labelY);

    DrawCheckboxText(g, MaxPowerDialogGraphics::kQuietSoundsLabel, mQuietSoundsCheckbox);
}
