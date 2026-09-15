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
#include "ShootSpeedDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace ShootSpeedDialogGraphics
{
    const char *const kTitle = "SHOOT SPEED";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";
    const char *const kInstantLabel = "Instant";

    // 0.1x..1x step 0.1, then 1x..5x step 0.5.
    const float kMultipliers[] = {
        0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f,
        1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 5.0f};
    const int kMultiplierCount = sizeof(kMultipliers) / sizeof(kMultipliers[0]);

    const int kSliderTopOffset = 40;
    const int kSliderHeight = 36;
    const int kLabelTopOffset = 10;
    const int kCheckboxTopOffset = 90;
    const int kExtraPreferredHeight = 140;

    const int kSpeedSliderId = 0;
    const int kInstantCheckboxId = 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ShootSpeedDialog::NearestMultiplierIndex(float theValue)
{
    int best = 0;
    float bestDist = (float)fabs(theValue - ShootSpeedDialogGraphics::kMultipliers[0]);
    for (int i = 1; i < ShootSpeedDialogGraphics::kMultiplierCount; i++)
    {
        float dist = (float)fabs(theValue - ShootSpeedDialogGraphics::kMultipliers[i]);
        if (dist < bestDist)
        {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

float ShootSpeedDialog::SnapMultiplier(float theValue)
{
    return ShootSpeedDialogGraphics::kMultipliers[NearestMultiplierIndex(theValue)];
}

double ShootSpeedDialog::MultiplierToSlider(float theMultiplier)
{
    int index = NearestMultiplierIndex(theMultiplier);
    if (ShootSpeedDialogGraphics::kMultiplierCount <= 1)
        return 0.0;
    return (double)index / (double)(ShootSpeedDialogGraphics::kMultiplierCount - 1);
}

ShootSpeedDialog::ShootSpeedDialog(float initialMultiplier, bool instant)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_ShootSpeed, true,
                   ShootSpeedDialogGraphics::kTitle,
                   ShootSpeedDialogGraphics::kEmptyLines,
                   ShootSpeedDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mMultiplier = SnapMultiplier(initialMultiplier);

    mSpeedSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                              ShootSpeedDialogGraphics::kSpeedSliderId, this);
    mSpeedSlider->SetValue(MultiplierToSlider(mMultiplier));

    mInstantCheckbox = MakeCheckbox(ShootSpeedDialogGraphics::kInstantCheckboxId, this);
    mInstantCheckbox->mChecked = instant;

    mYesButton->mLabel = ShootSpeedDialogGraphics::kApplyLabel;
    mNoButton->mLabel = ShootSpeedDialogGraphics::kCancelLabel;

    UpdateExclusiveState(false);
}

ShootSpeedDialog::~ShootSpeedDialog()
{
    delete mSpeedSlider;
    delete mInstantCheckbox;
}

bool ShootSpeedDialog::GetInstant() const
{
    return mInstantCheckbox != NULL && mInstantCheckbox->IsChecked();
}

void ShootSpeedDialog::UpdateExclusiveState(bool fromSlider)
{
    if (mInstantCheckbox == NULL || mSpeedSlider == NULL)
        return;

    if (fromSlider)
    {
        if (mInstantCheckbox->IsChecked())
            mInstantCheckbox->SetChecked(false, false);
        mSpeedSlider->SetDisabled(false);
        return;
    }

    mSpeedSlider->SetDisabled(mInstantCheckbox->IsChecked());
}

void ShootSpeedDialog::SetMultiplierFromSlider(double theVal)
{
    int last = ShootSpeedDialogGraphics::kMultiplierCount - 1;
    if (last < 0)
        last = 0;
    int index = (int)floor(theVal * last + 0.5);
    if (index < 0)
        index = 0;
    if (index > last)
        index = last;

    mMultiplier = ShootSpeedDialogGraphics::kMultipliers[index];

    double snappedVal = MultiplierToSlider(mMultiplier);
    if (fabs(snappedVal - theVal) > 0.0001)
        mSpeedSlider->SetValue(snappedVal);
}

void ShootSpeedDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop();
    int width = GetWidth();

    mSpeedSlider->Resize(left, top + ShootSpeedDialogGraphics::kSliderTopOffset,
                         width, ShootSpeedDialogGraphics::kSliderHeight);
    mInstantCheckbox->Resize(left, top + ShootSpeedDialogGraphics::kCheckboxTopOffset,
                             mInstantCheckbox->mWidth, mInstantCheckbox->mHeight);
}

int ShootSpeedDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + ShootSpeedDialogGraphics::kExtraPreferredHeight;
}

void ShootSpeedDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mSpeedSlider);
    theWidgetManager->AddWidget(mInstantCheckbox);
    UpdateExclusiveState(false);
}

void ShootSpeedDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mSpeedSlider);
    theWidgetManager->RemoveWidget(mInstantCheckbox);
}

void ShootSpeedDialog::SliderVal(int theId, double theVal)
{
    if (theId != ShootSpeedDialogGraphics::kSpeedSliderId)
        return;

    SetMultiplierFromSlider(theVal);
    UpdateExclusiveState(true);
    MarkDirty();
}

void ShootSpeedDialog::CheckboxChecked(int theId, bool checked)
{
    if (theId != ShootSpeedDialogGraphics::kInstantCheckboxId)
        return;

    (void)checked;
    UpdateExclusiveState(false);
    MarkDirty();
}

void ShootSpeedDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    if (GetInstant())
        g->SetColor(Color(120, 130, 90));
    else
        g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int labelY = GetTop() - mY + ShootSpeedDialogGraphics::kLabelTopOffset + FONT_DIALOG->GetAscent();
    g->DrawString(Sexy::StrFormat("Shot speed: %.1fx", mMultiplier), labelX, labelY);

    g->SetColor(mColors[COLOR_LINES]);
    DrawCheckboxText(g, ShootSpeedDialogGraphics::kInstantLabel, mInstantCheckbox);
}
