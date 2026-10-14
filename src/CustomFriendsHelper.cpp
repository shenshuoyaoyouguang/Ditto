#include "stdafx.h"
#include "CustomFriendsHelper.h"
#include "CP_Main.h"
#include "..\Shared\Tokenizer.h"
#include "..\Shared\TextConvert.h"
#include <tinyxml2.h>
#include "XmlFileLoad.h"


CCustomFriendsHelper::CCustomFriendsHelper()
{
}


CCustomFriendsHelper::~CCustomFriendsHelper()
{
}

void CCustomFriendsHelper::Load()
{
	m_list.clear();

	CString oldValues = CGetSetOptions::GetCustomSendToList();

	tinyxml2::XMLDocument doc;
	CStringA xmlA = CTextConvert::UnicodeToUTF8(oldValues);
	doc.Parse(xmlA);

	tinyxml2::XMLElement *ItemHeader = doc.FirstChildElement("CustomFriends");

	if (ItemHeader != NULL)
	{
		tinyxml2::XMLElement *ItemElement = ItemHeader->FirstChildElement();

		while (ItemElement)
		{
			Name_Desc array_item;
			array_item.m_name = AttrOrNull(ItemElement, "name");
			array_item.m_desc = AttrOrNull(ItemElement, "desc");
			
			m_list.push_back(array_item);

			ItemElement = ItemElement->NextSiblingElement();
		}
	}
}

void CCustomFriendsHelper::Save()
{
	/*CString values = _T("");

	int count = m_list.size();
	for (int i = 0; i < count; i++)
	{
		CString lineValue = m_list[i];
		values += _T(",");
		values += lineValue;
	}*/

	tinyxml2::XMLDocument doc;

	tinyxml2::XMLElement* friendOuter = doc.NewElement("CustomFriends");
	doc.InsertEndChild(friendOuter);

	for (auto & listItem : m_list)
	{
		tinyxml2::XMLElement* friendElement = doc.NewElement("Friend");

		CStringA nameA = CTextConvert::UnicodeToUTF8(listItem.m_name);
		friendElement->SetAttribute("name",  nameA);

		CStringA descA = CTextConvert::UnicodeToUTF8(listItem.m_desc);
		friendElement->SetAttribute("desc", descA);

		friendOuter->InsertEndChild(friendElement);
	}

	tinyxml2::XMLPrinter printer(nullptr, true);
	doc.Accept(&printer);
	CString cs = printer.CStr();

	CGetSetOptions::SetCustomSendToList(cs);
}

void CCustomFriendsHelper::AddToMenu(CMenu *pMenu)
{
	bool addedItem = false;
	int id = 0;
	for (auto & element : m_list) 
	{
		if (addedItem == false)
		{
			addedItem = true;
		}

		CString cs;
		if (element.m_desc != _T(""))
		{
			cs.Format(_T("(%s) - %s"), element.m_name, element.m_desc);
		}
		else
		{
			cs.Format(_T("%s"), element.m_name);
		}

		pMenu->AppendMenuW(MF_STRING, (CustomFriendStartId + id), cs);
		id++;
	}

	if (addedItem)
	{
		pMenu->AppendMenu(MF_SEPARATOR);
	}
	
	pMenu->AppendMenuW(MF_STRING, (CustomFriendStartId + PromptForCustom), theApp.m_Language.GetString("prompt_for_name", "Prompt For Name"));
}

void CCustomFriendsHelper::Add(CString item, CString desc)
{
	int count = (int)m_list.size();
	if (count < MaxCustomFriends)
	{
		Name_Desc array_item;
		array_item.m_name = item;
		array_item.m_desc = desc;
		m_list.push_back(array_item);
		Save();
	}
}

CString CCustomFriendsHelper::GetSendTo(int id, bool &showDlg)
{
	int index = id - CustomFriendStartId;
	if (index >= 0 && index < m_list.size())
	{
		return m_list[index].m_name;
	}

	if (index == PromptForCustom)
	{
		showDlg = true;
	}

	return _T("");
}

void CCustomFriendsHelper::ClearList()
{
	m_list.clear();
	this->Save();
}