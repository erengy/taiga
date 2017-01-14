/*
** base64.h: interface for the Base64Coder class.
** http://support.microsoft.com/kb/191239
*/

#pragma once

#include <string>
#include <windows.h>

// =============================================================================

class Base64Coder {
  class TempBucket {
    public:
    BYTE nData[4];
    BYTE nSize;
    void Clear() { ::ZeroMemory(nData, 4); nSize = 0; };
  };

  PBYTE m_pDBuffer;
  PBYTE m_pEBuffer;
  DWORD m_nDBufLen;
  DWORD m_nEBufLen;
  DWORD m_nDDataLen;
  DWORD m_nEDataLen;

public:
  Base64Coder();
  virtual ~Base64Coder();

  virtual void Encode(const PBYTE, DWORD);
  virtual void Decode(const PBYTE, DWORD);
  virtual void Encode(LPCSTR sMessage);
  virtual void Decode(LPCSTR sMessage);

  virtual std::string DecodedMessage() const;
  virtual std::string EncodedMessage() const;

  virtual void AllocEncode(DWORD);
  virtual void AllocDecode(DWORD);
  virtual void SetEncodeBuffer(const PBYTE pBuffer, DWORD nBufLen);
  virtual void SetDecodeBuffer(const PBYTE pBuffer, DWORD nBufLen);

protected:
  virtual void _EncodeToBuffer(const TempBucket& Decode, PBYTE pBuffer);
  virtual ULONG _DecodeToBuffer(const TempBucket& Decode, PBYTE pBuffer);
  virtual void _EncodeRaw(TempBucket&, const TempBucket&);
  virtual void _DecodeRaw(TempBucket&, const TempBucket&);
  virtual BOOL _IsBadMimeChar(BYTE);

  static char m_DecodeTable[256];
  static BOOL m_Init;
  void _Init();
};
