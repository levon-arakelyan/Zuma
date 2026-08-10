#ifndef __COLORSHIFTTARGETDIALOG_H__
#define __COLORSHIFTTARGETDIALOG_H__

#include "CircleDialog.h"
#include "CircleCommon.h"
#include <SexyAppFramework/CheckboxListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Checkbox;

    ///////////////////////////////////////////////////////////////////////////////
    // Pick destination color for one Color Shift source (excludes the source).
    ///////////////////////////////////////////////////////////////////////////////
    class ColorShiftTargetDialog : public CircleDialog, CheckboxListener
    {
    public:
        ColorShiftTargetDialog(int theSrcColor, int initialDest);
        virtual ~ColorShiftTargetDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void CheckboxChecked(int theId, bool checked);
        virtual void Draw(Graphics *g);

        int GetSrcColor() const { return mSrcColor; }
        int GetSelectedDest() const { return mSelectedDest; }

        Checkbox *mDestCheckboxes[MAX_BALL_COLORS];
        int mSrcColor;
        int mSelectedDest;
    };
};

#endif
