/*!
 * \file      list.c
 *
 * \brief     ʵ�������ݰ������ܣ���������ʽ��̬��Чʵ��δ֪�������ݰ��Ĺ���
 *
 * \copyright dyming (C)2023
 *
 * \code
 *
 *     ͷָ��-> �����ͷ-> ����ڵ�1 ->����ڵ�2 -> ......����ڵ�n
 *
 *      ######################### How to use this driver ####################
  ==============================================================================
 * 1�������ȴ���һ��list�� ��void list_creat(ListNode_t **list)
 * 2�����������ݰ���С�������β������һ���µĽڵ㣺ListNode_t * list_node_creat(ListNode_t *list,uint32_t bytes)
 * 3���������ݰ�push���ô����õĽڵ��У�bool list_node_data_push(ListNode_t *list,ListNode_t * node,void *data,uint16_t len)
 * 4)�����׽ڵ�ȡ���ڵ���������ݣ�ÿ��ȡ�׽ڵ���������ݷ����Ƚ��ȳ���ԭ��bool list_node_data_pop(ListNode_t *list,ListNode_t * node,void *data,uint16_t *len)
 *
  ******************************************************************************
 *
 * \author    dyming
 *
 * \author    
 */
#include <stdio.h>
#include "malloc.h"
#include "list.h"

static ListNode_t *pList=NULL;

/*!
* \brief    ����һ���µ�������ʼ������ͷ
* \remark   ͷ�����size ������ʾ��ǰ����ڵ�ĸ�����ͷ�ڵ����
*           ͷ����buf ������
* \param    [in out]list    ָ��һ������ָ���ָ�����
* \retval   void
*/
void list_creat(ListNode_t **list)
{
    (*list)=(ListNode_t *)mymalloc( SRAMIN,sizeof(ListNode_t));
    if(!(*list))  return;
    
    (*list)->size=0;
    (*list)->buf=NULL;
    (*list)->pNext=NULL;
}

/*!
* \brief    ��ָ��������β������һ���µĽڵ�
* \remark   

* \param    [in] list     ָ������
* \param    [in] bytes   �����Ľڵ��С

* \retval   ListNode     �����ɹ��Ľڵ�ָ�� [NULL:����ʧ��] 
*/
ListNode_t * list_node_creat(ListNode_t *list,uint32_t bytes)
{
  ListNode_t *pCreat;
  ListNode_t *pTemp;
  
  pCreat=(ListNode_t *)mymalloc(SRAMIN,sizeof(ListNode_t));//����һ���µĽڵ�
  if(pCreat==NULL) 
  {
    return NULL;
  }
  pTemp=list;
  for(;pTemp->pNext;pTemp=pTemp->pNext)//��������β��
  {
  }
  
  pCreat->size=bytes;
  pCreat->buf=(uint8_t *)mymalloc(SRAMIN,bytes);//���½ڵ�����ݴ洢�������ڴ�
  if(!(pCreat->buf)) 
  {
   return NULL;
  }
  
  memset(pCreat->buf,0,bytes);//���ݴ洢����0
  pTemp->pNext=pCreat;//�½ڵ���ӵ���ǰ����β��
  pCreat->pNext=NULL;
  list->size++;
  
  return pCreat;  
}

/*!
* \brief    ��ָ�������в���ָ���ڵ�
* \remark   
* \param    [in] list   ָ������
* \param    [in] node   ָ���ڵ�
* \retval   uint32_t   �ڵ�λ��   ��λ��index 1��ʼ
*/

uint32_t list_node_find(ListNode_t *list,ListNode_t *node)
{
    ListNode_t *pTemp;
    
    if((list==NULL)||(node==NULL)) return NULL;
    
    pTemp=list;
    for(uint8_t i=0;pTemp->pNext;pTemp=pTemp->pNext)
    {
      i++;
      if(node==pTemp->pNext)
      {
       return i;
      }
    }
    return NULL;//��������û���ҵ�ָ���ڵ�
}

/*!
* \brief    ��ָ��������ɾ��ָ���ڵ�
* \remark   
* \param    [in] list   ָ������
* \param    [in] node   ָ���ڵ�
* \retval   void
*/

void list_node_delete(ListNode_t *list,ListNode_t * node)
{
    ListNode_t *pTemp=list;
    ListNode_t *pLastNode;
    uint32_t pos=list_node_find(list,node);
  
    if(pos==0) return;//��������û���ҵ�ָ���ڵ�
  
    for(uint32_t i=0;i<pos-1;i++)
    {
      pTemp=pTemp->pNext;
    }
    pLastNode=pTemp;
  
    if(node->pNext==NULL){
    
      pLastNode->pNext=NULL;
    }
    else{
      pLastNode->pNext=node->pNext;
    }
    myfree(SRAMIN,node->buf);//�ͷŽڵ��������ڴ�
    myfree(SRAMIN,node);//�ͷŸýڵ㱾����ڴ�
   
    list->size--;
}

/*!
* \brief    �����ݰ�push��ָ�������ָ���ڵ���
* \remark   

* \param    [in] list   ָ������
* \param    [in] node   ָ���ڵ�
* \param    [in] data   ��Ҫpush������ָ��
* \param    [in] len    ��Ҫpush�����ݳ���  �ֽڸ���

* \retval   TRUE        push�ɹ�
* \retval   FALSE       pushʧ��
*/

bool list_node_data_push(ListNode_t *list,ListNode_t * node,void *data,uint16_t len)
{
    if(list_node_find(list,node)==NULL
       ||(node->size<len)||(!len))  
    {
      return FALSE;
    }
    
    for(uint16_t i=0;i<len;i++)
    {
        node->buf[i]=((uint8_t*)data)[i];
    }
    return TRUE;
}

/*!
* \brief    ��ָ������ָ���ڵ����������pop����
* \remark   

* \param    [in] list   ָ������
* \param    [in] node   ָ���ڵ�
* \param    [in] data   ���pop���ݵ�ָ��
* \param    [out] len   ���pop���ݳ��ȵ�ָ�� 

* \retval   TRUE        pop�ɹ�
* \retval   FALSE       popʧ��
*/

bool list_node_data_pop(ListNode_t *list,ListNode_t * node,void *data,uint16_t *len)
{
    if( data ==NULL ) return FALSE;
    
    if((list_node_find(list,node)==NULL))
    {
      return FALSE;
    }
       
    for(uint16_t i=0;i<node->size;i++)
    { 
      ((uint8_t *)data)[i]=node->buf[i];
    }
    *len=node->size;
   
    list_node_delete(list,node);//ʹ����󼴿�ɾ���ڵ�
    
    return TRUE;
}

/*!
* \brief    �����ܲ��Ժ���
* \remark   
* \param    void
* \retval   void
*/

void list_test()
{
  static ListNode_t *nd1,*nd2,*nd3,*nd4;
  static uint32_t index;
  static uint16_t use_rate;
  char str[]="this is a test";
  char str2[]="hello world!";
  uint8_t data1=14;
  char retval[20];
  uint16_t len;
  uint8_t retdata;
  
  use_rate=my_mem_perused(SRAMIN);
  printf("mem used per is:%d\r\n",use_rate);
  
  //����һ���µ�����
  list_creat(&pList);
  if(pList==NULL)
  {
    return;
  }
  
  printf("pList creat success\r\n");
  
   use_rate=my_mem_perused(SRAMIN);
   printf("mem used per is:%d\r\n",use_rate);
  
  //����ʵ��������ָ�������д����µĽڵ�
  nd1=list_node_creat(pList,sizeof(str));//index=1
  if(!nd1)
  {
     printf("node1 creat err!");
     while(1);
  }
  //�����ݰ�push��ָ�����������ָ���ڵ��� 
  if(list_node_data_push(pList,nd1,str,sizeof(str)))
  {
    printf(" nod1 push ok\r\n");
  }
  
  nd2=list_node_creat(pList,sizeof(str2));//index=2
  if(!nd2)
  {
     printf("node2 creat err!");
     while(1);
  }
  if(list_node_data_push(pList,nd2,str2,sizeof(str2)))
  {
     printf("node2 push ok\r\n");
  }
  
  nd3=list_node_creat(pList,sizeof(data1));//index=3
  if(!nd3)
  {
     printf("node3 creat err!");
     while(1);
  }
  if(list_node_data_push(pList,nd3,&data1,sizeof(data1)))
  {
     printf("node3 push ok\r\n");
  }
  
   use_rate=my_mem_perused(SRAMIN);
  printf("mem used per is:%d\r\n",use_rate);
  
  //��ָ�������������ָ���ڵ�
  //index=list_node_find(pList,nd2);
 
  //����������ָ���ڵ����ݰ�pop��ָ��Ŀ�ĵ�
 /* if(list_node_data_pop(pList,nd1,retval,&len))
  {
    printf("pop ok");
  }
   index=list_node_find(pList,nd2);
  if(list_node_data_pop(pList,nd2,retval,&len))
  {
     printf("pop ok");
  }
  index=list_node_find(pList,nd2);
  if(list_node_data_pop(pList,nd2,retval,&len))
  {
     printf("pop ok");
  }*/
  
  /*
  for(ListNode_t * pTemp=pList->pNext;pTemp;pTemp=pTemp->pNext)
  {
    if(list_node_data_pop(pList,pTemp,retval,&len))
    {
      printf("pop ok");
    }
  }*/
  
  if(list_node_data_pop(pList,pList->pNext,retval,&len))//ȡ����һ���ڵ����������
  {
    printf("nod1 pop ok\r\n");
    printf("retval is:%s\r\n",retval);
  }
  
  if(list_node_data_pop(pList,pList->pNext,retval,&len))//ȡ���ڶ����ڵ����������
  {
    printf("nod2 pop ok\r\n");
    printf("retval is:%s\r\n",retval);
  }
  
  if(list_node_data_pop(pList,pList->pNext,&retdata,&len))//ȡ���������ڵ����������
  {
    printf("nod3 pop ok\r\n");
    printf("retval is:%d\r\n",retdata);
  }
 
  use_rate=my_mem_perused(SRAMIN);
  printf("mem used per is:%d\r\n",use_rate);
  
  //ɾ��ָ�������е�ָ���ڵ�
  //list_node_delete(pList,nd3);
  //use_rate=my_mem_perused(SRAMIN);
  //printf("mem used per is:%d\r\n",use_rate);

}