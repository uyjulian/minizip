#include <stdio.h>
#include <windows.h>
#include "tp_stub.h"
#include "mz_compat.h"

static void* ZCALLBACK fopen64_file_func (void* opaque, const void* filename, int mode)
{
#if 0
	iTJSBinaryStream* file = NULL;
#else
	IStream* file = NULL;
#endif
	int tjsmode = 0;
	if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
		tjsmode = TJS_BS_READ;
    else
    if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
		tjsmode= TJS_BS_APPEND;
	else
    if (mode & ZLIB_FILEFUNC_MODE_CREATE)
		tjsmode = TJS_BS_WRITE;
	
	if ((filename!=NULL)) {
#if 0
		file = TVPCreateStream(ttstr((const tjs_char*)filename), tjsmode);
#else
		file = TVPCreateIStream(ttstr((const tjs_char*)filename), tjsmode);
#endif
	}
  return file;
}


static unsigned long ZCALLBACK fread_file_func (void* opaque, void* stream, void* buf, unsigned long size)
{
#if 0
	iTJSBinaryStream *is = (iTJSBinaryStream*)stream;
#else
	IStream *is = (IStream*)stream;
#endif
	if (is) {
#if 0
		return is->Read(buf,size);
#else
    ULONG len;
    if (is->Read(buf,size,&len) == S_OK) {
      return len;
    }
#endif
	}
	return 0;
}

static unsigned long ZCALLBACK fwrite_file_func (void* opaque, void* stream, const void* buf, unsigned long size)
{
#if 0
	iTJSBinaryStream *is = (iTJSBinaryStream*)stream;
#else
	IStream *is = (IStream*)stream;
#endif
	if (is) {
#if 0
		return is->Write(buf,size);
#else
    DWORD len;
    if (is->Write(buf,size,&len) == S_OK) {
      return len;
    }
#endif
	}
	return 0;
}

static ZPOS64_T ZCALLBACK ftell64_file_func (void* opaque, void* stream)
{
#if 0
	iTJSBinaryStream *is = (iTJSBinaryStream*)stream;
#else
	IStream *is = (IStream*)stream;
#endif
	if (is) {
#if 0
    return is->Seek(0, TJS_BS_SEEK_CUR);
#else
    LARGE_INTEGER move = {0};
    ULARGE_INTEGER newposition;
    if (is->Seek(move, STREAM_SEEK_CUR, &newposition) == S_OK) {
      return newposition.QuadPart;
    }
#endif
	}
	return -1;
}

static long ZCALLBACK fseek64_file_func (void*  opaque, void* stream, ZPOS64_T offset, int origin)
{
#if 0
	tjs_int dwOrigin;
#else
	DWORD dwOrigin;
#endif
	switch(origin) {
#if 0
	case ZLIB_FILEFUNC_SEEK_CUR: dwOrigin = TJS_BS_SEEK_CUR; break;
	case ZLIB_FILEFUNC_SEEK_END: dwOrigin = TJS_BS_SEEK_END; break;
	case ZLIB_FILEFUNC_SEEK_SET: dwOrigin = TJS_BS_SEEK_SET; break;
#else
  case ZLIB_FILEFUNC_SEEK_CUR: dwOrigin = STREAM_SEEK_CUR; break;
  case ZLIB_FILEFUNC_SEEK_END: dwOrigin = STREAM_SEEK_END; break;
  case ZLIB_FILEFUNC_SEEK_SET: dwOrigin = STREAM_SEEK_SET; break;
#endif
	default: return -1; //failed
	}
#if 0
	iTJSBinaryStream *is = (iTJSBinaryStream*)stream;
#else
	IStream *is = (IStream*)stream;
#endif
	if (is) {
#if 0
    return is->Seek(offset, dwOrigin);
#else
    LARGE_INTEGER move;
    move.QuadPart = offset;
    ULARGE_INTEGER newposition;
    if (is->Seek(move, dwOrigin, &newposition) == S_OK) {
      return newposition.QuadPart;
    }
#endif
	}
	return -1;
}


static int ZCALLBACK fclose_file_func (void* opaque, void* stream)
{
#if 0
	iTJSBinaryStream *is = (iTJSBinaryStream*)stream;
#else
	IStream *is = (IStream*)stream;
#endif
	if (is) {
#if 0
		is->Destruct();
#else
		is->Release();
#endif
		return 0;
	}
	return EOF;
}

static int ZCALLBACK ferror_file_func (void* opaque, void* stream)
{
#if 0
	iTJSBinaryStream *is = (iTJSBinaryStream*)stream;
#else
	IStream *is = (IStream*)stream;
#endif
	if (is) {
		return 0;
	}
	return EOF;
}

zlib_filefunc64_def TVPZlibFileFunc = {
	fopen64_file_func,
	fread_file_func,
	fwrite_file_func,
	ftell64_file_func,
	fseek64_file_func,
	fclose_file_func,
	ferror_file_func,
	NULL
};
