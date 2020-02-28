#############################################
##                                         ##
##    Copyright (C) 2020-2020 Julian Uy    ##
##  https://sites.google.com/site/awertyb  ##
##                                         ##
##   See details of license at "LICENSE"   ##
##                                         ##
#############################################

CC = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
WINDRES := i686-w64-mingw32-windres
GIT_TAG := $(shell git describe --abbrev=0 --tags)
INCFLAGS += -I. -I.. -I../ncbind -I. -I.. -I../ncbind -Iexternal/zlib -Iexternal/minizip -Iexternal/minizip/lib/bzip2  -Iexternal/minizip/lib/liblzma -Iexternal/minizip/lib/liblzma/api -Iexternal/minizip/lib/liblzma/check -Iexternal/minizip/lib/liblzma/common -Iexternal/minizip/lib/liblzma/lz -Iexternal/minizip/lib/liblzma/lzma -Iexternal/minizip/lib/liblzma/rangecoder
ALLSRCFLAGS += $(INCFLAGS) -DGIT_TAG=\"$(GIT_TAG)\"
CFLAGS += -O2 -flto
CFLAGS += $(ALLSRCFLAGS) -Wall -Wno-unused-value -Wno-format -DNDEBUG -DWIN32 -D_WIN32 -D_WINDOWS 
CFLAGS += -DHAVE_PKCRYPT -DHAVE_WZAES -DMZ_ZIP_NO_SIGNING -D__USE_FILE_OFFSET64 -D__USE_LARGEFILE64 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_STDINT_H -DHAVE_INTTYPES_H -DHAVE_ZLIB -DHAVE_BZIP2 -DBZ_NO_STDIO -DHAVE_LZMA -DLZMA_API_STATIC -DHAVE_CONFIG_H -DHAVE_LIMITS_H -DHAVE_STRING_H -DHAVE_STRINGS_H -DHAVE_MEMORY_H -DHAVE_STDBOOL_H -DHAVE_IMMINTRIN_H -DSYMBOLIC_LINK_FLAG_DIRECTORY=0x1
CFLAGS += -D_USRDLL -DMINGW_HAS_SECURE_API -DUNICODE -D_UNICODE -DNO_STRICT
CXXFLAGS += $(CFLAGS) -fpermissive
LDFLAGS += -static -static-libstdc++ -static-libgcc -shared -Wl,--kill-at
LDLIBS += -lodbc32 -lodbccp32 -lgdi32 -lcomctl32 -lcomdlg32 -lole32 -loleaut32 -luuid

%.o: %.c
	@printf '\t%s %s\n' CC $<
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp
	@printf '\t%s %s\n' CXX $<
	$(CXX) -c $(CXXFLAGS) -o $@ $<

%.o: %.rc
	@printf '\t%s %s\n' WINDRES $<
	$(WINDRES) $(WINDRESFLAGS) $< $@

SOURCES := ../tp_stub.cpp ../ncbind/ncbind.cpp ioapi.cpp main.cpp storage.cpp minizip.rc external/minizip/lib/bzip2/blocksort.c external/minizip/lib/bzip2/bzlib.c external/minizip/lib/bzip2/compress.c external/minizip/lib/bzip2/crctable.c external/minizip/lib/bzip2/decompress.c external/minizip/lib/bzip2/huffman.c external/minizip/lib/bzip2/randtable.c external/minizip/lib/liblzma/check/check.c external/minizip/lib/liblzma/check/crc32_fast.c external/minizip/lib/liblzma/check/crc32_table.c external/minizip/lib/liblzma/common/alone_decoder.c external/minizip/lib/liblzma/common/alone_encoder.c external/minizip/lib/liblzma/common/common.c external/minizip/lib/liblzma/common/filter_encoder.c external/minizip/lib/liblzma/lz/lz_decoder.c external/minizip/lib/liblzma/lz/lz_encoder.c external/minizip/lib/liblzma/lz/lz_encoder_mf.c external/minizip/lib/liblzma/lzma/fastpos_table.c external/minizip/lib/liblzma/lzma/lzma_decoder.c external/minizip/lib/liblzma/lzma/lzma_encoder.c external/minizip/lib/liblzma/lzma/lzma_encoder_optimum_fast.c external/minizip/lib/liblzma/lzma/lzma_encoder_optimum_normal.c external/minizip/lib/liblzma/lzma/lzma_encoder_presets.c external/minizip/lib/liblzma/rangecoder/price_table.c external/minizip/mz_compat.c external/minizip/mz_crypt.c external/minizip/mz_crypt_win32.c external/minizip/mz_os.c external/minizip/mz_os_win32.c external/minizip/mz_strm.c external/minizip/mz_strm_buf.c external/minizip/mz_strm_bzip.c external/minizip/mz_strm_lzma.c external/minizip/mz_strm_mem.c external/minizip/mz_strm_os_win32.c external/minizip/mz_strm_pkcrypt.c external/minizip/mz_strm_split.c external/minizip/mz_strm_wzaes.c external/minizip/mz_strm_zlib.c external/minizip/mz_zip.c external/minizip/mz_zip_rw.c external/zlib/adler32.c external/zlib/compress.c external/zlib/crc32.c external/zlib/deflate.c external/zlib/gzclose.c external/zlib/gzlib.c external/zlib/gzread.c external/zlib/gzwrite.c external/zlib/infback.c external/zlib/inffast.c external/zlib/inflate.c external/zlib/inftrees.c external/zlib/trees.c external/zlib/uncompr.c external/zlib/zutil.c
OBJECTS := $(SOURCES:.c=.o)
OBJECTS := $(OBJECTS:.cpp=.o)
OBJECTS := $(OBJECTS:.rc=.o)

BINARY ?= minizip.dll
ARCHIVE ?= minizip.$(GIT_TAG).7z

all: $(BINARY)

archive: $(ARCHIVE)

clean:
	rm -f $(OBJECTS) $(BINARY) $(ARCHIVE)

$(ARCHIVE): $(BINARY) 
	rm -f $(ARCHIVE)
	7z a $@ $^

$(BINARY): $(OBJECTS) 
	@printf '\t%s %s\n' LNK $@
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
