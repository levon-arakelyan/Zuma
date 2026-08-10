#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Checkbox.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/Common.h>

#include "CircleCheckbox.h"
#include "CircleCommon.h"
#include "ColorShiftTargetDialog.h"
#include "Res.h"

using namespace Sexy;

namespace ColorShiftTargetDialogGraphics
{
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    const char *const kColorNames[MAX_BALL_COLORS] = {
        "Blue", "Yellow", "Red", "Green", "Purple", "White"};

    const int kFirstCheckboxId = 0;
    const int kFirstCheckboxTopOffset = 10;
    const int kCheckboxVerticalGap = 8;
    const int kColumnCount = 2;
    const int kColumnWidth = 160;
    const int kExtraPreferredHeight = 200;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ColorShiftTargetDialog::ColorShiftTargetDialog(int theSrcColor, int initialDest)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_ColorShiftTarget, true,
                   Sexy::StrFormat("%s BECOMES", ColorShiftTargetDialogGraphics::kColorNames[
                       (theSrcColor >= 0 && theSrcColor < MAX_BALL_COLORS) ? theSrcColor : 0]),
                   ColorShiftTargetDialogGraphics::kEmptyLines,
                   ColorShiftTargetDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mSrcColor = theSrcColor;
    if (mSrcColor < 0 || mSrcColor >= MAX_BALL_COLORS)
        mSrcColor = 0;

    mSelectedDest = initialDest;
    if (mSelectedDest < 0 || mSelectedDest >= MAX_BALL_COLORS || mSelectedDest == mSrcColor)
        mSelectedDest = (mSrcColor + 1) % MAX_BALL_COLORS;

    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        mDestCheckboxes[i] = NULL;
        if (i == mSrcColor)
            continue;

        mDestCheckboxes[i] = MakeCheckbox(ColorShiftTargetDialogGraphics::kFirstCheckboxId + i, this);
        mDestCheckboxes[i]->mChecked = (i == mSelectedDest);
    }

    mYesButton->mLabel = ColorShiftTargetDialogGraphics::kApplyLabel;
    mNoButton->mLabel = ColorShiftTargetDialogGraphics::kCancelLabel;
}

ColorShiftTargetDialog::~ColorShiftTargetDialog()
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        delete mDestCheckboxes[i];
}

void ColorShiftTargetDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop() + ColorShiftTargetDialogGraphics::kFirstCheckboxTopOffset;
    int rowH = 28;
    if (mDestCheckboxes[(mSrcColor + 1) % MAX_BALL_COLORS] != NULL)
        rowH = mDestCheckboxes[(mSrcColor + 1) % MAX_BALL_COLORS]->mHeight +
               ColorShiftTargetDialogGraphics::kCheckboxVerticalGap;

    int slot = 0;
    for (int color = 0; color < MAX_BALL_COLORS; color++)
    {
        if (mDestCheckboxes[color] == NULL)
            continue;

        int col = slot % ColorShiftTargetDialogGraphics::kColumnCount;
        int row = slot / ColorShiftTargetDialogGraphics::kColumnCount;
        int x = left + col * ColorShiftTargetDialogGraphics::kColumnWidth;
        int y = top + row * rowH;
        mDestCheckboxes[color]->Resize(x, y,
                                       mDestCheckboxes[color]->mWidth,
                                       mDestCheckboxes[color]->mHeight);
        slot++;
    }
}

int ColorShiftTargetDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) +
           ColorShiftTargetDialogGraphics::kExtraPreferredHeight;
}

void ColorShiftTargetDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        if (mDestCheckboxes[i] != NULL)
            theWidgetManager->AddWidget(mDestCheckboxes[i]);
    }
}

void ColorShiftTargetDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        if (mDestCheckboxes[i] != NULL)
            theWidgetManager->RemoveWidget(mDestCheckboxes[i]);
    }
}

void ColorShiftTargetDialog::CheckboxChecked(int theId, bool checked)
{
    int color = theId - ColorShiftTargetDialogGraphics::kFirstCheckboxId;
    if (color < 0 || color >= MAX_BALL_COLORS || color == mSrcColor)
        return;

    if (checked)
    {
        mSelectedDest = color;
        for (int i = 0; i < MAX_BALL_COLORS; i++)
        {
            if (mDestCheckboxes[i] != NULL && i != color)
                mDestCheckboxes[i]->SetChecked(false, false);
        }
    }
    else if (mSelectedDest == color)
    {
        // Keep one destination selected.
        mDestCheckboxes[color]->SetChecked(true, false);
    }

    MarkDirty();
}

void ColorShiftTargetDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        if (mDestCheckboxes[i] == NULL)
            continue;
        DrawCheckboxText(g, ColorShiftTargetDialogGraphics::kColorNames[i], mDestCheckboxes[i]);
    }
}
