#pragma once

#include "Winsock2.h"
#include "..\EncryptDecrypt\Encryption.h"
#include "ServerDefines.h"
#include "FileTransferProgressDlg.h"

class CRecieveSocket
{
public:
	CRecieveSocket(SOCKET sk = NULL);

	~CRecieveSocket();
	
	LPVOID ReceiveEncryptedData(long lInSize, long &lOutSize);
	BOOL RecieveExactSize(char *pData, long lSize);
	BOOL RecieveCSendInfo(CSendInfo *pInfo);

	void FreeDecryptedData();

	SOCKET	GetSocket()				{ return m_Sock;	}
	void	SetSocket(SOCKET sock)	{ m_Sock = sock;	}

	void SetProgressBar(CFileTransferProgressDlg *pDlg) { m_pProgress = pDlg; }

protected:
	// Tries every configured network password; sets m_pDataReturnedFromDecrypt on success.
	BOOL DecryptWithConfiguredPasswords(const char *pInput, long lInSize, long &lOutSize);

	CEncryption *m_pEncryptor;
	SOCKET m_Sock;
	UCHAR *m_pDataReturnedFromDecrypt;	

	CFileTransferProgressDlg *m_pProgress;
};