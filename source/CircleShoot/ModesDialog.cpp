#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Checkbox.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/ButtonWidget.h>
#include <SexyAppFramework/SexyAppBase.h>

#include "CircleShootApp.h"
#include "CircleCheckbox.h"
#include "CircleCommon.h"
#include "ModesDialog.h"
#include "Res.h"

using namespace Sexy;

namespace ModesDialogGraphics
{
    // Dialog chrome
    const char *const kTitle = "MODES";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    // Mode option labels
    const char *const kColorsBanLabel = "Colors ban";
    const char *const kUnpoweredLabel = "Unpowered";
    const char *const kNoSwapLabel = "No swap";
    const char *const kSonicLabel = "Sonic";

    // Descriptions shown in the cursor tooltip while hovering a mode name
    const char *const kColorsBanDescription =
        "Choose which colors you cannot destroy directly. Matching three or more of a banned color causes you to lose.";
    const char *const kUnpoweredDescription =
        "Choose which power-ups are disabled and will not appear on the chain.";
    const char *const kNoSwapDescription =
        "The frog cannot swap colors with right-click, and the next ball color is hidden.";
    const char *const kSonicDescription =
        "Increase the ball chain speed. When enabled, choose a multiplier from 1x to 10x (in 0.5 steps).";

    // Checkbox ids
    const int kColorsBanCheckboxId = 0;
    const int kUnpoweredCheckboxId = 1;
    const int kNoSwapCheckboxId = 2;
    const int kSonicCheckboxId = 3;

    // Invisible hover hit-area ids (over mode labels)
    const int kColorsBanHitId = 10;
    const int kUnpoweredHitId = 11;
    const int kNoSwapHitId = 12;
    const int kSonicHitId = 13;

    // Layout: 2x2 grid  Colors ban | Unpowered / No swap | Sonic
    const int kFirstRowTopOffset = 14;
    const int kRowVerticalGap = 28;
    const int kColumnWidth = 180;
    const int kLabelHitPadX = 4;
    const int kLabelHitHeight = 32;

    // Tooltip
    const int kTooltipWidth = 230;
    const int kTooltipPadX = 8;
    const int kTooltipPadY = 6;
    const int kTooltipCursorOffsetX = 14;
    const int kTooltipCursorOffsetY = 18;
    const int kTooltipLineSpacing = -1;

    // Dialog sizing
    const int kExtraPreferredHeight = 160;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ModeHelpTooltip::ModeHelpTooltip()
{
    mMouseVisible = false;
    mClip = false;
    mHasTransparencies = true;
    mHasAlpha = true;
    SetVisible(false);
}

void ModeHelpTooltip::Draw(Graphics *g)
{
    g->SetColor(Color(16, 36, 16));
    g->FillRect(0, 0, mWidth, mHeight);

    g->SetColor(Color(0xD5E520));
    g->DrawRect(0, 0, mWidth - 1, mHeight - 1);

    g->SetFont(FONT_DIALOG);
    g->SetColor(Color(0xD5E520));
    WriteWordWrapped(g,
                     Rect(ModesDialogGraphics::kTooltipPadX,
                          ModesDialogGraphics::kTooltipPadY,
                          mWidth - 2 * ModesDialogGraphics::kTooltipPadX,
                          mHeight - 2 * ModesDialogGraphics::kTooltipPadY),
                     mText,
                     ModesDialogGraphics::kTooltipLineSpacing,
                     -1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ButtonWidget *ModesDialog::CreateModeHitArea(int theId)
{
    ButtonWidget *hit = new ButtonWidget(theId, this);
    hit->mBtnNoDraw = true;
    hit->mDoFinger = true;
    return hit;
}

ModesDialog::ModesDialog() : CircleDialog(Sexy::IMAGE_DIALOG_BACK, Sexy::IMAGE_DIALOG_BUTTON,
                                          DialogType_Modes, true,
                                          ModesDialogGraphics::kTitle,
                                          ModesDialogGraphics::kEmptyLines,
                                          ModesDialogGraphics::kEmptyFooter,
                                          Dialog::BUTTONS_OK_CANCEL, false)
{
    mHoveredModeId = -1;
    mTooltip = new ModeHelpTooltip();

    mColorsBanCheckbox = MakeCheckbox(ModesDialogGraphics::kColorsBanCheckboxId, this);
    mUnpoweredCheckbox = MakeCheckbox(ModesDialogGraphics::kUnpoweredCheckboxId, this);
    mNoSwapCheckbox = MakeCheckbox(ModesDialogGraphics::kNoSwapCheckboxId, this);
    mSonicCheckbox = MakeCheckbox(ModesDialogGraphics::kSonicCheckboxId, this);

    mColorsBanHit = CreateModeHitArea(ModesDialogGraphics::kColorsBanHitId);
    mUnpoweredHit = CreateModeHitArea(ModesDialogGraphics::kUnpoweredHitId);
    mNoSwapHit = CreateModeHitArea(ModesDialogGraphics::kNoSwapHitId);
    mSonicHit = CreateModeHitArea(ModesDialogGraphics::kSonicHitId);

    CircleShootApp *app = GetCircleShootApp();
    mColorsBanCheckbox->mChecked = app->mColorsBanMode;
    mUnpoweredCheckbox->mChecked = app->mUnpoweredMode;
    mNoSwapCheckbox->mChecked = app->mNoSwapMode;
    mSonicCheckbox->mChecked = app->mSonicMode;

    for (int i = 0; i < MAX_BALL_COLORS; i++)
        mPendingBannedColors[i] = app->mBannedColors[i];
    for (int i = 0; i < PowerType_Max; i++)
        mPendingDisabledPowerUps[i] = app->mDisabledPowerUps[i];
    mPendingChainSpeedMultiplier = app->mChainSpeedMultiplier;

    mYesButton->mLabel = ModesDialogGraphics::kApplyLabel;
    mNoButton->mLabel = ModesDialogGraphics::kCancelLabel;
}

ModesDialog::~ModesDialog()
{
    // PrepareClose() should already have detached & deleted these; keep guards for safety.
    delete mColorsBanCheckbox;
    delete mUnpoweredCheckbox;
    delete mNoSwapCheckbox;
    delete mSonicCheckbox;
    delete mColorsBanHit;
    delete mUnpoweredHit;
    delete mNoSwapHit;
    delete mSonicHit;
    delete mTooltip;
}

void ModesDialog::PrepareClose()
{
    HideTooltip();

    WidgetManager *wm = mWidgetManager;
    if (wm != NULL)
    {
        if (mColorsBanCheckbox != NULL && mColorsBanCheckbox->mWidgetManager != NULL)
            wm->RemoveWidget(mColorsBanCheckbox);
        if (mUnpoweredCheckbox != NULL && mUnpoweredCheckbox->mWidgetManager != NULL)
            wm->RemoveWidget(mUnpoweredCheckbox);
        if (mNoSwapCheckbox != NULL && mNoSwapCheckbox->mWidgetManager != NULL)
            wm->RemoveWidget(mNoSwapCheckbox);
        if (mSonicCheckbox != NULL && mSonicCheckbox->mWidgetManager != NULL)
            wm->RemoveWidget(mSonicCheckbox);
        if (mColorsBanHit != NULL && mColorsBanHit->mWidgetManager != NULL)
            wm->RemoveWidget(mColorsBanHit);
        if (mUnpoweredHit != NULL && mUnpoweredHit->mWidgetManager != NULL)
            wm->RemoveWidget(mUnpoweredHit);
        if (mNoSwapHit != NULL && mNoSwapHit->mWidgetManager != NULL)
            wm->RemoveWidget(mNoSwapHit);
        if (mSonicHit != NULL && mSonicHit->mWidgetManager != NULL)
            wm->RemoveWidget(mSonicHit);
        if (mTooltip != NULL && mTooltip->mWidgetManager != NULL)
            wm->RemoveWidget(mTooltip);
    }

    delete mColorsBanCheckbox;
    mColorsBanCheckbox = NULL;
    delete mUnpoweredCheckbox;
    mUnpoweredCheckbox = NULL;
    delete mNoSwapCheckbox;
    mNoSwapCheckbox = NULL;
    delete mSonicCheckbox;
    mSonicCheckbox = NULL;
    delete mColorsBanHit;
    mColorsBanHit = NULL;
    delete mUnpoweredHit;
    mUnpoweredHit = NULL;
    delete mNoSwapHit;
    mNoSwapHit = NULL;
    delete mSonicHit;
    mSonicHit = NULL;
    delete mTooltip;
    mTooltip = NULL;
}

void ModesDialog::LayoutModeCell(Checkbox *theCheckbox, ButtonWidget *theHit, const char *theLabel, int theX, int theY)
{
    int h = theCheckbox->mHeight;
    theCheckbox->Resize(theX, theY, theCheckbox->mWidth, h);

    // Hit area covers the label drawn by DrawCheckboxText (starts at checkbox right edge).
    int labelX = theX + theCheckbox->mWidth;
    int labelW = FONT_DIALOG->StringWidth(theLabel) + ModesDialogGraphics::kLabelHitPadX * 2;
    theHit->Resize(labelX, theY, labelW, ModesDialogGraphics::kLabelHitHeight);
}

void ModesDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    int left = GetLeft();
    int top = GetTop() + ModesDialogGraphics::kFirstRowTopOffset;
    int colW = ModesDialogGraphics::kColumnWidth;
    int rowGap = ModesDialogGraphics::kRowVerticalGap;
    int rowH = mColorsBanCheckbox->mHeight;

    // Row 0: Colors ban | Unpowered
    LayoutModeCell(mColorsBanCheckbox, mColorsBanHit, ModesDialogGraphics::kColorsBanLabel, left, top);
    LayoutModeCell(mUnpoweredCheckbox, mUnpoweredHit, ModesDialogGraphics::kUnpoweredLabel, left + colW, top);

    // Row 1: No swap | Sonic
    int row1Y = top + rowH + rowGap;
    LayoutModeCell(mNoSwapCheckbox, mNoSwapHit, ModesDialogGraphics::kNoSwapLabel, left, row1Y);
    LayoutModeCell(mSonicCheckbox, mSonicHit, ModesDialogGraphics::kSonicLabel, left + colW, row1Y);
}

int ModesDialog::GetPreferredHeight(int theWidth)
{
    return CircleDialog::GetPreferredHeight(theWidth) + ModesDialogGraphics::kExtraPreferredHeight;
}

void ModesDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mColorsBanCheckbox);
    theWidgetManager->AddWidget(mUnpoweredCheckbox);
    theWidgetManager->AddWidget(mNoSwapCheckbox);
    theWidgetManager->AddWidget(mSonicCheckbox);
    theWidgetManager->AddWidget(mColorsBanHit);
    theWidgetManager->AddWidget(mUnpoweredHit);
    theWidgetManager->AddWidget(mNoSwapHit);
    theWidgetManager->AddWidget(mSonicHit);
    theWidgetManager->AddWidget(mTooltip);
}

void ModesDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);

    // May already be removed by PrepareClose(); only remove if still attached.
    if (mColorsBanCheckbox != NULL && mColorsBanCheckbox->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mColorsBanCheckbox);
    if (mUnpoweredCheckbox != NULL && mUnpoweredCheckbox->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mUnpoweredCheckbox);
    if (mNoSwapCheckbox != NULL && mNoSwapCheckbox->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mNoSwapCheckbox);
    if (mSonicCheckbox != NULL && mSonicCheckbox->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mSonicCheckbox);
    if (mColorsBanHit != NULL && mColorsBanHit->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mColorsBanHit);
    if (mUnpoweredHit != NULL && mUnpoweredHit->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mUnpoweredHit);
    if (mNoSwapHit != NULL && mNoSwapHit->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mNoSwapHit);
    if (mSonicHit != NULL && mSonicHit->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mSonicHit);
    if (mTooltip != NULL && mTooltip->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mTooltip);
}

bool ModesDialog::IsModeHitId(int theId) const
{
    return theId == ModesDialogGraphics::kColorsBanHitId ||
           theId == ModesDialogGraphics::kUnpoweredHitId ||
           theId == ModesDialogGraphics::kNoSwapHitId ||
           theId == ModesDialogGraphics::kSonicHitId;
}

const char *ModesDialog::GetDescriptionForModeId(int theId) const
{
    if (theId == ModesDialogGraphics::kColorsBanHitId)
        return ModesDialogGraphics::kColorsBanDescription;
    if (theId == ModesDialogGraphics::kUnpoweredHitId)
        return ModesDialogGraphics::kUnpoweredDescription;
    if (theId == ModesDialogGraphics::kNoSwapHitId)
        return ModesDialogGraphics::kNoSwapDescription;
    if (theId == ModesDialogGraphics::kSonicHitId)
        return ModesDialogGraphics::kSonicDescription;
    return "";
}

void ModesDialog::PositionTooltipNearCursor()
{
    if (mWidgetManager == NULL || mTooltip == NULL || !mTooltip->mVisible)
        return;

    int x = mWidgetManager->mLastMouseX + ModesDialogGraphics::kTooltipCursorOffsetX;
    int y = mWidgetManager->mLastMouseY + ModesDialogGraphics::kTooltipCursorOffsetY;

    if (x + mTooltip->mWidth > CIRCLE_WINDOW_WIDTH)
        x = mWidgetManager->mLastMouseX - mTooltip->mWidth - 8;
    if (y + mTooltip->mHeight > CIRCLE_WINDOW_HEIGHT)
        y = mWidgetManager->mLastMouseY - mTooltip->mHeight - 8;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    mTooltip->Resize(x, y, mTooltip->mWidth, mTooltip->mHeight);
    mWidgetManager->BringToFront(mTooltip);
    mTooltip->MarkDirty();
}

void ModesDialog::ShowTooltip(const char *theText)
{
    if (mTooltip == NULL)
        return;

    mTooltip->mText = theText;

    // Match Graphics::GetWordWrappedHeight: measure with a dest-less Graphics.
    Graphics aMeasure;
    aMeasure.SetFont(FONT_DIALOG);

    int textWidth = ModesDialogGraphics::kTooltipWidth - 2 * ModesDialogGraphics::kTooltipPadX;
    int textHeight = aMeasure.GetWordWrappedHeight(textWidth, theText, ModesDialogGraphics::kTooltipLineSpacing);
    int tipW = ModesDialogGraphics::kTooltipWidth;
    int tipH = textHeight + 2 * ModesDialogGraphics::kTooltipPadY;
    if (tipH < FONT_DIALOG->GetHeight() + 2 * ModesDialogGraphics::kTooltipPadY)
        tipH = FONT_DIALOG->GetHeight() + 2 * ModesDialogGraphics::kTooltipPadY;

    mTooltip->Resize(0, 0, tipW, tipH);
    mTooltip->SetVisible(true);
    PositionTooltipNearCursor();
}

void ModesDialog::HideTooltip()
{
    if (mTooltip != NULL && mTooltip->mVisible)
    {
        mTooltip->SetVisible(false);
        mTooltip->MarkDirty();
    }
}

void ModesDialog::CheckboxChecked(int theId, bool checked)
{
    CircleShootApp *app = GetCircleShootApp();

    if (theId == ModesDialogGraphics::kColorsBanCheckboxId)
    {
        if (checked)
            app->DoColorsBanDialog();
    }
    else if (theId == ModesDialogGraphics::kUnpoweredCheckboxId)
    {
        if (checked)
            app->DoUnpoweredDialog();
    }
    else if (theId == ModesDialogGraphics::kSonicCheckboxId)
    {
        if (checked)
            app->DoSonicDialog();
    }

    MarkDirty();
}

void ModesDialog::ButtonMouseEnter(int theId)
{
    if (!IsModeHitId(theId))
        return;

    mHoveredModeId = theId;
    ShowTooltip(GetDescriptionForModeId(theId));
    MarkDirty();
}

void ModesDialog::ButtonMouseLeave(int theId)
{
    if (!IsModeHitId(theId))
        return;

    if (mHoveredModeId == theId)
    {
        mHoveredModeId = -1;
        HideTooltip();
        MarkDirty();
    }
}

void ModesDialog::ButtonMouseMove(int theId, int theX, int theY)
{
    if (IsModeHitId(theId) && mTooltip != NULL && mTooltip->mVisible)
        PositionTooltipNearCursor();
}

void ModesDialog::DrawModeRow(Graphics *g, Checkbox *theCheckbox, ButtonWidget *theHit, const char *theLabel)
{
    if (theCheckbox == NULL || theHit == NULL)
        return;

    if (theHit->mIsOver)
        g->SetColor(Color(0xFFFFFF));
    else
        g->SetColor(mColors[COLOR_LINES]);

    // Official Options-style drawing: label + line attached to the checkbox gem.
    DrawCheckboxText(g, theLabel, theCheckbox);
}

void ModesDialog::ButtonDepress(int theId)
{
    // Mode-name hit areas are hover-only.
    if (IsModeHitId(theId))
        return;

    // Defer to Dialog so Apply/Cancel go through CircleShootApp::CheckYesNoButton.
    // Do not touch sibling widgets here — FinishModesDialog calls PrepareClose first.
    Dialog::ButtonDepress(theId);
}

bool ModesDialog::IsColorsBanSelected() const
{
    return mColorsBanCheckbox->IsChecked();
}

bool ModesDialog::IsUnpoweredSelected() const
{
    return mUnpoweredCheckbox->IsChecked();
}

bool ModesDialog::IsNoSwapSelected() const
{
    return mNoSwapCheckbox->IsChecked();
}

bool ModesDialog::IsSonicSelected() const
{
    return mSonicCheckbox->IsChecked();
}

void ModesDialog::GetBannedColors(bool outBanned[MAX_BALL_COLORS]) const
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        outBanned[i] = mPendingBannedColors[i];
}

void ModesDialog::SetBannedColors(const bool banned[MAX_BALL_COLORS])
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        mPendingBannedColors[i] = banned[i];
}

void ModesDialog::SetColorsBanSelected(bool selected)
{
    mColorsBanCheckbox->SetChecked(selected, false);
    MarkDirty();
}

void ModesDialog::GetDisabledPowerUps(bool outDisabled[PowerType_Max]) const
{
    for (int i = 0; i < PowerType_Max; i++)
        outDisabled[i] = mPendingDisabledPowerUps[i];
}

void ModesDialog::SetDisabledPowerUps(const bool disabled[PowerType_Max])
{
    for (int i = 0; i < PowerType_Max; i++)
        mPendingDisabledPowerUps[i] = disabled[i];
}

void ModesDialog::SetUnpoweredSelected(bool selected)
{
    mUnpoweredCheckbox->SetChecked(selected, false);
    MarkDirty();
}

float ModesDialog::GetChainSpeedMultiplier() const
{
    return mPendingChainSpeedMultiplier;
}

void ModesDialog::SetChainSpeedMultiplier(float multiplier)
{
    mPendingChainSpeedMultiplier = multiplier;
}

void ModesDialog::SetSonicSelected(bool selected)
{
    mSonicCheckbox->SetChecked(selected, false);
    MarkDirty();
}

void ModesDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);

    g->SetFont(FONT_DIALOG);

    DrawModeRow(g, mColorsBanCheckbox, mColorsBanHit, ModesDialogGraphics::kColorsBanLabel);
    DrawModeRow(g, mUnpoweredCheckbox, mUnpoweredHit, ModesDialogGraphics::kUnpoweredLabel);
    DrawModeRow(g, mNoSwapCheckbox, mNoSwapHit, ModesDialogGraphics::kNoSwapLabel);
    DrawModeRow(g, mSonicCheckbox, mSonicHit, ModesDialogGraphics::kSonicLabel);
}
