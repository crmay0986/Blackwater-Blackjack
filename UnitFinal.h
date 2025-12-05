//---------------------------------------------------------------------------
#ifndef UnitFinalH
#define UnitFinalH
//---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <FMX.Types.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Objects.hpp>
#include <FMX.Ani.hpp>
#include <FMX.Effects.hpp>

#include <vector>

class TFormMainMenu;
extern PACKAGE TFormMainMenu *FormMainMenu;

class Settings {
public:
    int player_count         = 1;
    int player_initial_chips = 500;
    int goal_amount          = 1000;

    static Settings& getInstance() {
        static Settings instance;
        return instance;
    }

private:
    Settings() = default;
    Settings(const Settings&)            = delete;
    Settings& operator=(const Settings&) = delete;
};

class BJGame;

//---------------------------------------------------------------------------

class TForm1 : public TForm
{
__published:
    TColorAnimation *ColorAnimation1;
	TRectangle *leftPanel;
	TRectangle *rightPanel;
	TRectangle *centerStripe;
	TCircle *topLeftLight;
	TCircle *topRightLight;
	TGlowEffect *GlowEffect1;
	TGlowEffect *GlowEffect2;
	TRectangle *Rectangle1;
	TRectangle *Rectangle2;
	TRectangle *Rectangle3;
	TRectangle *Rectangle4;
    void __fastcall FormShow(TObject *Sender);
    void __fastcall FormHide(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);

private:
    BJGame*  game;

    bool   bettingPhase;
    bool   dealerHoleHidden;

    TLabel*                   dealerLabel;
    std::vector<TImage*>      dealerCardImages;

    TImage*       deckImage;
    TGlowEffect*  deckGlow;

    void __fastcall StartDeckShuffleAnimation();
    void __fastcall StopDeckShuffleAnimation();

    std::vector<TImage*> shuffleCards;
    TTimer*              shuffleCardTimer;

    void CreateShuffleCards();
    void StartShuffleCardTimer();
    void StopShuffleCardTimer();
    void __fastcall ShuffleCardTimerTick(TObject* Sender);

    bool   dealingAnimationActive;
    int    dealPhase;
    int    dealIndex;
    TTimer *dealTimer;

    bool  dealerPeekInProgress;
    bool  dealerPeekBlackjack;

    void  AnimateDealerPeek(bool hasBlackjack);
    void  __fastcall DealerPeekRotateFinished(TObject* Sender);

    TGlowEffect*  chipGlow;

    std::vector<TLabel*> playerNameLabels;
    std::vector<TLabel*> playerInfoLabels;
    std::vector<TLabel*> splitInfoLabels;
    std::vector<TLabel*> mainHandTitleLabels;
    std::vector<TLabel*> splitHandTitleLabels;

    std::vector<TGlowEffect*> playerGlowEffects;
    std::vector<TGlowEffect*> splitGlowEffects;

    std::vector<std::vector<TImage*>> playerCardImagesMain;
    std::vector<std::vector<TImage*>> playerCardImagesSplit;

    std::vector<TButton*> actionButtons;

    TPointF ComputeDealerCardTarget(int cardIndex, int totalCards);
    TPointF ComputePlayerMainCardTarget(int playerIndex, int cardIndex, int totalCards);

    TImage*       betChipImage;
    TLabel*       betHintLabel;
    TButton*      dealButton;
    int           bettingPlayerIndex;
    std::vector<TButton*> betConfirmButtons;
	std::vector<bool>     playerBetConfirmed;
    TImage *betChip1Image;
	TImage *betChip50Image;

    TRectangle*   roundOverPanel;
    TLabel*       roundOverLabel;
	TTimer*       roundOverTimer;
    TRectangle* betHintBackground = nullptr;
	TRectangle* dealerBackground = nullptr;

    bool          gameOverToMainMenu;

    std::vector<TCircle*> confettiPieces;

    std::vector<TRectangle*>       playerNameBackgrounds;

    std::vector<TFloatAnimation*>  playerGlowAnims;
    std::vector<TFloatAnimation*>  splitGlowAnims;

    bool                    collectingCards;
    int                     collectCardIndex;
    TTimer*                 collectTimer;
    std::vector<TImage*>    collectImages;

    void StartCollectCardsAnimation();
    void __fastcall CollectTimerTick(TObject *Sender);

    void CreateGameInstance();
    void StartGame();
    void EndGame();

    void BeginBettingPhase();
    void CreateBetUI();
    void DestroyBetUI();
    void CreateBetConfirmButtons();
    void DestroyBetConfirmButtons();

    void AnimateDealtCardToPlayer(int playerIndex);
    void AnimateDealtCardToDealer();
    void AnimateHitToCurrentHand();

    void AnimateSplitForCurrentHand();
    void __fastcall SplitAnimationFinished(TObject* Sender);

    void CreateDealerLabel();
    void DestroyDealerLabel();
    void CreatePlayerLabels();
    void DestroyPlayerLabels();

    void ClearDealerCardImages();
    void ClearPlayerCardImages();
    void DrawDealerCards();
	void DrawPlayerCards();

	void __fastcall AddChipOutline(TImage* chip);

    void __fastcall Chip1MouseDown(TObject *Sender, TMouseButton Button,
    TShiftState Shift, float X, float Y);
	void __fastcall Chip50MouseDown(TObject *Sender, TMouseButton Button,
    TShiftState Shift, float X, float Y);

    void CreateDeckImage();
    void UpdateDeckGlow(bool hovered);
    void __fastcall DeckMouseEnter(TObject *Sender);
    void __fastcall DeckMouseLeave(TObject *Sender);

    void StartDealingAnimation();
    void __fastcall DealTimerTick(TObject *Sender);

    void __fastcall betChipMouseEnter(TObject *Sender);
    void __fastcall betChipMouseLeave(TObject *Sender);
    void __fastcall HitAnimationFinished(TObject* Sender);

    bool CheckDealerBlackjack();

    void CreatePlayerActionButtons();
    void DestroyPlayerActionButtons();

    void ShowRoundOverOverlay();
    void __fastcall RoundOverTimerTick(TObject *Sender);
    void __fastcall RoundOverFadeOutFinished(TObject *Sender);

    void EndRoundAndCheckGameOver();
    void MarkBankruptPlayers();
	bool AnyActivePlayers();
	bool CheckGoalWinners(String &outText);
	void ShowGameOverBanner(const String& text);
    void CreateConfetti();
    void ClearConfetti();

    void playerHit();
    void playerDoubleDown();
    void playerSplit();
    void playerStand();

    void __fastcall HitButtonClick(TObject *Sender);
    void __fastcall StandButtonClick(TObject *Sender);
    void __fastcall DoubleDownButtonClick(TObject *Sender);
    void __fastcall SplitButtonClick(TObject *Sender);
    void __fastcall ChipMouseDown(TObject* Sender, TMouseButton Button,
                                  TShiftState Shift, float X, float Y);
    void __fastcall BetConfirmButtonClick(TObject* Sender);
    void __fastcall DealButtonClick(TObject *Sender);

    void UpdateAllLabels();

public:
	__fastcall TForm1(TComponent* Owner);
};

//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
