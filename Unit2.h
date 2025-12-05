//---------------------------------------------------------------------------

#ifndef Unit2H
#define Unit2H
//---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Edit.hpp>
#include <FMX.ListBox.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <FMX.Objects.hpp>
#include <FMX.WebBrowser.hpp>
#include <System.UITypes.hpp>


//---------------------------------------------------------------------------

class TFormMainMenu : public TForm
{
__published:    // IDE-managed Components
    TRectangle *rectStartGame;
	TImage *Image1;
    TLabel *lblStartGame;

    // Player settings
    TComboBox *ComboBoxPlayerCountMain;

    // Money inputs (user-typed)
    TEdit *EditStartMoney;
	TEdit *EditGoalMoney;
	TLabel *Label4;
	TLabel *Label5;
	TLabel *Label6;

	// Event handlers
    void __fastcall btnStartClick(TObject *Sender);
	void __fastcall lblStartGameClick(TObject *Sender);
    void __fastcall rectStartGameClick(TObject *Sender);
    void __fastcall rectStartGameMouseEnter(TObject *Sender);
	void __fastcall rectStartGameMouseLeave(TObject *Sender);
	// numbers-only keypress handler
void __fastcall EditNumbersOnly(TObject *Sender,
                                System::Word &Key,
                                System::WideChar &KeyChar,
								TShiftState Shift);

void __fastcall EditFixEmpty(TObject *Sender);




    void __fastcall ComboBoxPlayerCountMainChange(TObject *Sender);

    // Money edit events (simple TNotifyEvent)
    void __fastcall EditStartMoneyExit(TObject *Sender);
    void __fastcall EditGoalMoneyExit(TObject *Sender);

public:         // User declarations
    __fastcall TFormMainMenu(TComponent* Owner);
};

//---------------------------------------------------------------------------

extern PACKAGE TFormMainMenu *FormMainMenu;

//---------------------------------------------------------------------------

#endif

