/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
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

#include <stdint.h>

#ifndef __DICT_H
#define __DICT_H

#define DICT_OK 0
#define DICT_ERR 1

/* Unused arguments generate annoying warnings... */
#define DICT_NOTUSED(V) ((void) V)

//字典hash节点
typedef struct dictEntry {
    //hash键
    void *key;
    //hash值
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    //下一hash节点，采用了开链法才解决哈希冲突
    struct dictEntry *next;
} dictEntry;

//字典类型
typedef struct dictType {
    //hash函数
    uint64_t (*hashFunction)(const void *key);
    //key拷贝函数
    void *(*keyDup)(void *privdata, const void *key);
    //value拷贝函数
    void *(*valDup)(void *privdata, const void *obj);//键比较函数
    //键比较函数
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    //key析构函数
    void (*keyDestructor)(void *privdata, void *key);
    //value析构函数
    void (*valDestructor)(void *privdata, void *obj);
} dictType;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
//hash表
typedef struct dictht {
    //hash节点指针数组
    dictEntry **table;
    //指针数组大小
    unsigned long size;
    //指针数组掩码,用于计算索引值
    unsigned long sizemask;
    //hash表现有节点数
    unsigned long used;
} dictht;

//字典
typedef struct dict {
    //字典类型
    dictType *type;
    //私有数据
    void *privdata;
    //两张hash表
    dictht ht[2];
    //rehash不为-1时，表示正在rehash过程中
    long rehashidx; /* rehashing not in progress if rehashidx == -1 */
    //正在运行的迭代器数量
    unsigned long iterators; /* number of iterators currently running */
} dict;

/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
//字典迭代器
typedef struct dictIterator {
    //字典
    dict *d;
    //索引
    long index;
    //table表示主hash表（可进行写操作的ht表），若safe=1，则表示安全迭代器，反之则表示非安全迭代器
    int table, safe;
    //hash表节点,下一hash节点，用于迭代
    dictEntry *entry, *nextEntry;
    /* unsafe iterator fingerprint for misuse detection. */
    //指纹
    long long fingerprint;
} dictIterator;

typedef void (dictScanFunction)(void *privdata, const dictEntry *de);
typedef void (dictScanBucketFunction)(void *privdata, dictEntry **bucketref);

/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE     4

/* ------------------------------- Macros ------------------------------------*/
//字典value析构
#define dictFreeVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)
//字典值设置
#define dictSetVal(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        (entry)->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \//字典key设置
        (entry)->v.val = (_val_); \
} while(0)
//设置有符号整数
#define dictSetSignedIntegerVal(entry, _val_) \
    do { (entry)->v.s64 = _val_; } while(0)
//设置无符号整数
#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { (entry)->v.u64 = _val_; } while(0)
//设置double类型值
#define dictSetDoubleVal(entry, _val_) \
    do { (entry)->v.d = _val_; } while(0)
//字典key析构
#define dictFreeKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)
//字典key设置
#define dictSetKe//字典key比较函数y(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        (entry)->key = (_key_); \
} while(0)
//字典key比较函数
#define dictCompareKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))

#define dictHashKey(d, key) (d)->type->hashFunction(key) //key hash值
#define dictGetKey(he) ((he)->key) //获取key
#define dictGetVal(he) ((he)->v.val) //获取value
#define dictGetSignedIntegerVal(he) ((he)->v.s64) //获取由符号整数
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64) //获取无符号整数
#define dictGetDoubleVal(he) ((he)->v.d) //获取double值
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size) //获取hash表大小
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used) //获取hash表大小
#define dictIsRehashing(d) ((d)->rehashidx != -1) //是否正在rehash过程

/* API */
dict *dictCreate(dictType *type, void *privDataPtr); //创建字典
int dictExpand(dict *d, unsigned long size); //字典扩展
int dictAdd(dict *d, void *key, void *val); //向字典增加键值对
dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing); //增加键值对
dictEntry *dictAddOrFind(dict *d, void *key); //增加键
int dictReplace(dict *d, void *key, void *val); //替换键值对
int dictDelete(dict *d, const void *key); //删除键值对
dictEntry *dictUnlink(dict *ht, const void *key); 
void dictFreeUnlinkedEntry(dict *d, dictEntry *he);
void dictRelease(dict *d); //字典内存空间释放函数
dictEntry * dictFind(dict *d, const void *key);//根据key查找hash节点
void *dictFetchValue(dict *d, const void *key);//根据key查找value
int dictResize(dict *d);//重分配字典
dictIterator *dictGetIterator(dict *d); //获取字典迭代器
dictIterator *dictGetSafeIterator(dict *d); //获取安全迭代器
dictEntry *dictNext(dictIterator *iter);//获取迭代下一hash节点
void dictReleaseIterator(dictIterator *iter);//释放迭代器
dictEntry *dictGetRandomKey(dict *d);
unsigned int dictGetSomeKeys(dict *d, dictEntry **des, unsigned int count);
void dictGetStats(char *buf, size_t bufsize, dict *d);
uint64_t dictGenHashFunction(const void *key, int len);
uint64_t dictGenCaseHashFunction(const unsigned char *buf, int len);
void dictEmpty(dict *d, void(callback)(void*));
void dictEnableResize(void); //启用字典重分配
void dictDisableResize(void); //禁用字典重分配
int dictRehash(dict *d, int n); //字典rehash
int dictRehashMilliseconds(dict *d, int ms); 
void dictSetHashFunctionSeed(uint8_t *seed); //设置hash seed
uint8_t *dictGetHashFunctionSeed(void); //获取hash seed
unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, dictScanBucketFunction *bucketfn, void *privdata);
unsigned int dictGetHash(dict *d, const void *key); //计算key的hash值
dictEntry **dictFindEntryRefByPtrAndHash(dict *d, const void *oldptr, unsigned int hash);

/* Hash table types */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif /* __DICT_H */
