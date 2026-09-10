#include "Zuma_Prefix.pch"

#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Checkbox.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/Image.h>
#include <SexyAppFramework/DialogButton.h>
#include <SexyAppFramework/ButtonWidget.h>
#include <SexyAppFramework/ListWidget.h>
#include <SexyAppFramework/ScrollbarWidget.h>
#include <SexyAppFramework/SexyAppBase.h>

#include "CircleShootApp.h"
#include "CircleCheckbox.h"
#include "CircleCommon.h"
#include "ModesDialog.h"
#include "Res.h"

using namespace Sexy;

///////////////////////////////////////////////////////////////////////////////
// ModesCatalog — edit layout / groups / modes here only
///////////////////////////////////////////////////////////////////////////////
namespace Sexy
{
namespace ModesCatalog
{
    // --- Layout knobs ---
    const float kLeftPaneWidthFraction = 0.25f;
    const int kModesPerRow = 2;
    const int kModeRowGap = 28;
    const int kModeColGap = 12;
    const int kPanePad = 4;
    const int kScrollbarWidth = 16;
    const int kContentBottomPad = 8;
    const int kDividerHalfGap = 4;
    const int kLabelHitPadX = 4;
    const int kLabelHitHeight = 32;
    const int kGroupItemHeight = 18;

    // --- Dialog chrome ---
    const char *const kTitle = "MODES";
    const char *const kEmptyLines = "";
    const char *const kEmptyFooter = "";
    const char *const kApplyLabel = "Apply";
    const char *const kCancelLabel = "Cancel";

    // --- Descriptions ---
    const char *const kColorsBanDescription =
        "Choose which colors you cannot destroy directly, or randomly ban 1-5 colors each round. Matching three or more of a banned color causes you to lose.";
    const char *const kUnpoweredDescription =
        "Choose which power-ups are disabled and will not appear on the chain.";
    const char *const kNoSwapDescription =
        "The frog cannot swap colors with right-click, and the next ball color is hidden.";
    const char *const kSonicDescription =
        "Increase the ball chain speed. When enabled, choose a multiplier from 1x to 10x (in 0.5 steps).";
    const char *const kMachineGunDescription =
        "No fire-rate limit. Every left click throws a ball immediately, even while the frog is still animating.";
    const char *const kBomberDescription =
        "Hitting a chain of 2 or more balls of the matching color triggers a bomb explosion at the impact.";
    const char *const kUglyChainDescription =
        "The rolling chain never places the same color next to itself. Every neighbor pair is a different color.";
    const char *const kMovingHoleDescription =
        "The end hole crawls backward along the path at a speed you choose, so the chain has less and less distance before it falls in.";
    const char *const kMaxPowerDescription =
        "Choose what percent of balls on the chain spawn as power-ups. Optionally replace loud power-up sounds with regular destroy sounds.";
    const char *const kCombolessDescription =
        "Combos are disabled. Matching groups no longer suck together across gaps, and clears do not chain-react into further clears.";
    const char *const kGapFreeDescription =
        "Gap bonuses are disabled. Clearing through a gap awards normal points only.";
    const char *const kChainCountDescription =
        "Choose how many clears in a row are required before the chain bonus starts (1-20). Optionally disable the chain bonus entirely.";
    const char *const kBankruptDescription =
        "No coins appear. It never spawns during the level.";
    const char *const kColorShiftDescription =
        "All balls on the rolling chain periodically change color. Enable colors to shift, choose each destination, set the interval (1-10 sec), or use random remapping.";
    const char *const kInvisibleDescription =
        "Periodically hide a random percent of chain balls. Choose duration, wave interval, and how many balls go invisible.";
    const char *const kBaseMinimumDescription =
        "Strip the game to minimum: no HUD or path art, pitch-black levels, colored circles instead of sprites, no particles or trail highlights.";
    const char *const kAutoAdvanceDescription =
        "Levels start and advance immediately with no path-cracking or stage transition animations. After a loss, the level restarts immediately (or shows results on game over).";
    const char *const kKillerBallDescription =
        "Periodically a chain ball becomes a killer and flies at the frog. Shoot it with the matching color to destroy it. Choose spawn interval and flight time.";

    // --- Widget ids ---
    const int kGroupListId = 0;
    const int kModesScrollbarId = 1;
    const int kHitIdBase = 100;

    // --- Groups (order = left-list order) ---
    struct GroupDef
    {
        GroupId id;
        const char *name;
    };

    static const GroupDef kGroups[] = {
        {Group_GameMechanics, "Game Mechanics"},
        {Group_ForFun, "For Fun"},
        {Group_Challenges, "Challenges"},
    };
    static const int kGroupCount = sizeof(kGroups) / sizeof(kGroups[0]);

    // --- Modes (change groupId to move between groups) ---
    struct ModeDef
    {
        ModeId id;
        GroupId groupId;
        const char *label;
        const char *description;
        bool opensPicker;
    };

    static const ModeDef kModes[] = {
        {Mode_ColorsBan, Group_Challenges, "Colors ban", kColorsBanDescription, true},
        {Mode_MovingHole, Group_Challenges, "Moving hole", kMovingHoleDescription, true},
        {Mode_Sonic, Group_Challenges, "Sonic speed", kSonicDescription, true},
        {Mode_UglyChain, Group_Challenges, "Ugly chain", kUglyChainDescription, false},
        {Mode_ColorShift, Group_Challenges, "Color shift", kColorShiftDescription, true},
        {Mode_Invisible, Group_Challenges, "Invisible", kInvisibleDescription, true},
        {Mode_KillerBall, Group_Challenges, "Killer Ball", kKillerBallDescription, true},
        {Mode_Unpowered, Group_GameMechanics, "Unpowered", kUnpoweredDescription, true},
        {Mode_NoSwap, Group_GameMechanics, "No swap", kNoSwapDescription, false},
        {Mode_Comboless, Group_GameMechanics, "Comboless", kCombolessDescription, false},
        {Mode_GapFree, Group_GameMechanics, "Gap free", kGapFreeDescription, false},
        {Mode_ChainCount, Group_GameMechanics, "Chain count", kChainCountDescription, true},
        {Mode_Bankrupt, Group_GameMechanics, "Bankrupt", kBankruptDescription, false},
        {Mode_BaseMinimum, Group_GameMechanics, "Base Minimum", kBaseMinimumDescription, false},
        {Mode_AutoAdvance, Group_GameMechanics, "Auto-advance", kAutoAdvanceDescription, false},
        {Mode_MachineGun, Group_ForFun, "Machine gun", kMachineGunDescription, false},
        {Mode_Bomber, Group_ForFun, "Bomber", kBomberDescription, false},
        {Mode_MaxPower, Group_ForFun, "Max power", kMaxPowerDescription, true},
    };
    static const int kModeCount = sizeof(kModes) / sizeof(kModes[0]);

    static const ModeDef *FindModeDef(ModeId theId)
    {
        for (int i = 0; i < kModeCount; i++)
        {
            if (kModes[i].id == theId)
                return &kModes[i];
        }
        return NULL;
    }

    static int HitIdForMode(ModeId theId)
    {
        return kHitIdBase + (int)theId;
    }

    // --- Tooltip ---
    const int kTooltipWidth = 230;
    const int kTooltipPadX = 8;
    const int kTooltipPadY = 6;
    const int kTooltipCursorOffsetX = 14;
    const int kTooltipCursorOffsetY = 18;
    const int kTooltipLineSpacing = -1;

    static int aGroupListColors[5][3] = {
        {41, 73, 24},
        {0, 0, 0},
        {206, 227, 33},
        {255, 255, 255},
        {173, 40, 198}};
}
} // namespace Sexy / ModesCatalog

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
                     Rect(ModesCatalog::kTooltipPadX,
                          ModesCatalog::kTooltipPadY,
                          mWidth - 2 * ModesCatalog::kTooltipPadX,
                          mHeight - 2 * ModesCatalog::kTooltipPadY),
                     mText,
                     ModesCatalog::kTooltipLineSpacing,
                     -1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ModesPane::ModesPane(ModesDialog *theDialog)
{
    mDialog = theDialog;
    mScrollY = 0;
    mContentHeight = 0;
    mClip = true;
    mHasTransparencies = true;
    mHasAlpha = true;
}

void ModesPane::Draw(Graphics *g)
{
    if (mDialog != NULL)
        mDialog->DrawModeLabels(g);
}

void ModesPane::ScrollPosition(int theId, double thePosition)
{
    (void)theId;
    int newScroll = (int)thePosition;
    if (newScroll == mScrollY)
        return;

    mScrollY = newScroll;
    if (mDialog != NULL)
        mDialog->LayoutModeWidgets();
    MarkDirty();
}

void ModesPane::MouseWheel(int theDelta)
{
    if (mDialog == NULL || mDialog->mModesScrollbar == NULL)
        return;

    ScrollbarWidget *sb = mDialog->mModesScrollbar;
    if (!sb->mVisible)
        return;

    double step = 20.0;
    sb->SetValue(sb->mValue - theDelta * step);
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
                                          ModesCatalog::kTitle,
                                          ModesCatalog::kEmptyLines,
                                          ModesCatalog::kEmptyFooter,
                                          Dialog::BUTTONS_OK_CANCEL, false)
{
    mHoveredHitId = -1;
    mSelectedGroupIndex = 0;
    mDividerX = 0;
    mTooltip = new ModeHelpTooltip();
    mModesPane = new ModesPane(this);

    mGroupList = new ListWidget(ModesCatalog::kGroupListId, Sexy::FONT_DIALOG, this);
    mGroupScrollbar = new ScrollbarWidget(ModesCatalog::kGroupListId, mGroupList);
    mGroupList->mScrollbar = mGroupScrollbar;
    mGroupList->mJustify = ListWidget::JUSTIFY_CENTER;
    mGroupList->mItemHeight = ModesCatalog::kGroupItemHeight;
    mGroupList->SetColors(ModesCatalog::aGroupListColors, 5);
    mGroupList->mDrawOutline = true;
    mGroupScrollbar->SetInvisIfNoScroll(true);

    mModesScrollbar = new ScrollbarWidget(ModesCatalog::kModesScrollbarId, mModesPane);
    mModesScrollbar->SetInvisIfNoScroll(true);

    for (int i = 0; i < ModesCatalog::Mode_Count; i++)
    {
        mModeSlots[i].mCheckbox = NULL;
        mModeSlots[i].mHit = NULL;
    }

    for (int i = 0; i < ModesCatalog::kModeCount; i++)
    {
        const ModesCatalog::ModeDef &def = ModesCatalog::kModes[i];
        ModeWidgetSlot &slot = mModeSlots[def.id];
        slot.mCheckbox = MakeCheckbox((int)def.id, this);
        slot.mHit = CreateModeHitArea(ModesCatalog::HitIdForMode(def.id));
        mModesPane->AddWidget(slot.mCheckbox);
        mModesPane->AddWidget(slot.mHit);
    }

    mColorsBanCheckbox = mModeSlots[ModesCatalog::Mode_ColorsBan].mCheckbox;
    mUnpoweredCheckbox = mModeSlots[ModesCatalog::Mode_Unpowered].mCheckbox;
    mNoSwapCheckbox = mModeSlots[ModesCatalog::Mode_NoSwap].mCheckbox;
    mSonicCheckbox = mModeSlots[ModesCatalog::Mode_Sonic].mCheckbox;
    mMachineGunCheckbox = mModeSlots[ModesCatalog::Mode_MachineGun].mCheckbox;
    mBomberCheckbox = mModeSlots[ModesCatalog::Mode_Bomber].mCheckbox;
    mUglyChainCheckbox = mModeSlots[ModesCatalog::Mode_UglyChain].mCheckbox;
    mMovingHoleCheckbox = mModeSlots[ModesCatalog::Mode_MovingHole].mCheckbox;
    mMaxPowerCheckbox = mModeSlots[ModesCatalog::Mode_MaxPower].mCheckbox;
    mCombolessCheckbox = mModeSlots[ModesCatalog::Mode_Comboless].mCheckbox;
    mGapFreeCheckbox = mModeSlots[ModesCatalog::Mode_GapFree].mCheckbox;
    mChainCountCheckbox = mModeSlots[ModesCatalog::Mode_ChainCount].mCheckbox;
    mBankruptCheckbox = mModeSlots[ModesCatalog::Mode_Bankrupt].mCheckbox;
    mColorShiftCheckbox = mModeSlots[ModesCatalog::Mode_ColorShift].mCheckbox;
    mInvisibleCheckbox = mModeSlots[ModesCatalog::Mode_Invisible].mCheckbox;
    mBaseMinimumCheckbox = mModeSlots[ModesCatalog::Mode_BaseMinimum].mCheckbox;
    mAutoAdvanceCheckbox = mModeSlots[ModesCatalog::Mode_AutoAdvance].mCheckbox;
    mKillerBallCheckbox = mModeSlots[ModesCatalog::Mode_KillerBall].mCheckbox;

    CircleShootApp *app = GetCircleShootApp();
    if (mColorsBanCheckbox != NULL)
        mColorsBanCheckbox->mChecked = app->mColorsBanMode;
    if (mUnpoweredCheckbox != NULL)
        mUnpoweredCheckbox->mChecked = app->mUnpoweredMode;
    if (mNoSwapCheckbox != NULL)
        mNoSwapCheckbox->mChecked = app->mNoSwapMode;
    if (mSonicCheckbox != NULL)
        mSonicCheckbox->mChecked = app->mSonicMode;
    if (mMachineGunCheckbox != NULL)
        mMachineGunCheckbox->mChecked = app->mMachineGunMode;
    if (mBomberCheckbox != NULL)
        mBomberCheckbox->mChecked = app->mBomberMode;
    if (mUglyChainCheckbox != NULL)
        mUglyChainCheckbox->mChecked = app->mUglyChainMode;
    if (mMovingHoleCheckbox != NULL)
        mMovingHoleCheckbox->mChecked = app->mMovingHoleMode;
    if (mMaxPowerCheckbox != NULL)
        mMaxPowerCheckbox->mChecked = app->mMaxPowerMode;
    if (mCombolessCheckbox != NULL)
        mCombolessCheckbox->mChecked = app->mCombolessMode;
    if (mGapFreeCheckbox != NULL)
        mGapFreeCheckbox->mChecked = app->mGapFreeMode;
    if (mChainCountCheckbox != NULL)
        mChainCountCheckbox->mChecked = app->mChainCountMode;
    if (mBankruptCheckbox != NULL)
        mBankruptCheckbox->mChecked = app->mBankruptMode;
    if (mColorShiftCheckbox != NULL)
        mColorShiftCheckbox->mChecked = app->mColorShiftMode;
    if (mInvisibleCheckbox != NULL)
        mInvisibleCheckbox->mChecked = app->mInvisibleMode;
    if (mBaseMinimumCheckbox != NULL)
        mBaseMinimumCheckbox->mChecked = app->mBaseMinimumMode;
    if (mAutoAdvanceCheckbox != NULL)
        mAutoAdvanceCheckbox->mChecked = app->mAutoAdvanceMode;
    if (mKillerBallCheckbox != NULL)
        mKillerBallCheckbox->mChecked = app->mKillerBallMode;

    for (int i = 0; i < MAX_BALL_COLORS; i++)
        mPendingBannedColors[i] = app->mBannedColors[i];
    mPendingColorsBanRandom = app->mColorsBanRandom;
    mPendingColorsBanRandomCount = app->mColorsBanRandomCount;
    if (mPendingColorsBanRandomCount < 1 || mPendingColorsBanRandomCount > 5)
        mPendingColorsBanRandomCount = 1;
    for (int i = 0; i < PowerType_Max; i++)
        mPendingDisabledPowerUps[i] = app->mDisabledPowerUps[i];
    mPendingChainSpeedMultiplier = app->mChainSpeedMultiplier;
    mPendingMovingHoleSpeed = app->mMovingHoleSpeed;
    mPendingMaxPowerPercent = app->mMaxPowerPercent;
    mPendingMaxPowerQuietSounds = app->mMaxPowerQuietSounds;
    mPendingChainBonusThreshold = app->mChainBonusThreshold;
    if (mPendingChainBonusThreshold < 1 || mPendingChainBonusThreshold > 20)
        mPendingChainBonusThreshold = 5;
    mPendingChainBonusDisabled = app->mChainBonusDisabled;
    mPendingColorShiftHz = app->mColorShiftHz;
    if (mPendingColorShiftHz < 1.0f || mPendingColorShiftHz > 10.0f)
        mPendingColorShiftHz = 1.0f;
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        mPendingColorShiftMap[i] = app->mColorShiftMap[i];
        mPendingColorShiftEnabled[i] = app->mColorShiftEnabled[i];
    }
    mPendingColorShiftRandom = app->mColorShiftRandom;
    mPendingInvisibleDurationSec = app->mInvisibleDurationSec;
    if (mPendingInvisibleDurationSec < 0.5f || mPendingInvisibleDurationSec > 10.0f)
        mPendingInvisibleDurationSec = 3.0f;
    mPendingInvisibleIntervalSec = app->mInvisibleIntervalSec;
    if (mPendingInvisibleIntervalSec < 0.5f || mPendingInvisibleIntervalSec > 10.0f)
        mPendingInvisibleIntervalSec = 5.0f;
    mPendingInvisiblePercent = app->mInvisiblePercent;
    if (mPendingInvisiblePercent < 4 || mPendingInvisiblePercent > 100)
        mPendingInvisiblePercent = 50;
    mPendingInvisiblePercent = (mPendingInvisiblePercent / 2) * 2;
    if (mPendingInvisiblePercent < 4)
        mPendingInvisiblePercent = 4;
    mPendingKillerBallIntervalSec = app->mKillerBallIntervalSec;
    if (mPendingKillerBallIntervalSec < 2.0f || mPendingKillerBallIntervalSec > 15.0f)
        mPendingKillerBallIntervalSec = 10.0f;
    mPendingKillerBallFlightSec = app->mKillerBallFlightSec;
    if (mPendingKillerBallFlightSec < 2.0f || mPendingKillerBallFlightSec > 10.0f)
        mPendingKillerBallFlightSec = 10.0f;

    for (int i = 0; i < ModesCatalog::kGroupCount; i++)
        mGroupList->AddLine(ModesCatalog::kGroups[i].name, false);
    mGroupList->SetSelect(0);

    mYesButton->mLabel = ModesCatalog::kApplyLabel;
    mNoButton->mLabel = ModesCatalog::kCancelLabel;
}

ModesDialog::~ModesDialog()
{
    // PrepareClose() should already have cleaned up; keep guards for safety.
    for (int i = 0; i < ModesCatalog::Mode_Count; i++)
    {
        delete mModeSlots[i].mCheckbox;
        mModeSlots[i].mCheckbox = NULL;
        delete mModeSlots[i].mHit;
        mModeSlots[i].mHit = NULL;
    }
    mColorsBanCheckbox = NULL;
    mUnpoweredCheckbox = NULL;
    mNoSwapCheckbox = NULL;
    mSonicCheckbox = NULL;
    mMachineGunCheckbox = NULL;
    mBomberCheckbox = NULL;
    mUglyChainCheckbox = NULL;
    mMovingHoleCheckbox = NULL;
    mMaxPowerCheckbox = NULL;
    mCombolessCheckbox = NULL;
    mGapFreeCheckbox = NULL;
    mChainCountCheckbox = NULL;
    mBankruptCheckbox = NULL;
    mColorShiftCheckbox = NULL;
    mInvisibleCheckbox = NULL;
    mBaseMinimumCheckbox = NULL;
    mAutoAdvanceCheckbox = NULL;
    mKillerBallCheckbox = NULL;

    delete mModesPane;
    mModesPane = NULL;
    delete mModesScrollbar;
    mModesScrollbar = NULL;
    delete mGroupList;
    mGroupList = NULL;
    delete mGroupScrollbar;
    mGroupScrollbar = NULL;
    delete mTooltip;
    mTooltip = NULL;
}

void ModesDialog::PrepareClose()
{
    HideTooltip();

    WidgetManager *wm = mWidgetManager;
    if (wm != NULL)
    {
        if (mGroupList != NULL && mGroupList->mWidgetManager != NULL)
            wm->RemoveWidget(mGroupList);
        if (mGroupScrollbar != NULL && mGroupScrollbar->mWidgetManager != NULL)
            wm->RemoveWidget(mGroupScrollbar);
        if (mModesPane != NULL && mModesPane->mWidgetManager != NULL)
            wm->RemoveWidget(mModesPane);
        if (mModesScrollbar != NULL && mModesScrollbar->mWidgetManager != NULL)
            wm->RemoveWidget(mModesScrollbar);
        if (mTooltip != NULL && mTooltip->mWidgetManager != NULL)
            wm->RemoveWidget(mTooltip);
    }

    if (mModesPane != NULL)
        mModesPane->RemoveAllWidgets(false);

    for (int i = 0; i < ModesCatalog::Mode_Count; i++)
    {
        delete mModeSlots[i].mCheckbox;
        mModeSlots[i].mCheckbox = NULL;
        delete mModeSlots[i].mHit;
        mModeSlots[i].mHit = NULL;
    }
    mColorsBanCheckbox = NULL;
    mUnpoweredCheckbox = NULL;
    mNoSwapCheckbox = NULL;
    mSonicCheckbox = NULL;
    mMachineGunCheckbox = NULL;
    mBomberCheckbox = NULL;
    mUglyChainCheckbox = NULL;
    mMovingHoleCheckbox = NULL;
    mMaxPowerCheckbox = NULL;
    mCombolessCheckbox = NULL;
    mGapFreeCheckbox = NULL;
    mChainCountCheckbox = NULL;
    mBankruptCheckbox = NULL;
    mColorShiftCheckbox = NULL;
    mInvisibleCheckbox = NULL;
    mBaseMinimumCheckbox = NULL;
    mAutoAdvanceCheckbox = NULL;
    mKillerBallCheckbox = NULL;

    delete mModesPane;
    mModesPane = NULL;
    delete mModesScrollbar;
    mModesScrollbar = NULL;
    delete mGroupList;
    mGroupList = NULL;
    delete mGroupScrollbar;
    mGroupScrollbar = NULL;
    delete mTooltip;
    mTooltip = NULL;
}

void ModesDialog::LayoutModeWidgets()
{
    if (mModesPane == NULL)
        return;

    ModesCatalog::GroupId selectedGroup = ModesCatalog::Group_GameMechanics;
    if (mSelectedGroupIndex >= 0 && mSelectedGroupIndex < ModesCatalog::kGroupCount)
        selectedGroup = ModesCatalog::kGroups[mSelectedGroupIndex].id;

    int scrollY = mModesPane->mScrollY;
    int cols = ModesCatalog::kModesPerRow;
    if (cols < 1)
        cols = 1;

    int paneW = mModesPane->mWidth;
    int colW = (paneW - ModesCatalog::kModeColGap * (cols - 1)) / cols;
    if (colW < 1)
        colW = 1;

    int rowH = 0;
    int visibleIndex = 0;

    for (int i = 0; i < ModesCatalog::Mode_Count; i++)
    {
        ModeWidgetSlot &slot = mModeSlots[i];
        if (slot.mCheckbox == NULL || slot.mHit == NULL)
            continue;

        const ModesCatalog::ModeDef *def = ModesCatalog::FindModeDef((ModesCatalog::ModeId)i);
        bool inGroup = (def != NULL && def->groupId == selectedGroup);

        slot.mCheckbox->SetVisible(inGroup);
        slot.mHit->SetVisible(inGroup);

        if (!inGroup)
            continue;

        if (rowH == 0)
            rowH = slot.mCheckbox->mHeight;

        int col = visibleIndex % cols;
        int row = visibleIndex / cols;
        int x = col * (colW + ModesCatalog::kModeColGap);
        int y = ModesCatalog::kPanePad + row * (rowH + ModesCatalog::kModeRowGap) - scrollY;

        slot.mCheckbox->Resize(x, y, slot.mCheckbox->mWidth, slot.mCheckbox->mHeight);

        int labelX = x + slot.mCheckbox->mWidth;
        int labelW = FONT_DIALOG->StringWidth(def->label) + ModesCatalog::kLabelHitPadX * 2;
        slot.mHit->Resize(labelX, y, labelW, ModesCatalog::kLabelHitHeight);

        visibleIndex++;
    }

    if (rowH == 0)
        rowH = ModesCatalog::kLabelHitHeight;

    int rows = (visibleIndex + cols - 1) / cols;
    int contentH = 0;
    if (rows > 0)
        contentH = ModesCatalog::kPanePad * 2 + rows * rowH + (rows - 1) * ModesCatalog::kModeRowGap;

    mModesPane->mContentHeight = contentH;
    UpdateModesScrollbar();
}

void ModesDialog::UpdateModesScrollbar()
{
    if (mModesPane == NULL || mModesScrollbar == NULL)
        return;

    int page = mModesPane->mHeight;
    int content = mModesPane->mContentHeight;

    mModesScrollbar->SetPageSize(page);
    mModesScrollbar->SetMaxValue(content);

    int maxScroll = content - page;
    if (maxScroll < 0)
        maxScroll = 0;

    if (mModesPane->mScrollY > maxScroll)
    {
        mModesPane->mScrollY = maxScroll;
        mModesScrollbar->mValue = maxScroll;
    }
}

void ModesDialog::SelectGroup(int theIndex)
{
    if (theIndex < 0 || theIndex >= ModesCatalog::kGroupCount)
        return;

    mSelectedGroupIndex = theIndex;
    if (mGroupList != NULL)
        mGroupList->SetSelect(theIndex);

    if (mModesPane != NULL)
        mModesPane->mScrollY = 0;
    if (mModesScrollbar != NULL)
        mModesScrollbar->SetValue(0);

    LayoutModeWidgets();
    MarkDirty();
}

void ModesDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    CircleDialog::Resize(theX, theY, theWidth, theHeight);

    if (mGroupList == NULL || mModesPane == NULL)
        return;

    int contentLeft = GetLeft();
    int contentTop = GetTop();
    int contentWidth = GetWidth();
    int contentBottom = mYesButton->mY - ModesCatalog::kContentBottomPad;
    int contentHeight = contentBottom - contentTop;
    if (contentHeight < 40)
        contentHeight = 40;

    float leftFrac = ModesCatalog::kLeftPaneWidthFraction;
    if (leftFrac < 0.05f)
        leftFrac = 0.05f;
    if (leftFrac > 0.9f)
        leftFrac = 0.9f;

    int leftPaneW = (int)(contentWidth * leftFrac);
    int rightPaneW = contentWidth - leftPaneW;
    mDividerX = contentLeft + leftPaneW;

    int groupSbW = ModesCatalog::kScrollbarWidth;
    int groupListW = leftPaneW - ModesCatalog::kPanePad * 2 - groupSbW;
    if (groupListW < 20)
        groupListW = 20;

    int groupX = contentLeft + ModesCatalog::kPanePad;
    mGroupList->Resize(groupX, contentTop, groupListW, contentHeight);
    mGroupScrollbar->ResizeScrollbar(groupX + groupListW, contentTop, groupSbW, contentHeight);

    int modesX = mDividerX + ModesCatalog::kDividerHalfGap + ModesCatalog::kPanePad;
    int modesRight = contentLeft + contentWidth - ModesCatalog::kPanePad;
    int modesSbW = ModesCatalog::kScrollbarWidth;
    int modesPaneW = modesRight - modesX - modesSbW;
    if (modesPaneW < 40)
        modesPaneW = 40;

    mModesPane->Resize(modesX, contentTop, modesPaneW, contentHeight);
    mModesScrollbar->ResizeScrollbar(modesX + modesPaneW, contentTop, modesSbW, contentHeight);

    LayoutModeWidgets();
}

int ModesDialog::GetPreferredHeight(int theWidth)
{
    (void)theWidth;
    return CIRCLE_WINDOW_HEIGHT;
}

void ModesDialog::AddedToManager(WidgetManager *theWidgetManager)
{
    CircleDialog::AddedToManager(theWidgetManager);
    theWidgetManager->AddWidget(mGroupList);
    theWidgetManager->AddWidget(mGroupScrollbar);
    theWidgetManager->AddWidget(mModesPane);
    theWidgetManager->AddWidget(mModesScrollbar);
    theWidgetManager->AddWidget(mTooltip);
}

void ModesDialog::RemovedFromManager(WidgetManager *theWidgetManager)
{
    CircleDialog::RemovedFromManager(theWidgetManager);

    if (mGroupList != NULL && mGroupList->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mGroupList);
    if (mGroupScrollbar != NULL && mGroupScrollbar->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mGroupScrollbar);
    if (mModesPane != NULL && mModesPane->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mModesPane);
    if (mModesScrollbar != NULL && mModesScrollbar->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mModesScrollbar);
    if (mTooltip != NULL && mTooltip->mWidgetManager != NULL)
        theWidgetManager->RemoveWidget(mTooltip);
}

bool ModesDialog::IsModeHitId(int theId) const
{
    return theId >= ModesCatalog::kHitIdBase &&
           theId < ModesCatalog::kHitIdBase + ModesCatalog::Mode_Count;
}

ModesCatalog::ModeId ModesDialog::ModeIdFromHitId(int theId) const
{
    return (ModesCatalog::ModeId)(theId - ModesCatalog::kHitIdBase);
}

Checkbox *ModesDialog::CheckboxForMode(ModesCatalog::ModeId theId) const
{
    if (theId < 0 || theId >= ModesCatalog::Mode_Count)
        return NULL;
    return mModeSlots[theId].mCheckbox;
}

const char *ModesDialog::GetDescriptionForHitId(int theId) const
{
    if (!IsModeHitId(theId))
        return "";

    const ModesCatalog::ModeDef *def = ModesCatalog::FindModeDef(ModeIdFromHitId(theId));
    if (def == NULL)
        return "";
    return def->description;
}

void ModesDialog::PositionTooltipNearCursor()
{
    if (mWidgetManager == NULL || mTooltip == NULL || !mTooltip->mVisible)
        return;

    int x = mWidgetManager->mLastMouseX + ModesCatalog::kTooltipCursorOffsetX;
    int y = mWidgetManager->mLastMouseY + ModesCatalog::kTooltipCursorOffsetY;

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

    Graphics aMeasure;
    aMeasure.SetFont(FONT_DIALOG);

    int textWidth = ModesCatalog::kTooltipWidth - 2 * ModesCatalog::kTooltipPadX;
    int textHeight = aMeasure.GetWordWrappedHeight(textWidth, theText, ModesCatalog::kTooltipLineSpacing);
    int tipW = ModesCatalog::kTooltipWidth;
    int tipH = textHeight + 2 * ModesCatalog::kTooltipPadY;
    if (tipH < FONT_DIALOG->GetHeight() + 2 * ModesCatalog::kTooltipPadY)
        tipH = FONT_DIALOG->GetHeight() + 2 * ModesCatalog::kTooltipPadY;

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

void ModesDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
    (void)theClickCount;
    if (theId != ModesCatalog::kGroupListId)
        return;

    SelectGroup(theIdx);
}

void ModesDialog::CheckboxChecked(int theId, bool checked)
{
    (void)theId;
    (void)checked;
    MarkDirty();
}

void ModesDialog::OpenModePicker(ModesCatalog::ModeId theId)
{
    CircleShootApp *app = GetCircleShootApp();
    const ModesCatalog::ModeDef *def = ModesCatalog::FindModeDef(theId);
    if (app == NULL || def == NULL || !def->opensPicker)
        return;

    if (def->id == ModesCatalog::Mode_ColorsBan)
        app->DoColorsBanDialog();
    else if (def->id == ModesCatalog::Mode_Unpowered)
        app->DoUnpoweredDialog();
    else if (def->id == ModesCatalog::Mode_Sonic)
        app->DoSonicDialog();
    else if (def->id == ModesCatalog::Mode_MovingHole)
        app->DoMovingHoleDialog();
    else if (def->id == ModesCatalog::Mode_MaxPower)
        app->DoMaxPowerDialog();
    else if (def->id == ModesCatalog::Mode_ChainCount)
        app->DoChainCountDialog();
    else if (def->id == ModesCatalog::Mode_ColorShift)
        app->DoColorShiftDialog();
    else if (def->id == ModesCatalog::Mode_Invisible)
        app->DoInvisibleDialog();
    else if (def->id == ModesCatalog::Mode_KillerBall)
        app->DoKillerBallDialog();
}

void ModesDialog::ButtonMouseEnter(int theId)
{
    if (!IsModeHitId(theId))
        return;

    mHoveredHitId = theId;
    ShowTooltip(GetDescriptionForHitId(theId));
    if (mModesPane != NULL)
        mModesPane->MarkDirty();
    MarkDirty();
}

void ModesDialog::ButtonMouseLeave(int theId)
{
    if (!IsModeHitId(theId))
        return;

    if (mHoveredHitId == theId)
    {
        mHoveredHitId = -1;
        HideTooltip();
        if (mModesPane != NULL)
            mModesPane->MarkDirty();
        MarkDirty();
    }
}

void ModesDialog::ButtonMouseMove(int theId, int theX, int theY)
{
    (void)theX;
    (void)theY;
    if (IsModeHitId(theId) && mTooltip != NULL && mTooltip->mVisible)
        PositionTooltipNearCursor();
}

void ModesDialog::DrawModeLabels(Graphics *g)
{
    // Graphics is translated to ModesPane; child widget coords are pane-relative.
    g->SetFont(FONT_DIALOG);

    ModesCatalog::GroupId selectedGroup = ModesCatalog::Group_GameMechanics;
    if (mSelectedGroupIndex >= 0 && mSelectedGroupIndex < ModesCatalog::kGroupCount)
        selectedGroup = ModesCatalog::kGroups[mSelectedGroupIndex].id;

    for (int i = 0; i < ModesCatalog::kModeCount; i++)
    {
        const ModesCatalog::ModeDef &def = ModesCatalog::kModes[i];
        if (def.groupId != selectedGroup)
            continue;

        ModeWidgetSlot &slot = mModeSlots[def.id];
        if (slot.mCheckbox == NULL || slot.mHit == NULL || !slot.mCheckbox->mVisible)
            continue;

        if (slot.mHit->mIsOver)
            g->SetColor(Color(0xFFFFFF));
        else
            g->SetColor(mColors[COLOR_LINES]);

        // Local DrawCheckboxText equivalent (pane-relative coords).
        int aX = slot.mCheckbox->mX + slot.mCheckbox->mWidth;
        int aY = slot.mCheckbox->mY;
        g->DrawString(def.label, aX, aY + 25);

        Image *aImage = Sexy::IMAGE_DIALOG_CHECKBOXLINE;
        int aStartX = aX - 5;
        int aStrWidth = g->GetFont()->StringWidth(def.label);
        int aEndX = aStrWidth + 5;
        int aY2 = aY + 29;

        for (int px = 0; px < aEndX; px += aImage->GetWidth())
        {
            Rect aRect(0, 0, aEndX - px, aImage->GetHeight());
            int aWidth = aEndX - px;
            if (aWidth > aImage->GetWidth())
                aWidth = aImage->GetWidth();
            aRect.mWidth = aWidth;
            g->DrawImage(aImage, px + aStartX, aY2, aRect);
        }

        g->DrawImage(Sexy::IMAGE_DIALOG_CHECKBOXCAP, aEndX + aStartX, aY2);
    }
}

void ModesDialog::ButtonDepress(int theId)
{
    if (IsModeHitId(theId))
    {
        ModesCatalog::ModeId modeId = ModeIdFromHitId(theId);
        const ModesCatalog::ModeDef *def = ModesCatalog::FindModeDef(modeId);
        if (def != NULL && def->opensPicker)
        {
            OpenModePicker(modeId);
            MarkDirty();
            return;
        }

        Checkbox *checkbox = CheckboxForMode(modeId);
        if (checkbox != NULL && checkbox->mVisible)
            checkbox->SetChecked(!checkbox->IsChecked());
        return;
    }

    Dialog::ButtonDepress(theId);
}

bool ModesDialog::IsColorsBanSelected() const
{
    return mColorsBanCheckbox != NULL && mColorsBanCheckbox->IsChecked();
}

bool ModesDialog::IsUnpoweredSelected() const
{
    return mUnpoweredCheckbox != NULL && mUnpoweredCheckbox->IsChecked();
}

bool ModesDialog::IsNoSwapSelected() const
{
    return mNoSwapCheckbox != NULL && mNoSwapCheckbox->IsChecked();
}

bool ModesDialog::IsSonicSelected() const
{
    return mSonicCheckbox != NULL && mSonicCheckbox->IsChecked();
}

bool ModesDialog::IsMachineGunSelected() const
{
    return mMachineGunCheckbox != NULL && mMachineGunCheckbox->IsChecked();
}

bool ModesDialog::IsBomberSelected() const
{
    return mBomberCheckbox != NULL && mBomberCheckbox->IsChecked();
}

bool ModesDialog::IsUglyChainSelected() const
{
    return mUglyChainCheckbox != NULL && mUglyChainCheckbox->IsChecked();
}

bool ModesDialog::IsMovingHoleSelected() const
{
    return mMovingHoleCheckbox != NULL && mMovingHoleCheckbox->IsChecked();
}

bool ModesDialog::IsMaxPowerSelected() const
{
    return mMaxPowerCheckbox != NULL && mMaxPowerCheckbox->IsChecked();
}

bool ModesDialog::IsCombolessSelected() const
{
    return mCombolessCheckbox != NULL && mCombolessCheckbox->IsChecked();
}

bool ModesDialog::IsGapFreeSelected() const
{
    return mGapFreeCheckbox != NULL && mGapFreeCheckbox->IsChecked();
}

bool ModesDialog::IsChainCountSelected() const
{
    return mChainCountCheckbox != NULL && mChainCountCheckbox->IsChecked();
}

bool ModesDialog::IsBankruptSelected() const
{
    return mBankruptCheckbox != NULL && mBankruptCheckbox->IsChecked();
}

bool ModesDialog::IsColorShiftSelected() const
{
    return mColorShiftCheckbox != NULL && mColorShiftCheckbox->IsChecked();
}

bool ModesDialog::IsInvisibleSelected() const
{
    return mInvisibleCheckbox != NULL && mInvisibleCheckbox->IsChecked();
}

bool ModesDialog::IsBaseMinimumSelected() const
{
    return mBaseMinimumCheckbox != NULL && mBaseMinimumCheckbox->IsChecked();
}

bool ModesDialog::IsAutoAdvanceSelected() const
{
    return mAutoAdvanceCheckbox != NULL && mAutoAdvanceCheckbox->IsChecked();
}

bool ModesDialog::IsKillerBallSelected() const
{
    return mKillerBallCheckbox != NULL && mKillerBallCheckbox->IsChecked();
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
    if (mColorsBanCheckbox != NULL)
        mColorsBanCheckbox->SetChecked(selected, false);
    MarkDirty();
}

bool ModesDialog::GetColorsBanRandom() const
{
    return mPendingColorsBanRandom;
}

void ModesDialog::SetColorsBanRandom(bool random)
{
    mPendingColorsBanRandom = random;
}

int ModesDialog::GetColorsBanRandomCount() const
{
    return mPendingColorsBanRandomCount;
}

void ModesDialog::SetColorsBanRandomCount(int count)
{
    if (count < 1)
        count = 1;
    if (count > 5)
        count = 5;
    mPendingColorsBanRandomCount = count;
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
    if (mUnpoweredCheckbox != NULL)
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
    if (mSonicCheckbox != NULL)
        mSonicCheckbox->SetChecked(selected, false);
    MarkDirty();
}

int ModesDialog::GetMovingHoleSpeed() const
{
    return mPendingMovingHoleSpeed;
}

void ModesDialog::SetMovingHoleSpeed(int speed)
{
    mPendingMovingHoleSpeed = speed;
}

void ModesDialog::SetMovingHoleSelected(bool selected)
{
    if (mMovingHoleCheckbox != NULL)
        mMovingHoleCheckbox->SetChecked(selected, false);
    MarkDirty();
}

int ModesDialog::GetMaxPowerPercent() const
{
    return mPendingMaxPowerPercent;
}

void ModesDialog::SetMaxPowerPercent(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;
    mPendingMaxPowerPercent = percent;
}

bool ModesDialog::GetMaxPowerQuietSounds() const
{
    return mPendingMaxPowerQuietSounds;
}

void ModesDialog::SetMaxPowerQuietSounds(bool quiet)
{
    mPendingMaxPowerQuietSounds = quiet;
}

void ModesDialog::SetMaxPowerSelected(bool selected)
{
    if (mMaxPowerCheckbox != NULL)
        mMaxPowerCheckbox->SetChecked(selected, false);
    MarkDirty();
}

int ModesDialog::GetChainBonusThreshold() const
{
    return mPendingChainBonusThreshold;
}

void ModesDialog::SetChainBonusThreshold(int threshold)
{
    if (threshold < 1)
        threshold = 1;
    if (threshold > 20)
        threshold = 20;
    mPendingChainBonusThreshold = threshold;
}

bool ModesDialog::GetChainBonusDisabled() const
{
    return mPendingChainBonusDisabled;
}

void ModesDialog::SetChainBonusDisabled(bool disabled)
{
    mPendingChainBonusDisabled = disabled;
}

void ModesDialog::SetChainCountSelected(bool selected)
{
    if (mChainCountCheckbox != NULL)
        mChainCountCheckbox->SetChecked(selected, false);
    MarkDirty();
}

float ModesDialog::GetColorShiftHz() const
{
    return mPendingColorShiftHz;
}

void ModesDialog::SetColorShiftHz(float hz)
{
    if (hz < 1.0f)
        hz = 1.0f;
    if (hz > 10.0f)
        hz = 10.0f;
    mPendingColorShiftHz = hz;
}

void ModesDialog::GetColorShiftMap(int outMap[MAX_BALL_COLORS]) const
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        outMap[i] = mPendingColorShiftMap[i];
}

void ModesDialog::SetColorShiftMap(const int theMap[MAX_BALL_COLORS])
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        int dest = theMap[i];
        if (dest < 0 || dest >= MAX_BALL_COLORS)
            dest = (i + 1) % MAX_BALL_COLORS;
        mPendingColorShiftMap[i] = dest;
    }
}

void ModesDialog::GetColorShiftEnabled(bool outEnabled[MAX_BALL_COLORS]) const
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        outEnabled[i] = mPendingColorShiftEnabled[i];
}

void ModesDialog::SetColorShiftEnabled(const bool theEnabled[MAX_BALL_COLORS])
{
    if (theEnabled == NULL)
        return;
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        mPendingColorShiftEnabled[i] = theEnabled[i];
}

bool ModesDialog::GetColorShiftRandom() const
{
    return mPendingColorShiftRandom;
}

void ModesDialog::SetColorShiftRandom(bool random)
{
    mPendingColorShiftRandom = random;
}

void ModesDialog::SetColorShiftSelected(bool selected)
{
    if (mColorShiftCheckbox != NULL)
        mColorShiftCheckbox->SetChecked(selected, false);
    MarkDirty();
}

float ModesDialog::GetInvisibleDurationSec() const
{
    return mPendingInvisibleDurationSec;
}

void ModesDialog::SetInvisibleDurationSec(float sec)
{
    if (sec < 0.5f)
        sec = 0.5f;
    if (sec > 10.0f)
        sec = 10.0f;
    mPendingInvisibleDurationSec = sec;
}

float ModesDialog::GetInvisibleIntervalSec() const
{
    return mPendingInvisibleIntervalSec;
}

void ModesDialog::SetInvisibleIntervalSec(float sec)
{
    if (sec < 0.5f)
        sec = 0.5f;
    if (sec > 10.0f)
        sec = 10.0f;
    mPendingInvisibleIntervalSec = sec;
}

int ModesDialog::GetInvisiblePercent() const
{
    return mPendingInvisiblePercent;
}

void ModesDialog::SetInvisiblePercent(int percent)
{
    if (percent < 4)
        percent = 4;
    if (percent > 100)
        percent = 100;
    mPendingInvisiblePercent = (percent / 2) * 2;
    if (mPendingInvisiblePercent < 4)
        mPendingInvisiblePercent = 4;
}

void ModesDialog::SetInvisibleSelected(bool selected)
{
    if (mInvisibleCheckbox != NULL)
        mInvisibleCheckbox->SetChecked(selected, false);
    MarkDirty();
}

float ModesDialog::GetKillerBallIntervalSec() const
{
    return mPendingKillerBallIntervalSec;
}

void ModesDialog::SetKillerBallIntervalSec(float sec)
{
    if (sec < 2.0f)
        sec = 2.0f;
    if (sec > 15.0f)
        sec = 15.0f;
    mPendingKillerBallIntervalSec = sec;
}

float ModesDialog::GetKillerBallFlightSec() const
{
    return mPendingKillerBallFlightSec;
}

void ModesDialog::SetKillerBallFlightSec(float sec)
{
    if (sec < 2.0f)
        sec = 2.0f;
    if (sec > 10.0f)
        sec = 10.0f;
    mPendingKillerBallFlightSec = sec;
}

void ModesDialog::SetKillerBallSelected(bool selected)
{
    if (mKillerBallCheckbox != NULL)
        mKillerBallCheckbox->SetChecked(selected, false);
    MarkDirty();
}

void ModesDialog::Draw(Graphics *g)
{
    CircleDialog::Draw(g);
}
