#pragma once

/*
DlgBuilder.hpp

������������ ��������������� ��������
*/
/*
Copyright (c) 2009 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

struct DialogItemEx;
class string;

class DialogBuilder
{
	private:
		const wchar_t *HelpTopic;
		DialogItemEx *DialogItems;
		int DialogItemsCount;
		int DialogItemsAllocated;
		int OKButtonID;
		int NextY;

		DialogItemEx *AddDialogItem(int Type, const string &strData);
		void ReallocDialogItems();
		void UpdateBorderSize();
		int MaxTextWidth();
		void SaveValue(DialogItemEx *Item, int RadioGroupIndex);
		void SetNextY(DialogItemEx *Item);
		void LinkFlagsByID(DialogItemEx *Parent, int TargetID, FarDialogItemFlags Flags);

	public:
		DialogBuilder(int TitleMessageId, const wchar_t *HelpTopic);
		~DialogBuilder();

		// ��������� �������.
		DialogItemEx *AddCheckbox(int TextMessageId, BOOL *Value);

		// ��������� ������ �����������.
		void AddRadioButtons(int *Value, int OptionCount, int MessageIDs[]);

		// ��������� ����������� �����, ������������� �� ��������� ������ � �������.
		DialogItemEx *AddText(int LabelId);

		// ��������� ���� ���� DI_EDIT ��� �������������� ���������� ���������� ��������.
		DialogItemEx *AddEditField(string *Value, int Width, const wchar_t *HistoryID = NULL);

		// ��������� ���� ���� DI_FIXEDIT ��� �������������� ���������� ��������� ��������.
		DialogItemEx *AddIntEditField(int *Value, int Width);

		// ��������� ��������� ��������� ������ ����� �� �������� RelativeTo.
		DialogItemEx *AddTextBefore(DialogItemEx *RelativeTo, int LabelId);

		// ��������� ��������� ��������� ������ ������ �� �������� RelativeTo.
		DialogItemEx *AddTextAfter(DialogItemEx *RelativeTo, int LabelId);

		// ��������� ��������� ��������� Parent � Target. ����� Parent->Selected �����
		// false, ������������� ����� Flags � �������� Target; ����� ����� true -
		// ���������� �����.
		// ���� LinkLabels ����������� � true, �� ��������� ��������, ����������� � �������� Target
		// �������� AddTextBefore � AddTextAfter, ����� ����������� � ��������� Parent.
		void LinkFlags(DialogItemEx *Parent, DialogItemEx *Target, FarDialogItemFlags Flags, bool LinkLabels=true);

		// ��������� ���������.
		void AddSeparator();

		// ��������� ���������, ������ OK � Cancel.
		void AddOKCancel();
		bool ShowDialog();
};
