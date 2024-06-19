/*!
 * \file      list.c
 *
 * \brief     实现了数据包链表功能，利用链表方式动态高效实现未知长度数据包的管理
 *
 * \copyright dyming (C)2023
 *
 * \code
 *
 *     头指针-> 链表表头-> 链表节点1 ->链表节点2 -> ......链表节点n
 *
 *      ######################### How to use this driver ####################
  ==============================================================================
 * 1）、首先创建一个list， ：void list_creat(ListNode_t **list)
 * 2）、根据数据包大小在链表的尾部创建一个新的节点：ListNode_t * list_node_creat(ListNode_t *list,uint32_t bytes)
 * 3）、把数据包push到该创建好的节点中：bool list_node_data_push(ListNode_t *list,ListNode_t * node,void *data,uint16_t len)
 * 4)、从首节点取出节点里面的数据，每次取首节点里面的数据符合先进先出的原则：bool list_node_data_pop(ListNode_t *list,ListNode_t * node,void *data,uint16_t *len)
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
* \brief    创建一个新的链表并初始化链表头
* \remark   头结点中size 用来表示当前链表节点的个数，头节点除外
*           头结点的buf 无意义
* \param    [in out]list    指向一个链表指针的指针变量
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
* \brief    在指定的链表尾部创建一个新的节点
* \remark   

* \param    [in] list     指定链表
* \param    [in] bytes   创建的节点大小

* \retval   ListNode     创建成功的节点指针 [NULL:创建失败] 
*/
ListNode_t * list_node_creat(ListNode_t *list,uint32_t bytes)
{
  ListNode_t *pCreat;
  ListNode_t *pTemp;
  
  pCreat=(ListNode_t *)mymalloc(SRAMIN,sizeof(ListNode_t));//创建一个新的节点
  if(pCreat==NULL) 
  {
    return NULL;
  }
  pTemp=list;
  for(;pTemp->pNext;pTemp=pTemp->pNext)//查找链表尾部
  {
  }
  
  pCreat->size=bytes;
  pCreat->buf=(uint8_t *)mymalloc(SRAMIN,bytes);//给新节点的数据存储区申请内存
  if(!(pCreat->buf)) 
  {
   return NULL;
  }
  
  memset(pCreat->buf,0,bytes);//数据存储区清0
  pTemp->pNext=pCreat;//新节点添加到当前链表尾部
  pCreat->pNext=NULL;
  list->size++;
  
  return pCreat;  
}

/*!
* \brief    在指定链表中查找指定节点
* \remark   
* \param    [in] list   指定链表
* \param    [in] node   指定节点
* \retval   uint32_t   节点位置   从位置index 1开始
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
    return NULL;//链表里面没有找到指定节点
}

/*!
* \brief    在指定链表中删除指定节点
* \remark   
* \param    [in] list   指定链表
* \param    [in] node   指定节点
* \retval   void
*/

void list_node_delete(ListNode_t *list,ListNode_t * node)
{
    ListNode_t *pTemp=list;
    ListNode_t *pLastNode;
    uint32_t pos=list_node_find(list,node);
  
    if(pos==0) return;//链表里面没有找到指定节点
  
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
    myfree(SRAMIN,node->buf);//释放节点数据区内存
    myfree(SRAMIN,node);//释放该节点本身的内存
   
    list->size--;
}

/*!
* \brief    把数据包push到指定链表的指定节点中
* \remark   

* \param    [in] list   指定链表
* \param    [in] node   指定节点
* \param    [in] data   需要push的数据指针
* \param    [in] len    需要push的数据长度  字节个数

* \retval   TRUE        push成功
* \retval   FALSE       push失败
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
* \brief    从指定链表指定节点里面把数据pop出来
* \remark   

* \param    [in] list   指定链表
* \param    [in] node   指定节点
* \param    [in] data   存放pop数据的指针
* \param    [out] len   存放pop数据长度的指针 

* \retval   TRUE        pop成功
* \retval   FALSE       pop失败
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
   
    list_node_delete(list,node);//使用完后即可删除节点
    
    return TRUE;
}

/*!
* \brief    链表功能测试函数
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
  
  //创建一个新的链表
  list_creat(&pList);
  if(pList==NULL)
  {
    return;
  }
  
  printf("pList creat success\r\n");
  
   use_rate=my_mem_perused(SRAMIN);
   printf("mem used per is:%d\r\n",use_rate);
  
  //根据实际需求在指定链表中创建新的节点
  nd1=list_node_creat(pList,sizeof(str));//index=1
  if(!nd1)
  {
     printf("node1 creat err!");
     while(1);
  }
  //把数据包push到指定链表里面的指定节点中 
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
  
  //在指定链表里面查找指定节点
  //index=list_node_find(pList,nd2);
 
  //把链表里面指定节点数据包pop到指定目的地
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
  
  if(list_node_data_pop(pList,pList->pNext,retval,&len))//取出第一个节点里面的数据
  {
    printf("nod1 pop ok\r\n");
    printf("retval is:%s\r\n",retval);
  }
  
  if(list_node_data_pop(pList,pList->pNext,retval,&len))//取出第二个节点里面的数据
  {
    printf("nod2 pop ok\r\n");
    printf("retval is:%s\r\n",retval);
  }
  
  if(list_node_data_pop(pList,pList->pNext,&retdata,&len))//取出第三个节点里面的数据
  {
    printf("nod3 pop ok\r\n");
    printf("retval is:%d\r\n",retdata);
  }
 
  use_rate=my_mem_perused(SRAMIN);
  printf("mem used per is:%d\r\n",use_rate);
  
  //删除指定链表中的指定节点
  //list_node_delete(pList,nd3);
  //use_rate=my_mem_perused(SRAMIN);
  //printf("mem used per is:%d\r\n",use_rate);

}