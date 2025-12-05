//---------------------------------------------------------------------------
// PERSONAL INCLUDES
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <chrono>

#include <System.SysUtils.hpp>
#include <System.hpp>

#include <cstdlib>

static int RandInt(int min, int max)
{
    return min + std::rand() % (max - min + 1);
}

static float RandRange(float min, float max)
{
    float t = (float)std::rand() / (float)RAND_MAX;
    return min + (max - min) * t;
}


// APP INCLUDES
#include <fmx.h>
#pragma hdrstop

#include "Unit2.h"
#include "UnitFinal.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
#pragma resource "*.fmx"

// ---------------- ENUMS (renamed to avoid collisions) ----------------

enum class BJSuit : int {
    SuitSpade   = 0,
    SuitHeart   = 1,
    SuitClub    = 2,
    SuitDiamond = 3
};

// Rank values 2–14 (10, J, Q, K, A)
enum class BJRank : int {
    R2 = 2, R3, R4, R5, R6, R7, R8, R9, R10,
    RJ = 11, RQ = 12, RK = 13, RA = 14
};

enum class BJHandStatus { Active, Stood, Busted, Surrendered };

// ---------------- CARD ----------------

class BJCard {
private:
	BJSuit suit;
    BJRank rank;

public:
	BJCard(BJSuit s, BJRank r) : suit(s), rank(r) {}

	static String __fastcall GetCardsFolder();
	static String __fastcall GetCardFileName(const BJCard& c);

    BJSuit getSuit() const { return suit; }
    BJRank getRank() const { return rank; }

    std::string toString() const;
};

// --- BJCard method definitions ---

String __fastcall BJCard::GetCardsFolder()
{
    String exePath   = ExtractFilePath(ParamStr(0));
    String cardsPath = ExpandFileName(exePath + "..\\..\\cards\\");
    return cardsPath;
}

String __fastcall BJCard::GetCardFileName(const BJCard& c)
{
    String rankStr;
    int r = static_cast<int>(c.getRank());

    if (r >= 2 && r <= 10)
        rankStr = IntToStr(r);
    else if (r == 11)
        rankStr = "J";
    else if (r == 12)
        rankStr = "Q";
    else if (r == 13)
        rankStr = "K";
    else if (r == 14)
        rankStr = "A";

    String suitStr;
    switch (c.getSuit()) {
        case BJSuit::SuitClub:    suitStr = "C"; break;
        case BJSuit::SuitDiamond: suitStr = "D"; break;
        case BJSuit::SuitHeart:   suitStr = "H"; break;
        case BJSuit::SuitSpade:   suitStr = "S"; break;
    }

    return rankStr + suitStr + ".png";
}

std::string BJCard::toString() const
{
    static const char* suitNames[] = {
        "Spades", "Hearts", "Clubs", "Diamonds"
    };
    static const char* rankNames[] = {
        "", "", "2","3","4","5","6","7","8","9","10","J","Q","K","A"
    };

    return std::string(rankNames[(int)rank]) + " of " + suitNames[(int)suit];
};

// ---------------- HAND ----------------

class BJHand {
private:
    std::vector<BJCard> hand;
    BJHandStatus status;
public:
    BJHand() : status(BJHandStatus::Active) {}

    void addCard(const BJCard& c) { hand.push_back(c); }
    int  size() const             { return (int)hand.size(); }

    int value() const {
        int value = 0;
        int ace_count = 0;

        for (const auto& card : hand) {
            int r = static_cast<int>(card.getRank());

            if (r <= 10) {
                value += r;
            } else if (r <= 13) {
                value += 10;
            } else {
                ++ace_count;
            }
        }

        while (ace_count > 0) {
            if (21 - value >= 11) {
                value += 11;
            } else {
                value += 1;
            }
            --ace_count;
        }

        return value;
    }

    void clear() {
        hand.clear();
        status = BJHandStatus::Active;
    }

    const std::vector<BJCard>& GetCards() const { return hand; }

    BJHandStatus getStatus() const { return status; }
    void setStatus(BJHandStatus s) { status = s; }

    std::string toString() const {
        std::string output;
        for (const auto& card : hand) {
            output += card.toString() + "\n";
        }
        return output;
    }
};

static String DescribePoints(const BJHand& h)
{
    const auto& cards = h.GetCards();
    int nonAce = 0;
    int aces   = 0;

    for (const auto& card : cards) {
        int r = static_cast<int>(card.getRank());
        if (r == static_cast<int>(BJRank::RA)) {
            ++aces;
        } else if (r >= 2 && r <= 10) {
            nonAce += r;
        } else if (r >= 11 && r <= 13) {
            nonAce += 10;
        }
    }

    int hardTotal = nonAce + aces;
    int softTotal = hardTotal;

    if (aces > 0 && hardTotal + 10 <= 21) {
        softTotal = hardTotal + 10;
        return "Soft " + IntToStr(softTotal);
    }

    return IntToStr(hardTotal);
}

// ---------------- DECK ----------------

class BJDeck {
private:
    std::vector<BJCard> cards;
	int index;

public:
    BJDeck() : index(0) {
        resetDeck();
    }

	void resetDeck() {
        cards.clear();

        for (int s = 0; s < 4; ++s) {
            for (int r = 2; r <= 14; ++r) {
                cards.emplace_back((BJSuit)s, (BJRank)r);
            }
        }

        index = 0;
        shuffleCards();
    }

    void shuffleCards() {
        std::random_device rd;
        auto timeSeed = std::chrono::high_resolution_clock::now()
                            .time_since_epoch().count();

        unsigned int seed =
            static_cast<unsigned int>(rd()) ^
            static_cast<unsigned int>(timeSeed);

        std::mt19937 rng(seed);
        std::shuffle(cards.begin(), cards.end(), rng);
    }

    BJCard DrawCard() {
        if (index >= (int)cards.size()) {
            throw std::runtime_error("Deck empty.");
        }
        return cards[index++];
    }

    void dealCardTo(BJHand& hand) {
        hand.addCard(DrawCard());
    }

    int remaining() const {
        return (int)cards.size() - index;
    }
};

// ---------------- PLAYER ----------------

class BJPlayer {
private:
    int     id;
    BJHand  hand;
	BJHand  splitHand;
    bool    hasSplit;
    int     chips;
    int     currentBet;
    int     splitBet;


	int lastOutcomeMain;
    int lastOutcomeSplit;

	bool bankrupt;


	bool actedMain;
    bool actedSplit;

public:
    BJPlayer(int id, int initial_chips)
        : id(id),
          hand(),
          splitHand(),
          hasSplit(false),
          chips(initial_chips),
          currentBet(0),
          splitBet(0),
          lastOutcomeMain(0),
          lastOutcomeSplit(0),
          bankrupt(false),
          actedMain(false),
          actedSplit(false)
    {}

    int getID() const { return id; }

    BJHand&       GetHand()       noexcept { return hand; }
    const BJHand& GetHand() const noexcept { return hand; }

    BJHand&       GetSplitHand()       noexcept { return splitHand; }
    const BJHand& GetSplitHand() const noexcept { return splitHand; }

    bool hasSplitHand() const noexcept { return hasSplit; }
    void setHasSplit(bool v) noexcept  { hasSplit = v; }

    int  getChips() const        noexcept { return chips; }
    void adjustChips(int amount) noexcept { chips += amount; }

    void setBet(int amount) {
        if (amount < 0) throw std::invalid_argument("Bet cannot be negative");
        currentBet = amount;
    }
    int getBet() const noexcept { return currentBet; }

    void setSplitBet(int amount) {
        if (amount < 0) throw std::invalid_argument("Bet cannot be negative");
        splitBet = amount;
    }
    int getSplitBet() const noexcept { return splitBet; }

    void clearHands() {
        hand.clear();
        splitHand.clear();
        hasSplit = false;

		lastOutcomeMain  = 0;
        lastOutcomeSplit = 0;

        actedMain  = false;
        actedSplit = false;
    }


    void setRoundOutcomeMain(int o)  noexcept { lastOutcomeMain  = o; }
    void setRoundOutcomeSplit(int o) noexcept { lastOutcomeSplit = o; }
    int  getRoundOutcomeMain() const noexcept { return lastOutcomeMain; }
    int  getRoundOutcomeSplit() const noexcept { return lastOutcomeSplit; }


    bool isBankrupt() const noexcept { return bankrupt; }
    void setBankrupt(bool v) noexcept { bankrupt = v; }

	bool hasActedOnHand(int handIndex) const noexcept {
        return (handIndex == 0 ? actedMain : actedSplit);
    }

    void markActionOnHand(int handIndex) noexcept {
        if (handIndex == 0)
            actedMain = true;
        else
            actedSplit = true;
    }
};



// ---------------- DEALER ----------------

class BJDealer {
private:
	BJHand hand;
public:
	BJDealer() : hand() {}

	BJHand&       GetHand()       noexcept { return hand; }
	const BJHand& GetHand() const noexcept { return hand; }

	void clearHand() { hand.clear(); }
};

// ---------------- GAME ----------------

class BJGame {
private:
	BJDeck deck;
	BJDealer dealer;
    std::vector<BJPlayer> players;
    int current_player_index;
    int current_hand_index;

public:
    BJGame(int player_count, int player_initial_chips)
        : deck(), dealer(), players(), current_player_index(0), current_hand_index(0)
    {
        for (int i = 0; i < player_count; ++i) {
            players.emplace_back(i, player_initial_chips);
        }
    }

	BJDeck&       GetDeck()       noexcept { return deck; }
    const BJDeck& GetDeck() const noexcept { return deck; }

    BJDealer&       GetDealer()       noexcept { return dealer; }
    const BJDealer& GetDealer() const noexcept { return dealer; }

    BJPlayer&       GetCurrentPlayer()       { return players[current_player_index]; }
    const BJPlayer& GetCurrentPlayer() const { return players[current_player_index]; }

    BJPlayer&       GetPlayer(int i)       { return players[i]; }
    const BJPlayer& GetPlayer(int i) const { return players[i]; }

    int  getPlayerCount()        const { return (int)players.size(); }
    int  getCurrentPlayerIndex() const { return current_player_index; }
    int  getCurrentHandIndex()   const { return current_hand_index; }

    BJHand&       GetCurrentHand() {
        BJPlayer& p = players[current_player_index];
        return (current_hand_index == 0 ? p.GetHand() : p.GetSplitHand());
    }
    const BJHand& GetCurrentHand() const {
        const BJPlayer& p = players[current_player_index];
        return (current_hand_index == 0 ? p.GetHand() : p.GetSplitHand());
    }

    void startRound() {
        resetForNextRound();

        for (auto& p : players) {
            if (p.isBankrupt() || p.getBet() <= 0)
                continue;
            deck.dealCardTo(p.GetHand());
            deck.dealCardTo(p.GetHand());
        }

        deck.dealCardTo(dealer.GetHand());
        deck.dealCardTo(dealer.GetHand());

        current_player_index = 0;
        current_hand_index   = 0;

        for (int i = 0; i < (int)players.size(); ++i) {
            BJPlayer& p = players[i];
            if (!p.isBankrupt() && p.getBet() > 0 && p.GetHand().size() > 0) {
                current_player_index = i;
                break;
            }
        }
    }

    bool advanceTurn() {
        BJPlayer& p = players[current_player_index];

        if (current_hand_index == 0 &&
            p.hasSplitHand() &&
            p.getSplitBet() > 0 &&
            !p.isBankrupt())
        {
            current_hand_index = 1;
            return true;
        }

        current_hand_index = 0;
        int n = (int)players.size();
        for (int idx = current_player_index + 1; idx < n; ++idx) {
            BJPlayer& np = players[idx];
            if (!np.isBankrupt() && np.getBet() > 0 && np.GetHand().size() > 0) {
                current_player_index = idx;
                return true;
            }
        }
        return false;
    }

    void resolveDealerHand() {
        BJHand& h = dealer.GetHand();
        while (h.value() < 17) {
            deck.dealCardTo(h);
        }
    }

    void resetForNextRound() {
        dealer.clearHand();
        for (auto& p : players) {
            p.clearHands();
        }

        if (deck.remaining() < 40) {
            deck.resetDeck();
        }

        current_player_index = 0;
        current_hand_index   = 0;
    }

    void settleBets() {
        int dealerValue = dealer.GetHand().value();

        for (auto& p : players) {
            p.setRoundOutcomeMain(0);
            p.setRoundOutcomeSplit(0);

            {
                BJHand& h = p.GetHand();
                int bet   = p.getBet();
                int outcome = 0;

                if (bet > 0) {
                    int playerValue = h.value();

                    if (playerValue > 21 && dealerValue <= 21) {
                        outcome = -1;
                    }
                    else if (dealerValue > 21 && playerValue <= 21) {
                        outcome = 1;
                        p.adjustChips(bet * 2);
                    }
                    else if (playerValue > dealerValue && playerValue <= 21) {
                        outcome = 1;
                        p.adjustChips(bet * 2);
                    }
                    else if (playerValue < dealerValue && dealerValue <= 21) {
                        outcome = -1;
                    }
                    else {
                        outcome = 0;
                        p.adjustChips(bet);
                    }
                }

                p.setRoundOutcomeMain(outcome);
            }

            if (p.hasSplitHand()) {
                BJHand& h2 = p.GetSplitHand();
                int bet2   = p.getSplitBet();
                int outcome2 = 0;

                if (bet2 > 0) {
                    int playerValue2 = h2.value();

                    if (playerValue2 > 21 && dealerValue <= 21) {
                        outcome2 = -1;
                    }
                    else if (dealerValue > 21 && playerValue2 <= 21) {
                        outcome2 = 1;
                        p.adjustChips(bet2 * 2);
                    }
                    else if (playerValue2 > dealerValue && playerValue2 <= 21) {
                        outcome2 = 1;
                        p.adjustChips(bet2 * 2);
                    }
                    else if (playerValue2 < dealerValue && dealerValue <= 21) {
                        outcome2 = -1;
                    }
                    else {
                        outcome2 = 0;
                        p.adjustChips(bet2);
                    }
                }

                p.setRoundOutcomeSplit(outcome2);
            }
        }
    }
};


// -------------- Decision Manager ----------------

class BJDecisionManager {
public:
    static bool canAct(const BJPlayer& p, const BJGame& g) {
        if (p.isBankrupt())
            return false;
        if (p.getID() != g.GetCurrentPlayer().getID())
            return false;
        return true;
    }

    static bool canHit(const BJPlayer& p, const BJGame& g) {
        if (!canAct(p, g))
            return false;

        const BJHand& h = (g.getCurrentHandIndex() == 0 ? p.GetHand() : p.GetSplitHand());

        if (h.value() >= 21)
            return false;

        if (g.getCurrentHandIndex() == 0)
            return (p.getBet() > 0);
        else
            return (p.getSplitBet() > 0);
    }

    static bool canStand(const BJPlayer& p, const BJGame& g) {
        if (!canAct(p, g))
            return false;

        const BJHand& h = (g.getCurrentHandIndex() == 0 ? p.GetHand() : p.GetSplitHand());

        if (h.value() <= 0)
            return false;

        if (g.getCurrentHandIndex() == 0)
            return (p.getBet() > 0);
        else
            return (p.getSplitBet() > 0);
    }

            static bool canDoubleDown(const BJPlayer& p, const BJGame& g) {
        if (!canAct(p, g))
            return false;

        int handIndex = g.getCurrentHandIndex();
		const BJHand& h =
			(handIndex == 0 ? p.GetHand() : p.GetSplitHand());


        if (p.hasActedOnHand(handIndex))
            return false;


        if (h.size() != 2)
            return false;

        int bet = (handIndex == 0 ? p.getBet() : p.getSplitBet());
        if (bet <= 0)
            return false;

        return (p.getChips() >= bet);
    }



    static bool canSplit(const BJPlayer& p, const BJGame& g) {
        if (!canAct(p, g))
            return false;

        if (g.getCurrentHandIndex() != 0)
            return false;

        if (p.hasSplitHand())
            return false;

        const BJHand& h = p.GetHand();
        if (h.size() != 2)
            return false;

        const auto& cards = h.GetCards();
        int r1 = static_cast<int>(cards[0].getRank());
        int r2 = static_cast<int>(cards[1].getRank());

        auto rankValue = [](int r) -> int {
            if (r >= 11 && r <= 13) return 10;
            return r;
        };

        if (rankValue(r1) != rankValue(r2))
            return false;

        int bet = p.getBet();
        if (bet <= 0)
            return false;
        if (p.getChips() < bet)
            return false;

        return true;
    }
};



//---------------------------------------------------------------------------
// FORM IMPLEMENTATION
//---------------------------------------------------------------------------

TForm1 *Form1;

//---------------------------------------------------------------------------

__fastcall TForm1::TForm1(TComponent* Owner)
    : TForm(Owner),
      game(nullptr),
      bettingPhase(false),
      dealerHoleHidden(false),
      dealerLabel(nullptr),
      betChipImage(nullptr),
      betChip1Image(nullptr),
      betChip50Image(nullptr),
      betHintLabel(nullptr),
      dealButton(nullptr),
      bettingPlayerIndex(-1),
      roundOverPanel(nullptr),
      roundOverLabel(nullptr),
      roundOverTimer(nullptr),
      dealingAnimationActive(false),
      dealPhase(0),
      dealIndex(0),
      dealTimer(nullptr),
      dealerPeekInProgress(false),
      dealerPeekBlackjack(false),
      deckImage(nullptr),
      deckGlow(nullptr),
      chipGlow(nullptr),
      collectingCards(false),
      collectCardIndex(0),
      collectTimer(nullptr)
{

    gameOverToMainMenu = false;

    roundOverTimer = new TTimer(this);
    roundOverTimer->Enabled  = false;
    roundOverTimer->Interval = 3000;
    roundOverTimer->OnTimer  = RoundOverTimerTick;

    dealTimer = new TTimer(this);
    dealTimer->Enabled  = false;
    dealTimer->Interval = 220;
    dealTimer->OnTimer  = DealTimerTick;

    shuffleCardTimer = new TTimer(this);
    shuffleCardTimer->Enabled = false;
    shuffleCardTimer->Interval = 350;
    shuffleCardTimer->OnTimer = ShuffleCardTimerTick;

    collectTimer = new TTimer(this);
    collectTimer->Enabled  = false;
    collectTimer->Interval = 80;
    collectTimer->OnTimer  = CollectTimerTick;

    dealingAnimationActive = false;
    dealPhase  = 0;
    dealIndex  = 0;

    deckImage = nullptr;
}


//---------------------------------------------------------------------------

void __fastcall TForm1::FormShow(TObject *Sender)
{
    StartGame();
}

//---------------------------------------------------------------------------

void __fastcall TForm1::FormHide(TObject *Sender)
{
    EndGame();
}

//---------------------------------------------------------------------------
// GAME LIFECYCLE
//---------------------------------------------------------------------------

void TForm1::CreateGameInstance() {
    Settings& s = Settings::getInstance();
    game = new BJGame(s.player_count, s.player_initial_chips);
}

void TForm1::StartGame() {
    if (!game) {
        CreateGameInstance();
    }

    CreateDealerLabel();
    CreatePlayerLabels();
    CreateDeckImage();
    BeginBettingPhase();
}

void TForm1::EndGame() {
    DestroyDealerLabel();
    DestroyPlayerLabels();
    DestroyPlayerActionButtons();
    DestroyBetUI();
    DestroyBetConfirmButtons();
    ClearDealerCardImages();
    ClearPlayerCardImages();

    if (deckImage) {
        deckImage->Parent = nullptr;
        deckImage = nullptr;
    }

    if (game) {
        delete game;
        game = nullptr;
    }
}

//---------------------------------------------------------------------------
// BETTING PHASE
//---------------------------------------------------------------------------

void TForm1::BeginBettingPhase()
{
    if (!game) return;

    bettingPhase     = true;
    dealerHoleHidden = false;

    Settings& s = Settings::getInstance();
    int count = s.player_count;

    playerBetConfirmed.assign(count, false);
    bettingPlayerIndex = -1;

    for (int i = 0; i < count; ++i) {
        BJPlayer& p = game->GetPlayer(i);

        p.clearHands();
        p.setSplitBet(0);
        p.setBet(0);

        if (!p.isBankrupt() && bettingPlayerIndex == -1) {
            bettingPlayerIndex = i;
        }
    }

    game->GetDealer().clearHand();

    ClearDealerCardImages();
    ClearPlayerCardImages();

    DestroyPlayerActionButtons();

    CreateBetUI();
    CreateBetConfirmButtons();

    if (betHintBackground)
        betHintBackground->Visible = true;

    if (dealButton) {
        dealButton->Enabled = false;
        dealButton->Visible = true;
    }
    if (betChipImage) {
        betChipImage->Visible = true;
        betChipImage->HitTest = (bettingPlayerIndex != -1);
        betChipImage->Opacity = (bettingPlayerIndex != -1 ? 1.0f : 0.5f);
    }
    if (betChip1Image) {
        betChip1Image->Visible = true;
        betChip1Image->HitTest = (bettingPlayerIndex != -1);
        betChip1Image->Opacity = (bettingPlayerIndex != -1 ? 1.0f : 0.5f);
    }
    if (betChip50Image) {
        betChip50Image->Visible = true;
        betChip50Image->HitTest = (bettingPlayerIndex != -1);
        betChip50Image->Opacity = (bettingPlayerIndex != -1 ? 1.0f : 0.5f);
    }

	UpdateAllLabels();

    StartDeckShuffleAnimation();
}



//---------------------------------------------------------------------------
// CHIP HOVER GLOW (BETTING)
//---------------------------------------------------------------------------

void __fastcall TForm1::betChipMouseEnter(TObject *Sender)
{
    if (!chipGlow) return;

    TImage* img = dynamic_cast<TImage*>(Sender);
    if (!img) return;

    chipGlow->Parent  = img;
    chipGlow->Enabled = true;
}


void __fastcall TForm1::betChipMouseLeave(TObject *Sender)
{
    if (chipGlow)
        chipGlow->Enabled = false;
}


//---------------------------------------------------------------------------
// DECK IMAGE (TOP-RIGHT) + HOVER GLOW
//---------------------------------------------------------------------------

void TForm1::CreateDeckImage()
{
    if (deckImage) return;

    deckImage = new TImage(this);
    deckImage->Parent   = this;
    deckImage->Width    = 90;
    deckImage->Height   = 130;
    deckImage->WrapMode = TImageWrapMode::Fit;

    float margin = 20.f;
    deckImage->Position->X = ClientWidth  - deckImage->Width  - margin;
    deckImage->Position->Y = margin;

    deckImage->HitTest      = true;
    deckImage->OnMouseEnter = DeckMouseEnter;
    deckImage->OnMouseLeave = DeckMouseLeave;

    try {
        String folder = BJCard::GetCardsFolder();
        deckImage->Bitmap->LoadFromFile(folder + "back.png");
    } catch (...) {}

    deckGlow = new TGlowEffect(this);
    deckGlow->Parent    = deckImage;
    deckGlow->GlowColor = TAlphaColorRec::White;
    deckGlow->Softness  = 0.8f;
    deckGlow->Opacity   = 0.7f;
    deckGlow->Enabled   = false;

    CreateShuffleCards();
}

void TForm1::UpdateDeckGlow(bool hovered)
{
    if (!deckGlow) return;
    deckGlow->Enabled = hovered;
}

void __fastcall TForm1::DeckMouseEnter(TObject *Sender)
{
    UpdateDeckGlow(true);
}

void __fastcall TForm1::DeckMouseLeave(TObject *Sender)
{
    UpdateDeckGlow(false);
}

//---------------------------------------------------------------------------
// DEALER LABEL
//---------------------------------------------------------------------------

void TForm1::CreateDealerLabel() {
    if (dealerLabel) return;

    dealerLabel = new TLabel(this);
    dealerLabel->Parent = this;

    dealerLabel->Width  = 200;
    dealerLabel->Height = 80;

    dealerLabel->StyledSettings = TStyledSettings();
    dealerLabel->TextSettings->Font->Family = "Cooper";
    dealerLabel->TextSettings->Font->Size   = 18;
    dealerLabel->TextSettings->HorzAlign    = TTextAlign::Center;

    dealerLabel->Text = "Dealer";

    dealerLabel->Position->X = (ClientWidth - dealerLabel->Width) * 0.5f;
    dealerLabel->Position->Y = ClientHeight * 0.05f;
}

void TForm1::DestroyDealerLabel()
{
    if (dealerLabel) {
        dealerLabel->Parent = nullptr;
        dealerLabel = nullptr;
    }
}


//---------------------------------------------------------------------------
// PLAYER LABELS
//---------------------------------------------------------------------------

void TForm1::CreatePlayerLabels() {
    DestroyPlayerLabels();

    Settings& s = Settings::getInstance();
    int count = s.player_count;

    playerNameLabels.resize(count, nullptr);
    playerInfoLabels.resize(count, nullptr);
    splitInfoLabels.resize(count, nullptr);
    mainHandTitleLabels.resize(count, nullptr);
    splitHandTitleLabels.resize(count, nullptr);
    playerGlowEffects.resize(count, nullptr);
    splitGlowEffects.resize(count, nullptr);
    playerNameBackgrounds.resize(count, nullptr);
    playerGlowAnims.resize(count, nullptr);
    splitGlowAnims.resize(count, nullptr);

    for (int i = 0; i < count; ++i) {
        TLabel* nameLbl = new TLabel(this);
        nameLbl->Parent = this;
        nameLbl->StyledSettings = TStyledSettings();
        nameLbl->TextSettings->Font->Family = "Cooper";
        nameLbl->TextSettings->Font->Size   = 20;
        nameLbl->TextSettings->HorzAlign    = TTextAlign::Center;
        nameLbl->Text = "Player " + IntToStr(i + 1);
        playerNameLabels[i] = nameLbl;

        TRectangle* nameBg = new TRectangle(this);
        nameBg->Parent = this;
        nameBg->Fill->Kind  = TBrushKind::Solid;
        nameBg->Fill->Color = TAlphaColorRec::Black;
        nameBg->Opacity     = 0.4f;
        nameBg->Stroke->Kind = TBrushKind::None;
        nameBg->XRadius = 12;
        nameBg->YRadius = 12;
        nameBg->Visible = false;
        playerNameBackgrounds[i] = nameBg;

        TLabel* infoLbl = new TLabel(this);
        infoLbl->Parent = this;
        infoLbl->StyledSettings = TStyledSettings();
        infoLbl->TextSettings->Font->Family = "Cooper";
        infoLbl->TextSettings->Font->Size   = 16;
        infoLbl->TextSettings->HorzAlign    = TTextAlign::Center;
        infoLbl->Visible = false;
        playerInfoLabels[i] = infoLbl;

        TLabel* splitInfo = new TLabel(this);
        splitInfo->Parent = this;
        splitInfo->StyledSettings = TStyledSettings();
        splitInfo->TextSettings->Font->Family = "Cooper";
        splitInfo->TextSettings->Font->Size   = 16;
        splitInfo->TextSettings->HorzAlign    = TTextAlign::Center;
        splitInfo->Visible = false;
        splitInfoLabels[i] = splitInfo;

        TLabel* mainTitle = new TLabel(this);
        mainTitle->Parent = this;
        mainTitle->StyledSettings = TStyledSettings();
        mainTitle->TextSettings->Font->Family = "Cooper";
        mainTitle->TextSettings->Font->Size   = 14;
        mainTitle->TextSettings->HorzAlign    = TTextAlign::Center;
        mainTitle->Text = "Hand 1";
        mainTitle->Visible = false;
        mainHandTitleLabels[i] = mainTitle;

        TLabel* splitTitle = new TLabel(this);
        splitTitle->Parent = this;
        splitTitle->StyledSettings = TStyledSettings();
        splitTitle->TextSettings->Font->Family = "Cooper";
        splitTitle->TextSettings->Font->Size   = 14;
        splitTitle->TextSettings->HorzAlign    = TTextAlign::Center;
        splitTitle->Text = "Hand 2";
        splitTitle->Visible = false;
        splitHandTitleLabels[i] = splitTitle;

        TGlowEffect* mainGlow = new TGlowEffect(this);
        mainGlow->Parent  = infoLbl;
        mainGlow->Enabled = false;
        mainGlow->Opacity = 0.85f;
        mainGlow->Softness = 0.6f;
        mainGlow->GlowColor = TAlphaColorRec::Gold;
        playerGlowEffects[i] = mainGlow;

        TFloatAnimation* mainPulse = new TFloatAnimation(mainGlow);
        mainPulse->Parent        = mainGlow;
        mainPulse->PropertyName  = "Opacity";
        mainPulse->StartValue    = 0.4f;
        mainPulse->StopValue     = 0.9f;
        mainPulse->Duration      = 0.8f;
        mainPulse->AutoReverse   = true;
        mainPulse->Loop          = true;
        mainPulse->Enabled       = false;
        playerGlowAnims[i]       = mainPulse;

        TGlowEffect* splitGlow = new TGlowEffect(this);
        splitGlow->Parent  = splitInfo;
        splitGlow->Enabled = false;
        splitGlow->Opacity = 0.85f;
        splitGlow->Softness = 0.6f;
        splitGlow->GlowColor = TAlphaColorRec::Gold;
        splitGlowEffects[i] = splitGlow;

        TFloatAnimation* splitPulse = new TFloatAnimation(splitGlow);
        splitPulse->Parent        = splitGlow;
        splitPulse->PropertyName  = "Opacity";
        splitPulse->StartValue    = 0.4f;
        splitPulse->StopValue     = 0.9f;
        splitPulse->Duration      = 0.8f;
        splitPulse->AutoReverse   = true;
        splitPulse->Loop          = true;
        splitPulse->Enabled       = false;
        splitGlowAnims[i]         = splitPulse;
    }
}

void TForm1::DestroyPlayerLabels()
{
    for (auto* lbl : playerNameLabels) {
        if (lbl) lbl->Parent = nullptr;
    }
    for (auto* lbl : playerInfoLabels) {
        if (lbl) lbl->Parent = nullptr;
    }
    for (auto* lbl : splitInfoLabels) {
        if (lbl) lbl->Parent = nullptr;
    }
    for (auto* lbl : mainHandTitleLabels) {
        if (lbl) lbl->Parent = nullptr;
    }
    for (auto* lbl : splitHandTitleLabels) {
        if (lbl) lbl->Parent = nullptr;
    }
    for (auto* r : playerNameBackgrounds) {
        if (r) r->Parent = nullptr;
    }
    for (auto* a : playerGlowAnims) {
        if (a) a->Parent = nullptr;
    }
    for (auto* a : splitGlowAnims) {
        if (a) a->Parent = nullptr;
    }

    playerNameLabels.clear();
    playerInfoLabels.clear();
    splitInfoLabels.clear();
    mainHandTitleLabels.clear();
    splitHandTitleLabels.clear();
    playerNameBackgrounds.clear();
    playerGlowAnims.clear();
    splitGlowAnims.clear();
    playerGlowEffects.clear();
    splitGlowEffects.clear();
}



void TForm1::ClearPlayerCardImages()
{
    for (auto& row : playerCardImagesMain) {
        for (auto* img : row) {
            if (!img) continue;
            img->Parent = nullptr;
        }
        row.clear();
    }
    playerCardImagesMain.clear();

    for (auto& row : playerCardImagesSplit) {
        for (auto* img : row) {
            if (!img) continue;
            img->Parent = nullptr;
        }
        row.clear();
    }
    playerCardImagesSplit.clear();
}



//---------------------------------------------------------------------------
// CARD IMAGE HELPERS
//---------------------------------------------------------------------------

void TForm1::ClearDealerCardImages()
{
    for (auto* img : dealerCardImages) {
        if (!img) continue;
        img->Parent = nullptr;
    }
    dealerCardImages.clear();
}

static void GetDeckPosition(TImage* deckImage, float& x, float& y)
{
    if (deckImage) {
        x = deckImage->Position->X;
        y = deckImage->Position->Y;
    } else {
        x = 800.f;
        y = 20.f;
    }
}

//---------------------------------------------------------------------------
// CARD TARGET POSITIONS
//---------------------------------------------------------------------------

TPointF TForm1::ComputeDealerCardTarget(int cardIndex, int totalCards)
{
    TPointF pt;

    const float cardW   = 120.f;
    const float cardH   = 170.f;
    const float spacing = cardW + 12.f;

    float centerX;
    float baseY;

    if (dealerLabel) {
        centerX = dealerLabel->Position->X + dealerLabel->Width * 0.5f;
        baseY   = dealerLabel->Position->Y + dealerLabel->Height + 20.f;
    } else {
        centerX = ClientWidth  * 0.5f;
        baseY   = ClientHeight * 0.15f;
    }

    float totalWidth = cardW + (totalCards - 1) * spacing;
    float startX     = centerX - totalWidth * 0.5f;

    pt.X = startX + cardIndex * spacing;
    pt.Y = baseY;
    return pt;
}

TPointF TForm1::ComputePlayerMainCardTarget(int playerIndex, int cardIndex, int totalCards)
{
    TPointF pt;

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;

    const float cardW   = 90.f;
    const float cardH   = 130.f;
    const float fanSpacing   = cardW * 0.35f;
    const float baseYOffset  = 130.f;

    float buttonsY = ClientHeight - 80.f;
    float baseY    = buttonsY - cardH - baseYOffset;

    float extraPlayerSpacing = 40.f;

    float baseCenterX = (playerIndex + 1) * (ClientWidth / (playerCount + 1.0f));
    float centerX = baseCenterX;
    if (playerCount > 1) {
        float offset = (playerIndex - (playerCount - 1) / 2.0f) * extraPlayerSpacing;
        centerX += offset;
    }

    float totalWidth = cardW + (totalCards - 1) * fanSpacing;
    float startX     = centerX - totalWidth * 0.5f;

    pt.X = startX + cardIndex * fanSpacing;
    pt.Y = baseY;
    return pt;
}

//---------------------------------------------------------------------------
// DRAW DEALER CARDS
//---------------------------------------------------------------------------

void TForm1::DrawDealerCards() {
    if (!game) return;

    ClearDealerCardImages();

    BJHand& h = game->GetDealer().GetHand();
    const auto& cards = h.GetCards();
    if (cards.empty()) return;
    int dealerValue = h.value();
    bool dealerIs21 = (dealerValue == 21 && !dealerHoleHidden);

    String folder = BJCard::GetCardsFolder();
    int count = (int)cards.size();

    const float cardW = 120.f;
    const float cardH = 170.f;

    for (int i = 0; i < count; ++i) {
        TPointF pos = ComputeDealerCardTarget(i, count);

        TImage* img = new TImage(this);
        img->Parent = this;
        img->Width  = cardW;
        img->Height = cardH;
        img->WrapMode = TImageWrapMode::Fit;

        try {
            if (dealerHoleHidden && i == 1) {
                img->Bitmap->LoadFromFile(folder + "back.png");
            } else {
                img->Bitmap->LoadFromFile(folder + BJCard::GetCardFileName(cards[i]));
            }
        }
        catch (...) {}

        img->Position->X = pos.X;
        img->Position->Y = pos.Y;
        img->BringToFront();

        TShadowEffect* shadow = new TShadowEffect(this);
        shadow->Parent       = img;
        shadow->ShadowColor  = TAlphaColorRec::Black;
        shadow->Opacity      = 0.6f;
        shadow->Softness     = 0.7f;
        shadow->Distance     = 5;

        if (dealerIs21) {
            TGlowEffect* g = new TGlowEffect(this);
            g->Parent      = img;
            g->GlowColor   = TAlphaColorRec::Lime;
            g->Softness    = 0.9f;
            g->Opacity     = 0.8f;
            g->Enabled     = true;
        }

        dealerCardImages.push_back(img);
    }
}

//---------------------------------------------------------------------------
// DEALER BLACKJACK PEEK
//---------------------------------------------------------------------------

bool TForm1::CheckDealerBlackjack()
{
    if (!game) return false;

    BJHand& dh = game->GetDealer().GetHand();
    const auto& cards = dh.GetCards();
    if (cards.size() < 2) return false;

    const BJCard& up = cards[0];

    int upRank = static_cast<int>(up.getRank());
    bool upIsTenLike =
        (upRank == static_cast<int>(BJRank::R10)) ||
        (upRank == static_cast<int>(BJRank::RJ))  ||
        (upRank == static_cast<int>(BJRank::RQ))  ||
        (upRank == static_cast<int>(BJRank::RK));
    bool upIsAce = (upRank == static_cast<int>(BJRank::RA));

    if (!upIsTenLike && !upIsAce)
        return false;

    bool isBlackjack = (dh.value() == 21);

    if (dealerCardImages.size() < 2 || dealerCardImages[1] == nullptr) {
        if (isBlackjack) {
            dealerHoleHidden = false;
            UpdateAllLabels();
            game->settleBets();
            ShowRoundOverOverlay();
        }
        return isBlackjack;
    }

    dealerPeekBlackjack  = isBlackjack;
    dealerPeekInProgress = true;

    AnimateDealerPeek(isBlackjack);

    return isBlackjack;
}



//---------------------------------------------------------------------------
// DRAW PLAYER CARDS
//---------------------------------------------------------------------------

void TForm1::DrawPlayerCards()
{
    if (!game) return;

    ClearPlayerCardImages();

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;

    playerCardImagesMain.resize(playerCount);
    playerCardImagesSplit.resize(playerCount);

    const float cardW   = 90.f;
    const float cardH   = 130.f;

    const float fanSpacing   = cardW * 0.35f;
    const float stackOffsetY = 25.f;

    float buttonsY = ClientHeight - 80.f;
    float baseYOffset = 130.f;

    float extraPlayerSpacing = 40.f;

    String folder = BJCard::GetCardsFolder();

    for (int i = 0; i < playerCount; ++i) {
        BJPlayer& p  = game->GetPlayer(i);
        BJHand&  mainH  = p.GetHand();
        BJHand&  splitH = p.GetSplitHand();

        int mainVal  = mainH.value();
        int splitVal = splitH.value();
        bool mainIs21  = (mainVal == 21);
        bool splitIs21 = (splitVal == 21);

        const auto& mainCards  = mainH.GetCards();
        const auto& splitCards = splitH.GetCards();

        bool hasSplit = p.hasSplitHand();

        float baseCenterX = (i + 1) * (ClientWidth / (playerCount + 1.0f));

        float centerX = baseCenterX;
        if (playerCount > 1) {
            float offset = (i - (playerCount - 1) / 2.0f) * extraPlayerSpacing;
            centerX += offset;
        }

        float baseY = buttonsY - cardH - baseYOffset;

        if (i < (int)playerNameLabels.size() && playerNameLabels[i]) {
            TLabel* nameLbl = playerNameLabels[i];
            nameLbl->AutoSize = true;
            nameLbl->ApplyStyleLookup();
            nameLbl->Position->X = centerX - nameLbl->Width * 0.5f;
            nameLbl->Position->Y = baseY - nameLbl->Height - 10.f;
        }

        float mainCenterX  = centerX;
        float splitCenterX = centerX;

        if (hasSplit) {
            const float pairGap = 35.f;
            mainCenterX  = centerX - (cardW + pairGap) * 0.5f;
            splitCenterX = centerX + (cardW + pairGap) * 0.5f;
        }

        if (!hasSplit) {
            if (!mainCards.empty()) {
                int mainCount = (int)mainCards.size();

                for (int c = 0; c < mainCount; ++c) {
                    TPointF pos = ComputePlayerMainCardTarget(i, c, mainCount);

                    TImage* img = new TImage(this);
                    img->Parent = this;
                    img->Width  = cardW;
                    img->Height = cardH;
                    img->Position->X = pos.X;
                    img->Position->Y = pos.Y;
                    img->WrapMode    = TImageWrapMode::Fit;

                    try {
                        img->Bitmap->LoadFromFile(folder + BJCard::GetCardFileName(mainCards[c]));
                    } catch (...) {}

                    img->BringToFront();

                    TShadowEffect* shadow = new TShadowEffect(this);
                    shadow->Parent       = img;
                    shadow->ShadowColor  = TAlphaColorRec::Black;
                    shadow->Opacity      = 0.6f;
                    shadow->Softness     = 0.7f;
                    shadow->Distance     = 5;

                    if (mainIs21) {
                        TGlowEffect* g = new TGlowEffect(this);
                        g->Parent      = img;
                        g->GlowColor   = TAlphaColorRec::Lime;
                        g->Softness    = 0.9f;
                        g->Opacity     = 0.8f;
                        g->Enabled     = true;
                    }

                    playerCardImagesMain[i].push_back(img);
                }
            }
        }
        else {
            if (!mainCards.empty()) {
                for (int c = 0; c < (int)mainCards.size(); ++c) {
                    TImage* img = new TImage(this);
                    img->Parent = this;
                    img->Width  = cardW;
                    img->Height = cardH;

                    img->Position->X = mainCenterX - cardW * 0.5f;
                    img->Position->Y = baseY + c * stackOffsetY;

                    img->WrapMode    = TImageWrapMode::Fit;

                    try {
                        img->Bitmap->LoadFromFile(folder + BJCard::GetCardFileName(mainCards[c]));
                    } catch (...) {}

                    img->BringToFront();

                    TShadowEffect* shadow = new TShadowEffect(this);
                    shadow->Parent       = img;
                    shadow->ShadowColor  = TAlphaColorRec::Black;
                    shadow->Opacity      = 0.6f;
                    shadow->Softness     = 0.7f;
                    shadow->Distance     = 5;

                    if (mainIs21) {
                        TGlowEffect* g = new TGlowEffect(this);
                        g->Parent      = img;
                        g->GlowColor   = TAlphaColorRec::Lime;
                        g->Softness    = 0.9f;
                        g->Opacity     = 0.8f;
                        g->Enabled     = true;
                    }

                    playerCardImagesMain[i].push_back(img);
                }
            }

            if (!splitCards.empty()) {
                for (int c = 0; c < (int)splitCards.size(); ++c) {
                    TImage* img = new TImage(this);
                    img->Parent = this;
                    img->Width  = cardW;
                    img->Height = cardH;

                    img->Position->X = splitCenterX - cardW * 0.5f;
                    img->Position->Y = baseY + c * stackOffsetY;

                    img->WrapMode    = TImageWrapMode::Fit;

                    try {
                        img->Bitmap->LoadFromFile(folder + BJCard::GetCardFileName(splitCards[c]));
                    } catch (...) {}

                    img->BringToFront();

                    TShadowEffect* shadow = new TShadowEffect(this);
                    shadow->Parent       = img;
                    shadow->ShadowColor  = TAlphaColorRec::Black;
                    shadow->Opacity      = 0.6f;
                    shadow->Softness     = 0.7f;
                    shadow->Distance     = 5;

                    if (splitIs21) {
                        TGlowEffect* g = new TGlowEffect(this);
                        g->Parent      = img;
                        g->GlowColor   = TAlphaColorRec::Lime;
                        g->Softness    = 0.9f;
                        g->Opacity     = 0.8f;
                        g->Enabled     = true;
                    }

                    playerCardImagesSplit[i].push_back(img);
                }
            }
        }

        if (i < (int)playerInfoLabels.size() && playerInfoLabels[i]) {
            TLabel* info = playerInfoLabels[i];
            info->AutoSize = true;
            info->ApplyStyleLookup();
            info->Position->X = mainCenterX - info->Width * 0.5f;

            float extraY = 0.f;
            if (hasSplit && mainCards.size() > 1) {
                extraY = (mainCards.size() - 1) * stackOffsetY;
            }
            info->Position->Y = baseY + cardH + 8.f + extraY;
            info->BringToFront();
        }

        if (hasSplit &&
            i < (int)splitInfoLabels.size() && splitInfoLabels[i]) {

            TLabel* info2 = splitInfoLabels[i];
            info2->AutoSize = true;
            info2->ApplyStyleLookup();
            info2->Position->X = splitCenterX - info2->Width * 0.5f;

            float extraY2 = 0.f;
            if (splitCards.size() > 1) {
                extraY2 = (splitCards.size() - 1) * stackOffsetY;
            }
            info2->Position->Y = baseY + cardH + 8.f + extraY2;
            info2->BringToFront();
        }

        if (i < (int)mainHandTitleLabels.size() && mainHandTitleLabels[i]) {
            TLabel* t = mainHandTitleLabels[i];
            t->AutoSize = true;
            t->ApplyStyleLookup();
            t->Position->X = mainCenterX - t->Width * 0.5f;
            t->Position->Y = baseY - t->Height - 5.f;
        }

        if (i < (int)splitHandTitleLabels.size() && splitHandTitleLabels[i]) {
            TLabel* t2 = splitHandTitleLabels[i];
            t2->AutoSize = true;
            t2->ApplyStyleLookup();
            t2->Position->X = splitCenterX - t2->Width * 0.5f;
            t2->Position->Y = baseY - t2->Height - 5.f;
        }
    }
}

//---------------------------------------------------------------------------
// ANIMATIONS: DEALT CARD TO PLAYER
//---------------------------------------------------------------------------

void TForm1::AnimateDealtCardToPlayer(int playerIndex)
{
    if (!game) return;

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;
    if (playerIndex < 0 || playerIndex >= playerCount) return;

    BJPlayer& p       = game->GetPlayer(playerIndex);
    BJHand&  mainH    = p.GetHand();
    const auto& cards = mainH.GetCards();
    if (cards.empty()) return;

    int mainCount = (int)cards.size();
    int cardIndex = mainCount - 1;

    const float cardW   = 90.f;
    const float cardH   = 130.f;

    if ((int)playerCardImagesMain.size() <= playerIndex)
        playerCardImagesMain.resize(playerIndex + 1);

    auto &row = playerCardImagesMain[playerIndex];

    for (int c = 0; c < cardIndex && c < (int)row.size(); ++c) {
        TPointF pos = ComputePlayerMainCardTarget(playerIndex, c, mainCount);
        row[c]->Position->X = pos.X;
        row[c]->Position->Y = pos.Y;
    }

    float deckX, deckY;
    GetDeckPosition(deckImage, deckX, deckY);

    TPointF target = ComputePlayerMainCardTarget(playerIndex, cardIndex, mainCount);

    TImage* img = new TImage(this);
    img->Parent = this;
    img->Width  = cardW;
    img->Height = cardH;
    img->Position->X = deckX;
    img->Position->Y = deckY;
    img->WrapMode    = TImageWrapMode::Fit;

    try {
        String folder = BJCard::GetCardsFolder();
        img->Bitmap->LoadFromFile(folder + BJCard::GetCardFileName(cards[cardIndex]));
    } catch (...) {}

    img->BringToFront();
    row.push_back(img);

    TFloatAnimation* animX = new TFloatAnimation(img);
    animX->Parent           = img;
    animX->PropertyName     = "Position.X";
    animX->StartFromCurrent = true;
    animX->StopValue        = target.X;
    animX->Duration         = 0.25f;
    animX->Enabled          = true;

    TFloatAnimation* animY = new TFloatAnimation(img);
    animY->Parent           = img;
    animY->PropertyName     = "Position.Y";
    animY->StartFromCurrent = true;
    animY->StopValue        = target.Y;
    animY->Duration         = 0.25f;
    animY->Enabled          = true;
}

//---------------------------------------------------------------------------
// ANIMATIONS: DEALT CARD TO DEALER
//---------------------------------------------------------------------------

void TForm1::AnimateDealtCardToDealer()
{
    if (!game) return;

    BJHand& h = game->GetDealer().GetHand();
    const auto& cards = h.GetCards();
    if (cards.empty()) return;

    int count     = (int)cards.size();
    int cardIndex = count - 1;

    const float cardW   = 120.f;
    const float cardH   = 170.f;

    if ((int)dealerCardImages.size() < count)
        dealerCardImages.resize(count, nullptr);

    for (int i = 0; i < cardIndex; ++i) {
        if (dealerCardImages[i]) {
            TPointF pos = ComputeDealerCardTarget(i, count);
            dealerCardImages[i]->Position->X = pos.X;
            dealerCardImages[i]->Position->Y = pos.Y;
        }
    }

    float deckX, deckY;
    GetDeckPosition(deckImage, deckX, deckY);

    TPointF target = ComputeDealerCardTarget(cardIndex, count);

    TImage* img = new TImage(this);
    img->Parent = this;
    img->Width  = cardW;
    img->Height = cardH;
    img->Position->X = deckX;
    img->Position->Y = deckY;
    img->WrapMode    = TImageWrapMode::Fit;

    try {
        String folder = BJCard::GetCardsFolder();
        if (dealerHoleHidden && cardIndex == 1) {
            img->Bitmap->LoadFromFile(folder + "back.png");
        } else {
            img->Bitmap->LoadFromFile(folder + BJCard::GetCardFileName(cards[cardIndex]));
        }
    } catch (...) {}

    img->BringToFront();
    dealerCardImages[cardIndex] = img;

    TFloatAnimation* animX = new TFloatAnimation(img);
    animX->Parent           = img;
    animX->PropertyName     = "Position.X";
    animX->StartFromCurrent = true;
    animX->StopValue        = target.X;
    animX->Duration         = 0.25f;
    animX->Enabled          = true;

    TFloatAnimation* animY = new TFloatAnimation(img);
    animY->Parent           = img;
    animY->PropertyName     = "Position.Y";
    animY->StartFromCurrent = true;
    animY->StopValue        = target.Y;
    animY->Duration         = 0.25f;
    animY->Enabled          = true;
}


void TForm1::AnimateHitToCurrentHand()
{
    if (!game) return;

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;

    int playerIndex = game->getCurrentPlayerIndex();
    int handIndex   = game->getCurrentHandIndex();

    if (playerIndex < 0 || playerIndex >= playerCount) return;

    BJPlayer& p = game->GetPlayer(playerIndex);
    BJHand&   h = (handIndex == 0) ? p.GetHand() : p.GetSplitHand();
    const auto& cards = h.GetCards();
    if (cards.empty()) return;

    int cardIndex = (int)cards.size() - 1;

    const float cardW        = 90.f;
    const float cardH        = 130.f;
    const float fanSpacing   = cardW * 0.35f;
    const float stackOffsetY = 25.f;
    const float baseYOffset  = 130.f;
    const float extraPlayerSpacing = 40.f;

    float buttonsY = ClientHeight - 80.f;
    float baseY    = buttonsY - cardH - baseYOffset;

    float baseCenterX = (playerIndex + 1) * (ClientWidth / (playerCount + 1.0f));
    float centerX     = baseCenterX;
    if (playerCount > 1) {
        float offset = (playerIndex - (playerCount - 1) / 2.0f) * extraPlayerSpacing;
        centerX += offset;
    }

    float mainCenterX  = centerX;
    float splitCenterX = centerX;

    bool hasSplit = p.hasSplitHand();
    if (hasSplit) {
        const float pairGap = 35.f;
        mainCenterX  = centerX - (cardW + pairGap) * 0.5f;
        splitCenterX = centerX + (cardW + pairGap) * 0.5f;
    }

    TPointF target;

    if (handIndex == 0 && !hasSplit) {
        target = ComputePlayerMainCardTarget(playerIndex, cardIndex, (int)cards.size());
    }
    else if (handIndex == 0 && hasSplit) {
        target.X = mainCenterX - cardW * 0.5f;
        target.Y = baseY + cardIndex * stackOffsetY;
    }
    else {
        target.X = splitCenterX - cardW * 0.5f;
        target.Y = baseY + cardIndex * stackOffsetY;
    }

    float deckX, deckY;
    GetDeckPosition(deckImage, deckX, deckY);

    String folder = BJCard::GetCardsFolder();

    TImage* animImg = new TImage(nullptr);
    animImg->Parent = this;
    animImg->Width  = cardW;
    animImg->Height = cardH;
    animImg->WrapMode = TImageWrapMode::Fit;
    animImg->Position->X = deckX;
    animImg->Position->Y = deckY;

    try {
        animImg->Bitmap->LoadFromFile(folder + BJCard::GetCardFileName(cards[cardIndex]));
    }
    catch (...) {}

    animImg->BringToFront();

    const float dur = 0.25f;

    TFloatAnimation* animX = new TFloatAnimation(animImg);
    animX->Parent           = animImg;
    animX->PropertyName     = "Position.X";
    animX->StartFromCurrent = true;
    animX->StopValue        = target.X;
    animX->Duration         = dur;
    animX->Enabled          = true;

    TFloatAnimation* animY = new TFloatAnimation(animImg);
    animY->Parent           = animImg;
    animY->PropertyName     = "Position.Y";
    animY->StartFromCurrent = true;
    animY->StopValue        = target.Y;
    animY->Duration         = dur;
    animY->OnFinish         = HitAnimationFinished;
    animY->Enabled          = true;
}


//---------------------------------------------------------------------------
// DEALER PEEK ANIMATION (DIAGONAL TILT, THEN BACK)
//---------------------------------------------------------------------------

void TForm1::AnimateDealerPeek(bool /*hasBlackjack*/)
{
    if (dealerCardImages.size() < 2 || !dealerCardImages[1]) return;

    TImage* holeImg = dealerCardImages[1];

    float originalX = holeImg->Position->X;
    float originalY = holeImg->Position->Y;

    const float tiltAngle = 25.0f;
    const float moveX     = -10.0f;
    const float moveY     = -100.0f;
    const float dur       = 0.5f;

    TFloatAnimation* animRot = new TFloatAnimation(holeImg);
    animRot->Parent           = holeImg;
    animRot->PropertyName     = "RotationAngle";
    animRot->StartValue       = 0.0f;
    animRot->StopValue        = tiltAngle;
    animRot->Duration         = dur;
    animRot->AutoReverse      = true;
    animRot->Loop             = false;
    animRot->OnFinish         = DealerPeekRotateFinished;
    animRot->Enabled          = true;

    TFloatAnimation* animX = new TFloatAnimation(holeImg);
    animX->Parent           = holeImg;
    animX->PropertyName     = "Position.X";
    animX->StartFromCurrent = true;
    animX->StopValue        = originalX + moveX;
    animX->Duration         = dur;
    animX->AutoReverse      = true;
    animX->Loop             = false;
    animX->Enabled          = true;

    TFloatAnimation* animY = new TFloatAnimation(holeImg);
    animY->Parent           = holeImg;
    animY->PropertyName     = "Position.Y";
    animY->StartFromCurrent = true;
    animY->StopValue        = originalY + moveY;
    animY->Duration         = dur;
    animY->AutoReverse      = true;
    animY->Loop             = false;
    animY->Enabled          = true;

    holeImg->BringToFront();
}


void __fastcall TForm1::DealerPeekRotateFinished(TObject* Sender)
{
    dealerPeekInProgress = false;

    if (!game) return;

    if (!dealerPeekBlackjack)
        return;

    dealerHoleHidden = false;
    UpdateAllLabels();
    game->settleBets();
    ShowRoundOverOverlay();
}

void TForm1::AnimateSplitForCurrentHand()
{
    if (!game) return;

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;
    int idx = game->getCurrentPlayerIndex();
    if (idx < 0 || idx >= playerCount) {
        UpdateAllLabels();
        CreatePlayerActionButtons();
        return;
    }

    if ((int)playerCardImagesMain.size() <= idx ||
        playerCardImagesMain[idx].size() < 2)
    {
        UpdateAllLabels();
        CreatePlayerActionButtons();
        return;
    }

    auto& row = playerCardImagesMain[idx];
    TImage* imgMain  = row[0];
    TImage* imgSplit = row[1];

    const float cardW        = 90.f;
    const float cardH        = 130.f;
    const float baseYOffset  = 130.f;
    const float extraPlayerSpacing = 40.f;
    const float pairGap      = 35.f;

    float buttonsY = ClientHeight - 80.f;
    float baseY    = buttonsY - cardH - baseYOffset;

    float baseCenterX = (idx + 1) * (ClientWidth / (playerCount + 1.0f));
    float centerX     = baseCenterX;
    if (playerCount > 1) {
        float offset = (idx - (playerCount - 1) / 2.0f) * extraPlayerSpacing;
        centerX += offset;
    }

    float mainCenterX  = centerX - (cardW + pairGap) * 0.5f;
    float splitCenterX = centerX + (cardW + pairGap) * 0.5f;

    float targetMainX  = mainCenterX  - cardW * 0.5f;
    float targetSplitX = splitCenterX - cardW * 0.5f;
    float targetY      = baseY;

    if (imgMain)  imgMain->BringToFront();
    if (imgSplit) imgSplit->BringToFront();

    const float dur = 0.25f;

    if (imgMain) {
        TFloatAnimation* aX = new TFloatAnimation(imgMain);
        aX->Parent           = imgMain;
        aX->PropertyName     = "Position.X";
        aX->StartFromCurrent = true;
        aX->StopValue        = targetMainX;
        aX->Duration         = dur;
        aX->Enabled          = true;

        TFloatAnimation* aY = new TFloatAnimation(imgMain);
        aY->Parent           = imgMain;
        aY->PropertyName     = "Position.Y";
        aY->StartFromCurrent = true;
        aY->StopValue        = targetY;
        aY->Duration         = dur;
        aY->Enabled          = true;
    }

    if (imgSplit) {
        TFloatAnimation* bX = new TFloatAnimation(imgSplit);
        bX->Parent           = imgSplit;
        bX->PropertyName     = "Position.X";
        bX->StartFromCurrent = true;
        bX->StopValue        = targetSplitX;
        bX->Duration         = dur;
        bX->Enabled          = true;

        TFloatAnimation* bY = new TFloatAnimation(imgSplit);
        bY->Parent           = imgSplit;
        bY->PropertyName     = "Position.Y";
        bY->StartFromCurrent = true;
        bY->StopValue        = targetY;
        bY->Duration         = dur;
        bY->OnFinish         = SplitAnimationFinished;
        bY->Enabled          = true;
    }
}

void __fastcall TForm1::SplitAnimationFinished(TObject* Sender)
{
    UpdateAllLabels();
    CreatePlayerActionButtons();
}



void __fastcall TForm1::HitAnimationFinished(TObject* Sender)
{
    TFloatAnimation* anim = dynamic_cast<TFloatAnimation*>(Sender);
    if (!anim) return;

    TImage* animImg = dynamic_cast<TImage*>(anim->Owner);
    if (!animImg)
        animImg = dynamic_cast<TImage*>(anim->Parent);

    if (animImg)
    {
        animImg->Visible = false;
        animImg->Parent  = nullptr;
    }

    if (!game) return;

    UpdateAllLabels();

    BJHand& h = game->GetCurrentHand();
    if (h.value() >= 21)
    {
        playerStand();
    }
    else
    {
        CreatePlayerActionButtons();
    }
}



void TForm1::CreateShuffleCards()
{
    shuffleCards.clear();

    if (!deckImage) return;

    String folder = BJCard::GetCardsFolder();

    const int   count = 4;
    const float cardW = 90.f;
    const float cardH = 130.f;

    for (int i = 0; i < count; ++i)
    {
        TImage* img = new TImage(this);
        img->Parent = this;
        img->Width  = cardW;
        img->Height = cardH;
        img->WrapMode = TImageWrapMode::Fit;

        try {
            img->Bitmap->LoadFromFile(folder + "back.png");
        } catch (...) {}

        img->Visible = false;
        img->Position->X = deckImage->Position->X;
        img->Position->Y = deckImage->Position->Y;

        shuffleCards.push_back(img);
    }
}



void __fastcall TForm1::ShuffleCardTimerTick(TObject* Sender)
{
    if (!deckImage) return;
    if (shuffleCards.empty()) return;

    static int idx = 0;
    if (idx >= (int)shuffleCards.size())
        idx = 0;

    TImage* img = shuffleCards[idx];
    idx++;

    if (!img) return;

    float startX = deckImage->Position->X;
    float startY = deckImage->Position->Y;

    float dx = RandRange(-20.0f, 20.0f);
    float dy = RandRange(-70.0f, -40.0f);

    float targetX = startX + dx;
    float targetY = startY + dy;

    float dur = RandRange(0.20f, 0.30f);

    img->Visible       = true;
    img->Position->X   = startX;
    img->Position->Y   = startY;
    img->RotationAngle = 0.0f;

    TFloatAnimation* animX = new TFloatAnimation(img);
    animX->Parent           = img;
    animX->PropertyName     = "Position.X";
    animX->StartFromCurrent = true;
    animX->StopValue        = targetX;
    animX->Duration         = dur;
    animX->AutoReverse      = true;
    animX->Loop             = false;
    animX->Enabled          = true;

    TFloatAnimation* animY = new TFloatAnimation(img);
    animY->Parent           = img;
    animY->PropertyName     = "Position.Y";
    animY->StartFromCurrent = true;
    animY->StopValue        = targetY;
    animY->Duration         = dur;
    animY->AutoReverse      = true;
    animY->Loop             = false;
    animY->Enabled          = true;

    TFloatAnimation* animRot = new TFloatAnimation(img);
    animRot->Parent           = img;
    animRot->PropertyName     = "RotationAngle";
    animRot->StartValue       = -10.0f;
    animRot->StopValue        =  10.0f;
    animRot->Duration         = dur;
    animRot->AutoReverse      = true;
    animRot->Loop             = false;
    animRot->Enabled          = true;
}



void TForm1::StartDeckShuffleAnimation()
{
    if (!deckImage) return;
    if (shuffleCardTimer)
        shuffleCardTimer->Enabled = true;
}

void TForm1::StopDeckShuffleAnimation()
{
    if (shuffleCardTimer)
        shuffleCardTimer->Enabled = false;

    if (!deckImage) return;

    for (auto* img : shuffleCards)
    {
        if (!img) continue;
        img->Visible       = false;
        img->Position->X   = deckImage->Position->X;
        img->Position->Y   = deckImage->Position->Y;
        img->RotationAngle = 0.0f;
    }
}



//---------------------------------------------------------------------------
// PLAYER ACTION BUTTONS
//---------------------------------------------------------------------------

// ---------------- PLAYER ACTION BUTTONS ----------------

void TForm1::CreatePlayerActionButtons()
{

    DestroyPlayerActionButtons();

    if (!game || bettingPhase)
        return;


    int playerIndex = game->getCurrentPlayerIndex();
    int handIndex   = game->getCurrentHandIndex();

    if (playerIndex < 0)
        return;

    BJPlayer& p = game->GetPlayer(playerIndex);
    BJHand&   h = (handIndex == 0) ? p.GetHand() : p.GetSplitHand();
    const auto& cards = h.GetCards();


    int betForThisHand = (handIndex == 0)
                         ? p.getBet()
                         : p.getSplitBet();

    int total = h.value();


    if (p.isBankrupt() || betForThisHand <= 0 || cards.empty() || total >= 21)
    {
        playerStand();
        return;
    }

    // ---------- RULE LOGIC ----------

    bool canHit    = (total < 21);
    bool canStand  = true;
    bool canSplit  = false;
    bool canDouble = false;

    bool alreadyActed = p.hasActedOnHand(handIndex);


    if (handIndex == 0 && !p.hasSplitHand() && cards.size() == 2 && !alreadyActed)
    {
        int r1 = static_cast<int>(cards[0].getRank());
        int r2 = static_cast<int>(cards[1].getRank());

        if (r1 == r2 && p.getChips() >= betForThisHand)
        {
            canSplit = true;
        }
    }


    if (!alreadyActed &&
        cards.size() == 2 &&
        total >= 9 && total <= 11 &&
        p.getChips() >= betForThisHand)
    {
        canDouble = true;
    }

    // ---------- CREATE / LAYOUT BUTTONS ----------

    const int buttonWidth  = 140;
    const int buttonHeight = 42;
    const int buttonGap    = 20;
    const int buttonCount  = 4;

    const int totalWidth = buttonCount * buttonWidth + (buttonCount - 1) * buttonGap;
    const int startX     = (ClientWidth - totalWidth) / 2;
    const int posY       = ClientHeight - 80;

    for (int i = 0; i < buttonCount; ++i)
    {
        TButton* playerButton = new TButton(this);
        playerButton->Parent = this;

        switch (i)
        {
            case 0: playerButton->Text = "Hit";         break;
            case 1: playerButton->Text = "Stand";       break;
            case 2: playerButton->Text = "Double Down"; break;
            case 3: playerButton->Text = "Split";       break;
        }

        playerButton->Width  = buttonWidth;
        playerButton->Height = buttonHeight;
        playerButton->Position->X = startX + i * (buttonWidth + buttonGap);
        playerButton->Position->Y = posY;

        playerButton->StyledSettings = TStyledSettings();
        playerButton->TextSettings->Font->Family = "Cooper";
        playerButton->TextSettings->Font->Size   = 18;

        switch (i) {
            case 0: playerButton->TintColor = TAlphaColorRec::Forestgreen; break;
            case 1: playerButton->TintColor = TAlphaColorRec::Goldenrod;   break;
            case 2: playerButton->TintColor = TAlphaColorRec::Royalblue;   break;
            case 3: playerButton->TintColor = TAlphaColorRec::Darkviolet;  break;
        }

        bool enabled = false;
        switch (i)
        {
            case 0: enabled = canHit;    break;
            case 1: enabled = canStand;  break;
            case 2: enabled = canDouble; break;
            case 3: enabled = canSplit;  break;
        }
        playerButton->Enabled = enabled;

        switch (i)
        {
            case 0: playerButton->OnClick = HitButtonClick;        break;
            case 1: playerButton->OnClick = StandButtonClick;      break;
            case 2: playerButton->OnClick = DoubleDownButtonClick; break;
            case 3: playerButton->OnClick = SplitButtonClick;      break;
        }

        playerButton->Opacity = 0.0f;
        TFloatAnimation* fade = new TFloatAnimation(playerButton);
        fade->Parent          = playerButton;
        fade->PropertyName    = "Opacity";
		fade->StartValue      = 0.0f;
        fade->StopValue       = 1.0f;
        fade->Duration        = 0.25f;
        fade->Enabled         = true;

        actionButtons.push_back(playerButton);
    }
}




void TForm1::DestroyPlayerActionButtons()
{
	for (auto* btn : actionButtons)
	{
		if (!btn) continue;
		btn->Parent = nullptr;
		btn->DisposeOf();
	}
	actionButtons.clear();
}


void __fastcall TForm1::AddChipOutline(TImage* chip)
{
    if (!chip) return;

    TCircle* circle = new TCircle(chip);
    circle->Parent   = chip;
    circle->HitTest  = false;
    circle->TagString = "ChipOutline";

    circle->Fill->Kind        = TBrushKind::None;
    circle->Stroke->Kind      = TBrushKind::Solid;
    circle->Stroke->Color     = TAlphaColorRec::Black;
    circle->Stroke->Thickness = 3.0f;

    float size = chip->Width;
    if (chip->Height < size) size = chip->Height;
    size -= -5.0f;
    if (size < 0) size = 0;

    circle->Width  = size;
    circle->Height = size;

    circle->Position->X = (chip->Width  - circle->Width)  * 0.5f;
    circle->Position->Y = (chip->Height - circle->Height) * 0.5f;

    circle->BringToFront();
}



//---------------------------------------------------------------------------
// BETTING UI (CHIP, HINT, DEAL BUTTON)
//---------------------------------------------------------------------------

void TForm1::CreateBetUI()
{
    if (!game) return;

    if (!betChipImage) {
        betChipImage = new TImage(this);
        betChipImage->Parent = this;

        betChipImage->Width  = 160;
        betChipImage->Height = 160;

        betChipImage->WrapMode = TImageWrapMode::Fit;
        betChipImage->HitTest  = true;

        betChipImage->OnMouseDown  = ChipMouseDown;
        betChipImage->OnMouseEnter = betChipMouseEnter;
        betChipImage->OnMouseLeave = betChipMouseLeave;

        try {
            String exePath = ExtractFilePath(ParamStr(0));
            betChipImage->Bitmap->LoadFromFile(exePath + "..\\..\\chip10.png");
        } catch (...) {
        }

        if (!chipGlow) {
            chipGlow = new TGlowEffect(this);
            chipGlow->GlowColor = TAlphaColorRec::Gold;
            chipGlow->Softness  = 0.9f;
            chipGlow->Opacity   = 0.9f;
            chipGlow->Enabled   = false;
            chipGlow->Parent    = betChipImage;
        }
    }

    betChipImage->Position->X = (ClientWidth - betChipImage->Width) * 0.5f;
    betChipImage->Position->Y = ClientHeight * 0.20f;
    betChipImage->Visible     = true;
    betChipImage->HitTest     = true;

    if (betChipImage)  AddChipOutline(betChipImage);

    if (!betChip1Image) {
        betChip1Image = new TImage(this);
        betChip1Image->Parent = this;

        betChip1Image->Width  = 120;
        betChip1Image->Height = 120;
        betChip1Image->WrapMode = TImageWrapMode::Fit;
        betChip1Image->HitTest  = true;

        betChip1Image->OnMouseDown  = Chip1MouseDown;
        betChip1Image->OnMouseEnter = betChipMouseEnter;
        betChip1Image->OnMouseLeave = betChipMouseLeave;

        try {
            String exePath = ExtractFilePath(ParamStr(0));
            betChip1Image->Bitmap->LoadFromFile(exePath + "..\\..\\chip1.png");
        } catch (...) {
        }
    }

    {
        float spacing = 40.0f;
        betChip1Image->Position->X =
            betChipImage->Position->X - betChip1Image->Width - spacing;
        betChip1Image->Position->Y =
            betChipImage->Position->Y + (betChipImage->Height - betChip1Image->Height) * 0.5f;
        betChip1Image->Visible = true;
        betChip1Image->HitTest = true;

        if (betChip1Image) AddChipOutline(betChip1Image);
    }

    if (!betChip50Image) {
        betChip50Image = new TImage(this);
        betChip50Image->Parent = this;

        betChip50Image->Width  = 120;
        betChip50Image->Height = 120;
        betChip50Image->WrapMode = TImageWrapMode::Fit;
        betChip50Image->HitTest  = true;
        AddChipOutline(betChip50Image);

        betChip50Image->OnMouseDown  = Chip50MouseDown;
        betChip50Image->OnMouseEnter = betChipMouseEnter;
        betChip50Image->OnMouseLeave = betChipMouseLeave;

        try {
            String exePath = ExtractFilePath(ParamStr(0));
            betChip50Image->Bitmap->LoadFromFile(exePath + "..\\..\\chip50.png");
        } catch (...) {
        }
    }

    {
        float spacing = 40.0f;
        betChip50Image->Position->X =
            betChipImage->Position->X + betChipImage->Width + spacing;
        betChip50Image->Position->Y =
            betChipImage->Position->Y + (betChipImage->Height - betChip50Image->Height) * 0.5f;
        betChip50Image->Visible = true;
        betChip50Image->HitTest = true;
        if (betChip50Image) AddChipOutline(betChip50Image);
    }

    if (!betHintBackground) {
        betHintBackground = new TRectangle(this);
        betHintBackground->Parent = this;

        betHintBackground->Width  = 540;
        betHintBackground->Height = 42;

        betHintBackground->Fill->Kind  = TBrushKind::Solid;
        betHintBackground->Fill->Color = TAlphaColorRec::Black;
        betHintBackground->Opacity     = 0.35f;

        betHintBackground->Stroke->Kind = TBrushKind::None;
        betHintBackground->XRadius      = 12;
        betHintBackground->YRadius      = 12;
    }

    if (!betHintLabel) {
        betHintLabel = new TLabel(this);
        betHintLabel->Parent = this;

        betHintLabel->StyledSettings = TStyledSettings();
        betHintLabel->TextSettings->Font->Family = "Cooper";
        betHintLabel->TextSettings->Font->Size   = 17;
        betHintLabel->TextSettings->FontColor    = TAlphaColorRec::White;
        betHintLabel->TextSettings->HorzAlign    = TTextAlign::Center;

        betHintLabel->Text =
            "Left click: +$1 / +$10 / +$50    |    Right click: -$1 / -$10 / -$50";

        betHintLabel->AutoSize = false;
        betHintLabel->Width    = betHintBackground->Width;
        betHintLabel->Height   = betHintBackground->Height;
    }

    {
        float hintY = betChipImage->Position->Y - betHintBackground->Height - 40.0f;

        betHintBackground->Position->X =
            (ClientWidth - betHintBackground->Width) * 0.5f;
        betHintBackground->Position->Y = hintY;
        betHintBackground->Visible     = true;

        betHintLabel->Position->X = betHintBackground->Position->X;
        betHintLabel->Position->Y = betHintBackground->Position->Y;
        betHintLabel->Visible     = true;
    }

    if (!dealButton) {
        dealButton = new TButton(this);
        dealButton->Parent = this;
        dealButton->Text   = "Deal";

        dealButton->StyledSettings = TStyledSettings();
        dealButton->TextSettings->Font->Family = "Cooper";
        dealButton->TextSettings->Font->Size   = 20;

        dealButton->Width   = 140;
        dealButton->Height  = 44;
        dealButton->OnClick = DealButtonClick;
    }

    dealButton->Visible = true;
    dealButton->Enabled = false;
    dealButton->Position->X =
        (ClientWidth - dealButton->Width) * 0.5f;
    dealButton->Position->Y =
        betChipImage->Position->Y + betChipImage->Height + 40.0f;
}




void TForm1::DestroyBetUI()
{
    if (betChipImage) {
        delete betChipImage;
        betChipImage = nullptr;
    }
    if (betChip1Image) {
        delete betChip1Image;
        betChip1Image = nullptr;
    }
    if (betChip50Image) {
        delete betChip50Image;
        betChip50Image = nullptr;
    }
    if (betHintLabel) {
        delete betHintLabel;
        betHintLabel = nullptr;
    }
    if (dealButton) {
        delete dealButton;
        dealButton = nullptr;
    }
}



//---------------------------------------------------------------------------
// BET CONFIRM BUTTONS
//---------------------------------------------------------------------------

void TForm1::CreateBetConfirmButtons()
{
    DestroyBetConfirmButtons();
    if (!game) return;

    Settings& s = Settings::getInstance();
    int count = s.player_count;

    betConfirmButtons.resize(count, nullptr);

    for (int i = 0; i < count; ++i) {
        TButton* btn = new TButton(this);
        btn->Parent = this;
        btn->Text   = "Confirm Bet";

        btn->StyledSettings = TStyledSettings();
        btn->TextSettings->Font->Family = "Cooper";
        btn->TextSettings->Font->Size   = 14;

        btn->Width  = 120;
        btn->Height = 32;

        float centerX = (i + 1) * (ClientWidth / (count + 1.0f));
        btn->Position->X = centerX - btn->Width * 0.5f;
        btn->Position->Y = ClientHeight * 0.84f;

        btn->Tag = i;
        btn->OnClick = BetConfirmButtonClick;

        BJPlayer& p = game->GetPlayer(i);
        if (p.isBankrupt()) {
            btn->Enabled = false;
        }

        betConfirmButtons[i] = btn;
    }
}


void TForm1::DestroyBetConfirmButtons()
{
    for (auto* b : betConfirmButtons) {
        if (b) b->Parent = nullptr;
    }
    betConfirmButtons.clear();
}

//---------------------------------------------------------------------------
// CHIP MOUSE DOWN (BETTING)
//---------------------------------------------------------------------------

void __fastcall TForm1::ChipMouseDown(
    TObject* Sender,
    TMouseButton Button,
    TShiftState Shift,
    float X, float Y)
{
    if (!bettingPhase || !game) return;

    Settings& s = Settings::getInstance();
    int count = s.player_count;

    if (bettingPlayerIndex < 0 || bettingPlayerIndex >= count) return;
    if ((int)playerBetConfirmed.size() != count) return;
    if (playerBetConfirmed[bettingPlayerIndex]) return;

    BJPlayer& p = game->GetPlayer(bettingPlayerIndex);
    if (p.isBankrupt())
        return;
    bool isLeft  = (Button == TMouseButton::mbLeft);
    bool isRight = (Button == TMouseButton::mbRight);

    if (isLeft) {
        if (p.getChips() >= 10) {
            p.adjustChips(-10);
            p.setBet(p.getBet() + 10);
        }
    }
    else if (isRight) {
        if (p.getBet() >= 10) {
            p.setBet(p.getBet() - 10);
            p.adjustChips(+10);
        }
    }

    UpdateAllLabels();
}

void __fastcall TForm1::Chip1MouseDown(
    TObject* Sender,
    TMouseButton Button,
    TShiftState Shift,
    float X, float Y)
{
    if (!bettingPhase || !game) return;

    Settings& s = Settings::getInstance();
    int count = s.player_count;

    if (bettingPlayerIndex < 0 || bettingPlayerIndex >= count) return;
    if ((int)playerBetConfirmed.size() != count) return;
    if (playerBetConfirmed[bettingPlayerIndex]) return;

    BJPlayer& p = game->GetPlayer(bettingPlayerIndex);
    if (p.isBankrupt()) return;

    bool isLeft  = (Button == TMouseButton::mbLeft);
    bool isRight = (Button == TMouseButton::mbRight);

    if (isLeft) {
        if (p.getChips() >= 1) {
            p.adjustChips(-1);
            p.setBet(p.getBet() + 1);
        }
    }
    else if (isRight) {
        if (p.getBet() >= 1) {
            p.setBet(p.getBet() - 1);
            p.adjustChips(+1);
        }
    }

	UpdateAllLabels();
}


void __fastcall TForm1::Chip50MouseDown(
    TObject* Sender,
    TMouseButton Button,
    TShiftState Shift,
    float X, float Y)
{
    if (!bettingPhase || !game) return;

    Settings& s = Settings::getInstance();
    int count = s.player_count;

    if (bettingPlayerIndex < 0 || bettingPlayerIndex >= count) return;
    if ((int)playerBetConfirmed.size() != count) return;
    if (playerBetConfirmed[bettingPlayerIndex]) return;

    BJPlayer& p = game->GetPlayer(bettingPlayerIndex);
    if (p.isBankrupt()) return;

    bool isLeft  = (Button == TMouseButton::mbLeft);
    bool isRight = (Button == TMouseButton::mbRight);

    if (isLeft) {
        if (p.getChips() >= 50) {
            p.adjustChips(-50);
            p.setBet(p.getBet() + 50);
        }
    }
    else if (isRight) {
        if (p.getBet() >= 50) {
            p.setBet(p.getBet() - 50);
            p.adjustChips(+50);
        }
    }

    UpdateAllLabels();
}



//---------------------------------------------------------------------------
// START DEALING ANIMATION (ROUND START)
//---------------------------------------------------------------------------

void TForm1::StartDealingAnimation()
{
    if (!game) return;

    game->resetForNextRound();

    dealingAnimationActive = true;
    dealPhase  = 0;
    dealIndex  = 0;

    dealerHoleHidden = true;

    ClearDealerCardImages();
    ClearPlayerCardImages();

    UpdateAllLabels();

    if (dealTimer)
        dealTimer->Enabled = true;
}

//---------------------------------------------------------------------------
// DEAL TIMER: STEP THROUGH ANIMATED DEALING
//---------------------------------------------------------------------------

void __fastcall TForm1::DealTimerTick(TObject *Sender)
{
    if (!game || !dealingAnimationActive) {
        if (dealTimer) dealTimer->Enabled = false;
        return;
    }

    int playerCount = game->getPlayerCount();

    switch (dealPhase)
    {
        case 0:
        {
            if (dealIndex < playerCount) {
                int idx = dealIndex;
                BJPlayer &p = game->GetPlayer(idx);
                game->GetDeck().dealCardTo(p.GetHand());

                AnimateDealtCardToPlayer(idx);
                ++dealIndex;
                UpdateAllLabels();
                return;
            }

            dealPhase = 1;
            dealIndex = 0;
            return;
        }

        case 1:
        {
            game->GetDeck().dealCardTo(game->GetDealer().GetHand());
            AnimateDealtCardToDealer();
            UpdateAllLabels();
            dealPhase = 2;
            dealIndex = 0;
            return;
        }

        case 2:
        {
            if (dealIndex < playerCount) {
                int idx = dealIndex;
                BJPlayer &p = game->GetPlayer(idx);
                game->GetDeck().dealCardTo(p.GetHand());

                AnimateDealtCardToPlayer(idx);
                ++dealIndex;
                UpdateAllLabels();
                return;
            }

            dealPhase = 3;
            dealIndex = 0;
            return;
        }

        case 3:
        {
            game->GetDeck().dealCardTo(game->GetDealer().GetHand());

            AnimateDealtCardToDealer();

            dealingAnimationActive = false;
            if (dealTimer) dealTimer->Enabled = false;

            UpdateAllLabels();

            bool dealerBJ = CheckDealerBlackjack();

            if (!dealerBJ) {
                CreatePlayerActionButtons();
            }

            if (dealerLabel) dealerLabel->BringToFront();
            for (auto *img : dealerCardImages) {
                if (img) img->BringToFront();
            }
            return;
        }

        default:
            break;
    }
}


//---------------------------------------------------------------------------
// BET CONFIRM BUTTON CLICK
//---------------------------------------------------------------------------

void __fastcall TForm1::BetConfirmButtonClick(TObject* Sender)
{
    if (!bettingPhase || !game) return;

    TButton* btn = dynamic_cast<TButton*>(Sender);
    if (!btn) return;

	int idx = btn->Tag;

    Settings& s = Settings::getInstance();
	int count = s.player_count;

    if (idx < 0 || idx >= count) return;
    if (idx != bettingPlayerIndex) return;

    BJPlayer& p = game->GetPlayer(idx);

    if (p.getBet() <= 0) {
        ShowMessage("Please place a bet before confirming.");
        return;
    }

    playerBetConfirmed[idx] = true;
    btn->Enabled = false;

	int next = -1;
    for (int i = idx + 1; i < count; ++i) {
        BJPlayer& np = game->GetPlayer(i);
        if (!playerBetConfirmed[i] && !np.isBankrupt()) {
            next = i;
            break;
        }
    }

    if (next >= 0) {
        bettingPlayerIndex = next;
        if (betChipImage) {
            betChipImage->HitTest = true;
            betChipImage->Opacity = 1.0f;
        }
    } else {
        bettingPlayerIndex = -1;
        if (betChipImage) {
            betChipImage->HitTest = false;
            betChipImage->Opacity = 0.7f;
        }

        if (dealButton) {
            dealButton->Enabled = true;
            dealButton->Visible = true;
        }
    }

    UpdateAllLabels();
}

//---------------------------------------------------------------------------
// ROUND OVER OVERLAY
//---------------------------------------------------------------------------

void TForm1::ShowRoundOverOverlay()
{
    if (!roundOverPanel) {
        roundOverPanel = new TRectangle(this);
        roundOverPanel->Parent = this;

        roundOverPanel->Fill->Kind   = TBrushKind::Solid;
        roundOverPanel->Fill->Color  = TAlphaColorRec::Black;
        roundOverPanel->Stroke->Kind = TBrushKind::None;
        roundOverPanel->XRadius      = 20;
        roundOverPanel->YRadius      = 20;
        roundOverPanel->HitTest      = true;
    }

    roundOverPanel->Width  = 600;
    roundOverPanel->Height = 200;
    roundOverPanel->Opacity = 0.0f;

    float targetX = (ClientWidth  - roundOverPanel->Width)  * 0.5f;
    float targetY = (ClientHeight - roundOverPanel->Height) * 0.5f;

    roundOverPanel->Position->X = targetX;
    roundOverPanel->Position->Y = -roundOverPanel->Height;

    roundOverPanel->Visible = true;
    roundOverPanel->BringToFront();

    if (!roundOverLabel) {
        roundOverLabel = new TLabel(this);
        roundOverLabel->Parent = roundOverPanel;
        roundOverLabel->Align  = TAlignLayout::Center;

        roundOverLabel->StyledSettings = TStyledSettings();
        roundOverLabel->TextSettings->Font->Family = "Cooper";
        roundOverLabel->TextSettings->Font->Size   = 40;
        roundOverLabel->TextSettings->Font->Style  = TFontStyles();
        roundOverLabel->TextSettings->FontColor    = TAlphaColorRec::White;
        roundOverLabel->TextSettings->HorzAlign    = TTextAlign::Center;
        roundOverLabel->TextSettings->VertAlign    = TTextAlign::Center;

        roundOverLabel->WordWrap = true;
        roundOverLabel->AutoSize = false;
    }

    roundOverLabel->Width    = roundOverPanel->Width;
    roundOverLabel->Height   = roundOverPanel->Height;
    roundOverLabel->Text     = "Round Over";
    roundOverLabel->Visible  = true;
    roundOverLabel->BringToFront();

    for (auto* btn : actionButtons) {
        if (btn) btn->Enabled = false;
    }

    TFloatAnimation* slideIn = new TFloatAnimation(roundOverPanel);
    slideIn->Parent           = roundOverPanel;
    slideIn->PropertyName     = "Position.Y";
    slideIn->StartFromCurrent = true;
    slideIn->StopValue        = targetY;
    slideIn->Duration         = 0.35f;
    slideIn->Enabled          = true;

    TFloatAnimation* fadeIn = new TFloatAnimation(roundOverPanel);
    fadeIn->Parent           = roundOverPanel;
    fadeIn->PropertyName     = "Opacity";
    fadeIn->StartFromCurrent = true;
    fadeIn->StopValue        = 0.9f;
    fadeIn->Duration         = 0.35f;
    fadeIn->Enabled          = true;

    if (roundOverTimer) {
        roundOverTimer->Interval = 3000;
        roundOverTimer->Enabled  = true;
    }
}

void __fastcall TForm1::RoundOverTimerTick(TObject *Sender) {
	if (!roundOverTimer) return;

	roundOverTimer->Enabled = false;

    if (!roundOverPanel) {
        if (gameOverToMainMenu) {
            EndRoundAndCheckGameOver();
		} else {
            StartCollectCardsAnimation();
        }
        return;
    }

    TFloatAnimation* fadeOut = new TFloatAnimation(roundOverPanel);
    fadeOut->Parent           = roundOverPanel;
    fadeOut->PropertyName     = "Opacity";
	fadeOut->StartFromCurrent = true;
    fadeOut->StopValue        = 0.0f;
	fadeOut->Duration         = 0.30f;
    fadeOut->OnFinish         = RoundOverFadeOutFinished;
    fadeOut->Enabled          = true;
}

void __fastcall TForm1::RoundOverFadeOutFinished(TObject *Sender)
{
    if (roundOverLabel)  roundOverLabel->Visible = false;
    if (roundOverPanel) {
        roundOverPanel->Visible = false;
        roundOverPanel->Opacity = 0.75f;
    }

	if (gameOverToMainMenu) {

        if (dealTimer)        dealTimer->Enabled        = false;
        if (shuffleCardTimer) shuffleCardTimer->Enabled = false;
		if (collectTimer)     collectTimer->Enabled     = false;

		if (FormMainMenu) {
            this->Hide();
			FormMainMenu->Show();
        } else {
            Close();
        }
    } else {
        StartCollectCardsAnimation();
    }
}

void TForm1::StartCollectCardsAnimation()
{
    collectingCards = false;
    collectImages.clear();

    for (auto* img : dealerCardImages) {
        if (img) collectImages.push_back(img);
    }
    for (auto& row : playerCardImagesMain) {
        for (auto* img : row) {
            if (img) collectImages.push_back(img);
        }
    }
    for (auto& row : playerCardImagesSplit) {
        for (auto* img : row) {
            if (img) collectImages.push_back(img);
        }
    }

    if (collectImages.empty()) {
        EndRoundAndCheckGameOver();
        return;
    }

    String folder = BJCard::GetCardsFolder();
    for (auto* img : collectImages) {
        try {
            img->Bitmap->LoadFromFile(folder + "back.png");
        } catch (...) {}
    }

    collectCardIndex = 0;
    collectingCards  = true;

    if (collectTimer) {
        collectTimer->Enabled = true;
    }
}

void __fastcall TForm1::CollectTimerTick(TObject *Sender)
{
    if (!collectingCards) {
        if (collectTimer) collectTimer->Enabled = false;
        return;
    }

    if (collectCardIndex >= (int)collectImages.size()) {
        collectingCards = false;
        if (collectTimer) collectTimer->Enabled = false;

        ClearDealerCardImages();
		ClearPlayerCardImages();

        EndRoundAndCheckGameOver();
        return;
    }

    TImage* img = collectImages[collectCardIndex];
    ++collectCardIndex;

    if (!img) return;

    float deckX, deckY;
	GetDeckPosition(deckImage, deckX, deckY);

    const float dur = 0.18f;

    TFloatAnimation* animX = new TFloatAnimation(img);
    animX->Parent           = img;
    animX->PropertyName     = "Position.X";
    animX->StartFromCurrent = true;
    animX->StopValue        = deckX;
    animX->Duration         = dur;
    animX->Enabled          = true;

    TFloatAnimation* animY = new TFloatAnimation(img);
    animY->Parent           = img;
    animY->PropertyName     = "Position.Y";
	animY->StartFromCurrent = true;
    animY->StopValue        = deckY;
    animY->Duration         = dur;
    animY->Enabled          = true;
}

//---------------------------------------------------------------------------
// END-OF-ROUND GAME-OVER LOGIC
//---------------------------------------------------------------------------

void TForm1::EndRoundAndCheckGameOver()
{
	if (!game) {
		BeginBettingPhase();
		return;
	}

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;

    for (int i = 0; i < playerCount; ++i) {
        BJPlayer& p = game->GetPlayer(i);
        p.setBet(0);
        p.setSplitBet(0);
    }

    MarkBankruptPlayers();

    String winnerText;
    bool haveWinner  = CheckGoalWinners(winnerText);
    bool anyActive   = AnyActivePlayers();

    if (!anyActive && !haveWinner) {
        gameOverToMainMenu = true;
        ShowGameOverBanner("All players are out of chips!");
        return;
    }

    if (haveWinner) {
        gameOverToMainMenu = true;
        ShowGameOverBanner(winnerText);
        return;
    }

    gameOverToMainMenu = false;
    BeginBettingPhase();
}

void TForm1::MarkBankruptPlayers()
{
    if (!game) return;

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;

    for (int i = 0; i < playerCount; ++i) {
        BJPlayer& p = game->GetPlayer(i);

        if (p.getChips() <= 0) {
			p.setBankrupt(true);
        }
    }
}

bool TForm1::AnyActivePlayers()
{
    if (!game) return false;

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;

    for (int i = 0; i < playerCount; ++i) {
        BJPlayer& p = game->GetPlayer(i);
        if (!p.isBankrupt() && p.getChips() > 0) {
            return true;
        }
    }
    return false;
}

bool TForm1::CheckGoalWinners(String &outText)
{
    if (!game) return false;

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;
    int goal = s.goal_amount;

    int bestAmount = -1;
    std::vector<int> bestPlayers;

    for (int i = 0; i < playerCount; ++i) {
        BJPlayer& p = game->GetPlayer(i);
        if (p.isBankrupt()) continue;

        int chips = p.getChips();
        if (chips < goal) continue;

        if (chips > bestAmount) {
            bestAmount = chips;
            bestPlayers.clear();
            bestPlayers.push_back(i);
        } else if (chips == bestAmount) {
            bestPlayers.push_back(i);
        }
    }

    if (bestPlayers.empty())
        return false;

    if (bestPlayers.size() == 1) {
        int idx = bestPlayers[0];
        outText = "Player " + IntToStr(idx + 1) +
                  " reached the goal with $" + IntToStr(bestAmount) + "!";
    } else if (bestPlayers.size() == 2) {
        outText =
            "Players " + IntToStr(bestPlayers[0] + 1) + " and " +
            IntToStr(bestPlayers[1] + 1) +
            " reached the goal with $" + IntToStr(bestAmount) + "!";
    } else {
        outText = "Multiple players reached the goal with $" +
                  IntToStr(bestAmount) + "!";
    }

    return true;
}

void TForm1::ShowGameOverBanner(const String& text)
{
    if (!roundOverPanel) {
        roundOverPanel = new TRectangle(this);
        roundOverPanel->Parent = this;

        roundOverPanel->Fill->Kind   = TBrushKind::Solid;
        roundOverPanel->Fill->Color  = TAlphaColorRec::Black;
        roundOverPanel->Stroke->Kind = TBrushKind::None;
        roundOverPanel->XRadius      = 24;
        roundOverPanel->YRadius      = 24;
        roundOverPanel->HitTest      = false;
    }

    roundOverPanel->Width   = 600;
    roundOverPanel->Height  = 200;
    roundOverPanel->Opacity = 0.0f;

    float targetX = (ClientWidth  - roundOverPanel->Width)  * 0.5f;
    float targetY = (ClientHeight - roundOverPanel->Height) * 0.5f;

    roundOverPanel->Position->X = targetX;
    roundOverPanel->Position->Y = -roundOverPanel->Height;

    roundOverPanel->Visible = true;
    roundOverPanel->BringToFront();

    if (!roundOverLabel) {
        roundOverLabel = new TLabel(this);
        roundOverLabel->Parent = roundOverPanel;
        roundOverLabel->Align  = TAlignLayout::Center;

        roundOverLabel->StyledSettings = TStyledSettings();
        roundOverLabel->TextSettings->Font->Family = "Cooper";
        roundOverLabel->TextSettings->Font->Size   = 32;
        roundOverLabel->TextSettings->Font->Style  = TFontStyles();
        roundOverLabel->TextSettings->FontColor    = TAlphaColorRec::White;
        roundOverLabel->TextSettings->HorzAlign    = TTextAlign::Center;
        roundOverLabel->TextSettings->VertAlign    = TTextAlign::Center;

        roundOverLabel->WordWrap = true;
        roundOverLabel->AutoSize = false;
    }

    roundOverLabel->Width    = roundOverPanel->Width;
    roundOverLabel->Height   = roundOverPanel->Height;
    roundOverLabel->Text     = text;
    roundOverLabel->Visible  = true;
    roundOverLabel->BringToFront();

    for (auto* btn : actionButtons) {
        if (btn) btn->Enabled = false;
    }

	CreateConfetti();

    TFloatAnimation* slideIn = new TFloatAnimation(roundOverPanel);
    slideIn->Parent           = roundOverPanel;
    slideIn->PropertyName     = "Position.Y";
    slideIn->StartFromCurrent = true;
	slideIn->StopValue        = targetY;
	slideIn->Duration         = 0.35f;
    slideIn->Enabled          = true;

    TFloatAnimation* fadeIn = new TFloatAnimation(roundOverPanel);
	fadeIn->Parent           = roundOverPanel;
	fadeIn->PropertyName     = "Opacity";
	fadeIn->StartFromCurrent = true;
	fadeIn->StopValue        = 0.9f;
	fadeIn->Duration         = 0.35f;
	fadeIn->Enabled          = true;

	if (roundOverTimer) {
		roundOverTimer->Interval = 3500;
		roundOverTimer->Enabled  = true;
	}
}

void TForm1::CreateConfetti()
{
    ClearConfetti();

    const int count = 60;

    for (int i = 0; i < count; ++i) {
        TCircle* c = new TCircle(nullptr);
        c->Parent = this;

        float size = RandRange(6.0f, 14.0f);
        c->Width  = size;
        c->Height = size;

        c->Position->X = RandRange(0.0f, ClientWidth - size);
        c->Position->Y = RandRange(-ClientHeight * 0.5f, -size);

        c->Stroke->Kind = TBrushKind::None;
        c->Fill->Kind   = TBrushKind::Solid;

        int colorPick = RandInt(0, 4);
        switch (colorPick) {
            case 0: c->Fill->Color = TAlphaColorRec::Red;      break;
            case 1: c->Fill->Color = TAlphaColorRec::Lime;     break;
            case 2: c->Fill->Color = TAlphaColorRec::Yellow;   break;
            case 3: c->Fill->Color = TAlphaColorRec::Aqua;     break;
            case 4: c->Fill->Color = TAlphaColorRec::Fuchsia;  break;
        }

        c->Opacity = 0.0f;
        c->Visible = true;

        TFloatAnimation* fade = new TFloatAnimation(c);
        fade->Parent           = c;
        fade->PropertyName     = "Opacity";
        fade->StartValue       = 0.0f;
        fade->StopValue        = 1.0f;
        fade->Duration         = 0.3f;
        fade->Enabled          = true;

        float dropTime = RandRange(0.8f, 1.6f);
        TFloatAnimation* fall = new TFloatAnimation(c);
        fall->Parent           = c;
        fall->PropertyName     = "Position.Y";
        fall->StartFromCurrent = true;
        fall->StopValue        = ClientHeight + 40.0f;
        fall->Duration         = dropTime;
        fall->Enabled          = true;

        TFloatAnimation* wiggle = new TFloatAnimation(c);
        wiggle->Parent           = c;
        wiggle->PropertyName     = "Position.X";
        wiggle->StartFromCurrent = true;
        wiggle->StopValue        = c->Position->X + RandRange(-40.0f, 40.0f);
        wiggle->Duration         = dropTime * 0.5f;
        wiggle->AutoReverse      = true;
        wiggle->Loop             = true;
        wiggle->Enabled          = true;

        confettiPieces.push_back(c);
    }
}

void TForm1::ClearConfetti()
{
    for (auto* c : confettiPieces)
    {
        if (!c) continue;

        c->Parent = nullptr;

        delete c;
    }

    confettiPieces.clear();
}

//---------------------------------------------------------------------------
// DEAL BUTTON CLICK
//---------------------------------------------------------------------------

void __fastcall TForm1::DealButtonClick(TObject *Sender)
{
    if (!game) return;

    bettingPhase     = false;
    dealerHoleHidden = true;

    if (betChipImage) {
    betChipImage->Visible = false;
    betChipImage->HitTest = false;
    betChipImage->Opacity = 0.5f;
}
if (betHintBackground)
    betHintBackground->Visible = false;

if (betChip1Image) {
    betChip1Image->Visible = false;
    betChip1Image->HitTest = false;
}
if (betChip50Image) {
    betChip50Image->Visible = false;
    betChip50Image->HitTest = false;
}
if (betHintLabel) {
    betHintLabel->Visible = false;
}
if (dealButton) {
    dealButton->Visible = false;
    dealButton->Enabled = false;
}
for (auto* btn : betConfirmButtons) {
    if (btn) btn->Visible = false;
}

	DestroyPlayerActionButtons();
	StopDeckShuffleAnimation();
	StartDealingAnimation();
}

//---------------------------------------------------------------------------
// PLAYER ACTION HANDLERS
//---------------------------------------------------------------------------

void __fastcall TForm1::HitButtonClick(TObject* Sender)   { playerHit(); }
void __fastcall TForm1::StandButtonClick(TObject* Sender) { playerStand(); }
void __fastcall TForm1::DoubleDownButtonClick(TObject* Sender) { playerDoubleDown(); }
void __fastcall TForm1::SplitButtonClick(TObject* Sender) { playerSplit(); }

// ---------------- PLAYER ACTIONS ----------------

void TForm1::playerHit() {
    if (!game || bettingPhase) return;

    BJPlayer& p = game->GetCurrentPlayer();
    BJHand&   h = game->GetCurrentHand();

    game->GetDeck().dealCardTo(h);

    // mark that this hand has acted
    p.markActionOnHand(game->getCurrentHandIndex());

    AnimateHitToCurrentHand();
}

void TForm1::playerStand() {
    if (!game || bettingPhase) return;

    BJPlayer& p = game->GetCurrentPlayer();
    int handIndex = game->getCurrentHandIndex();

    // mark that this hand has acted
    p.markActionOnHand(handIndex);

    if (game->advanceTurn()) {
        UpdateAllLabels();
        CreatePlayerActionButtons();
        return;
    }

    dealerHoleHidden = false;
    game->resolveDealerHand();
    game->settleBets();

    UpdateAllLabels();
    ShowRoundOverOverlay();
}

void TForm1::playerDoubleDown() {
    if (!game || bettingPhase) return;

    BJPlayer& p = game->GetCurrentPlayer();
    BJHand&   h = game->GetCurrentHand();
    int handIndex = game->getCurrentHandIndex();

    int bet = (handIndex == 0 ? p.getBet() : p.getSplitBet());
    if (bet > 0 && p.getChips() >= bet && !p.hasActedOnHand(handIndex)) {
        p.adjustChips(-bet);

        if (handIndex == 0)
            p.setBet(bet * 2);
        else
            p.setSplitBet(bet * 2);

        p.markActionOnHand(handIndex);

        game->GetDeck().dealCardTo(h);
        UpdateAllLabels();
        playerStand();
    } else {
        ShowMessage("You don't have enough chips to double down.");
    }
}

void TForm1::playerSplit() {
    if (!game || bettingPhase) return;

    BJPlayer& p = game->GetCurrentPlayer();

    if (!BJDecisionManager::canSplit(p, *game)) {
        ShowMessage("Cannot split this hand.");
        return;
    }

    BJHand& h = p.GetHand();
    const auto& cards = h.GetCards();
    if (cards.size() != 2) {
        ShowMessage("Split only works on exactly two cards.");
        return;
    }

    BJCard c1 = cards[0];
    BJCard c2 = cards[1];

    h.clear();
    h.addCard(c1);

    BJHand& sh = p.GetSplitHand();
    sh.clear();
    sh.addCard(c2);
    p.setHasSplit(true);

    int bet = p.getBet();
    if (bet > 0 && p.getChips() >= bet) {
        p.adjustChips(-bet);
        p.setSplitBet(bet);
    } else {
        p.setHasSplit(false);
        sh.clear();
        ShowMessage("Not enough chips to split.");
        return;
    }

	// mark both hands as having acted, so no double down afterward
    p.markActionOnHand(0);
    p.markActionOnHand(1);

    DestroyPlayerActionButtons();
    AnimateSplitForCurrentHand();
}


//---------------------------------------------------------------------------
// FORM CLOSE
//---------------------------------------------------------------------------

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
	Application->Terminate();
	Action = TCloseAction::caFree;
}

//---------------------------------------------------------------------------
// UPDATE ALL LABELS
//---------------------------------------------------------------------------

void TForm1::UpdateAllLabels()
{
    if (!game) return;

    Settings& s = Settings::getInstance();
    int playerCount = s.player_count;

    if (bettingPhase)
    {
        if (dealerLabel)       dealerLabel->Visible = false;
        if (dealerBackground)  dealerBackground->Visible = false;

        ClearDealerCardImages();
        ClearPlayerCardImages();

        for (int i = 0; i < playerCount; ++i)
        {
            BJPlayer& p = game->GetPlayer(i);

            if (i < (int)playerNameLabels.size() && playerNameLabels[i]) {
                TLabel* lbl = playerNameLabels[i];
                lbl->Visible = true;

                lbl->StyledSettings = TStyledSettings();
                lbl->TextSettings->Font->Family = "Cooper";
                lbl->TextSettings->Font->Size   = 22;
                lbl->TextSettings->HorzAlign    = TTextAlign::Center;

                lbl->Text =
                    "Player " + IntToStr(i + 1) +
                    " - $" + IntToStr(p.getChips()) +
                    "  (Bet: $" + IntToStr(p.getBet()) + ")";

                lbl->AutoSize = true;
                lbl->ApplyStyleLookup();

                float centerX = (i + 1) * (ClientWidth / (playerCount + 1.0f));
                lbl->Position->X = centerX - (lbl->Width * 0.5f);
                lbl->Position->Y = ClientHeight * 0.70f;

                if (i < (int)playerNameBackgrounds.size() && playerNameBackgrounds[i]) {
                    TRectangle* bg = playerNameBackgrounds[i];
                    bg->Visible = lbl->Visible;
                    bg->Width   = lbl->Width + 20;
                    bg->Height  = lbl->Height + 10;
                    bg->Position->X = lbl->Position->X - 10;
                    bg->Position->Y = lbl->Position->Y - 5;
                    bg->SendToBack();
                }

                if (p.isBankrupt() || p.getChips() <= 0) {
                    lbl->FontColor = TAlphaColorRec::Gray;
                } else if (i == bettingPlayerIndex) {
                    lbl->FontColor = TAlphaColorRec::Gold;
                } else {
                    lbl->FontColor = TAlphaColorRec::White;
                }
            }

            if (i < (int)playerInfoLabels.size() && playerInfoLabels[i])
                playerInfoLabels[i]->Visible = false;
            if (i < (int)splitInfoLabels.size() && splitInfoLabels[i])
                splitInfoLabels[i]->Visible = false;
            if (i < (int)mainHandTitleLabels.size() && mainHandTitleLabels[i])
                mainHandTitleLabels[i]->Visible = false;
            if (i < (int)splitHandTitleLabels.size() && splitHandTitleLabels[i])
                splitHandTitleLabels[i]->Visible = false;

            if (i < (int)playerGlowEffects.size() && playerGlowEffects[i])
                playerGlowEffects[i]->Enabled = false;
            if (i < (int)splitGlowEffects.size() && splitGlowEffects[i])
                splitGlowEffects[i]->Enabled = false;

            if (i < (int)betConfirmButtons.size() && betConfirmButtons[i]) {
                TButton* btn = betConfirmButtons[i];
                btn->Visible = true;

                BJPlayer& p2 = game->GetPlayer(i);
				btn->Enabled = (i == bettingPlayerIndex && !p2.isBankrupt());
            }
        }

        return;
    }

    if (dealerLabel) {
        dealerLabel->Visible = true;
        dealerLabel->StyledSettings = TStyledSettings();
		dealerLabel->TextSettings->Font->Family = "Cooper";
		dealerLabel->TextSettings->Font->Size   = 20;
		dealerLabel->TextSettings->HorzAlign    = TTextAlign::Center;
		dealerLabel->TextSettings->FontColor = TAlphaColorRec::White;

        BJHand& dh = game->GetDealer().GetHand();
        const auto& cards = dh.GetCards();

        String points;
        if (dealerHoleHidden && cards.size() >= 2)
            points = "?";
        else
            points = DescribePoints(dh);

        dealerLabel->Text = "Dealer\r\nPoints: " + points;

        dealerLabel->AutoSize = true;
        dealerLabel->ApplyStyleLookup();
		dealerLabel->Repaint();

		dealerLabel->Position->Y = 40;
		dealerLabel->Position->X = (ClientWidth - dealerLabel->Width) * 0.5f;

        if (!dealerBackground)
        {
            dealerBackground = new TRectangle(this);
            dealerBackground->Parent = this;

            dealerBackground->Fill->Kind   = TBrushKind::Solid;
            dealerBackground->Fill->Color  = TAlphaColorRec::Black;
            dealerBackground->Opacity      = 0.35f;
            dealerBackground->Stroke->Kind = TBrushKind::None;

            dealerBackground->XRadius = 12;
            dealerBackground->YRadius = 12;
        }

        dealerBackground->Width  = dealerLabel->Width  + 30;
        dealerBackground->Height = dealerLabel->Height + 20;

        dealerBackground->Position->X = dealerLabel->Position->X - 15;
        dealerBackground->Position->Y = dealerLabel->Position->Y - 10;

        dealerBackground->Visible = true;
        dealerBackground->SendToBack();
    }

    int activePlayer = game->getCurrentPlayerIndex();
    int activeHand   = game->getCurrentHandIndex();

    const float cardW        = 90.f;
    const float cardH        = 130.f;
    const float stackOffsetY = 25.f;
    const float baseYOffset  = 130.f;
    const float extraPlayerSpacing = 40.f;

    float buttonsY = ClientHeight - 80.f;
    float baseY    = buttonsY - cardH - baseYOffset;

    for (int i = 0; i < playerCount; ++i) {
        BJPlayer& p = game->GetPlayer(i);
        BJHand&   h = p.GetHand();
        BJHand&   sh = p.GetSplitHand();
        const auto& mainCards  = h.GetCards();
        const auto& splitCards = sh.GetCards();
        bool hasSplit = p.hasSplitHand();

        float baseCenterX = (i + 1) * (ClientWidth / (playerCount + 1.0f));
        float centerX     = baseCenterX;
        if (playerCount > 1) {
            float offset = (i - (playerCount - 1) / 2.0f) * extraPlayerSpacing;
            centerX += offset;
        }

        float mainCenterX  = centerX;
        float splitCenterX = centerX;
        if (hasSplit) {
            const float pairGap = 35.f;
            mainCenterX  = centerX - (cardW + pairGap) * 0.5f;
            splitCenterX = centerX + (cardW + pairGap) * 0.5f;
        }

        if (i < (int)playerNameLabels.size() && playerNameLabels[i]) {
            TLabel* lbl = playerNameLabels[i];
            lbl->StyledSettings = TStyledSettings();
            lbl->TextSettings->Font->Family = "Cooper";
            lbl->TextSettings->Font->Size   = 20;
            lbl->TextSettings->HorzAlign    = TTextAlign::Center;
            lbl->Text =
                "Player " + IntToStr(i + 1) +
                " - $" + IntToStr(p.getChips());
            lbl->Visible = true;

            if (p.isBankrupt() || p.getChips() <= 0)
                lbl->FontColor = TAlphaColorRec::Gray;
            else
                lbl->FontColor = TAlphaColorRec::White;

            lbl->AutoSize = true;
			lbl->ApplyStyleLookup();
            lbl->Position->X = centerX - (lbl->Width * 0.5f);
            lbl->Position->Y = baseY - lbl->Height - 10.f;

            if (i < (int)playerNameBackgrounds.size() && playerNameBackgrounds[i]) {
                TRectangle* bg = playerNameBackgrounds[i];
                bg->Visible = true;
                bg->Width   = lbl->Width + 20;
                bg->Height  = lbl->Height + 10;
                bg->Position->X = lbl->Position->X - 10;
                bg->Position->Y = lbl->Position->Y - 5;
                bg->SendToBack();
            }
        }

        if (i < (int)playerInfoLabels.size() && playerInfoLabels[i]) {
            TLabel* info = playerInfoLabels[i];
            info->Visible = true;

            info->StyledSettings = TStyledSettings();
            info->TextSettings->Font->Family = "Cooper";
            info->TextSettings->Font->Size   = 16;
            info->TextSettings->HorzAlign    = TTextAlign::Center;

            info->Text =
                "Points: " + DescribePoints(h) +
                "\nBet: $" + IntToStr(p.getBet());

            int val = h.value();
            bool active = (i == activePlayer && activeHand == 0 && val <= 21);

            if (!dealerHoleHidden && p.getBet() > 0) {
                int outcome = p.getRoundOutcomeMain();
                if (outcome > 0)
                    info->FontColor = TAlphaColorRec::Lime;
                else if (outcome < 0)
                    info->FontColor = TAlphaColorRec::Red;
                else
                    info->FontColor = TAlphaColorRec::White;
            } else {
                if (val > 21)
                    info->FontColor = TAlphaColorRec::Red;
                else if (active)
                    info->FontColor = TAlphaColorRec::Gold;
                else
                    info->FontColor = TAlphaColorRec::White;
            }

            bool mainActive = active && dealerHoleHidden;

            if (i < (int)playerGlowEffects.size() && playerGlowEffects[i])
                playerGlowEffects[i]->Enabled = mainActive;
            if (i < (int)playerGlowAnims.size() && playerGlowAnims[i])
                playerGlowAnims[i]->Enabled = mainActive;

            info->Repaint();

            info->AutoSize = true;
            info->ApplyStyleLookup();
            float extraY = 0.f;
            if (hasSplit && mainCards.size() > 1) {
                extraY = (mainCards.size() - 1) * stackOffsetY;
            }
            info->Position->X = mainCenterX - info->Width * 0.5f;
            info->Position->Y = baseY + cardH + 8.f + extraY;

            if (p.hasSplitHand() &&
                i < (int)mainHandTitleLabels.size() && mainHandTitleLabels[i]) {
                mainHandTitleLabels[i]->Visible = true;
            } else if (i < (int)mainHandTitleLabels.size() && mainHandTitleLabels[i]) {
                mainHandTitleLabels[i]->Visible = false;
            }
        }

        if (p.hasSplitHand() &&
            i < (int)splitInfoLabels.size() && splitInfoLabels[i]) {

            TLabel* info2 = splitInfoLabels[i];
            info2->Visible = true;

            BJHand& sh2 = p.GetSplitHand();
            info2->StyledSettings = TStyledSettings();
            info2->TextSettings->Font->Family = "Cooper";
            info2->TextSettings->Font->Size   = 16;
            info2->TextSettings->HorzAlign    = TTextAlign::Center;

            info2->Text =
                "Points: " + DescribePoints(sh2) +
                "\nBet: $" + IntToStr(p.getSplitBet());

            int val2      = sh2.value();
            bool active2  = (i == activePlayer && activeHand == 1 && val2 <= 21);

            if (!dealerHoleHidden && p.getSplitBet() > 0) {
                int outcome2 = p.getRoundOutcomeSplit();
                if (outcome2 > 0)
                    info2->FontColor = TAlphaColorRec::Lime;
                else if (outcome2 < 0)
                    info2->FontColor = TAlphaColorRec::Red;
                else
                    info2->FontColor = TAlphaColorRec::White;
            } else {
                if (val2 > 21)
                    info2->FontColor = TAlphaColorRec::Red;
                else if (active2)
                    info2->FontColor = TAlphaColorRec::Gold;
                else
                    info2->FontColor = TAlphaColorRec::White;
            }

            bool splitActive = active2 && dealerHoleHidden;

            if (i < (int)splitGlowEffects.size() && splitGlowEffects[i])
				splitGlowEffects[i]->Enabled = splitActive;
			if (i < (int)splitGlowAnims.size() && splitGlowAnims[i])
                splitGlowAnims[i]->Enabled = splitActive;

            info2->Repaint();

            info2->AutoSize = true;
            info2->ApplyStyleLookup();
            float extraY2 = 0.f;
            if (splitCards.size() > 1) {
                extraY2 = (splitCards.size() - 1) * stackOffsetY;
            }
            info2->Position->X = splitCenterX - info2->Width * 0.5f;
            info2->Position->Y = baseY + cardH + 8.f + extraY2;

            if (i < (int)splitHandTitleLabels.size() && splitHandTitleLabels[i])
                splitHandTitleLabels[i]->Visible = true;
        }
        else if (i < (int)splitInfoLabels.size() && splitInfoLabels[i]) {
            splitInfoLabels[i]->Visible = false;
            if (i < (int)splitGlowEffects.size() && splitGlowEffects[i])
				splitGlowEffects[i]->Enabled = false;
            if (i < (int)splitHandTitleLabels.size() && splitHandTitleLabels[i])
                splitHandTitleLabels[i]->Visible = false;
        }
	}

    if (!dealingAnimationActive) {
        DrawDealerCards();
        DrawPlayerCards();
	}
}







