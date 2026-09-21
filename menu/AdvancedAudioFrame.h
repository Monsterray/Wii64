/**
 * Wii64 - AdvancedAudioFrame.h
 *
 * Wii64 homepage: http://www.emulatemii.com
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#ifndef ADVANCEDAUDIOFRAME_H
#define ADVANCEDAUDIOFRAME_H

#include "../libgui/Frame.h"

class AdvancedAudioFrame : public menu::Frame
{
public:
	AdvancedAudioFrame();
	~AdvancedAudioFrame();
	void activateSubmenu(int submenu);

	enum AdvancedAudioSubmenus
	{
		SUBMENU_REINIT=0
	};
};

#endif
