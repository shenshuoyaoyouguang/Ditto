#pragma once

#include <windows.h>
#include <wincrypt.h> // CryptoAPI for the streaming SHA-256 helper (advapi32, default-linked)

// Network "friends" transfer is plaintext LAN traffic: use on trusted networks only.
// File-transfer integrity hashes ride inside CSendInfo (see below).

#define CHUNK_WRITE_SIZE 65536

// CSendInfo wire versions. V1 (MD5 only) is what all released builds send.
// V2 appends a hex SHA-256 while still filling m_md5, so V2 receivers verify
// SHA-256 first and fall back to MD5; V1 senders keep working against V2
// receivers via the MD5 fallback. V1 receivers reject V2 packets by size, so
// upgrade both sides together (old-to-new keeps working, new-to-old does not).
#define SEND_INFO_VERSION_1 1
#define SEND_INFO_VERSION_2 2

#define SEND_INFO_SHA256_HEX_LEN 64
#define SEND_INFO_SHA256_BUF_LEN 65 // 64 hex chars + NUL

class MyEnums
{
public:
	enum eSendType{START, DATA, DATA_START, DATA_END, END, EXIT, REQUEST_FILES};
};

// Streaming SHA-256 over CryptoAPI (no new lib dependency).
// Usage mirrors CMd5: Begin, Update per chunk, EndHex. Any failure leaves out empty.
class CSha256Stream
{
public:
	CSha256Stream() : m_hProv(NULL), m_hHash(NULL), m_ok(FALSE) {}
	~CSha256Stream() { Cleanup(); }

	BOOL Begin()
	{
		Cleanup();
		if (CryptAcquireContext(&m_hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) == FALSE)
			return FALSE;
		if (CryptCreateHash(m_hProv, CALG_SHA_256, 0, 0, &m_hHash) == FALSE)
		{
			Cleanup();
			return FALSE;
		}
		m_ok = TRUE;
		return TRUE;
	}

	BOOL Update(const void *pBuf, DWORD len)
	{
		if (m_ok == FALSE)
			return FALSE;
		if (len == 0)
			return TRUE;
		return CryptHashData(m_hHash, static_cast<const BYTE*>(pBuf), len, 0);
	}

	BOOL EndHex(char outBuf[SEND_INFO_SHA256_BUF_LEN])
	{
		outBuf[0] = 0;
		if (m_ok == FALSE)
			return FALSE;
		BYTE digest[32];
		DWORD digestLen = sizeof(digest);
		if (CryptGetHashParam(m_hHash, HP_HASHVAL, digest, &digestLen, 0) == FALSE || digestLen != sizeof(digest))
			return FALSE;
		static const char hexDigits[] = "0123456789abcdef";
		for (int i = 0; i < 32; i++)
		{
			outBuf[i * 2] = hexDigits[(digest[i] >> 4) & 0xF];
			outBuf[i * 2 + 1] = hexDigits[digest[i] & 0xF];
		}
		outBuf[SEND_INFO_SHA256_HEX_LEN] = 0;
		return TRUE;
	}

private:
	CSha256Stream(const CSha256Stream &);
	void operator=(const CSha256Stream &);

	void Cleanup()
	{
		if (m_hHash != NULL)
		{
			CryptDestroyHash(m_hHash);
			m_hHash = NULL;
		}
		if (m_hProv != NULL)
		{
			CryptReleaseContext(m_hProv, 0);
			m_hProv = NULL;
		}
		m_ok = FALSE;
	}

	HCRYPTPROV m_hProv;
	HCRYPTHASH m_hHash;
	BOOL m_ok;
};

class CSendInfo
{
public:
	CSendInfo()
	{
		memset(this, 0, sizeof(*this));
		m_nSize = sizeof(CSendInfo);
		m_nVersion = SEND_INFO_VERSION_2;
		m_lParameter1 = -1;
		m_lParameter2 = -1;
		m_manualSend = 0;
		m_respondPort = 0;
	}
	int					m_nSize;
	MyEnums::eSendType	m_Type;
	int					m_nVersion;
	CHAR				m_cIP[20];
	CHAR				m_cComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	CHAR				m_cDesc[250];
	long				m_lParameter1;
	long				m_lParameter2;
	CHAR				m_md5[32];
	char				m_manualSend;
	short				m_respondPort;
	char				m_cExtra[15];
	// V2+: hex SHA-256 of the transferred file, NUL-terminated. Empty when talking to V1 peers.
	CHAR				m_sha256[SEND_INFO_SHA256_BUF_LEN];
};

// Exact V1 wire layout (pre-SHA-256). Used to accept packets from released builds.
// Must stay byte-identical to the old CSendInfo; see the static_assert below.
class CSendInfoV1
{
public:
	CSendInfoV1()
	{
		memset(this, 0, sizeof(*this));
		m_nSize = sizeof(CSendInfoV1);
		m_nVersion = SEND_INFO_VERSION_1;
		m_lParameter1 = -1;
		m_lParameter2 = -1;
		m_manualSend = 0;
		m_respondPort = 0;
	}
	int					m_nSize;
	MyEnums::eSendType	m_Type;
	int					m_nVersion;
	CHAR				m_cIP[20];
	CHAR				m_cComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	CHAR				m_cDesc[250];
	long				m_lParameter1;
	long				m_lParameter2;
	CHAR				m_md5[32];
	char				m_manualSend;
	short				m_respondPort;
	char				m_cExtra[15];
};

// The V2 append must stay a pure tail grow (+64 on the wire after alignment) so the
// receive path can derive V1/V2 sizes instead of hard-coding them.
static_assert(sizeof(CSendInfo) == sizeof(CSendInfoV1) + 64, "CSendInfo layout drift: update RecieveSocket size math");

class CDittoCF_HDROP
{
public:
	CDittoCF_HDROP()
	{
		memset(m_cIP, 0, sizeof(m_cIP));
		memset(m_cComputerName, 0, sizeof(m_cComputerName));
		respondPort = 0;
	}
	char m_cIP[25];
	char m_cComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	short respondPort;
};