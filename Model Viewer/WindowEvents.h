#pragma once

class WindowListener
{
public:
	WindowListener();
	virtual ~WindowListener() = default;
	virtual void OnResize() = 0;
};