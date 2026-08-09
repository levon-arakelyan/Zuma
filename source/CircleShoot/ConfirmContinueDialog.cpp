#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/DialogButton.h>

#include "CircleButton.h"
#include "CircleCommon.h"
#include "CircleShootApp.h"
#include "ConfirmContinueDialog.h"
#include "Res.h"

using namespace Sexy;

namespace ConfirmContinueDialogGraphics
{
    const char *const kTitle = "CONTINUE?";
    const char *const kContinueLabel = "Continue";
    const char *const kNewGameLabel = "New Game";
    const char *const kCancelLabel = "Cancel";
    const int kCancelButtonId = 0;
    const int kButtonRowGap = 8;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ConfirmContinueDialog::ConfirmContinueDialog(const std::string &theText)
    : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                   DialogType_ConfirmContinue, true,
                   ConfirmContinueDialogGraphics::kTitle, theText, "",
                   Dialog::BUTTONS_OK_CANCEL, false)
{
    mYesButton->mLabel = ConfirmContinueDialogGraphics::kContinueLabel;
    mNoButton->mLabel = ConfirmContinueDialogGraphics::kNewGameLabel;
    mCancelButton = MakeButton(ConfirmContinueDialogGraphics::kCancelButtonId, this,
                               ConfirmContinueDialogGraphics::kCancelLabel,
                               CircleButton::CB_ClickSound, NULL, 3);
}

ConfirmContinueDialog::~ConfirmContinueDialog()
{
    delete mCancelButton;
}

int ConfirmContinueDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + mButtonHeight +
           ConfirmContinueDialogGraphics::kButtonRowGap;
}

void ConfirmContinueDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = mX + mBackgroundInsets.mLeft + mContentInsets.mLeft;
    int width = mWidth - mContentInsets.mLeft - mContentInsets.mRight -
                mBackgroundInsets.mLeft - mBackgroundInsets.mRight;
    int gap = ConfirmContinueDialogGraphics::kButtonRowGap;
    int btnHeight = mButtonHeight;

    // Lift Continue / New Game one row; Cancel sits full-width on the bottom.
    if (mYesButton != NULL)
        mYesButton->mY -= (btnHeight + gap);
    if (mNoButton != NULL)
        mNoButton->mY -= (btnHeight + gap);

    int cancelY = mY + mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom - btnHeight;
    mCancelButton->Resize(left, cancelY, width, btnHeight);
}

void ConfirmContinueDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mCancelButton);
}

void ConfirmContinueDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);
    theWidgetManager->RemoveWidget(mCancelButton);
}

void ConfirmContinueDialog::ButtonDepress(int theId)
{
    CircleDialog::ButtonDepress(theId);

    if (theId == ConfirmContinueDialogGraphics::kCancelButtonId)
        GetCircleShootApp()->FinishConfirmContinueDialogCancel();
}
