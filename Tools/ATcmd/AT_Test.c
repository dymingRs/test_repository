#include "ATcmd.h"
#include "list.h"
#include "malloc.h"

/**
 * @brief   ����AT������
 */
static at_obj_t *at_obj;

extern ListNode_t *pList;

static void at_error(at_response_t *r);
static void at_debug(const char *fmt, ...);
static int wifi_ready_handler(at_urc_info_t *info);
static int wifi_connected_handler(at_urc_info_t *info);
static int wifi_disconnected_handler(at_urc_info_t *info);
void wifi_query_version(void);

unsigned int wifi_uart_write(const void *buf, unsigned int len);
unsigned int wifi_uart_read(void *buf);

static const uint8_t app_at_buffer[40]="hello world,this is a ATcmd test\r\n";

/**
 * @brief   wifi URC��
 */
static const urc_item_t urc_table[] = {
    "ready",'\n',             wifi_ready_handler,
    "WIFI CONNECTED:", '\n',   wifi_connected_handler,
    "WIFI DISCONNECTED", '\n', wifi_disconnected_handler,
};

/** 
 * @brief   AT������
 */
static const at_adapter_t  at_adapter = {
    .write         = wifi_uart_write,
    .read          = wifi_uart_read,
    .error         = at_error,
    .debug         = at_debug,
    .urc_bufsize   = 128,
    .recv_bufsize  = 256
};

 /* @brief   wifi���������¼�
 */
static int wifi_ready_handler(at_urc_info_t *info)
{
    printf("WIFI ready...\r\n");
    return 0;
}

/**
 * @brief   wifi�����¼�
 */
static int wifi_connected_handler(at_urc_info_t *info)
{
    printf("WIFI connection detected...\r\n");
    return 0;
}
/* 
 * @brief   wifi�Ͽ������¼�
 */
static int wifi_disconnected_handler(at_urc_info_t *info)
{
    printf("WIFI disconnect detected...\r\n");
    return 0;
}

/* 
 * @brief   WIFI��������״̬��
 * @return  true - �˳�״̬��, false - ����״̬��,
 */
static int wifi_reset_work(at_env_t *e)
{
    switch (e->state) {
    case 0:                                //�ر�WIFI��Դ
      //  wifi_close();
        e->reset_timer(e);
        e->state++;
        break;
    case 1:
        if (e->is_timeout(e, 2000))       //��ʱ�ȴ�2s
            e->state++;
        break;
    case 2:
       // wifi_open();                       //��������wifi
        e->state++;
        break;
    case 3:
        if (e->is_timeout(e, 3000))       //��Լ��ʱ�ȴ�3s��wifi����
            return true;  
        break;
    }
    return false;
}

/**
 * @brief ��ӡ���
 */
static void at_debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
/**
 * @brief   wifi ͨ���쳣����
 */
static void at_error(at_response_t *r)
{
    printf("wifi AT communication error\r\n");
    //ִ��������ҵ
   // at_do_work(at_obj, NULL, wifi_reset_work);    
    
}


/*
 * @brief	    �򴮿ڷ��ͻ�������д�����ݲ���������
 * @param[in]   buf       -  ���ݻ���
 * @param[in]   len       -  ���ݳ���
 * @return 	    ʵ��д�볤��(�����ʱ��������,�򷵻�len)
 */
unsigned int wifi_uart_write(const void *buf, unsigned int len)
{   
    unsigned int ret;

    printf((const char*)buf);
    return ret; 
}

/*
 * @brief	    ��ȡ���ڽ��ջ�����������
 * @param[in]   buf       -  ���ݻ���
 * @return 	    ��ȡ���ĳ���
 */
unsigned int wifi_uart_read(void *buf)
{ 
     uint16_t size=0;
     
     list_node_data_pop(pList,pList->pNext,(uint8_t *)buf,&size);
     
     return  size;
}

/* 
 * @brief    �Զ���AT������
 */
static void at_ver_sender(at_env_t *e)
{
    e->println(e, "AT+VER");
}
/* 
 * @brief    �汾��ѯ�ص�
 */
static void query_version_callback(at_response_t *r)
{
    if (r->code == AT_RESP_OK ) {
        printf("wifi version info : %s\r\n", r->prefix);
    } 
    else
    {
     printf("wifi version query failure...\r\n"); 
    } 
}

static void data_send_callback(at_response_t *r)
{
      if (r->code == AT_RESP_OK ) {
        printf("data send ok\r\n");
    } 
    else
    {
     //printf("wifi version query failure...\r\n"); 
    } 
}

/* 
 * @brief    ִ�а汾��ѯ����
 */
void wifi_query_version(void)
{
    at_attr_t attr;
    at_attr_deinit(&attr);
    attr.cb     = data_send_callback;    
    //attr.prefix = "VERSION:",                //��Ӧǰ׺
    attr.prefix = "OK",                //��Ӧǰ׺
    attr.suffix = "\n";                      //��Ӧ��׺
    //at_custom_cmd(at_obj, &attr, at_ver_sender);    
    at_send_singlline(at_obj, &attr, "AT+VER");  
    //at_send_data(at_obj, &attr, "hello ATcmd\r\n", 13);
}



/* 
 * @brief    at��ʼ��
 */
void at_init(void)
{
  //����AT������
  at_obj = at_obj_create(&at_adapter);
  //����URC��
  at_obj_set_urc(at_obj, urc_table, sizeof(urc_table) / sizeof(urc_table[0]));         
  //����һ���µ�����,���ڴ������ݰ��Ľ���
  list_creat(&pList); 
  if(pList==NULL)
  {
    return;
  }
}


void at_test()
{  
  at_init();
  wifi_query_version();
  while(1)
  {
    at_obj_process(at_obj);
    
    if( !at_work_is_busy(at_obj ) )
    {
       // wifi_query_version();
    }
  }
}