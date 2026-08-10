#ifndef __MODESDIALOG_H__
#define __MODESDIALOG_H__

#include "CircleDialog.h"
#include "CircleCommon.h"
#include <SexyAppFramework/CheckboxListener.h>
#include <SexyAppFramework/ListListener.h>
#include <SexyAppFramework/ScrollListener.h>
#include <SexyAppFramework/Widget.h>
#include <string>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Checkbox;
    class ButtonWidget;
    class ListWidget;
    class ScrollbarWidget;
    class ModesDialog;

    ///////////////////////////////////////////////////////////////////////////////
    // Editable catalog enums (add groups/modes here; details live in ModesDialog.cpp)
    ///////////////////////////////////////////////////////////////////////////////
    namespace ModesCatalog
    {
        enum GroupId
        {
            Group_GameMechanics = 0,
            Group_ForFun,
            Group_Challenges,
            Group_Count
        };

        enum ModeId
        {
            Mode_ColorsBan = 0,
            Mode_Unpowered,
            Mode_NoSwap,
            Mode_Sonic,
            Mode_MachineGun,
            Mode_Bomber,
            Mode_UglyChain,
            Mode_MovingHole,
            Mode_MaxPower,
            Mode_Comboless,
            Mode_GapFree,
            Mode_ChainCount,
            Mode_Bankrupt,
            Mode_ColorShift,
            Mode_Invisible,
            Mode_Count
        };
    }

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    class ModeHelpTooltip : public Widget
    {
    public:
        ModeHelpTooltip();
        virtual void Draw(Graphics *g);

        std::string mText;
    };

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    class ModesPane : public Widget, public ScrollListener
    {
    public:
        ModesPane(ModesDialog *theDialog);
        virtual void Draw(Graphics *g);
        virtual void ScrollPosition(int theId, double thePosition);
        virtual void MouseWheel(int theDelta);

        ModesDialog *mDialog;
        int mScrollY;
        int mContentHeight;
    };

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    struct ModeWidgetSlot
    {
        Checkbox *mCheckbox;
        ButtonWidget *mHit;
    };

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    class ModesDialog : public CircleDialog, CheckboxListener, ListListener
    {
    public:
        ModesDialog();
        virtual ~ModesDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void CheckboxChecked(int theId, bool checked);
        virtual void ListClicked(int theId, int theIdx, int theClickCount);
        virtual void ButtonMouseEnter(int theId);
        virtual void ButtonMouseLeave(int theId);
        virtual void ButtonMouseMove(int theId, int theX, int theY);
        virtual void ButtonDepress(int theId);
        virtual void Draw(Graphics *g);

        bool IsColorsBanSelected() const;
        bool IsUnpoweredSelected() const;
        bool IsNoSwapSelected() const;
        bool IsSonicSelected() const;
        bool IsMachineGunSelected() const;
        bool IsBomberSelected() const;
        bool IsUglyChainSelected() const;
        bool IsMovingHoleSelected() const;
        bool IsMaxPowerSelected() const;
        bool IsCombolessSelected() const;
        bool IsGapFreeSelected() const;
        bool IsChainCountSelected() const;
        bool IsBankruptSelected() const;
        bool IsColorShiftSelected() const;
        bool IsInvisibleSelected() const;

        void GetBannedColors(bool outBanned[MAX_BALL_COLORS]) const;
        void SetBannedColors(const bool banned[MAX_BALL_COLORS]);
        void SetColorsBanSelected(bool selected);

        void GetDisabledPowerUps(bool outDisabled[PowerType_Max]) const;
        void SetDisabledPowerUps(const bool disabled[PowerType_Max]);
        void SetUnpoweredSelected(bool selected);

        float GetChainSpeedMultiplier() const;
        void SetChainSpeedMultiplier(float multiplier);
        void SetSonicSelected(bool selected);

        int GetMovingHoleSpeed() const;
        void SetMovingHoleSpeed(int speed);
        void SetMovingHoleSelected(bool selected);

        int GetMaxPowerPercent() const;
        void SetMaxPowerPercent(int percent);
        bool GetMaxPowerQuietSounds() const;
        void SetMaxPowerQuietSounds(bool quiet);
        void SetMaxPowerSelected(bool selected);

        int GetChainBonusThreshold() const;
        void SetChainBonusThreshold(int threshold);
        bool GetChainBonusDisabled() const;
        void SetChainBonusDisabled(bool disabled);
        void SetChainCountSelected(bool selected);

        float GetColorShiftHz() const;
        void SetColorShiftHz(float hz);
        void GetColorShiftMap(int outMap[MAX_BALL_COLORS]) const;
        void SetColorShiftMap(const int theMap[MAX_BALL_COLORS]);
        void GetColorShiftEnabled(bool outEnabled[MAX_BALL_COLORS]) const;
        void SetColorShiftEnabled(const bool theEnabled[MAX_BALL_COLORS]);
        bool GetColorShiftRandom() const;
        void SetColorShiftRandom(bool random);
        void SetColorShiftSelected(bool selected);

        float GetInvisibleDurationSec() const;
        void SetInvisibleDurationSec(float sec);
        float GetInvisibleIntervalSec() const;
        void SetInvisibleIntervalSec(float sec);
        int GetInvisiblePercent() const;
        void SetInvisiblePercent(int percent);
        void SetInvisibleSelected(bool selected);

        void PrepareClose();

        // Named aliases kept for CircleShootApp / Finish* helpers
        Checkbox *mColorsBanCheckbox;
        Checkbox *mUnpoweredCheckbox;
        Checkbox *mNoSwapCheckbox;
        Checkbox *mSonicCheckbox;
        Checkbox *mMachineGunCheckbox;
        Checkbox *mBomberCheckbox;
        Checkbox *mUglyChainCheckbox;
        Checkbox *mMovingHoleCheckbox;
        Checkbox *mMaxPowerCheckbox;
        Checkbox *mCombolessCheckbox;
        Checkbox *mGapFreeCheckbox;
        Checkbox *mChainCountCheckbox;
        Checkbox *mBankruptCheckbox;
        Checkbox *mColorShiftCheckbox;
        Checkbox *mInvisibleCheckbox;

        ModeWidgetSlot mModeSlots[ModesCatalog::Mode_Count];

        ListWidget *mGroupList;
        ScrollbarWidget *mGroupScrollbar;
        ModesPane *mModesPane;
        ScrollbarWidget *mModesScrollbar;
        ModeHelpTooltip *mTooltip;

        bool mPendingBannedColors[MAX_BALL_COLORS];
        bool mPendingDisabledPowerUps[PowerType_Max];
        float mPendingChainSpeedMultiplier;
        int mPendingMovingHoleSpeed;
        int mPendingMaxPowerPercent;
        bool mPendingMaxPowerQuietSounds;
        int mPendingChainBonusThreshold;
        bool mPendingChainBonusDisabled;
        float mPendingColorShiftHz;
        int mPendingColorShiftMap[MAX_BALL_COLORS];
        bool mPendingColorShiftRandom;
        bool mPendingColorShiftEnabled[MAX_BALL_COLORS];
        float mPendingInvisibleDurationSec;
        float mPendingInvisibleIntervalSec;
        int mPendingInvisiblePercent;
        int mHoveredHitId;
        int mSelectedGroupIndex;
        int mDividerX;

        void LayoutModeWidgets();
        void DrawModeLabels(Graphics *g);

    private:
        ButtonWidget *CreateModeHitArea(int theId);
        void OpenModePicker(ModesCatalog::ModeId theId);
        void SelectGroup(int theIndex);
        void UpdateModesScrollbar();
        void ShowTooltip(const char *theText);
        void HideTooltip();
        void PositionTooltipNearCursor();
        const char *GetDescriptionForHitId(int theId) const;
        bool IsModeHitId(int theId) const;
        ModesCatalog::ModeId ModeIdFromHitId(int theId) const;
        Checkbox *CheckboxForMode(ModesCatalog::ModeId theId) const;
    };
};

#endif
