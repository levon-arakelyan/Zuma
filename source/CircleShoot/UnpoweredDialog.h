#ifndef __UNPOWEREDDIALOG_H__
#define __UNPOWEREDDIALOG_H__

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
    class UnpoweredDialog : public CircleDialog, CheckboxListener
    {
    public:
        UnpoweredDialog(const bool initialDisabled[PowerType_Max]);
        virtual ~UnpoweredDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void CheckboxChecked(int theId, bool checked);
        virtual void Draw(Graphics *g);

        void GetDisabledPowerUps(bool outDisabled[PowerType_Max]) const;

        Checkbox *mPowerCheckboxes[PowerType_Max];
    };
};

#endif
