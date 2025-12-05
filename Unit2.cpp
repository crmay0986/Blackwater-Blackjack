//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include "Unit2.h"
#include "UnitFinal.h"

#include <FMX.ListBox.hpp>

//---------------------------------------------------------------------------

#pragma package(smart_init)
#pragma resource "*.fmx"

TFormMainMenu *FormMainMenu;

//---------------------------------------------------------------------------
// CONSTRUCTOR
//---------------------------------------------------------------------------

__fastcall TFormMainMenu::TFormMainMenu(TComponent* Owner)
    : TForm(Owner)
{
    lblStartGame->HitTest = true;
    lblStartGame->Cursor  = crHandPoint;

    rectStartGame->HitTest = true;
    rectStartGame->Cursor  = crHandPoint;

    Settings& s = Settings::getInstance();

    ComboBoxPlayerCountMain->Items->Clear();
    ComboBoxPlayerCountMain->Items->Add("1");
    ComboBoxPlayerCountMain->Items->Add("2");
    ComboBoxPlayerCountMain->Items->Add("3");
    ComboBoxPlayerCountMain->Items->Add("4");

	int idxPlayers = s.player_count - 1;
    if (idxPlayers < 0) idxPlayers = 0;
    if (idxPlayers > 3) idxPlayers = 3;

    ComboBoxPlayerCountMain->ItemIndex = idxPlayers;
    ComboBoxPlayerCountMain->OnChange  = ComboBoxPlayerCountMainChange;

    EditStartMoney->Text   = IntToStr(s.player_initial_chips);
	EditStartMoney->OnExit = EditStartMoneyExit;

	EditStartMoney->OnKeyDown = EditNumbersOnly;
	EditGoalMoney->OnKeyDown  = EditNumbersOnly;

    EditGoalMoney->Text   = IntToStr(s.goal_amount);
    EditGoalMoney->OnExit = EditGoalMoneyExit;

    ComboBoxPlayerCountMain->ApplyStyleLookup();

	ComboBoxPlayerCountMain->Width  = 121;
	ComboBoxPlayerCountMain->Height = 48;

	Fmx::Listbox::TComboListBox* lb = ComboBoxPlayerCountMain->ListBox;
    if (lb) {
        for (int i = 0; i < lb->Items->Count; ++i) {
            TListBoxItem* item = dynamic_cast<TListBoxItem*>(lb->ListItems[i]);
            if (!item) continue;

            item->Height = 32;
            item->StyledSettings = TStyledSettings();

            item->TextSettings->Font->Family = "Cooper";
			item->TextSettings->Font->Size   = 24;
            item->TextSettings->HorzAlign    = TTextAlign::Center;
        }
    }
}

//---------------------------------------------------------------------------
// MAIN START BUTTON LOGIC
//---------------------------------------------------------------------------

void __fastcall TFormMainMenu::btnStartClick(TObject *Sender)
{
    Settings& s = Settings::getInstance();

    EditStartMoneyExit(EditStartMoney);
    EditGoalMoneyExit(EditGoalMoney);

    if (s.goal_amount <= s.player_initial_chips) {
        ShowMessage("Goal money must be greater than starting money.");
        return;
    }

    if (Form1) {
        Form1->Show();
        this->Hide();
    } else {
        ShowMessage("Game form (Form1) has not been created.");
    }
}

//---------------------------------------------------------------------------
// Clicking the label should start the game
//---------------------------------------------------------------------------

void __fastcall TFormMainMenu::lblStartGameClick(TObject *Sender)
{
    btnStartClick(Sender);
}

//---------------------------------------------------------------------------
// Clicking the rectangle should also start the game
//---------------------------------------------------------------------------

void __fastcall TFormMainMenu::rectStartGameClick(TObject *Sender)
{
    btnStartClick(Sender);
}

//---------------------------------------------------------------------------
// Hover effects for the rectangle
//---------------------------------------------------------------------------

void __fastcall TFormMainMenu::rectStartGameMouseEnter(TObject *Sender)
{
    rectStartGame->Opacity = 0.95f;
}

void __fastcall TFormMainMenu::rectStartGameMouseLeave(TObject *Sender)
{
    rectStartGame->Opacity = 1.0f;
}

//---------------------------------------------------------------------------
// PLAYER COUNT: 1–4
//---------------------------------------------------------------------------

void __fastcall TFormMainMenu::ComboBoxPlayerCountMainChange(TObject *Sender)
{
    auto cb = dynamic_cast<TComboBox*>(Sender);
    if (!cb) return;

    if (cb->ItemIndex < 0 || cb->ItemIndex >= cb->Items->Count)
		return;

    int val = StrToInt(cb->Items->Strings[cb->ItemIndex]);
    if (val < 1) val = 1;
	if (val > 4) val = 4;

    Settings::getInstance().player_count = val;
}

void __fastcall TFormMainMenu::EditNumbersOnly(TObject *Sender,
                                               System::Word &Key,
                                               System::WideChar &KeyChar,
                                               TShiftState Shift)
{
    if (Key == vkBack || Key == vkDelete ||
        Key == vkLeft || Key == vkRight ||
        Key == vkHome || Key == vkEnd)
    {
        return;
    }

    if (KeyChar < '0' || KeyChar > '9')
    {
        Key     = 0;
        KeyChar = 0;
    }
}

void __fastcall TFormMainMenu::EditFixEmpty(TObject *Sender)
{
    TEdit* e = dynamic_cast<TEdit*>(Sender);
    if (!e) return;
    if (e->Text.IsEmpty())
        e->Text = "0";
}

//---------------------------------------------------------------------------
// STARTING MONEY EXIT: sync to Settings, fix invalid input
//---------------------------------------------------------------------------

void __fastcall TFormMainMenu::EditStartMoneyExit(TObject *Sender)
{
    auto edit = dynamic_cast<TEdit*>(Sender);
    if (!edit) return;

    int value = StrToIntDef(edit->Text, -1);

    if (value <= 0) {
        value = 100;
        edit->Text = IntToStr(value);
    }

    Settings::getInstance().player_initial_chips = value;
}

//---------------------------------------------------------------------------
// GOAL MONEY EXIT: sync to Settings, fix invalid input
//---------------------------------------------------------------------------

void __fastcall TFormMainMenu::EditGoalMoneyExit(TObject *Sender)
{
    auto edit = dynamic_cast<TEdit*>(Sender);
    if (!edit) return;

    int value = StrToIntDef(edit->Text, -1);

    if (value <= 0) {
        value = 200;
        edit->Text = IntToStr(value);
    }

	Settings::getInstance().goal_amount = value;
}

//---------------------------------------------------------------------------

