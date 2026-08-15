#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Checkbox.h>
#include <SexyAppFramework/Slider.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCheckbox.h"
#include "CircleCommon.h"
#include "ColorsBanDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace ColorsBanDialogGraphics
{
    const char *const kTitle = "COLORS BAN";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";
    const char *const kRandomLabel = "Random";

    const char *const kColorNames[MAX_BALL_COLORS] = {
        "Blue",
        "Yellow",
        "Red",
        "Green",
        "Purple",
        "White"};

    // Grid layout order (ball type indexes). Red and Blue are swapped vs type order.
    // Layout: Red|Yellow, Blue|Green, Purple|White
    const int kDisplayOrder[MAX_BALL_COLORS] = {2, 1, 0, 3, 4, 5};

    const int kFirstColorCheckboxId = 0;
    const int kRandomCheckboxId = 50;
    const int kCountSliderId = 100;

    const int kMinCount = 1;
    const int kMaxCount = 5;
    const int kCountStep = 1;
    const float kCountSpan = (float)(kMaxCount - kMinCount);

    const int kFirstCheckboxTopOffset = 10;
    const int kColorCheckboxVerticalGap = 10;
    const int kColorColumnCount = 2;
    const int kColorColumnWidth = 160;
    // Space below the last color row before Random (checkbox art + ledge need room).
    const int kAfterColorsGap = 20;
    // Space below Random checkbox before the count label.
    const int kAfterRandomGap = 18;
    // Space below the count label before the slider.
    const int kAfterLabelGap = 12;
    const int kSliderHeight = 36;
    const int kExtraPreferredHeight = 320;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ColorsBanDialog::SnapCount(float theValue)
{
    int snapped = (int)floor(theValue / ColorsBanDialogGraphics::kCountStep + 0.5f) *
                  ColorsBanDialogGraphics::kCountStep;
    if (snapped < ColorsBanDialogGraphics::kMinCount)
        snapped = ColorsBanDialogGraphics::kMinCount;
    if (snapped > ColorsBanDialogGraphics::kMaxCount)
        snapped = ColorsBanDialogGraphics::kMaxCount;
    return snapped;
}

double ColorsBanDialog::CountToSlider(int theCount)
{
    if (ColorsBanDialogGraphics::kCountSpan <= 0.0f)
        return 0.0;
    return (theCount - ColorsBanDialogGraphics::kMinCount) /
           ColorsBanDialogGraphics::kCountSpan;
}

ColorsBanDialog::ColorsBanDialog(const bool initialBanned[MAX_BALL_COLORS], bool random, int randomCount)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_ColorsBan, true,
                   ColorsBanDialogGraphics::kTitle,
                   ColorsBanDialogGraphics::kEmptyLines,
                   ColorsBanDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mRandomCount = SnapCount((float)randomCount);

    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        mColorCheckboxes[i] = MakeCheckbox(ColorsBanDialogGraphics::kFirstColorCheckboxId + i, this);
        mColorCheckboxes[i]->mChecked = initialBanned[i];
    }

    mRandomCheckbox = MakeCheckbox(ColorsBanDialogGraphics::kRandomCheckboxId, this);
    mRandomCheckbox->mChecked = random;

    mCountSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                              ColorsBanDialogGraphics::kCountSliderId, this);
    mCountSlider->SetValue(CountToSlider(mRandomCount));

    UpdateColorCheckboxesForRandom();

    mYesButton->mLabel = ColorsBanDialogGraphics::kApplyLabel;
    mNoButton->mLabel = ColorsBanDialogGraphics::kCancelLabel;
}

ColorsBanDialog::~ColorsBanDialog()
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        delete mColorCheckboxes[i];
    delete mRandomCheckbox;
    delete mCountSlider;
}

bool ColorsBanDialog::GetRandom() const
{
    return mRandomCheckbox != NULL && mRandomCheckbox->IsChecked();
}

int ColorsBanDialog::CountUncheckedColors() const
{
    int count = 0;
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        if (!mColorCheckboxes[i]->IsChecked())
            count++;
    }
    return count;
}

void ColorsBanDialog::UpdateColorCheckboxesForRandom()
{
    bool randomOn = GetRandom();
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        if (mColorCheckboxes[i] == NULL)
            continue;
        mColorCheckboxes[i]->SetDisabled(randomOn);
    }
    if (mCountSlider != NULL)
        mCountSlider->SetDisabled(!randomOn);
    MarkDirty();
}

void ColorsBanDialog::SetCountFromSlider(double theVal)
{
    float count = ColorsBanDialogGraphics::kMinCount +
                  (float)theVal * ColorsBanDialogGraphics::kCountSpan;
    mRandomCount = SnapCount(count);

    double snappedVal = CountToSlider(mRandomCount);
    if (fabs(snappedVal - theVal) > 0.0001)
        mCountSlider->SetValue(snappedVal);
}

void ColorsBanDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop() + ColorsBanDialogGraphics::kFirstCheckboxTopOffset;
    int rowH = mColorCheckboxes[0]->mHeight + ColorsBanDialogGraphics::kColorCheckboxVerticalGap;

    for (int slot = 0; slot < MAX_BALL_COLORS; slot++)
    {
        int colorIndex = ColorsBanDialogGraphics::kDisplayOrder[slot];
        int col = slot % ColorsBanDialogGraphics::kColorColumnCount;
        int row = slot / ColorsBanDialogGraphics::kColorColumnCount;
        int x = left + col * ColorsBanDialogGraphics::kColorColumnWidth;
        int y = top + row * rowH;
        mColorCheckboxes[colorIndex]->Resize(x, y, mColorCheckboxes[colorIndex]->mWidth, mColorCheckboxes[colorIndex]->mHeight);
    }

    // 3 color rows (0..2); place Random below the last row's checkbox + ledge art.
    int lastColorRowY = top + 2 * rowH;
    int randomY = lastColorRowY + mColorCheckboxes[0]->mHeight + ColorsBanDialogGraphics::kAfterColorsGap;
    mRandomCheckbox->Resize(left, randomY, mRandomCheckbox->mWidth, mRandomCheckbox->mHeight);

    int labelY = randomY + mRandomCheckbox->mHeight + ColorsBanDialogGraphics::kAfterRandomGap;
    int sliderY = labelY + FONT_DIALOG->GetHeight() + ColorsBanDialogGraphics::kAfterLabelGap;
    mCountSlider->Resize(left, sliderY, GetWidth(), ColorsBanDialogGraphics::kSliderHeight);
}

int ColorsBanDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + ColorsBanDialogGraphics::kExtraPreferredHeight;
}

void ColorsBanDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);

    for (int i = 0; i < MAX_BALL_COLORS; i++)
        theWidgetManager->AddWidget(mColorCheckboxes[i]);
    theWidgetManager->AddWidget(mRandomCheckbox);
    theWidgetManager->AddWidget(mCountSlider);
}

void ColorsBanDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);

    for (int i = 0; i < MAX_BALL_COLORS; i++)
        theWidgetManager->RemoveWidget(mColorCheckboxes[i]);
    theWidgetManager->RemoveWidget(mRandomCheckbox);
    theWidgetManager->RemoveWidget(mCountSlider);
}

void ColorsBanDialog::SliderVal(int theId, double theVal)
{
    if (theId == ColorsBanDialogGraphics::kCountSliderId)
    {
        SetCountFromSlider(theVal);
        MarkDirty();
    }
}

void ColorsBanDialog::CheckboxChecked(int theId, bool checked)
{
    if (theId == ColorsBanDialogGraphics::kRandomCheckboxId)
    {
        UpdateColorCheckboxesForRandom();
        return;
    }

    if (theId < ColorsBanDialogGraphics::kFirstColorCheckboxId ||
        theId >= ColorsBanDialogGraphics::kFirstColorCheckboxId + MAX_BALL_COLORS)
        return;

    int colorIndex = theId - ColorsBanDialogGraphics::kFirstColorCheckboxId;

    if (GetRandom())
        return;

    // Already toggled by Checkbox; do not allow banning every color.
    if (checked && CountUncheckedColors() == 0)
        mColorCheckboxes[colorIndex]->SetChecked(false, false);

    MarkDirty();
}

void ColorsBanDialog::GetBannedColors(bool outBanned[MAX_BALL_COLORS]) const
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        outBanned[i] = mColorCheckboxes[i]->IsChecked();
}

void ColorsBanDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);

    Color normalColor = mColors[COLOR_LINES];
    Color dimColor(normalColor.GetRed(), normalColor.GetGreen(), normalColor.GetBlue(), 90);
    bool randomOn = GetRandom();

    for (int slot = 0; slot < MAX_BALL_COLORS; slot++)
    {
        int colorIndex = ColorsBanDialogGraphics::kDisplayOrder[slot];
        g->SetColor(randomOn ? dimColor : normalColor);
        DrawCheckboxText(g, ColorsBanDialogGraphics::kColorNames[colorIndex], mColorCheckboxes[colorIndex]);
    }

    g->SetColor(normalColor);
    DrawCheckboxText(g, ColorsBanDialogGraphics::kRandomLabel, mRandomCheckbox);

    g->SetColor(randomOn ? normalColor : dimColor);
    int labelX = mCountSlider->mX - mX;
    int labelY = mRandomCheckbox->mY - mY + mRandomCheckbox->mHeight +
                 ColorsBanDialogGraphics::kAfterRandomGap + FONT_DIALOG->GetAscent();
    g->DrawString(Sexy::StrFormat("Banned colors: %d", mRandomCount), labelX, labelY);
}
