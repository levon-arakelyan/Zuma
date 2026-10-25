#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Slider.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCommon.h"
#include "MultiFrogDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace MultiFrogDialogGraphics
{
    const char *const kTitle = "MULTI-FROG";
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
int MultiFrogDialog::SnapCount(float theValue)
{
    int snapped = (int)floor(theValue + 0.5f);
    if (snapped < MultiFrogDialogGraphics::kMinCount)
        snapped = MultiFrogDialogGraphics::kMinCount;
    if (snapped > MultiFrogDialogGraphics::kMaxCount)
        snapped = MultiFrogDialogGraphics::kMaxCount;
    return snapped;
}

double MultiFrogDialog::CountToSlider(int theCount)
{
    return (theCount - MultiFrogDialogGraphics::kMinCount) /
           MultiFrogDialogGraphics::kSliderSpan;
}

MultiFrogDialog::MultiFrogDialog(int initialCount)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_MultiFrog, true,
                   MultiFrogDialogGraphics::kTitle,
                   MultiFrogDialogGraphics::kEmptyLines,
                   MultiFrogDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    if (initialCount < MultiFrogDialogGraphics::kMinCount ||
        initialCount > MultiFrogDialogGraphics::kMaxCount)
        initialCount = MultiFrogDialogGraphics::kDefaultCount;

    mFrogCount = SnapCount((float)initialCount);

    mCountSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                              MultiFrogDialogGraphics::kCountSliderId, this);
    mCountSlider->SetValue(CountToSlider(mFrogCount));

    mYesButton->mLabel = MultiFrogDialogGraphics::kApplyLabel;
    mNoButton->mLabel = MultiFrogDialogGraphics::kCancelLabel;
}

MultiFrogDialog::~MultiFrogDialog()
{
    delete mCountSlider;
}

void MultiFrogDialog::SetCountFromSlider(double theVal)
{
    float count = MultiFrogDialogGraphics::kMinCount +
                  (float)theVal * MultiFrogDialogGraphics::kSliderSpan;
    mFrogCount = SnapCount(count);

    double snappedVal = CountToSlider(mFrogCount);
    if (fabs(snappedVal - theVal) > 0.0001)
        mCountSlider->SetValue(snappedVal);
}

void MultiFrogDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    mCountSlider->Resize(GetLeft(), GetTop() + MultiFrogDialogGraphics::kSliderTopOffset,
                         GetWidth(), MultiFrogDialogGraphics::kSliderHeight);
}

int MultiFrogDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + MultiFrogDialogGraphics::kExtraPreferredHeight;
}

void MultiFrogDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mCountSlider);
}

void MultiFrogDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mCountSlider);
}

void MultiFrogDialog::SliderVal(int theId, double theVal)
{
    if (theId == MultiFrogDialogGraphics::kCountSliderId)
    {
        SetCountFromSlider(theVal);
        MarkDirty();
    }
}

void MultiFrogDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int labelY = GetTop() - mY + MultiFrogDialogGraphics::kLabelTopOffset + FONT_DIALOG->GetAscent();
    g->DrawString(Sexy::StrFormat("Frogs: %d", mFrogCount), labelX, labelY);
}
