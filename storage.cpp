#include <stdio.h>
#include <string.h>
#include "ncbind/ncbind.hpp"
#include "istream_compat.h"
#include <map>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include "mz_compat.h"
#include "mz_strm.h"
#include "zlib.h"

#include "narrow.h"

#define CASESENSITIVITY (0)

#define BASENAME TJS_W("zip")

// UTF8なファイル名かどうかのフラグ
#define FLAG_UTF8 (1<<11)
extern void storeFilename(ttstr &name, const char *narrowName, bool utf8);

// ファイルアクセス用
extern  zlib_filefunc_def KrkrFileFuncDef;

/**
 * Zip 展開処理クラス
 */
class UnzipBase {

public:
	UnzipBase() : refCount(1), uf(NULL), utf8(false) {
#if 0
		::InitializeCriticalSection(&cs);
#endif
	}

	void AddRef() {
		refCount++;
	};

	void Release() {
		if (refCount == 1) {
			delete this;
		} else {
			refCount--;
		}
	};
	
	/**
	 * ZIPファイルを開く
	 * @param filename ファイル名
	 */
	bool init(const ttstr &filename) {
		done();
		if ((uf = unzOpen2_64((const void*)filename.c_str(), &KrkrFileFuncDef)) != NULL) {
			lock();
			unzGoToFirstFile(uf);
			unz_file_info file_info;
			// UTF8判定
			if (unzGetCurrentFileInfo(uf, &file_info,NULL,0,NULL,0,NULL,0) == UNZ_OK) {
				utf8 = (file_info.flag & FLAG_UTF8) != 0;
			}
			do {
				char filename_inzip[1024];
				unz_file_info file_info;
				if (unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip),NULL,0,NULL,0) == UNZ_OK) {
					ttstr filename;
					storeFilename(filename, filename_inzip, utf8);
					entryName(filename);
				}
			} while (unzGoToNextFile(uf) == UNZ_OK);
			unlock();
			return true;
		}
		return false;
	}

	/**
	 * 個別の展開用ファイルを開く
	 */
	bool open(const ttstr &srcname, ULONG *size) {
		if (uf) {
			lock();
			if (unzLocateFile(uf, NarrowString(srcname, utf8), CASESENSITIVITY) == UNZ_OK) {
				if (size) {
					unz_file_info file_info;
					if (unzGetCurrentFileInfo(uf, &file_info, NULL,0,NULL,0,NULL,0) == UNZ_OK) {
						*size = file_info.uncompressed_size;
					}
				}
				if (unzOpenCurrentFile(uf) == UNZ_OK) {
					return true;
				}
			}
			unlock();
		}
		return false;
	}

	/**
	 * 個別の展開用ファイルからデータを読み込む
	 */
	HRESULT read(void *pv, ULONG cb, ULONG *pcbRead) {
		if (uf) {
			lock();
			DWORD size = unzReadCurrentFile(uf, pv,cb);
			if (pcbRead) {
				*pcbRead = size;
			}
			unlock();
			return size < cb ? S_FALSE : S_OK;
		}
		return STG_E_ACCESSDENIED;
	}

	HRESULT seek(ZPOS64_T pos) {
		if (uf) {
			HRESULT ret;
			lock();
			ret = unzSetOffset64(uf, pos) == UNZ_OK ? S_OK : S_FALSE;
			unlock();
			return ret;
		}
		return STG_E_ACCESSDENIED;
	}
	
	ZPOS64_T tell() {
		ZPOS64_T ret = 0;
		if (uf) {
			lock();
			ret = unzTell64(uf);
			unlock();
		}
		return ret;
	}

	/**
	 * 個別の展開用ファイルを閉じる
	 */
	void close() {
		if (uf) {
			lock();
			unzCloseCurrentFile(uf);
			unlock();
		}
	}
	
	bool CheckExistentStorage(const ttstr &name) {
		bool ret = true;
		if (uf) {
			lock();
			ret = unzLocateFile(uf, NarrowString(name, utf8), CASESENSITIVITY) == UNZ_OK;
			unlock();
		}
		return ret;
	}
	
	void GetListAt(const ttstr &name, iTVPStorageLister *lister) {
		ttstr fname = "/";
		fname += name;
		std::map<ttstr,FileNameList>::const_iterator it = dirEntryTable.find(fname);
		if (it != dirEntryTable.end()) {
			std::vector<ttstr>::const_iterator fit = it->second.begin();
			while (fit != it->second.end()) {
				lister->Add(*fit);
				fit++;
			}
		}
	}

protected:

	/**
	 * デストラクタ
	 */
	virtual ~UnzipBase() {
		done();
#if 0
		::DeleteCriticalSection(&cs);
#endif
	}

	void done() {
		if (uf) {
			unzClose(uf);
			uf = NULL;
		}
	}

	// ロック
	void lock() {
#if 0
		::EnterCriticalSection(&cs);
#endif
		CS.Enter();
	}

	// ロック解除
	void unlock() {
#if 0
		::LeaveCriticalSection(&cs);
#endif
		CS.Leave();
	}

	void entryName(const ttstr &name) {
		ttstr dname = TJS_W("/");
		ttstr fname;
		tjs_string path = name.AsStdString();
		size_t last_dir_pos = path.find_last_of(TJS_W("/"));
		if (last_dir_pos != tjs_string::npos)
		{
			dname += ttstr(path.substr(0, last_dir_pos));
			fname = ttstr(path.substr(last_dir_pos, path.length()));
		}
		else
		{
			fname = name;
		}
		dirEntryTable[dname].push_back(fname);
	}
	
private:
	int refCount;
	// zipファイル情報
	unzFile uf;
	bool utf8;
#if 0
	CRITICAL_SECTION cs;
#endif
	tTJSCriticalSection CS;

	// ディレクトリ別ファイル名エントリ情報
	typedef std::vector<ttstr> FileNameList;
	std::map<ttstr,FileNameList> dirEntryTable;
};

/**
 * ZIP展開ストリームクラス
 */
class UnzipStream : public IStream {

public:
	/**
	 * コンストラクタ
	 */
	UnzipStream(UnzipBase *unzip) : refCount(1), unzip(unzip) {
		unzip->AddRef();
	};

	// IUnknown
#if 0
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) {
		if (riid == IID_IUnknown || riid == IID_ISequentialStream || riid == IID_IStream) {
			if (ppvObject == NULL)
				return E_POINTER;
			*ppvObject = this;
			AddRef();
			return S_OK;
		} else {
			*ppvObject = 0;
			return E_NOINTERFACE;
		}
	}
#endif

	ULONG STDMETHODCALLTYPE AddRef(void) {
		refCount++;
		return refCount;
	}
	
	ULONG STDMETHODCALLTYPE Release(void) {
		int ret = --refCount;
		if (ret <= 0) {
			delete this;
			ret = 0;
		}
		return ret;
	}

	// ISequentialStream
	HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead) {
		return unzip->read(pv, cb, pcbRead);
	}

	HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten) {
		return E_NOTIMPL;
	}

	// IStream
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove,	DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) {
		// 先頭にだけ戻せる
		ZPOS64_T cur;
		switch (dwOrigin) {
		case STREAM_SEEK_CUR:
			cur = unzip->tell();
			cur += dlibMove.QuadPart;
			break;
		case STREAM_SEEK_SET:
			cur = dlibMove.QuadPart;
			break;
		case STREAM_SEEK_END:
			cur = this->size;
			cur += dlibMove.QuadPart;
			break;
		}
		unzip->seek(cur);
		if (plibNewPosition) {
			plibNewPosition->QuadPart = cur;
		}
		return S_OK;
	}
	
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize) {
		return E_NOTIMPL;
	}
	
	HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) {
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags) {
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Revert(void) {
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
		return E_NOTIMPL;
	}
	
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
		return E_NOTIMPL;
	}
	
	HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag) {
		if(pstatstg) {
			memset(pstatstg, 0, sizeof(*pstatstg));

#if 0
			// pwcsName
			// this object's storage pointer does not have a name ...
			if(!(grfStatFlag &  STATFLAG_NONAME)) {
				// anyway returns an empty string
				LPWSTR str = (LPWSTR)CoTaskMemAlloc(sizeof(*str));
				if(str == NULL) return E_OUTOFMEMORY;
				*str = L'\0';
				pstatstg->pwcsName = str;
			}
#endif

#if 0
			// type
			pstatstg->type = STGTY_STREAM;
#endif
			
			// cbSize
			pstatstg->cbSize.QuadPart = size;
			
			// mtime, ctime, atime unknown

#if 0
			// grfMode unknown
			pstatstg->grfMode = STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE ;
			
			// grfLockSuppoted
			pstatstg->grfLocksSupported = 0;
#endif
			
			// grfStatBits unknown
		} else {
			return E_INVALIDARG;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm) {
		return E_NOTIMPL;
	}

	bool init(const ttstr filename) {
		bool ret = false;
		if ((ret = unzip->open(filename, &size))) {
			this->filename = filename;
		}
		return ret;
	}
	
protected:
	/**
	 * デストラクタ
	 */
	virtual ~UnzipStream() {
		close();
		unzip->Release();
	}

	void close() {
		unzip->close();
	}
	
	
private:
	int refCount;
	ttstr filename;
	UnzipBase *unzip;
	ULONG size;
};

/**
 * ZIPストレージ
 */
class ZipStorage : public iTVPStorageMedia
{

public:
	/**
	 * コンストラクタ
	 */
	ZipStorage() : refCount(1) {
	}

	/**
	 * デストラクタ
	 */
	virtual ~ZipStorage() {
		// 全情報を破棄
		std::map<ttstr, UnzipBase*>::iterator it = unzipTable.begin();
		while (it != unzipTable.end()) {
			it->second->Release();
			it = unzipTable.erase(it);
		}
	}

public:
	// -----------------------------------
	// iTVPStorageMedia Intefaces
	// -----------------------------------

	virtual void TJS_INTF_METHOD AddRef() {
		refCount++;
	};

	virtual void TJS_INTF_METHOD Release() {
		if (refCount == 1) {
			delete this;
		} else {
			refCount--;
		}
	};

	// returns media name like "file", "http" etc.
	virtual void TJS_INTF_METHOD GetName(ttstr &name) {
		name = BASENAME;
	}

	//	virtual ttstr TJS_INTF_METHOD IsCaseSensitive() = 0;
	// returns whether this media is case sensitive or not

	// normalize domain name according with the media's rule
	virtual void TJS_INTF_METHOD NormalizeDomainName(ttstr &name) {
		// nothing to do
	}

	// normalize path name according with the media's rule
	virtual void TJS_INTF_METHOD NormalizePathName(ttstr &name) {
		// nothing to do
	}

	// check file existence
	virtual bool TJS_INTF_METHOD CheckExistentStorage(const ttstr &name) {
		ttstr fname;
		UnzipBase *unzip = getUnzip(name, fname);
		return unzip ? unzip->CheckExistentStorage(fname) : false;
	}

	// open a storage and return a tTJSBinaryStream instance.
	// name does not contain in-archive storage name but
	// is normalized.
	virtual tTJSBinaryStream * TJS_INTF_METHOD Open(const ttstr & name, tjs_uint32 flags) {
		if (flags == TJS_BS_READ) { // 読み込みのみ
			ttstr fname;
			UnzipBase *unzip = getUnzip(name, fname);
			if (unzip) {
				UnzipStream *stream = new UnzipStream(unzip);
				if (stream) {
					if (stream->init(fname)) {
						tTJSBinaryStream *ret = TVPCreateBinaryStreamAdapter(stream);
						stream->Release();
						return ret;
					}
					stream->Release();
				}
			}
		}
		TVPThrowExceptionMessage(TJS_W("%1:cannot open zipfile"), name);
		return NULL;
	}

	// list files at given place
	virtual void TJS_INTF_METHOD GetListAt(const ttstr &name, iTVPStorageLister * lister) {
		ttstr fname;
		UnzipBase *unzip = getUnzip(name, fname);
		if (unzip) {
			unzip->GetListAt(fname, lister);
		}
	}

	// basically the same as above,
	// check wether given name is easily accessible from local OS filesystem.
	// if true, returns local OS native name. otherwise returns an empty string.
	virtual void TJS_INTF_METHOD GetLocallyAccessibleName(ttstr &name) {
		name = "";
	}

public:

	/**
	 * zipファイルをファイルシステムとして mount します
	 * zip://ドメイン名/ファイル名 でアクセス可能になります。読み込み専用になります。
	 * @param name ドメイン名
	 * @param zipfile マウントするZIPファイル名
	 * @return マウントに成功したら true
	 */
	bool mount(const ttstr &name, const ttstr &zipfile) {
		unmount(name);
		UnzipBase *newUnzip = new UnzipBase();
		if (newUnzip) {
			if (newUnzip->init(zipfile)) {
				unzipTable[name] = newUnzip;
				return true;
			} else {
				newUnzip->Release();
			}
		}
		return false;
	}

	/**
	 * zipファイルを unmount します
	 * @param name ドメイン名
	 * @return アンマウントに成功したら true
	 */
	bool unmount(const ttstr &name) {
		std::map<ttstr, UnzipBase*>::iterator it = unzipTable.find(name);
		if (it != unzipTable.end()) {
			it->second->Release();
			unzipTable.erase(it);
			return true;
		}
		return false;
	}

protected:

	/*
	 * ドメインに合致した Unzip 情報を取得
	 * @param name ファイル名
	 * @param fname ファイル名を返す
	 * @return Unzip情報
	 */
	UnzipBase *getUnzip(const ttstr &name, ttstr &fname) {
		ttstr dname;
		const tjs_char *p = name.c_str();
		const tjs_char *q;
		if ((q = TJS_strchr(p, '/'))) {
			dname = ttstr(p, q-p);
			fname = ttstr(q+1);
		} else {
			TVPThrowExceptionMessage(TJS_W("invalid path:%1"), name);
		}
		std::map<ttstr, UnzipBase*>::const_iterator it = unzipTable.find(dname);
		if (it != unzipTable.end()) {
			return it->second;
		}
		return NULL;
	}
	
private:
	tjs_uint refCount; //< リファレンスカウント
	std::map<ttstr, UnzipBase*> unzipTable; //< zip情報
};


/**
 * メソッド追加用
 */
class StoragesZip {

public:
	
	static void init() {
		if (zip == NULL) {
			zip = new ZipStorage();
			TVPRegisterStorageMedia(zip);
		}
	}

	static void done() {
		if (zip != NULL) {
			TVPUnregisterStorageMedia(zip);
			zip->Release();
			zip = NULL;
		}
	}

	/**
	 * zipファイルをファイルシステムとして mount します
	 * zip://ドメイン名/ファイル名 でアクセス可能になります。読み込み専用になります。
	 * @param name ドメイン名
	 * @param zipfile マウントするZIPファイル名
	 * @return マウントに成功したら true
	 */
	static bool mountZip(const tjs_char *name, const tjs_char *zipfile) {
		if (zip) {
			return zip->mount(ttstr(name), ttstr(zipfile));
		}
		return false;
	}

	/**
	 * zipファイルを unmount します
	 * @param name ドメイン名
	 * @return アンマウントに成功したら true
	 */
	static bool unmountZip(const tjs_char *name) {
		if (zip) {
			return zip->unmount(ttstr(name));
		}
		return false;
	}

protected:
	static ZipStorage *zip;
};

ZipStorage *StoragesZip::zip = NULL;

NCB_ATTACH_CLASS(StoragesZip, Storages) {
	NCB_METHOD(mountZip);
	NCB_METHOD(unmountZip);
};

void initZipStorage()
{
	StoragesZip::init();
}

void doneZipStorage()
{
	StoragesZip::done();
}
