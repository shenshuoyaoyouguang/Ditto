// Post-migration port of the TinyXml1 (2.6.2) freeze to tinyxml2 11.0.0
// (vcpkg manifest port). MFC/CString-free on purpose: only tinyxml2 plus
// the MFC-free src/XmlFileLoad.h shim plus CRT/Win32/plain-C++17, so this
// compiles outside the main project (same convention as
// src/ThemeColorMath.h).
//
// What it freezes (mirroring exactly what the app reads, no more):
//   - Language files (src/MultiLanguage.cpp:359-414): LoadUtf8FileForXml +
//     Parse, ErrorStr/ErrorLineNum, root attrs Version/Author/Notes/
//     LanguageCode, per-section <Item> counts (all items AND items with
//     FirstChild text, because LoadSection only loads the latter),
//     first/last English_Text + ID over all items and over text-bearing
//     items.
//   - Theme files (src/Theme.cpp:135-241): LoadUtf8FileForXml + Parse,
//     ErrorStr, root attrs, presence + FirstChild raw text of every legacy
//     colour/int node in load order, presence + raw text of every v4 token
//     node in LoadTokensFromXml order, Accent_Default/Font_Family raw text.
//   - The 3 writer idioms (ChaiScriptXml.cpp:74-111,
//     CustomFriendsHelper.cpp:60-83, QPasteWnd.cpp SavePastSearchesFrom):
//     doc.NewElement / InsertEndChild / SetAttribute / XMLPrinter(nullptr,
//     true) + Accept + CStr, on fixed inputs (CJK bytes, entity chars
//     &<>"', empty list). Byte-exact printer output (length + escaped
//     bytes) plus a Parse-back readback using the same Attribute() calls
//     the loaders use -- including the "vesion" typo read in
//     ChaiScriptXml.cpp:59, recorded as-is (NULL), NOT fixed.
//   - Writers persist strings to options/registry, never files; no XML
//     declaration is ever emitted (also frozen by the goldens).
//
// Build & run (local, no MFC needed; tinyxml2.dll must be next to the exe
// or on PATH at runtime):
//   cl /nologo /std:c++17 /W4 /EHsc /I src /I vcpkg_installed\x64-windows\include tests\test_xml_parity.cpp /link /LIBPATH:vcpkg_installed\x64-windows\lib tinyxml2.lib /Fe:test_xml_parity.exe
//   .\test_xml_parity.exe Debug\Language Debug\Themes <fresh-out-dir>
// Delete the exe/obj afterwards to keep the repo root clean.
//
// Golden layout under <fresh-out-dir> (written with \n, binary mode):
//   lang/<FileName>.golden.txt    one per Debug/Language/*.xml (30)
//   themes/<FileName>.golden.txt  one per Debug/Themes/*.xml (14)
//   writers.golden.txt            byte-exact printer output of the writers
//
// Exit code is 0 on PASS (structural round-trip checks hold), 1 on FAIL.
//
// Porting notes vs the TinyXml1 TU (output sections, escaping and ordering
// are unchanged; only the library idiom changed):
//   - Load: wchar LoadFile -> LoadUtf8FileForXml (the exact app shim from
//     src/XmlFileLoad.h, included here) + doc.Parse(bytes.c_str()), exactly
//     as MultiLanguage.cpp:359-369 and Theme.cpp:135-145 do.
//   - ErrorDesc/ErrorRow/ErrorCol -> ErrorStr()/ErrorLineNum(); tinyxml2
//     has no column API, so the lang FAIL line reports col=0 (no corpus
//     file takes this branch; every frozen file is load=OK).
//   - Attribute(name, &int) -> QueryIntAttribute (keeps the default on
//     missing/illegal, verified by probe); presence for the
//     "active_attr=" line is probed via Attribute("active") -- the one
//     harness-vs-app idiom difference (the app uses bare
//     QueryIntAttribute; the probe exists only to preserve the golden
//     line and emits no extra output).
//   - new TiXmlElement + LinkEndChild -> doc.NewElement + InsertEndChild.
//   - TiXmlPrinter + SetLineBreak("") -> XMLPrinter(nullptr, true), the
//     exact app writer idiom (ChaiScriptXml.cpp:106).
//   - Version line now records tinyxml2=11.0.0 (same line shape).
//   - Known printer-byte divergences vs the TinyXml1 goldens (explained,
//     not fixed -- fixing them would deviate from the app idiom): the
//     compact printer emits no inter-tag indent (TiXml1 kept 4 spaces with
//     SetLineBreak("")), always double-quotes attributes (TiXml1 switched
//     to '...' when the value held a '"'), and prints empty elements as
//     <X/> (TiXml1 printed <X />). Parsed-back values are unaffected.

// The real app shim, not a copy: the harness exercises the same
// LoadUtf8FileForXml + AttrOrNull the migrated loaders use.
#include "XmlFileLoad.h"
#include <tinyxml2.h>

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static int g_failures = 0;

#define CHECK_TRUE(cond) do { \
	if (!(cond)) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s\n", __FILE__, (long)__LINE__, #cond); \
	} \
} while (0)

// ---------------------------------------------------------------- io ----

// Single-line deterministic escaping for golden text: backslash, LF, CR
// and non-printables become \xNN-proof ASCII; everything else (incl. UTF-8
// bytes) passes through untouched.
static std::string Escape(const char* s)
{
	static const char* kHex = "0123456789ABCDEF";
	std::string out;
	if (s == NULL)
		return "(null)";
	for (const unsigned char* p = (const unsigned char*)s; *p; ++p)
	{
		if (*p == '\\') { out += "\\\\"; }
		else if (*p == '\n') { out += "\\n"; }
		else if (*p == '\r') { out += "\\r"; }
		else if (*p < 0x20 || *p == 0x7F)
		{
			out += "\\x";
			out += kHex[*p >> 4];
			out += kHex[*p & 0xF];
		}
		else { out += (char)*p; }
	}
	return out;
}

static void EmitAttrLine(std::string& out, const tinyxml2::XMLElement* e, const char* name)
{
	const char* v = e->Attribute(name);
	char len[32];
	if (v == NULL)
		out += std::string("  attr ") + name + "=(null)\n";
	else
	{
		std::snprintf(len, sizeof(len), "%u", (unsigned)std::strlen(v));
		out += std::string("  attr ") + name + " len=" + len + " text=" + Escape(v) + "\n";
	}
}

// Raw FirstChild()->Value() bytes as the app reads them
// (Theme.cpp:290-298, MultiLanguage.cpp:456/503), before any trimming.
static void EmitNodeTextLine(std::string& out, const char* label, tinyxml2::XMLNode* child)
{
	char len[32];
	if (child == NULL || child->Value() == NULL)
	{
		out += std::string(label) + " present=0\n";
		return;
	}
	std::snprintf(len, sizeof(len), "%u", (unsigned)std::strlen(child->Value()));
	out += std::string(label) + " present=1 len=" + len + " text=" + Escape(child->Value()) + "\n";
}

static bool WriteFile(const fs::path& p, const std::string& bytes)
{
	FILE* f = NULL;
#if defined(_MSC_VER)
	if (_wfopen_s(&f, p.c_str(), L"wb") != 0)
		f = NULL;
#else
	f = std::fopen(p.string().c_str(), "wb");
#endif
	if (f == NULL)
		return false;
	bool ok = std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
	std::fclose(f);
	return ok;
}

static bool ToWide(const std::string& utf8, std::wstring& wide)
{
	if (utf8.empty()) { wide.clear(); return true; }
	int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
	if (n <= 0)
		return false;
	std::vector<wchar_t> buf((size_t)n);
	if (MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, buf.data(), n) <= 0)
		return false;
	wide.assign(buf.data());
	return true;
}

static std::vector<fs::path> SortedXml(const fs::path& dir)
{
	std::vector<fs::path> files;
	for (const auto& e : fs::directory_iterator(dir))
	{
		if (e.is_regular_file() && e.path().extension() == ".xml")
			files.push_back(e.path());
	}
	std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
		return a.filename().string() < b.filename().string();
	});
	return files;
}

// Loads + parses exactly like the migrated loaders (MultiLanguage.cpp:
// 359-369, Theme.cpp:135-145). Returns true on success; on failure fills
// desc/row for the golden FAIL line.
static bool LoadForFreeze(const std::wstring& w, tinyxml2::XMLDocument& doc,
	std::string& desc, int& row)
{
	std::string bytes;
	std::string err;
	bool loaded = LoadUtf8FileForXml(w.c_str(), bytes, err);
	if (loaded && doc.Parse(bytes.c_str()) != tinyxml2::XML_SUCCESS)
	{
		loaded = false;
		err = doc.ErrorStr() != nullptr ? doc.ErrorStr() : "";
		row = (int)doc.ErrorLineNum();
	}
	if (!loaded)
	{
		desc = err;
		return false;
	}
	desc.clear();
	row = 0;
	return true;
}

// ------------------------------------------------------- language ----

// Exact section order in CMultiLanguage::Load (MultiLanguage.cpp:395-414).
static const char* const kLangSections[] = {
	"Ditto_Right_Click_Menu",
	"Ditto_Groups_Right_Click_Menu",
	"Ditto_Options_General",
	"Ditto_Clip_Properties",
	"Ditto_Options_Supported_Types",
	"Ditto_Options_Shortcuts",
	"Ditto_Options_Quick_Paste",
	"Ditto_Options_Quick_Paste_Keyboard",
	"Ditto_Options_Friends",
	"Ditto_Options_Friends_Detail",
	"Ditto_Options_Stats",
	"Ditto_Options_Supported_Types_Add",
	"Ditto_Move_To_Groups",
	"Ditto_Options_Sheet",
	"Ditto_Tray_Icon_Menu",
	"Ditto_Options_CopyBuffers",
	"Ditto_GlobalHotKeys",
	"Ditto_DeleteClipData",
	"Ditto_String_Table",
};

static std::string FreezeLanguageFile(const fs::path& p)
{
	std::string out;
	out += "file=" + p.filename().string() + "\n";

	std::wstring w;
	CHECK_TRUE(ToWide(p.string(), w));
	tinyxml2::XMLDocument doc;
	// Same LoadUtf8FileForXml + Parse idiom as MultiLanguage.cpp:359-369.
	std::string desc;
	int row = 0;
	if (!LoadForFreeze(w, doc, desc, row))
	{
		char rc[64];
		std::snprintf(rc, sizeof(rc), "%d", row);
		// tinyxml2 exposes no column API; 0 pins the field shape.
		out += std::string("load=FAIL desc=") + Escape(desc.c_str())
			+ " row=" + rc + " col=0\n";
		return out;
	}
	out += "load=OK\n";

	// Same root lookup as MultiLanguage.cpp:377.
	tinyxml2::XMLElement* root = doc.FirstChildElement("Ditto_Language_File");
	if (root == NULL)
	{
		out += "root=(missing Ditto_Language_File)\n";
		return out;
	}
	out += "root=Ditto_Language_File\n";
	EmitAttrLine(out, root, "Version");
	EmitAttrLine(out, root, "Author");
	EmitAttrLine(out, root, "Notes");
	EmitAttrLine(out, root, "LanguageCode");

	for (const char* section : kLangSections)
	{
		tinyxml2::XMLNode* node = root->FirstChildElement(section);
		if (node == NULL)
		{
			out += std::string("section ") + section + " present=0\n";
			continue;
		}
		// Same traversal as LoadSection/LoadStringTableSection:
		// FirstChildElement + NextSiblingElement over <Item> elements.
		int countAll = 0, countWithText = 0;
		const tinyxml2::XMLElement* first = NULL;
		const tinyxml2::XMLElement* last = NULL;
		const tinyxml2::XMLElement* firstText = NULL;
		const tinyxml2::XMLElement* lastText = NULL;
		for (tinyxml2::XMLElement* item = node->FirstChildElement(); item; item = item->NextSiblingElement())
		{
			if (first == NULL) first = item;
			last = item;
			countAll++;
			if (item->FirstChild() != NULL) // LoadSection skips text-less items
			{
				if (firstText == NULL) firstText = item;
				lastText = item;
				countWithText++;
			}
		}
		char nb[64];
		std::snprintf(nb, sizeof(nb), "%d %d", countAll, countWithText);
		out += std::string("section ") + section + " present=1 items_all=" + nb + "\n";
		if (first)
		{
			out += "  first";
			const char* t = first->Attribute("English_Text");
			const char* id = first->Attribute("ID");
			out += std::string(" English_Text=") + Escape(t) + " ID=" + Escape(id) + "\n";
		}
		if (last && last != first)
		{
			out += "  last";
			const char* t = last->Attribute("English_Text");
			const char* id = last->Attribute("ID");
			out += std::string(" English_Text=") + Escape(t) + " ID=" + Escape(id) + "\n";
		}
		if (firstText)
		{
			out += "  first_with_text";
			const char* t = firstText->Attribute("English_Text");
			const char* id = firstText->Attribute("ID");
			const tinyxml2::XMLNode* f = firstText->FirstChild();
			out += std::string(" English_Text=") + Escape(t) + " ID=" + Escape(id)
				+ " foreign=" + Escape(f ? f->Value() : NULL) + "\n";
		}
		if (lastText && lastText != firstText)
		{
			out += "  last_with_text";
			const char* t = lastText->Attribute("English_Text");
			const char* id = lastText->Attribute("ID");
			const tinyxml2::XMLNode* f = lastText->FirstChild();
			out += std::string(" English_Text=") + Escape(t) + " ID=" + Escape(id)
				+ " foreign=" + Escape(f ? f->Value() : NULL) + "\n";
		}
	}
	return out;
}

// ---------------------------------------------------------- theme ----

// Legacy colour/int nodes in CTheme::Load order (Theme.cpp:197-239).
static const char* const kLegacyNodes[] = {
	"CaptionLeft", "CaptionRight", "CaptionLeftTopMost", "CaptionRightTopMost",
	"CaptionLeftNotConnected", "CaptionRightNotConnected", "CaptionTextColor",
	"ListBoxOddRowsBG", "ListBoxEvenRowsBG", "ListBoxOddRowsText",
	"ListBoxEvenRowsText", "ListBoxSelectedBG", "ListBoxSelectedNoFocusBG",
	"ListBoxSelectedText", "ListBoxSelectedNoFocusText",
	"ClipPastedColor", "MainWindowBG", "ListSmallQuickPasteIndexColor",
	"SearchTextBoxFocusBG", "SearchTextBoxFocusText", "SearchTextBoxFocusBorder",
	"SearchTextHighlight",
	"Border", "BorderTopMost", "BorderNotConnected",
	"GroupTreeBG", "GroupTreeText",
	"CaptionSize", "CaptionFontSize",
	"DescriptionWindowBG", "DescriptionWindowText",
	"ScrollBarThumb", "ScrollBarThumbHover", "ScrollBarTrack",
};

// v4 token nodes in CTheme::LoadTokensFromXml order,
// Accent_Default and Font_Family read via FirstChild()->Value() raw text.
static const char* const kTokenNodes[] = {
	"Surface_Base", "Surface_Elevated", "Surface_RowAlt",
	"Text_Primary", "Text_Secondary", "Text_Disabled",
	"Stroke_Card", "Stroke_Divider",
	"Control_Fill", "Control_Hover", "Control_Pressed", "Control_DisabledBG",
	"State_Hover", "State_SelectedBG", "State_SelectedText",
	"Text_OnAccent", "Accent_Subtle", "Accent_Text",
	"Indicator_Badge",
	"Accent_Default",
	"Radius_Control", "RowHeight_Compact", "RowHeight_Comfortable",
	"Font_Family",
};

static std::string FreezeThemeFile(const fs::path& p)
{
	std::string out;
	out += "file=" + p.filename().string() + "\n";

	std::wstring w;
	CHECK_TRUE(ToWide(p.string(), w));
	tinyxml2::XMLDocument doc;
	// Same LoadUtf8FileForXml + Parse idiom as Theme.cpp:135-145.
	std::string desc;
	int row = 0;
	(void)row;
	if (!LoadForFreeze(w, doc, desc, row))
	{
		out += std::string("load=FAIL desc=") + Escape(desc.c_str()) + "\n";
		return out;
	}
	out += "load=OK\n";

	// Same root lookup as Theme.cpp:170.
	tinyxml2::XMLElement* root = doc.FirstChildElement("Ditto_Theme_File");
	if (root == NULL)
	{
		out += "root=(missing Ditto_Theme_File)\n";
		return out;
	}
	out += "root=Ditto_Theme_File\n";
	EmitAttrLine(out, root, "Version");
	EmitAttrLine(out, root, "Author");
	EmitAttrLine(out, root, "Notes");

	// Same per-node read as LoadElement (Theme.cpp:280-298):
	// FirstChildElement(node) then FirstChild()->Value().
	for (const char* name : kLegacyNodes)
	{
		tinyxml2::XMLElement* el = root->FirstChildElement(name);
		if (el == NULL)
			out += std::string("legacy ") + name + " present=0\n";
		else
			EmitNodeTextLine(out, (std::string("legacy ") + name).c_str(), el->FirstChild());
	}
	for (const char* name : kTokenNodes)
	{
		tinyxml2::XMLElement* el = root->FirstChildElement(name);
		if (el == NULL)
			out += std::string("token ") + name + " present=0\n";
		else
			EmitNodeTextLine(out, (std::string("token ") + name).c_str(), el->FirstChild());
	}
	return out;
}

// --------------------------------------------------------- writers ----

// Fixed inputs. Non-ASCII is hex-escaped UTF-8 so the source encoding can
// never change the bytes: E5 89 AA E8 B4 B4 = U+526A U+8D34,
// E6 9D BF = U+677F, E4 B8 AD E6 96 87 = U+4E2D U+6587.
static const char kCjkA[] = "\xE5\x89\xAA\xE8\xB4\xB4 CJK \xE4\xB8\xAD\xE6\x96\x87";
static const char kCjkB[] = "\xE6\x9D\xBF test \xE5\x89\xAA\xE8\xB4\xB4\xE6\x9D\xBF";
static const char kEntities[] = "Tom & Jerry <tag> \"quoted\" 'apos'";

static void EmitPrinterBytes(std::string& out, const char* label, tinyxml2::XMLDocument& doc)
{
	// Exact app writer idiom: XMLPrinter(nullptr, true) + Accept + CStr
	// (ChaiScriptXml.cpp:106, CustomFriendsHelper.cpp:78, QPasteWnd.cpp).
	tinyxml2::XMLPrinter printer(nullptr, true);
	doc.Accept(&printer);
	const char* s = printer.CStr();
	char len[32];
	std::snprintf(len, sizeof(len), "%u", (unsigned)(s ? std::strlen(s) : 0));
	out += std::string(label) + " len=" + (s ? len : "0") + " bytes=" + Escape(s) + "\n";
}

// Rollback-compat probe, invisible on success (no output lines): a byte
// string in the OLD TinyXml1 printer shape (single-quote delimiters,
// 4-space indents, "<ChaiScripts />" empty form) must still parse under
// tinyxml2 with identical values, and vice versa the new compact bytes
// must carry identical values. Fires CHECK_TRUE on mismatch.
static void CheckWireCompat()
{
	// Old-producer bytes -> new parser.
	{
		tinyxml2::XMLDocument back;
		CHECK_TRUE(back.Parse(
			"<ChaiScripts>    "
			"<ChaiScriptItem active=\"1\" name='Tom &amp; Jerry' version=\"1.2&amp;3\" />"
			"</ChaiScripts>") == tinyxml2::XML_SUCCESS);
		tinyxml2::XMLElement* header = back.FirstChildElement("ChaiScripts");
		CHECK_TRUE(header != NULL);
		tinyxml2::XMLElement* item = header ? header->FirstChildElement() : NULL;
		CHECK_TRUE(item != NULL);
		if (item)
		{
			int active = -1;
			CHECK_TRUE(item->QueryIntAttribute("active", &active) == tinyxml2::XML_SUCCESS);
			CHECK_TRUE(active == 1);
			const char* nm = item->Attribute("name");
			CHECK_TRUE(nm && std::strcmp(nm, "Tom & Jerry") == 0);
			const char* ver = item->Attribute("version");
			CHECK_TRUE(ver && std::strcmp(ver, "1.2&3") == 0);
			CHECK_TRUE(item->Attribute("vesion") == NULL); // typo read stays NULL
		}
	}
	// Empty-table old form "<ChaiScripts />" parses to zero items.
	{
		tinyxml2::XMLDocument back;
		CHECK_TRUE(back.Parse("<ChaiScripts />") == tinyxml2::XML_SUCCESS);
		tinyxml2::XMLElement* header = back.FirstChildElement("ChaiScripts");
		CHECK_TRUE(header != NULL);
		tinyxml2::XMLElement* first = header ? header->FirstChildElement() : NULL;
		CHECK_TRUE(first == NULL);
	}
}

// ChaiScript save idiom (ChaiScriptXml.cpp:74-111): root "ChaiScripts",
// children "ChaiScriptItem", int "active" + 5 string attrs.
static void FreezeChaiWriters(std::string& out)
{
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* outer = doc.NewElement("ChaiScripts");
		doc.InsertEndChild(outer);

		tinyxml2::XMLElement* e1 = doc.NewElement("ChaiScriptItem");
		e1->SetAttribute("active", 1);
		e1->SetAttribute("name", kEntities);
		e1->SetAttribute("description", kCjkA);
		e1->SetAttribute("script", "print(\"hi & <bye>\")");
		e1->SetAttribute("guid", "11111111-1111-1111-1111-111111111111");
		e1->SetAttribute("version", "1.2&3");
		outer->InsertEndChild(e1);

		tinyxml2::XMLElement* e2 = doc.NewElement("ChaiScriptItem");
		e2->SetAttribute("active", 0);
		e2->SetAttribute("name", kCjkB);
		e2->SetAttribute("description", "");
		e2->SetAttribute("script", kCjkA);
		e2->SetAttribute("guid", "22222222-2222-2222-2222-222222222222");
		e2->SetAttribute("version", "");
		outer->InsertEndChild(e2);

		EmitPrinterBytes(out, "chaiscript.items", doc);

		// Load-side readback with the exact attribute reads from
		// ChaiScriptXml.cpp:54-59, including the "vesion" typo as-is.
		tinyxml2::XMLPrinter printer(nullptr, true);
		doc.Accept(&printer);
		tinyxml2::XMLDocument back;
		CHECK_TRUE(back.Parse(printer.CStr()) == tinyxml2::XML_SUCCESS);
		tinyxml2::XMLElement* header = back.FirstChildElement("ChaiScripts");
		CHECK_TRUE(header != NULL);
		int n = 0;
		for (tinyxml2::XMLElement* item = header ? header->FirstChildElement() : NULL; item;
			item = item->NextSiblingElement())
		{
			// Presence probe for the golden line only; the app uses bare
			// QueryIntAttribute (harness-vs-app idiom note in the header).
			const char* r = item->Attribute("active");
			int active = -1;
			if (r != NULL)
				item->QueryIntAttribute("active", &active);
			char nb[128];
			std::snprintf(nb, sizeof(nb), "%d active=%d", n, active);
			out += std::string("chaiscript.readback[") + nb + "] active_attr="
				+ (r ? "present" : "(null)") + "\n";
			EmitAttrLine(out, item, "name");
			EmitAttrLine(out, item, "description");
			EmitAttrLine(out, item, "script");
			EmitAttrLine(out, item, "guid");
			// Frozen typo: the loader reads "vesion", the writer stores
			// "version", so the typo read is always NULL. Do NOT fix.
			const char* typo = item->Attribute("vesion");
			const char* fixed = item->Attribute("version");
			out += std::string("chaiscript.readback[") + std::to_string(n)
				+ "] vesion_typo=" + Escape(typo)
				+ " version=" + Escape(fixed) + "\n";
			if (n == 0)
			{
				CHECK_TRUE(active == 1);
				CHECK_TRUE(typo == NULL);
				CHECK_TRUE(fixed && std::strcmp(fixed, "1.2&3") == 0);
				const char* nm = item->Attribute("name");
				CHECK_TRUE(nm && std::strcmp(nm, kEntities) == 0);
			}
			n++;
		}
		CHECK_TRUE(n == 2);
	}
	{
		// Empty list.
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* outer = doc.NewElement("ChaiScripts");
		doc.InsertEndChild(outer);
		EmitPrinterBytes(out, "chaiscript.empty", doc);
	}
}

// CustomFriends save idiom (CustomFriendsHelper.cpp:60-83).
static void FreezeFriendWriters(std::string& out)
{
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* outer = doc.NewElement("CustomFriends");
		doc.InsertEndChild(outer);

		tinyxml2::XMLElement* e1 = doc.NewElement("Friend");
		e1->SetAttribute("name", kEntities);
		e1->SetAttribute("desc", kCjkA);
		outer->InsertEndChild(e1);

		tinyxml2::XMLElement* e2 = doc.NewElement("Friend");
		e2->SetAttribute("name", kCjkB);
		e2->SetAttribute("desc", "a&b<c>d\"e'f");
		outer->InsertEndChild(e2);

		EmitPrinterBytes(out, "friends.items", doc);

		tinyxml2::XMLPrinter printer(nullptr, true);
		doc.Accept(&printer);
		tinyxml2::XMLDocument back;
		CHECK_TRUE(back.Parse(printer.CStr()) == tinyxml2::XML_SUCCESS);
		tinyxml2::XMLElement* header = back.FirstChildElement("CustomFriends");
		CHECK_TRUE(header != NULL);
		int n = 0;
		for (tinyxml2::XMLElement* item = header ? header->FirstChildElement() : NULL; item;
			item = item->NextSiblingElement())
		{
			EmitAttrLine(out, item, "name");
			EmitAttrLine(out, item, "desc");
			if (n == 0)
			{
				const char* nm = item->Attribute("name");
				const char* ds = item->Attribute("desc");
				CHECK_TRUE(nm && std::strcmp(nm, kEntities) == 0);
				CHECK_TRUE(ds && std::strcmp(ds, kCjkA) == 0);
			}
			n++;
		}
		CHECK_TRUE(n == 2);
	}
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* outer = doc.NewElement("CustomFriends");
		doc.InsertEndChild(outer);
		EmitPrinterBytes(out, "friends.empty", doc);
	}
}

// Past-searches save idiom (QPasteWnd.cpp SavePastSearchesFrom).
static void FreezeSearchWriters(std::string& out)
{
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* outer = doc.NewElement("PastSearches");
		doc.InsertEndChild(outer);

		tinyxml2::XMLElement* e1 = doc.NewElement("Search");
		e1->SetAttribute("text", kEntities);
		outer->InsertEndChild(e1);

		tinyxml2::XMLElement* e2 = doc.NewElement("Search");
		e2->SetAttribute("text", kCjkA);
		outer->InsertEndChild(e2);

		EmitPrinterBytes(out, "searches.items", doc);

		tinyxml2::XMLPrinter printer(nullptr, true);
		doc.Accept(&printer);
		tinyxml2::XMLDocument back;
		CHECK_TRUE(back.Parse(printer.CStr()) == tinyxml2::XML_SUCCESS);
		tinyxml2::XMLElement* header = back.FirstChildElement("PastSearches");
		CHECK_TRUE(header != NULL);
		int n = 0;
		for (tinyxml2::XMLElement* item = header ? header->FirstChildElement() : NULL; item;
			item = item->NextSiblingElement())
		{
			EmitAttrLine(out, item, "text");
			if (n == 0)
			{
				const char* t = item->Attribute("text");
				CHECK_TRUE(t && std::strcmp(t, kEntities) == 0);
			}
			if (n == 1)
			{
				const char* t = item->Attribute("text");
				CHECK_TRUE(t && std::strcmp(t, kCjkA) == 0);
			}
			n++;
		}
		CHECK_TRUE(n == 2);
	}
	{
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLElement* outer = doc.NewElement("PastSearches");
		doc.InsertEndChild(outer);
		EmitPrinterBytes(out, "searches.empty", doc);
	}
}

static std::string FreezeWriters()
{
	std::string out;
	out += "tinyxml2=11.0.0 major=11 minor=0 patch=0\n";
	out += "note=no XML declaration is ever emitted; writers persist strings only\n";
	CheckWireCompat();
	FreezeChaiWriters(out);
	FreezeFriendWriters(out);
	FreezeSearchWriters(out);
	return out;
}

// ------------------------------------------------------------ main ----

int main(int argc, char** argv)
{
	if (argc != 4)
	{
		std::printf("usage: test_xml_parity.exe <lang-dir> <theme-dir> <golden-out-dir>\n");
		return 2;
	}

	fs::path langDir = argv[1];
	fs::path themeDir = argv[2];
	fs::path outDir = argv[3];

	std::vector<fs::path> langFiles = SortedXml(langDir);
	std::vector<fs::path> themeFiles = SortedXml(themeDir);

	// Corpus size freeze: 30 language files + 14 theme files.
	CHECK_TRUE(langFiles.size() == 30);
	CHECK_TRUE(themeFiles.size() == 14);

	std::error_code ec;
	fs::create_directories(outDir / "lang", ec);
	fs::create_directories(outDir / "themes", ec);

	int langOk = 0, themeOk = 0;
	for (const fs::path& p : langFiles)
	{
		std::string golden = FreezeLanguageFile(p);
		std::string head = golden.substr(0, golden.find('\n'));
		(void)head;
		if (golden.find("load=OK\n") != std::string::npos)
			langOk++;
		else
		{
			g_failures++;
			std::printf("FAIL load %s\n", p.string().c_str());
		}
		fs::path dst = outDir / "lang" / (p.filename().string() + ".golden.txt");
		if (!WriteFile(dst, golden))
		{
			g_failures++;
			std::printf("FAIL write %s\n", dst.string().c_str());
		}
	}
	for (const fs::path& p : themeFiles)
	{
		std::string golden = FreezeThemeFile(p);
		if (golden.find("load=OK\n") != std::string::npos)
			themeOk++;
		else
		{
			g_failures++;
			std::printf("FAIL load %s\n", p.string().c_str());
		}
		fs::path dst = outDir / "themes" / (p.filename().string() + ".golden.txt");
		if (!WriteFile(dst, golden))
		{
			g_failures++;
			std::printf("FAIL write %s\n", dst.string().c_str());
		}
	}

	std::string writers = FreezeWriters();
	if (!WriteFile(outDir / "writers.golden.txt", writers))
	{
		g_failures++;
		std::printf("FAIL write writers.golden.txt\n");
	}

	std::printf("lang=%d/%u themes=%d/%u\n",
		langOk, (unsigned)langFiles.size(), themeOk, (unsigned)themeFiles.size());
	if (g_failures == 0)
	{
		std::printf("PASS xml_parity: %u lang + %u theme goldens + writers\n",
			(unsigned)langFiles.size(), (unsigned)themeFiles.size());
		return 0;
	}
	std::printf("FAIL xml_parity: %d failures\n", g_failures);
	return 1;
}
