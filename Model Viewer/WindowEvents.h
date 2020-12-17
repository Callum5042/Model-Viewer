#pragma once

class WindowListener
{
public:
	WindowListener() = default;
	virtual ~WindowListener() = default;
	virtual void OnResize() = 0;
};