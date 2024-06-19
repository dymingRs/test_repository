/*!
 * \file      mac.c
 *
 * \brief    用户无需过多研究了解RF底层。通过mac抽象协议层，应用层通过简单的配置即可以轻松实现想要的通讯效果；协议层逻辑可重复利用，对产品的稳定性与项目效率的提升有很大帮助
 *            
 * ###################################### features #############################
 *          1）、多通讯模式：定点通讯、广播通讯、定点自动应答（with no msg\with msg）
 *          2）、可支持功耗接收：低功耗接收LDC模式【low power duty cycle mode】
 *          3）、抗同频干扰：信道忙检测BLT 【listen before transmit】
 *          4）、安全：AES128加密
 *          5)、异步回调方式处理mac任务
  ==============================================================================
 * \copyright dyming
 *
 * \code  
 *  mac射频的SPI通讯与单载波测试函数:    static void mac_rf_test( uint32_t freq, int8_t power );
 *  mac对象创建函数:           static Mac_t  * mac_creat( mac_item_t it );
 *  mac初始化函数:             static void mac_int( Mac_t *mac, uint8_t *msg, uint16_t size, AppCallBack callback, LpCallBack lpcb );
 *  mac rf参数配置函数：       static void mac_radio_config( Mac_t *mac, uint32_t freq, int8_t power,uint32_t sf, uint32_t bandwidth, uint8_t coderate, uint32_t per );
 *  mac角色配置为sender函数:   static void mac_config_as_sender( Mac_t *mac, uint32_t app_id, uint32_t dev_id, uint8_t ch_id , uint32_t des_id,  bool ack,
                                                                 uint32_t timeout, uint8_t repeat, uint8_t type, bool encrypt, const uint8_t *key );
 *  mac角色配置为reciver函数:  static void mac_config_as_reciver( Mac_t *mac, uint8_t *msg, uint8_t len, uint32_t app_id, uint32_t dev_id, uint8_t ch_id, bool  afilter, 
                                                                  bool decrypt, const uint8_t *key );
 *  mac发射数据包制作函数:     static uint8_t * mac_txpk_make( Mac_t *mac, uint8_t *msg, uint8_t len ,uint16_t *buflen);
 *  mac工作状态设置函数:       static void mac_states_set(States_t s);
 *  mac工作忙状态获取函数：    static bool mac_busy_get(void);
 *  mac工作运行函数:           static void mac_abstract_task_running(void);
 *  mac LBT信道忙检测函数:     static void mac_lbt( void );
 *  mac接收数据解析函数:       static bool mac_payload_parse(Mac_t *mac, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr, void *arg);
 *  mac业务流程结束处理函数：  static void mac_done_process( void );
 *  mac ack数据包等待超时回调函数: static void mac_ack_timer_callback(MultiTimer* timer, void* userData);
 *  mac任务回调函数:           static void mac_task_callback(Mac_t *mac,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr,void *arg);
 *
 * (C)2023
 * ###################################### How to use this driver ###############
  ==============================================================================
 * 作为SENDER
 * 1）、创建mac对象：Mac.Creat();
 * 2）、配置rf基本参数：Mac.RadioConfig();
 * 3）、配置想要的mac角色：Mac.AsSender();
 * 4)、构建mac发射数据包：Mac.PkMake();
 * 5)、初始化mac：Mac.Int();
 * 6)、设置mac为start_tx状态：Mac.SSet(START_TX);

 * 作为REVICER
 * 1）、创建mac对象：Mac.Creat();
 * 2）、配置rf基本参数：Mac.RadioConfig();
 * 3）、配置想要的mac角色：Mac.AsReciver();
 * 4 )、初始化mac：Mac.Int();
 * 5)、设置mac为start_rx状态：Mac.SSet( START_RX );
 *
  ==============================================================================
 *
 * \author    dyming
 *
 * \author    
 */

/*********************************************************
head file include
*********************************************************/
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "MultiTimer.h"
#include "crc.h"
#include "malloc.h"
#include "mac.h"
#include "mac_aes.h"   

/*********************************************************
extern function declare
*********************************************************/
extern void mac_phy_radio_deint( void );
extern void mac_phy_int( AppCallBack callback );
extern void mac_phy_task_running( mac_obj_t *obj );
extern void mac_phy_cad_check( mac_obj_t *obj );
extern void mac_phy_radio_config( mac_obj_t *obj, uint32_t freq, int8_t power,uint32_t sf, uint32_t bandwidth, uint8_t coderate, uint32_t per );
extern void mac_phy_radio_test( uint32_t freq, int8_t power );
extern void mac_phy_radio_ready( mac_obj_t *obj );
extern void mac_phy_channel_set( uint32_t freq );
extern void mac_phy_power_set( int8_t power );

/********************************************************
static function declare
*********************************************************/
static void mac_int( void );
static void mac_radio_deint(  mac_obj_t * obj );
static void mac_radio_config( mac_obj_t *obj, uint32_t freq, int8_t power,uint32_t sf, uint32_t bandwidth, uint8_t coderate, uint32_t per );
static void mac_config_attribute_of_sender( mac_obj_t *obj,
                                 uint32_t app_id, uint32_t dev_id, uint8_t ch_id , uint32_t des_id,
                                 bool ack, uint32_t timeout, uint8_t repeat,
                                 bool encrypt, const uint8_t *key );    
static void mac_config_attribute_of_reciver( mac_obj_t *obj, 
                                  uint8_t *msg, uint8_t len,
                                  uint32_t app_id, uint32_t dev_id, uint8_t ch_id, bool  afilter, 
                                  bool decrypt, const uint8_t *key );
static uint8_t * mac_txpk_make( mac_obj_t *obj, uint8_t type, uint8_t *msg, uint8_t len, uint16_t *buflen); 
void mac_states_set( mac_obj_t *obj, States_t s);
static mac_obj_t * mac_creat( mac_adapter_t *adp );
static void mac_task_running(void);
static bool mac_busy_get( mac_obj_t * obj );
static bool mac_payload_parse( mac_obj_t *obj, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr, void *arg );
static void mac_task_callback( mac_obj_t *obj, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr,void *arg);
static void mac_done_process( mac_obj_t *obj );
static void mac_lbt( mac_obj_t *obj );
static void mac_ack_timer_callback(MultiTimer* timer, void* userData);
static void mac_rf_test( mac_obj_t *obj, uint32_t freq, int8_t power );
static void mac_radio_ready( mac_obj_t *obj);
static void mac_radio_channel_set( mac_obj_t *obj, uint32_t freq );
static void mac_radio_power_set( mac_obj_t *obj, int8_t power );

#if MULTI_MODULE_MODE_IS_CLIENT

static void mac_updownlink_channel_table_set( mac_obj_t *obj, uint32_t downlink, uint8_t argc,  ...);
static void mac_updownlink_ch_id_table_set( mac_obj_t *obj, uint8_t downlink_ch_id,uint8_t argc, ...);
static uint32_t  mac_downlink_channel_get( mac_obj_t *obj );
static uint32_t  mac_uplink_channel_get( mac_obj_t *obj, uint8_t ch );
static uint8_t mac_downlink_channel_id_get( mac_obj_t *obj );
static uint8_t mac_uplink_channel_id_get( mac_obj_t *obj, uint8_t ch );
static void mac_uplink_channel_set( mac_obj_t *obj, uint8_t ch );
static void mac_uplink_channel_id_set( mac_obj_t *obj, uint8_t ch );

#endif

/********************************************************
static varabale declare
*********************************************************/
static bool                 app_id_filter = true;
static uint8_t              ack_times=0;
static uint16_t             pk_len;
static MultiTimer           pk_ack_timer;
//static uint8_t              seq=0xff;
static bool                aes_enable_ctr;
static const uint8_t       *pAES_Key;
static const uint8_t       aes_key[16] = {0x31,0x90,0x22,0x58,0x49,0xc4,0xd0,0x79,0x08,0x56,0xb7,0xbb,0xcc,0xd0,0xee,0x0f}; //aes密匙
static mac_obj_t            *obj_cursor; 
static struct list_head    mac_obj_list; 
static ListNode_t           *pAckList=NULL; 
static mac_obj_t            shadow_obj;  
static mac_obj_t            *downlink_obj;  

/*!
* \brief    mac方法结构体实例
* \remark   
*          
* \param       
* \retval   
*/
const  MacFunction_t Mac =
{
  .Int =                 mac_int,
  .ObjCreat =            mac_creat,
  .AttributeOfSender  =  mac_config_attribute_of_sender,
  .AttributeOfReciver =  mac_config_attribute_of_reciver,
 // .AsSender=             mac_role_as_sender,
 //  .AsReciver=            mac_role_as_reciver,
  .PkMake =              mac_txpk_make,
  .SSet =                mac_states_set,
  .BusyGet =             mac_busy_get,
  .Running =             mac_task_running,

    {
      .RadioReady=        mac_radio_ready, 
      .RfTest =           mac_rf_test,
      .RadioDeInt=        mac_radio_deint,
      .RadioConfig =      mac_radio_config,
      .RadioChannelSet=   mac_radio_channel_set,
      .RadioPowerSet=     mac_radio_power_set,
      
#if MULTI_MODULE_MODE_IS_CLIENT
      
      .RadioUpDownChTbSet=  mac_updownlink_channel_table_set,
      .RadioUpDownChIdTbSet=mac_updownlink_ch_id_table_set,
      .RadioUpLinkChSet=    mac_uplink_channel_set,
      .RadioUpLinkChIdSet=  mac_uplink_channel_id_set,
      
      .RadioDownLinkChGet=  mac_downlink_channel_get,
      .RadioDownLinkChIdGet= mac_downlink_channel_id_get,
      .RadioUpLinkChGet=    mac_uplink_channel_get,
      .RadioUpLinkChIdGet=  mac_uplink_channel_id_get,
      
#endif
    
    }
};

#if MULTI_MODULE_MODE_IS_CLIENT

static void mac_updownlink_channel_table_set( mac_obj_t *obj, uint32_t downlink, uint8_t argc,  ...)
{
  va_list args;
  mac_infor_t *infor = obj_map_infor( obj);
  
  infor->adapter->updownlink_table.downlink.downlink_ch = downlink;
  
  va_start( args, argc );
  
  for( uint8_t i=0;i<argc;i++ )
  {
    infor->adapter->updownlink_table.uplink.uplink_ch_table[i] = va_arg( args, uint32_t );
  }
  
  va_end(args);
}


static void mac_updownlink_ch_id_table_set( mac_obj_t *obj, uint8_t downlink_ch_id,uint8_t argc, ...)
{
  va_list args;
  mac_infor_t *infor = obj_map_infor( obj);
  
  
  infor->adapter->updownlink_table.downlink.downlink_id = downlink_ch_id;
  
  va_start( args, argc );
  
  for( uint8_t i=0;i<argc;i++ )
  {
    infor->adapter->updownlink_table.uplink.uplink_id_table[i] = va_arg( args, int );
  }
  
  va_end(args);
}

static void mac_uplink_channel_set( mac_obj_t *obj, uint8_t ch )
{
    mac_infor_t *infor = obj_map_infor( obj);
    
    mac_radio_channel_set( obj, infor->adapter->updownlink_table.uplink.uplink_ch_table[ch] );
}


static void mac_uplink_channel_id_set( mac_obj_t *obj, uint8_t ch )
{
    mac_infor_t *infor = obj_map_infor( obj);
    
    obj->mac.mac_head.ch_id = infor->adapter->updownlink_table.uplink.uplink_id_table[ch];

}


static uint32_t  mac_downlink_channel_get( mac_obj_t *obj )
{
   mac_infor_t *infor = obj_map_infor( obj);
   
   return infor->adapter->updownlink_table.downlink.downlink_ch;
}


static uint8_t mac_downlink_channel_id_get( mac_obj_t *obj )
{
    mac_infor_t *infor = obj_map_infor( obj);
    
    return infor->adapter->updownlink_table.downlink.downlink_id;
}


static uint32_t  mac_uplink_channel_get( mac_obj_t *obj, uint8_t ch )
{
   mac_infor_t *infor = obj_map_infor( obj);
   
   return infor->adapter->updownlink_table.uplink.uplink_ch_table[ch];
}


static uint8_t mac_uplink_channel_id_get( mac_obj_t *obj, uint8_t ch )
{
   mac_infor_t *infor = obj_map_infor( obj);
    
   return infor->adapter->updownlink_table.uplink.uplink_id_table[ch];
}

#endif

static void mac_radio_channel_set( mac_obj_t *obj, uint32_t freq )
{
    mac_set_cursor_enter(obj);
    mac_phy_channel_set( freq );
    mac_set_cursor_exit();
}



static void mac_radio_power_set( mac_obj_t *obj, int8_t power )
{
    mac_set_cursor_enter(obj);
    mac_phy_power_set( power );
    mac_set_cursor_exit();
}


static void mac_radio_ready( mac_obj_t *obj)
{
  mac_set_cursor_enter(obj);
  mac_phy_radio_ready( obj );
  mac_set_cursor_exit();
}


mac_obj_t * mac_downlink_obj_get( void )
{
    return downlink_obj;
}

void mac_downlink_obj_set( mac_obj_t *obj )
{
  downlink_obj = obj;
  memcpy( &shadow_obj, obj, sizeof( mac_obj_t ) );

}

ListNode_t * mac_dwlk_queue_get( void )
{
  return pAckList;
}


mac_infor_t * obj_map_infor( mac_obj_t *obj )
{
  return ( mac_infor_t *)obj;
}

mac_obj_t * mac_get_cursor( void )
{
    return obj_cursor;
}

void mac_set_cursor_enter( mac_obj_t *obj )
{
    obj_cursor = obj;
}

void mac_set_cursor_exit( void )
{
    //obj_cursor = NULL;
}

/*!
* \brief    mac射频的SPI通讯与单载波测试函数
* \remark   
*          
* \param    [in] freq       频点
* \param    [in] power      发射功率   
* \retval   void
*/
static void mac_rf_test( mac_obj_t *obj, uint32_t freq, int8_t power )
{
    mac_set_cursor_enter( obj );
    mac_phy_radio_test( freq, power );
    mac_set_cursor_exit();
}


/*!
* \brief    mac object创建函数
* \remark   
*          
* \param    void   
* \retval   [out] pMacObj       mac对象指针
*/
static mac_obj_t * mac_creat( mac_adapter_t *adap)
{
  mac_infor_t *  mi;//mac object指针
  
  mi = mymalloc( SRAMIN,sizeof( mac_infor_t ) );
  memset( mi, 0, sizeof( mac_infor_t) );//mac object清零
  mi->adapter = adap;
  pAES_Key = aes_key;//config as default aes key
  
  list_add_tail( &(mi->obj.node), &mac_obj_list);
    
  return &(mi->obj);
}

static void mac_radio_deint(  mac_obj_t * obj )
{
     mac_set_cursor_enter( obj );
     mac_phy_radio_deint( );//radio初始化
     mac_set_cursor_exit();
}

/*!
* \brief    mac初始化函数
* \remark   

* \param    [in] mac        mac对象指针
* \param    [in] msg        用户数据指针
* \param    [in] size       用户数据大小
* \param    [in] callback   应用层回调函数 
* \param    [in] lpcb       mac LDC低功耗接收功能的低功耗处理回调函数

* \retval   void     
*/
static void mac_int( void )
{
  INIT_LIST_HEAD(&mac_obj_list);
  
#if MULTI_MODULE_MODE_IS_SEVER
   
   list_creat( &pAckList );//queue for ack
   
#endif
  
  mac_phy_int( mac_task_callback );
}


/*!
* \brief    mac rf参数配置函数
* \remark   

* \param    [in] freq       rf工作频点    
* \param    [in] power      rf 发射功率
* \param    [in] sf         rf扩频因子
* \param    [in] bandwidth  rf带宽
* \param    [in] coderate   rf编码率code rate 
* \param    [in] per        mac LDC低功耗功能cadPreamble period 

* \retval   void     
*/
static void mac_radio_config( mac_obj_t *obj, uint32_t freq, int8_t power,uint32_t sf, uint32_t bandwidth, uint8_t coderate, uint32_t per )
{
 mac_set_cursor_enter( obj );
 obj->period = per;
 mac_phy_radio_config( obj, freq, power, sf,  bandwidth,  coderate, per );
 mac_set_cursor_exit();
}


/*!
* \brief    mac角色配置为sender函数
* \remark   

* \param    [in] mac        mac对象指针
* \param    [in] app_id     应用id
* \param    [in] dev_id     本地设备id 
* \param    [in] ch_id      信道id
* \param    [in] des_id     目标设备id
* \param    [in] ack        ack功能使能控制
* \param    [in] timeout    ack超时时间设置
* \param    [in] repeat     ack重发次数设置
* \param    [in] type       mac数据类型设置 [ MAC_TYPE_DATA:数据， MAC_TYPE_ACK：应答 ]
* \param    [in] encrypt    AES加密使能控制
* \param    [in] key        AES密匙指针

* \retval   void     
*/
static void mac_config_attribute_of_sender( mac_obj_t *obj,
                                 uint32_t app_id, uint32_t dev_id, uint8_t ch_id , uint32_t des_id, 
                                 bool ack, uint32_t timeout, uint8_t repeat,
                                 bool encrypt, const uint8_t *key )                         
{
  obj->mac.mac_head.magic =  MAC_MAGIC_WORD;
  obj->mac.mac_head.ch_id = ch_id;
  //obj->mac.mac_head.role = MAC_SENDER;
  obj->mac.mac_head.ack=ack;
  obj->mac.mac_head.ack_time_out=timeout;
  obj->mac.mac_head.repeats=repeat;
  //obj->mac.mac_head.type=type;
  obj->mac.mac_head.app_id = app_id;
  obj->mac.mac_head.dev_id =dev_id;
  obj->mac.mac_head.des_id = des_id;
  obj->mac.mac_head.seq = 0;
  
  aes_enable_ctr = encrypt;
  if( key )
  {
    pAES_Key = key;
  }
}


/*!
* \brief    mac角色配置为reciver函数
* \remark   

* \param    [in] mac        mac对象指针
* \param    [in] msg        ack数据指针
* \param    [in] len        ack数据长度
* \param    [in] app_id     应用id
* \param    [in] dev_id     本地设备id
* \param    [in] ch_id      信道id
* \param    [in] afilter    应用id过滤使能控制
* \param    [in] decrypt    AES解密使能控制
* \param    [in] key        AES密匙指针

* \retval   void     
*/
static void mac_config_attribute_of_reciver( mac_obj_t *obj, uint8_t *msg, uint8_t len, uint32_t app_id, uint32_t dev_id, uint8_t ch_id, bool  afilter, bool decrypt, const uint8_t *key )                       
{
  obj->ackBuf = msg;
  obj->ackLen = len;
  
  obj->mac_head_reciver.magic =  MAC_MAGIC_WORD;
  obj->mac_head_reciver.ch_id = ch_id;
  //obj->mac.mac_head.role = MAC_RECIVER;
  
  obj->mac_head_reciver.app_id = app_id;
  obj->mac_head_reciver.dev_id =dev_id;
  app_id_filter = afilter; //0:只收和本app_id 一致的数据包  0xFFFF：即使与本身app_id不一致也接收
  
  aes_enable_ctr = decrypt;
  if( key )
  {
    pAES_Key = key;
  }
}


/*!
* \brief    mac发射数据包制作函数
* \remark   

* \param    [in] mac         mac对象指针
* \param    [in] msg        用户数据指针
* \param    [in] len        用户数据长度
* \param    [out] buflen     制作好的数据包长

* \retval   [out]pBuf      制作好的数据包指针     
*/
static uint8_t * mac_txpk_make( mac_obj_t *obj, uint8_t type, uint8_t *msg, uint8_t len, uint16_t *buflen)
{
  uint16_t crc, size;
  uint8_t *pBuf;
 
  if( len > MAC_MAX_LEN )
  {
    return NULL;
  }
  
  if( type == MAC_TYPE_ACK )
  {
     obj->mac.mac_head.role = MAC_RECIVER;
     obj->mac.mac_head.type = MAC_TYPE_ACK;
  }
  else
  {
   obj->mac.mac_head.role = MAC_SENDER;
   obj->mac.mac_head.type = MAC_TYPE_DATA;
  }
  
  size = sizeof( MacHead_t) + sizeof( MacBody_t ) + sizeof( obj->mac.mac_crc ) + len;
  *buflen = size;
  
  obj->mac.mac_body.msg_len = len;
  pBuf = (uint8_t*)mymalloc(SRAMIN,size);//alloc a buf mem
  obj->buf = pBuf;
  obj->len = size;
  
  obj->mac.mac_head.seq++;
  
  crc = RadioCompute_CRC8( (uint8_t*)&(obj->mac), sizeof(MacHead_t)-sizeof( obj->mac.mac_head.head_crc ) );
  obj->mac.mac_head.head_crc = crc;//mac head crc
  
  memcpy( pBuf, (uint8_t *)&(obj->mac), sizeof(MacHead_t) + sizeof(obj->mac.mac_body.msg_len) ); //copy mac head into buf
  if( ( obj->mac.mac_head.type != MAC_TYPE_ACK ) || ( obj->buf ) )
  {
   memcpy( (uint8_t*)&(pBuf[ sizeof(MacHead_t) + sizeof(obj->mac.mac_body.msg_len)] ), msg, len );//copy msg into buf
  }
  
  crc=RadioCompute_CRC8( pBuf, size-sizeof(obj->mac.mac_crc) );//mac crc
  memcpy( &pBuf[size-sizeof(obj->mac.mac_crc)], &crc, sizeof(obj->mac.mac_crc) );//copy mac crc into buf
  if( aes_enable_ctr )
  {
    mac_aes_encrypt( pBuf, *buflen, obj->mac.mac_head.dev_id, obj->mac.mac_head.type, obj->mac.mac_head.seq, pAES_Key );//aes encrypt the buf data
  }
  
 // memcpy(rx_data_buf, pBuf, obj->len);
  return pBuf;
}


/*!
* \brief    mac工作状态设置函数
* \remark   

* \param    [in] s        mac工作状态

* \retval   void          
*/
void mac_states_set( mac_obj_t *obj, States_t s)
{
    obj->st = s;
    
    switch( s )
    {
    case START_TX: 
     
      obj->busy = true;//object radio busy
      
      printf("start tx\r\n");
      
      break;
      
    case START_RX:
      
      if( obj->period )
      {
        obj->st = LDC_START_RX;
      }
      
      break;
      
    case ACK_TX_PREPARE: 
      
      obj->busy = true;
      
      break;
         
    case ACK_START_TX:
      
      mac_lbt( obj );//listen before talk
      obj->st = s;
      
      break;
      
    case ACK_START_RX:
      
      if( obj->period )
      {
        obj->st = LDC_START_RX;
      }
      MultiTimerStart( &pk_ack_timer, obj->mac.mac_head.ack_time_out, mac_ack_timer_callback, obj );//开启ack 接收超时定时器
    
      break;
     
      default: break;
    }
}


/*!
* \brief    mac工作忙状态获取函数
* \remark   

* \param    void

* \retval   void          
*/
static bool mac_busy_get( mac_obj_t * obj )
{
  return obj->busy;
}


/*!
* \brief    mac工作运行函数
* \remark   

* \param    void

* \retval   void          
*/
static void mac_task_running(void)
{
  struct list_head * pos;
  mac_obj_t * mac_obj;
  
  list_for_each( pos,&mac_obj_list )
  {
    mac_obj= list_entry( pos, mac_obj_t, node);
    
    mac_set_cursor_enter( mac_obj );
    
    mac_phy_task_running( mac_obj );
    
    mac_set_cursor_exit();
  }
}


/*!
* \brief    mac LBT信道忙检测函数
* \remark   

* \param    void

* \retval   void          
*/
static void mac_lbt( mac_obj_t *obj )
{
  mac_set_cursor_enter( obj );
  
  mac_phy_cad_check( obj ); //listen before tx task
  
  mac_set_cursor_exit();
}



__weak void mac_ack_start_rx_hook( mac_obj_t *obj )
{
  
//#if MULTI_MODULE_MODE_IS_CLIENT 
//  
//  mac_config_attribute_of_reciver( obj, //mac object pointer
//                NULL, //ack buffer pointer 
//                0, //ack msg len
//                obj->mac.mac_head.app_id, //app_id
//                obj->mac.mac_head.dev_id, //dev_id
//                mac_downlink_channel_id_get( obj ), //ch_id
//                true, //app_id filter
//                true, //aes decrypt enable control
//                pAES_Key);// aes key pointer
//  
//#else
//  
//    mac_config_attribute_of_reciver( obj, //mac object pointer
//                NULL, //ack buffer pointer 
//                0, //ack msg len
//                obj->mac.mac_head.app_id, //app_id
//                obj->mac.mac_head.dev_id, //dev_id
//                0, //ch_id
//                true, //app_id filter
//                true, //aes decrypt enable control
//                pAES_Key);// aes key pointer
//  
// #endif 
    
    //mac_role_as_reciver( obj );
}


__weak void mac_ack_msg_update_hook( mac_obj_t *obj )
{ 
  
}


/*!
* \brief    mac接收数据解析函数
* \remark   

* \param    [in] payload     待解析数据包指针
* \param    [in] size        待解析数据包长度
* \param    [in] rssi        接收到数据包的rssi值[dBm]
* \param    [in] snr         接收到数据包的信噪比
* \param    [in] arg         预留

* \retval   [out]true\false     true:解析成功，数据包合法     false:解析失败，数据包非法     
*/
static bool mac_payload_parse( mac_obj_t *obj, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr, void *arg )
{   
  if( aes_enable_ctr )
  {
   if( mac_aes_decrypt( payload, size, pAES_Key ) ==false ) goto Mac_Fail; //aes fail
  }
  
  uint16_t len;
  uint8_t crc;
  uint16_t mac_crc;
  
  mac_crc = payload[size-1];
  if( RadioCompute_CRC8( payload, size-sizeof( obj->mac.mac_crc) ) != mac_crc ) goto Mac_Fail;  // mac crc check
  
  obj->tempMac = (Mac_t*)payload;
  len = sizeof( MacHead_t) + sizeof( MacBody_t ) + sizeof( obj->mac.mac_crc ) + obj->tempMac->mac_body.msg_len;
  
  if( payload[0]!=MAC_MAGIC_WORD )  goto Mac_Fail; //magic word match fail
  if( payload[1]!=obj->mac.mac_head.ch_id ) goto Mac_Fail; //channel id match fail
  if(size < len) goto Mac_Fail; //payload len fail
  if( ( obj->tempMac->mac_body.msg_len + sizeof( MacHead_t) + sizeof( MacBody_t ) + sizeof( obj->mac.mac_crc ) ) > MAC_MAX_LEN ) goto Mac_Fail;  //msg len fail
  
  crc=RadioCompute_CRC8( payload, sizeof(MacHead_t)-sizeof(obj->tempMac->mac_head.head_crc) );
  if( crc!= obj->tempMac->mac_head.head_crc ) goto Mac_Fail; //head crc error
  
  if(!app_id_filter)
  {//app_id  filter 关闭: 跳过app_id 匹配
    goto App_Id_Match;
  }
  else  
  {//app_id filter 开启 :不接收app广播消息
    if( obj->tempMac->mac_head.app_id != MAC_APP_BROADCAST )
    {//app_id 非广播
      
      if( obj->tempMac->mac_head.app_id == obj->mac.mac_head.app_id )//app_id 匹配
      {
      App_Id_Match:  
        //if(!dev_id_filter)//dev_id filter关闭 : 忽略dev_id ，直接接收dev广播消息与非广播消息
        {
          // goto Mac_Rx_Done;
        }
        
        //dev_id filter开启：dev广播消息直接接收，非dev广播消息匹配接收
        if( obj->tempMac->mac_head.des_id != MAC_DEV_BROADCAST )
        {//dev_id 非广播
          goto Dev_Id_Match;
        }
        else
        {//dev_id 广播
          goto Mac_Rx_Done;
        } 
      }
      else
      {
        mac_states_set( obj, START_RX); 
        return false;
      }
    }
    else
    {//app_id 广播
      
      mac_states_set( obj, START_RX);  
      return false;
    }
  }
  
Dev_Id_Match:    
  if( obj->tempMac->mac_head.des_id == obj->mac.mac_head.dev_id )//dev_id 匹配
  {
    if( (obj->mac.mac_head.role == MAC_SENDER) &&(obj->tempMac->mac_head.type == MAC_TYPE_ACK) )
    {//作为SENDER接收到ACK数据，作为SENDER接收到数据只认类型为MAC_TYPE_ACK的数据包
      
      if( obj->mac.mac_head.ack == true )
      {
        obj->st = ACK_RX_DONE;
        if( obj->tempMac->mac_body.msg_len )
        {
          obj_map_infor(obj)->adapter->appCb( obj, (uint8_t*)&(obj->tempMac->mac_body), obj->tempMac->mac_body.msg_len+1, rssi, snr, NULL );
        }
        else
        {
          obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL ); 
        }
      }
    }
    else if( (obj->mac.mac_head.role == MAC_RECIVER) && (obj->tempMac->mac_head.type == MAC_TYPE_DATA) )
    {//作为RECIVER接收到数据，作为RECIVER只接收类型为MAC_TYPE_DATA的数据包
      
     // if( (obj->tempMac->mac_head.seq != seq) || ( (obj->tempMac->mac_head.seq == 1) && (seq ==1 ) ) )
      { 
       // seq = obj->tempMac->mac_head.seq;
        
        obj_map_infor(obj)->adapter->appCb( obj, (uint8_t*)&(obj->tempMac->mac_body), obj->tempMac->mac_body.msg_len+1, rssi, snr, NULL );
      }
      
      mac_states_set( obj, START_RX);
      
      if( obj->tempMac->mac_head.ack == true )//请求应答
      {
        
#if MULTI_MODULE_MODE_IS_SEVER   
        
//        mac_config_attribute_of_sender( &shadow_obj, downlink_obj->mac.mac_head.app_id, downlink_obj->mac.mac_head.dev_id, downlink_obj->mac.mac_head.ch_id, obj->tempMac->mac_head.dev_id, 
//                             false,0, 0,   //ack disable
//                             MAC_TYPE_ACK, //config as ack data type
//                             aes_enable_ctr, pAES_Key );
        
       // mac_role_as_sender( obj, MAC_TYPE_ACK );
        //(&shadow_obj)->st = ACK_RELOAD;
        //obj_map_infor(obj)->adapter->appCb( &shadow_obj, NULL,NULL, NULL, NULL, NULL );
        mac_ack_msg_update_hook( obj );
        
        if( ((&shadow_obj)->ackBuf) && ( (&shadow_obj)->ackLen ) )
        {
          mac_txpk_make( &shadow_obj, MAC_TYPE_ACK, (&shadow_obj)->ackBuf, (&shadow_obj)->ackLen, &pk_len);//make a ack tx pk  with ack msg 
        }
        else
        {
          mac_txpk_make( &shadow_obj, MAC_TYPE_ACK, NULL, 0, &pk_len);//make a ack tx pk  with no msg 
        }
        
        ListNode_t *node;
        node = list_node_creat( pAckList,(&shadow_obj)->len );
        list_node_data_push( pAckList, node, (&shadow_obj)->buf, (&shadow_obj)->len ); 
        
        mac_states_set( obj, START_RX);
    
#else        
        mac_states_set( obj, ACK_TX_PREPARE );
#endif        
        
      }
    }
    else if( (obj->mac.mac_head.role == MAC_RECIVER) && (obj->tempMac->mac_head.type == MAC_TYPE_ACK) )
    { // 作为RECIVER接收到数据，UP_DOWN_MODE 中作为RECIVER可以接收类型为MAC_TYPE_DATA的数据包，也可以接收类型为MAC_TYPE_ACK的数据包
      
#if MULTI_MODULE_MODE_IS_SEVER
      
      ack_times = 0;
      MultiTimerStop( &pk_ack_timer );//stop ack wait timer
      
      obj->st = ACK_RX_DONE;
      if( obj->tempMac->mac_body.msg_len )//ack包有数据
      {
        obj_map_infor(obj)->adapter->appCb( obj, (uint8_t*)&(obj->tempMac->mac_body), obj->tempMac->mac_body.msg_len+1, rssi, snr, NULL );
      }
      else //ack 包无数据
      {
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL ); 
      }
      
      mac_done_process( mac_downlink_obj_get() );
      mac_states_set( obj, START_RX);
      
#endif
      
#if  MULTI_MODULE_MODE_IS_CLIENT
      
      ack_times = 0;
      MultiTimerStop( &pk_ack_timer );//stop ack wait timer
      
      obj->st = ACK_RX_DONE;
      if( obj->tempMac->mac_body.msg_len )//ack包有数据
      {
        obj_map_infor(obj)->adapter->appCb( obj, (uint8_t*)&(obj->tempMac->mac_body), obj->tempMac->mac_body.msg_len+1, rssi, snr, NULL );
      }
      else //ack 包无数据
      {
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL ); 
      }
      
      mac_done_process( obj );
      mac_states_set( obj, START_RX);
      
      
#endif
      
    }
    else
    {
      goto Mac_Fail;
    }
  }
  else
  {
     goto Mac_Fail;
  }
  
  goto Func_End;
  
  
Mac_Fail:
  
  mac_states_set( obj, START_RX);
  
  return false;
  
Mac_Rx_Done:
  
  obj->st = RX_DONE;
  obj_map_infor(obj)->adapter->appCb( obj, (uint8_t*)&(obj->tempMac->mac_body), obj->tempMac->mac_body.msg_len +1, rssi, snr, NULL );
  
  mac_states_set( obj, START_RX );
  
Func_End:     
  
  return true;
}

/*!
* \brief    mac业务流程结束处理函数
* \remark   

* \param    void

* \retval   void          
*/
static void mac_done_process( mac_obj_t *obj )
{
    obj->busy = false;//object radio free
    myfree(SRAMIN, obj->buf);//mem内存释放
    obj->buf = NULL;
    
#if MULTI_MODULE_MODE_IS_SEVER    
    
    mac_states_set( obj, STAND_BY );
    
#else
    
    mac_states_set( obj, START_RX );
    
#endif
    
}


/*!
* \brief    mac ack数据包等待超时回调函数
* \remark   

* \param    [in]timer       multitimer定时器指针
* \param    [in]userData   指向任意数据类型的指针[预留]

* \retval   void          
*/
static void mac_ack_timer_callback(MultiTimer* timer, void* userData)
{ 
  mac_obj_t *obj = (mac_obj_t *)userData;
  
  if( ack_times < obj->mac.mac_head.repeats )
  {
    ack_times++;
    mac_states_set( obj, START_TX );
    
    printf( "ack timeout\r\n" );
    printf( "retransmit %d ......\r\n",ack_times );
  }
  else
  {
    ack_times = 0;
    mac_states_set( obj, TX_FAIL );
  }
}


/*!
* \brief    mac任务回调函数
* \remark   供mac_phy文件调用

* \param    [in] payload       数据接收指针
* \param    [in]size           接收数据长度
* \param    rssi               接收数据rssi值
* \param    snr                接收数据snr信噪比
* \param    arg                指向任意数据类型的指针[预留]

* \retval   void          
*/
static void mac_task_callback( mac_obj_t *obj, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr,void *arg)
{
  switch( obj->st )
  {
  case TX_DONE:
    
    if( obj->mac.mac_head.role == MAC_SENDER )
    {
      
#if   MULTI_MODULE_MODE_IS_SEVER      
      
      if( ( obj->mac.mac_head.ack == true ) &&( obj->mac.mac_head.des_id !=MAC_DEV_BROADCAST ) &&( obj->mac.mac_head.des_id !=MAC_APP_BROADCAST ) )//请求节点的应答
      {
        MultiTimerStart( &pk_ack_timer, obj->mac.mac_head.ack_time_out, mac_ack_timer_callback, obj );//开启ack 接收超时定时器
        
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL );//通知app发送完成
      }
      else
      {
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL );//通知app发送完成
        mac_done_process( obj );
      }
        
#else     
      
      if( ( obj->mac.mac_head.ack == true ) &&( obj->mac.mac_head.des_id !=MAC_DEV_BROADCAST ) &&( obj->mac.mac_head.des_id !=MAC_APP_BROADCAST ) )//需要应答
      {
        mac_states_set( obj, ACK_START_RX );
        mac_ack_start_rx_hook( obj ); 
        
#if MULTI_MODULE_MODE_IS_CLIENT
  
     mac_radio_channel_set( obj, obj_map_infor(obj)->adapter->updownlink_table.downlink.downlink_ch ); //set radio rx channel

#endif          
        
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL );//通知app发送完成
      }
      else
      {
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL );//通知app发送完成
        mac_done_process( obj );
      }
      
 #endif     
      
    }
   
    break;
    
    
  case RX_DONE:
    
    mac_payload_parse( obj, payload, size, rssi, snr, arg );
    
  break;
    
    
    
  case ACK_TX_PREPARE:

//    mac_config_attribute_of_sender( obj, obj->mac.mac_head.app_id, obj->mac.mac_head.dev_id, obj->mac.mac_head.ch_id, obj->tempMac->mac_head.dev_id, 
//                         false,0, 0, 
//                         MAC_TYPE_ACK, 
//                         aes_enable_ctr, pAES_Key );//config as ack data type
    
  //  mac_role_as_sender( obj, MAC_TYPE_ACK);
    
    if( (obj->ackBuf) && ( obj->ackLen ) )
    {
         mac_txpk_make( obj, MAC_TYPE_ACK, obj->ackBuf, obj->ackLen, &pk_len);//make a ack tx pk  with ack msg 
    }
    else
    {
         mac_txpk_make( obj, MAC_TYPE_ACK,  NULL, 0, &pk_len);//make a ack tx pk  with no msg 
    }
    mac_states_set( obj, ACK_START_TX );
    
    printf("start tx ack\r\n");
    
    break;
    
    
  case ACK_TX_DONE:   //ACK数据包发送完成
     
     mac_done_process( obj );
     printf("tx ack done\r\n");
     
    break;
    
    
  case TX_FAIL:

    obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL );
    
    mac_done_process( obj );
    
    break;
    
    
  case ACK_RX_DONE:
    
    ack_times = 0;
    MultiTimerStop( &pk_ack_timer );//stop ack wait timer
    mac_done_process( obj );

    break;

  default: break;
  }
}