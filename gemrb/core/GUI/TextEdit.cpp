/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */


#include "GUI/TextEdit.h"

#include "GameData.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

namespace GemRB {

TextEdit::TextEdit(const Region& frame, unsigned short maxLength, unsigned short px, unsigned short py)
	: Control(frame)
{
	ControlType = IE_GUI_EDIT;
	max = maxLength;
	FontPosX = px;
	FontPosY = py;
	Alignment = IE_FONT_ALIGN_MIDDLE | IE_FONT_ALIGN_LEFT;
	font = NULL;
	Cursor = NULL;
	Back = NULL;
	CurPos = 0;
	Text.reserve(max);
	ResetEventHandler( EditOnChange );
	ResetEventHandler( EditOnDone );
	ResetEventHandler( EditOnCancel );
	//Original engine values
	//Color white = {0xc8, 0xc8, 0xc8, 0x00}, black = {0x3c, 0x3c, 0x3c, 0x00};
	palette = core->CreatePalette( ColorWhite, ColorBlack );
}

TextEdit::~TextEdit(void)
{
	Video *video = core->GetVideoDriver();
	gamedata->FreePalette( palette );
	video->FreeSprite( Back );
	video->FreeSprite( Cursor );
}

void TextEdit::SetAlignment(unsigned char Alignment)
{
    this->Alignment = Alignment;
    MarkDirty();
}

/** Draws the Control on the Output Display */
void TextEdit::DrawInternal(Region& rgn)
{
	ieWord yOff = FontPosY;
	Video* video = core->GetVideoDriver();
	if (Back) {
		video->BlitSprite( Back, rgn.x, rgn.y, true );
		if (yOff) yOff += Back->Height;
	}
	if (!font)
		return;

	// FIXME: we should clip text to the background right?
	//The aligning of textedit fields is done by absolute positioning (FontPosX, FontPosY)
	if (hasFocus) {
		font->Print( Region( rgn.x + FontPosX, rgn.y + FontPosY, Width, Height ),
					Text, palette, Alignment, true );
		int w = font->CalcStringWidth(Text.substr(0, CurPos));
		ieWord vcenter = (rgn.h / 2) + (Cursor->Height / 2);
		if (w > rgn.w) {
			int rows = (w / rgn.w);
			vcenter += rows * font->maxHeight;
			w = w - (rgn.w * rows);
		}
		// FIXME: font is still stupid and forces IE_FONT_PADDING
		video->BlitSprite(Cursor, w + rgn.x + IE_FONT_PADDING + FontPosX,
						  FontPosY + vcenter + rgn.y, true);
	} else {
		font->Print( Region( rgn.x + FontPosX, rgn.y - yOff, rgn.w, rgn.h ), Text,
				palette, Alignment, true );
	}
}

/** Set Font */
void TextEdit::SetFont(Font* f)
{
	if (f != NULL) {
		font = f;
		MarkDirty();
		return;
	}
	Log(ERROR, "TextEdit", "Invalid font set!");
}

Font *TextEdit::GetFont() { return font; }

/** Set Cursor */
void TextEdit::SetCursor(Sprite2D* cur)
{
	core->GetVideoDriver()->FreeSprite( Cursor );
	if (cur != NULL) {
		Cursor = cur;
	}
	MarkDirty();
}

/** Set BackGround */
void TextEdit::SetBackGround(Sprite2D* back)
{
	//if 'back' is NULL then no BackGround will be drawn
	if (Back)
		core->GetVideoDriver()->FreeSprite(Back);
	Back = back;
	MarkDirty();
}

/** Key Press Event */
bool TextEdit::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	if (Key >= 0x20) {
		if (Value && ( (Key<'0') || (Key>'9') ) )
			return false;
		MarkDirty();
		if (Text.length() < max) {
			Text.insert(CurPos++, 1, Key);
		}
		RunEventHandler( EditOnChange );
		return true;
	}
	return false;
}
/** Special Key Press */
bool TextEdit::OnSpecialKeyPress(unsigned char Key)
{
	MarkDirty();
	switch (Key) {
		case GEM_HOME:
			CurPos = 0;
			break;
		case GEM_END:
			CurPos = Text.length();
			break;
		case GEM_LEFT:
			if (CurPos > 0)
				CurPos--;
			break;
		case GEM_RIGHT:
			if (CurPos < Text.length()) {
				CurPos++;
			}
			break;
		case GEM_DELETE:
			if (CurPos < Text.length()) {
				Text.erase(CurPos, 1);
			}
			break;		
		case GEM_BACKSP:
			if (CurPos != 0) {
				Text.erase(--CurPos, 1);
			}
			break;
		case GEM_RETURN:
			RunEventHandler( EditOnDone );
	}
	RunEventHandler( EditOnChange );
	return true;
}

void TextEdit::SetFocus(bool focus)
{
	Control::SetFocus(focus);
	if (hasFocus) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

/** Sets the Text of the current control */
void TextEdit::SetText(const char* string)
{
	if (string) {
		String* text = StringFromCString(string);
		Text = *text;
		delete text;
	}
	if (Text.length() > max) CurPos = max + 1;
	else CurPos = Text.length();
	MarkDirty();
}

void TextEdit::SetBufferLength(ieWord buflen)
{
	if(buflen<1) return;
	if(buflen!=max) {
		Text.resize(buflen);
		max = buflen;
	}
}

/** Simply returns the pointer to the text, don't modify it! */
const String& TextEdit::QueryText() const
{
	return Text;
}

bool TextEdit::SetEvent(int eventType, EventHandler handler)
{
	switch (eventType) {
	case IE_GUI_EDIT_ON_CHANGE:
		EditOnChange = handler;
		break;
	case IE_GUI_EDIT_ON_DONE:
		EditOnDone = handler;
		break;
	case IE_GUI_EDIT_ON_CANCEL:
		EditOnCancel = handler;
		break;
	default:
		return false;
	}

	return true;
}

}
