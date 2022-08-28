#define _POSIX_C_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <zlib.h>

#include "compresser.h"
#include "files.h"
#include "utils.h"

#define CHUNK 16384

void compress_file(const char* input, const char* output) {
    FILE* source = fopen(input, "rb");
    FILE* dest = fopen(output, "wb");

    int ret, flush, size=0;
    unsigned have;
    z_stream strm;
    unsigned char temp[CHUNK];
    unsigned char* in = malloc(1000 * CHUNK);
    unsigned char* out = malloc(1000 * CHUNK);
    const unsigned char header[] = { 0x69, 0x0 };

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit(&strm, Z_BEST_COMPRESSION);
    ASSERT(ret == Z_OK);

    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            deflateEnd(&strm);
            break;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;
        
        do {
            strm.avail_out = CHUNK;
            strm.next_out = temp;
            ret = deflate(&strm, flush);
            ASSERT(ret != Z_STREAM_ERROR);
            have = CHUNK - strm.avail_out;

            for(int i=size, j=0; i<size+have; i++, j++) {
                out[i] = temp[j];
            }
            out[size+have] = '\0';

            size += have;
        } while(strm.avail_out == 0);
    } while(flush != Z_FINISH);

    free(in);
    deflateEnd(&strm);

    fwrite(header, 1, 2, dest);
    fwrite(out, 1, size, dest);

    free(out);
    fclose(source);
    fclose(dest);
}

void compress_dir(const char* input, const char* output) {

}

void decompress_file(const char* input, const char* output) {
    FILE* source = fopen(input, "rb");
    FILE* dest = fopen(output, "wb");

    unsigned char header[2];
    fread(header, 1, 2, source);
    printf("magic number: %d\n", header[0]);

    int ret;
    unsigned have;
    z_stream strm;
    unsigned char* in = malloc(1000 * CHUNK);
    unsigned char* out = malloc(1000 * CHUNK);

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    ret = inflateInit(&strm);
    ASSERT(ret == Z_OK);

    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            inflateEnd(&strm);
            break;
        }
        if(strm.avail_in == 0)
            break;
        strm.next_in = in;
        
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            ASSERT(ret != Z_STREAM_ERROR);

            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    inflateEnd(&strm);
                    return;
            }

            have = CHUNK - strm.avail_out;
            if(fwrite(out, 1, have, dest) != have || ferror(dest)) {
                inflateEnd(&strm);
                return;
            }
        } while(strm.avail_out == 0);
    } while(ret != Z_STREAM_END);

    free(in);
    free(out);
    inflateEnd(&strm);

    fclose(source);
    fclose(dest);
}

void decompress_dir(const char* input, const char* output) {

}
