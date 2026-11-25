#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Slider.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCommon.h"
#include "MultiGameDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace MultiGameDialogGraphics
{
    const char *const kTitle = "MULTI GAME";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    const int kMinCount = 1;
    const int kMaxCount = 4;
    const int kDefaultCount = 2;
    const float kSliderSpan = (float)(kMaxCount - kMinCount);

    const int kSliderTopOffset = 40;
    const int kSliderHeight = 36;
    const int kLabelTopOffset = 10;
    const int kExtraPreferredHeight = 90;

    const int kCountSliderId = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int MultiGameDialog::SnapCount(float theValue)
{
    int snapped = (int)floor(theValue + 0.5f);
    if (snapped < MultiGameDialogGraphics::kMinCount)
        snapped = MultiGameDialogGraphics::kMinCount;
    if (snapped > MultiGameDialogGraphics::kMaxCount)
        snapped = MultiGameDialogGraphics::kMaxCount;
    return snapped;
}

double MultiGameDialog::CountToSlider(int theCount)
{
    return (theCount - MultiGameDialogGraphics::kMinCount) /
           MultiGameDialogGraphics::kSliderSpan;
}

MultiGameDialog::MultiGameDialog(int initialCount)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_MultiGame, true,
                   MultiGameDialogGraphics::kTitle,
                   MultiGameDialogGraphics::kEmptyLines,
                   MultiGameDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    if (initialCount < MultiGameDialogGraphics::kMinCount ||
        initialCount > MultiGameDialogGraphics::kMaxCount)
        initialCount = MultiGameDialogGraphics::kDefaultCount;

    mGameCount = SnapCount((float)initialCount);

    mCountSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                              MultiGameDialogGraphics::kCountSliderId, this);
    mCountSlider->SetValue(CountToSlider(mGameCount));

    mYesButton->mLabel = MultiGameDialogGraphics::kApplyLabel;
    mNoButton->mLabel = MultiGameDialogGraphics::kCancelLabel;
}

MultiGameDialog::~MultiGameDialog()
{
    delete mCountSlider;
}

void MultiGameDialog::SetCountFromSlider(double theVal)
{
    float count = MultiGameDialogGraphics::kMinCount +
                  (float)theVal * MultiGameDialogGraphics::kSliderSpan;
    mGameCount = SnapCount(count);

    double snappedVal = CountToSlider(mGameCount);
    if (fabs(snappedVal - theVal) > 0.0001)
        mCountSlider->SetValue(snappedVal);
}

void MultiGameDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    mCountSlider->Resize(GetLeft(), GetTop() + MultiGameDialogGraphics::kSliderTopOffset,
                         GetWidth(), MultiGameDialogGraphics::kSliderHeight);
}

int MultiGameDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + MultiGameDialogGraphics::kExtraPreferredHeight;
}

void MultiGameDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mCountSlider);
}

void MultiGameDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mCountSlider);
}

void MultiGameDialog::SliderVal(int theId, double theVal)
{
    if (theId == MultiGameDialogGraphics::kCountSliderId)
    {
        SetCountFromSlider(theVal);
        MarkDirty();
    }
}

void MultiGameDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int labelY = GetTop() - mY + MultiGameDialogGraphics::kLabelTopOffset + FONT_DIALOG->GetAscent();
    g->DrawString(Sexy::StrFormat("Instances: %d", mGameCount), labelX, labelY);
}
