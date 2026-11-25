#ifndef __BOARD_H__
#define __BOARD_H__

#include <SexyAppFramework/ButtonListener.h>
#include <SexyAppFramework/Widget.h>
#include <SexyAppFramework/SexyVector.h>
#include "LevelParser.h"
#include "CurveMgr.h"
#include "SpriteMgr.h"

#include <list>
#include <vector>

namespace Sexy
{

	class Ball;
	class Bullet;
	class Buffer;
	class ButtonWidget;
	class CircleShootApp;
	class CurveMgr;
	class DataSync;
	class Gun;
	class LevelDesc;
	class ParticleMgr;
	class SoundInstance;
	class SoundMgr;
	class SpriteMgr;
	class TransitionMgr;
	class TreasurePoint;
	class WayPointMgr;

	typedef std::list<Ball *> BallList;
	typedef std::list<Bullet *> BulletList;

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
	enum GameState
	{
		GameState_None,
		GameState_Playing,
		GameState_Losing,
		GameState_LevelUp,
		GameState_LevelBegin
	};

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
	enum PracticeMode
	{
		PracticeMode_None,
		PracticeMode_Single,
		PracticeMode_Random,
		PracticeMode_Sequence
	};

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
	struct GameStats
	{
		int mTimePlayed;
		int mNumBallsCleared;
		int mNumGemsCleared;
		int mNumGaps;
		int mNumCombos;
		int mMaxCombo, mMaxComboScore;
		int mMaxInARow, mMaxInARowScore;

		GameStats();
		void Reset();
		void Add(const GameStats &theStats);
		void SyncState(DataSync &theSync);
	};

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
	class Board : public Widget, public ButtonListener
	{
	protected:
		friend class CurveMgr;
		friend class TransitionMgr;
		friend class StatsDialog;

		enum
		{
			MAX_CURVES = 3
		};
		CurveMgr *mCurveMgr[MAX_CURVES];
		CurveMgr *mNextCurveMgr[MAX_CURVES];

		SpriteMgr *mSpriteMgr;
		SpriteMgr *mNextSpriteMgr;
		ParticleMgr *mParticleMgr;
		SoundMgr *mSoundMgr;
		TransitionMgr *mTransitionMgr;

		int mLoadingThreadParam;

		int mNumCurves;

		ButtonWidget *mMenuButton;
		ButtonWidget *mContinueButton;
		Widget *mOverlayWidget;

		TreasurePoint *mCurTreasure;
		int mCurTreasureNum;
		int mTreasureEndFrame;
		int mTreasureZoomX;
		int mTreasureZoomY;
		int mTreasureZoomFrame;

		CircleShootApp *mApp;
		int mScore;
		int mScoreDisplay;
		int mLives;
		int mStateCount;
		GameState mGameState;
		int mPauseCount;
		int mDialogCount;
		int mPauseUpdateCnt;
		int mPauseFade;
		int mFlashCount;
		int mDifficultyDiff;
		int mLevelEndFrame;
		unsigned int mLastBallClickTick;
		unsigned int mLastExplosionTick;
		unsigned int mLastPauseTick;
		bool mSlowMo;
		bool mShowParTimer;
		LevelDesc *mLevelDesc;
		LevelDesc *mNextLevelDesc;
		IntPoint mGuide[4];
		SexyVector3 mGuideCenter;
		Ball *mGuideBall;
		bool mShowGuide;
		bool mDoGuide;
		bool mRecalcGuide;
		int mScoreTarget;
		int mLevelBeginScore;
		int mStartLevel;
		int mCurBarSize;
		int mTargetBarSize;
		int mBarBlinkCount;
		int mLivesBlinkCount;
		int mAccuracyCount;
		bool mDestroyAll;
		bool mHaveReachedTarget;
		bool mLevelBeginning;
		bool mStatsDialogUp;
		bool mIsSavedGame;
		bool mShowBallsDuringPause;
		bool mIsWinning;
		bool mIsEndless;
		int mNumClearsInARow;
		int mCurInARowBonus;
		int mNextDifficultyIncFrame;
		std::string mPracticeBoard;
		std::string mOriginalPracticeBoard;
		PracticeMode mPracticeMode;
		std::list<std::string> mOldCurveList;

		GameStats mLevelStats;
		GameStats mGameStats;
		std::string mLevelString;
		std::string mVerboseLevelString;

		int mNumCleared;
		int mClearedXSum;
		int mClearedYSum;
		int mCurComboCount;
		int mCurComboScore;
		BallList mNeedComboCount;

		typedef std::map<int, int> ColorMap;
		ColorMap mBallColorMap;

		BulletList mBulletList;
		Gun *mGun;
		int mLevel;
		int mMaxStage;
		int mKillerBallCooldown; // frames until next killer launch
		bool mKillerHitFrog;     // lose after this frame; do not SetLosing while iterating bullets

		// Level swap. Cooldown < 0 means not armed yet. Transition counts down
		// from 50 (0.5s): fade out to 0.2s, full black until 0.3s, swap at
		// 0.25s, then fade back in.
		int mLevelSwapCooldown;
		int mLevelSwapTransition;

		// Missile strike button: cooldown ticks left (100/s) and whether the aim screen is open.
		int mMissileStrikeCooldown;
		bool mMissileStrikeSelecting;
		int mStrikeMouseX;
		int mStrikeMouseY;
		// Mouth ball shown as a bomb while aiming, and its power before that.
		Bullet *mMissileStrikeArmedBullet;
		PowerType mMissileStrikeArmedOldPower;
		int mStrikeShakeTicks; // board shake after a strike hits, counts down

		// ---- Multi game: several copies of the level played at once ----
		// The Board's own fields always hold one copy (the "loaded" one); the
		// others wait in mMulti[] and are swapped in to be updated. Outside of
		// MultiUpdatePlaying the loaded copy is the one on screen (mMultiView).
		struct MultiInstance
		{
			CurveMgr *mCurveMgr[MAX_CURVES];
			LevelDesc *mOwnLevelDesc; // extra copies: own level data (paths change it)
			Gun *mGun;
			ParticleMgr *mParticleMgr;
			SoundMgr *mSoundMgr;
			BulletList mBulletList;
			ColorMap mBallColorMap;
			BallList mNeedComboCount;
			SpriteMgr::HoleState mHoles;
			TreasurePoint *mCurTreasure;
			int mCurTreasureNum;
			int mTreasureEndFrame;
			int mTreasureZoomX;
			int mTreasureZoomY;
			int mTreasureZoomFrame;
			IntPoint mGuide[4];
			SexyVector3 mGuideCenter;
			Ball *mGuideBall;
			bool mShowGuide;
			bool mDoGuide;
			bool mRecalcGuide;
			int mAccuracyCount;
			int mNumCleared;
			int mClearedXSum;
			int mClearedYSum;
			int mCurComboCount;
			int mCurComboScore;
			int mNumClearsInARow;
			int mCurInARowBonus;
			bool mLevelBeginning;
			int mLevelEndFrame;
			int mKillerBallCooldown;
			bool mKillerHitFrog;
			bool mStoppedAdding;
			MultiInstance();
		};
		enum { MAX_MULTI_GAMES = 4 };
		MultiInstance mMulti[MAX_MULTI_GAMES];
		LevelDesc *mMultiOwnLevelDesc; // loaded copy's own level data (NULL for copy 1)
		bool mStoppedAdding;           // loaded copy: target-reached stop already applied
		int mMultiCount;               // copies in play (1 = mode off)
		int mMultiLoaded;              // copy whose state is in the Board's fields
		int mMultiView;                // copy on screen
		int mMultiSwitchTicks;         // > 0 while fading to another copy
		int mMultiSwitchTarget;
		bool mMultiBackground;         // updating a copy that is not on screen
		int mMultiEndRequest;          // copy whose loss/win fired in the background (-1 none)
		bool mMultiDanger[MAX_MULTI_GAMES];

		bool IsMultiGameActive() const { return mMultiCount > 1; }
		void MultiSwapLoaded(MultiInstance &theSlot);
		void MultiExchange(int theIndex);
		void MultiDestroyExtras();
		void MultiRebuildExtras();
		void MultiStartLevelCopies();
		void MultiUpdatePlaying();
		bool MultiLoadedInDanger();
		void MultiCheckBackgroundEnd();
		void MultiSetView(int theIndex);
		void MultiStartSwitch(int theIndex);
		void MultiUpdateSwitch();
		void MultiApplyVolume(double theFactor);
		void MultiCancelSwitch();
		bool MultiHandleKeyChar(char theChar);
		void DrawMultiGameHud(Graphics *g);
		void DrawMultiSwitchFade(Graphics *g);

		static void StaticLoadProc(void *theData);
		void LoadProc();

		void DoAccuracy(bool accuracy);
		void PlayBallClick(int theSound);
		void StartLevel();
		void SetLosing();
		void DoLevelUp(bool playSounds = true, bool isCheat = false);
		void CheckEndConditions();
		void SyncPracticeMode();

		void AdvanceFreeBullet(BulletList::iterator &theBulletItr);
		// Returns true if the bullet left mBulletList (iterator already advanced).
		bool AdvancePlayerBulletStep(BulletList::iterator &theBulletItr);
		void UpdateBullets();
		void UpdatePlaying();
		void UpdateLosing();
		void UpdateLevelBegin();
		void UpdateLevelUp();
		void CheckReload();
		void UpdateBallColorMap(Ball *theBall, bool added);
		void UpdateGuide();
		void UpdateTreasure();
		void UpdateColorShift();
		void UpdateInvisible();
		void UpdateKillerBall();
		bool UpdateLevelSwap();
		bool BeginLevelSwap();
		void ClearLevelSwap();
		int LevelSwapIntervalTicks() const;
		void PaintCurvesOntoBackground();
		void DrawLevelSwapFade(Graphics *g);
		void ResolveKillerCollisions();
		void PlayKillerExplosion(float theX, float theY, int theType);
		void ReleaseKillerBallColor(int theType);
		void DestroyKillersInBlast(Ball *theCenter, float theScale);
		void AwardKillerPoints(float theX, float theY, int theType);
		void ArmMissileStrikeBullet();
		void DisarmMissileStrikeBullet();
		bool FireMissileStrikeBomb(int theX, int theY);
		void SetMissileStrikeSelecting(bool selecting);
		void GetMissileStrikeIconCenter(int &theX, int &theY);
		bool MissileStrikeIconHit(int theX, int theY);
		void DrawMissileStrikeIcon(Graphics *g);
		void DrawMissileStrikeOverlay(Graphics *g);
		void UpdateMiscStuff();

		void DrawTreasure(Graphics *g);
		void DrawPlaying(Graphics *g);
		void DrawLosing(Graphics *g);
		void DrawLevelUp(Graphics *g);
		void DrawLevelBegin(Graphics *g);
		void DrawBalls(Graphics *g);
		void DrawBullets(Graphics *g);
		void DrawText(Graphics *g);

		void DoSetupNextLevel();
		void DoGenerateBackgroundTransitions();
		void SetLevelToNextLevel();
		void GetLevel(int theLevel, LevelDesc *theLevelDesc, const char *theLevelName = NULL);
		void SetupNextLevel(int theLevel, const char *theLevelName = NULL);
		void DeleteBullets();
		void CheckHighScore();
		void CreateThumbnail(SpriteMgr *theSpriteMgr, LevelDesc *theLevelDesc);

	public:
		Board(CircleShootApp *theApp);
		virtual ~Board();

		void DrawOverlay(Graphics *g);

		virtual void Update();
		virtual void Draw(Graphics *g);
		virtual void MouseMove(int x, int y);
		virtual void MouseDrag(int x, int y);
		virtual void MouseDown(int x, int y, int theClickCount);
		virtual void KeyChar(char theChar);
		virtual void KeyDown(KeyCode theKey);
		virtual void AddedToManager(WidgetManager *theManager);
		virtual void RemovedFromManager(WidgetManager *theManager);
		virtual void ButtonPress(int theId);
		virtual void ButtonDepress(int theId);

		// theVolume < 1 plays the power-up sound quieter (used for combo clears).
		void ActivatePower(Ball *theBall, double theVolume = 1.0);
		void WaitForLoadingThread();
		void Reset(bool gameOver, bool isLevelReset = false);

		void Pause(bool pause, bool becauseOfDialog = false);
		int GetPauseCount() { return mPauseCount; }
		int GetStateCount() { return mStateCount; }

		void SyncState(DataSync &theSync);
		void SaveGame(const std::string &theFilePath);
		void LoadGame(const std::string &theFilePath);
		void LoadGame(Buffer &theBuffer);
		void IncScore(int theInc, bool delayDisplay = true);
		bool NeedSaveGame();
		bool IsGameOver();
		bool IsWinning() { return mIsWinning; }
		int GetCurrentStage();

		void SetPracticeBoard(const std::string &theBoard)
		{
			mPracticeBoard = theBoard;
			mOriginalPracticeBoard = theBoard;
		}
		void SetStartLevel(int theLevel) { mStartLevel = theLevel; }

		bool IsPracticeMode() { return !mPracticeBoard.empty(); }
		bool IsSingleMode() { return mPracticeMode == PracticeMode_Single; }
		bool IsSavedGame() { return mIsSavedGame; }
		void SetShowBallsDuringPause(bool show) { mShowBallsDuringPause = show; }
		void SetFullPauseFade() { mPauseFade = 50; }
		int GetParBonus(int theLevelTime, int theParTime);
		void SetIsEndless(bool endless) { mIsEndless = endless; }

		void ResetInARowBonus();
		std::string GetUniqueLevelString();
	};

}

#endif //__BOARD_H__