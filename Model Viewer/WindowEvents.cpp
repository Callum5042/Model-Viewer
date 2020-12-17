#include "Pch.h"
#include "WindowEvents.h"
#include "EventDispatcher.h"

WindowListener::WindowListener()
{
	EventDispatcher::Attach(this);
}
