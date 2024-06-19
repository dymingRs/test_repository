#ifndef _LIST_H_
#define _LIST_H_

#include <string.h>
#include <stdbool.h>

typedef struct Node{
  uint16_t    size;//当前申请的数据区节点大小 字节个数
  uint8_t     *buf;//当前节点的数据存储区指针
  struct Node *pNext;//下一个节点的指针
} ListNode_t;//__attribute__((packed))

#define FALSE    false
#define TRUE     true

void list_creat(ListNode_t **list);
ListNode_t * list_node_creat(ListNode_t *list,uint32_t bytes);
bool list_node_data_push(ListNode_t *list,ListNode_t * node,void *data,uint16_t len);
bool list_node_data_pop(ListNode_t *list,ListNode_t * node,void *data,uint16_t *len);
uint32_t list_node_find(ListNode_t *list,ListNode_t *node);
void list_node_delete(ListNode_t *list,ListNode_t * node);
void list_test();

#endif