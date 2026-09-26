#include <cstdio>
#include <string>

#include "no_vcl.hpp"

using namespace no_vcl;

namespace
{

int g_destroyedLabels = 0;

// ラッパーが実際に delete されたかを数えるためのラベル。
// コンポーネントの派生クラスは、ヒープ生成を強制するためデストラクタを protected にする。
class TTracedLabel : public TLabel
{
public:
    explicit TTracedLabel(TComponent* AOwner) : TLabel(AOwner) {}

protected:
    ~TTracedLabel() override
    {
        ++g_destroyedLabels;
        std::printf("~TTracedLabel: destroyed labels=%d\n", g_destroyedLabels);
        std::fflush(stdout);
    }
};

// C++Builder のフォームユニットと同じく、フォームはグローバルなポインタ変数で持つ。
class TMainForm;
TMainForm* Form1 = nullptr;

// 閉じ方の確認用のサブフォーム。
//   - Release me ボタン: 自身のボタンのハンドラの中から Release() する(Free() と違い安全)。
//   - ウィンドウを閉じる: OnClose で Action = caFree にし、LCL に Release させる。
class TSubForm : public TForm
{
public:
    TButton* ReleaseButton;

    explicit TSubForm(TComponent* AOwner) : TForm(AOwner)
    {
        Caption = "Sub form";
        Width = 260;
        Height = 120;

        ReleaseButton = new TButton(this);
        ReleaseButton->Parent = this;
        ReleaseButton->Caption = "Release me";
        ReleaseButton->Left = 20;
        ReleaseButton->Top = 20;
        ReleaseButton->Width = 120;
        ReleaseButton->OnClick = [this](TObject*) {
            std::printf("TSubForm: Release() from its own button\n");
            std::fflush(stdout);
            Release();
        };

        // new で直接生成したフォームの OnCreate は、最初に表示される直前に呼ばれる。
        OnCreate = [this](TObject* Sender) {
            std::printf("TSubForm OnCreate: Sender is this: %s\n", Sender == this ? "yes" : "no");
            std::fflush(stdout);
        };
        OnClose = [](TObject*, TCloseAction& Action) {
            std::printf("TSubForm OnClose: default Action=%d, set caFree\n", (int)Action);
            std::fflush(stdout);
            Action = caFree;
        };
        OnActivate = [](TObject*) { std::printf("TSubForm OnActivate\n"); std::fflush(stdout); };
        OnDeactivate = [](TObject*) { std::printf("TSubForm OnDeactivate\n"); std::fflush(stdout); };
        OnHide = [](TObject*) { std::printf("TSubForm OnHide\n"); std::fflush(stdout); };
        OnDestroy = [this](TObject*) {
            std::printf("TSubForm OnDestroy: ReleaseButton->Caption=%s\n", std::string(ReleaseButton->Caption).c_str());
            std::fflush(stdout);
        };
    }

protected:
    ~TSubForm() override
    {
        std::printf("~TSubForm\n");
        std::fflush(stdout);
    }
};

// デザイナーが生成することを想定した形のフォーム。
// コントロールは生ポインタメンバとして持ち、コンストラクタ本体で new する(破棄は Owner に任せる)。
// イベントハンドラはメンバ関数にし、[this] のラムダで割り当てる。
class TMainForm : public TForm
{
public:
    TButton*      Button1;
    TLabel*       Label1;
    TEdit*        Edit1;
    TCheckBox*    CheckBox1;
    TRadioButton* RadioButton1;
    TRadioButton* RadioButton2;
    TPanel*       Panel1;
    TButton*      PanelButton;
    TGroupBox*    GroupBox1;
    TComboBox*    ComboBox1;
    TListBox*     ListBox1;
    TMemo*        Memo1;
    TTracedLabel* TickLabel;
    TTimer*       Timer1;
    TPaintBox*    PaintBox1;

    // C++Builder と同じく Owner を受け取り、TForm に渡す(Application->CreateForm が Application を渡す)。
    explicit TMainForm(TComponent* AOwner) : TForm(AOwner)
    {
        Caption = "no_vcl C++ wrapper";
        Width = 640;
        Height = 420;

        Button1 = new TButton(this);
        Button1->Parent = this;
        Button1->Caption = "Click me";
        Button1->Left = 20;
        Button1->Top = 20;
        Button1->Width = 100;
        Button1->Height = 30;
        Button1->OnClick = [this](TObject* Sender) { Button1Click(Sender); };

        Label1 = new TLabel(this);
        Label1->Parent = this;
        Label1->Caption = "Label text";
        Label1->Left = 20;
        Label1->Top = 60;

        Edit1 = new TEdit(this);
        Edit1->Parent = this;
        Edit1->Text = "Edit me";
        Edit1->Left = 20;
        Edit1->Top = 90;
        Edit1->Width = 150;
        Edit1->OnChange = [this](TObject* Sender) { TextChange(Sender); };

        CheckBox1 = new TCheckBox(this);
        CheckBox1->Parent = this;
        CheckBox1->Caption = "Check me";
        CheckBox1->Left = 20;
        CheckBox1->Top = 130;
        CheckBox1->OnClick = [this](TObject* Sender) { CheckBox1Click(Sender); };

        // 2つのラジオボタンで同じハンドラを共有し、Sender でどちらが押されたかを区別する。
        RadioButton1 = new TRadioButton(this);
        RadioButton1->Parent = this;
        RadioButton1->Caption = "Option A";
        RadioButton1->Left = 20;
        RadioButton1->Top = 160;
        RadioButton1->Checked = true;
        RadioButton1->OnClick = [this](TObject* Sender) { RadioButtonClick(Sender); };

        RadioButton2 = new TRadioButton(this);
        RadioButton2->Parent = this;
        RadioButton2->Caption = "Option B";
        RadioButton2->Left = 20;
        RadioButton2->Top = 190;
        RadioButton2->OnClick = RadioButton1->OnClick;

        Panel1 = new TPanel(this);
        Panel1->Parent = this;
        Panel1->Left = 220;
        Panel1->Top = 20;
        Panel1->Width = 180;
        Panel1->Height = 60;

        // Panel の中にボタンを置く(Parent が TWinControl* なので Panel も親にできる)。
        PanelButton = new TButton(this);
        PanelButton->Parent = Panel1;
        PanelButton->Caption = "In panel";
        PanelButton->Left = 10;
        PanelButton->Top = 15;
        PanelButton->OnClick = [this](TObject* Sender) { PanelButtonClick(Sender); };

        GroupBox1 = new TGroupBox(this);
        GroupBox1->Parent = this;
        GroupBox1->Caption = "Group";
        GroupBox1->Left = 220;
        GroupBox1->Top = 90;
        GroupBox1->Width = 180;
        GroupBox1->Height = 60;

        ComboBox1 = new TComboBox(this);
        ComboBox1->Parent = this;
        ComboBox1->ItemsAdd("Combo A");
        ComboBox1->ItemsAdd("Combo B");
        ComboBox1->ItemsAdd("Combo C");
        ComboBox1->ItemIndex = 0;
        ComboBox1->Left = 220;
        ComboBox1->Top = 160;
        ComboBox1->Width = 150;
        ComboBox1->OnChange = [this](TObject* Sender) { TextChange(Sender); };

        ListBox1 = new TListBox(this);
        ListBox1->Parent = this;
        ListBox1->ItemsAdd("List 1");
        ListBox1->ItemsAdd("List 2");
        ListBox1->ItemsAdd("List 3");
        ListBox1->Left = 220;
        ListBox1->Top = 190;
        ListBox1->Width = 150;
        ListBox1->Height = 80;
        ListBox1->OnClick = [this](TObject* Sender) { ListBox1Click(Sender); };

        // TMemo は TCustomEdit の派生なので、Edit1 と同じ TextChange を共有できる。
        Memo1 = new TMemo(this);
        Memo1->Parent = this;
        Memo1->LinesAdd("Memo line 1");
        Memo1->LinesAdd("Memo line 2");
        Memo1->Left = 220;
        Memo1->Top = 280;
        Memo1->Width = 150;
        Memo1->Height = 80;
        Memo1->OnChange = Edit1->OnChange;

        TickLabel = new TTracedLabel(this);
        TickLabel->Parent = this;
        TickLabel->Caption = "Tick: 0";
        TickLabel->Left = 20;
        TickLabel->Top = 230;

        Timer1 = new TTimer(this);
        Timer1->Interval = 500;
        Timer1->OnTimer = [this](TObject* Sender) { Timer1Timer(Sender); };
        Timer1->Enabled = true;

        PaintBox1 = new TPaintBox(this);
        PaintBox1->Parent = this;
        PaintBox1->Left = 400;
        PaintBox1->Top = 20;
        PaintBox1->Width = 220;
        PaintBox1->Height = 130;
        PaintBox1->OnPaint = [this](TObject* Sender) { PaintBox1Paint(Sender); };

        OpenSubButton = new TButton(this);
        OpenSubButton->Parent = this;
        OpenSubButton->Caption = "Open sub";
        OpenSubButton->Left = 400;
        OpenSubButton->Top = 170;
        OpenSubButton->OnClick = [this](TObject* Sender) { OpenSubButtonClick(Sender); };

        OnCreate = [this](TObject* Sender) { FormCreate(Sender); };
        OnShow = [this](TObject* Sender) { FormShow(Sender); };
        OnCloseQuery = [this](TObject* Sender, bool& CanClose) { FormCloseQuery(Sender, CanClose); };
        OnClose = [this](TObject* Sender, TCloseAction& Action) { FormClose(Sender, Action); };
        OnDestroy = [this](TObject* Sender) { FormDestroy(Sender); };
    }

    TButton* OpenSubButton;

protected:
    // Application が所有するフォームは、main から戻った後にまとめて破棄される。
    ~TMainForm() override
    {
        std::printf("~TMainForm\n");
        std::fflush(stdout);
    }

private:
    int clicks_ = 0;
    int ticks_ = 0;
    int closeAttempts_ = 0;

    void FormCreate(TObject* Sender)
    {
        std::printf("FormCreate: Sender is Form: %s, Form1 assigned: %s\n",
                    Sender == this ? "yes" : "no", Form1 == this ? "yes" : "no");
        std::fflush(stdout);
    }

    void FormShow(TObject*)
    {
        std::printf("FormShow\n");
        std::fflush(stdout);
    }

    // 閉じる操作の 1 回目は OnCloseQuery で、2 回目は OnClose で取りやめ、
    // 3 回目は既定の動作(MainForm なので caFree = アプリケーションの終了)のままにする。
    void FormCloseQuery(TObject*, bool& CanClose)
    {
        ++closeAttempts_;
        std::printf("FormCloseQuery: attempt=%d, default CanClose=%d\n", closeAttempts_, (int)CanClose);
        if (closeAttempts_ == 1)
        {
            CanClose = false;
            std::printf("FormCloseQuery: blocked (CanClose = false)\n");
        }
        std::fflush(stdout);
    }

    void FormClose(TObject*, TCloseAction& Action)
    {
        std::printf("FormClose: attempt=%d, default Action=%d\n", closeAttempts_, (int)Action);
        if (closeAttempts_ == 2)
        {
            Action = caNone;
            std::printf("FormClose: blocked (Action = caNone)\n");
        }
        std::fflush(stdout);
    }

    // main から戻った後の終了処理で呼ばれる。この時点では子コントロールもまだ有効。
    void FormDestroy(TObject*)
    {
        std::printf("FormDestroy: Button1->Caption=%s\n", std::string(Button1->Caption).c_str());
        std::fflush(stdout);
    }

    void OpenSubButtonClick(TObject*);

    void Button1Click(TObject* Sender)
    {
        TButton* button = static_cast<TButton*>(Sender);
        ++clicks_;
        button->Caption = "Clicked " + std::to_string(clicks_);
        std::printf("Button1Click: Sender is Button1: %s, count=%d\n", Sender == Button1 ? "yes" : "no", clicks_);
        std::fflush(stdout);
    }

    void CheckBox1Click(TObject* Sender)
    {
        std::printf("CheckBox1Click: Checked=%d\n", (bool)static_cast<TCheckBox*>(Sender)->Checked);
        std::fflush(stdout);
    }

    void RadioButtonClick(TObject* Sender)
    {
        TRadioButton* radio = static_cast<TRadioButton*>(Sender);
        std::printf("RadioButtonClick: %s Checked=%d\n", std::string(radio->Caption).c_str(), (bool)radio->Checked);
        std::fflush(stdout);
    }

    void PanelButtonClick(TObject* Sender)
    {
        TWinControl* parent = static_cast<TControl*>(Sender)->Parent;
        std::printf("PanelButtonClick: Parent is Panel1: %s\n", parent == Panel1 ? "yes" : "no");
        std::fflush(stdout);
    }

    // Edit1 / Memo1 / ComboBox1 で共有する。dynamic_cast で Sender の種類を判定する。
    void TextChange(TObject* Sender)
    {
        const char* kind = dynamic_cast<TMemo*>(Sender)      ? "Memo"
                         : dynamic_cast<TCustomEdit*>(Sender) ? "Edit"
                         : dynamic_cast<TComboBox*>(Sender)   ? "ComboBox"
                         : "?";
        std::string text = dynamic_cast<TComboBox*>(Sender) ? std::string(ComboBox1->Text)
                         : std::string(static_cast<TCustomEdit*>(Sender)->Text);
        std::printf("TextChange: %s Text=%s\n", kind, text.c_str());
        std::fflush(stdout);
    }

    void ListBox1Click(TObject* Sender)
    {
        std::printf("ListBox1Click: ItemIndex=%d\n", (int)static_cast<TListBox*>(Sender)->ItemIndex);
        std::fflush(stdout);
    }

    void Timer1Timer(TObject*)
    {
        ++ticks_;
        TickLabel->Caption = "Tick: " + std::to_string(ticks_);
        std::printf("Timer tick! count=%d\n", ticks_);
        std::fflush(stdout);
    }

    void PaintBox1Paint(TObject* Sender)
    {
        TCanvas& canvas = static_cast<TPaintBox*>(Sender)->Canvas;
        canvas.Pen.Color = clRed;
        canvas.Pen.Width = 2;
        canvas.Brush.Color = clYellow;
        canvas.Rectangle(10, 10, 110, 70);

        canvas.Pen.Color = clBlue;
        canvas.Brush.Color = clWhite;
        canvas.Ellipse(120, 10, 200, 70);

        canvas.Pen.Color = clBlack;
        canvas.MoveTo(10, 90);
        canvas.LineTo(200, 90);

        canvas.Font.Color = clGreen;
        canvas.Font.Size = 14;
        canvas.TextOut(10, 100, "Canvas drawing test");
    }
};

void TMainForm::OpenSubButtonClick(TObject*)
{
    // Owner を this にしているが、caFree / Release で先に破棄されても Owner 側から外れるだけで問題ない。
    TSubForm* sub = new TSubForm(this);
    sub->Show();
}

} // namespace

int main()
{
    Application->Initialize();
    Application->Title = "no_vcl test";
    Application->CreateForm(&Form1);

    std::printf("Title: %s\n", std::string(Application->Title).c_str());
    std::printf("Application->MainForm is Form1: %s\n", Application->MainForm == Form1 ? "yes" : "no");
    std::printf("MainForm caption: %s\n", std::string(Application->MainForm->Caption).c_str());
    std::printf("Button1->Parent->Caption: %s\n", std::string(Form1->Button1->Parent->Caption).c_str());

    // 2 つ目以降に生成したフォームは MainForm にならない。
    TForm* subForm = new TForm(Application);
    std::printf("MainForm after creating another form is still Form1: %s\n", Application->MainForm == Form1 ? "yes" : "no");
    subForm->Free();

    // ハンドラの解除(nullptr の代入)。解除したボタンを押しても何も起きない。
    TButton* disabledHandler = new TButton(Form1);
    disabledHandler->Parent = Form1;
    disabledHandler->Caption = "No handler";
    disabledHandler->Left = 20;
    disabledHandler->Top = 280;
    disabledHandler->OnClick = [](TObject*) { std::printf("must not be called\n"); };
    disabledHandler->OnClick = nullptr;

    // Application->Terminate() でメッセージループを抜ける(ウィンドウを閉じても抜ける)。
    TButton* quitButton = new TButton(Form1);
    quitButton->Parent = Form1;
    quitButton->Caption = "Quit";
    quitButton->Left = 20;
    quitButton->Top = 320;
    quitButton->OnClick = [](TObject*) { Application->Terminate(); };

    // Free() で個別に破棄すると、ラッパーも破棄される。
    TTracedLabel* tempLabel = new TTracedLabel(Form1);
    tempLabel->Parent = Form1;
    tempLabel->Free();
    std::printf("Destroyed labels after Free(): %d (expected 1)\n", g_destroyedLabels);

    std::printf("Running (click the buttons, then close the window three times: the first two closes are blocked, or press Quit)...\n");
    std::fflush(stdout);
    Application->Run();

    std::printf("Run returned. Terminated=%d\n", (bool)Application->Terminated);
    std::printf("OK (Form1 and its components are destroyed after main returns)\n");
    std::fflush(stdout);
    return 0;
}
