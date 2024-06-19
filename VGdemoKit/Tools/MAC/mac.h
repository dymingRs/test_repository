#ifndef _MAC_H_
#define _MAC_H_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include "list.h"
#include "linux_list.h"

#define MAC_MAGIC_WORD      0x33 

#define MAC_MAX_LEN         253

#define MAC_TYPE_ACK        1u
#define MAC_TYPE_DATA       2u

#define MAC_APP_BROADCAST   0xFFFF
#define MAC_DEV_BROADCAST   0xFFFFFFFF

#define MAC_SENDER          0x55
#define MAC_RECIVER         0xAA

#define DEV_ID_INDEX        10
#define TYPE_INDEX          7
#define SEQ_INDEX           14

#define MAC_FRAME_LEN       ( sizeof(Mac_t) )    

#define MAX_UPLINK_CH       8

#define MULTI_MODULE_MODE_IS_SEVER    0
#define MULTI_MODULE_MODE_IS_CLIENT   1

typedef enum
{
  SET_LOW,
  SET_HIG,
}mac_port_pin_state_t;

typedef enum
{
  STAND_BY,
  SLEEP,
  LOWPOWER,
  
  START_TX,
  TX_RUNNING,
  TX_DONE,
  TX_TIMEOUT,
  TX_FAIL,
  
  START_RX,
  RX_RUNNING,
  RX_DONE,
  RX_TIMEOUT,
  RX_ERROR,
  
  //cad 
  START_CAD,
  CAD_RUNNING,
  CAD_DETED,
  CAD_NDETED,

  // ack tx
  ACK_TX_PREPARE,
  ACK_START_TX,
  ACK_TX_RUNNING,
  ACK_TX_DONE,
  
  //ack rx
  ACK_START_RX,
  ACK_RX_RUNNING,
  ACK_RX_DONE,
  
  //low power duty cycle
  LDC_START_RX,
  LDC_RX_RUNNING,
  LDC_RX_DONE,
  
  ACK_RELOAD,

}States_t;

typedef struct
{   
    uint8_t  magic;          //magic_word£¬can be uesed as customer identify
    uint8_t  ch_id;          //channel id
    uint8_t  role;           //role [ MAC_SENDER, MAC_RECIVER ]
    bool     ack;            //ack enable control
    uint16_t ack_time_out;   //ack wait time out
    uint8_t  repeats;        //ack repeat times
    uint8_t  type;           //data type
    uint16_t app_id;         //aplication id
    uint32_t dev_id;         //dev id
    uint8_t  seq;            //package sequence
    uint32_t des_id;         //destination id
    uint8_t head_crc;        //mac head crc check
}__attribute__((packed)) MacHead_t;

typedef struct
{ 
    uint8_t  msg_len;        //user msg length
}__attribute__((packed)) MacBody_t;


typedef struct
{   
    MacHead_t    mac_head;     //mac head struct  /* Inherit MacHead_t*/  
    MacBody_t    mac_body;     //mac body struct  /* Inherit MacBody_t*/  
    uint8_t      mac_crc;      //mac crc check
    
} __attribute__((packed))Mac_t;


typedef struct
{
  struct list_head    node;
  Mac_t                mac;//Inherit Mac_t
  Mac_t                *tempMac;//temp mac
  MacHead_t            mac_head_reciver;
  uint8_t              role;  
  uint8_t              *buf;//pk for send buf
  uint8_t              len; //pk for send buf len
  uint8_t              *ackBuf;//ack msg buf
  uint8_t              ackLen;//ack msg len
  uint32_t             period;//cadPreamble per
  States_t             st;//work state
  bool                 irq;//rf irq statue
  bool                 busy;
  
} mac_obj_t;


typedef void (*AppCallBack)( mac_obj_t *obj, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr,void *arg );
typedef void (*LpCallBack)( mac_obj_t *obj );

typedef struct 
{
  uint32_t        downlink_ch;
  uint8_t         downlink_id;
  
}downlink_t;

typedef struct 
{
  uint32_t           uplink_ch_table[MAX_UPLINK_CH];
  uint8_t            uplink_id_table[MAX_UPLINK_CH];
  
}uplink_t;


typedef  struct 
{
  downlink_t  downlink;
  uplink_t    uplink;
  
}updownlink_table_t;

typedef struct
{
  void ( *module_nss_ctr )( mac_port_pin_state_t s );//module  nss select
  void ( *module_rst_ctr )( mac_port_pin_state_t s );//module  rst control
  bool ( *module_busy_check)(void);
  void ( *module_irq_polling)( void );//module irq polling 
  
  AppCallBack          appCb;  //app callback
  LpCallBack           appLpCb;//app low power callback
  struct tag                  //object tag
  {
    uint8_t             sn;    //serial number
    char               *name; //name
  };

#if  MULTI_MODULE_MODE_IS_CLIENT 
  
  updownlink_table_t   updownlink_table;
  
#endif
  
} mac_adapter_t;


typedef struct
{ 
    mac_obj_t            obj;//Inherit mac_obj_t
    mac_adapter_t        *adapter;
}mac_infor_t;


typedef struct{
  
  void (*RadioReady)( mac_obj_t *obj);
  void (*RfTest)( mac_obj_t *obj, uint32_t freq, int8_t power );
  void (*RadioDeInt)( mac_obj_t *obj ); 
  void (*RadioConfig)( mac_obj_t * obj, uint32_t freq, int8_t power,uint32_t datarate, uint32_t bandwidth, uint8_t coderate, uint32_t per );
  void (*RadioChannelSet)( mac_obj_t *obj, uint32_t freq );
  void (*RadioPowerSet)( mac_obj_t *obj, int8_t power );
  
#if MULTI_MODULE_MODE_IS_CLIENT
  
  void (*RadioUpDownChTbSet)( mac_obj_t *obj, uint32_t downlink, uint8_t argc, ...);
  void (*RadioUpDownChIdTbSet)( mac_obj_t *obj, uint8_t downlink_ch_id,uint8_t argc, ...);
  void (*RadioUpLinkChSet)( mac_obj_t *obj, uint8_t ch );
  void (*RadioUpLinkChIdSet)( mac_obj_t *obj, uint8_t ch );
  uint32_t (*RadioDownLinkChGet)( mac_obj_t *obj );
  uint32_t (*RadioUpLinkChGet)( mac_obj_t *obj, uint8_t ch );
  uint8_t  (*RadioDownLinkChIdGet)( mac_obj_t *obj );
  uint8_t  (*RadioUpLinkChIdGet)( mac_obj_t *obj, uint8_t ch );
  
#endif
  
}Radio_Adapter_t;
  
  

typedef struct 
{
  
  void (*Int)( void );
  mac_obj_t * (*ObjCreat)( mac_adapter_t *adap );
  void (*AttributeOfSender)( mac_obj_t *obj, uint32_t app_id, uint32_t dev_id, uint8_t ch_id , uint32_t des_id,  bool ack, uint32_t timeout, uint8_t repeat, bool encrypt, const uint8_t *key ); 
  void (*AttributeOfReciver)( mac_obj_t *obj, uint8_t *msg, uint8_t len, uint32_t app_id, uint32_t dev_id, uint8_t ch_id, bool  afilter, bool encrypt, const uint8_t *key );
  void (*AsSender)( mac_obj_t *obj, uint8_t type );
  void (*AsReciver)( mac_obj_t *obj );
  uint8_t *(*PkMake)( mac_obj_t *obj, uint8_t type,  uint8_t *msg, uint8_t len, uint16_t *buflen);
  void (*SSet)( mac_obj_t * obj, States_t s);
  bool (*BusyGet)( mac_obj_t *obj );
  void (*Running)(void);
  
  Radio_Adapter_t  Radio_Adapter;
    
}MacFunction_t;

extern const MacFunction_t Mac;

mac_obj_t * mac_get_cursor( void );
void mac_set_cursor_enter( mac_obj_t *obj );
void mac_set_cursor_exit( void );
mac_infor_t * obj_map_infor( mac_obj_t *obj );
void mac_downlink_obj_set( mac_obj_t *obj );
mac_obj_t * mac_downlink_obj_get( void );
ListNode_t * mac_dwlk_queue_get( void );
ListNode_t * mac_dwlk_RemitList_get( void );
void mac_states_set( mac_obj_t *obj, States_t s);

#endif