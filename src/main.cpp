/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019, djcj <djcj@gmx.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Windows.h>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>

#include <string>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>

#include "keyCodes.h"
#include "images.h"
#include "lang.h"
#include "configuration.hpp"

#define MAX_DRIVE_LENGTH     _MAX_DRIVE * sizeof(wchar_t)
#define MAX_PATH_LENGTH      2048 * sizeof(wchar_t)
#define MAX_DIR_LENGTH       MAX_PATH_LENGTH
#define STRINGIFY(x)         #x
#define XSTRINGIFY(x)        STRINGIFY(x)
#define FLTK_VERSION_STRING  XSTRINGIFY(FL_MAJOR_VERSION) "." XSTRINGIFY(FL_MINOR_VERSION) "." XSTRINGIFY(FL_PATCH_VERSION)
#define LS                   12  /* default labelsize */

class MyChoice : public Fl_Choice
{
private:
	int _prev;
	Fl_Menu_Item *_menu;

	void prev(int v) { _prev = v; }
	int prev() { return _prev; }

public:
	MyChoice(int X, int Y, int W, int H, const char *L = NULL)
		: Fl_Choice(X, Y, W, H, L)
	{
		labelsize(LS);
		align(FL_ALIGN_TOP_LEFT);
		clear_visible_focus();
	}

	void menu(const Fl_Menu_Item *m);
	Fl_Menu_Item *menu() { return _menu; }

	int handle(int event);
};

class kbButton : public Fl_Button
{
private:
	uchar _dxkey;

public:
	kbButton(int X, int Y, int W, int H, const char *L = NULL)
		: Fl_Button(X, Y, W, H, L), _dxkey(0)
	{
		labelsize(LS);
		clear_visible_focus();
	}

	void dxkey(uchar n);
	uchar dxkey() { return _dxkey; }
};

class PadBox : public Fl_Box
{
private:
	const int _minW = 50;
	int measure_width(void);

public:
	PadBox(int X, int Y, int H, const char *L = NULL, Fl_Align align = FL_ALIGN_LEFT);
};

class MyWindow : public Fl_Double_Window
{
private:
	kbButton *_but;

public:
	MyWindow(int W, int H, const char *L = NULL)
		: Fl_Double_Window(W, H, L), _but(NULL)
	{}

	void but(kbButton *o) { _but = o; }
	kbButton *but() { return _but; }

	int handle(int event);
};


static const char *windowTitle = "SONIC THE HEDGEHOG 4 Episode I";
static void startWindow(bool restart);

static configuration *config;
static MyWindow *win;
static Fl_Group *g2_keyboard, *g2_gamepad;
static kbButton *btUp, *btDown, *btLeft, *btRight, *btA, *btB, *btX, *btY, *btStart;

#define IMAGE(x)  static Fl_PNG_Image x(NULL, x##_png, sizeof(x##_png))
IMAGE(arrow_01);
IMAGE(arrow_02);
IMAGE(arrow_03);
IMAGE(arrow_04);
IMAGE(back1);
IMAGE(back2);
IMAGE(back3);
IMAGE(button_01);
IMAGE(button_02);
IMAGE(button_03);
IMAGE(button_04);
IMAGE(button_05);
IMAGE(icon);
IMAGE(pad_controls_v02);
#undef IMAGE

static int rv = 0;
static int lang = 0;
static int save_x = 0, save_y = 0;

static wchar_t moduleRootDir[MAX_PATH_LENGTH];
static wchar_t confFile[MAX_PATH_LENGTH];

static const Fl_Menu_Item langItems[] =
{
	{ "English", 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 },
	{ "Deutsch", 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 },
	{ "Espa" "\xC3\xB1" "ol", 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 },  /* Español */
	{ "Fran" "\xC3\xA7" "ais", 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 },  /* Français */
	{ "Italiano", 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 },
	{ "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E", 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 },  /* Japanese */
	//{ "Polski", 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 },  /* not available as in-game language */
	{ NULL, 0,0,0,0,0,0,0,0 }
};


int MyWindow::handle(int event)
{
	kbButton *bt = but();

	if (event == FL_KEYDOWN && bt) {
		uchar dx = configuration::getDxKey(Fl::event_key());

		if (dx == 0) {
			/* restore label */
			bt->dxkey(bt->dxkey());
		} else {
			bt->dxkey(dx);
		}
		but(NULL);
		redraw();
	}
	return Fl_Double_Window::handle(event);
}

void kbButton::dxkey(uchar n)
{
	_dxkey = n;
	label(configuration::getNameDx(n));
}

PadBox::PadBox(int X, int Y, int H, const char *L, Fl_Align align)
	: Fl_Box(X, Y, 1, H, L)
{
	int W = measure_width();
	if (align == FL_ALIGN_RIGHT) {
		X = X - (W - _minW);
	}
	resize(X, Y, W, H);

	box(FL_BORDER_BOX);
	color(FL_WHITE);
	labelsize(LS);
}

int PadBox::measure_width(void)
{
	int w = 0;

	if (!label()) {
		return _minW;
	}

	Fl_Box *o = new Fl_Box(0, 0, 0, 0, label());
	fl_font(o->labelfont(), o->labelsize());
	w = static_cast<int>(fl_width(o->label()));

	if (w < _minW) {
		w = _minW;
	}

	delete o;
	return w;
}

void MyChoice::menu(const Fl_Menu_Item *m)
{
	/* make sure we have a local copy of the menu with write access */
	Fl_Choice::copy(m);
	_menu = const_cast<Fl_Menu_Item *>(Fl_Choice::menu());
}

int MyChoice::handle(int event)
{
	const Fl_Menu_Item *m;

	if (!menu() || !menu()->text) {
		return 0;
	}

	_menu[prev()].labelfont_ = labelfont();
	_menu[value()].labelfont_ = labelfont();
	prev(value());

	if (event == FL_PUSH) {
		/* highlight the currently selected value */
		_menu[value()].labelfont_ = labelfont() | FL_BOLD;

		if (Fl::visible_focus()) {
			Fl::focus(this);
		}

		if (Fl::scheme() || fl_contrast(textcolor(), FL_BACKGROUND2_COLOR) != textcolor()) {
			m = menu()->pulldown(x(), y(), w(), h(), NULL, this);
		} else {
			Fl_Color c = color();
			color(FL_BACKGROUND2_COLOR);
			m = menu()->pulldown(x(), y(), w(), h(), NULL, this);
			color(c);
		}

		if (!m) {
			return 1;
		}
		
		if (m != mvalue()) {
			redraw();
		}

		picked(m);
		return 1;
	}

	return Fl_Choice::handle(event);
}

static bool getModuleRootDir(void)
{
	wchar_t mod[MAX_PATH_LENGTH];
	wchar_t drv[MAX_DRIVE_LENGTH];
	wchar_t dir[MAX_DIR_LENGTH];

	if (!GetModuleFileNameW(NULL, mod, MAX_PATH_LENGTH)) {
		return false;
	}

	if (_wsplitpath_s(mod, drv, MAX_DRIVE_LENGTH, dir, MAX_DIR_LENGTH, NULL, 0, NULL, 0) != 0) {
		return false;
	}

	SecureZeroMemory(&moduleRootDir, MAX_DIR_LENGTH);
	wcscpy_s(moduleRootDir, MAX_DIR_LENGTH - 1, drv);
	wcscat_s(moduleRootDir, MAX_DIR_LENGTH - 1, dir);

	SecureZeroMemory(&confFile, MAX_PATH_LENGTH);
	wcscpy_s(confFile, MAX_PATH_LENGTH - 1, moduleRootDir);
	wcscat_s(confFile, MAX_PATH_LENGTH - 1, L"//main.conf");

	return true;
}

static int launchGame(void)
{
	const char *title = "Error: Sonic_vis.exe";
	wchar_t command[MAX_PATH_LENGTH];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	wcscpy_s(command, MAX_PATH_LENGTH - 1, moduleRootDir);
	wcscat_s(command, MAX_PATH_LENGTH - 1, L"//Sonic_vis.exe");

	SecureZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	SecureZeroMemory(&pi, sizeof(pi));

	if (CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) == FALSE) {
		MessageBoxA(0, "Failed calling CreateProcess()", title, MB_ICONERROR|MB_OK);
		return 1;
	}

	rv = WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	if (rv == WAIT_ABANDONED) {
		MessageBoxA(0, "Process abandoned.", title, MB_ICONERROR|MB_OK);
	} else if (rv == WAIT_TIMEOUT) {
		MessageBoxA(0, "Process time-out error.", title, MB_ICONERROR|MB_OK);
	} else if (rv == WAIT_FAILED) {
		MessageBoxA(0, "Process failed", title, MB_ICONERROR|MB_OK);
	}

	return rv;
}

static void setResolution_cb(Fl_Widget *o, void *)
{
	MyChoice *b = dynamic_cast<MyChoice *>(o);
	config->resN(b->value());
}

static void setDisplay_cb(Fl_Widget *o, void *)
{
	MyChoice *b = dynamic_cast<MyChoice *>(o);
	config->display(static_cast<uchar>(b->value()));
}

static void setLang_cb(Fl_Widget *o, void *)
{
	MyChoice *b = dynamic_cast<MyChoice *>(o);
	lang = b->value();
	config->language(static_cast<uchar>(lang));
	save_x = win->x();
	save_y = win->y();
	win->hide();
	startWindow(true);
}

static void fullscreen_cb(Fl_Widget *, void *)
{
	config->fullscreen(config->fullscreen() == 0 ? 1 : 0);
}

static void vibrate_cb(Fl_Widget *, void *)
{
	config->vibra(config->vibra() == 0 ? 1 : 0);
}

static void setController_cb(Fl_Widget *o, void *v)
{
	MyChoice *p = dynamic_cast<MyChoice *>(o);
	Fl_Box *b = reinterpret_cast<Fl_Box *>(v);
	int n = p->value();

	if (n == configuration::GAMEPAD_CTRLS) {
		config->controls(n);
		b->image(&back2);
		g2_keyboard->hide();
		g2_gamepad->show();
	} else {
		config->controls(configuration::KEYBOARD_CTRLS);
		b->image(&back3);
		g2_keyboard->show();
		g2_gamepad->hide();
	}

	win->redraw();
}

static void setDefaultKeys_cb(Fl_Widget *, void *)
{
	config->setDefaultKeys();

	/* "refresh" buttons */
	btUp->dxkey(config->keyUp());
	btDown->dxkey(config->keyDown());
	btLeft->dxkey(config->keyLeft());
	btRight->dxkey(config->keyRight());
	btA->dxkey(config->keyA());
	btB->dxkey(config->keyB());
	btX->dxkey(config->keyX());
	btY->dxkey(config->keyY());
	btStart->dxkey(config->keyStart());

	win->redraw();
}

static void setKey_cb(Fl_Widget *o, void *)
{
	kbButton *b = reinterpret_cast<kbButton *>(o);
	b->label(ui_Press[lang]);
	win->but(b);
	win->redraw();
}

static void bigButton_cb(Fl_Widget *, void *)
{
	/* save current key values in config */
	config->keyUp(btUp->dxkey());
	config->keyDown(btDown->dxkey());
	config->keyLeft(btLeft->dxkey());
	config->keyRight(btRight->dxkey());
	config->keyA(btA->dxkey());
	config->keyB(btB->dxkey());
	config->keyX(btX->dxkey());
	config->keyY(btY->dxkey());
	config->keyStart(btStart->dxkey());

	if (!config->saveConfig()) {
		MessageBoxA(0, "Failed to save configuration.", windowTitle, MB_ICONERROR|MB_OK);
	}
	win->hide();
	rv = launchGame();
}

static int esc_handler(int event)
{
	if (event == FL_SHORTCUT && Fl::event_key() == FL_Escape) {
		return 1; /* ignore Escape key */
	}
	return 0;
}

static void startWindow(bool restart)
{
	Fl_Tabs *tabs;
	Fl_Group *g1, *g2;
	Fl_Button *bigButton;
	std::string *devLabels;
	Fl_Menu_Item *devItems;
	char buf[128], bufJB[128], bufJS[128];

	int sc = config->screenCount();
	devLabels = new std::string[sc];
	devItems = new Fl_Menu_Item[sc + 1];

	if (!restart && !config->loadConfig()) {
		config->loadDefaultConfig();
	}
	lang = config->language();

	if (lang >= (sizeof(langItems) / sizeof(*langItems)) - 1) {
		lang = 0;  /* English */
	}

	Fl::add_handler(esc_handler);
	Fl::get_system_colors();

	/* The icon from the exe's resources isn't used by default. Weird. */
	Fl_Window::default_icon(&icon);

	win = new MyWindow(762, 656, windowTitle);
	{
		tabs = new Fl_Tabs(32, 16, 698, 532);
		{
			/* "Settings" */
			g1 = new Fl_Group(32, 36, 698, 512, ui_Settings[lang]);
			{
				/* Resolution list */
				const int szResItems = sizeof(configuration::resList) / sizeof(*configuration::resList);
				Fl_Menu_Item resItems[szResItems + 1];

				for (int i = 0; i < szResItems; ++i) {
					resItems[i] = { configuration::resList[i].l, 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 };
				}
				resItems[szResItems] = { NULL, 0,0,0,0,0,0,0,0 };

				/* Display list */
				for (int i = 0; i < sc; ++i) {
					_snprintf_s(buf, sizeof(buf) - 1, "Display %d", i);
					devLabels[i] = buf;
					devItems[i] = { devLabels[i].c_str(), 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 };
				}
				devItems[sc] = { NULL, 0,0,0,0,0,0,0,0 };

				/* Background image */
				{ Fl_Box *o = new Fl_Box(-1, 9, 1, 1);
				o->align(FL_ALIGN_BOTTOM_LEFT);
				o->image(&back1); }

				/* Display selection */
				{ MyChoice *o = new MyChoice(42, 64, 328, 24, ui_GraphicsDevice[lang]);
				o->menu(devItems);
				o->callback(setDisplay_cb); }

				/* Resolution */
				{ MyChoice *o = new MyChoice(42, 112, 328, 24, ui_Resolution[lang]);
				o->menu(resItems);
				o->value(config->resN());
				o->callback(setResolution_cb); }
				
				/* Fullscreen */
				{ Fl_Check_Button *o = new Fl_Check_Button(42, 150, 328, 24, ui_Fullscreen[lang]);
				o->labelsize(LS);
				o->value(config->fullscreen() == 0 ? 0 : 1);
				o->clear_visible_focus();
				o->callback(fullscreen_cb); }

				/* Language */
				{ MyChoice *o = new MyChoice(42, 228, 328, 24, ui_Language[lang]);
				o->menu(langItems);
				o->value(lang);
				o->callback(setLang_cb); }
			}
			g1->end();
			g1->labelsize(LS);

			_snprintf_s(buf, sizeof(buf) - 1, "%s %d", ui_Player[lang], 1);
			_snprintf_s(bufJB, sizeof(bufJB) - 1, "%s / %s", ui_Jump[lang], ui_Back[lang]);
			_snprintf_s(bufJS, sizeof(bufJS) - 1, "%s / %s", ui_Jump[lang], ui_Select[lang]);

			/* "Player 1" */
			g2 = new Fl_Group(32, 36, 698, 512);
			g2->copy_label(buf);
			{
				const Fl_Menu_Item conItems[] = {
					{ ui_Keyboard[lang], 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 },
					{ ui_Gamepad[lang], 0,0,0,0, FL_NORMAL_LABEL, 0, LS, 0 },
					{ NULL, 0,0,0,0,0,0,0,0 }
				};

				/* Background image */
				Fl_Box *bg = new Fl_Box(-1, 2, 1, 1);
				bg->align(FL_ALIGN_BOTTOM_LEFT);

				/* Keyboard bindings */
				g2_keyboard = new Fl_Group(32, 36, 698, 512);
				{
					/* Reset settings */
					{ Fl_Button *o = new Fl_Button(42, 102, 328, 24, ui_ResetToDefault[lang]);
					o->labelsize(LS);
					o->clear_visible_focus();
					o->callback(setDefaultKeys_cb); }

					/* "Movement" frame */
					{ Fl_Box *o = new Fl_Box(59, 192, 312, 277, ui_Movement[lang]);
					o->labelsize(LS);
					o->align(FL_ALIGN_TOP_LEFT);
					o->box(FL_ENGRAVED_FRAME); }
					
					/* Up */
					btUp = new kbButton(174, 241, 89, 38);
					btUp->dxkey(config->keyUp());
					btUp->callback(setKey_cb);
					{ Fl_Box *o = new Fl_Box(174, 203, 89, 38, ui_Up[lang]);
					o->labelsize(LS); }
					{ Fl_Box *o = new Fl_Box(216, 299, 1, 1);
					o->image(&arrow_04); }

					/* Left */
					btLeft = new kbButton(70, 311, 89, 38);
					btLeft->dxkey(config->keyLeft());
					btLeft->callback(setKey_cb);
					{ Fl_Box *o = new Fl_Box(70, 273, 89, 38, ui_Left[lang]);
					o->labelsize(LS); }
					{ Fl_Box *o = new Fl_Box(179, 330, 1, 1);
					o->image(&arrow_01); }

					/* Right */
					btRight = new kbButton(274, 311, 89, 38);
					btRight->dxkey(config->keyRight());
					btRight->callback(setKey_cb);
					{ Fl_Box *o = new Fl_Box(274, 273, 89, 38, ui_Right[lang]);
					o->labelsize(LS); }
					{ Fl_Box *o = new Fl_Box(254, 330, 1, 1);
					o->image(&arrow_02); }

					/* Down */
					btDown = new kbButton(174, 381, 89, 38);
					btDown->dxkey(config->keyDown());
					btDown->callback(setKey_cb);
					{ Fl_Box *o = new Fl_Box(174, 423, 89, 38, ui_Down[lang]);
					o->labelsize(LS); }
					{ Fl_Box *o = new Fl_Box(216, 365, 1, 1);
					o->image(&arrow_03); }

					/* "Action" frame */
					{ Fl_Box *o = new Fl_Box(407, 192, 294, 277, ui_Action[lang]);
					o->labelsize(LS);
					o->align(FL_ALIGN_TOP_LEFT);
					o->box(FL_ENGRAVED_FRAME); }

					/* Score Attack / Time Attack */
					btX = new kbButton(432, 243, 89, 38);
					btX->dxkey(config->keyX());
					btX->callback(setKey_cb);
					{ Fl_Box *o = new Fl_Box(452, 223, 1, 1, ui_ScoreAttack[lang]);
					o->labelsize(LS);
					o->align(FL_ALIGN_RIGHT); }
					{ Fl_Box *o = new Fl_Box(432, 223, 1, 1);
					o->image(&button_04); }
					
					/* Super Sonic */
					btY = new kbButton(432, 329, 89, 38);
					btY->dxkey(config->keyY());
					btY->callback(setKey_cb);
					{ Fl_Box *o = new Fl_Box(452, 305, 1, 1, ui_SuperSonic[lang]);
					o->labelsize(LS);
					o->align(FL_ALIGN_RIGHT); }
					{ Fl_Box *o = new Fl_Box(432, 305, 1, 1);
					o->image(&button_01); }

					/* Jump / Back */
					btB = new kbButton(432, 411, 89, 38);
					btB->dxkey(config->keyB());
					btB->callback(setKey_cb);
					{ Fl_Box *o = new Fl_Box(452, 387, 1, 1, bufJB);
					o->labelsize(LS);
					o->align(FL_ALIGN_RIGHT); }
					{ Fl_Box *o = new Fl_Box(432, 387, 1, 1);
					o->image(&button_02); }

					/* Start */
					btStart = new kbButton(590, 329, 89, 38);
					btStart->dxkey(config->keyStart());
					btStart->callback(setKey_cb);
					{ Fl_Box *o = new Fl_Box(590, 305, 1, 1, ui_Start[lang]);
					o->labelsize(LS);
					o->align(FL_ALIGN_RIGHT); }
					{ Fl_Box *o = new Fl_Box(570, 305, 1, 1);
					o->image(&button_05); }

					/* Jump / Select */
					btA = new kbButton(590, 411, 89, 38);
					btA->dxkey(config->keyA());
					btA->callback(setKey_cb);
					{ Fl_Box *o = new Fl_Box(590, 387, 1, 1, bufJS);
					o->labelsize(LS);
					o->align(FL_ALIGN_RIGHT); }
					{ Fl_Box *o = new Fl_Box(570, 387, 1, 1);
					o->image(&button_03); }
				}
				g2_keyboard->end();

				/* Gamepad bindings */
				g2_gamepad = new Fl_Group(32, 36, 698, 512);
				{
					/* Vibrate */
					{ Fl_Check_Button *o = new Fl_Check_Button(42, 102, 328, 24, ui_Vibrate[lang]);
					o->labelsize(LS);
					o->value(config->vibra() == 0 ? 0 : 1);
					o->clear_visible_focus();
					o->callback(vibrate_cb); }

					/* Gamepad overlay image */
					{ Fl_Box *o = new Fl_Box(368, 298, 1, 1);
					o->image(&pad_controls_v02); }

					new PadBox(144, 207, 18, ui_Back[lang], FL_ALIGN_RIGHT);
					new PadBox(144, 240, 18, ui_Up[lang], FL_ALIGN_RIGHT);
					new PadBox(144, 268, 18, ui_Right[lang], FL_ALIGN_RIGHT);
					new PadBox(144, 295, 18, ui_Left[lang], FL_ALIGN_RIGHT);
					new PadBox(144, 322, 18, ui_Down[lang], FL_ALIGN_RIGHT);
					new PadBox(542, 207, 18, ui_Start[lang]);
					new PadBox(542, 234, 18, ui_SuperSonic[lang]);
					new PadBox(542, 258, 18, ui_ScoreAttack[lang]);
					new PadBox(542, 302, 18, bufJB);
					new PadBox(542, 328, 18, bufJS);
				}
				g2_gamepad->end();

				/* Select keyboard/controller */
				{ MyChoice *o = new MyChoice(42, 64, 328, 24, ui_ControllerSelection[lang]);
				o->menu(conItems);
				o->value(config->controls());
				o->callback(setController_cb, reinterpret_cast<void *>(bg));
				/* Run callback once */
				setController_cb(o, reinterpret_cast<void *>(bg)); }
			}
			g2->end();
			g2->labelsize(LS);
		}
		tabs->end();
		tabs->clear_visible_focus();

		/* launch button */
		bigButton = new Fl_Button(62, 564, 642, 68, ui_SaveSettings[lang]);
		bigButton->labelsize(16);
		bigButton->clear_visible_focus();
		bigButton->callback(bigButton_cb);

		{ Fl_Box *o = new Fl_Box(762, 641, 1, 1, "using FLTK " FLTK_VERSION_STRING);
		o->align(FL_ALIGN_LEFT_TOP);
		o->labelsize(10);
		o->deactivate(); }
	}
	win->end();

	if (restart) {
		/* window restarted, restore old positions */
		win->position(save_x, save_y);
	} else {
		/* new window, position in center */
		win->position((Fl::w() - 762) / 2, (Fl::h() - 656) / 2);
	}
	win->show();

	Fl::run();

	delete[] devItems;
	delete[] devLabels;
}

int main(int argc, wchar_t *argv[])
{
	if (!getModuleRootDir()) {
		MessageBoxA(0, "Failed calling GetModuleFileName()", windowTitle, MB_ICONERROR|MB_OK);
		return 1;
	}

	config = new configuration(confFile);

	if (argc > 1 && wcscmp(argv[1], L"-QuickBoot")) {
		if (!config->loadConfig()) {
			config->loadDefaultConfig();
			config->saveConfig();
		}
		delete config;
		return launchGame();
	}

	startWindow(false);

	delete config;
	return rv;
}