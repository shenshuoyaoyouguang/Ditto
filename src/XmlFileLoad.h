#pragma once

// Byte loader for the TinyXml -> tinyxml2 migration. No MFC/CString
// dependencies, so tests/test_xml_file_load.cpp can compile this header
// outside the main project (same convention as src/ThemeColorMath.h).
// <windows.h> is used only for WideCharToMultiByte when rendering the
// wide path into the narrow err string; the file itself is read as raw
// bytes, no text conversion is applied.
//
// Semantics mirror TiXmlDocument::LoadFile (src/TinyXml/tinyxml.cpp:966-1007):
// binary open via _wfopen_s(path, L"rb"), fseek/ftell size pre-allocation,
// false on missing file (TIXML_ERROR_OPENING_FILE) and on zero-length input
// (TIXML_ERROR_DOCUMENT_EMPTY). Bytes are returned verbatim: a UTF-8 BOM is
// kept (tinyxml2::XMLDocument::Parse skips it) and CRLF is preserved (the
// parser normalises EOL itself, which is why the open is binary).
//
// Callers (src/MultiLanguage.cpp:357-365, src/Theme.cpp:133-140) pass a
// wide path (CStringW GetBuffer()) and log file+reason on failure; the err
// string therefore carries both the UTF-8-rendered path and the reason.

#include <windows.h>

#include <cstdio>
#include <string>

// Best-effort wide -> UTF-8 render for log strings. Never fails: falls back
// to a placeholder when the path is null or the conversion fails.
inline std::string NarrowPathForLog(const wchar_t* path)
{
	if (path == nullptr || *path == L'\0')
	{
		return "<null>";
	}

	const int wideLen = static_cast<int>(wcslen(path));
	const int bytesNeeded = ::WideCharToMultiByte(CP_UTF8, 0, path, wideLen, nullptr, 0, nullptr, nullptr);
	if (bytesNeeded <= 0)
	{
		return "<path>";
	}

	std::string out(static_cast<size_t>(bytesNeeded), '\0');
	const int bytesWritten = ::WideCharToMultiByte(CP_UTF8, 0, path, wideLen, &out[0], bytesNeeded, nullptr, nullptr);
	if (bytesWritten != bytesNeeded)
	{
		return "<path>";
	}
	return out;
}

// Reads the whole file at |path| into |outBytes|. Returns true on success,
// false on missing/unreadable/empty/short-read input with file+reason in
// |err| for the caller to log. |outBytes| is cleared on every call and left
// empty on failure.
inline bool LoadUtf8FileForXml(const wchar_t* path, std::string& outBytes, std::string& err)
{
	outBytes.clear();
	err.clear();

	if (path == nullptr || *path == L'\0')
	{
		err = "cannot open file \"" + NarrowPathForLog(path) + "\": null or empty path";
		return false;
	}

	FILE* fp = nullptr;
#if defined(_MSC_VER)
	const errno_t openErr = _wfopen_s(&fp, path, L"rb");
	if (openErr != 0 || fp == nullptr)
	{
		err = "cannot open file \"" + NarrowPathForLog(path) + "\": error " + std::to_string(openErr);
		return false;
	}
#else
	fp = _wfopen(path, L"rb");
	if (fp == nullptr)
	{
		err = "cannot open file \"" + NarrowPathForLog(path) + "\": open failed";
		return false;
	}
#endif

	// Get the file size, so we can pre-allocate the string. HUGE speed impact.
	// (Mirrors tinyxml.cpp:996-1007, including the length <= 0 empty check.)
	fseek(fp, 0, SEEK_END);
	const long length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Strange case, but good to handle up front.
	if (length <= 0)
	{
		fclose(fp);
		err = "file is empty \"" + NarrowPathForLog(path) + "\" (" + std::to_string(length) + " bytes)";
		return false;
	}

	outBytes.resize(static_cast<size_t>(length));
	const size_t got = fread(&outBytes[0], 1, outBytes.size(), fp);
	fclose(fp);

	if (got != outBytes.size())
	{
		err = "short read \"" + NarrowPathForLog(path) + "\": expected " +
			std::to_string(outBytes.size()) + " bytes, got " + std::to_string(got);
		outBytes.clear();
		return false;
	}

	return true;
}

// Null-preserving attribute read. Templated on the element type so this
// header compiles WITHOUT tinyxml2 headers today; the migration task wires
// it to tinyxml2::XMLElement (whose Attribute(const char*) const member fits
// this duck type) with no change needed here. Missing attribute (or null
// element/name) yields nullptr, never "".
template <typename Elem>
inline const char* AttrOrNull(const Elem* elem, const char* name)
{
	if (elem == nullptr || name == nullptr)
	{
		return nullptr;
	}
	return elem->Attribute(name);
}
