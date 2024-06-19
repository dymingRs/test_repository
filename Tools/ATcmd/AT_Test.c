#include "ATcmd.h"
#include "list.h"
#include "malloc.h"

/**
 * @brief   定义AT控制器
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
 * @brief   wifi URC表
 */
static const urc_item_t urc_table[] = {
    "ready",'\n',             wifi_ready_handler,
    "WIFI CONNECTED:", '\n',   wifi_connected_handler,
    "WIFI DISCONNECTED", '\n', wifi_disconnected_handler,
};

/** 
 * @brief   AT适配器
 */
static const at_adapter_t  at_adapter = {
    .write         = wifi_uart_write,
    .read          = wifi_uart_read,
    .error         = at_error,
    .debug         = at_debug,
    .urc_bufsize   = 128,
    .recv_bufsize  = 256
};

 /* @brief   wifi开机就绪事件
 */
static int wifi_ready_handler(at_urc_info_t *info)
{
    printf("WIFI ready...\r\n");
    return 0;
}

/**
 * @brief   wifi连接事件
 */
static int wifi_connected_handler(at_urc_info_t *info)
{
    printf("WIFI connection detected...\r\n");
    return 0;
}
/* 
 * @brief   wifi断开连接事件
 */
static int wifi_disconnected_handler(at_urc_info_t *info)
{
    printf("WIFI disconnect detected...\r\n");
    return 0;
}

/* 
 * @brief   WIFI重启任务状态机
 * @return  true - 退出状态机, false - 保持状态机,
 */
static int wifi_reset_work(at_env_t *e)
{
    switch (e->state) {
    case 0:                                //关闭WIFI电源
      //  wifi_close();
        e->reset_timer(e);
        e->state++;
        break;
    case 1:
        if (e->is_timeout(e, 2000))       //延时等待2s
            e->state++;
        break;
    case 2:
       // wifi_open();                       //重启启动wifi
        e->state++;
        break;
    case 3:
        if (e->is_timeout(e, 3000))       //大约延时等待3s至wifi启动
            return true;  
        break;
    }
    return false;
}

/**
 * @brief 打印输出
 */
static void at_debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
/**
 * @brief   wifi 通信异常处理
 */
static void at_error(at_response_t *r)
{
    printf("wifi AT communication error\r\n");
    //执行重启作业
   // at_do_work(at_obj, NULL, wifi_reset_work);    
    
}


/*
 * @brief	    向串口发送缓冲区内写入数据并启动发送
 * @param[in]   buf       -  数据缓存
 * @param[in]   len       -  数据长度
 * @return 	    实际写入长度(如果此时缓冲区满,则返回len)
 */
unsigned int wifi_uart_write(const void *buf, unsigned int len)
{   
    unsigned int ret;

    printf((const char*)buf);
    return ret; 
}

/*
 * @brief	    读取串口接收缓冲区的数据
 * @param[in]   buf       -  数据缓存
 * @return 	    读取到的长度
 */
unsigned int wifi_uart_read(void *buf)
{ 
     uint16_t size=0;
     
     list_node_data_pop(pList,pList->pNext,(uint8_t *)buf,&size);
     
     return  size;
}

/* 
 * @brief    自定义AT发送器
 */
static void at_ver_sender(at_env_t *e)
{
    e->println(e, "AT+VER");
}
/* 
 * @brief    版本查询回调
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
 * @brief    执行版本查询命令
 */
void wifi_query_version(void)
{
    at_attr_t attr;
    at_attr_deinit(&attr);
    attr.cb     = data_send_callback;    
    //attr.prefix = "VERSION:",                //响应前缀
    attr.prefix = "OK",                //响应前缀
    attr.suffix = "\n";                      //响应后缀
    //at_custom_cmd(at_obj, &attr, at_ver_sender);    
    at_send_singlline(at_obj, &attr, "AT+VER");  
    //at_send_data(at_obj, &attr, "hello ATcmd\r\n", 13);
}



/* 
 * @brief    at初始化
 */
void at_init(void)
{
  //创建AT控制器
  at_obj = at_obj_create(&at_adapter);
  //设置URC表
  at_obj_set_urc(at_obj, urc_table, sizeof(urc_table) / sizeof(urc_table[0]));         
  //创建一个新的链表,用于串口数据包的接收
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