/* adlist.h - A generic doubly linked list implementation
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

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */
//双端链表节点
typedef struct listNode {
    struct listNode *prev; //前向节点指针
    struct listNode *next; //后向节点指针
    void *value; //数据指针
} listNode;

//双端链表迭代器
typedef struct listIter {
    listNode *next; //下一节点指针
    int direction; //遍历方向
} listIter;

//双端链表结构体
typedef struct list {
    listNode *head; //头节点指针
    listNode *tail; //尾节点指针
    void *(*dup)(void *ptr);//拷贝函数
    void (*free)(void *ptr);//内存释放函数
    int (*match)(void *ptr, void *key);//匹配函数
    unsigned long len;//链表长度
} list;

/* Functions implemented as macros */
#define listLength(l) ((l)->len) //获取链表长度函数
#define listFirst(l) ((l)->head) //获取链表头节点
#define listLast(l) ((l)->tail) //获取链表尾节点
#define listPrevNode(n) ((n)->prev)//获取前向节点
#define listNextNode(n) ((n)->next)//获取后向节点
#define listNodeValue(n) ((n)->value)//获取节点数据

#define listSetDupMethod(l,m) ((l)->dup = (m))//设置拷贝函数
#define listSetFreeMethod(l,m) ((l)->free = (m))//设置内存释放函数
#define listSetMatchMethod(l,m) ((l)->match = (m))//设置匹配函数

#define listGetDupMethod(l) ((l)->dup)//获取拷贝函数
#define listGetFree(l) ((l)->free)//获取内存释放函数
#define listGetMatchMethod(l) ((l)->match)//获取匹配函数

/* Prototypes */
list *listCreate(void); //创建链表
void listRelease(list *list); //释放链表空间
list *listAddNodeHead(list *list, void *value); //向链表头增加节点
list *listAddNodeTail(list *list, void *value); //向链表尾增加节点
list *listInsertNode(list *list, listNode *old_node, void *value, int after);//插入节点
void listDelNode(list *list, listNode *node);//删除节点
listIter *listGetIterator(list *list, int direction);//获取链表迭代器
listNode *listNext(listIter *iter);//获取下一节点
void listReleaseIterator(listIter *iter);//释放链表迭代器
list *listDup(list *orig);//链表拷贝
listNode *listSearchKey(list *list, void *key);//搜索指定key
listNode *listIndex(list *list, long index);//根据index指定节点
void listRewind(list *list, listIter *li);//重置迭代器
void listRewindTail(list *list, listIter *li);//重置迭代器
void listRotate(list *list);//翻转链表

/* Directions for iterators */
#define AL_START_HEAD 0 //迭代器方向，从链表头开始
#define AL_START_TAIL 1 //迭代器方向，从链表尾开始

#endif /* __ADLIST_H__ */
