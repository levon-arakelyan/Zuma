#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Checkbox.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>

#include "CircleCheckbox.h"
#include "CircleCommon.h"
#include "ColorsBanDialog.h"
#include "Res.h"

using namespace Sexy;

namespace ColorsBanDialogGraphics
{
    // Dialog chrome
    const char *const kTitle = "BANNED COLORS";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    // Ball color names (indexes match gBallColors / ball types)
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

    // Checkbox ids (passed to MakeCheckbox / CheckboxChecked)
    const int kFirstColorCheckboxId = 0; // ...through MAX_BALL_COLORS - 1

    // Layout: color checkboxes relative to dialog content area
    const int kFirstCheckboxTopOffset = 10;   // pixels below GetTop()
    const int kColorCheckboxVerticalGap = 10; // space between color rows
    const int kColorColumnCount = 2;          // two columns per kDisplayOrder
    const int kColorColumnWidth = 160;        // horizontal spacing between color columns

    // Dialog sizing
    const int kExtraPreferredHeight = 180;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ColorsBanDialog::ColorsBanDialog(const bool initialBanned[MAX_BALL_COLORS])
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_ColorsBan, true,
                   ColorsBanDialogGraphics::kTitle,
                   ColorsBanDialogGraphics::kEmptyLines,
                   ColorsBanDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        mColorCheckboxes[i] = MakeCheckbox(ColorsBanDialogGraphics::kFirstColorCheckboxId + i, this);
        mColorCheckboxes[i]->mChecked = initialBanned[i];
    }

    mYesButton->mLabel = ColorsBanDialogGraphics::kApplyLabel;
    mNoButton->mLabel = ColorsBanDialogGraphics::kCancelLabel;
}

ColorsBanDialog::~ColorsBanDialog()
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        delete mColorCheckboxes[i];
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

void ColorsBanDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop() + ColorsBanDialogGraphics::kFirstCheckboxTopOffset;

    for (int slot = 0; slot < MAX_BALL_COLORS; slot++)
    {
        int colorIndex = ColorsBanDialogGraphics::kDisplayOrder[slot];
        int col = slot % ColorsBanDialogGraphics::kColorColumnCount;
        int row = slot / ColorsBanDialogGraphics::kColorColumnCount;
        int x = left + col * ColorsBanDialogGraphics::kColorColumnWidth;
        int y = top + row * (mColorCheckboxes[0]->mHeight + ColorsBanDialogGraphics::kColorCheckboxVerticalGap);
        mColorCheckboxes[colorIndex]->Resize(x, y, mColorCheckboxes[colorIndex]->mWidth, mColorCheckboxes[colorIndex]->mHeight);
    }
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
}

void ColorsBanDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);

    for (int i = 0; i < MAX_BALL_COLORS; i++)
        theWidgetManager->RemoveWidget(mColorCheckboxes[i]);
}

void ColorsBanDialog::CheckboxChecked(int theId, bool checked)
{
    if (theId < ColorsBanDialogGraphics::kFirstColorCheckboxId ||
        theId >= ColorsBanDialogGraphics::kFirstColorCheckboxId + MAX_BALL_COLORS)
        return;

    // Already toggled by Checkbox; do not allow banning every color.
    if (checked && CountUncheckedColors() == 0)
        mColorCheckboxes[theId - ColorsBanDialogGraphics::kFirstColorCheckboxId]->SetChecked(false, false);

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
    g->SetColor(mColors[COLOR_LINES]);

    for (int slot = 0; slot < MAX_BALL_COLORS; slot++)
    {
        int colorIndex = ColorsBanDialogGraphics::kDisplayOrder[slot];
        DrawCheckboxText(g, ColorsBanDialogGraphics::kColorNames[colorIndex], mColorCheckboxes[colorIndex]);
    }
}
