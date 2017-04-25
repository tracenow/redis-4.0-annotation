/* adlist.c - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
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


#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h"

/* Create a new list. The created list can be freed with
 * AlFreeList(), but private value of every node need to be freed
 * by the user before to call AlFreeList().
 *
 * On error, NULL is returned. Otherwise the pointer to the new list. */
 //创建链表
list *listCreate(void)
{
    struct list *list; //定义链表

    if ((list = zmalloc(sizeof(*list))) == NULL) //分配内存空间，若失败，则返回NULl
        return NULL;
    //初始化链表各字段
    list->head = list->tail = NULL; //头尾节点指针均为NULL
    list->len = 0; //链表长度为0
    list->dup = NULL; //链表拷贝函数为NULL
    list->free = NULL; //链表内存释放函数为NULL
    list->match = NULL; //链表节点值匹配函数为NULL
    return list;
}

/* Free the whole list.
 *
 * This function can't fail. */
 //释放双端链表
void listRelease(list *list)
{
    unsigned long len;
    listNode *current, *next;

    current = list->head;//当前节点,从链表头节点开始
    len = list->len;//链表长度
    while(len--) {//遍历所有节点
        next = current->next;//获取下一节点
        if (list->free) list->free(current->value);//释放节点值
        zfree(current);//释放当前节点
        current = next;//令当前节点指向下一节点
    }
    zfree(list);//释放链表
}

/* Add a new node to the list, to head, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
//向链表头增加节点
list *listAddNodeHead(list *list, void *value)
{
    listNode *node; //定义新增的节点

    if ((node = zmalloc(sizeof(*node))) == NULL)//为新节点分配内存空间，若失败，则返回NULL
        return NULL;
    node->value = value;//设置数据值
    if (list->len == 0) {//若此时链表为空表
        list->head = list->tail = node;//则设置头尾指针指向新增节点
        node->prev = node->next = NULL;//此时新增节点同时为头尾节点，则其前向指针以及后向指针均为NULL
    } else { //若此时链表不为空
        node->prev = NULL; //新增节点为新的头节点，其前向指针为NULL
        node->next = list->head;//新增节点的后向指针为旧的头节点
        list->head->prev = node;//旧的头节点的前向指针指向新的头节点，即新增节点
        list->head = node;//更新链表的头节点为新增节点
    }
    list->len++; //链表长度+1
    return list;
}

/* Add a new node to the list, to tail, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
list *listAddNodeTail(list *list, void *value)
{
    listNode *node; //定义新增的节点

    if ((node = zmalloc(sizeof(*node))) == NULL)//为新节点分配内存空间，若失败，则返回NULL
        return NULL;
    node->value = value;//设置数据值
    if (list->len == 0) {//若链表为空表，则与listAddNodeHead操作一致
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else { //此处与listAddNodeHead处理头节点的方式类似
        node->prev = list->tail;
        node->next = NULL;
        list->tail->next = node;
        list->tail = node;
    }
    list->len++;
    return list;
}
//插入节点
list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
    listNode *node; //定义新增节点

    if ((node = zmalloc(sizeof(*node))) == NULL)//为新节点分配内存空间，若失败，则返回NULL
        return NULL;
    node->value = value; //设置数据值
    if (after) { //若插入节点位于old_node之后
        node->prev = old_node; //设置新增节点的前向指针指向old_node
        node->next = old_node->next;//设置新增节点的后向指针指向old_node的后继节点
        if (list->tail == old_node) { //若old_node为原链表尾节点，则更新尾节点为新增节点
            list->tail = node;
        }
    } else { //若插入节点位于old_node之前
        node->next = old_node; //设置新增节点的后向指针指向old_node节点
        node->prev = old_node->prev;//设置新增节点的前向指针指向old_node的前继节点
        if (list->head == old_node) {//若old_node为原链表头节点，则更新头节点为新增节点
            list->head = node;
        }
    }
    if (node->prev != NULL) {//若新增节点不为头节点
        node->prev->next = node; //则令前继节点的后向指针指向新增节点
    }
    if (node->next != NULL) {//若新增节点不为尾节点
        node->next->prev = node;//则令后继节点的前向指针指向新增节点
    }
    list->len++;
    return list;
}

/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * This function can't fail. */
 //删除节点
void listDelNode(list *list, listNode *node)
{
    if (node->prev) //若待删除节点不为头节点
        node->prev->next = node->next; //则令待删除节点的前继节点的后向指针指向待删除节点的后继节点
    else //若待删除节点为头节点
        list->head = node->next; //则更新头节点为待删除节点的后继节点
    if (node->next) //若待删除节点不为尾节点
        node->next->prev = node->prev;//则令待删除节点的后继节点的前向指针指向待删除节点的前继节点
    else //若待删除节点为尾节点
        list->tail = node->prev; //则更新尾节点为待删除节点的前继节点
    if (list->free) list->free(node->value);
    zfree(node);
    list->len--;
}

/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * This function can't fail. */
 //获取双端链表的迭代器
listIter *listGetIterator(list *list, int direction)
{
    listIter *iter; //定义迭代器

    if ((iter = zmalloc(sizeof(*iter))) == NULL) return NULL;//为迭代器分配空间，若分配失败，则返回NULL
    if (direction == AL_START_HEAD) //若反向为AL_START_HEAD，则设置迭代器迭代指针指向链表头节点
        iter->next = list->head;
    else //反之，则设置迭代器迭代指针指向链表尾节点
        iter->next = list->tail;
    iter->direction = direction; //更新迭代器的迭代方向
    return iter;
}

/* Release the iterator memory */
//是否迭代器
void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/* Create an iterator in the list private iterator structure */
//重置迭代器
void listRewind(list *list, listIter *li) {
    li->next = list->head;
    li->direction = AL_START_HEAD;
}

//重置迭代器
void listRewindTail(list *list, listIter *li) {
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}

/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage patter
 * is:
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 *
 * */
 //获取迭代的下一节点
listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;

    if (current != NULL) {
        if (iter->direction == AL_START_HEAD)
            iter->next = current->next;
        else
            iter->next = current->prev;
    }
    return current;
}

/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 *
 * The original list both on success or error is never modified. */
 //链表拷贝
list *listDup(list *orig)
{
    list *copy;
    listIter iter;
    listNode *node;

    if ((copy = listCreate()) == NULL)
        return NULL;
    copy->dup = orig->dup;
    copy->free = orig->free;
    copy->match = orig->match;
    listRewind(orig, &iter);
    while((node = listNext(&iter)) != NULL) {
        void *value;

        if (copy->dup) {
            value = copy->dup(node->value);
            if (value == NULL) {
                listRelease(copy);
                return NULL;
            }
        } else
            value = node->value;
        if (listAddNodeTail(copy, value) == NULL) {
            listRelease(copy);
            return NULL;
        }
    }
    return copy;
}

/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. */
 //根据key搜索表节点
listNode *listSearchKey(list *list, void *key)
{
    listIter iter;
    listNode *node;
    //重置迭代器
    listRewind(list, &iter);
    //遍历表，若设置匹配函数，则根据匹配函数匹配，否则，默认根据==匹配
    while((node = listNext(&iter)) != NULL) {
        if (list->match) {
            if (list->match(node->value, key)) {
                return node;
            }
        } else {
            if (key == node->value) {
                return node;
            }
        }
    }
    return NULL;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. */
 //根据索引index获取链表节点
listNode *listIndex(list *list, long index) {
    listNode *n;

    if (index < 0) {
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;
    } else {
        n = list->head;
        while(index-- && n) n = n->next;
    }
    return n;
}

/* Rotate the list removing the tail node and inserting it to the head. */
//链表翻转
void listRotate(list *list) {
    listNode *tail = list->tail;

    if (listLength(list) <= 1) return;

    /* Detach current tail */
    list->tail = tail->prev;
    list->tail->next = NULL;
    /* Move it as head */
    list->head->prev = tail;
    tail->prev = NULL;
    tail->next = list->head;
    list->head = tail;
}
