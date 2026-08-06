#ifndef __MODESDIALOG_H__
#define __MODESDIALOG_H__

#include "CircleDialog.h"
#include "CircleCommon.h"
#include <SexyAppFramework/CheckboxListener.h>
#include <SexyAppFramework/Widget.h>
#include <string>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Checkbox;
    class ButtonWidget;

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
    class ModesDialog : public CircleDialog, CheckboxListener
    {
    public:
        ModesDialog();
        virtual ~ModesDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void CheckboxChecked(int theId, bool checked);
        virtual void ButtonMouseEnter(int theId);
        virtual void ButtonMouseLeave(int theId);
        virtual void ButtonMouseMove(int theId, int theX, int theY);
        virtual void ButtonDepress(int theId);
        virtual void Draw(Graphics *g);

        bool IsColorsBanSelected() const;
        bool IsUnpoweredSelected() const;
        bool IsNoSwapSelected() const;
        bool IsSonicSelected() const;

        void GetBannedColors(bool outBanned[MAX_BALL_COLORS]) const;
        void SetBannedColors(const bool banned[MAX_BALL_COLORS]);
        void SetColorsBanSelected(bool selected);

        void GetDisabledPowerUps(bool outDisabled[PowerType_Max]) const;
        void SetDisabledPowerUps(const bool disabled[PowerType_Max]);
        void SetUnpoweredSelected(bool selected);

        float GetChainSpeedMultiplier() const;
        void SetChainSpeedMultiplier(float multiplier);
        void SetSonicSelected(bool selected);

        void PrepareClose();

        Checkbox *mColorsBanCheckbox;
        Checkbox *mUnpoweredCheckbox;
        Checkbox *mNoSwapCheckbox;
        Checkbox *mSonicCheckbox;

        ButtonWidget *mColorsBanHit;
        ButtonWidget *mUnpoweredHit;
        ButtonWidget *mNoSwapHit;
        ButtonWidget *mSonicHit;

        ModeHelpTooltip *mTooltip;

        bool mPendingBannedColors[MAX_BALL_COLORS];
        bool mPendingDisabledPowerUps[PowerType_Max];
        float mPendingChainSpeedMultiplier;
        int mHoveredModeId;

    private:
        ButtonWidget *CreateModeHitArea(int theId);
        void LayoutModeCell(Checkbox *theCheckbox, ButtonWidget *theHit, const char *theLabel, int theX, int theY);
        void DrawModeRow(Graphics *g, Checkbox *theCheckbox, ButtonWidget *theHit, const char *theLabel);
        void ShowTooltip(const char *theText);
        void HideTooltip();
        void PositionTooltipNearCursor();
        const char *GetDescriptionForModeId(int theId) const;
        bool IsModeHitId(int theId) const;
    };
};

#endif
