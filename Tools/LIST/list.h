#ifndef _LIST_H_
#define _LIST_H_

#include <string.h>
#include <stdbool.h>

typedef struct Node{
  uint16_t    size;//��ǰ������������ڵ��С �ֽڸ���
  uint8_t     *buf;//��ǰ�ڵ�����ݴ洢��ָ��
  struct Node *pNext;//��һ���ڵ��ָ��
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