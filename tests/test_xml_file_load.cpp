// Unit tests for src/XmlFileLoad.h -- raw byte loader + null-preserving
// attribute read for the TinyXml -> tinyxml2 migration, no MFC.
//
// Expected values are hand-derived: byte counts from the literal payloads
// below, BOM bytes from the Unicode standard (EF BB BF), never from the
// implementation under test.
//
// Build & run (local, no MFC/tinyxml2 needed):
//   cl /nologo /std:c++17 /W4 /EHsc /I src tests\test_xml_file_load.cpp /Fe:test_xml_file_load.exe
//   .\test_xml_file_load.exe
// Delete the exe/obj afterwards to keep the repo root clean.
//
// Red-green proof for this shim: compile once with a stubbed XmlFileLoad.h
// earlier on the /I chain (a version whose LoadUtf8FileForXml always returns
// false and whose AttrOrNull always returns "stub") and watch these tests
// fail; recompile with /I src and watch them pass.

#include "XmlFileLoad.h"

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>

// Minimal duck-type stand-in for tinyxml2::XMLElement: the header's
// AttrOrNull only needs Attribute(const char*) const, so this proves the
// null-preserving contract without tinyxml2 headers installed.
struct FakeElement
{
	const char* Attribute(const char* name) const
	{
		if (name == nullptr)
		{
			return nullptr;
		}
		if (std::strcmp(name, "present") == 0)
		{
			return "value";
		}
		if (std::strcmp(name, "empty") == 0)
		{
			return "";
		}
		return nullptr;
	}
};

static int g_failures = 0;

#define CHECK_TRUE(cond) do { \
	if (!(cond)) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  %s\n", __FILE__, (long)__LINE__, #cond); \
	} \
} while (0)

#define CHECK_EQ_STR(a, b) do { \
	if (std::strcmp((a), (b)) != 0) { \
		g_failures++; \
		std::printf("FAIL %s:%ld  [%s] != [%s]\n", __FILE__, (long)__LINE__, (a), (b)); \
	} \
} while (0)

static std::wstring TempPath(unsigned n, const wchar_t* suffix)
{
	wchar_t dir[MAX_PATH] = {};
	DWORD dirLen = ::GetTempPathW(MAX_PATH, dir);
	CHECK_TRUE(dirLen > 0 && dirLen < MAX_PATH);
	wchar_t buf[MAX_PATH] = {};
	swprintf_s(buf, L"%sDittoXmlFileLoad_%u_%s", dir, n, suffix);
	return std::wstring(buf);
}

static bool WriteBytes(const std::wstring& path, const char* data, size_t len)
{
	FILE* fp = nullptr;
	if (_wfopen_s(&fp, path.c_str(), L"wb") != 0 || fp == nullptr)
	{
		return false;
	}
	bool ok = true;
	if (len > 0)
	{
		ok = fwrite(data, 1, len, fp) == len;
	}
	fclose(fp);
	return ok;
}

static void test_missing_file()
{
	// Given: a path nothing was written to
	const std::wstring path = TempPath(1, L"missing.xml");
	_wremove(path.c_str());
	std::string bytes("poison");
	std::string err;

	// When: loading it
	const bool ok = LoadUtf8FileForXml(path.c_str(), bytes, err);

	// Then: false, empty output, and a loggable file+reason error
	CHECK_TRUE(!ok);
	CHECK_TRUE(bytes.empty());
	CHECK_TRUE(!err.empty());
	CHECK_TRUE(err.find("cannot open file") != std::string::npos);
}

static void test_empty_file()
{
	// Given: a zero-byte file
	const std::wstring path = TempPath(2, L"empty.xml");
	CHECK_TRUE(WriteBytes(path, nullptr, 0));
	std::string bytes("poison");
	std::string err;

	// When: loading it (mirrors TIXML_ERROR_DOCUMENT_EMPTY at length <= 0)
	const bool ok = LoadUtf8FileForXml(path.c_str(), bytes, err);

	// Then: false with an "empty" reason, matching tinyxml's empty rejection
	CHECK_TRUE(!ok);
	CHECK_TRUE(bytes.empty());
	CHECK_TRUE(err.find("empty") != std::string::npos);
	_wremove(path.c_str());
}

static void test_utf8_with_bom()
{
	// Given: UTF-8 bytes with BOM (EF BB BF per the Unicode standard)
	const char payload[] = "\xEF\xBB\xBF<Ditto_Language_File><Item>hi</Item></Ditto_Language_File>";
	const std::wstring path = TempPath(3, L"bom.xml");
	CHECK_TRUE(WriteBytes(path, payload, sizeof(payload) - 1));
	std::string bytes;
	std::string err;

	// When: loading it
	const bool ok = LoadUtf8FileForXml(path.c_str(), bytes, err);

	// Then: bytes pass through verbatim, BOM kept for the parser to skip
	CHECK_TRUE(ok);
	CHECK_TRUE(err.empty());
	CHECK_TRUE(bytes.size() == sizeof(payload) - 1);
	CHECK_TRUE(memcmp(bytes.data(), payload, sizeof(payload) - 1) == 0);
	CHECK_TRUE(bytes.size() >= 3 &&
		static_cast<unsigned char>(bytes[0]) == 0xEF &&
		static_cast<unsigned char>(bytes[1]) == 0xBB &&
		static_cast<unsigned char>(bytes[2]) == 0xBF);
	_wremove(path.c_str());
}

static void test_cjk_path_and_content()
{
	// Given: CJK characters in both the filename and the file content
	const char payload[] = "<r><Item>\xE6\xB5\x8B\xE8\xAF\x95\xE4\xB8\xAD\xE6\x96\x87</Item></r>";
	const std::wstring path = TempPath(4, L"\x6D4B\x8BD5_cjk.xml");
	CHECK_TRUE(WriteBytes(path, payload, sizeof(payload) - 1));
	std::string bytes;
	std::string err;

	// When: loading through the wide-char path
	const bool ok = LoadUtf8FileForXml(path.c_str(), bytes, err);

	// Then: exact round-trip, byte count from the literal above
	CHECK_TRUE(ok);
	CHECK_TRUE(err.empty());
	CHECK_TRUE(bytes.size() == sizeof(payload) - 1);
	CHECK_TRUE(memcmp(bytes.data(), payload, sizeof(payload) - 1) == 0);
	_wremove(path.c_str());
}

static void test_crlf_preserved()
{
	// Given: CRLF line endings (file must be opened binary, no EOL translation)
	const char payload[] = "<r>\r\n<Item>a</Item>\r\n</r>\r\n";
	const std::wstring path = TempPath(5, L"crlf.xml");
	CHECK_TRUE(WriteBytes(path, payload, sizeof(payload) - 1));
	std::string bytes;
	std::string err;

	// When: loading it
	const bool ok = LoadUtf8FileForXml(path.c_str(), bytes, err);

	// Then: CR bytes survive verbatim; the parser (not the loader) owns EOL
	CHECK_TRUE(ok);
	CHECK_TRUE(bytes.size() == sizeof(payload) - 1);
	CHECK_TRUE(bytes.find('\r') != std::string::npos);
	CHECK_TRUE(memcmp(bytes.data(), payload, sizeof(payload) - 1) == 0);
	_wremove(path.c_str());
}

static void test_null_path()
{
	// Given: a null path
	std::string bytes("poison");
	std::string err;

	// When: loading it
	const bool ok = LoadUtf8FileForXml(nullptr, bytes, err);

	// Then: false with a loggable reason, no crash
	CHECK_TRUE(!ok);
	CHECK_TRUE(bytes.empty());
	CHECK_TRUE(!err.empty());
}

static void test_attr_or_null()
{
	// Given: an element with one set, one empty, and no other attributes
	const FakeElement elem;

	// When/Then: present reads through, missing stays null (never "")
	CHECK_TRUE(std::strcmp(AttrOrNull(&elem, "present"), "value") == 0);
	CHECK_TRUE(AttrOrNull(&elem, "absent") == nullptr);
	CHECK_TRUE(AttrOrNull(&elem, "vesion") == nullptr); // bug-compatible typo read: null, not ""
	CHECK_TRUE(AttrOrNull(static_cast<const FakeElement*>(nullptr), "present") == nullptr);
	CHECK_TRUE(AttrOrNull(&elem, nullptr) == nullptr);

	// Empty attribute is set-but-empty: non-null "" pins the null-vs-empty split
	const char* empty = AttrOrNull(&elem, "empty");
	CHECK_TRUE(empty != nullptr);
	if (empty != nullptr)
	{
		CHECK_EQ_STR(empty, "");
	}
}

int main()
{
	test_missing_file();
	test_empty_file();
	test_utf8_with_bom();
	test_cjk_path_and_content();
	test_crlf_preserved();
	test_null_path();
	test_attr_or_null();

	if (g_failures == 0)
	{
		std::printf("PASS test_xml_file_load: all checks passed\n");
		return 0;
	}
	std::printf("FAIL test_xml_file_load: %d check(s) failed\n", g_failures);
	return 1;
}
