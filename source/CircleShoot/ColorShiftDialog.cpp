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
#include "CircleShootApp.h"
#include "ColorShiftDialog.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

namespace ColorShiftDialogGraphics
{
    const char *const kTitle = "COLOR SHIFT";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";
    const char *const kRandomLabel = "Shift randomly";

    const char *const kColorNames[MAX_BALL_COLORS] = {
        "Blue", "Yellow", "Red", "Green", "Purple", "White"};

    const float kMinSec = 1.0f;
    const float kMaxSec = 10.0f;
    const float kSecStep = 0.25f;
    const float kSliderSpan = kMaxSec - kMinSec;

    const int kSliderTopOffset = 40;
    const int kSliderHeight = 36;
    const int kIntervalLabelTopOffset = 15;
    const int kRandomTopOffset = 0;
    const int kColorTopOffset = 90;
    const int kColorsPerRow = 2;
    const int kColorRowGap = 10;
    const int kExtraPreferredHeight = 270;

    const int kSecSliderId = 0;
    const int kRandomCheckboxId = 1;
    const int kColorCheckboxIdBase = 10;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
float ColorShiftDialog::SnapSec(float theValue)
{
    float snapped = (float)floor(theValue / ColorShiftDialogGraphics::kSecStep + 0.5f) *
                    ColorShiftDialogGraphics::kSecStep;
    if (snapped < ColorShiftDialogGraphics::kMinSec)
        snapped = ColorShiftDialogGraphics::kMinSec;
    if (snapped > ColorShiftDialogGraphics::kMaxSec)
        snapped = ColorShiftDialogGraphics::kMaxSec;
    return snapped;
}

double ColorShiftDialog::SecToSlider(float theSec)
{
    return (theSec - ColorShiftDialogGraphics::kMinSec) /
           ColorShiftDialogGraphics::kSliderSpan;
}

ColorShiftDialog::ColorShiftDialog(float initialSec,
                                   const int initialMap[MAX_BALL_COLORS],
                                   const bool initialEnabled[MAX_BALL_COLORS],
                                   bool randomMap)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_ColorShift, true,
                   ColorShiftDialogGraphics::kTitle,
                   ColorShiftDialogGraphics::kEmptyLines,
                   ColorShiftDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mIntervalSec = SnapSec(initialSec);
    mPendingTargetSrc = -1;

    mSecSlider = new Slider(Sexy::IMAGE_SLIDER_TRACK, Sexy::IMAGE_SLIDER_THUMB,
                            ColorShiftDialogGraphics::kSecSliderId, this);
    mSecSlider->SetValue(SecToSlider(mIntervalSec));

    mRandomCheckbox = MakeCheckbox(ColorShiftDialogGraphics::kRandomCheckboxId, this);
    mRandomCheckbox->mChecked = randomMap;

    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        int dest = (initialMap != NULL) ? initialMap[i] : ((i + 1) % MAX_BALL_COLORS);
        if (dest < 0 || dest >= MAX_BALL_COLORS || dest == i)
            dest = (i + 1) % MAX_BALL_COLORS;
        mMap[i] = dest;
        mEnabled[i] = (initialEnabled != NULL) ? initialEnabled[i] : true;

        mColorCheckboxes[i] = MakeCheckbox(ColorShiftDialogGraphics::kColorCheckboxIdBase + i, this);
        mColorCheckboxes[i]->mChecked = mEnabled[i];
    }

    UpdateColorCheckboxesForRandom();

    mYesButton->mLabel = ColorShiftDialogGraphics::kApplyLabel;
    mNoButton->mLabel = ColorShiftDialogGraphics::kCancelLabel;
}

ColorShiftDialog::~ColorShiftDialog()
{
    delete mSecSlider;
    delete mRandomCheckbox;
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        delete mColorCheckboxes[i];
}

bool ColorShiftDialog::GetRandomMap() const
{
    return mRandomCheckbox != NULL && mRandomCheckbox->IsChecked();
}

void ColorShiftDialog::GetColorMap(int outMap[MAX_BALL_COLORS]) const
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        outMap[i] = mMap[i];
}

void ColorShiftDialog::GetEnabledColors(bool outEnabled[MAX_BALL_COLORS]) const
{
    // Random mode remaps every color in gameplay.
    if (GetRandomMap())
    {
        for (int i = 0; i < MAX_BALL_COLORS; i++)
            outEnabled[i] = true;
        return;
    }

    for (int i = 0; i < MAX_BALL_COLORS; i++)
        outEnabled[i] = mEnabled[i];
}

void ColorShiftDialog::UpdateColorCheckboxesForRandom()
{
    bool randomOn = GetRandomMap();
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        if (mColorCheckboxes[i] == NULL)
            continue;
        mColorCheckboxes[i]->SetDisabled(randomOn);
    }
    MarkDirty();
}

void ColorShiftDialog::SetSecFromSlider(double theVal)
{
    float sec = ColorShiftDialogGraphics::kMinSec +
                (float)theVal * ColorShiftDialogGraphics::kSliderSpan;
    mIntervalSec = SnapSec(sec);

    double snappedVal = SecToSlider(mIntervalSec);
    if (fabs(snappedVal - theVal) > 0.0001)
        mSecSlider->SetValue(snappedVal);
}

void ColorShiftDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop();
    int width = GetWidth();

    mSecSlider->Resize(left, top + ColorShiftDialogGraphics::kSliderTopOffset,
                       width, ColorShiftDialogGraphics::kSliderHeight);

    // Same row as "Shift every N seconds": random sits on the right (Cancel column).
    int randomX = left;
    if (mNoButton != NULL)
        randomX = mNoButton->mX;
    mRandomCheckbox->Resize(randomX, top + ColorShiftDialogGraphics::kRandomTopOffset,
                            mRandomCheckbox->mWidth, mRandomCheckbox->mHeight);

    int colorTop = top + ColorShiftDialogGraphics::kColorTopOffset;
    int rowH = mColorCheckboxes[0]->mHeight + ColorShiftDialogGraphics::kColorRowGap;
    int rightColX = left;
    if (mNoButton != NULL)
        rightColX = mNoButton->mX;

    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        int col = i % ColorShiftDialogGraphics::kColorsPerRow;
        int row = i / ColorShiftDialogGraphics::kColorsPerRow;
        int x = (col == 0) ? left : rightColX;
        int y = colorTop + row * rowH;
        mColorCheckboxes[i]->Resize(x, y, mColorCheckboxes[i]->mWidth, mColorCheckboxes[i]->mHeight);
    }
}

int ColorShiftDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) +
           ColorShiftDialogGraphics::kExtraPreferredHeight;
}

void ColorShiftDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mSecSlider);
    theWidgetManager->AddWidget(mRandomCheckbox);
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        theWidgetManager->AddWidget(mColorCheckboxes[i]);
}

void ColorShiftDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mSecSlider);
    theWidgetManager->RemoveWidget(mRandomCheckbox);
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        theWidgetManager->RemoveWidget(mColorCheckboxes[i]);
}

void ColorShiftDialog::SliderVal(int theId, double theVal)
{
    if (theId == ColorShiftDialogGraphics::kSecSliderId)
    {
        SetSecFromSlider(theVal);
        MarkDirty();
    }
}

void ColorShiftDialog::CheckboxChecked(int theId, bool checked)
{
    if (theId == ColorShiftDialogGraphics::kRandomCheckboxId)
    {
        if (checked)
            GetCircleShootApp()->KillDialog(DialogType_ColorShiftTarget);
        UpdateColorCheckboxesForRandom();
        return;
    }

    int src = theId - ColorShiftDialogGraphics::kColorCheckboxIdBase;
    if (src < 0 || src >= MAX_BALL_COLORS)
        return;

    // Ignore color toggles while random mode disables them.
    if (GetRandomMap())
    {
        if (mColorCheckboxes[src] != NULL)
            mColorCheckboxes[src]->SetChecked(mEnabled[src], false);
        return;
    }

    if (checked)
    {
        mEnabled[src] = true;
        mPendingTargetSrc = src;
        GetCircleShootApp()->DoColorShiftTargetDialog(src, mMap[src]);
    }
    else
    {
        mEnabled[src] = false;
        MarkDirty();
    }
}

void ColorShiftDialog::OnTargetPicked(int theSrcColor, int theDestColor, bool apply)
{
    if (theSrcColor < 0 || theSrcColor >= MAX_BALL_COLORS)
        return;

    mPendingTargetSrc = -1;

    if (apply)
    {
        if (theDestColor >= 0 && theDestColor < MAX_BALL_COLORS && theDestColor != theSrcColor)
            mMap[theSrcColor] = theDestColor;
        mEnabled[theSrcColor] = true;
        if (mColorCheckboxes[theSrcColor] != NULL)
            mColorCheckboxes[theSrcColor]->SetChecked(true, false);
    }
    else
    {
        // Cancel: leave disabled if user was enabling for the first time this session
        // without a prior applied destination pick — always uncheck on cancel of the open.
        mEnabled[theSrcColor] = false;
        if (mColorCheckboxes[theSrcColor] != NULL)
            mColorCheckboxes[theSrcColor]->SetChecked(false, false);
    }

    MarkDirty();
}

void ColorShiftDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    int labelX = GetLeft() - mX;
    int labelY = GetTop() - mY + ColorShiftDialogGraphics::kIntervalLabelTopOffset + FONT_DIALOG->GetAscent();
    std::string intervalText;
    int hundredths = (int)floor(mIntervalSec * 100.0f + 0.5f);
    if ((hundredths % 100) == 0)
        intervalText = Sexy::StrFormat("Shift every %d seconds", hundredths / 100);
    else if ((hundredths % 50) == 0)
        intervalText = Sexy::StrFormat("Shift every %.1f seconds", (float)hundredths / 100.0f);
    else
        intervalText = Sexy::StrFormat("Shift every %.2f seconds", (float)hundredths / 100.0f);
    g->DrawString(intervalText, labelX, labelY);

    DrawCheckboxText(g, ColorShiftDialogGraphics::kRandomLabel, mRandomCheckbox);

    bool randomOn = GetRandomMap();
    Color normalColor = mColors[COLOR_LINES];
    Color dimColor(normalColor.GetRed(), normalColor.GetGreen(), normalColor.GetBlue(), 90);

    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        if (mColorCheckboxes[i] == NULL)
            continue;

        std::string label = ColorShiftDialogGraphics::kColorNames[i];
        if (mEnabled[i])
        {
            label = Sexy::StrFormat("%s -> %s",
                                    ColorShiftDialogGraphics::kColorNames[i],
                                    ColorShiftDialogGraphics::kColorNames[mMap[i]]);
        }

        g->SetColor(randomOn ? dimColor : normalColor);
        DrawCheckboxText(g, label, mColorCheckboxes[i]);
    }
}
