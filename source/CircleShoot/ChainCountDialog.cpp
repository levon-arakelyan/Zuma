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
#include "ChainCountDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace ChainCountDialogGraphics
{
    const char *const kTitle = "CHAIN COUNT";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";
    const char *const kDisableBonusLabel = "Disable chain bonus";

    const int kMinThreshold = 1;
    const int kMaxThreshold = 20;
    const int kDefaultThreshold = 5;
    const int kThresholdStep = 1;
    const float kSliderSpan = (float)(kMaxThreshold - kMinThreshold);

    const int kSliderTopOffset = 40;
    const int kSliderHeight = 36;
    const int kLabelTopOffset = 10;
    const int kCheckboxTopOffset = 90;
    const int kExtraPreferredHeight = 140;

    const int kThresholdSliderId = 0;
    const int kDisableBonusCheckboxId = 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ChainCountDialog::SnapThreshold(float theValue)
{
    int snapped = (int)floor(theValue / ChainCountDialogGraphics::kThresholdStep + 0.5f) *
                  ChainCountDialogGraphics::kThresholdStep;
    if (snapped < ChainCountDialogGraphics::kMinThreshold)
        snapped = ChainCountDialogGraphics::kMinThreshold;
    if (snapped > ChainCountDialogGraphics::kMaxThreshold)
        snapped = ChainCountDialogGraphics::kMaxThreshold;
    return snapped;
}

double ChainCountDialog::ThresholdToSlider(int theThreshold)
{
    return (theThreshold - ChainCountDialogGraphics::kMinThreshold) /
           ChainCountDialogGraphics::kSliderSpan;
}

ChainCountDialog::ChainCountDialog(int initialThreshold, bool disableBonus)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_ChainCount, true,
                   ChainCountDialogGraphics::kTitle,
                   ChainCountDialogGraphics::kEmptyLines,
                   ChainCountDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    if (initialThreshold < ChainCountDialogGraphics::kMinThreshold ||
        initialThreshold > ChainCountDialogGraphics::kMaxThreshold)
        initialThreshold = ChainCountDialogGraphics::kDefaultThreshold;

    mThreshold = SnapThreshold((float)initialThreshold);

    mThresholdSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                                  ChainCountDialogGraphics::kThresholdSliderId, this);
    mThresholdSlider->SetValue(ThresholdToSlider(mThreshold));

    // NULL listener: toggle is local UI only; Apply reads IsChecked().
    mDisableBonusCheckbox = MakeCheckbox(ChainCountDialogGraphics::kDisableBonusCheckboxId, NULL);
    mDisableBonusCheckbox->mChecked = disableBonus;

    mYesButton->mLabel = ChainCountDialogGraphics::kApplyLabel;
    mNoButton->mLabel = ChainCountDialogGraphics::kCancelLabel;
}

ChainCountDialog::~ChainCountDialog()
{
    delete mThresholdSlider;
    delete mDisableBonusCheckbox;
}

bool ChainCountDialog::GetDisableBonus() const
{
    return mDisableBonusCheckbox != NULL && mDisableBonusCheckbox->IsChecked();
}

void ChainCountDialog::SetThresholdFromSlider(double theVal)
{
    float threshold = ChainCountDialogGraphics::kMinThreshold +
                      (float)theVal * ChainCountDialogGraphics::kSliderSpan;
    mThreshold = SnapThreshold(threshold);

    double snappedVal = ThresholdToSlider(mThreshold);
    if (fabs(snappedVal - theVal) > 0.0001)
        mThresholdSlider->SetValue(snappedVal);
}

void ChainCountDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop();
    int width = GetWidth();

    mThresholdSlider->Resize(left, top + ChainCountDialogGraphics::kSliderTopOffset,
                             width, ChainCountDialogGraphics::kSliderHeight);
    mDisableBonusCheckbox->Resize(left, top + ChainCountDialogGraphics::kCheckboxTopOffset,
                                  mDisableBonusCheckbox->mWidth, mDisableBonusCheckbox->mHeight);
}

int ChainCountDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + ChainCountDialogGraphics::kExtraPreferredHeight;
}

void ChainCountDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mThresholdSlider);
    theWidgetManager->AddWidget(mDisableBonusCheckbox);
}

void ChainCountDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mThresholdSlider);
    theWidgetManager->RemoveWidget(mDisableBonusCheckbox);
}

void ChainCountDialog::SliderVal(int theId, double theVal)
{
    if (theId == ChainCountDialogGraphics::kThresholdSliderId)
    {
        SetThresholdFromSlider(theVal);
        MarkDirty();
    }
}

void ChainCountDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int labelY = GetTop() - mY + ChainCountDialogGraphics::kLabelTopOffset + FONT_DIALOG->GetAscent();
    g->DrawString(Sexy::StrFormat("Chain bonus after: %d hit%s", mThreshold,
                                  mThreshold == 1 ? "" : "s"),
                  labelX, labelY);

    DrawCheckboxText(g, ChainCountDialogGraphics::kDisableBonusLabel, mDisableBonusCheckbox);
}
