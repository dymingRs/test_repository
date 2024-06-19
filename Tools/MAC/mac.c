/*!
 * \file      mac.c
 *
 * \brief    �û���������о��˽�RF�ײ㡣ͨ��mac����Э��㣬Ӧ�ò�ͨ���򵥵����ü���������ʵ����Ҫ��ͨѶЧ����Э����߼����ظ����ã��Բ�Ʒ���ȶ�������ĿЧ�ʵ������кܴ����
 *            
 * ###################################### features #############################
 *          1������ͨѶģʽ������ͨѶ���㲥ͨѶ�������Զ�Ӧ��with no msg\with msg��
 *          2������֧�ֹ��Ľ��գ��͹��Ľ���LDCģʽ��low power duty cycle mode��
 *          3������ͬƵ���ţ��ŵ�æ���BLT ��listen before transmit��
 *          4������ȫ��AES128����
 *          5)���첽�ص���ʽ����mac����
  ==============================================================================
 * \copyright dyming
 *
 * \code  
 *  mac��Ƶ��SPIͨѶ�뵥�ز����Ժ���:    static void mac_rf_test( uint32_t freq, int8_t power );
 *  mac���󴴽�����:           static Mac_t  * mac_creat( mac_item_t it );
 *  mac��ʼ������:             static void mac_int( Mac_t *mac, uint8_t *msg, uint16_t size, AppCallBack callback, LpCallBack lpcb );
 *  mac rf�������ú�����       static void mac_radio_config( Mac_t *mac, uint32_t freq, int8_t power,uint32_t sf, uint32_t bandwidth, uint8_t coderate, uint32_t per );
 *  mac��ɫ����Ϊsender����:   static void mac_config_as_sender( Mac_t *mac, uint32_t app_id, uint32_t dev_id, uint8_t ch_id , uint32_t des_id,  bool ack,
                                                                 uint32_t timeout, uint8_t repeat, uint8_t type, bool encrypt, const uint8_t *key );
 *  mac��ɫ����Ϊreciver����:  static void mac_config_as_reciver( Mac_t *mac, uint8_t *msg, uint8_t len, uint32_t app_id, uint32_t dev_id, uint8_t ch_id, bool  afilter, 
                                                                  bool decrypt, const uint8_t *key );
 *  mac�������ݰ���������:     static uint8_t * mac_txpk_make( Mac_t *mac, uint8_t *msg, uint8_t len ,uint16_t *buflen);
 *  mac����״̬���ú���:       static void mac_states_set(States_t s);
 *  mac����æ״̬��ȡ������    static bool mac_busy_get(void);
 *  mac�������к���:           static void mac_abstract_task_running(void);
 *  mac LBT�ŵ�æ��⺯��:     static void mac_lbt( void );
 *  mac�������ݽ�������:       static bool mac_payload_parse(Mac_t *mac, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr, void *arg);
 *  macҵ�����̽�����������  static void mac_done_process( void );
 *  mac ack���ݰ��ȴ���ʱ�ص�����: static void mac_ack_timer_callback(MultiTimer* timer, void* userData);
 *  mac����ص�����:           static void mac_task_callback(Mac_t *mac,uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr,void *arg);
 *
 * (C)2023
 * ###################################### How to use this driver ###############
  ==============================================================================
 * ��ΪSENDER
 * 1��������mac����Mac.Creat();
 * 2��������rf����������Mac.RadioConfig();
 * 3����������Ҫ��mac��ɫ��Mac.AsSender();
 * 4)������mac�������ݰ���Mac.PkMake();
 * 5)����ʼ��mac��Mac.Int();
 * 6)������macΪstart_tx״̬��Mac.SSet(START_TX);

 * ��ΪREVICER
 * 1��������mac����Mac.Creat();
 * 2��������rf����������Mac.RadioConfig();
 * 3����������Ҫ��mac��ɫ��Mac.AsReciver();
 * 4 )����ʼ��mac��Mac.Int();
 * 5)������macΪstart_rx״̬��Mac.SSet( START_RX );
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
static const uint8_t       aes_key[16] = {0x31,0x90,0x22,0x58,0x49,0xc4,0xd0,0x79,0x08,0x56,0xb7,0xbb,0xcc,0xd0,0xee,0x0f}; //aes�ܳ�
static mac_obj_t            *obj_cursor; 
static struct list_head    mac_obj_list; 
static ListNode_t           *pAckList=NULL; 
static mac_obj_t            shadow_obj;  
static mac_obj_t            *downlink_obj;  

/*!
* \brief    mac�����ṹ��ʵ��
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
* \brief    mac��Ƶ��SPIͨѶ�뵥�ز����Ժ���
* \remark   
*          
* \param    [in] freq       Ƶ��
* \param    [in] power      ���书��   
* \retval   void
*/
static void mac_rf_test( mac_obj_t *obj, uint32_t freq, int8_t power )
{
    mac_set_cursor_enter( obj );
    mac_phy_radio_test( freq, power );
    mac_set_cursor_exit();
}


/*!
* \brief    mac object��������
* \remark   
*          
* \param    void   
* \retval   [out] pMacObj       mac����ָ��
*/
static mac_obj_t * mac_creat( mac_adapter_t *adap)
{
  mac_infor_t *  mi;//mac objectָ��
  
  mi = mymalloc( SRAMIN,sizeof( mac_infor_t ) );
  memset( mi, 0, sizeof( mac_infor_t) );//mac object����
  mi->adapter = adap;
  pAES_Key = aes_key;//config as default aes key
  
  list_add_tail( &(mi->obj.node), &mac_obj_list);
    
  return &(mi->obj);
}

static void mac_radio_deint(  mac_obj_t * obj )
{
     mac_set_cursor_enter( obj );
     mac_phy_radio_deint( );//radio��ʼ��
     mac_set_cursor_exit();
}

/*!
* \brief    mac��ʼ������
* \remark   

* \param    [in] mac        mac����ָ��
* \param    [in] msg        �û�����ָ��
* \param    [in] size       �û����ݴ�С
* \param    [in] callback   Ӧ�ò�ص����� 
* \param    [in] lpcb       mac LDC�͹��Ľ��չ��ܵĵ͹��Ĵ���ص�����

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
* \brief    mac rf�������ú���
* \remark   

* \param    [in] freq       rf����Ƶ��    
* \param    [in] power      rf ���书��
* \param    [in] sf         rf��Ƶ����
* \param    [in] bandwidth  rf����
* \param    [in] coderate   rf������code rate 
* \param    [in] per        mac LDC�͹��Ĺ���cadPreamble period 

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
* \brief    mac��ɫ����Ϊsender����
* \remark   

* \param    [in] mac        mac����ָ��
* \param    [in] app_id     Ӧ��id
* \param    [in] dev_id     �����豸id 
* \param    [in] ch_id      �ŵ�id
* \param    [in] des_id     Ŀ���豸id
* \param    [in] ack        ack����ʹ�ܿ���
* \param    [in] timeout    ack��ʱʱ������
* \param    [in] repeat     ack�ط���������
* \param    [in] type       mac������������ [ MAC_TYPE_DATA:���ݣ� MAC_TYPE_ACK��Ӧ�� ]
* \param    [in] encrypt    AES����ʹ�ܿ���
* \param    [in] key        AES�ܳ�ָ��

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
* \brief    mac��ɫ����Ϊreciver����
* \remark   

* \param    [in] mac        mac����ָ��
* \param    [in] msg        ack����ָ��
* \param    [in] len        ack���ݳ���
* \param    [in] app_id     Ӧ��id
* \param    [in] dev_id     �����豸id
* \param    [in] ch_id      �ŵ�id
* \param    [in] afilter    Ӧ��id����ʹ�ܿ���
* \param    [in] decrypt    AES����ʹ�ܿ���
* \param    [in] key        AES�ܳ�ָ��

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
  app_id_filter = afilter; //0:ֻ�պͱ�app_id һ�µ����ݰ�  0xFFFF����ʹ�뱾��app_id��һ��Ҳ����
  
  aes_enable_ctr = decrypt;
  if( key )
  {
    pAES_Key = key;
  }
}


/*!
* \brief    mac�������ݰ���������
* \remark   

* \param    [in] mac         mac����ָ��
* \param    [in] msg        �û�����ָ��
* \param    [in] len        �û����ݳ���
* \param    [out] buflen     �����õ����ݰ���

* \retval   [out]pBuf      �����õ����ݰ�ָ��     
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
* \brief    mac����״̬���ú���
* \remark   

* \param    [in] s        mac����״̬

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
      MultiTimerStart( &pk_ack_timer, obj->mac.mac_head.ack_time_out, mac_ack_timer_callback, obj );//����ack ���ճ�ʱ��ʱ��
    
      break;
     
      default: break;
    }
}


/*!
* \brief    mac����æ״̬��ȡ����
* \remark   

* \param    void

* \retval   void          
*/
static bool mac_busy_get( mac_obj_t * obj )
{
  return obj->busy;
}


/*!
* \brief    mac�������к���
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
* \brief    mac LBT�ŵ�æ��⺯��
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
* \brief    mac�������ݽ�������
* \remark   

* \param    [in] payload     ���������ݰ�ָ��
* \param    [in] size        ���������ݰ�����
* \param    [in] rssi        ���յ����ݰ���rssiֵ[dBm]
* \param    [in] snr         ���յ����ݰ��������
* \param    [in] arg         Ԥ��

* \retval   [out]true\false     true:�����ɹ������ݰ��Ϸ�     false:����ʧ�ܣ����ݰ��Ƿ�     
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
  {//app_id  filter �ر�: ����app_id ƥ��
    goto App_Id_Match;
  }
  else  
  {//app_id filter ���� :������app�㲥��Ϣ
    if( obj->tempMac->mac_head.app_id != MAC_APP_BROADCAST )
    {//app_id �ǹ㲥
      
      if( obj->tempMac->mac_head.app_id == obj->mac.mac_head.app_id )//app_id ƥ��
      {
      App_Id_Match:  
        //if(!dev_id_filter)//dev_id filter�ر� : ����dev_id ��ֱ�ӽ���dev�㲥��Ϣ��ǹ㲥��Ϣ
        {
          // goto Mac_Rx_Done;
        }
        
        //dev_id filter������dev�㲥��Ϣֱ�ӽ��գ���dev�㲥��Ϣƥ�����
        if( obj->tempMac->mac_head.des_id != MAC_DEV_BROADCAST )
        {//dev_id �ǹ㲥
          goto Dev_Id_Match;
        }
        else
        {//dev_id �㲥
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
    {//app_id �㲥
      
      mac_states_set( obj, START_RX);  
      return false;
    }
  }
  
Dev_Id_Match:    
  if( obj->tempMac->mac_head.des_id == obj->mac.mac_head.dev_id )//dev_id ƥ��
  {
    if( (obj->mac.mac_head.role == MAC_SENDER) &&(obj->tempMac->mac_head.type == MAC_TYPE_ACK) )
    {//��ΪSENDER���յ�ACK���ݣ���ΪSENDER���յ�����ֻ������ΪMAC_TYPE_ACK�����ݰ�
      
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
    {//��ΪRECIVER���յ����ݣ���ΪRECIVERֻ��������ΪMAC_TYPE_DATA�����ݰ�
      
     // if( (obj->tempMac->mac_head.seq != seq) || ( (obj->tempMac->mac_head.seq == 1) && (seq ==1 ) ) )
      { 
       // seq = obj->tempMac->mac_head.seq;
        
        obj_map_infor(obj)->adapter->appCb( obj, (uint8_t*)&(obj->tempMac->mac_body), obj->tempMac->mac_body.msg_len+1, rssi, snr, NULL );
      }
      
      mac_states_set( obj, START_RX);
      
      if( obj->tempMac->mac_head.ack == true )//����Ӧ��
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
    { // ��ΪRECIVER���յ����ݣ�UP_DOWN_MODE ����ΪRECIVER���Խ�������ΪMAC_TYPE_DATA�����ݰ���Ҳ���Խ�������ΪMAC_TYPE_ACK�����ݰ�
      
#if MULTI_MODULE_MODE_IS_SEVER
      
      ack_times = 0;
      MultiTimerStop( &pk_ack_timer );//stop ack wait timer
      
      obj->st = ACK_RX_DONE;
      if( obj->tempMac->mac_body.msg_len )//ack��������
      {
        obj_map_infor(obj)->adapter->appCb( obj, (uint8_t*)&(obj->tempMac->mac_body), obj->tempMac->mac_body.msg_len+1, rssi, snr, NULL );
      }
      else //ack ��������
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
      if( obj->tempMac->mac_body.msg_len )//ack��������
      {
        obj_map_infor(obj)->adapter->appCb( obj, (uint8_t*)&(obj->tempMac->mac_body), obj->tempMac->mac_body.msg_len+1, rssi, snr, NULL );
      }
      else //ack ��������
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
* \brief    macҵ�����̽���������
* \remark   

* \param    void

* \retval   void          
*/
static void mac_done_process( mac_obj_t *obj )
{
    obj->busy = false;//object radio free
    myfree(SRAMIN, obj->buf);//mem�ڴ��ͷ�
    obj->buf = NULL;
    
#if MULTI_MODULE_MODE_IS_SEVER    
    
    mac_states_set( obj, STAND_BY );
    
#else
    
    mac_states_set( obj, START_RX );
    
#endif
    
}


/*!
* \brief    mac ack���ݰ��ȴ���ʱ�ص�����
* \remark   

* \param    [in]timer       multitimer��ʱ��ָ��
* \param    [in]userData   ָ�������������͵�ָ��[Ԥ��]

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
* \brief    mac����ص�����
* \remark   ��mac_phy�ļ�����

* \param    [in] payload       ���ݽ���ָ��
* \param    [in]size           �������ݳ���
* \param    rssi               ��������rssiֵ
* \param    snr                ��������snr�����
* \param    arg                ָ�������������͵�ָ��[Ԥ��]

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
      
      if( ( obj->mac.mac_head.ack == true ) &&( obj->mac.mac_head.des_id !=MAC_DEV_BROADCAST ) &&( obj->mac.mac_head.des_id !=MAC_APP_BROADCAST ) )//����ڵ��Ӧ��
      {
        MultiTimerStart( &pk_ack_timer, obj->mac.mac_head.ack_time_out, mac_ack_timer_callback, obj );//����ack ���ճ�ʱ��ʱ��
        
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL );//֪ͨapp�������
      }
      else
      {
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL );//֪ͨapp�������
        mac_done_process( obj );
      }
        
#else     
      
      if( ( obj->mac.mac_head.ack == true ) &&( obj->mac.mac_head.des_id !=MAC_DEV_BROADCAST ) &&( obj->mac.mac_head.des_id !=MAC_APP_BROADCAST ) )//��ҪӦ��
      {
        mac_states_set( obj, ACK_START_RX );
        mac_ack_start_rx_hook( obj ); 
        
#if MULTI_MODULE_MODE_IS_CLIENT
  
     mac_radio_channel_set( obj, obj_map_infor(obj)->adapter->updownlink_table.downlink.downlink_ch ); //set radio rx channel

#endif          
        
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL );//֪ͨapp�������
      }
      else
      {
        obj_map_infor(obj)->adapter->appCb( obj, NULL, NULL, NULL, NULL, NULL );//֪ͨapp�������
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
    
    
  case ACK_TX_DONE:   //ACK���ݰ��������
     
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