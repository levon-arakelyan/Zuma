#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Checkbox.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>

#include "CircleCheckbox.h"
#include "CircleCommon.h"
#include "UnpoweredDialog.h"
#include "Res.h"

using namespace Sexy;

namespace UnpoweredDialogGraphics
{
    // Dialog chrome
    const char *const kTitle = "DISABLED POWER-UPS";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    // Power-up names (indexes match PowerType enum)
    const char *const kPowerNames[PowerType_Max] = {
        "Bomb",
        "Slowdown",
        "Accuracy",
        "Backwards"};

    // Checkbox ids
    const int kFirstPowerCheckboxId = 0;

    // Layout
    const int kFirstCheckboxTopOffset = 10;
    const int kPowerCheckboxVerticalGap = 12;
    const int kPowerColumnCount = 2;
    const int kPowerColumnWidth = 160;

    // Dialog sizing
    const int kExtraPreferredHeight = 120;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
UnpoweredDialog::UnpoweredDialog(const bool initialDisabled[PowerType_Max])
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_Unpowered, true,
                   UnpoweredDialogGraphics::kTitle,
                   UnpoweredDialogGraphics::kEmptyLines,
                   UnpoweredDialogGraphics::kEmptyFooter,
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    for (int i = 0; i < PowerType_Max; i++)
    {
        mPowerCheckboxes[i] = MakeCheckbox(UnpoweredDialogGraphics::kFirstPowerCheckboxId + i, this);
        mPowerCheckboxes[i]->mChecked = initialDisabled[i];
    }

    mYesButton->mLabel = UnpoweredDialogGraphics::kApplyLabel;
    mNoButton->mLabel = UnpoweredDialogGraphics::kCancelLabel;
}

UnpoweredDialog::~UnpoweredDialog()
{
    for (int i = 0; i < PowerType_Max; i++)
        delete mPowerCheckboxes[i];
}

void UnpoweredDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop() + UnpoweredDialogGraphics::kFirstCheckboxTopOffset;

    for (int i = 0; i < PowerType_Max; i++)
    {
        int col = i % UnpoweredDialogGraphics::kPowerColumnCount;
        int row = i / UnpoweredDialogGraphics::kPowerColumnCount;
        int x = left + col * UnpoweredDialogGraphics::kPowerColumnWidth;
        int y = top + row * (mPowerCheckboxes[0]->mHeight + UnpoweredDialogGraphics::kPowerCheckboxVerticalGap);
        mPowerCheckboxes[i]->Resize(x, y, mPowerCheckboxes[i]->mWidth, mPowerCheckboxes[i]->mHeight);
    }
}

int UnpoweredDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + UnpoweredDialogGraphics::kExtraPreferredHeight;
}

void UnpoweredDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);

    for (int i = 0; i < PowerType_Max; i++)
        theWidgetManager->AddWidget(mPowerCheckboxes[i]);
}

void UnpoweredDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);

    for (int i = 0; i < PowerType_Max; i++)
        theWidgetManager->RemoveWidget(mPowerCheckboxes[i]);
}

void UnpoweredDialog::CheckboxChecked(int theId, bool checked)
{
    MarkDirty();
}

void UnpoweredDialog::GetDisabledPowerUps(bool outDisabled[PowerType_Max]) const
{
    for (int i = 0; i < PowerType_Max; i++)
        outDisabled[i] = mPowerCheckboxes[i]->IsChecked();
}

void UnpoweredDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);
    g->SetColor(mColors[COLOR_LINES]);

    for (int i = 0; i < PowerType_Max; i++)
        DrawCheckboxText(g, UnpoweredDialogGraphics::kPowerNames[i], mPowerCheckboxes[i]);
}
