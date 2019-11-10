#ifndef _WinPostMan_h
#define _WinPostMan_h

#include "../Little_Multiplayer_Online/Client.h"

#define POST_OPT_ALL	'p'	

class WinContent
{
public:
	virtual void show() = 0;
	virtual void hide() = 0;
	virtual void display(const char* msg, int len) = 0;
	virtual void showTitle(const char* msg, int len) = 0;
};

class WinGameControl :public PostMan
{
public:
	WinGameControl(int capacity);
	~WinGameControl();

	// Í¨¹ý PostMan ¼Ì³Ð
	virtual void delivery(int id, const char* msg, int len) override;

	bool addContent(WinContent* p);

	void show();
	void hide();

	void showTitle(int id, const char* msg, int len);

	bool empty() { return size == 0; }

private:
	WinContent** contents;
	int size;
	int capacity;
};

#endif
