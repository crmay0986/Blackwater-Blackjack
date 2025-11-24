//---------------------------------------------------------------------------
#include <vector>
#include <string>
#include <algorithm>
#include <random>

#include <fmx.h>
#pragma hdrstop

#include "Unit4.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
#pragma resource ("*.Windows.fmx", _PLAT_MSWINDOWS)

enum class Suit {Spades, Hearts, Clubs, Diamonds};
enum class Rank {Two = 2, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace};

class Card {
	private: // Cards can't be modified once created
		Suit suit;
		Rank rank;
	public:

		Card(Suit s, Rank r) : suit(s), rank(r) {}
		// BASIC GETTER: suit
		Suit getSuit() const { return suit; }
		// BASIC GETTER: rank
		Rank getRank() const { return rank; }
		// BASIC STRINGIFY:
		std::string toString() const {

			static const char* suitNames[] =
			{
			"Spades",
			"Hearts",
			"Clubs",
			"Diamonds"
			};

			static const char* rankNames[] =
			{
				"",
				"",
				"2",
				"3",
				"4",
				"5",
				"6",
				"7",
				"8",
				"9",
				"10",
				"J",
				"Q",
				"K",
				"A"
			};

			return std::string(rankNames[(int)rank]) + " of " + suitNames[(int)suit];

		}

};

class Hand {
	private:
		std::vector<Card> cards;
	public: // No constructor since hand starts empty.

		void addCard(const Card& c) {
			cards.push_back(c);
		}
		// Number of cards in hand
		int size() const {
			return cards.size();
		}
		// Clear all cards from hand (do not deallocate memory)
		void clear() {
			cards.clear();
		}
		// BASIC GETTER: cards
		const std::vector<Card>& getCards() const {
			return cards;
		}
		// BASIC STRINGIFY:
		std::string toString() const {
			std::string output;
			for (const auto& card : cards) {
				output += card.toString() + "\n";
			}
			return output;
		}

};

class Deck {
	private:
		std::vector<Card> cards;
		int index = 0; // To keep track of cards dealt.
	public:
		Deck() {
			resetDeck();
		}

		void resetDeck() {
			cards.clear();

			for (int s = 0; s < 4; ++s) {
				for (int r = 2; r <= 14; ++r) {
					// Constructs card inside vector instead of outside (.push_back())
					cards.emplace_back((Suit)s, (Rank)r);
				}
			}

			index = 0;
			shuffleCards();
		}

		void shuffleCards() {
			// Static because the instance doesn't matter
			// "mt19937" is abbreviated for a good random number generator
			// used in video games.
			static std::mt19937 rng(std::random_device{}());
			// rng is used for shuffle seed.
			std::shuffle(cards.begin(), cards.end(), rng);
		}

		Card drawCard() { // Checks and validates that a card can be dealt
			if (index >= cards.size()) {
				throw std::runtime_error("Deck empty.");
			}

			return cards[index++];
		}

		// Pass-By-Reference to easily modify hand outside deck class
		void dealCardTo(Hand& hand) {
			hand.addCard(drawCard());
		}

        int remaining() const {
        	return cards.size() - index;
		}

};

class Player {

};

class Dealer {

};

class Game {

};

TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm1::StartGameButtonClick(TObject *Sender)
{
	for (int i = 0; i < 3; i) {
		TButton* playerButton = new TButton(this); // this sets the owner to the form. Important
		playerButton->Parent = this; // Sets form as visual parent. Important

		// Give Names
		playerButton->Name = "DynamicButton" + IntToStr(i);
		// Give Positions




		// Assign the shared event handler function
		if () {
			playerButton->OnClick = FoldCards();
		}
		playerButton->OnClick = DynamicButtonClick;

		// Button ID:
		playerButton->Tag = i;
	}
}
//---------------------------------------------------------------------------
