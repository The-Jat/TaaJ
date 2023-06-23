#include "app.h"
#include <StringView.h>

#include <Khidki.h>
//#include <Window.h>

///#include<syscalls.h>

//#include "Eigen/Dense"
//using Eigen::MatrixXd;

tst::tst(void)
	:	BApplication("application/x-vnd.dw-Myfirstapp")
{

	BRect frame(100,100,500,400);

	KWindow *myWindow=new KWindow(frame,"My first",B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE,B_ALL_WORKSPACES);

/*KWindow *mywindow = new KWindow(frame,
		"My first", B_BORDERED_WINDOW,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ZOOMABLE | B_NOT_CLOSABLE
			| B_NOT_MINIMIZABLE | B_NOT_MOVABLE | B_NOT_V_RESIZABLE
			| B_AVOID_FRONT | B_ASYNCHRONOUS_CONTROLS,
		B_ALL_WORKSPACES);*/

	//frame.Set(10,10,11,11);

//const char* info=test_api();//"jaat rocks again";
//char* info=(char*)_kern_testcall();//"jaat rocks again";
///int chk_syscall=_kern_testcall();
/*
MatrixXd m(2,2);
m(0,0)=3;
m(1,0)=2;
m(0,1)=7;
m(1,1)=9;
if (chk_syscall==7){
BStringView *label=new BStringView(frame,"mylabel","jaat keep rocking");
*/

/*fInfoTextView = new BTextView("");
	fInfoTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fInfoTextView->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	fInfoTextView->MakeEditable(false);
	fInfoTextView->MakeSelectable(false);
	fInfoTextView->MakeResizable(false);
	
	// Carefully designed to not exceed the 640x480 resolution with a 12pt font.
	float width = 640 * be_plain_font->Size() / 12 - (labelWidth + 64);
	float height = be_plain_font->Size() * 23;
	fInfoTextView->SetExplicitMinSize(BSize(width, height));
	fInfoTextView->SetExplicitMaxSize(BSize(width, B_SIZE_UNSET));
	*/

////label->ResizeToPreferred();
////mywindow->AddChild(label);
//}
/*if(m(1,1)==9){
BStringView *label1=new BStringView(frame,"mylabel1","eigen here!");
label1->ResizeToPreferred();
mywindow->AddChild(label1);
*/
//}
//mywindow->AddChild(fInfoTextView);
//fInfoTextView->SetText((char*)chk_syscall);
	myWindow->Show();
}

int main(void)
{ 
	tst *app=new tst();
	app->Run();
	delete app;
	return 0;
}
