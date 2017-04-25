/* SDSLib 2.0 -- A C dynamic strings library
 *
 * Copyright (c) 2006-2015, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2015, Oran Agra
 * Copyright (c) 2015, Redis Labs, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SDS_H
#define __SDS_H

#define SDS_MAX_PREALLOC (1024*1024)

#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>

//char*与c语言字符串类型兼容
typedef char *sds;

/* Note: sdshdr5 is never used, we just access the flags byte directly.
 * However is here to document the layout of type 5 SDS strings. */
//5种header定义，节省内存使用，字段解释：
//len : 字符串实际长度
//alloc : 字符串已拥有的内存空间容量
//flags : 表示类型,见宏定义
//buf[] : 字符串内容
struct __attribute__ ((__packed__)) sdshdr5 {
    unsigned char flags; /* 3 lsb of type, and 5 msb of string length */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr8 {
    uint8_t len; /* used */
    uint8_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 {
    uint16_t len; /* used */
    uint16_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr32 {
    uint32_t len; /* used */
    uint32_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr64 {
    uint64_t len; /* used */
    uint64_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};

//flags类型定义
#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
#define SDS_TYPE_MASK 7 //类型掩码
#define SDS_TYPE_BITS 3 
#define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T))); //获取header头指针,其中##是拼接的含义
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T)))) //获取header头指针
#define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS) //获取sdshdr5的长度

//计算sds长度
static inline size_t sdslen(const sds s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->len;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->len;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->len;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->len;
    }
    return 0;
}
//计算sds可用空间
static inline size_t sdsavail(const sds s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5: {
            return 0;
        }
        case SDS_TYPE_8: {
            SDS_HDR_VAR(8,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_16: {
            SDS_HDR_VAR(16,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_32: {
            SDS_HDR_VAR(32,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_64: {
            SDS_HDR_VAR(64,s);
            return sh->alloc - sh->len;
        }
    }
    return 0;
}
//设置sds长度
static inline void sdssetlen(sds s, size_t newlen) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            {
                unsigned char *fp = ((unsigned char*)s)-1;
                *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
            }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len = newlen;
            break;
    }
}

static inline void sdsinclen(sds s, size_t inc) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            {
                unsigned char *fp = ((unsigned char*)s)-1;
                unsigned char newlen = SDS_TYPE_5_LEN(flags)+inc;
                *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
            }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len += inc;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len += inc;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len += inc;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len += inc;
            break;
    }
}

/* sdsalloc() = sdsavail() + sdslen() */
//
static inline size_t sdsalloc(const sds s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->alloc;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->alloc;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->alloc;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->alloc;
    }
    return 0;
}

static inline void sdssetalloc(sds s, size_t newlen) {
    unsigned char flags = s[-1]; //[len,alloc,flags,buf[]] 获取flag
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            /* Nothing to do, this type has no total allocation info. */
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->alloc = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->alloc = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->alloc = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->alloc = newlen;
            break;
    }
}

sds sdsnewlen(const void *init, size_t initlen); //sds创建函数
sds sdsnew(const char *init); //根据C字符串初始化sds字符串
sds sdsempty(void); //清空sds
sds sdsdup(const sds s); //字符串拷贝函数
void sdsfree(sds s); //内存释放函数
sds sdsgrowzero(sds s, size_t len); //扩展字符串到指定长度
sds sdscatlen(sds s, const void *t, size_t len); //拼接指定长度的字符串
sds sdscat(sds s, const char *t); //字符串拼接
sds sdscatsds(sds s, const sds t); //字符串拼接
sds sdscpylen(sds s, const char *t, size_t len); //拷贝指定长度的字符串
sds sdscpy(sds s, const char *t); //拷贝字符串

sds sdscatvprintf(sds s, const char *fmt, va_list ap);
#ifdef __GNUC__
sds sdscatprintf(sds s, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
#else
sds sdscatprintf(sds s, const char *fmt, ...);
#endif

sds sdscatfmt(sds s, char const *fmt, ...); //字符串格式化输出
sds sdstrim(sds s, const char *cset); //字符串缩减
void sdsrange(sds s, int start, int end); //字符串截取函数
void sdsupdatelen(sds s); //更新sds长度
void sdsclear(sds s); //清空sds内容
int sdscmp(const sds s1, const sds s2);//字符串比较
sds *sdssplitlen(const char *s, int len, const char *sep, int seplen, int *count); //根据sep分割s，返回sds数组
void sdsfreesplitres(sds *tokens, int count); //释放sds数组空间
void sdstolower(sds s); //sds转小写
void sdstoupper(sds s); //sds转大写
sds sdsfromlonglong(long long value); //根据long long类型的值构建sds字符串
sds sdscatrepr(sds s, const char *p, size_t len); //拼接字符串
sds *sdssplitargs(const char *line, int *argc);
sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen);
sds sdsjoin(char **argv, int argc, char *sep); //字符串拼接
sds sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen); //字符串拼接

/* Low level functions exposed to the user API */
sds sdsMakeRoomFor(sds s, size_t addlen); //sds扩展函数
void sdsIncrLen(sds s, int incr); //sds扩展函数
sds sdsRemoveFreeSpace(sds s); //sds收缩函数
size_t sdsAllocSize(sds s); //sds分配的空间大小
void *sdsAllocPtr(sds s); //sds起始地址

/* Export the allocator used by SDS to the program using SDS.
 * Sometimes the program SDS is linked to, may use a different set of
 * allocators, but may want to allocate or free things that SDS will
 * respectively free or allocate. */
void *sds_malloc(size_t size); //sds分配内存空间
void *sds_realloc(void *ptr, size_t size); //sds重分配内存空间
void sds_free(void *ptr); //释放sds

#ifdef REDIS_TEST
int sdsTest(int argc, char *argv[]);
#endif

#endif
