#ifndef _AT_H_
#define _AT_H_

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "ATport.h"
#include "malloc.h"

struct at_obj;                      
struct at_adapter;                  
struct at_response;                 

/**
 *@brief AT work running state.
 */
typedef enum {
    AT_WORK_STAT_IDLE,              
    AT_WORK_STAT_READY,             
    AT_WORK_STAT_RUN,               
    AT_WORK_STAT_FINISH,            
    AT_WORK_STAT_ABORT,             
} at_work_state;

/**
 *@brief AT command response code
 */
typedef enum {
    AT_RESP_OK = 0,                 
    AT_RESP_ERROR,                  
    AT_RESP_TIMEOUT,                
    AT_RESP_ABORT                   
} at_resp_code;

/**
 *@brief AT command request priority
 */
typedef enum {
    AT_PRIORITY_LOW = 0,
    AT_PRIORITY_HIGH   
} at_cmd_priority;

/**
 *@brief URC frame receiving status
 */
typedef enum {
    URC_RECV_OK = 0,               /* URC frame received successfully. */
    URC_RECV_TIMEOUT               /* Receive timeout (The frame prefix is matched but the suffix is not matched within AT_URC_TIMEOUT) */
} urc_recv_status;

/**
 * @brief URC frame info.
 */
typedef struct {
    urc_recv_status status;        /* URC frame receiving status.*/
    char *urcbuf;                  /* URC frame buffer*/
    int   urclen;                  /* URC frame buffer length*/
} at_urc_info_t;

/**
 * @brief URC subscription item.
 */
typedef struct {
    const char *prefix;            /* URC frame prefix,such as '+CSQ:'*/
    const char  endmark;           /* URC frame end mark (can only be selected from AT_URC_END_MARKS)*/
    /**
     * @brief   URC handler (triggered when matching prefix and mark are match)
     * @params  info   - URC frame info.
     * @return  Indicates the remaining unreceived bytes of the current URC frame.
     *          @retval 0 Indicates that the current URC frame has been completely received
     *          @retval n It still needs to wait to receive n bytes (AT manager continues 
     *                    to receive the remaining data and continues to call back this interface).
     */    
    int (*handler)(at_urc_info_t *info);
} urc_item_t;

/**
 * @brief AT response information
 */
typedef struct {
    struct at_obj  *obj;            /* AT object*/   
    void           *params;         /* User parameters (referenced from ->at_attr_t.params)*/	
    at_resp_code    code;           /* AT command response code.*/    
	unsigned short  recvcnt;        /* Receive data length*/    
    char           *recvbuf;        /* Receive buffer (raw data)*/
    /* Pointer to the receiving content prefix, valid when code=AT_RESP_OK, 
       if no prefix is specified, it pointer to recvbuf
    */
    char           *prefix;      
    /* Pointer to the receiving content suffix, valid when code=AT_RESP_OK, 
       if no suffix is specified, it pointer to recvbuf
    */
    char           *suffix;
} at_response_t;

/**
 * @brief  The configuration for transparent transmission mode.
 */
typedef struct {
    /**
     * @brief       Exit command(Example:AT+TRANS=0). When this command is matched through the 
     *              read interface, the on_exit event is generated.
     */    
    const char *exit_cmd;
    /**
     * @brief       Exit event, triggered when the exit command is currently matched. At this time, 
     *              you can invoke 'at_raw_transport_exit' to exit the transparent transport mode.
     */ 
    void         (*on_exit)(void);    
    /**
     * @brief       Writting interface for transparent data transmission
     * @param       buf   Data buffer
     * @param       len    data length
     * @return      Indicates the length of the written data
     */
    unsigned int (*write)(const void *buf, unsigned int len);
    /**
     * @brief       Reading interface for transparent data transmission
     * @param       buf   Data buffer
     * @param       len    data length
     * @return      The length of the data actually read
     */    
    unsigned int (*read) (void *buf, unsigned int len);
} at_raw_trans_conf_t;

/**
 * @brief AT interface adapter
 */
typedef struct  {
    //Lock, used in OS environment, fill in NULL if not required.
    void (*lock)(void);
    //Unlock, used in OS environment, fill in NULL if not required.
    void (*unlock)(void);
    /**
     * @brief       Data write operation (non-blocking)
     * @param       buf   Data buffer
     * @param       len    data length
     * @return      Indicates the length of the written data
     */    
    unsigned int (*write)(const void *buf, unsigned int len); 
    /**
     * @brief       Data read operation (non-blocking)
     * @param       buf   Data buffer
     * @param       len    data length
     * @return      The length of the data actually read
     */        
    unsigned int (*read)(void *buf);       
    /**
     * @brief       AT error event ( if not required, fill in NULL)
     */    
    void (*error)(at_response_t *);
    /**
     * @brief       Log output interface, which can print the complete AT interaction 
     *              process, fill in NULL if not required.
     */      
    void (*debug)(const char *fmt, ...);  
#if AT_URC_WARCH_EN
    //URC buffer size, set according to the actual maximum URC frame when used.
	unsigned short urc_bufsize;
#endif    
    //Command response receiving buffer size, set according to the actual maximum command response length
    unsigned short recv_bufsize;
} at_adapter_t;

/**
 * @brief Public environment for AT work 
 */
typedef struct at_env {
    struct at_obj *obj;
    //Public variables (add as needed), these values are reset every time a new work starts.
    int i, j, state;     
    //User parameters (referenced from ->at_attr_t.params)
    void        *params;
    //Set the next polling wait interval (only takes effect once)
    void        (*next_wait)(struct at_env *self, unsigned int ms);
    //Reset timer
    void        (*reset_timer)(struct at_env *self);               
    //Timeout indication
    bool        (*is_timeout)(struct at_env *self, unsigned int ms);
    //Formatted printout with newlines
    void        (*println)(struct at_env *self, const char *fmt, ...);
    //Find a keyword from the received content      
    char *      (*contains)(struct at_env *self, const char *str); 
    //Get receives buffer       
    char *      (*recvbuf)(struct at_env *self);                  
    //Get receives buffer length
    unsigned int(*recvlen)(struct at_env *self);                  
    //Clear the receives buffer 
    void        (*recvclr)(struct at_env *self);
    //Indicates whether the current work has been abort
    bool        (*disposing)(struct at_env *self);
    //End the work and set the response code
    void        (*finish)(struct at_env *self, at_resp_code code);
} at_env_t;

/**
 *@brief AT execution callback
 *@param r   AT response information (including execution results, content information returned by the device)
 */
typedef void (*at_callback_t)(at_response_t *r);            

/**
 *@brief   AT work polling handler
 *@param   env  The public operating environment for AT work, including some common 
 *              variables and relevant interfaces needed to communicate AT commands.
 *@return  Work processing status, which determines whether to continue running the 
 *         work on the next polling cycle.
 * 
 *     @retval true  Indicates that the current work processing has been finished, 
 *                   and the work response code is set to AT_RESP_OK
 *     @retval false Indicates unfinished work processing, keep running.
 *@note   It must be noted that if the env->finish() operation is invoked in the 
 *        current work, the work will be forcibly terminated regardless of the return value.
 */
typedef int  (*at_work_t)(at_env_t *env);

#if AT_WORK_CONTEXT_EN

/**
 *@brief AT work item context (used to monitor the entire life cycle of AT work item)
 */
typedef struct {
    at_work_state   work_state;   /* Indicates the state at which the AT work item is running. */
    at_resp_code    code;         /* Indicates the response code after the AT command has been run.*/    
    unsigned short  bufsize;      /* Indicates receive buffer size*/
    unsigned short  resplen;      /* Indicates the actual response valid data length*/
    unsigned char   *respbuf;     /* Point to the receive buffer*/
} at_context_t;

#endif

/**
 *@brief AT attributes
 */
typedef struct {
#if AT_WORK_CONTEXT_EN
    at_context_t  *ctx;          /* Pointer to the Work context. */
#endif    
    void          *params;       /* User parameter, fill in NULL if not required. */
    const char    *prefix;       /* Response prefix, fill in NULL if not required. */ 
    const char    *suffix;       /* Response suffix, fill in NULL if not required. */
    at_callback_t  cb;           /* Response callback handler, Fill in NULL if not required. */
    unsigned short timeout;      /* Response timeout(ms).. */
    unsigned char  retry;        /* Response error retries. */
    at_cmd_priority priority;    /* Command execution priority. */
} at_attr_t;

/**
 *@brief AT object.
 */
typedef struct at_obj {
    const at_adapter_t *adap;    
//#if AT_RAW_TRANSPARENT_EN    
    const at_raw_trans_conf_t *raw_conf;  
//#endif    
    void  *user_data;                                                 
} at_obj_t;

/**AT work type (corresponding to different state machine polling handler.) */
typedef enum {
    WORK_TYPE_GENERAL = 0,             /* General work */
    WORK_TYPE_SINGLLINE,               /* Singlline command */
    WORK_TYPE_MULTILINE,               /* Multiline command */        
    WORK_TYPE_CMD,                     /* Standard command */    
    WORK_TYPE_CUSTOM,                  /* Custom command */
    WORK_TYPE_BUF,                     /* Buffer */
    WORK_TYPE_MAX
} work_type;

/**
 * @brief AT command execution state.
 */
typedef enum {
    AT_STAT_SEND = 0,
    AT_STAT_RECV,
    AT_STAT_RETRY,
} at_cmd_state;

/**
 * @brief AT receive match mask
 */
#define MATCH_MASK_PREFIX 0x01         
#define MATCH_MASK_SUFFIX 0x02         
#define MATCH_MASK_ERROR  0x04         

/**
 * @brief AT work item object
 */
typedef struct {
    //struct list_head  node;            /* list node */
    at_attr_t         attr;            /* AT attributes */
   // unsigned int      magic : 16;      /* AT magic*/
    unsigned int      state : 3;       /* State of work */
    unsigned int      type  : 3;       /* Type of work */
    unsigned int      code  : 3;       /* Response code*/
    unsigned int      life  : 6;       /* Life cycle countdown(s)*/
    unsigned int      dirty : 1;       /* Dirty flag*/
    union {
        const void *info;
        at_work_t work;                /* Custom work */
        const char * singlline;        
        const char **multiline;        
        void (*sender)(at_env_t *env); /* Custom sender */
        struct {
            unsigned int bufsize;      
            char buf[0];               
        };
    };
} work_item_t;

/**
 * @brief AT Object infomation.
 */
typedef struct {
    at_obj_t          obj;              /* Inherit at_obj*/   
    at_env_t          env;              /* Public work environment*/
    work_item_t      *item;           /* Currently running work*/
   // struct list_head  hlist, llist;     /* High and low priority queue*/
  //  struct list_head *clist;            /* Queue currently in use*/
    unsigned int      timer;            /* General purpose timer*/   
    unsigned int      next_delay;       /* Next cycle delay time*/
    unsigned int      delay_timer;      /* Delay timer*/
    char             *recvbuf;          /* Command response receive buffer*/
    char             *prefix;           /* Point to prefix match*/
    char             *suffix;           /* Point to suffix match*/
#if AT_URC_WARCH_EN    
    const urc_item_t *urc_tbl;
    const urc_item_t *urc_item;         /* The currently matched URC item*/
    char             *urcbuf;           
    unsigned int      urc_timer;        
    unsigned short    urc_bufsize;      
    unsigned short    urc_cnt;          
    unsigned short    urc_target;       /* The target data length of the current URC frame*/
    unsigned short    urc_tbl_size;    
    unsigned short    urc_disable_time;     
#endif    
    unsigned short    list_cnt;         
    unsigned short    recv_bufsize;     
    unsigned short    recv_cnt;         /* Command response receives counter*/
    unsigned short    match_len;        /* Response information matching length*/
    unsigned char     match_mask;       /* Response information matching mask*/
    unsigned          urc_enable: 1;    
    unsigned          urc_match : 1;    
    unsigned          enable    : 1;    /* Enable the work */
    unsigned          disposing : 1;    
    unsigned          err_occur : 1;    
    unsigned          raw_trans : 1;
} at_info_t;

at_obj_t *at_obj_create(const at_adapter_t *adap);
void at_obj_destroy(at_obj_t *obj);
void at_obj_set_urc(at_obj_t *at, const urc_item_t *tbl, int count);
bool at_custom_cmd(at_obj_t *at, const at_attr_t *attr, void (*sender)(at_env_t *env));
bool at_send_singlline(at_obj_t *at, const at_attr_t *attr, const char *cmd);
bool at_send_data(at_obj_t *at, const at_attr_t *attr, const void *databuf, unsigned int bufsize);
bool at_do_work(at_obj_t *at, void *params, at_work_t work);
void at_attr_deinit(at_attr_t *attr);
void at_obj_process(at_obj_t *at);
bool  at_work_is_busy( at_obj_t *at );


#endif
