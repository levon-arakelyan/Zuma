#include "Zuma_Prefix.pch"

#include <SexyAppFramework/DDInterface.h>
#include <SexyAppFramework/D3DInterface.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/Image.h>
#include <SexyAppFramework/MemoryImage.h>
#include <SexyAppFramework/SexyAppBase.h>
#include <SexyAppFramework/SexyMatrix.h>

#include "Bullet.h"
#include "Gun.h"
#include "CircleShootApp.h"
#include "CircleCommon.h"
#include "Res.h"
#include "DataSync.h"

#include <math.h>

using namespace Sexy;

Sexy::MemoryImage *gShooterTop = 0;

static const int kFrogSliceSize = 108;
static const int kFrogSliceHalf = 54;
static MemoryImage *gFrogSliceImage = NULL;
static MemoryImage *gNextBallCircle = NULL;
static MemoryImage *gWedgeMask[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static void RotateXY(float &x, float &y, float cx, float cy, float rad);

static int GetMultiFrogCount()
{
    CircleShootApp *app = GetCircleShootApp();
    if (app == NULL || !app->mMultiFrogMode)
        return 1;
    int n = app->mMultiFrogCount;
    if (n < 1)
        n = 1;
    if (n > 4)
        n = 4;
    return n;
}

static MemoryImage *GetWedgeMask(int theCount)
{
    if (theCount < 2 || theCount > 4)
        return NULL;
    if (gWedgeMask[theCount] != NULL)
        return gWedgeMask[theCount];

    MemoryImage *mask = new MemoryImage(gSexyAppBase);
    mask->Create(kFrogSliceSize, kFrogSliceSize);
    mask->SetImageMode(true, true);
    ulong *bits = mask->GetBits();
    float cx = (kFrogSliceSize - 1) * 0.5f;
    float cy = (kFrogSliceSize - 1) * 0.5f;
    float half = SEXY_PI / (float)theCount;
    float facing = SEXY_PI * 0.5f;
    for (int y = 0; y < kFrogSliceSize; y++)
    {
        for (int x = 0; x < kFrogSliceSize; x++)
        {
            float ox = (x + 0.5f) - cx;
            float oy = (y + 0.5f) - cy;
            float ang = (float)atan2(oy, ox);
            float diff = ang - facing;
            while (diff > SEXY_PI)
                diff -= SEXY_PI * 2.0f;
            while (diff < -SEXY_PI)
                diff += SEXY_PI * 2.0f;
            bits[y * kFrogSliceSize + x] = (fabs(diff) <= half + 0.02f) ? 0xFFFFFFFF : 0;
        }
    }
    mask->BitsChanged();
    gWedgeMask[theCount] = mask;
    return mask;
}

static void ApplyWedgeMask(MemoryImage *theImage, int theCount)
{
    MemoryImage *mask = GetWedgeMask(theCount);
    if (mask == NULL || theImage == NULL)
        return;

    ulong *ib = theImage->GetBits();
    ulong *mb = mask->GetBits();
    int count = kFrogSliceSize * kFrogSliceSize;
    for (int i = 0; i < count; i++)
    {
        if ((mb[i] & 0xFF000000) == 0)
            ib[i] = 0;
    }
    theImage->BitsChanged();
}

static MemoryImage *GetFrogSliceImage()
{
    if (gFrogSliceImage == NULL)
    {
        gFrogSliceImage = new MemoryImage(gSexyAppBase);
        gFrogSliceImage->Create(kFrogSliceSize, kFrogSliceSize);
        gFrogSliceImage->SetImageMode(true, true);
    }
    return gFrogSliceImage;
}

static MemoryImage *BuildNextBallCircle(int theType)
{
    int aColorWidth = Sexy::IMAGE_NEXT_BALL->mWidth / 6;
    int aColorHeight = Sexy::IMAGE_NEXT_BALL->mHeight;
    int size = aColorWidth;
    if (aColorHeight < size)
        size = aColorHeight;
    if (size < 2)
        size = 2;

    if (gNextBallCircle == NULL)
    {
        gNextBallCircle = new MemoryImage(gSexyAppBase);
        gNextBallCircle->Create(size, size);
        gNextBallCircle->SetImageMode(true, true);
    }
    else if (gNextBallCircle->mWidth != size || gNextBallCircle->mHeight != size)
    {
        gNextBallCircle->Create(size, size);
        gNextBallCircle->SetImageMode(true, true);
    }

    gNextBallCircle->Clear();

    int pivotX = 7;
    int pivotY = 32;
    if (pivotX < 0 || pivotX >= aColorWidth)
        pivotX = aColorWidth / 2;
    if (pivotY < 0 || pivotY >= aColorHeight)
        pivotY = aColorHeight / 2;

    Rect aSrc(theType * aColorWidth, 0, aColorWidth, aColorHeight);
    {
        Graphics sg(gNextBallCircle);
        sg.DrawImage(Sexy::IMAGE_NEXT_BALL, size / 2 - pivotX, size / 2 - pivotY, aSrc);
    }

    ulong *bits = gNextBallCircle->GetBits();
    float cx = (size - 1) * 0.5f;
    float cy = (size - 1) * 0.5f;
    float r = size * 0.5f;
    float r2 = r * r;
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            float ox = (x + 0.5f) - cx;
            float oy = (y + 0.5f) - cy;
            if (ox * ox + oy * oy > r2)
                bits[y * size + x] = 0;
        }
    }
    gNextBallCircle->BitsChanged();
    gNextBallCircle->SetImageMode(true, true);
    return gNextBallCircle;
}

Gun::Gun()
{
    mCenterX = 320;
    mCenterY = 240;
    mWidth = Sexy::IMAGE_SHOOTER_BOTTOM->GetWidth();
    mHeight = Sexy::IMAGE_SHOOTER_BOTTOM->GetHeight();

    if (!gShooterTop)
        gShooterTop = Sexy::CutoutImageFromAlpha(
            reinterpret_cast<MemoryImage *>(Sexy::IMAGE_SHOOTER_BOTTOM),
            reinterpret_cast<MemoryImage *>(Sexy::IMAGE_SHOOTER_TOP),
            31, 12);

    mAngle = 0.0f;
    mRecoilCount = 0;
    mBlinkCount = 0;
    mWink = 0;
    mFireVel = 6.0f;
    mState = GunState_Normal;
    mBullet = NULL;
    mNextBullet = NULL;
    mExtraBulletCount = 0;
    for (int i = 0; i < kMaxExtraFrogShots; i++)
        mExtraBullets[i] = NULL;
    mCachedGunImage = NULL;
    mCachedGunAngle = 0.0f;
    mShowNextBall = true;
}

Gun::~Gun()
{
    EmptyBullets();
}

void Gun::SyncState(DataSync &theSync)
{
    DataReader *aReader = theSync.mReader;
    DataWriter *aWriter = theSync.mWriter;

    if (aReader)
    {
        EmptyBullets();

        bool aHasBullet = aReader->ReadBool();
        if (aHasBullet)
        {
            mBullet = new Bullet();
            mBullet->SyncState(theSync);
        }

        bool aHasNextBullet = aReader->ReadBool();
        if (aHasNextBullet)
        {
            mNextBullet = new Bullet();
            mNextBullet->SyncState(theSync);
        }
    }
    else
    {
        aWriter->WriteBool(mBullet != NULL);
        if (mBullet)
            mBullet->SyncState(theSync);

        aWriter->WriteBool(mNextBullet != NULL);
        if (mNextBullet)
            mNextBullet->SyncState(theSync);
    }

    theSync.SyncFloat(mAngle);
    theSync.SyncShort(mCenterX);
    theSync.SyncShort(mCenterY);
    theSync.SyncShort(mRecoilX1);
    theSync.SyncShort(mRecoilY1);
    theSync.SyncShort(mRecoilX2);
    theSync.SyncShort(mRecoilY2);
    theSync.SyncShort(mRecoilCount);
    theSync.SyncFloat(mStatePercent);
    theSync.SyncFloat(mFireVel);
    theSync.SyncShort(mBlinkCount);
    theSync.SyncBool(mWink);
    theSync.SyncFloat(mCachedGunAngle);

    if (aReader)
    {
        theSync.SyncBytes(&mState, 4);
#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mState = (GunState)ByteSwap((unsigned int)GunState);
#endif
    }
    else
    {
#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mState = (GunState)ByteSwap((unsigned int)GunState);
#endif
        theSync.SyncBytes(&mState, 4);
    }

    if (aReader)
    {
        CreateCachedGunImage();
    }
}

bool Gun::NeedsReload()
{
    return mNextBullet == NULL || mBullet == NULL;
}

// void Gun::DeleteBullet()
// {
// }

void Gun::Reload(int theType, bool delay, PowerType thePower)
{
    Bullet *aBullet = new Bullet();
    aBullet->SetType(theType);
    aBullet->SetPowerType(thePower, false);

    // Fill only the next slot if the chamber already has a ball (e.g. after interrupting a shot).
    if (mBullet != NULL && mNextBullet == NULL)
    {
        mNextBullet = aBullet;
        return;
    }

    mStatePercent = 0.0f;
    if (mBullet)
        delete mBullet;

    mBullet = mNextBullet;
    mNextBullet = aBullet;
    mState = GunState_Reloading;

    if (!delay)
    {
        mState = GunState_Normal;
        mStatePercent = 1.0f;
    }

    CalcAngle();
}

void Gun::SetAngle(float theAngle)
{
    mAngle = theAngle;
    CalcAngle();
}

void Gun::DrawFrogLayers(Graphics *g, float theAngle, int theCX, int theCY, bool hasChamber, bool applyWink, bool drawBase, bool drawCover)
{
    int aCornerX = theCX - kFrogSliceHalf;
    int aCornerY = theCY - kFrogSliceHalf;

    if (drawBase)
    {
        g->DrawImageRotated(Sexy::IMAGE_SHOOTER_BOTTOM, aCornerX, aCornerY, theAngle, kFrogSliceHalf, kFrogSliceHalf);

        int aOff;
        switch (mState)
        {
        case GunState_Normal:
            aOff = 51;
            break;
        case GunState_Firing:
            aOff = (mStatePercent * 30.0f) + (1.0f - mStatePercent) * 51.0f;
            break;
        case GunState_Reloading:
            aOff = (mStatePercent * 51.0f) + (1.0f - mStatePercent) * 30.0f;
            break;
        }

        int aTongueSomething = 17;
        if (hasChamber)
            aTongueSomething = 54 - aOff;
        else
            aOff = 37;

        g->DrawImageRotated(Sexy::IMAGE_SHOOTER_TONGUE, aCornerX + 38, aCornerY + aOff, theAngle, 16, aTongueSomething);
    }

    if (!drawCover)
        return;

    g->DrawImageRotated(gShooterTop, aCornerX + 31, aCornerY + 12, theAngle, 23, 42);

    if (mBlinkCount == 0 || mBlinkCount > 24)
        return;

    int blink = 0;
    if (mBlinkCount > 4 && mBlinkCount < 20)
        blink = 1;

    int aWidth = Sexy::IMAGE_EYE_BLINK->mWidth;
    int aHeight = Sexy::IMAGE_EYE_BLINK->mHeight / 2;
    if (applyWink && mWink)
        mWidth /= 2;

    Rect anEyeRect(0, blink * aHeight, aWidth, aHeight);
    g->DrawImageRotatedF(Sexy::IMAGE_EYE_BLINK, aCornerX + 25, aCornerY + 42, theAngle, 29, 12, &anEyeRect);
}

void Gun::Draw(Graphics *g)
{
    CircleShootApp *app = GetCircleShootApp();
    int aFrogCount = GetMultiFrogCount();
    bool aUseWedge = aFrogCount > 1;

    if (!aUseWedge)
    {
        if (app != NULL && app->mBaseMinimumMode)
        {
            g->SetColor(Color(0x228B22));
            Sexy::FillCircle(g, mCenterX, mCenterY, 28);

            if (mBullet != NULL)
                mBullet->Draw(g);

            if (mShowNextBall && !app->mNoSwapMode && mNextBullet != NULL && mState != GunState_Reloading)
            {
                int aColorWidth = Sexy::IMAGE_NEXT_BALL->mWidth / 6;
                int aColorHeight = Sexy::IMAGE_NEXT_BALL->mHeight;
                int nextR = aColorWidth < aColorHeight ? aColorWidth / 2 : aColorHeight / 2;
                if (nextR < 1)
                    nextR = 1;

                float nx = (float)(mCenterX + 47 + aColorWidth / 2 - kFrogSliceHalf);
                float ny = (float)(mCenterY + 22 + aColorHeight / 2 - kFrogSliceHalf);
                RotateXY(nx, ny, (float)mCenterX, (float)mCenterY, mAngle);

                uint c = Sexy::gBallColors[mNextBullet->GetType()];
                g->SetColor(Color(c));
                Sexy::FillCircle(g, (int)(nx + 0.5f), (int)(ny + 0.5f), nextR);
            }
            return;
        }

        int aCornerX = mCenterX - kFrogSliceHalf;
        int aCornerY = mCenterY - kFrogSliceHalf;

        g->DrawImageRotated(Sexy::IMAGE_SHOOTER_BOTTOM, aCornerX, aCornerY, mAngle, kFrogSliceHalf, kFrogSliceHalf);

        int aOff;
        switch (mState)
        {
        case GunState_Normal:
            aOff = 51;
            break;
        case GunState_Firing:
            aOff = (mStatePercent * 30.0f) + (1.0f - mStatePercent) * 51.0f;
            break;
        case GunState_Reloading:
            aOff = (mStatePercent * 51.0f) + (1.0f - mStatePercent) * 30.0f;
            break;
        }

        int aTongueSomething = 17;
        if (mBullet != NULL)
            aTongueSomething = 54 - aOff;
        else
            aOff = 37;

        g->DrawImageRotated(Sexy::IMAGE_SHOOTER_TONGUE, aCornerX + 38, aCornerY + aOff, mAngle, 16, aTongueSomething);

        if (mBullet != NULL)
            mBullet->Draw(g);

        if (mShowNextBall && app != NULL && !app->mNoSwapMode &&
            mNextBullet != NULL && mState != GunState_Reloading)
        {
            int aColorWidth = Sexy::IMAGE_NEXT_BALL->mWidth / 6;
            int aColorHeight = Sexy::IMAGE_NEXT_BALL->mHeight;
            Rect aColorRect(mNextBullet->GetType() * aColorWidth, 0, aColorWidth, aColorHeight);
            g->DrawImageRotatedF(Sexy::IMAGE_NEXT_BALL, aCornerX + 47, aCornerY + 22, mAngle, 7, 32, &aColorRect);
        }

        g->DrawImageRotated(gShooterTop, aCornerX + 31, aCornerY + 12, mAngle, 23, 42);

        if (mBlinkCount == 0 || mBlinkCount > 24)
            return;

        int blink = 0;
        if (mBlinkCount > 4 && mBlinkCount < 20)
            blink = 1;

        int aWidth = Sexy::IMAGE_EYE_BLINK->mWidth;
        int aHeight = Sexy::IMAGE_EYE_BLINK->mHeight / 2;
        if (mWink)
            mWidth /= 2;

        Rect anEyeRect(0, blink * aHeight, aWidth, aHeight);
        g->DrawImageRotatedF(Sexy::IMAGE_EYE_BLINK, aCornerX + 25, aCornerY + 42, mAngle, 29, 12, &anEyeRect);
        return;
    }

    for (int aPass = 0; aPass < aFrogCount; aPass++)
    {
        // Draw extras first; the cursor-following frog (index 0) is last / on top.
        int aFrog = (aPass + 1) % aFrogCount;
        float aDelta = aFrog * ((SEXY_PI * 2.0f) / (float)aFrogCount);
        float aAngle = mAngle + aDelta;

        Bullet *aChamber = mBullet;
        bool aRestoreChamber = false;
        float aSavedX = 0.0f;
        float aSavedY = 0.0f;
        float aSavedRot = 0.0f;

        if (aFrog == 0)
        {
            aChamber = mBullet;
        }
        else if (aFrog - 1 < mExtraBulletCount && mExtraBullets[aFrog - 1] != NULL)
        {
            aChamber = mExtraBullets[aFrog - 1];
        }
        else if (mBullet != NULL)
        {
            aRestoreChamber = true;
            aSavedX = mBullet->GetX();
            aSavedY = mBullet->GetY();
            aSavedRot = mBullet->GetRotation();
            float aPosX = aSavedX;
            float aPosY = aSavedY;
            RotateXY(aPosX, aPosY, (float)mCenterX, (float)mCenterY, aDelta);
            mBullet->SetPos(aPosX, aPosY);
            mBullet->SetRotation(aSavedRot + aDelta);
            aChamber = mBullet;
        }
        else
        {
            aChamber = NULL;
        }

        MemoryImage *slice = GetFrogSliceImage();
        slice->Clear();
        {
            Graphics sg(slice);
            if (app != NULL && app->mBaseMinimumMode)
            {
                sg.SetColor(Color(0x228B22));
                Sexy::FillCircle(&sg, kFrogSliceHalf, kFrogSliceHalf, 28);
            }
            else
            {
                DrawFrogLayers(&sg, 0.0f, kFrogSliceHalf, kFrogSliceHalf, aChamber != NULL, false, true, false);
            }
        }
        ApplyWedgeMask(slice, aFrogCount);
        g->DrawImageRotated(slice, mCenterX - kFrogSliceHalf, mCenterY - kFrogSliceHalf,
                            aAngle, kFrogSliceHalf, kFrogSliceHalf);

        if (aChamber != NULL)
            aChamber->Draw(g);

        if (app == NULL || !app->mBaseMinimumMode)
        {
            slice->Clear();
            {
                Graphics sg(slice);
                DrawFrogLayers(&sg, 0.0f, kFrogSliceHalf, kFrogSliceHalf, aChamber != NULL, aFrog == 0, false, true);
            }
            ApplyWedgeMask(slice, aFrogCount);
            g->DrawImageRotated(slice, mCenterX - kFrogSliceHalf, mCenterY - kFrogSliceHalf,
                                aAngle, kFrogSliceHalf, kFrogSliceHalf);
        }

        if (aRestoreChamber)
        {
            mBullet->SetPos(aSavedX, aSavedY);
            mBullet->SetRotation(aSavedRot);
        }
    }

    if (mShowNextBall && app != NULL && !app->mNoSwapMode &&
        mNextBullet != NULL && mState != GunState_Reloading)
    {
        int aColorWidth = Sexy::IMAGE_NEXT_BALL->mWidth / 6;
        int aColorHeight = Sexy::IMAGE_NEXT_BALL->mHeight;

        if (app->mBaseMinimumMode)
        {
            int nextR = aColorWidth < aColorHeight ? aColorWidth / 2 : aColorHeight / 2;
            if (nextR < 1)
                nextR = 1;

            uint c = Sexy::gBallColors[mNextBullet->GetType()];
            g->SetColor(Color(c));
            Sexy::FillCircle(g, mCenterX, mCenterY, nextR);
        }
        else
        {
            MemoryImage *aCircle = BuildNextBallCircle(mNextBullet->GetType());
            int aHalf = aCircle->mWidth / 2;
            g->DrawImageRotatedF(aCircle,
                                 (float)(mCenterX - aHalf),
                                 (float)(mCenterY - aHalf),
                                 0.0f,
                                 (float)aHalf,
                                 (float)aHalf);
        }
    }
}

void Gun::DrawShadow(Graphics *g)
{
    // empty
}

void Gun::ForceFinishShot()
{
    if (mState != GunState_Firing || mBullet == NULL)
        return;

    mPendingFired.push_back(mBullet);
    mBullet = mNextBullet;
    mNextBullet = NULL;
    ReleaseExtraBullets();
    mState = GunState_Normal;
    mStatePercent = 1.0f;
}

bool Gun::StartFire(bool recoil)
{
    CircleShootApp *app = GetCircleShootApp();
    bool machineGun = app != NULL && app->mMachineGunMode;

    if (machineGun)
    {
        if (mState == GunState_Reloading)
        {
            mState = GunState_Normal;
            mStatePercent = 1.0f;
        }

        if (mState == GunState_Firing)
            ForceFinishShot();
    }
    else if (mState != GunState_Normal || mBullet == NULL)
    {
        return false;
    }

    if (mBullet == NULL)
        return false;

    mStatePercent = 0.0f;
    mState = GunState_Firing;
    mCenterX = mRecoilX1;
    mCenterY = mRecoilY1;

    Bullet *aBullet = mBullet;
    float rad = mAngle - (SEXY_PI / 2);
    float vx = cosf(rad);
    float vy = -sinf(rad);

    float speed = mFireVel;
    if (app != NULL && app->mShootSpeedMode && !app->mShootSpeedInstant)
    {
        float mult = app->mShootSpeedMultiplier;
        if (mult < 0.1f)
            mult = 0.1f;
        if (mult > 5.0f)
            mult = 5.0f;
        speed *= mult;
    }

    aBullet->SetVelocity(vx * speed, vy * speed);
    aBullet->SetPos(
        -40.0f * vx + aBullet->GetX(),
        -40.0f * vy + aBullet->GetY());

    ClearExtraBullets();
    int aFrogCount = GetMultiFrogCount();
    for (int i = 1; i < aFrogCount && i - 1 < kMaxExtraFrogShots; i++)
    {
        float aDelta = i * ((SEXY_PI * 2.0f) / (float)aFrogCount);
        mExtraBullets[mExtraBulletCount++] = CreateRotatedBullet(aBullet, aDelta);
    }

    mRecoilX1 = mCenterX;
    mRecoilY1 = mCenterY;
    mRecoilX2 = mCenterX + vx * -6.0f;
    mRecoilY2 = mCenterY + vy * -6.0f;

    if (recoil)
        mRecoilCount = 25;

    mWink = false;
    mBlinkCount = 25;

    CalcAngle();

    // Instant shoot-speed: release the shot this frame (no muzzle travel animation).
    if (app != NULL && app->mShootSpeedMode && app->mShootSpeedInstant)
        ForceFinishShot();

    return true;
}

Bullet *Gun::GetFiredBullet()
{
    if (!mPendingFired.empty())
    {
        Bullet *aBullet = mPendingFired.front();
        mPendingFired.pop_front();
        return aBullet;
    }

    if (mState == GunState_Firing && mStatePercent >= 1)
    {
        Bullet *aBullet = mBullet;
        mBullet = NULL;
        mState = GunState_Normal;

        ReleaseExtraBullets();

        return aBullet;
    }

    return NULL;
}

void Gun::SetPos(int theX, int theY)
{
    mCenterX = theX;
    mCenterY = theY;
    mRecoilX1 = theX;
    mRecoilY1 = theY;

    CalcAngle();
}

void Gun::SetSize(int theWidth, int theHeight)
{
    // educated guess, not in executable
    mWidth = theWidth;
    mHeight = theHeight;
}

void Gun::Update()
{
    if (mRecoilCount > 0)
    {
        mRecoilCount--;
        if (mRecoilCount <= 20)
        {
            if (mRecoilCount == 1)
            {
                mCenterX = mRecoilX1;
                mCenterY = mRecoilY1;
            }
            else
            {
                if (mRecoilCount > 14)
                {
                    mCenterX = (mRecoilCount - 16) * (mRecoilX1 - mRecoilX2) / 5 + mRecoilX2;
                    mCenterY = (mRecoilCount - 16) * (mRecoilY1 - mRecoilY2) / 5 + mRecoilY2;
                }
                else
                {
                    mCenterX = mRecoilX1 + mRecoilCount * (mRecoilX2 - mRecoilX1) / 15;
                    mCenterY = mRecoilY1 + mRecoilCount * (mRecoilY2 - mRecoilY1) / 15;
                }
            }
        }
    }

    if (mBlinkCount > 0)
        mBlinkCount--;

    if (mState == GunState_Firing)
    {
        mStatePercent += 0.15f;

        if (mStatePercent > 0.6f)
        {
            mBullet->Update();
            for (int i = 0; i < mExtraBulletCount; i++)
            {
                if (mExtraBullets[i] != NULL)
                    mExtraBullets[i]->Update();
            }
        }
    }
    else
    {
        mStatePercent += 0.07f;
    }

    if (mStatePercent > 1.0f)
    {
        mStatePercent = 1.0f;
        if (mState == GunState_Reloading)
            mState = GunState_Normal;
    }

    CalcAngle();
}

void Gun::SwapBullets(bool playSound)
{
    if (GetCircleShootApp()->mNoSwapMode)
        return;

    if (mState != GunState_Normal)
        return;

    if (mBullet == NULL)
        return;

    if (mNextBullet == NULL)
        return;

    if (mBullet->GetType() == mNextBullet->GetType())
        return;

    if (playSound)
    {
        gSexyAppBase->PlaySample(Sexy::SOUND_BUTTON2);
    }

    Bullet *aBullet = mBullet;
    mBullet = mNextBullet;
    mNextBullet = aBullet;

    CalcAngle();
}

void Gun::EmptyBullets()
{
    mState = GunState_Normal;

    while (!mPendingFired.empty())
    {
        delete mPendingFired.front();
        mPendingFired.pop_front();
    }

    if (mNextBullet != NULL)
    {
        delete mNextBullet;
        mNextBullet = NULL;
    }

    if (mBullet != NULL)
    {
        delete mBullet;
        mBullet = NULL;
    }

    ClearExtraBullets();
}

void Gun::CreateCachedGunImage()
{
    if (mCachedGunImage == NULL)
    {
        int *aSide = (mWidth < mHeight) ? &mHeight : &mWidth;

        int aSize = *aSide * 1.25;

        mCachedGunImage = new MemoryImage(gSexyAppBase);
        mCachedGunImage->Create(aSize, aSize);
    }

    mCachedGunImage->Clear();

    int aCenterX = mCenterX;
    int aCenterY = mCenterY;
    int aBlinkCount = mBlinkCount;
    float anAngle = mAngle;
    Bullet *aNextBullet = mNextBullet;
    Bullet *aBullet = mBullet;
    Bullet *aExtraBullets[kMaxExtraFrogShots];
    int aExtraCount = mExtraBulletCount;
    for (int i = 0; i < kMaxExtraFrogShots; i++)
    {
        aExtraBullets[i] = mExtraBullets[i];
        mExtraBullets[i] = NULL;
    }

    mNextBullet = NULL;
    mBullet = NULL;
    mExtraBulletCount = 0;
    mBlinkCount = 0;
    mCenterX = mCachedGunImage->mWidth / 2;
    mCenterY = mCachedGunImage->mHeight / 2;
    mRecoilX1 = mCachedGunImage->mWidth / 2;
    mRecoilY1 = mCachedGunImage->mHeight / 2;
    CalcAngle();

    if (gSexyAppBase->Is3DAccelerated())
    {
        mAngle = 0.0f;
        CalcAngle();
    }

    mCachedGunAngle = mAngle;

    Graphics g(mCachedGunImage);
    Draw(&g);

    mCenterX = aCenterX;
    mCenterY = aCenterY;
    mRecoilX1 = aCenterX;
    mRecoilY1 = aCenterY;
    CalcAngle();

    mAngle = anAngle;
    CalcAngle();

    mBlinkCount = aBlinkCount;
    mNextBullet = aNextBullet;
    mBullet = aBullet;
    mExtraBulletCount = aExtraCount;
    for (int i = 0; i < kMaxExtraFrogShots; i++)
        mExtraBullets[i] = aExtraBullets[i];
}

void Gun::DrawCachedGunImage(Graphics *g, float theZoom)
{
    int aWidth = mCachedGunImage->mWidth;
    int aCenterX = aWidth / 2;
    int aHeight = mCachedGunImage->mHeight;
    int aCenterY = aHeight / 2;

    Rect a4(0, 0, aWidth, aHeight);

    if (gSexyAppBase->Is3DAccelerated())
    {
        SexyTransform2D aTrans;
        aTrans.Translate(-aCenterX, -aCenterY);
        aTrans.Scale(theZoom, theZoom);
        // aTrans.RotateRad(mAngle);
        aTrans.Rotate(-mAngle);
        aTrans.Translate(mCenterX + ((theZoom - 1.0f) * -50.0f),
                         mCenterY + ((theZoom - 1.0f) * 100.0f));

        D3DInterface *aD3D = gSexyAppBase->mDDInterface->mD3DInterface;
        aD3D->BltTransformed(mCachedGunImage, &g->mClipRect, Color(0, 0, 0, 96), Graphics::DRAWMODE_NORMAL, a4, aTrans, true);

        aTrans.LoadIdentity();
        aTrans.Translate(-aCenterX, -aCenterY);
        aTrans.Scale(theZoom, theZoom);
        // aTrans.RotateRad(mAngle);
        aTrans.Rotate(-mAngle);
        aTrans.Translate(mCenterX, mCenterY);
        aD3D->BltTransformed(mCachedGunImage, &g->mClipRect, Color::White, Graphics::DRAWMODE_NORMAL, a4, aTrans, true);
    }
    else
    {
        Rect a3(mCenterX - (aCenterX * theZoom),
                mCenterY - (aCenterY * theZoom),
                (2 * aCenterX) * theZoom,
                (2 * aCenterY) * theZoom);

        bool isFastStretch = g->GetFastStretch();
        g->SetFastStretch(true);
        g->DrawImage(mCachedGunImage, a3, a4);
        g->SetFastStretch(isFastStretch);
    }
}

void Gun::SetBulletType(int theType)
{
    if (mBullet != NULL)
        mBullet->SetType(theType);
}

void Gun::SetNextBulletType(int theType)
{
    if (mNextBullet != NULL)
        mNextBullet->SetType(theType);
}

void Gun::DoBlink(bool wink)
{
    mWink = wink;
    mBlinkCount = 25;
}

void Gun::ClearExtraBullets()
{
    for (int i = 0; i < kMaxExtraFrogShots; i++)
    {
        if (mExtraBullets[i] != NULL)
        {
            delete mExtraBullets[i];
            mExtraBullets[i] = NULL;
        }
    }
    mExtraBulletCount = 0;
}

void Gun::ReleaseExtraBullets()
{
    for (int i = 0; i < mExtraBulletCount; i++)
    {
        if (mExtraBullets[i] != NULL)
        {
            mPendingFired.push_back(mExtraBullets[i]);
            mExtraBullets[i] = NULL;
        }
    }
    mExtraBulletCount = 0;
}

static void RotateXY(float &x, float &y, float cx, float cy, float rad)
{
    float ox = x - cx;
    float oy = y - cy;

    x = cx + ox * cosf(rad) + oy * sinf(rad);
    y = cy + oy * cosf(rad) - ox * sinf(rad);
}

Bullet *Gun::CreateRotatedBullet(Bullet *theSrc, float theAngleDelta)
{
    Bullet *aClone = new Bullet();
    aClone->SetType(theSrc->GetType());
    aClone->SetPowerType(theSrc->GetPowerType(), false);

    float aPosX = theSrc->GetX();
    float aPosY = theSrc->GetY();
    RotateXY(aPosX, aPosY, (float)mCenterX, (float)mCenterY, theAngleDelta);
    aClone->SetPos(aPosX, aPosY);
    aClone->SetRotation(theSrc->GetRotation() + theAngleDelta);

    float vx = theSrc->GetVelX();
    float vy = theSrc->GetVelY();
    float c = cosf(theAngleDelta);
    float s = sinf(theAngleDelta);
    aClone->SetVelocity(vx * c + vy * s, vy * c - vx * s);
    return aClone;
}

void Gun::CalcAngle()
{
    if (!mBullet)
        return;

    float start = mCenterY - 20;
    float end = mCenterY + 25;
    float aPointX = (mCenterX + 1);
    float aPointY = 0.0f;

    if (mState == GunState_Normal)
    {
        aPointY = end;
    }
    else if (mState == GunState_Reloading)
    {
        aPointY = start + (end - start) * mStatePercent;
    }
    else if (mStatePercent <= 0.6f)
    {
        aPointY = end + (((mCenterY + 10) - end) * mStatePercent) / 0.6f;
    }
    else
    {
        return;
    }

    RotateXY(aPointX, aPointY, mCenterX, mCenterY, mAngle);
    mBullet->SetPos(aPointX, aPointY);
    mBullet->SetRotation(mAngle);
}