#ifndef __CONFIRMCONTINUEDIALOG_H__
#define __CONFIRMCONTINUEDIALOG_H__

#include "CircleDialog.h"

namespace Sexy
{
    class WidgetManager;
    class CircleButton;

    ///////////////////////////////////////////////////////////////////////////////
    // Continue / New Game with a full-width Cancel row underneath.
    ///////////////////////////////////////////////////////////////////////////////
    class ConfirmContinueDialog : public CircleDialog
    {
    public:
        ConfirmContinueDialog(const std::string &theText);
        virtual ~ConfirmContinueDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void ButtonDepress(int theId);

        CircleButton *mCancelButton;
    };
};

#endif
