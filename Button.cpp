#include "OS.h"


const wchar* sayings[] = {
	L"You're a sadist aren't you.",
	L"No one likes you.",
	L"Is life hard if you are illiterate?",
	L"Were you dropped on your head as a child?"
};

unsigned times_clicked = 0;

EXPORT void MadButton_OnClick(OS::Window& button)
{
	if(times_clicked == 0)
	{
		button.setName(L"DO NOT CLICK");
		MessageBox(nullptr,L"Are you illiterate, perhaps?  Please don't click me again.",L"Disgruntled Button",MB_OK);
	}
	else
	{
		button.setName(sayings[rand() % 4]);

		if(times_clicked == 1)
		{
			MessageBox(nullptr,L"Congratulations.  Now you've gone and pissed me off.  From now on, I will just ignore you.",L"Mad Button",MB_OK);
		}
	}

	++times_clicked;
}

EXPORT void MadButton_OnCreate(OS::Window& button)
{
	button.setName(L"Do Not Click");
	button.setPosition(15,15);
	button.setDimensions(300,25);
}

EXPORT void MadButton_OnDestroy(OS::Window& button)
{
	if(times_clicked == 0)
	{
		MessageBox(nullptr,L"I appreciate the fact that you didn't click me.",L"Happy Button",MB_OK);
	}
	if(times_clicked == 1)
	{
		MessageBox(nullptr,L"Thank you for not clicking me again after I asked you to stop.  Now leave.",L"Disgruntled Button",MB_OK);
	}
	else
	{
		MessageBox(nullptr,L"Thank you for leaving and always remember, nobody loves you.",L"Mad Button",MB_OK);
	}
}