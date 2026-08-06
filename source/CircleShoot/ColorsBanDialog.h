#ifndef __COLORSBANDIALOG_H__
#define __COLORSBANDIALOG_H__

#include "CircleDialog.h"
#include "CircleCommon.h"
#include <SexyAppFramework/CheckboxListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Checkbox;

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    class ColorsBanDialog : public CircleDialog, CheckboxListener
    {
    public:
        ColorsBanDialog(const bool initialBanned[MAX_BALL_COLORS]);
        virtual ~ColorsBanDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void CheckboxChecked(int theId, bool checked);
        virtual void Draw(Graphics *g);

        void GetBannedColors(bool outBanned[MAX_BALL_COLORS]) const;

        Checkbox *mColorCheckboxes[MAX_BALL_COLORS];

    private:
        int CountUncheckedColors() const;
    };
};

#endif
