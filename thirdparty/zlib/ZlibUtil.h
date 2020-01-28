#ifndef __ZLIB_UTIL_H__
#define __ZLIB_UTIL_H__
#include <string>

class Zlib
{
public:
    static bool compressBuf(const char* pSrcBuf, size_t nSrcBufLength, char* pDestBuf, size_t& nDestBufLength);
    static bool compressBuf(const std::string& strSrcBuf, std::string& strDestBuf);
    static bool uncompressBuf(const std::string& strSrcBuf, std::string& strDestBuf, size_t nDestBufLength);

    //gzip—πÀı
    static bool inflate(const std::string& strSrc, std::string& dest);
    //gzipΩ‚—π
    static bool deflate(const std::string& strSrc, std::string& strDest);

};

#endif //!__ZLIB_UTIL_H__