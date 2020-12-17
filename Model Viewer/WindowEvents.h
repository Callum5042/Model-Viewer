#pragma once

class WindowListener
{
public:
	WindowListener() = default;
	virtual ~WindowListener() = default;
	virtual void OnResize(int width, int height) = 0;
};