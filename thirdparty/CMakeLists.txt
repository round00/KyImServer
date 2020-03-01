set(trdparty_src
        jsoncpp/json_reader.cpp
        jsoncpp/json_value.cpp
        jsoncpp/json_writer.cpp

        zlib/zutil.c
        zlib/uncompr.c
        zlib/trees.c
        zlib/inftrees.c
        zlib/inflate.c
        zlib/inffast.c
        zlib/infback.c
        zlib/gzwrite.c
        zlib/gzread.c
        zlib/gzlib.c
        zlib/gzclose.c
        zlib/deflate.c
        zlib/crc32.c
        zlib/compress.c
        zlib/adler32.c
        zlib/ZlibUtil.cpp
        )


add_library(trdparty ${trdparty_src})