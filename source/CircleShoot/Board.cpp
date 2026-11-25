#include "Zuma_Prefix.pch"

#include <SexyAppFramework/Common.h>
#include <SexyAppFramework/Debug.h>
#include <SexyAppFramework/SexyAppBase.h>
#include <SexyAppFramework/ButtonWidget.h>
#include <SexyAppFramework/ButtonListener.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/MemoryImage.h>
#include <SexyAppFramework/KeyCodes.h>
#include <SexyAppFramework/SoundManager.h>
#include <SexyAppFramework/SoundInstance.h>
#include <SexyAppFramework/MusicInterface.h>

#include "Ball.h"
#include "Board.h"
#include "Bullet.h"
#include "CircleShootApp.h"
#include "CircleButton.h"
#include "CircleCommon.h"
#include "CurveMgr.h"
#include "DataSync.h"
#include "LevelParser.h"
#include "Gun.h"
#include "HighScoreMgr.h"
#include "ParticleMgr.h"
#include "ProfileMgr.h"
#include "SoundMgr.h"
#include "SpriteMgr.h"
#include "StatsDialog.h"
#include "TransitionMgr.h"
#include "Res.h"
#include "WayPoint.h"
#include "WorkerThread.h"

using namespace Sexy;

class BoardOverlay : public Widget
{
public:
    BoardOverlay(Board *board) : Widget()
    {
        mBoard = board;
        mMouseVisible = false;
        mHasTransparencies = true;
        mHasAlpha = true;
    }

    virtual void Draw(Graphics *g)
    {
        mBoard->DrawOverlay(g);
    }

    Board *mBoard;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

GameStats::GameStats()
{
    mTimePlayed = 0;
    mNumBallsCleared = 0;
    mNumGemsCleared = 0;
    mNumGaps = 0;
    mNumCombos = 0;
    mMaxCombo = -1;
    mMaxComboScore = 0;
    mMaxInARow = 0;
    mMaxInARowScore = 0;
}

void GameStats::Reset()
{
    mTimePlayed = 0;
    mNumBallsCleared = 0;
    mNumGemsCleared = 0;
    mNumGaps = 0;
    mNumCombos = 0;
    mMaxCombo = -1;
    mMaxComboScore = 0;
    mMaxInARow = 0;
    mMaxInARowScore = 0;
}

void GameStats::Add(const GameStats &theStats)
{
    mTimePlayed += theStats.mTimePlayed;
    mNumBallsCleared += theStats.mNumBallsCleared;
    mNumGemsCleared += theStats.mNumGemsCleared;
    mNumCombos += theStats.mNumCombos;
    mNumGaps += theStats.mNumGaps;

    if (theStats.mMaxCombo > mMaxCombo || theStats.mMaxCombo == mMaxCombo && theStats.mMaxComboScore > mMaxComboScore)
    {
        mMaxCombo = theStats.mMaxCombo;
        mMaxComboScore = theStats.mMaxComboScore;
    }

    if (theStats.mMaxInARow > this->mMaxInARow)
    {
        mMaxInARow = theStats.mMaxInARow;
        mMaxInARowScore = theStats.mMaxInARowScore;
    }
}

void GameStats::SyncState(DataSync &theSync)
{
    theSync.SyncLong(mTimePlayed);
    theSync.SyncLong(mNumBallsCleared);
    theSync.SyncLong(mNumGemsCleared);
    theSync.SyncLong(mNumGaps);
    theSync.SyncLong(mNumCombos);
    theSync.SyncLong(mMaxCombo);
    theSync.SyncLong(mMaxComboScore);
    theSync.SyncLong(mMaxInARow);
    theSync.SyncLong(mMaxInARowScore);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Board::Board(CircleShootApp *theApp)
{
    mApp = theApp;
    mGun = new Gun();

    mMenuButton = new ButtonWidget(0, this);
    mContinueButton = MakeButton(1, this, "CONTINUE", CircleButton::CB_ClickSound, NULL, 3);
    mContinueButton->SetVisible(false);

    mOverlayWidget = new BoardOverlay(this);

    mMenuButton->mButtonImage = Sexy::IMAGE_MENU_BUTTON;
    Sexy::Image *menuImage = mMenuButton->mButtonImage;
    int menuWidth = menuImage->mWidth / 3;
    int menuHeight = menuImage->mHeight;
    mMenuButton->mNormalRect = Rect(0, 0, menuWidth, menuHeight);
    mMenuButton->mOverRect = Rect(menuWidth, 0, menuWidth, menuHeight);
    mMenuButton->mDownRect = Rect(menuWidth * 2, 0, menuWidth, menuHeight);

    mMenuButton->Resize(540, 3, menuWidth, menuHeight);
    mContinueButton->Resize(410, 450 - mContinueButton->mHeight, 200, mContinueButton->mHeight);

    mOverlayWidget->Resize(0, 0, CIRCLE_WINDOW_WIDTH, CIRCLE_WINDOW_HEIGHT);

    mScore = 0;
    mScoreDisplay = 0;
    mLevel = 0;
    mPauseCount = 0;
    mDialogCount = 0;
    mPauseUpdateCnt = 0;
    mLastBallClickTick = 0;
    mLastExplosionTick = 0;
    mStartLevel = 0;
    mLevelBeginScore = 0;
    mLives = 3;
    mDifficultyDiff = 0;
    mFlashCount = 30;
    mLastPauseTick = 0;
    mMaxStage = 0;
    mGameState = GameState_None;
    mSlowMo = 0;
    mDoGuide = 0;
    mIsSavedGame = false;
    mShowBallsDuringPause = false;
    mShowParTimer = false;

    for (int i = 0; i < MAX_CURVES; i++)
    {
        mCurveMgr[i] = new CurveMgr(this);
        mNextCurveMgr[i] = new CurveMgr(this);
    }

    mSpriteMgr = new SpriteMgr();
    mNextSpriteMgr = new SpriteMgr();
    mParticleMgr = new ParticleMgr(this);
    mTransitionMgr = new TransitionMgr(this);
    mSoundMgr = new SoundMgr();
    mLevelDesc = new LevelDesc();
    mNextLevelDesc = new LevelDesc();
    mIsEndless = false;
    mKillerBallCooldown = 0;
    mKillerHitFrog = false;
    mLevelSwapCooldown = -1;
    mLevelSwapTransition = 0;
    mMissileStrikeCooldown = 0;
    mMissileStrikeSelecting = false;
    mStrikeMouseX = 0;
    mStrikeMouseY = 0;
    mMissileStrikeArmedBullet = NULL;
    mMissileStrikeArmedOldPower = PowerType_None;
    mStrikeShakeTicks = 0;
    mMultiOwnLevelDesc = NULL;
    mStoppedAdding = false;
    mMultiCount = 1;
    mMultiLoaded = 0;
    mMultiView = 0;
    mMultiSwitchTicks = 0;
    mMultiSwitchTarget = 0;
    mMultiBackground = false;
    mMultiEndRequest = -1;
    for (int i = 0; i < MAX_MULTI_GAMES; i++)
        mMultiDanger[i] = false;
}

Board::~Board()
{
    WaitForLoadingThread();
    MultiDestroyExtras();
    DeleteBullets();
    for (int i = 0; i < MAX_CURVES; i++)
    {
        delete mCurveMgr[i];
        delete mNextCurveMgr[i];
    }
    delete mSpriteMgr;
    delete mNextSpriteMgr;
    delete mParticleMgr;
    delete mTransitionMgr;
    delete mSoundMgr;
    delete mGun;
    delete mLevelDesc;
    delete mNextLevelDesc;
    delete mMultiOwnLevelDesc;
    delete mMenuButton;
    delete mContinueButton;
    delete mOverlayWidget;
}

void Board::StaticLoadProc(void *theData)
{
    ((Board *)theData)->LoadProc();
}

void Board::LoadProc()
{
    if (mLoadingThreadParam == 0)
    {
        Sexy::MirrorPoint(mNextLevelDesc->mGunX, mNextLevelDesc->mGunY, MirrorType_None);

        for (TreasurePointList::iterator anItr = mNextLevelDesc->mTreasurePoints.begin();
             anItr != mNextLevelDesc->mTreasurePoints.end();
             anItr++)
        {
            Sexy::MirrorPoint(anItr->x, anItr->y, MirrorType_None);
        }

        mNextSpriteMgr->SetupLevel(*mNextLevelDesc, MirrorType_None);

        for (int i = 0; i < 3; i++)
        {
            mNextCurveMgr[i]->SetupLevel(mNextLevelDesc, mNextSpriteMgr, i, MirrorType_None);
        }

        CurveDrawer aCurveDrawer(mSpriteMgr);
        if (!mApp->mBaseMinimumMode)
        {
            for (int i = 0; i < 3; i++)
            {
                if (mNextLevelDesc->mCurveDesc[i].mDrawCurve && mNextLevelDesc->mCurveDesc[i].mDrawTunnels)
                    mNextCurveMgr[i]->DrawTunnel(aCurveDrawer);
            }

            for (int i = 0; i < 3; i++)
            {
                if (mNextLevelDesc->mCurveDesc[i].mDrawCurve)
                    mNextCurveMgr[i]->DrawCurve(aCurveDrawer);
            }

            aCurveDrawer.AddCurvesToMgr();
        }
        CreateThumbnail(mNextSpriteMgr, mNextLevelDesc);
    }
    else if (mLoadingThreadParam == 1)
    {
        LevelDesc *aLevelDesc = mLevelDesc;
        if (aLevelDesc->mBackgroundAlpha.empty())
            aLevelDesc = &mApp->mLevelParser->GetBackgroundTransition();

        mSpriteMgr->GenerateBackgroundTransitionSprites(*aLevelDesc);
    }
}

void Board::DoAccuracy(bool accuracy)
{
    mRecalcGuide = accuracy;
    mShowGuide = accuracy;
    mDoGuide = accuracy;

    if (accuracy)
    {
        mGun->SetFireSpeed(15.0f);
    }
    else
    {
        mAccuracyCount = 0;
        mGun->SetFireSpeed(mLevelDesc->mFireSpeed);
    }
}

void Board::PlayBallClick(int theSound)
{
    int aTick = Sexy::BoardGetTickCount();
    if (aTick - mLastBallClickTick >= 250)
    {
        mApp->PlaySample(theSound);
        mLastBallClickTick = aTick;
    }
}

void Board::StartLevel()
{
    mGameState = GameState_Playing;
    mStateCount = 0;
    mSpriteMgr->ClearHoleFlashes();
    mParticleMgr->Clear();
    mTransitionMgr->Clear();
    mGun->SetFireSpeed(mLevelDesc->mFireSpeed);
    mGun->SetPos(mLevelDesc->mGunX, mLevelDesc->mGunY);
    mSoundMgr->PlayLoop(LoopType_RollIn);
    mLevelBeginning = true;
    mKillerHitFrog = false;
    SetMissileStrikeSelecting(false);
    mMissileStrikeCooldown = 0; // strike button is ready at every level start

    // First killer after one full interval from level start.
    if (mApp->mKillerBallMode)
    {
        float intervalSec = mApp->mKillerBallIntervalSec;
        if (intervalSec < 2.0f)
            intervalSec = 2.0f;
        if (intervalSec > 15.0f)
            intervalSec = 15.0f;
        mKillerBallCooldown = (int)(intervalSec * 100.0f + 0.5f);
        if (mKillerBallCooldown < 1)
            mKillerBallCooldown = 1;
    }
    else
    {
        mKillerBallCooldown = 0;
    }

    ClearLevelSwap();
    if (mApp->mLevelSwapMode)
        mLevelSwapCooldown = LevelSwapIntervalTicks();
    else
        mLevelSwapCooldown = -1;

    MultiStartLevelCopies();
}

void Board::SetLosing()
{
    mApp->mDoPlayCount = true;
    mSoundMgr->StopLoop(LoopType_RollIn);

    ResetInARowBonus();
    mLevelStats.mTimePlayed = mStateCount;
    mGameStats.Add(mLevelStats);
    mLevelStats.Reset();
    ClearLevelSwap();
    DeleteBullets();

    if (!mGun->StartFire(false))
    {
        mGun->EmptyBullets();
    }

    for (int i = 0; i < mNumCurves; i++)
    {
        mCurveMgr[i]->SetLosing();
    }

    mSoundMgr->PlayLoop(LoopType_RollOut);
    mGameState = GameState_Losing;
    mStateCount = 0;
    mLives = mLives - 1;

    if (mLives <= 0)
    {
        mApp->PlaySong(39, false, 0.01);
        CheckHighScore();
    }

    mTransitionMgr->DoLosing();
}

void Board::DoLevelUp(bool playSounds, bool isCheat)
{
    ClearLevelSwap();
    SetMissileStrikeSelecting(false);
    mMissileStrikeCooldown = 0; // strike button is ready again once the level is cleared
    mApp->mDoPlayCount = true;
    if (mPracticeBoard.empty())
    {
        if (mNextLevelDesc->mStage > mApp->mProfile->mMaxStage)
        {
            mApp->mProfile->mMaxStage = mNextLevelDesc->mStage;
            mApp->SaveProfile();
        }
    }
    else
    {
        mApp->mProfile->UpdateMaxLevel(mOriginalPracticeBoard, mLevel + 1);
        mApp->SaveProfile();
    }

    if (mIsEndless)
    {
        int aTreasure = (mCurTreasure != NULL) ? mCurTreasureNum : -1;
        mLevel++;

        GetLevel(mLevel, mLevelDesc, mLevelDesc->mName.c_str());

        if (aTreasure >= 0)
        {
            TreasurePointList::iterator anItr = mLevelDesc->mTreasurePoints.begin();
            std::advance(anItr, mCurTreasureNum);

            mCurTreasure = &*anItr;
        }

        mLevelString = Sexy::StrFormat("%s %d", gSmallGauntletStages[mLevelDesc->mStage], mLevelDesc->mLevel + 1);
        mVerboseLevelString = Sexy::StrFormat("%s %d", gGauntletStages[mLevelDesc->mStage], mLevelDesc->mLevel + 1);

        int aTextWidth = Sexy::FONT_HUGE->StringWidth(mVerboseLevelString);

        mParticleMgr->AddFloatingText(600 - aTextWidth, 440, 0xFFFFFF, mVerboseLevelString, Sexy::FONT_HUGE_ID, 0, 0, 150, true);

        for (int i = 0; i < mNumCurves; i++)
        {
            mCurveMgr[i]->DoEndlessZumaEffect();
            mCurveMgr[i]->SetupLevelDesc(mLevelDesc);
            IncScore(100 * (100 - mCurveMgr[i]->GetFarthestBallPercent()));
        }

        mLevelBeginScore = mScore;
        mScoreTarget = mScore + 1250;
        return;
    }

    mSoundMgr->StopLoop(LoopType_RollIn);
    mTargetBarSize = 256;
    mCurBarSize = 256;

    if (!isCheat)
    {
        std::string aLvlString = GetUniqueLevelString();
        mApp->mHighScoreMgr->SubmitLowTime(aLvlString, mApp->mProfile->mName, mStateCount / 100);
    }

    if (mNumClearsInARow > mLevelStats.mMaxInARow)
    {
        mLevelStats.mMaxInARow = mNumClearsInARow;
        mLevelStats.mMaxInARowScore = mCurInARowBonus;
    }

    ResetInARowBonus();
    mLevelStats.mTimePlayed = mStateCount;
    mGameStats.Add(mLevelStats);

    mApp->PlaySong(38, false, 0.01);
    DoAccuracy(false);
    WaitForLoadingThread();

    if (mPracticeMode != PracticeMode_Single)
    {
        DoGenerateBackgroundTransitions();
    }

    for (int i = 0; i < mNumCurves; i++)
    {
        mCurveMgr[i]->DeleteBalls();
    }

    DeleteBullets();

    if (!mGun->StartFire(false))
        mGun->EmptyBullets();

    mGun->CreateCachedGunImage();
    mGameState = GameState_LevelUp;
    mTransitionMgr->DoLevelUp();
    mStateCount = 0;

    MarkDirty();
}

void Board::CheckEndConditions()
{
    if (mGameState != GameState_Playing)
        return;

    // Multi game: a copy updating in the background only reports a loss or
    // win; MultiUpdatePlaying then shows that copy and acts on it.
    if (mMultiBackground)
    {
        MultiCheckBackgroundEnd();
        return;
    }

    // Apply after the bullet loop so SetLosing cannot invalidate those iterators.
    if (mKillerHitFrog)
    {
        mKillerHitFrog = false;
        SetLosing();
        return;
    }

    int i;
    int mNumCurves = this->mNumCurves;

    // Danger music must update even while bullets are in flight / gun is firing.
    // Otherwise moving-hole danger (and frantic shooting near the hole) never switches the track.
    for (i = 0; i < mNumCurves; i++)
    {
        if (mCurveMgr[i]->IsInDanger())
            break;
    }
    mApp->SwitchSong((i == mNumCurves) ? 0 : 36);

    // Moving hole snaps to the lead when it reaches the chain (UpdateMovingHole) and
    // does not crawl through balls while we wait. Lose still goes through IsLosing so
    // in-flight shots, clears, and chain-reaction sucks can still save the player.

    // Player shots in flight can still save a lose or finish a clear; killer balls
    // must not block level completion when the chains are already empty.
    bool hasPlayerBullet = false;
    for (BulletList::iterator anItr = mBulletList.begin(); anItr != mBulletList.end(); ++anItr)
    {
        if (!(*anItr)->IsKillerBall())
        {
            hasPlayerBullet = true;
            break;
        }
    }
    if (hasPlayerBullet || mGun->IsFiring())
        return;

    for (i = 0; i < mNumCurves; i++)
    {
        if (!mCurveMgr[i]->IsWinning())
        {
            break;
        }
    }

    if (i == mNumCurves)
    {
        // Drop any leftover killers so they cannot hit the frog after the win.
        for (BulletList::iterator anItr = mBulletList.begin(); anItr != mBulletList.end();)
        {
            if ((*anItr)->IsKillerBall())
            {
                int aType = (*anItr)->GetType();
                delete *anItr;
                anItr = mBulletList.erase(anItr);
                ReleaseKillerBallColor(aType);
            }
            else
            {
                ++anItr;
            }
        }

        DoLevelUp(true, false);
        return;
    }

    for (i = 0; i < mNumCurves; i++)
    {
        if (mCurveMgr[i]->IsLosing())
        {
            break;
        }
    }

    if (i != mNumCurves)
    {
        SetLosing();
        return;
    }
}

void Board::SyncPracticeMode()
{
    if (mPracticeBoard.empty())
    {
        mPracticeMode = PracticeMode_None;
    }
    else if (mPracticeBoard == "random")
    {
        mPracticeMode = PracticeMode_Random;
    }
    else if (mPracticeBoard == "sequence")
    {
        mPracticeMode = PracticeMode_Sequence;
    }
    else
    {
        mPracticeMode = PracticeMode_Single;
    }
}

// Missile strike tuning.
namespace
{
    const float kMissileStrikeBombScale = 1.3f;    // blast reach vs. the Bomb power-up
    const int kMissileStrikeCooldownTicks = 2500;  // 25 s button recharge
    const int kStrikeHoleRadius = 40;              // clear circle in the overlay (matches the white ring)
    const int kStrikeShakeTicks = 100;              // 0.4 s earthquake after a hit
    const float kStrikeShakeAmplitude = 11.0f;      // px at the start, fades to 0
}

void Board::AdvanceFreeBullet(BulletList::iterator &theBulletItr)
{
    Bullet *aBullet = *theBulletItr;

    if (aBullet->IsKillerBall())
    {
        aBullet->Update();

        float dx = aBullet->GetX() - (float)mGun->GetCenterX();
        float dy = aBullet->GetY() - (float)mGun->GetCenterY();
        float hitR = (float)(aBullet->GetRadius() + 36);
        if (dx * dx + dy * dy <= hitR * hitR)
        {
            int aType = aBullet->GetType();
            float aX = aBullet->GetX();
            float aY = aBullet->GetY();
            delete aBullet;
            theBulletItr = mBulletList.erase(theBulletItr);
            PlayKillerExplosion(aX, aY, aType);
            ReleaseKillerBallColor(aType);
            mKillerHitFrog = true;
            return;
        }

        if (aBullet->GetX() >= 0.0f && aBullet->GetY() >= 0.0f &&
            (aBullet->GetX() - aBullet->GetRadius()) < mWidth &&
            (aBullet->GetY() - aBullet->GetRadius()) < mHeight)
        {
            ++theBulletItr;
        }
        else
        {
            int aType = aBullet->GetType();
            delete aBullet;
            theBulletItr = mBulletList.erase(theBulletItr);
            ReleaseKillerBallColor(aType);
        }
        return;
    }

    if (mApp->mShootSpeedMode && mApp->mShootSpeedInstant)
    {
        float vx = aBullet->GetVelX();
        float vy = aBullet->GetVelY();
        float mag = (float)sqrt((double)(vx * vx + vy * vy));
        if (mag < 0.001f)
            mag = 1.0f;

        // Small steps so we cannot skip over chain balls in one update.
        const float kStep = 4.0f;
        aBullet->SetVelocity((vx / mag) * kStep, (vy / mag) * kStep);

        for (int i = 0; i < 500; i++)
        {
            if (AdvancePlayerBulletStep(theBulletItr))
                return;
        }

        ResetInARowBonus();
        delete *theBulletItr;
        theBulletItr = mBulletList.erase(theBulletItr);
        return;
    }

    if (mApp->mShootSpeedMode)
    {
        float vx = aBullet->GetVelX();
        float vy = aBullet->GetVelY();
        float mag = (float)sqrt((double)(vx * vx + vy * vy));
        const float kStep = 4.0f;
        if (mag > kStep)
        {
            int steps = (int)(mag / kStep + 0.999f);
            if (steps < 1)
                steps = 1;
            if (steps > 500)
                steps = 500;

            aBullet->SetVelocity((vx / mag) * kStep, (vy / mag) * kStep);
            for (int i = 0; i < steps; i++)
            {
                if (AdvancePlayerBulletStep(theBulletItr))
                    return;
            }

            aBullet->SetVelocity(vx, vy);
            ++theBulletItr;
            return;
        }
    }

    if (AdvancePlayerBulletStep(theBulletItr))
        return;

    ++theBulletItr;
}

bool Board::AdvancePlayerBulletStep(BulletList::iterator &theBulletItr)
{
    Bullet *aBullet = *theBulletItr;
    aBullet->Update();

    if (mCurTreasure != NULL)
    {
        float dx = aBullet->GetX() - mCurTreasure->x - 15;
        float dy = aBullet->GetY() - mCurTreasure->y - 15;
        float r = aBullet->GetRadius() + 20;

        if (dx * dx + dy * dy < r * r)
        {
            ++mLevelStats.mNumGemsCleared;
            FloatingTextHelper aFloat;

            int bonus = ((mScoreTarget - mLevelBeginScore) / 600) * 100;

            if (mIsEndless)
            {
                if (bonus < 200)
                    bonus = 200;
            }
            else
            {
                if (bonus < 500)
                    bonus = 500;
            }

            aFloat.AddText(StrFormat("BONUS +%d", bonus), Sexy::FONT_FLOAT_ID, 0xFFFF00);
            aFloat.AddToMgr(mParticleMgr, mCurTreasure->x, mCurTreasure->y);
            IncScore(bonus);

            mApp->PlaySample(Sexy::SOUND_JEWEL_HIT);

            mTreasureZoomFrame = 1;
            mTreasureZoomX = mCurTreasure->x;
            mTreasureZoomY = mCurTreasure->y;

            for (int i = 0; i < 10; i++)
            {
                float sparkVx = cosf(i * 360 / 10 * SEXY_PI / 180.0f);
                float sparkVy = sinf(i * 360 / 10 * SEXY_PI / 180.0f);
                uint color = gBrightBallColors[Sexy::Rand() % 6];

                mParticleMgr->AddSparkle(mCurTreasure->x + sparkVx * 16.0f, mCurTreasure->y + sparkVx * 16.0f,
                                         2 * sparkVx, 2 * sparkVy, 5, 0, Sexy::Rand() % 5, color);
            }

            mCurTreasure = NULL;

            delete aBullet;
            theBulletItr = mBulletList.erase(theBulletItr);
            return true;
        }
    }

    if (aBullet->mMissileStrike)
    {
        for (int i = 0; i < mNumCurves; i++)
        {
            Ball *aHit = mCurveMgr[i]->FindShootableBallTouching(aBullet);
            if (aHit != NULL)
            {
                mApp->PlaySample(Sexy::SOUND_EXPLODE);
                mCurveMgr[i]->ActivateScaledBomb(aHit, kMissileStrikeBombScale);
                // The blast also reaches balls on other paths, tunnels included.
                for (int j = 0; j < mNumCurves; j++)
                {
                    if (j != i)
                        mCurveMgr[j]->ClearBallsInBlast(aHit, kMissileStrikeBombScale);
                }
                mStrikeShakeTicks = kStrikeShakeTicks;
                delete aBullet;
                theBulletItr = mBulletList.erase(theBulletItr);
                return true;
            }
        }
    }
    else if (gCheckCollision)
    {
        for (int i = 0; i < mNumCurves; i++)
        {
            if (mCurveMgr[i]->CheckCollision(aBullet))
            {
                theBulletItr = mBulletList.erase(theBulletItr);
                return true;
            }
        }
    }

    for (int i = 0; i < mNumCurves; i++)
    {
        mCurveMgr[i]->CheckGapShot(aBullet);
    }

    if (aBullet->GetX() >= 0.0f && aBullet->GetY() >= 0.0f &&
        (aBullet->GetX() - aBullet->GetRadius()) < mWidth &&
        (aBullet->GetY() - aBullet->GetRadius()) < mHeight)
    {
        return false;
    }

    ResetInARowBonus();
    delete aBullet;
    theBulletItr = mBulletList.erase(theBulletItr);
    return true;
}

// Show the mouth ball as a bomb while the aim screen is open.
void Board::ArmMissileStrikeBullet()
{
    Bullet *aBullet = mGun->GetBullet();
    if (aBullet == mMissileStrikeArmedBullet)
        return;

    DisarmMissileStrikeBullet();
    if (aBullet == NULL)
        return;

    mMissileStrikeArmedBullet = aBullet;
    mMissileStrikeArmedOldPower = aBullet->GetPowerType();
    aBullet->SetPowerType(PowerType_Bomb, false);
}

void Board::DisarmMissileStrikeBullet()
{
    Bullet *aBullet = mMissileStrikeArmedBullet;
    mMissileStrikeArmedBullet = NULL;

    // Only touch it if the gun still holds it.
    if (aBullet != NULL && (aBullet == mGun->GetBullet() || aBullet == mGun->GetNextBullet()))
        aBullet->SetPowerType(mMissileStrikeArmedOldPower, false);
}

bool Board::FireMissileStrikeBomb(int theX, int theY)
{
    for (int i = 0; i < mNumCurves; i++)
    {
        if (!mCurveMgr[i]->CanFire())
            return false;
    }

    ArmMissileStrikeBullet();
    Bullet *aBullet = mMissileStrikeArmedBullet;
    if (aBullet == NULL)
        return false;

    // Face the click so the shot flies straight at it.
    MouseMove(theX, theY);

    aBullet->mMissileStrike = true;
    if (!mGun->StartFire())
    {
        aBullet->mMissileStrike = false;
        return false;
    }

    // Multi-frog copies the shot; only the main frog fires the bomb.
    for (int i = 0; i < mGun->mExtraBulletCount; i++)
    {
        if (mGun->mExtraBullets[i] != NULL)
            mGun->mExtraBullets[i]->SetPowerType(mMissileStrikeArmedOldPower, false);
    }

    mMissileStrikeArmedBullet = NULL; // fired: keep the bomb look
    mMissileStrikeCooldown = kMissileStrikeCooldownTicks;
    mApp->PlaySample(Sexy::SOUND_BALLFIRE);
    return true;
}

void Board::SetMissileStrikeSelecting(bool selecting)
{
    if (mMissileStrikeSelecting == selecting)
        return;

    mMissileStrikeSelecting = selecting;
    if (selecting)
        ArmMissileStrikeBullet();
    else
        DisarmMissileStrikeBullet();

    gSexyAppBase->SetCursor(selecting ? Sexy::CURSOR_NONE : Sexy::CURSOR_POINTER);
    MarkDirty();
}

void Board::GetMissileStrikeIconCenter(int &theX, int &theY)
{
    theX = 28 + 12;
    theY = 3 + Sexy::IMAGE_FROG_LIVES->mHeight + 4 + 20;
}

bool Board::MissileStrikeIconHit(int theX, int theY)
{
    int cx, cy;
    GetMissileStrikeIconCenter(cx, cy);
    return theX >= cx - 16 && theX <= cx + 16 && theY >= cy - 16 && theY <= cy + 16;
}

///////////////////////////////////////////////////////////////////////////////
// Missile strike circles. The library has no circle drawing, so each ring or
// disc is rendered once into a white, anti-aliased image and cached; drawing
// tints it with the current color. The cache lives for the whole run.
///////////////////////////////////////////////////////////////////////////////
namespace
{
    struct AimCircleImage
    {
        int mRadiusKey;    // radius * 4
        int mThicknessKey; // thickness * 4; 0 = filled disc
        MemoryImage *mImage;
    };

    const int kMaxAimCircleImages = 16;
    AimCircleImage gAimCircleImages[kMaxAimCircleImages];
    int gAimCircleImageCount = 0;
    MemoryImage *gStrikeHoleImage = NULL;
}

// White image of a ring (theThickness > 0) or a filled disc (theThickness <= 0),
// centered in a square of odd size so it lines up with whole pixels.
static MemoryImage *GetAimCircleImage(float theRadius, float theThickness)
{
    int aRadiusKey = (int)(theRadius * 4.0f + 0.5f);
    int aThicknessKey = theThickness > 0.0f ? (int)(theThickness * 4.0f + 0.5f) : 0;
    for (int i = 0; i < gAimCircleImageCount; i++)
    {
        if (gAimCircleImages[i].mRadiusKey == aRadiusKey && gAimCircleImages[i].mThicknessKey == aThicknessKey)
            return gAimCircleImages[i].mImage;
    }

    float aHalf = theThickness > 0.0f ? theThickness * 0.5f : 0.0f;
    int aSize = (int)ceilf(theRadius + aHalf) * 2 + 3;

    MemoryImage *anImage = new MemoryImage(gSexyAppBase);
    anImage->Create(aSize, aSize);
    anImage->SetImageMode(true, true);
    ulong *aBits = anImage->GetBits();

    float aCenter = aSize * 0.5f;
    for (int y = 0; y < aSize; y++)
    {
        for (int x = 0; x < aSize; x++)
        {
            float dx = (x + 0.5f) - aCenter;
            float dy = (y + 0.5f) - aCenter;
            float d = sqrtf(dx * dx + dy * dy);

            // Coverage of this pixel, with a one-pixel soft edge.
            float aCover;
            if (theThickness > 0.0f)
                aCover = aHalf + 0.5f - fabsf(d - theRadius);
            else
                aCover = theRadius + 0.5f - d;
            if (aCover < 0.0f)
                aCover = 0.0f;
            if (aCover > 1.0f)
                aCover = 1.0f;

            ulong anAlpha = (ulong)(aCover * 255.0f + 0.5f);
            aBits[y * aSize + x] = (anAlpha << 24) | 0x00FFFFFF;
        }
    }
    anImage->BitsChanged();

    if (gAimCircleImageCount < kMaxAimCircleImages)
    {
        AimCircleImage &anEntry = gAimCircleImages[gAimCircleImageCount++];
        anEntry.mRadiusKey = aRadiusKey;
        anEntry.mThicknessKey = aThicknessKey;
        anEntry.mImage = anImage;
    }
    return anImage;
}

static void DrawAimCircleImage(Graphics *g, int theX, int theY, float theRadius, float theThickness)
{
    MemoryImage *anImage = GetAimCircleImage(theRadius, theThickness);
    bool wasColorize = g->GetColorizeImages();
    g->SetColorizeImages(true);
    g->DrawImage(anImage, theX - anImage->mWidth / 2, theY - anImage->mHeight / 2);
    g->SetColorizeImages(wasColorize);

    if (gAimCircleImageCount >= kMaxAimCircleImages)
        delete anImage; // cache full (should not happen): don't leak
}

// Ring outline in the current color.
static void DrawAimRing(Graphics *g, int theX, int theY, float theRadius, int theThickness)
{
    DrawAimCircleImage(g, theX, theY, theRadius, (float)theThickness);
}

// Filled disc in the current color.
static void DrawAimDisc(Graphics *g, int theX, int theY, float theRadius)
{
    DrawAimCircleImage(g, theX, theY, theRadius, 0.0f);
}

// Ring, four ticks crossing it, and a center dot.
static void DrawAimCrosshair(Graphics *g, int theX, int theY, int theRadius, const Color &theColor)
{
    g->SetColor(theColor);
    int aThick = theRadius >= 24 ? 3 : 2;
    DrawAimRing(g, theX, theY, (float)theRadius * 0.7f, aThick);

    // Ticks from 30% to 100% of the radius, aThick wide, centered on the axes.
    int anInner = theRadius * 3 / 10;
    int aLen = theRadius - anInner;
    int anOff = aThick / 2;
    g->FillRect(theX - theRadius, theY - anOff, aLen, aThick);
    g->FillRect(theX + anInner + 1, theY - anOff, aLen, aThick);
    g->FillRect(theX - anOff, theY - theRadius, aThick, aLen);
    g->FillRect(theX - anOff, theY + anInner + 1, aThick, aLen);

    int aDot = theRadius / 8;
    if (aDot < 2)
        aDot = 2;
    DrawAimDisc(g, theX, theY, (float)aDot);
}

// Square of 75% black with a soft-edged clear circle in the middle.
static MemoryImage *GetStrikeHoleImage(int theRadius)
{
    if (gStrikeHoleImage != NULL)
        return gStrikeHoleImage;

    int aSize = theRadius * 2 + 3;
    MemoryImage *anImage = new MemoryImage(gSexyAppBase);
    anImage->Create(aSize, aSize);
    anImage->SetImageMode(true, true);
    ulong *aBits = anImage->GetBits();

    float aCenter = aSize * 0.5f;
    for (int y = 0; y < aSize; y++)
    {
        for (int x = 0; x < aSize; x++)
        {
            float dx = (x + 0.5f) - aCenter;
            float dy = (y + 0.5f) - aCenter;
            float aCover = sqrtf(dx * dx + dy * dy) - (float)theRadius + 0.5f;
            if (aCover < 0.0f)
                aCover = 0.0f;
            if (aCover > 1.0f)
                aCover = 1.0f;

            ulong anAlpha = (ulong)(aCover * 191.0f + 0.5f);
            aBits[y * aSize + x] = anAlpha << 24; // black
        }
    }
    anImage->BitsChanged();
    gStrikeHoleImage = anImage;
    return anImage;
}

void Board::DrawMissileStrikeIcon(Graphics *g)
{
    int cx, cy;
    GetMissileStrikeIconCenter(cx, cy);

    bool aReady = mMissileStrikeCooldown <= 0;

    g->SetColor(Color(0, 0, 0));
    DrawAimDisc(g, cx, cy, 15.0f);
    g->SetColor(aReady ? Color(255, 220, 0) : Color(110, 110, 110));
    DrawAimRing(g, cx, cy, 14.0f, 2);

    if (aReady)
    {
        DrawAimCrosshair(g, cx, cy, 12, Color(255, 70, 50));
        return;
    }

    DrawAimCrosshair(g, cx, cy, 12, Color(90, 90, 90));

    int aSeconds = (mMissileStrikeCooldown + 99) / 100;
    std::string aText = Sexy::StrFormat("%d", aSeconds);
    g->SetFont(Sexy::FONT_DIALOG);
    int aWidth = Sexy::FONT_DIALOG->StringWidth(aText);
    int aBaseline = cy + Sexy::FONT_DIALOG->GetAscent() / 2;
    g->SetColor(Color(0, 0, 0));
    g->DrawString(aText, cx - aWidth / 2 + 1, aBaseline + 1);
    g->SetColor(Color(255, 255, 255));
    g->DrawString(aText, cx - aWidth / 2, aBaseline);
}

void Board::DrawMissileStrikeOverlay(Graphics *g)
{
    // 75% black everywhere except a clear circle around the crosshair. The
    // circle comes from a pre-rendered image; plain rects cover the rest.
    MemoryImage *aHole = GetStrikeHoleImage(kStrikeHoleRadius);
    int aSize = aHole->mWidth;
    int aLeft = mStrikeMouseX - aSize / 2;
    int aTop = mStrikeMouseY - aSize / 2;

    g->SetColor(Color(0, 0, 0, 191));
    g->FillRect(0, 0, mWidth, aTop);                                      // above
    g->FillRect(0, aTop + aSize, mWidth, mHeight - (aTop + aSize));       // below
    g->FillRect(0, aTop, aLeft, aSize);                                   // left
    g->FillRect(aLeft + aSize, aTop, mWidth - (aLeft + aSize), aSize);    // right
    g->DrawImage(aHole, aLeft, aTop);
}

void Board::UpdateBullets()
{
    ResolveKillerCollisions();

    for (BulletList::iterator aBulletItr = mBulletList.begin(); aBulletItr != mBulletList.end();)
    {
        AdvanceFreeBullet(aBulletItr);
    }
}

int Board::LevelSwapIntervalTicks() const
{
    int sec = mApp->mLevelSwapIntervalSec;
    if (sec < 1)
        sec = 1;
    if (sec > 60)
        sec = 60;
    return sec * 100;
}

void Board::ClearLevelSwap()
{
    mLevelSwapTransition = 0;
}

void Board::PaintCurvesOntoBackground()
{
    if (mApp->mBaseMinimumMode)
        return;

    CurveDrawer aCurveDrawer(mSpriteMgr);
    for (int i = 0; i < mNumCurves; i++)
    {
        if (mLevelDesc->mCurveDesc[i].mDrawCurve && mLevelDesc->mCurveDesc[i].mDrawTunnels)
            mCurveMgr[i]->DrawTunnel(aCurveDrawer);
    }
    for (int i = 0; i < mNumCurves; i++)
    {
        if (mLevelDesc->mCurveDesc[i].mDrawCurve)
            mCurveMgr[i]->DrawCurve(aCurveDrawer);
    }
    aCurveDrawer.AddCurvesToMgr();
}

void Board::DrawLevelSwapFade(Graphics *g)
{
    if (mLevelSwapTransition <= 0)
        return;

    // 50 ticks = 0.5s. Fade out over 0.2s, hold black through 0.3s, fade in to 0.5s.
    float u;
    if (mLevelSwapTransition >= 30)
        u = (50.0f - (float)mLevelSwapTransition) / 20.0f;
    else if (mLevelSwapTransition <= 20)
        u = (float)mLevelSwapTransition / 20.0f;
    else
        u = 1.0f;
    if (u < 0.0f)
        u = 0.0f;
    if (u > 1.0f)
        u = 1.0f;
    u = u * u * (3.0f - 2.0f * u);

    int alpha = (int)(u * 255.0f + 0.5f);
    if (alpha <= 0)
        return;

    g->SetColor(Color(0, 0, 0, alpha));
    g->FillRect(0, 0, CIRCLE_WINDOW_WIDTH, CIRCLE_WINDOW_HEIGHT);
}

bool Board::BeginLevelSwap()
{
    if (mLevelDesc == NULL || mApp->mLevelParser == NULL)
        return false;

    LevelDesc aGfx;
    std::string avoid = mLevelDesc->mCurveDesc[0].mPath;
    if (!mApp->mLevelParser->PickRandomGraphics(mNumCurves, avoid, aGfx))
        return false;

    for (int i = 0; i < 3; i++)
    {
        mLevelDesc->mCurveDesc[i].mPath = aGfx.mCurveDesc[i].mPath;
        mLevelDesc->mCurveDesc[i].mSkullRotation = aGfx.mCurveDesc[i].mSkullRotation;
        mLevelDesc->mCurveDesc[i].mDrawCurve = aGfx.mCurveDesc[i].mDrawCurve;
        mLevelDesc->mCurveDesc[i].mDrawTunnels = aGfx.mCurveDesc[i].mDrawTunnels;
        mLevelDesc->mCurveDesc[i].mCurveColor = aGfx.mCurveDesc[i].mCurveColor;
    }
    mLevelDesc->mGunX = aGfx.mGunX;
    mLevelDesc->mGunY = aGfx.mGunY;
    mLevelDesc->mBGColor = aGfx.mBGColor;
    mLevelDesc->mImagePath = aGfx.mImagePath;
    mLevelDesc->mSprites = aGfx.mSprites;
    mLevelDesc->mBackgroundAlpha = aGfx.mBackgroundAlpha;
    mLevelDesc->mInSpace = aGfx.mInSpace;

    mGun->SetPos(mLevelDesc->mGunX, mLevelDesc->mGunY);
    mSpriteMgr->SetupLevel(*mLevelDesc, MirrorType_None);

    for (int i = 0; i < mNumCurves; i++)
    {
        mCurveMgr[i]->SetupLevel(mLevelDesc, mSpriteMgr, i, MirrorType_None);
        mCurveMgr[i]->ShiftChainToLoadedPath();
    }

    PaintCurvesOntoBackground();

    // Move a live coin onto the nearest new-level spot it could actually appear.
    int oldX = 0;
    int oldY = 0;
    bool hadCoin = mCurTreasure != NULL;
    if (hadCoin)
    {
        oldX = mCurTreasure->x;
        oldY = mCurTreasure->y;
    }
    mCurTreasure = NULL;
    mLevelDesc->mTreasurePoints = aGfx.mTreasurePoints;
    if (hadCoin && !mLevelDesc->mTreasurePoints.empty())
    {
        int best = -1;
        int bestDist = 0;
        int nearest = -1;
        int nearestDist = 0;
        int idx = 0;
        for (TreasurePointList::iterator anItr = mLevelDesc->mTreasurePoints.begin();
             anItr != mLevelDesc->mTreasurePoints.end(); ++anItr, ++idx)
        {
            int dx = anItr->x - oldX;
            int dy = anItr->y - oldY;
            int dist = dx * dx + dy * dy;
            if (nearest < 0 || dist < nearestDist)
            {
                nearest = idx;
                nearestDist = dist;
            }

            bool canSpawn = true;
            for (int j = 0; j < mNumCurves; j++)
            {
                if (anItr->mCurveDist[j] == 0)
                    continue;
                if (anItr->mCurveDist[j] > mCurveMgr[j]->GetFarthestBallPercent())
                {
                    canSpawn = false;
                    break;
                }
            }
            if (!canSpawn)
                continue;
            if (best < 0 || dist < bestDist)
            {
                best = idx;
                bestDist = dist;
            }
        }

        if (best < 0)
            best = nearest;
        mCurTreasureNum = best;
        TreasurePointList::iterator anItr = mLevelDesc->mTreasurePoints.begin();
        std::advance(anItr, best);
        mCurTreasure = &*anItr;
    }
    return true;
}

bool Board::UpdateLevelSwap()
{
    if (!mApp->mLevelSwapMode || mGameState != GameState_Playing)
        return false;

    if (mLevelSwapTransition > 0)
    {
        mLevelSwapTransition--;
        // 0.25s into the fade (counter 25). The screen is already fully black.
        if (mLevelSwapTransition == 25)
            BeginLevelSwap();
        if (mLevelSwapTransition == 0)
            mLevelSwapCooldown = LevelSwapIntervalTicks();
        return false;
    }

    if (mLevelSwapCooldown < 0)
    {
        mLevelSwapCooldown = LevelSwapIntervalTicks();
        return false;
    }

    if (mLevelSwapCooldown > 0)
    {
        mLevelSwapCooldown--;
        return false;
    }

    mLevelSwapTransition = 50;
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Multi game
///////////////////////////////////////////////////////////////////////////////
namespace
{
    const int kMultiSwitchTicks = 50; // 0.5 s: fade to black, switch, fade back
    const int kMultiSwitchHalf = kMultiSwitchTicks / 2;
}

Board::MultiInstance::MultiInstance()
{
    for (int i = 0; i < MAX_CURVES; i++)
        mCurveMgr[i] = NULL;
    mOwnLevelDesc = NULL;
    mGun = NULL;
    mParticleMgr = NULL;
    mSoundMgr = NULL;
    mCurTreasure = NULL;
    mCurTreasureNum = 0;
    mTreasureEndFrame = 0;
    mTreasureZoomX = 0;
    mTreasureZoomY = 0;
    mTreasureZoomFrame = 0;
    mGuideBall = NULL;
    mShowGuide = false;
    mDoGuide = false;
    mRecalcGuide = false;
    mAccuracyCount = 0;
    mNumCleared = 0;
    mClearedXSum = 0;
    mClearedYSum = 0;
    mCurComboCount = 0;
    mCurComboScore = 0;
    mNumClearsInARow = 0;
    mCurInARowBonus = 0;
    mLevelBeginning = false;
    mLevelEndFrame = 0;
    mKillerBallCooldown = 0;
    mKillerHitFrog = false;
    mStoppedAdding = false;
}

// Swap every per-copy field between the Board and theSlot.
void Board::MultiSwapLoaded(MultiInstance &s)
{
    for (int i = 0; i < MAX_CURVES; i++)
        std::swap(mCurveMgr[i], s.mCurveMgr[i]);
    std::swap(mMultiOwnLevelDesc, s.mOwnLevelDesc);
    std::swap(mGun, s.mGun);
    std::swap(mParticleMgr, s.mParticleMgr);
    std::swap(mSoundMgr, s.mSoundMgr);
    mBulletList.swap(s.mBulletList);
    mBallColorMap.swap(s.mBallColorMap);
    mNeedComboCount.swap(s.mNeedComboCount);
    std::swap(mCurTreasure, s.mCurTreasure);
    std::swap(mCurTreasureNum, s.mCurTreasureNum);
    std::swap(mTreasureEndFrame, s.mTreasureEndFrame);
    std::swap(mTreasureZoomX, s.mTreasureZoomX);
    std::swap(mTreasureZoomY, s.mTreasureZoomY);
    std::swap(mTreasureZoomFrame, s.mTreasureZoomFrame);
    for (int i = 0; i < 4; i++)
        std::swap(mGuide[i], s.mGuide[i]);
    std::swap(mGuideCenter, s.mGuideCenter);
    std::swap(mGuideBall, s.mGuideBall);
    std::swap(mShowGuide, s.mShowGuide);
    std::swap(mDoGuide, s.mDoGuide);
    std::swap(mRecalcGuide, s.mRecalcGuide);
    std::swap(mAccuracyCount, s.mAccuracyCount);
    std::swap(mNumCleared, s.mNumCleared);
    std::swap(mClearedXSum, s.mClearedXSum);
    std::swap(mClearedYSum, s.mClearedYSum);
    std::swap(mCurComboCount, s.mCurComboCount);
    std::swap(mCurComboScore, s.mCurComboScore);
    std::swap(mNumClearsInARow, s.mNumClearsInARow);
    std::swap(mCurInARowBonus, s.mCurInARowBonus);
    std::swap(mLevelBeginning, s.mLevelBeginning);
    std::swap(mLevelEndFrame, s.mLevelEndFrame);
    std::swap(mKillerBallCooldown, s.mKillerBallCooldown);
    std::swap(mKillerHitFrog, s.mKillerHitFrog);
    std::swap(mStoppedAdding, s.mStoppedAdding);
}

// Make copy theIndex the loaded one (its state moves into the Board's fields).
void Board::MultiExchange(int theIndex)
{
    if (theIndex == mMultiLoaded || theIndex < 0 || theIndex >= MAX_MULTI_GAMES)
        return;

    MultiInstance &aOld = mMulti[mMultiLoaded];
    mSpriteMgr->GetHoleState(aOld.mHoles);
    MultiSwapLoaded(aOld); // loaded copy -> its slot

    MultiInstance &aNew = mMulti[theIndex];
    MultiSwapLoaded(aNew); // slot -> Board
    mSpriteMgr->SetHoleState(aNew.mHoles);

    mMultiLoaded = theIndex;
}

// Free every copy that isn't loaded (slots of the loaded copy are empty).
void Board::MultiDestroyExtras()
{
    for (int k = 0; k < MAX_MULTI_GAMES; k++)
    {
        if (k == mMultiLoaded)
            continue;

        MultiInstance &s = mMulti[k];
        for (BulletList::iterator anItr = s.mBulletList.begin(); anItr != s.mBulletList.end(); ++anItr)
            delete *anItr;
        s.mBulletList.clear();
        s.mBallColorMap.clear();
        s.mNeedComboCount.clear();

        for (int i = 0; i < MAX_CURVES; i++)
        {
            delete s.mCurveMgr[i];
            s.mCurveMgr[i] = NULL;
        }
        delete s.mOwnLevelDesc;
        delete s.mGun;
        delete s.mParticleMgr;
        delete s.mSoundMgr;

        s = MultiInstance();
    }
}

// Called at the end of Reset (copy 1 loaded and freshly set up): build
// copies 2..N of the same level, each with its own paths, chain and frog.
void Board::MultiRebuildExtras()
{
    MultiDestroyExtras();

    mMultiCount = 1;
    mMultiView = 0;
    mMultiEndRequest = -1;
    for (int i = 0; i < MAX_MULTI_GAMES; i++)
        mMultiDanger[i] = false;
    mSoundMgr->SetSilent(false);

    if (!mApp->mMultiGameMode)
        return;

    int aCount = mApp->mMultiGameCount;
    if (aCount < 1)
        aCount = 1;
    if (aCount > MAX_MULTI_GAMES)
        aCount = MAX_MULTI_GAMES;
    if (aCount <= 1)
        return;

    // Holes as copy 1 left them; SetupLevel would add duplicates otherwise.
    SpriteMgr::HoleState aBaseHoles;
    mSpriteMgr->GetHoleState(aBaseHoles);

    for (int k = 1; k < aCount; k++)
    {
        MultiInstance &s = mMulti[k];
        s = MultiInstance();
        s.mOwnLevelDesc = new LevelDesc(*mLevelDesc);
        for (int i = 0; i < MAX_CURVES; i++)
            s.mCurveMgr[i] = new CurveMgr(this);
        s.mGun = new Gun();
        s.mParticleMgr = new ParticleMgr(this);
        s.mSoundMgr = new SoundMgr();
        s.mSoundMgr->SetSilent(true);
        s.mHoles = aBaseHoles;

        // Set the copy up while it is loaded, so its balls register in its
        // own color map and its paths use its own level data.
        MultiExchange(k);
        for (int i = 0; i < MAX_CURVES; i++)
            mCurveMgr[i]->SetupLevel(mMultiOwnLevelDesc, mSpriteMgr, i, MirrorType_None);
        mSpriteMgr->SetHoleState(aBaseHoles);

        mGun->SetFireSpeed(mLevelDesc->mFireSpeed);
        mGun->SetPos(mLevelDesc->mGunX, mLevelDesc->mGunY);
        mGun->EmptyBullets();
        mBallColorMap.clear();
        for (int i = 0; i < mNumCurves; i++)
            mCurveMgr[i]->StartLevel();

        MultiExchange(0);
    }

    mMultiCount = aCount;
}

// Per-copy part of StartLevel for copies 2..N (copy 1 ran the real StartLevel).
void Board::MultiStartLevelCopies()
{
    if (!IsMultiGameActive())
        return;

    int aView = mMultiLoaded;
    for (int k = 0; k < mMultiCount; k++)
    {
        if (k == aView)
            continue;

        MultiExchange(k);
        mParticleMgr->Clear();
        mGun->SetFireSpeed(mLevelDesc->mFireSpeed);
        mGun->SetPos(mLevelDesc->mGunX, mLevelDesc->mGunY);
        mSoundMgr->PlayLoop(LoopType_RollIn); // silent: no sound
        mLevelBeginning = true;
        mKillerHitFrog = false;
        if (mApp->mKillerBallMode)
        {
            float intervalSec = mApp->mKillerBallIntervalSec;
            if (intervalSec < 2.0f)
                intervalSec = 2.0f;
            if (intervalSec > 15.0f)
                intervalSec = 15.0f;
            mKillerBallCooldown = (int)(intervalSec * 100.0f + 0.5f);
            if (mKillerBallCooldown < 1)
                mKillerBallCooldown = 1;
        }
        else
        {
            mKillerBallCooldown = 0;
        }
    }
    MultiExchange(aView);
}

bool Board::MultiLoadedInDanger()
{
    for (int i = 0; i < mNumCurves; i++)
    {
        if (mCurveMgr[i]->IsInDanger())
            return true;
    }
    return false;
}

// Same tests as CheckEndConditions, without acting: remember the copy.
void Board::MultiCheckBackgroundEnd()
{
    if (mKillerHitFrog)
    {
        mMultiEndRequest = mMultiLoaded;
        return;
    }

    for (BulletList::iterator anItr = mBulletList.begin(); anItr != mBulletList.end(); ++anItr)
    {
        if (!(*anItr)->IsKillerBall())
            return; // a shot in flight can still save it
    }
    if (mGun->IsFiring())
        return;

    bool allWinning = true;
    bool anyLosing = false;
    for (int i = 0; i < mNumCurves; i++)
    {
        if (!mCurveMgr[i]->IsWinning())
            allWinning = false;
        if (mCurveMgr[i]->IsLosing())
            anyLosing = true;
    }

    if (allWinning || anyLosing)
        mMultiEndRequest = mMultiLoaded;
}

// Every frame while playing: update the copies off screen (silently), then
// the one on screen; if a background copy lost or won, show it and act.
void Board::MultiUpdatePlaying()
{
    int aView = mMultiView;
    mMultiEndRequest = -1;

    for (int k = 0; k < mMultiCount; k++)
    {
        if (k == aView)
            continue;

        MultiExchange(k);
        mMultiBackground = true;
        mApp->mMuteGameSfx = true;
        UpdatePlaying();
        mApp->mMuteGameSfx = false;
        mMultiBackground = false;
        mMultiDanger[k] = MultiLoadedInDanger();
    }

    MultiExchange(aView);
    UpdatePlaying();
    mMultiDanger[aView] = MultiLoadedInDanger();

    if (mMultiEndRequest >= 0 && mGameState == GameState_Playing)
    {
        int aCopy = mMultiEndRequest;
        mMultiEndRequest = -1;
        MultiCancelSwitch();
        MultiSetView(aCopy); // show the copy that lost (or won)
        CheckEndConditions();
    }
}

// Instantly put copy theIndex on screen: its sounds on, the others silent,
// and the music matching its danger state.
void Board::MultiSetView(int theIndex)
{
    if (theIndex == mMultiView && theIndex == mMultiLoaded)
        return;

    SetMissileStrikeSelecting(false);
    mSoundMgr->SetSilent(true);
    MultiExchange(theIndex);
    mMultiView = theIndex;
    mSoundMgr->SetSilent(false);
    if (mLevelBeginning)
        mSoundMgr->PlayLoop(LoopType_RollIn);

    // Music for this copy right away (no 5 s debounce: the screen is black).
    mApp->PlaySong(MultiLoadedInDanger() ? 36 : 0, false, 0.01);

    mRecalcGuide = true;
    if (mWidgetManager != NULL)
        MouseMove(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY); // new frog aims at the cursor
    MarkDirty();
}

void Board::MultiStartSwitch(int theIndex)
{
    if (!IsMultiGameActive() || mGameState != GameState_Playing)
        return;
    if (theIndex < 0 || theIndex >= mMultiCount || theIndex == mMultiView)
        return;
    if (mMultiSwitchTicks > 0)
        return;

    SetMissileStrikeSelecting(false);
    mMultiSwitchTarget = theIndex;
    mMultiSwitchTicks = kMultiSwitchTicks;
}

// Fade the picture and all audio to black/silence over 0.25 s, switch, then
// fade back in over 0.25 s. Runs every unpaused tick.
void Board::MultiUpdateSwitch()
{
    if (mMultiSwitchTicks <= 0)
        return;

    if (mGameState != GameState_Playing || !IsMultiGameActive())
    {
        MultiCancelSwitch();
        return;
    }

    mMultiSwitchTicks--;
    if (mMultiSwitchTicks == kMultiSwitchHalf)
        MultiSetView(mMultiSwitchTarget);

    int aDist = mMultiSwitchTicks - kMultiSwitchHalf;
    if (aDist < 0)
        aDist = -aDist;
    MultiApplyVolume(mMultiSwitchTicks == 0 ? 1.0 : (double)aDist / kMultiSwitchHalf);
    MarkDirty();
}

void Board::MultiApplyVolume(double theFactor)
{
    if (theFactor >= 1.0)
    {
        // Back to the player's settings (also honours a muted app).
        mApp->SetSfxVolume(mApp->GetSfxVolume());
        mApp->SetMusicVolume(mApp->GetMusicVolume());
        return;
    }

    if (theFactor < 0.0)
        theFactor = 0.0;
    if (mApp->mSoundManager != NULL)
        mApp->mSoundManager->SetVolume(mApp->GetSfxVolume() * theFactor);
    if (mApp->mMusicInterface != NULL)
        mApp->mMusicInterface->SetVolume(mApp->GetMusicVolume() * theFactor);
}

void Board::MultiCancelSwitch()
{
    if (mMultiSwitchTicks > 0)
    {
        mMultiSwitchTicks = 0;
        MultiApplyVolume(1.0);
    }
}

bool Board::MultiHandleKeyChar(char theChar)
{
    if (!IsMultiGameActive())
        return false;

    char c = toupper(theChar);
    bool isKey = c == 'A' || c == 'D' || (c >= '1' && c <= '4');
    if (!isKey)
        return false;

    if (mGameState == GameState_Playing && mPauseCount == 0 && mDialogCount == 0)
    {
        if (c == 'A')
            MultiStartSwitch((mMultiView + mMultiCount - 1) % mMultiCount);
        else if (c == 'D')
            MultiStartSwitch((mMultiView + 1) % mMultiCount);
        else
            MultiStartSwitch(c - '1');
    }
    return true; // swallow the debug meaning of these keys while the mode is on
}

// Warning triangle for Multi game, rendered once like the aim icons: each pixel
// is 4x4 supersampled against the triangle's edges, giving smooth anti-aliased
// edges, a dark rim and an orange-to-red fill. Drawn tinted for the pulse.
static MemoryImage *gMultiWarningImage = NULL;

static MemoryImage *GetMultiWarningImage(int theWidth, int theHeight)
{
    if (gMultiWarningImage != NULL)
        return gMultiWarningImage;

    // Triangle corners (pixel units), with a small margin so edges can soften.
    const float ax = theWidth * 0.5f, ay = 0.75f;              // top
    const float bx = theWidth - 0.75f, by = theHeight - 0.75f; // bottom right
    const float cx = 0.75f, cy = theHeight - 0.75f;            // bottom left
    const float kRim = 2.2f;                                    // dark rim width

    MemoryImage *anImage = new MemoryImage(gSexyAppBase);
    anImage->Create(theWidth, theHeight);
    anImage->SetImageMode(true, true);
    ulong *aBits = anImage->GetBits();

    // Inward distance from a point to edge P->Q (triangle is clockwise on screen).
    struct Edge
    {
        static float Dist(float px, float py, float x0, float y0, float x1, float y1)
        {
            float ex = x1 - x0, ey = y1 - y0;
            float len = sqrtf(ex * ex + ey * ey);
            return ((px - x0) * ey - (py - y0) * ex) / len * -1.0f;
        }
    };

    for (int y = 0; y < theHeight; y++)
    {
        for (int x = 0; x < theWidth; x++)
        {
            float aSumA = 0.0f, aSumR = 0.0f, aSumG = 0.0f, aSumB = 0.0f;
            for (int sy = 0; sy < 4; sy++)
            {
                for (int sx = 0; sx < 4; sx++)
                {
                    float px = x + (sx + 0.5f) / 4.0f;
                    float py = y + (sy + 0.5f) / 4.0f;
                    float d0 = Edge::Dist(px, py, ax, ay, bx, by);
                    float d1 = Edge::Dist(px, py, bx, by, cx, cy);
                    float d2 = Edge::Dist(px, py, cx, cy, ax, ay);
                    float d = d0 < d1 ? d0 : d1;
                    if (d2 < d)
                        d = d2;
                    if (d < 0.0f)
                        continue; // outside

                    float r, gr, b;
                    if (d < kRim)
                    {
                        r = 45.0f; gr = 8.0f; b = 4.0f; // dark rim
                    }
                    else
                    {
                        // Orange at the top fading to red at the bottom, plus a
                        // soft highlight along the inner top edges.
                        float t = (py - ay) / (cy - ay);
                        r = 255.0f;
                        gr = 150.0f - 110.0f * t;
                        b = 30.0f - 10.0f * t;
                        float aGlow = 1.0f - (d - kRim) / 3.0f;
                        if (aGlow > 0.0f && d2 < d0 + 0.5f)
                        {
                            gr += (230.0f - gr) * aGlow * 0.35f;
                            b += (160.0f - b) * aGlow * 0.35f;
                        }
                    }
                    aSumA += 1.0f;
                    aSumR += r;
                    aSumG += gr;
                    aSumB += b;
                }
            }

            if (aSumA <= 0.0f)
            {
                aBits[y * theWidth + x] = 0;
                continue;
            }

            ulong a = (ulong)(aSumA / 16.0f * 255.0f + 0.5f);
            ulong r = (ulong)(aSumR / aSumA + 0.5f);
            ulong gg = (ulong)(aSumG / aSumA + 0.5f);
            ulong bb = (ulong)(aSumB / aSumA + 0.5f);
            aBits[y * theWidth + x] = (a << 24) | (r << 16) | (gg << 8) | bb;
        }
    }
    anImage->BitsChanged();
    gMultiWarningImage = anImage;
    return anImage;
}

// Top right: which copy is on screen, and a warning triangle with the number
// of every other copy whose chain is near the hole.
void Board::DrawMultiGameHud(Graphics *g)
{
    const int kRight = mWidth - 18;
    const int kTop = 36;
    const int kTriW = 30;
    const int kTriH = 26;
    const int kGap = 6;

    g->SetFont(Sexy::FONT_DIALOG);
    std::string aLabel = Sexy::StrFormat("%d/%d", mMultiView + 1, mMultiCount);
    int aLabelW = Sexy::FONT_DIALOG->StringWidth(aLabel);
    // Label on top, warning triangles in a row underneath it.
    int aBaseline = kTop + Sexy::FONT_DIALOG->GetAscent();
    g->SetColor(Color(0, 0, 0));
    g->DrawString(aLabel, kRight - aLabelW + 1, aBaseline + 1);
    g->SetColor(Color(0xD5E520));
    g->DrawString(aLabel, kRight - aLabelW, aBaseline);

    // Pulse so the warning catches the eye.
    int aPhase = mStateCount % 60;
    int aAlpha = 155 + (aPhase < 30 ? aPhase : 60 - aPhase) * 100 / 30;

    // Triangles in a column under the label, right-aligned with it, in copy order.
    int aTriTop = kTop + Sexy::FONT_DIALOG->GetHeight() + kGap;
    int x = kRight - kTriW;
    for (int k = 0; k < mMultiCount; k++)
    {
        if (k == mMultiView || !mMultiDanger[k])
            continue;

        MemoryImage *aTri = GetMultiWarningImage(kTriW, kTriH);
        bool wasColorize = g->GetColorizeImages();
        g->SetColorizeImages(true);
        g->SetColor(Color(255, 255, 255, aAlpha));
        g->DrawImage(aTri, x, aTriTop);
        g->SetColorizeImages(wasColorize);

        // Copy number in the lower middle of the triangle, with a shadow.
        std::string aNum = Sexy::StrFormat("%d", k + 1);
        int aNumW = Sexy::FONT_DIALOG->StringWidth(aNum);
        int aNumX = x + (kTriW - aNumW) / 2;
        int aNumY = aTriTop + kTriH - 5;
        g->SetColor(Color(60, 0, 0, aAlpha));
        g->DrawString(aNum, aNumX + 1, aNumY + 1);
        g->SetColor(Color(255, 255, 255, aAlpha));
        g->DrawString(aNum, aNumX, aNumY);

        aTriTop += kTriH + kGap;
    }
}

void Board::DrawMultiSwitchFade(Graphics *g)
{
    if (mMultiSwitchTicks <= 0)
        return;

    int aDist = mMultiSwitchTicks - kMultiSwitchHalf;
    if (aDist < 0)
        aDist = -aDist;
    int anAlpha = 255 - aDist * 255 / kMultiSwitchHalf;
    if (anAlpha <= 0)
        return;

    g->SetColor(Color(0, 0, 0, anAlpha));
    g->FillRect(0, 0, mWidth, mHeight);
}

void Board::UpdatePlaying()
{
    UpdateLevelSwap();

    Bullet *aBullet;
    while ((aBullet = mGun->GetFiredBullet()) != NULL)
    {
        mBulletList.push_back(aBullet);
        CheckReload();
    }

    UpdateBullets();
    UpdateTreasure();
    UpdateColorShift();
    UpdateInvisible();
    UpdateKillerBall();

    if (mLevelBeginning)
    {
        bool stillStarting = false;

        for (int i = 0; i < mNumCurves; ++i)
        {
            if (!mCurveMgr[i]->HasReachedCruisingSpeed())
            {
                stillStarting = true;
            }
        }

        if (!stillStarting || mStateCount > 500)
        {
            mLevelBeginning = false;
            mSoundMgr->StopLoop(LoopType_RollIn);
        }
    }

    // Shared event: handled by the copy on screen (so its sound isn't muted).
    if (!mHaveReachedTarget && !mMultiBackground && mCurBarSize == 256 && mScore >= mScoreTarget)
    {
        mBarBlinkCount = 224;
        mFlashCount = 50;
        mApp->PlaySample(Sexy::SOUND_BAR_FULL);

        if (mIsEndless)
        {
            DoLevelUp();
        }
        else
        {
            mHaveReachedTarget = true;
            mStoppedAdding = true;
            for (int i = 0; i < mNumCurves; ++i)
            {
                mCurveMgr[i]->SetStopAddingBalls(true);

                if (!mDestroyAll)
                {
                    mCurveMgr[i]->DetonateBalls();
                }
            }
        }

        if (!mDestroyAll)
        {
            mApp->PlaySample(Sexy::SOUND_EXPLODE);
        }
    }

    // Multi game: the target is shared, so the other copies stop adding balls
    // the first time they update after it was reached.
    if (mHaveReachedTarget && !mStoppedAdding && !mIsEndless)
    {
        mStoppedAdding = true;
        for (int i = 0; i < mNumCurves; ++i)
        {
            mCurveMgr[i]->SetStopAddingBalls(true);
            if (!mDestroyAll)
                mCurveMgr[i]->DetonateBalls();
        }
    }

    int aScoreDiff = mScoreTarget - mLevelBeginScore;
    if (aScoreDiff > 0)
    {
        int aScoreRemaining = mScoreTarget - mScore;
        if (aScoreRemaining < 0)
        {
            if (mLevelEndFrame != 0)
            {
                if (mStateCount - 3000 == mLevelEndFrame)
                {
                    // Each path's own level data (Multi game copies have their own).
                    for (int i = 0; i < mNumCurves; i++)
                    {
                        CurveDesc *aDesc = mCurveMgr[i]->GetCurveDesc();
                        aDesc->mPowerUpFreq[PowerType_Bomb] = 500;
                        aDesc->mPowerUpFreq[PowerType_SlowDown] = 0;
                        aDesc->mPowerUpFreq[PowerType_Accuracy] = 0;
                        aDesc->mPowerUpFreq[PowerType_MoveBackwards] = 0;
                        aDesc->mAccelerationRate = 0.0003;
                    }
                }
            }
            else if (mBallColorMap.size() == 2)
            {
                mLevelEndFrame = mStateCount;
            }
        }

        mTargetBarSize = 256 - (aScoreRemaining * 256 / aScoreDiff);
    }

    for (int i = 0; i < mNumCurves; ++i)
    {
        mCurveMgr[i]->UpdatePlaying();
    }

    CheckEndConditions();
    if (mStateCount > 50)
    {
        CheckReload();
    }

    MarkDirty();
}

void Board::UpdateLosing()
{
    MarkDirty();
    mTransitionMgr->Update();
}

void Board::UpdateLevelUp()
{
    MarkDirty();
    mTransitionMgr->Update();
}

void Board::UpdateLevelBegin()
{
    MarkDirty();
    mTransitionMgr->Update();
}

void Board::CheckReload()
{
    if (mBallColorMap.empty())
        return;

    Bullet *aBullet = mGun->GetBullet();

    if (aBullet != NULL && mBallColorMap.find(aBullet->GetType()) == mBallColorMap.end())
    {
        int rand = Sexy::AppRand() % mBallColorMap.size();
        ColorMap::iterator it = mBallColorMap.begin();

        for (int i = 0; i < rand; i++)
            it++;

        mGun->SetBulletType(it->first);
    }

    aBullet = mGun->GetNextBullet();

    if (aBullet != NULL && mBallColorMap.find(aBullet->GetType()) == mBallColorMap.end())
    {
        int rand = Sexy::AppRand() % mBallColorMap.size();
        ColorMap::iterator it = mBallColorMap.begin();

        for (int i = 0; i < rand; i++)
            it++;

        mGun->SetNextBulletType(it->first);
    }

    while (mGun->NeedsReload())
    {
        int rand = Sexy::AppRand() % mBallColorMap.size();
        ColorMap::iterator it = mBallColorMap.begin();

        for (int i = 0; i < rand; i++)
            it++;

        mGun->Reload(it->first, !GetCircleShootApp()->mMachineGunMode, Sexy::PowerType_Max);
    }
}

void Board::UpdateBallColorMap(Ball *theBall, bool added)
{
    if (added)
    {
        mBallColorMap[theBall->GetType()]++;
    }
    else
    {
        ColorMap::iterator anItr = mBallColorMap.find(theBall->GetType());

        if (anItr != mBallColorMap.end())
        {
            anItr->second--;

            if (anItr->second <= 0)
            {
                mBallColorMap.erase(anItr);
            }
        }
    }
}

static const int kMaxExtraGuides = 5;
static IntPoint gExtraGuides[kMaxExtraGuides][4];
static int gExtraGuideCount = 0;

static int GetBoardMultiFrogCount()
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

static Ball *ComputeAccuracyGuide(Gun *theGun, float theGunAngle, CurveMgr **theCurves, int theNumCurves,
                                  IntPoint *theGuide, SexyVector3 *theEnd, BulletList *theKillers = NULL)
{
    float anAngle = theGunAngle - (SEXY_PI / 2);
    float dx = sinf(anAngle);
    float dy = cosf(anAngle);
    float dx2 = dx;
    float dy2 = dy;
    float dx3 = dx * 16.0f;
    float dy3 = dy * 16.0f;

    const SexyVector3 aCenter(
        (float)theGun->GetCenterX() + dy * 50.0f,
        (float)theGun->GetCenterY() + dx * -50.0f,
        0.0f);

    SexyVector3 g1(aCenter.x - dx3, aCenter.y - dy3, 0.0f);
    SexyVector3 g2(aCenter.x + dx3, aCenter.y + dy3, 0.0f);
    SexyVector3 v1(cosf(anAngle), -sinf(anAngle), 0.0f);
    float t = 10000000.0f;

    Ball *aBall = NULL;
    for (int i = 0; i < theNumCurves; i++)
    {
        Ball *anIntersectBall = theCurves[i]->CheckBallIntersection(g1, v1, t);
        if (anIntersectBall)
            aBall = anIntersectBall;
    }

    for (int i = 0; i < theNumCurves; i++)
    {
        Ball *anIntersectBall = theCurves[i]->CheckBallIntersection(g2, v1, t);
        if (anIntersectBall)
            aBall = anIntersectBall;
    }

    // Killer balls in flight block the shot too, so the guide stops at them.
    if (theKillers != NULL)
    {
        for (BulletList::iterator anItr = theKillers->begin(); anItr != theKillers->end(); ++anItr)
        {
            Bullet *aKiller = *anItr;
            if (!aKiller->IsKillerBall())
                continue;

            float t2;
            if (aKiller->Intersects(g1, v1, t2) && t2 > 0.0f && t2 < t)
            {
                t = t2;
                aBall = aKiller;
            }
            if (aKiller->Intersects(g2, v1, t2) && t2 > 0.0f && t2 < t)
            {
                t = t2;
                aBall = aKiller;
            }
        }
    }

    if (aBall == NULL)
        t = 1000.0f / v1.Magnitude();

    SexyVector3 aGuide = aCenter + v1 * t;
    if (theEnd != NULL)
        *theEnd = aGuide;

    if (theGuide != NULL)
    {
        theGuide[0].mX = g1.x + dx3 * 0.5f;
        theGuide[0].mY = g1.y + dy3 * 0.5f;
        theGuide[1].mX = g2.x + dx3 * -0.5f;
        theGuide[1].mY = g2.y + dy3 * -0.5f;
        theGuide[2].mX = aGuide.x + dx2;
        theGuide[2].mY = aGuide.y + dy2;
        theGuide[3].mX = aGuide.x - dx2;
        theGuide[3].mY = aGuide.y - dy2;
    }

    return aBall;
}

static void DrawAccuracyGuide(Graphics *g, IntPoint *theGuide, bool baseMinimum)
{
    if (baseMinimum)
    {
        int x0 = (theGuide[0].mX + theGuide[1].mX) / 2;
        int y0 = (theGuide[0].mY + theGuide[1].mY) / 2;
        int x1 = (theGuide[2].mX + theGuide[3].mX) / 2;
        int y1 = (theGuide[2].mY + theGuide[3].mY) / 2;
        g->DrawLineAA(x0, y0, x1, y1);
    }
    else
    {
        g->PolyFill(theGuide, 4, false);
    }
}

void Board::UpdateGuide()
{
    SexyVector3 aGuide;
    IntPoint aTmpGuide[4];
    Ball *aBall = ComputeAccuracyGuide(mGun, mGun->GetAngle(), mCurveMgr, mNumCurves, aTmpGuide, &aGuide, &mBulletList);

    mGuideBall = aBall;

    if (!mRecalcGuide && mShowGuide && (mGuideCenter - aGuide).Magnitude() < 20.0f)
        return;

    mGuideCenter = aGuide;
    mShowGuide = true;
    mRecalcGuide = false;
    mGuide[0] = aTmpGuide[0];
    mGuide[1] = aTmpGuide[1];
    mGuide[2] = aTmpGuide[2];
    mGuide[3] = aTmpGuide[3];

    gExtraGuideCount = 0;
    int aFrogCount = GetBoardMultiFrogCount();
    for (int i = 1; i < aFrogCount && gExtraGuideCount < kMaxExtraGuides; i++)
    {
        float aDelta = i * ((SEXY_PI * 2.0f) / (float)aFrogCount);
        ComputeAccuracyGuide(mGun, mGun->GetAngle() + aDelta, mCurveMgr, mNumCurves,
                             gExtraGuides[gExtraGuideCount], NULL, &mBulletList);
        gExtraGuideCount++;
    }
}

int GetTreasurePitch(int x)
{
    return 3000 * (x - 320) / 320;
}

void Board::UpdateTreasure()
{
    if (mTreasureZoomFrame != 0)
    {
        mTreasureZoomFrame++;
        if (mTreasureZoomFrame > 40)
        {
            mTreasureZoomFrame = 0;
        }
    }

    if (mCurTreasure != NULL && !gForceTreasure)
    {
        if (mStateCount > mTreasureEndFrame)
        {
            mApp->PlaySample(Sexy::SOUND_JEWEL_DISAPPEAR, GetTreasurePitch(mCurTreasure->x));
            mCurTreasure = NULL;
        }
        return;
    }

    if (!gForceTreasure)
    {
        if (GetCircleShootApp()->mBankruptMode)
            return;

        if (mScore >= mScoreTarget)
            return;

        if (mLevelDesc->mTreasurePoints.empty())
            return;

        int aRnd = Sexy::AppRand() % mLevelDesc->mTreasureFreq;
        if (aRnd != 0 || mLevelDesc->mTreasureFreq >= mStateCount - mTreasureEndFrame)
            return;
    }

    for (int i = 0; i < mLevelDesc->mTreasurePoints.size() * 2; i++)
    {
        int aTreasureNum;
        if (gForceTreasure)
        {
            aTreasureNum = gForceTreasureNum - 1;
        }
        else
        {
            aTreasureNum = Sexy::AppRand();
        }

        mCurTreasureNum = aTreasureNum % mLevelDesc->mTreasurePoints.size();

        TreasurePointList::iterator anItr = mLevelDesc->mTreasurePoints.begin();
        std::advance(anItr, mCurTreasureNum);

        mCurTreasure = &*anItr;

        if (!gForceTreasure)
        {
            for (int j = 0; j < mNumCurves; j++)
            {
                if (mCurTreasure->mCurveDist[j] == 0)
                    continue;

                float aFarthestBallPercent = mCurveMgr[j]->GetFarthestBallPercent();
                if (mCurTreasure->mCurveDist[j] > aFarthestBallPercent)
                {
                    mCurTreasure = NULL;
                    gForceTreasure = false;
                    return;
                }
            }
        }

        if (mCurTreasure != NULL)
        {
            mApp->PlaySample(Sexy::SOUND_JEWEL_APPEAR, GetTreasurePitch(mCurTreasure->x));
            mTreasureEndFrame = mStateCount + 1000;

            for (int j = 0; j < 360; j += 90)
            {
                uint aColor = gBrightBallColors[Sexy::Rand() % 6];
                int aRand = Sexy::Rand() % 5;

                float y = mCurTreasure->y + sinf(j * SEXY_PI / 180.0) * 4.0f - 15.0f;
                float x = mCurTreasure->x + cosf(j * SEXY_PI / 180.0) * 4.0f + 10.0f;

                mParticleMgr->AddSparkle(x, y, 0.0f, 0.25f, 5, 0, aRand, aColor);
            }
            break;
        }
    }

    gForceTreasure = false;
}

void Board::UpdateColorShift()
{
    CircleShootApp *app = GetCircleShootApp();
    if (app == NULL || !app->mColorShiftMode)
        return;

    float intervalSec = app->mColorShiftHz;
    if (intervalSec < 1.0f)
        intervalSec = 1.0f;
    if (intervalSec > 10.0f)
        intervalSec = 10.0f;

    // Board updates ~100 times per second.
    int interval = (int)(intervalSec * 100.0f + 0.5f);
    if (interval < 1)
        interval = 1;

    if (mStateCount == 0 || (mStateCount % interval) != 0)
        return;

    // Build a map per path. Random shifts stay within the colors that path
    // uses (e.g. no white on a 4-color level); a custom map is the player's
    // choice and is applied as set.
    for (int c = 0; c < mNumCurves; c++)
    {
        int aNumColors = mCurveMgr[c]->GetNumColors();

        int map[MAX_BALL_COLORS];
        for (int i = 0; i < MAX_BALL_COLORS; i++)
            map[i] = i;

        if (app->mColorShiftRandom)
        {
            // Shuffle only the level's colors among themselves.
            for (int i = aNumColors - 1; i > 0; i--)
            {
                int j = Sexy::AppRand() % (i + 1);
                int tmp = map[i];
                map[i] = map[j];
                map[j] = tmp;
            }
        }
        else
        {
            for (int i = 0; i < MAX_BALL_COLORS; i++)
            {
                if (!app->mColorShiftEnabled[i])
                    continue;

                int dest = app->mColorShiftMap[i];
                if (dest < 0 || dest >= MAX_BALL_COLORS)
                    dest = (i + 1) % MAX_BALL_COLORS;
                map[i] = dest;
            }
        }

        mCurveMgr[c]->ApplyColorShift(map);
    }
}

void Board::UpdateInvisible()
{
    CircleShootApp *app = GetCircleShootApp();
    if (app == NULL || !app->mInvisibleMode)
        return;

    float intervalSec = app->mInvisibleIntervalSec;
    if (intervalSec < 0.5f)
        intervalSec = 0.5f;
    if (intervalSec > 10.0f)
        intervalSec = 10.0f;

    float durationSec = app->mInvisibleDurationSec;
    if (durationSec < 0.5f)
        durationSec = 0.5f;
    if (durationSec > 10.0f)
        durationSec = 10.0f;

    // Board updates ~100 times per second.
    int interval = (int)(intervalSec * 100.0f + 0.5f);
    if (interval < 1)
        interval = 1;

    if (mStateCount == 0 || (mStateCount % interval) != 0)
        return;

    int durationFrames = (int)(durationSec * 100.0f + 0.5f);
    if (durationFrames < 1)
        durationFrames = 1;

    int percent = app->mInvisiblePercent;
    if (percent < 4)
        percent = 4;
    if (percent > 100)
        percent = 100;

    for (int i = 0; i < mNumCurves; i++)
        mCurveMgr[i]->ApplyInvisible(durationFrames, percent);
}

void Board::PlayKillerExplosion(float theX, float theY, int theType)
{
    if (theType < 0 || theType >= MAX_BALL_COLORS)
        theType = 0;

    int aColor = Sexy::gBallColors[theType];
    int aX = (int)(theX + 0.5f);
    int aY = (int)(theY + 0.5f);

    mApp->PlaySample(Sexy::SOUND_EXPLODE);
    mParticleMgr->AddExplosion(aX, aY, 0, aColor, 5);

    int step = Sexy::IMAGE_EXPLOSION->mWidth / 3;
    for (int i = step, stagger = 7; i < 100; i += step, stagger += 4)
    {
        float angle = 0.0f;
        do
        {
            mParticleMgr->AddExplosion(
                aX + (Sexy::AppRand() % 21 - 10) + (int)(sinf(angle) * i),
                aY + (Sexy::AppRand() % 21 - 10) + (int)(cosf(angle) * i),
                0,
                aColor,
                stagger);
            angle += ((float)step / (float)i);
        } while (angle < SEXY_PI * 2);
    }

    mParticleMgr->AddExplosion(aX, aY, 0, aColor, 0);
}

void Board::ReleaseKillerBallColor(int theType)
{
    ColorMap::iterator anItr = mBallColorMap.find(theType);
    if (anItr == mBallColorMap.end())
        return;

    anItr->second--;
    if (anItr->second <= 0)
        mBallColorMap.erase(anItr);
}

// Points for taking a killer ball down (by matching shot or bomb blast).
static const int kKillerBallPoints = 100;

void Board::AwardKillerPoints(float theX, float theY, int theType)
{
    if (theType < 0 || theType >= MAX_BALL_COLORS)
        theType = 0;

    // Same text color the normal score popups use for this ball color.
    FloatingTextHelper aFloat;
    aFloat.AddText(StrFormat("+%d", kKillerBallPoints), Sexy::FONT_FLOAT_ID, Sexy::gTextBallColors[theType]);
    aFloat.AddToMgr(mParticleMgr, (int)(theX + 0.5f), (int)(theY + 0.5f));
    IncScore(kKillerBallPoints);
}

// A bomb (power-up or Missile strike) also destroys killer balls in its reach.
// Only killers are erased, so an iterator the caller holds on a player shot stays valid.
void Board::DestroyKillersInBlast(Ball *theCenter, float theScale)
{
    int aPad = CurveMgr::GetBlastPad(theCenter, theScale);

    for (BulletList::iterator anItr = mBulletList.begin(); anItr != mBulletList.end();)
    {
        Bullet *aKiller = *anItr;
        if (!aKiller->IsKillerBall() || !aKiller->CollidesWithPhysically(theCenter, aPad))
        {
            ++anItr;
            continue;
        }

        int aType = aKiller->GetType();
        float aX = aKiller->GetX();
        float aY = aKiller->GetY();
        int aColor = Sexy::gBallColors[aType];

        // The bomb already plays the big blast and sound; add a small burst here.
        mParticleMgr->AddExplosion((int)(aX + 0.5f), (int)(aY + 0.5f), 0, aColor, 5);
        mParticleMgr->AddExplosion((int)(aX + 0.5f), (int)(aY + 0.5f), 0, aColor, 0);

        delete aKiller;
        anItr = mBulletList.erase(anItr);
        ReleaseKillerBallColor(aType);
        AwardKillerPoints(aX, aY, aType);
    }
}

void Board::ResolveKillerCollisions()
{
    for (BulletList::iterator aShotItr = mBulletList.begin(); aShotItr != mBulletList.end();)
    {
        Bullet *aShot = *aShotItr;
        if (aShot->IsKillerBall())
        {
            ++aShotItr;
            continue;
        }

        bool destroyed = false;
        for (BulletList::iterator aKillerItr = mBulletList.begin(); aKillerItr != mBulletList.end(); ++aKillerItr)
        {
            Bullet *aKiller = *aKillerItr;
            if (!aKiller->IsKillerBall() || aKiller == aShot)
                continue;

            if (!aShot->CollidesWithPhysically(aKiller))
                continue;

            // Same color destroys the killer. Colors Ban must NOT end the game here.
            if (aShot->GetType() == aKiller->GetType())
            {
                int aType = aKiller->GetType();
                int aColor = Sexy::gBallColors[aType];
                int aX = (int)(aKiller->GetX() + 0.5f);
                int aY = (int)(aKiller->GetY() + 0.5f);

                // Smaller hit burst — full explosion is only when a killer reaches the frog.
                mApp->PlaySample(Sexy::SOUND_EXPLODE);
                mParticleMgr->AddExplosion(aX, aY, 0, aColor, 5);
                mParticleMgr->AddExplosion(aX, aY, 0, aColor, 0);

                delete aKiller;
                mBulletList.erase(aKillerItr);
                delete aShot;
                aShotItr = mBulletList.erase(aShotItr);
                ReleaseKillerBallColor(aType);
                AwardKillerPoints((float)aX, (float)aY, aType);
                destroyed = true;
                break;
            }
        }

        if (!destroyed)
            ++aShotItr;
    }
}

void Board::UpdateKillerBall()
{
    if (!mApp->mKillerBallMode || mGameState != GameState_Playing || mKillerHitFrog)
        return;

    if (mLevelBeginning || mPauseCount != 0)
        return;

    if (mKillerBallCooldown > 0)
    {
        --mKillerBallCooldown;
        return;
    }

    float flightSec = mApp->mKillerBallFlightSec;
    if (flightSec < 2.0f)
        flightSec = 2.0f;
    if (flightSec > 10.0f)
        flightSec = 10.0f;

    float frogX = (float)mGun->GetCenterX();
    float frogY = (float)mGun->GetCenterY();

    Bullet *killer = NULL;
    if (mNumCurves > 0)
    {
        int start = (int)(Sexy::AppRand() % mNumCurves);
        for (int n = 0; n < mNumCurves && killer == NULL; n++)
        {
            int i = (start + n) % mNumCurves;
            killer = mCurveMgr[i]->LaunchKillerBall(flightSec, frogX, frogY);
        }
    }

    float intervalSec = mApp->mKillerBallIntervalSec;
    if (intervalSec < 2.0f)
        intervalSec = 2.0f;
    if (intervalSec > 15.0f)
        intervalSec = 15.0f;

    if (killer != NULL)
    {
        mBulletList.push_back(killer);
        mApp->PlaySample(Sexy::SOUND_BALLCLICK1);
        mKillerBallCooldown = (int)(intervalSec * 100.0f + 0.5f);
    }
    else
    {
        // No eligible ball yet — retry shortly.
        mKillerBallCooldown = 50;
    }

    if (mKillerBallCooldown < 1)
        mKillerBallCooldown = 1;
}

void Board::UpdateMiscStuff()
{
    bool flag;

    if (mScoreDisplay == mScore)
    {
        flag = false;
    }
    else
    {
        if (mScoreDisplay < mScore)
            mScoreDisplay += ((mScore - mScoreDisplay) / 50.0) + 2.0;

        if (mScoreDisplay > mScore)
            mScoreDisplay = mScore;

        flag = true;
    }

    if (mCurBarSize != mTargetBarSize)
    {
        if (mCurBarSize < mTargetBarSize)
            mCurBarSize++;

        if (mCurBarSize > mTargetBarSize)
            mCurBarSize--;

        flag = true;
    }

    if (mBarBlinkCount > 0)
    {
        mBarBlinkCount--;
        flag = true;
    }

    if (mFlashCount > 0)
    {
        gBallBlink = true;

        if (--mFlashCount == 0)
            gBallBlink = false;

        flag = true;
    }

    if (mLivesBlinkCount > 0)
    {
        mLivesBlinkCount--;
        flag = true;
    }

    if (mAccuracyCount > 0)
    {
        if (--mAccuracyCount == 0)
            DoAccuracy(false);
    }

    if (!flag)
        return;

    MarkDirty();
}

void Board::DrawTreasure(Graphics *g)
{
    if (mApp->mBaseMinimumMode)
    {
        const int kCoinColor = 0xFF8800;
        if (mCurTreasure != NULL)
        {
            g->SetColor(Color(kCoinColor));
            Sexy::FillCircle(g, mCurTreasure->x, mCurTreasure->y, 12);
        }
        if (mTreasureZoomFrame != 0)
        {
            float aScale = 1.0f + mTreasureZoomFrame / 30.0f;
            int r = (int)(12.0f * aScale + 0.5f);
            int a = 255 - 255 * mTreasureZoomFrame / 40;
            if (a < 0)
                a = 0;
            g->SetColor(Color((kCoinColor >> 16) & 0xFF, (kCoinColor >> 8) & 0xFF, kCoinColor & 0xFF, a));
            Sexy::FillCircle(g, mTreasureZoomX, mTreasureZoomY, r);
        }
        return;
    }

    if (mCurTreasure != NULL)
    {
        int x = mCurTreasure->x;
        int y = mCurTreasure->y;
        int v10 = (mStateCount / 4) % 30;

        g->SetColorizeImages(true);
        g->SetColor(Color(0, 0, 0, 96));
        g->DrawImageCel(Sexy::IMAGE_COIN, x - 28, y - 8, v10);
        g->SetColorizeImages(false);
        g->DrawImageCel(Sexy::IMAGE_COIN, x - 18, y - 18, v10);
    }

    if (mTreasureZoomFrame != 0)
    {
        float aScale = 1.0f + mTreasureZoomFrame / 30.0f;
        int x = mTreasureZoomX + aScale * -18.0f;
        int y = mTreasureZoomY + aScale * -18.0f;

        int width = Sexy::IMAGE_COIN->mWidth;
        int height = Sexy::IMAGE_COIN->mHeight / Sexy::IMAGE_COIN->mNumRows;

        int offY = mTreasureZoomFrame + ((mStateCount - mTreasureZoomFrame + 1) / 4);

        Rect rc1(0, height * (offY % 30), width, height);
        Rect rc2(x, y, width * aScale, height * aScale);

        bool hadFastStretch = g->GetFastStretch();

        if (mApp->Is3DAccelerated())
        {
            Rect a3(x + aScale * -10.0f, y + aScale * 10.0f, width * aScale, height * aScale);

            g->SetColorizeImages(true);
            g->SetColor(Color(0, 0, 0, 96 - 96 * mTreasureZoomFrame / 40));
            g->DrawImage(Sexy::IMAGE_COIN, a3, rc1);
            g->SetFastStretch(false);
            g->SetColor(Color(255, 255, 255, 255 - 255 * mTreasureZoomFrame / 40));
        }
        else
        {
            g->SetFastStretch(true);
        }

        g->DrawImage(Sexy::IMAGE_COIN, rc2, rc1);
        g->SetFastStretch(hadFastStretch);
        g->SetColorizeImages(false);
    }
}

void Board::DrawPlaying(Graphics *g)
{
    static BallDrawer aDrawer;

    mSpriteMgr->DrawBackground(g);

    if (mApp->mBaseMinimumMode)
    {
        if (mPauseCount == 0 || mShowBallsDuringPause)
        {
            // Tunnel balls under overlays; balls on top of intersections stay above.
            for (int i = 0; i < mNumCurves; i++)
                mCurveMgr[i]->DrawBallsInTunnel(g, true);
            for (int i = 0; i < mNumCurves; i++)
                mCurveMgr[i]->DrawBaseMinimumOverlays(g);
            for (int i = 0; i < mNumCurves; i++)
                mCurveMgr[i]->DrawBallsInTunnel(g, false);
        }
        else
        {
            for (int i = 0; i < mNumCurves; i++)
                mCurveMgr[i]->DrawBaseMinimumOverlays(g);
        }

        if (mApp->mMovingHoleMode)
        {
            for (int p = 0; p < 5; p++)
                mSpriteMgr->DrawHoles(g, p);
        }
    }
    else
    {
        aDrawer.Reset();

        if (mPauseCount == 0 || mShowBallsDuringPause)
        {
            for (int i = 0; i < mNumCurves; i++)
            {
                mCurveMgr[i]->DrawBalls(aDrawer);
            }
        }

        aDrawer.Draw(g, mSpriteMgr, mParticleMgr);
    }

    mGun->Draw(g);
    DrawTreasure(g);

    if (mShowGuide)
    {
        int anAlpha = 120;

        if (mAccuracyCount <= 300)
        {
            anAlpha = (120 * mAccuracyCount) / 300 + 8;
        }

        g->SetColor(Color(0, 255, 255, anAlpha));
        DrawAccuracyGuide(g, mGuide, mApp->mBaseMinimumMode);
        for (int i = 0; i < gExtraGuideCount; i++)
            DrawAccuracyGuide(g, gExtraGuides[i], mApp->mBaseMinimumMode);
    }

    DrawBullets(g);
}

void Board::DrawLosing(Graphics *g)
{
    mTransitionMgr->Draw(g);
}

void Board::DrawLevelUp(Graphics *g)
{
    mTransitionMgr->Draw(g);
}

void Board::DrawLevelBegin(Graphics *g)
{
    mTransitionMgr->Draw(g);
}

void Board::DrawBalls(Graphics *g)
{
}

void Board::DrawBullets(Graphics *g)
{
    for (BulletList::iterator anItr = mBulletList.begin(); anItr != mBulletList.end(); anItr++)
        (*anItr)->DrawShadow(g);

    for (BulletList::iterator anItr = mBulletList.begin(); anItr != mBulletList.end(); anItr++)
        (*anItr)->Draw(g);
}

void Board::DrawText(Graphics *g)
{
    if (mApp->mBaseMinimumMode)
        return;

    std::string aText;
    bool v19 = false;
    int v20 = 0;

    g->SetFont(Sexy::FONT_DIALOG);

    if ((mLivesBlinkCount & 0x10) == 0)
    {
        int aLives = mLives - 1;

        if (mGameState == GameState_Losing)
        {
            aLives = mLives;
        }

        if (mIsEndless)
        {
            g->SetColor(Color(0xFFFF00));
            aText = "Survival";
            int aTextWidth = Sexy::FONT_DIALOG->StringWidth(aText);
            g->DrawString(aText, 64 - aTextWidth / 2, 20);

            v19 = false;
            v20 = 0;
        }
        else
        {
            if (aLives == 0)
            {
                g->SetColor(Color(0xFFFF00));
                aText = "Last Life";
                int aTextWidth = Sexy::FONT_DIALOG->StringWidth(aText);
                g->DrawString(aText, 64 - aTextWidth / 2, 20);
                v19 = false;
                v20 = 0;
            }
            else if (aLives <= 3)
            {
                v20 = aLives;
                v19 = false;
            }
            else
            {
                v20 = 1;
                v19 = true;
            }
        }

        int aFrogX = 28;
        for (int i = 0; i < v20; i++)
        {
            g->DrawImage(Sexy::IMAGE_FROG_LIVES, aFrogX, 3);
            aFrogX += 26;
        }

        if (v19)
        {
            g->SetColor(Color(0xFFFF00));
            g->SetFont(Sexy::FONT_FLOAT);
            std::string aText = Sexy::StrFormat("x%d", aLives);
            g->DrawString(aText, aFrogX + 4, 22);
            g->SetFont(Sexy::FONT_DIALOG);
        }
    }

    g->SetColor(Color(0xFFFF00));
    int aLvlWidth = Sexy::FONT_DIALOG->StringWidth(mLevelString);
    g->DrawString(mLevelString, 200 - aLvlWidth / 2, 15);

    g->SetFont(Sexy::FONT_MONODIGIT);
    std::string aScoreTxt = Sexy::StrFormat("%d", mScoreDisplay);
    int aScoreWidth = Sexy::FONT_MONODIGIT->StringWidth(aScoreTxt);
    g->DrawString(aScoreTxt, 360 - aScoreWidth, 17);
    g->SetFont(Sexy::FONT_DIALOG);
    g->SetColor(Color(0xFFFF00));

    if (mScoreTarget - mLevelBeginScore > 0)
    {
        Image *aBarImage = Sexy::IMAGE_ZUMA_BAR;
        int aBarWidth = Sexy::IMAGE_ZUMA_BAR->mWidth;

        if ((mBarBlinkCount & 0x20) == 0)
        {
            int v11 = mCurBarSize * aBarWidth;
            int v12 = v11 + 255;
            if (v11 > -1)
            {
                v12 = v11;
            }

            int v13 = v12 >> 8;
            if (aBarWidth <= v13 || mGameState == GameState_LevelUp)
            {
                aBarImage = Sexy::IMAGE_ZUMA_BAR_DONE;
                v13 = aBarWidth;
            }
            else if (mBarBlinkCount != 0)
            {
                aBarImage = Sexy::IMAGE_ZUMA_BAR_DONE;
            }

            Rect aRect(0, 0, v13, aBarImage->mHeight);
            g->DrawImage(aBarImage, 409, 2, aRect);
        }
    }

    if (mShowParTimer && mGameState == GameState_Playing && !mIsEndless)
    {
        int aTime = mLevelDesc->mParTime - mStateCount / 100;
        bool aOverTime = false;

        if (aTime < 0)
        {
            aTime = mStateCount / 100 - mLevelDesc->mParTime;
            aOverTime = true;
        }

        std::string aTimeText = Sexy::GetTimeString(aTime);
        if (aOverTime)
        {
            aTimeText = "-" + aTimeText;
        }

        g->SetFont(Sexy::FONT_MAIN10OUTLINE);
        g->SetColor(Color(0xFFFF00));
        g->DrawString(aTimeText, 580, 460);
    }
}

void Board::DrawOverlay(Graphics *g)
{
    mParticleMgr->DrawTopMost(g);

    if (mPauseCount == 0)
    {
        DrawLevelSwapFade(g);
        return;
    }

    if (!mShowBallsDuringPause && mLives > 0)
    {
        g->SetColor(Color(0, 0, 0, 255 * mPauseFade / 100));
        g->FillRect(0, 0, CIRCLE_WINDOW_WIDTH, CIRCLE_WINDOW_HEIGHT);
    }

    if (mDialogCount == 0)
    {
        g->SetFont(Sexy::FONT_HUGE);

        std::string pauseText = "PAUSED";
        int textY = ((mHeight - Sexy::FONT_HUGE->GetHeight()) / 2) + Sexy::FONT_HUGE->GetAscent() - 20;
        int textWidth = Sexy::FONT_HUGE->StringWidth(pauseText);

        g->SetColor(Color(0xFFFFFF));
        g->DrawString(pauseText, (mWidth - textWidth) / 2, textY);
    }

    DrawLevelSwapFade(g);
}

void Board::Update()
{
    static bool inUpdate = false;

    Widget::Update();

    if (mSlowMo && (mUpdateCnt & 3) != 0)
        return;

    if (gSpeedUp && !inUpdate)
    {
        inUpdate = true;
        for (int i = 0; i < 2; i++)
        {
            Update();
        }
        inUpdate = false;
        return;
    }

    if (mPauseCount != 0)
    {
        if (mApp->GetDialog(DialogType_Stats) != NULL)
        {
            if (mParticleMgr->Update())
            {
                MarkDirty();
            }

            mSoundMgr->Update();
            UpdateMiscStuff();
        }

        if (mPauseFade <= 50)
        {
            mPauseFade++;
            MarkDirty();
        }
    }
    else
    {
        if (mMissileStrikeCooldown > 0)
            mMissileStrikeCooldown--;
        if (mStrikeShakeTicks > 0)
        {
            mStrikeShakeTicks--;
            MarkDirty();
        }

        // The aim screen does not pause play. Close it if the level ends, and
        // keep the mouth ball shown as a bomb after a reload.
        if (mMissileStrikeSelecting)
        {
            if (mGameState != GameState_Playing || !mApp->mMissileStrikeMode)
                SetMissileStrikeSelecting(false);
            else
                ArmMissileStrikeBullet();
        }

        mApp->mUnk29++;
        mGun->Update();
        mSpriteMgr->Update();
        mParticleMgr->Update();
        mSoundMgr->Update();

        mStateCount++;

        MultiUpdateSwitch();

        switch (mGameState)
        {
        case GameState_Playing:
            if (IsMultiGameActive())
                MultiUpdatePlaying();
            else
                UpdatePlaying();
            break;
        case GameState_Losing:
            UpdateLosing();
            break;
        case GameState_LevelUp:
            UpdateLevelUp();
            break;
        case GameState_LevelBegin:
            UpdateLevelBegin();
            break;
        default:
            break;
        }

        if (mDoGuide)
        {
            UpdateGuide();
        }

        UpdateMiscStuff();
    }
}

void Board::Draw(Graphics *g)
{
    if (mApp->mWorkerThread->mTaskProc)
    {
        mApp->mWorkerThread->WaitForTask();
    }

    // Strike earthquake: shift the whole board by a fading wobble. Uses no RNG
    // so the game's random sequence is unaffected.
    int aShakeX = 0;
    int aShakeY = 0;
    if (mStrikeShakeTicks > 0)
    {
        float anAmp = kStrikeShakeAmplitude * (float)mStrikeShakeTicks / (float)kStrikeShakeTicks;
        aShakeX = (int)(anAmp * sinf(mStrikeShakeTicks * 1.9f));
        aShakeY = (int)(anAmp * cosf(mStrikeShakeTicks * 2.7f));
        if (aShakeX != 0 || aShakeY != 0)
        {
            g->SetColor(Color(0, 0, 0));
            g->FillRect(0, 0, mWidth, mHeight);
            g->Translate(aShakeX, aShakeY);
        }
    }

    if (mGameState == GameState_Losing)
    {
        DrawLosing(g);
    }
    else if (mGameState == GameState_LevelUp)
    {
        DrawLevelUp(g);
    }
    else if (mGameState == GameState_LevelBegin)
    {
        DrawLevelBegin(g);
    }
    else
    {
        DrawPlaying(g);
    }

    if (!mApp->mBaseMinimumMode)
        mSpriteMgr->DrawBorder(g);
    DrawText(g);

    // The strike button and aim screen stay still.
    if (aShakeX != 0 || aShakeY != 0)
        g->Translate(-aShakeX, -aShakeY);

    if (mApp->mMissileStrikeMode && mGameState == GameState_Playing)
    {
        if (mMissileStrikeSelecting)
            DrawMissileStrikeOverlay(g);
        DrawMissileStrikeIcon(g);
        if (mMissileStrikeSelecting)
        {
            g->SetColor(Color(255, 255, 255));
            DrawAimRing(g, mStrikeMouseX, mStrikeMouseY, 40.0f, 3);
            DrawAimCrosshair(g, mStrikeMouseX, mStrikeMouseY, 30, Color(255, 70, 50));
        }
    }

    if (IsMultiGameActive() && mGameState == GameState_Playing)
        DrawMultiGameHud(g);
    DrawMultiSwitchFade(g);
}

void Board::MouseMove(int x, int y)
{
    Widget::MouseMove(x, y);

    mStrikeMouseX = x;
    mStrikeMouseY = y;

    if (mPauseCount != 0)
        return;

    int fromCenterX = x - mGun->GetCenterX();
    float angle;

    if (fromCenterX != 0)
    {
        float v4 = atanf((float)(mGun->GetCenterY() - y) / (float)fromCenterX);
        if (fromCenterX < 0)
        {
            v4 += SEXY_PI;
        }

        angle = v4 + (SEXY_PI / 2.0);
    }
    else
    {
        angle = 0.0f;
        if (y < mGun->GetCenterY())
        {
            angle = SEXY_PI;
        }
    }

    mGun->SetAngle(angle);
    mRecalcGuide = true;
    MarkDirty();
}

void Board::MouseDrag(int x, int y)
{
    Widget::MouseDrag(x, y);
    Widget::MouseMove(x, y);
}

void Board::MouseDown(int x, int y, int theClickCount)
{
    Widget::MouseDown(x, y, theClickCount);

    if (mPauseCount != 0)
    {
        Pause(false);
    }
    else if (mPauseUpdateCnt != mUpdateCnt)
    {
        // Multi game: no shooting while fading to another copy.
        if (mMultiSwitchTicks > 0 && mGameState == GameState_Playing)
            return;

        if (mMissileStrikeSelecting)
        {
            // Right-click or clicking the button again backs out at no cost.
            if (theClickCount < 0 || MissileStrikeIconHit(x, y))
            {
                SetMissileStrikeSelecting(false);
                return;
            }

            // The strike fires straight at the click; the cooldown starts here.
            if (FireMissileStrikeBomb(x, y))
                SetMissileStrikeSelecting(false);
            return;
        }

        if (mGameState == GameState_Playing && mApp->mMissileStrikeMode && MissileStrikeIconHit(x, y))
        {
            if (mMissileStrikeCooldown <= 0)
            {
                mStrikeMouseX = x;
                mStrikeMouseY = y;
                SetMissileStrikeSelecting(true);
                mApp->PlaySample(Sexy::SOUND_BUTTON1);
            }
            return;
        }

        if (mGameState == GameState_Playing)
        {
            for (int i = 0; i < mNumCurves; i++)
            {
                if (!mCurveMgr[i]->CanFire())
                    return;
            }

            if (theClickCount >= 0)
            {
                if (mGun->StartFire())
                {
                    mApp->PlaySample(Sexy::SOUND_BALLFIRE);
                }
            }
            else
            {
                mGun->SwapBullets();
            }
        }
        else
        {
            mTransitionMgr->MouseDown(x, y, theClickCount);
        }
    }
}

extern bool gForceStageComplete;

void Board::KeyChar(char theChar)
{
    // Multi game takes over A/D and 1-4 (debug keys otherwise).
    if (MultiHandleKeyChar(theChar))
        return;

    char c = toupper(theChar);
    if (theChar == ' ' && mDialogCount == 0)
    {
        int aTickCount = Sexy::AppGetTickCount();
        if (aTickCount - mLastPauseTick >= 100)
        {
            mLastPauseTick = aTickCount;
            Pause(mPauseCount == 0, false);
        }
    }
    else if (c == 'T')
    {
        mShowParTimer = !mShowParTimer;
        MarkDirty();
    }
    else if (c == 'S')
    {
        mSpriteMgr->ToggleSpaceScroll();
    }
#ifndef RELEASEFINAL
    else if (c == 'P') // TODO THIS IS EXTRA DEBUG FUNCTIONALITY
    {
        mScore = mScoreTarget;
        mCurBarSize = mTargetBarSize;

        for (int i = 0; i < mNumCurves; i++)
        {
            mCurveMgr[i]->DetonateBalls();
        }
    }
    else if (c == 'O') // TODO THIS IS EXTRA DEBUG FUNCTIONALITY
    {
        SetLosing();
    }
	 //LHR DEBUG KEY ADDS
    else if (c == 'B') // Next bullet type, this needs work
    {
        mGun->SetNextBulletType(1);
    }
    else if (c == 'D') // Draw stats, I like the slide compared to AX but if you don't change the first true to false, A in ZR but D is cooler
    {
        mApp->DoStatsDialog(true, false);
    }
    else if (c == 'C') // Custom
    {
        gForceTreasureNum++; //Probably a simple variable increment as the original spawns use RNG, For RNG, set this to Rand(3);
        if (gForceTreasure == 2)
            gForceTreasureNum = 0;
        gForceTreasure = true;
    }
    else if (c == 'E') // Extra life, I like the SFX but if you don't comment those lines (ZD Flash)
    {
        mLives += 1;
        mLivesBlinkCount = 150;
        mApp->PlaySample(Sexy::SOUND_EXTRA_LIFE);
        mSoundMgr->AddSound(Sexy::SOUND_EXTRA_LIFE, 30);
        mSoundMgr->AddSound(Sexy::SOUND_EXTRA_LIFE, 60);
    }
    else if (c == 'F') // Add force stage complete (Custom)
    {
        gForceStageComplete = true;
        DoLevelUp(true, true);
    }
    else if (c == 'G') // Game over cheat why not
    {
        mLives = 1; //Set this to 1 so the right sound plays
        SetLosing();
    }
    else if (c == 'K') // Kill frog, I like K more
    {
        SetLosing();
    }
    else if (c == 'L') // Instant win
    {
        //Board::IncScore(mScoreTarget - mScore);
        DoLevelUp(true, true);
    }
    else if (c == 'M' || c == '[') // Prev level
    {
        if (mLevel > 0)
        {
            mStartLevel = mLevel - 1;
            Reset(false, true);
        }
    }
    else if (c == 'N' || c == ']') // Next level
    {
        Board::DoLevelUp(false, true);
        if (mGameState == GameState_LevelUp)
        {
            Reset(false, false);
        }
    }
    else if (c == 'R') // Reset
    {
        Reset(false, true);
    }
    else if (c == '+') // Score up 1000 from ZR Flash
    {
        mScore += 1000;
        //Board::DoLevelUp(false, true);
    }
    /*else if (c == 'U') //AX upsell
    {

    }*/
    else if (c == '|') //Speed up
    {
        gSpeedUp ^= true;
    }
    else if (c == 'W') // Lose life from AX
    {
        mLives -= 1;
        mLivesBlinkCount = 150;
    }
    else if (c == 'X') //Toggle collision detection from AX
    {
        if (gCheckCollision)
        {
            gCheckCollision = false;
        }
        else
        {
            gCheckCollision = true;
        }
    }
    else if (c == 'Z') // Zuma score from Flash
    {
        IncScore(mScoreTarget - mScore);
    }
    else if (c >= '1' && theChar <= '9') //Ported from Flash, why are they detecting 1-9 were there more powers that got cut?
    {
        if (mGameState == GameState_Playing)
        {
            if (c - 49 >= 0 && c - 49 < PowerType_Max)
                mCurveMgr[Rand(mNumCurves)]->AddPowerUp((PowerType)(c - 49));
        }   
    }
    else if (c == 127) // Delete highlighted ball
    {
        //todo
    }
    else if (c == 'A') // Color override (Custom)
    {
        if (gColorOverride <= gNumColors)
        {
            gColorOverride += 1;
        }
        else
        {
            gColorOverride = 0;
        }
    }
#endif
}

void Board::KeyDown(KeyCode theKey)
{
    Widget::KeyDown(theKey);

    if (IsMultiGameActive() && mGameState == GameState_Playing && mPauseCount == 0 && mDialogCount == 0)
    {
        if (theKey == KEYCODE_LEFT)
            MultiStartSwitch((mMultiView + mMultiCount - 1) % mMultiCount);
        else if (theKey == KEYCODE_RIGHT)
            MultiStartSwitch((mMultiView + 1) % mMultiCount);
    }

    // Esc opens the same MENU dialog as the Menu button.
    if (theKey == KEYCODE_ESCAPE && mDialogCount == 0)
        mApp->DoOptionsDialog();
}

void Board::AddedToManager(WidgetManager *theManager)
{
    Widget::AddedToManager(theManager);

    theManager->AddWidget(mMenuButton);
    mMenuButton->SetVisible(!mApp->mBaseMinimumMode);
    theManager->AddWidget(mContinueButton);
    theManager->AddWidget(mOverlayWidget);
}

void Board::RemovedFromManager(WidgetManager *theManager)
{
    Widget::RemovedFromManager(theManager);

    theManager->RemoveWidget(mMenuButton);
    theManager->RemoveWidget(mContinueButton);
    theManager->RemoveWidget(mOverlayWidget);
}

void Board::ButtonPress(int theId)
{
    mApp->PlaySample(Sexy::SOUND_BUTTON1);
}

void Board::ButtonDepress(int theId)
{
    if (theId == 0)
    {
        mApp->DoOptionsDialog();
    }
    else if (theId == 1)
    {
        mIsWinning = true;

        int aStage = mLevelDesc->mStage + 1;

        mContinueButton->SetVisible(false);

        if (aStage == 13)
        {
            mApp->DoStatsDialog(true, true);
            if (!mApp->mProfile->mHasWon)
            {
                mApp->mProfile->mHasWon = true;
                mApp->SaveProfile();
            }
        }
        else if (aStage == 12 || aStage <= mMaxStage)
        {
            Reset(false, false);
        }
        else
        {
            mVerboseLevelString = Sexy::StrFormat("LeVeL %d-%d", mNextLevelDesc->mStage + 1, mNextLevelDesc->mLevel + 1);
            SaveGame(GetSaveGameName(false, mApp->mProfile->mId));
            mApp->ShowAdventureScreen(false, true);
        }
    }
}

static void PlayPowerSample(CircleShootApp *theApp, int theSound, double theVolume)
{
    if (theApp->mMuteGameSfx)
        return;

    if (theVolume >= 1.0)
    {
        theApp->PlaySample(theSound);
        return;
    }

    SoundInstance *aSound = theApp->mSoundManager->GetSoundInstance(theSound);
    if (aSound != NULL)
    {
        aSound->SetVolume(theVolume);
        aSound->Play(false, true);
    }
}

void Board::ActivatePower(Ball *theBall, double theVolume)
{
    switch (theBall->GetPowerTypeWussy())
    {
    case PowerType_Bomb:
    {
        int aTicks = Sexy::BoardGetTickCount();
        if (aTicks - mLastExplosionTick > 250)
        {
            mLastExplosionTick = aTicks;
            PlayPowerSample(mApp, Sexy::SOUND_EXPLODE, theVolume);
        }
        break;
    }
    case PowerType_MoveBackwards:
        PlayPowerSample(mApp, Sexy::SOUND_BACKWARDS_BALL, theVolume);
        break;
    case PowerType_SlowDown:
        PlayPowerSample(mApp, Sexy::SOUND_SLOWDOWN_BALL, theVolume);
        break;
    case PowerType_Accuracy:
        PlayPowerSample(mApp, Sexy::SOUND_ACCURACY_BALL, theVolume);
        mAccuracyCount = 2000;
        DoAccuracy(true);
        break;
    }

    for (int i = 0; i < mNumCurves; i++)
    {
        mCurveMgr[i]->ActivatePower(theBall);
    }
}

void Board::WaitForLoadingThread()
{
    mApp->mWorkerThread->WaitForTask();
    mApp->ClearUpdateBacklog();
}

void Board::Reset(bool gameOver, bool isLevelReset)
{
    // Multi game: level setup always runs on copy 1; the others are rebuilt below.
    MultiCancelSwitch();
    if (mMultiLoaded != 0)
        MultiExchange(0);
    mMultiView = 0;
    mStoppedAdding = false;

    mStateCount = 0;
    bool aSave = false;
    gBallBlink = 0;

    DoAccuracy(false);
    mSoundMgr->KillAllSounds();
    mContinueButton->SetVisible(false);
    mMenuButton->SetVisible(!mApp->mBaseMinimumMode);

    WaitForLoadingThread();

    mParticleMgr->Clear();
    mTransitionMgr->Clear();
    DeleteBullets();

    if (gameOver)
    {
        mGameStats.Reset();

        mScore = 0;
        mScoreDisplay = 0;
        mCurBarSize = 0;
        mDifficultyDiff = 0;
        mLevel = mStartLevel;
        mLives = 2 * (!mIsEndless) + 1;

        mOldCurveList.clear();
        SyncPracticeMode();

        mApp->DemoAddMarker("NewGame");
        mGun->EmptyBullets();
    }
    else
    {
        if (!isLevelReset)
            mLevel++;

        mApp->DemoAddMarker(Sexy::StrFormat("NewLevel: %d", mLevel));
    }

    mLevelStats.Reset();

    mShowGuide = false;
    mRecalcGuide = false;

    gWasReset = isLevelReset;

    if (isLevelReset)
    {
        GetLevel(mLevel, mLevelDesc, mLevelDesc->mName.c_str());
    }
    else
    {
        if (gameOver)
        {
            SetupNextLevel(mPracticeMode == PracticeMode_Single ? 0 : mLevel);

            if (mPracticeMode == PracticeMode_Single)
            {
                SetLevelToNextLevel();
                mDifficultyDiff = mLevelDesc->mDifficulty;
            }
        }

        if (mPracticeMode == PracticeMode_Single)
        {
            GetLevel(mLevel, mLevelDesc);
            GetLevel(mLevel + 1, mNextLevelDesc);
        }
        else
        {
            SetupNextLevel(mLevel + 1);
        }
    }

    mGun->SetPos(mLevelDesc->mGunX, mLevelDesc->mGunY);
    mBallColorMap.clear();
    mGun->EmptyBullets();

    // After level desc is ready: ban only colors that appear on this level.
    int poolSize = 0;
    if (mLevelDesc != NULL)
    {
        for (int i = 0; i < mLevelDesc->GetNumCurves(); i++)
        {
            int n = mLevelDesc->mCurveDesc[i].mNumColors;
            if (n > poolSize)
                poolSize = n;
        }
    }
    mApp->RollRandomBannedColors(poolSize);

    for (int i = 0; i < mNumCurves; i++)
    {
        mCurveMgr[i]->StartLevel();
    }

    mDestroyAll = true;
    int aScoreTarget = 0;

    for (int i = 0; i < mNumCurves; i++)
    {
        if (aScoreTarget < mLevelDesc->mCurveDesc[i].mScoreTarget)
            aScoreTarget = mLevelDesc->mCurveDesc[i].mScoreTarget;

        if (!mLevelDesc->mCurveDesc[i].mDestroyAll)
            mDestroyAll = false;
    }

    if (aScoreTarget < 0)
    {
        mScoreTarget = 0;
    }
    else
    {
        mScoreTarget = mScore + aScoreTarget;
    }

    if (mIsEndless)
    {
        mScoreTarget = mScore + 1000;
        mNextDifficultyIncFrame = 3000;
    }

    if (!mPracticeBoard.empty())
    {
        mLevelString = Sexy::StrFormat("%s %d", gSmallGauntletStages[mLevelDesc->mStage], mLevelDesc->mLevel + 1);
        mVerboseLevelString = Sexy::StrFormat("%s %d", gGauntletStages[mLevelDesc->mStage], mLevelDesc->mLevel + 1);
    }
    else
    {
        mLevelString = Sexy::StrFormat("LVL %d-%d", mLevelDesc->mStage + 1, mLevelDesc->mLevel + 1);
        mVerboseLevelString = Sexy::StrFormat("LeVeL %d-%d", mLevelDesc->mStage + 1, mLevelDesc->mLevel + 1);
    }

    mHaveReachedTarget = false;
    mLevelBeginScore = mScore;
    mTargetBarSize = 0;
    mCurTreasure = 0;
    mTreasureEndFrame = 0;
    mTreasureZoomX = 0;
    mTreasureZoomY = 0;
    mTreasureZoomFrame = 0;
    mBarBlinkCount = 0;
    mAccuracyCount = 0;
    mLivesBlinkCount = 0;
    mNumClearsInARow = 0;
    mCurInARowBonus = 0;
    mFlashCount = 0;
    mLevelEndFrame = 0;
    mIsWinning = false;

    MultiRebuildExtras();

    mGameState = GameState_LevelBegin;

    mSpriteMgr->ClearHoleFlashes();
    mTransitionMgr->DoLevelBegin(gameOver);

    if (mLevel > mApp->mProfile->mMaxLevel)
    {
        mApp->mProfile->mMaxLevel = mLevel;
        aSave = true;
    }

    if (mPracticeBoard.empty())
    {
        if (mApp->mProfile->UpdateMaxLevel(mLevelDesc->mName, 0))
            aSave = true;

        if (mLevelDesc->mStage > mApp->mProfile->mMaxStage)
        {
            mApp->mProfile->mMaxStage = mLevelDesc->mStage;
            aSave = true;
        }
    }

    if (aSave)
    {
        mApp->SaveProfile();
    }

    mMaxStage = mApp->mProfile->mMaxStage;
}

void Board::Pause(bool pause, bool becauseOfDialog)
{
    if (pause)
    {
        if (mPauseCount == 0)
        {
            mPauseFade = 0;
            mSoundMgr->Mute(true);
        }

        if (becauseOfDialog)
        {
            mDialogCount++;
        }

        mPauseCount++;
    }
    else
    {
        if (becauseOfDialog)
        {
            mDialogCount--;
        }
        mPauseCount--;
    }

    if (mPauseCount == 0)
    {
        mShowBallsDuringPause = false;
        mSoundMgr->Mute(false);
        mPauseUpdateCnt = mUpdateCnt;
        MouseMove(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
    }

    MarkDirty();
}

void Board::SyncState(DataSync &theSync)
{
    DataReader *aReader = theSync.mReader;
    DataWriter *aWriter = theSync.mWriter;

    mGun->SyncState(theSync);
    mParticleMgr->SyncState(theSync);
    mTransitionMgr->SyncState(theSync);
    mSpriteMgr->SyncState(theSync);

    for (int i = 0; i < mNumCurves; i++)
    {
        mCurveMgr[i]->SyncState(theSync);
    }

    theSync.SyncLong(mScore);
    theSync.SyncLong(mScoreDisplay);
    theSync.SyncLong(mStateCount);
    theSync.SyncShort(mLives);
    theSync.SyncLong(mCurBarSize);
    theSync.SyncLong(mTargetBarSize);
    theSync.SyncLong(mBarBlinkCount);
    theSync.SyncLong(mFlashCount);
    theSync.SyncLong(mLivesBlinkCount);
    theSync.SyncLong(mAccuracyCount);
    theSync.SyncLong(mDifficultyDiff);

    if (theSync.mReader)
    {
        theSync.SyncBytes(&mGameState, 4);

#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mGameState = (GameState)ByteSwap(mGameState);
#endif
    }
    else
    {
#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mGameState = (GameState)ByteSwap(mGameState);
#endif

        theSync.SyncBytes(&mGameState, 4);
    }

    theSync.SyncLong(mLastBallClickTick);
    theSync.SyncLong(mLastExplosionTick);
    theSync.SyncBool(mDoGuide);
    theSync.SyncBool(mIsWinning);
    theSync.SyncLong(mTreasureEndFrame);
    theSync.SyncLong(mTreasureZoomX);
    theSync.SyncLong(mTreasureZoomY);
    theSync.SyncLong(mTreasureZoomFrame);
    theSync.SyncLong(mLevelEndFrame);
    theSync.SyncLong(mLevelBeginScore);
    theSync.SyncLong(mScoreTarget);
    theSync.SyncBool(mDestroyAll);
    theSync.SyncLong(mPauseFade);
    theSync.SyncShort(mNumClearsInARow);
    theSync.SyncLong(mCurInARowBonus);
    theSync.SyncBool(mHaveReachedTarget);
    theSync.SyncBool(mLevelBeginning);
    mLevelStats.SyncState(theSync);
    mGameStats.SyncState(theSync);
    theSync.SyncString(mLevelString);
    theSync.SyncString(mVerboseLevelString);
    theSync.SyncLong(mNextDifficultyIncFrame);
    theSync.SyncShort(mMaxStage);
    theSync.SyncBool(mIsEndless);
    theSync.SyncString(mOriginalPracticeBoard);

    if (aReader)
    {
        mGuideBall = NULL;
        mShowGuide = false;
        mRecalcGuide = true;

        DeleteBullets();
        mBallColorMap.clear();

        int aColorMapCount = aReader->ReadShort();
        for (int i = 0; i < aColorMapCount; i++)
        {
            int aColor = aReader->ReadByte();
            int aCount = aReader->ReadShort();
            mBallColorMap[aColor] = aCount;
        }

        int aBulletCount = aReader->ReadShort();
        for (int i = 0; i < aBulletCount; i++)
        {
            Bullet *aBullet = new Bullet();
            aBullet->SyncState(theSync);
            mBulletList.push_back(aBullet);
        }

        mCurTreasureNum = (signed char)aReader->ReadByte();
        if (mCurTreasureNum < 0)
        {
            mCurTreasure = 0;
        }
        else
        {
            TreasurePointList::iterator anItr = mLevelDesc->mTreasurePoints.begin();
            std::advance(anItr, mCurTreasureNum);

            mCurTreasure = &*anItr;
        }

        mOldCurveList.clear();
        int aOldCurveCount = aReader->ReadShort();
        for (int i = 0; i < aOldCurveCount; i++)
        {
            std::string aCurveName;
            aReader->ReadString(aCurveName);
            mOldCurveList.push_back(aCurveName);
        }
    }
    else
    {
        aWriter->WriteShort(mBallColorMap.size());
        for (ColorMap::iterator anItr = mBallColorMap.begin();
             anItr != mBallColorMap.end();
             anItr++)
        {
            aWriter->WriteByte(anItr->first);
            aWriter->WriteShort(anItr->second);
        }

        aWriter->WriteShort(mBulletList.size());
        for (BulletList::iterator anItr = mBulletList.begin();
             anItr != mBulletList.end();
             anItr++)
        {
            (*anItr)->SyncState(theSync);
        }

        signed char aCurTreasureNum = mCurTreasure != NULL ? mCurTreasureNum : -1;
        aWriter->WriteByte((uchar)aCurTreasureNum);

        aWriter->WriteShort(mOldCurveList.size());
        for (LevelParser::StringList::iterator anItr = mOldCurveList.begin();
             anItr != mOldCurveList.end();
             anItr++)
        {
            aWriter->WriteString(*anItr);
        }
    }
}

void Board::SaveGame(const std::string &theFilePath)
{
    WaitForLoadingThread();
    DataWriter aWriter;
    // idk what this is for, some debugging leftover?
    // just gets written to in the original code, but never read anywhere.
    int aUnkState = 0;
    aWriter.OpenMemory(0x20);

    StatsDialog *aDialog = reinterpret_cast<StatsDialog *>(mApp->GetDialog(DialogType_Stats));
    if (aDialog)
        aDialog->FinishScoreInc();

    aWriter.WriteLong(gSaveGameVersion);

    Sexy::LevelDesc *aLevelDesc = mIsWinning ? mNextLevelDesc : mLevelDesc;

    aWriter.WriteBool(!mPracticeBoard.empty());
    aWriter.WriteString(mVerboseLevelString);
    aWriter.WriteString(aLevelDesc->mDisplayName);
    aWriter.WriteLong(mScore);
    aWriter.WriteBool(mApp->mMovingHoleMode);
    aWriter.WriteLong(mApp->mMovingHoleSpeed);
    aWriter.WriteString(mPracticeBoard);
    aWriter.WriteShort(mLevel);
    aWriter.WriteBool(aDialog != NULL);
    aWriter.WriteString(mLevelDesc->mName);
    aWriter.WriteString(mNextLevelDesc->mName);

    {
        DataSync aDataSync(aWriter);
        aUnkState = 1;

        SyncState(aDataSync);
        aDataSync.SyncPointers();
        mApp->WriteBytesToFile(theFilePath, aWriter.mMemoryHandle, aWriter.mMemoryPosition);
        mApp->ClearUpdateBacklog();
        aUnkState = 0;
    }

    aUnkState = -1;
}

// probably not original code, not present in known versions.
void Board::LoadGame(const std::string &theFilePath)
{
    Buffer aBuffer;
    if (mApp->ReadBufferFromFile(theFilePath, &aBuffer))
        LoadGame(aBuffer);
}

void Board::LoadGame(Buffer &theBuffer)
{
    this->mIsSavedGame = true;

    DataReader aReader;
    // Same thing as in Board::SaveGame()
    int aUnkState = 0;
    aReader.OpenMemory(theBuffer.GetDataPtr(), theBuffer.GetDataLen(), false);

    try
    {
        DataSync aDataSync(aReader);
        aUnkState = 2;

        // all of those get their return values ignored for some reason.
        int version = aReader.ReadLong();
        std::string aLevelString;
        aUnkState = 3;

        bool isPractice = aReader.ReadBool();
        aReader.ReadString(aLevelString);
        aReader.ReadString(aLevelString);
        int aScore = aReader.ReadLong();

        // The save stores the Moving hole settings, but the current Modes menu
        // choice wins (like every other mode): turning the mode off and then
        // continuing must not switch it back on. Read the values only to keep
        // the file format. The hole's position is restored below and is ignored
        // when the mode is off.
        if (version >= 7)
        {
            aReader.ReadBool();  // saved Moving hole on/off
            aReader.ReadLong();  // saved Moving hole speed
        }

        aReader.ReadString(mPracticeBoard);
        mLevel = aReader.ReadShort();
        bool isStatsDialogOpen = aReader.ReadBool();

        {
            std::string aLevelName;
            std::string aNextLevelName;
            aUnkState = 5;

            aReader.ReadString(aLevelName);
            aReader.ReadString(aNextLevelName);
            SyncPracticeMode();

            SetupNextLevel(mLevel, aLevelName.c_str());

            if (mPracticeMode == PracticeMode_Single)
            {
                SetLevelToNextLevel();
                GetLevel(mLevel + 1, mNextLevelDesc);
            }
            else
            {
                SetupNextLevel(mLevel + 1, aNextLevelName.c_str());
            }

            // Level geometry must be fully ready before SyncState touches curves/holes.
            WaitForLoadingThread();

            SyncState(aDataSync);
            aDataSync.SyncPointers();

            WaitForLoadingThread();
            for (int i = 0; i < mNumCurves; i++)
                mCurveMgr[i]->RestoreMovingHoleAfterLoad();

            if (mIsWinning && mLevelDesc->mStage < 13)
                Reset(false);
            else if (!mIsWinning && mApp->mMultiGameMode && mApp->mMultiGameCount > 1)
                Reset(false, true); // copies aren't saved: restart this level (score/lives kept)

            if (isStatsDialogOpen)
            {
                mApp->DoStatsDialog(false, false);
                mPauseFade = 50;
            }
        }

        aUnkState = 1;
        mApp->ClearUpdateBacklog();
        aUnkState = 0;
    }
    catch (DataReaderException &)
    {
        // Corrupt / mismatched save — leave a playable board rather than a frozen UI.
        mIsSavedGame = false;
        mApp->ClearUpdateBacklog();
    }

    aUnkState = -1;
}

void Board::IncScore(int theInc, bool delayDisplay)
{
    if (theInc <= 0)
        return;

    int aScore = mScore;
    mScore += theInc;
    if (aScore / 50000 < (aScore + theInc) / 50000 && !mIsEndless && !mIsWinning)
    {
        mLives += (aScore + theInc) / 50000 - aScore / 50000;
        mLivesBlinkCount = 150;
        mApp->PlaySample(Sexy::SOUND_EXTRA_LIFE);
        mSoundMgr->AddSound(Sexy::SOUND_EXTRA_LIFE, 30);
        mSoundMgr->AddSound(Sexy::SOUND_EXTRA_LIFE, 60);
    }

    if (!delayDisplay)
        mScoreDisplay = mScore;
}

bool Board::NeedSaveGame()
{
    if (mIsWinning)
    {
        return mApp->GetDialog(DialogType_Stats) != NULL;
    }

    if (mGameState == GameState_None)
    {
        return false;
    }

    if (mGameState == GameState_Losing)
    {
        return mLives > 0;
    }

    return mScore > 0;
}

bool Board::IsGameOver()
{
    return mGameState == GameState_Losing && mLives <= 0 || mIsWinning;
}

int Board::GetCurrentStage()
{
    return mLevelDesc->mStage;
}

int Board::GetParBonus(int theLevelTime, int theParTime)
{
    if (theLevelTime > theParTime)
    {
        return 0;
    }

    return 100 * int(((1.0f - ((float)theLevelTime / theParTime)) * 25000.0f) / 100) + 100;
}

void Board::ResetInARowBonus()
{
    if (mNumClearsInARow > mLevelStats.mMaxInARow)
    {
        mLevelStats.mMaxInARow = mNumClearsInARow;
        mLevelStats.mMaxInARowScore = mCurInARowBonus;
    }

    mNumClearsInARow = 0;
    mCurInARowBonus = 0;
}

std::string Board::GetUniqueLevelString()
{
    std::string result;
    if (mIsEndless)
        result += "e";

    if (mPracticeBoard.empty())
    {
        result += Sexy::StrFormat("a_%d", mLevel);
    }
    else
    {
        result += Sexy::StrFormat("p_%s_%d_%d", mLevelDesc->mName.c_str(), mLevelDesc->mDifficulty, mLevelDesc->mCurveDesc[0].mNumColors);
    }

    return result;
}

void Board::DoSetupNextLevel()
{
    WaitForLoadingThread();
    mLoadingThreadParam = 0;
    mApp->mWorkerThread->DoTask(Board::StaticLoadProc, (void *)this);
}

void Board::DoGenerateBackgroundTransitions()
{
    WaitForLoadingThread();
    mLoadingThreadParam = 1;

    mApp->mWorkerThread->DoTask(Board::StaticLoadProc, (void *)this);
}

void Board::SetLevelToNextLevel()
{
    WaitForLoadingThread();

    // LevelDesc *anOldLevelDesc = mLevelDesc;
    // mLevelDesc = mNextLevelDesc;
    // mNextLevelDesc = anOldLevelDesc;
    std::swap(mLevelDesc, mNextLevelDesc);

    for (int i = 0; i < 3; i++)
    {
        // CurveMgr *aCurveMgr = mCurveMgr[i];
        // mCurveMgr[i] = mNextCurveMgr[i];
        // mNextCurveMgr[i] = aCurveMgr;
        std::swap(mCurveMgr[i], mNextCurveMgr[i]);
    }

    // SpriteMgr *aOldSprMgr = mSpriteMgr;
    // mSpriteMgr = mNextSpriteMgr;
    // mNextSpriteMgr = aOldSprMgr;
    std::swap(mSpriteMgr, mNextSpriteMgr);

    mNumCurves = mLevelDesc->GetNumCurves();
}

void Board::GetLevel(int theLevel, LevelDesc *theLevelDesc, const char *theLevelName)
{
    int aNumColors = 0;

    if (!mPracticeBoard.empty())
    {
        int aLevel = theLevel % 7;
        int aStage = theLevel / 7;
        int aDifficulty = mDifficultyDiff + aLevel + 2 * aStage;

        for (aNumColors = aStage + 4; aNumColors > 6; --aNumColors)
            aDifficulty += 5;

        if (theLevelName != NULL)
        {
            *theLevelDesc = mApp->mLevelParser->GetLevelByName(theLevelName, aDifficulty);
        }
        else if (mPracticeMode == PracticeMode_Single)
        {
            *theLevelDesc = mApp->mLevelParser->GetLevelByName(mPracticeBoard, aDifficulty);
        }
        else if (mPracticeMode == PracticeMode_Random)
        {
            int aShuffleCount = mApp->mProfile->mMaxBoardLevel.size() - 5;
            if (aShuffleCount > 10)
            {
                aShuffleCount = 10;
            }
            else if (aShuffleCount < 1)
            {
                aShuffleCount = 1;
            }

            while ((int)mOldCurveList.size() > aShuffleCount)
            {
                mOldCurveList.pop_back();
            }

            *theLevelDesc = mApp->mLevelParser->GetRandomBoard(aDifficulty, theLevel, mApp->mProfile, mOldCurveList);
            mOldCurveList.push_front(theLevelDesc->mName);

            if (mIsEndless)
            {
                mPracticeBoard = theLevelDesc->mName;
                mPracticeMode = PracticeMode_Single;
            }
        }
        else
        {
            *theLevelDesc = mApp->mLevelParser->GetSequenceBoard(aDifficulty, theLevel, mApp->mProfile);
        }

        while (aStage > 3)
        {
            aStage--;
            aLevel += 7;
        }

        theLevelDesc->mStage = aStage;
        theLevelDesc->mLevel = aLevel;
        theLevelDesc->mParTime += 10 * aNumColors - 40;
    }
    else
    {
        *theLevelDesc = mApp->mLevelParser->GetStageLevel(theLevel);
        aNumColors = theLevelDesc->mStage / 3 + 4;

        if (aNumColors > 6)
            aNumColors = 6;
    }

    for (int i = 0; i < theLevelDesc->GetNumCurves(); i++)
    {
        theLevelDesc->mCurveDesc[i].mNumColors = aNumColors;
    }
}

void Board::SetupNextLevel(int theLevel, const char *theLevelName)
{
    WaitForLoadingThread();
    SetLevelToNextLevel();
    GetLevel(theLevel, mNextLevelDesc, theLevelName);
    DoSetupNextLevel();
}

void Board::DeleteBullets()
{
    for (BulletList::iterator i = mBulletList.begin(); i != mBulletList.end(); i++)
    {
        delete *i;
    }

    mBulletList.clear();
}

void Board::CheckHighScore() {}

void Board::CreateThumbnail(SpriteMgr *theSpriteMgr, LevelDesc *theLevelDesc) {}