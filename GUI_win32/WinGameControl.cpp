#include "WinGameControl.h"

WinGameControl::WinGameControl(int _capacity): contents(nullptr),size(0),capacity(_capacity)
{
	if (capacity < 0) capacity = 0;
	contents = new WinContent * [capacity];
	for (int i = 0; i < capacity; ++i)
		contents[i] = nullptr;
}

WinGameControl::~WinGameControl()
{
	for (int i = 0; i < size; ++i)
		delete contents[i];
	delete contents;
}

void WinGameControl::delivery(int id, const char* msg, int len)
{
	if (id < 0 || id > size) return;
	if (id == 0)		// 发给所有人的消息
	{
		char* buf = new char[len + 1];
		::memcpy(buf + 1, msg, sizeof(char) * len);
		buf[0] = POST_OPT_ALL;
		for(int i = 0; i < size; ++i)
			contents[i]->display(msg, len + 1);
		delete[] buf;
	}
	else
		contents[id - 1]->display(msg, len);
}

bool WinGameControl::addContent(WinContent* p)
{
	if (size == capacity) return false;
	contents[size++] = p;
	return true;
}

void WinGameControl::show()
{
	for (int i = 0; i < size; ++i)
		contents[i]->show();
}

void WinGameControl::hide()
{
	for (int i = 0; i < size; ++i)
		contents[i]->hide();
}

void WinGameControl::showTitle(int id, const char* msg, int len)
{
	if (id < 0 && id >= size) return;
	contents[id]->showTitle(msg, len);
}
