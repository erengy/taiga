// Base64Coder.h: interface for the Base64Coder class.
// http://support.microsoft.com/kb/191239
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BASE64CODER_H__B2E45717_0625_11D2_A80A_00C04FB6794C__INCLUDED_)
#define AFX_BASE64CODER_H__B2E45717_0625_11D2_A80A_00C04FB6794C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef _AFXDLL
	#include <afx.h>
	typedef CString		String;
#else
	#include <windows.h>
	#include <string>
	typedef std::string	String;
#endif

class Base64Coder  
{
	// Internal bucket class.
	class TempBucket
	{
	public:
		BYTE		nData[4];
		BYTE		nSize;
		void		Clear() { ::ZeroMemory(nData, 4); nSize = 0; };
	};

	PBYTE					m_pDBuffer;
	PBYTE					m_pEBuffer;
	DWORD					m_nDBufLen;
	DWORD					m_nEBufLen;
	DWORD					m_nDDataLen;
	DWORD					m_nEDataLen;

public:
	Base64Coder();
	virtual ~Base64Coder();

public:
	virtual void		Encode(const PBYTE, DWORD);
	virtual void		Decode(const PBYTE, DWORD);
	virtual void		Encode(LPCSTR sMessage);
	virtual void		Decode(LPCSTR sMessage);

	virtual LPCSTR	DecodedMessage() const;
	virtual LPCSTR	EncodedMessage() const;

	virtual void		AllocEncode(DWORD);
	virtual void		AllocDecode(DWORD);
	virtual void		SetEncodeBuffer(const PBYTE pBuffer, DWORD nBufLen);
	virtual void		SetDecodeBuffer(const PBYTE pBuffer, DWORD nBufLen);

protected:
	virtual void		_EncodeToBuffer(const TempBucket &Decode, PBYTE pBuffer);
	virtual ULONG		_DecodeToBuffer(const TempBucket &Decode, PBYTE pBuffer);
	virtual void		_EncodeRaw(TempBucket &, const TempBucket &);
	virtual void		_DecodeRaw(TempBucket &, const TempBucket &);
	virtual BOOL		_IsBadMimeChar(BYTE);

	static  char		m_DecodeTable[256];
	static  BOOL		m_Init;
	void					_Init();
};

#endif // !defined(AFX_BASE64CODER_H__B2E45717_0625_11D2_A80A_00C04FB6794C__INCLUDED_)
