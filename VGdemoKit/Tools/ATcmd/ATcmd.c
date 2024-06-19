#include "ATcmd.h"   

static int do_work_handler(at_info_t *ai);
static int do_cmd_handler(at_info_t *ai);
static work_item_t *create_work_item(at_info_t *ai, int type, const at_attr_t *attr, const void *info, int extend_size);
static void resp_recv_process(at_info_t *ai, const char *buf, unsigned int size);
static void urc_recv_process(at_info_t *ai, char *buf, unsigned int size);
static inline at_info_t *obj_map(at_obj_t *at);
static void update_work_state(work_item_t *wi, at_work_state state, at_resp_code code);
static void at_work_process(at_info_t *ai);
static void at_raw_trans_process(at_obj_t *obj);
static inline const  at_adapter_t *__get_adapter(at_info_t *ai);
static void println(at_env_t *env, const char *cmd, ...);
static inline void send_data(at_info_t *at, const void *buf, unsigned int len);
static void send_cmdline(at_info_t *at, const char *cmd);
static bool at_is_timeout(at_env_t *env, unsigned int ms);
static void at_reset_timer(at_env_t *env);
static void recvbuf_clear(at_env_t *env);
static void urc_timeout_process(at_info_t *ai);
static unsigned int get_recv_count(at_env_t *env);
static char *get_recvbuf(at_env_t *env);
static bool at_isabort(at_env_t *env);
static void at_finish(struct at_env *env, at_resp_code code);
static void at_send_line(at_info_t *ai, const char *fmt, va_list args);
static void urc_handler_entry(at_info_t *ai, urc_recv_status status, char *urc, unsigned int size);
static void work_item_destroy(work_item_t *it);
static void work_item_recycle(at_info_t *ai, work_item_t *it);
static const urc_item_t *find_urc_item(at_info_t *ai, char *urc_buf, unsigned int size);

static uint16_t use_rate;

void at_obj_destroy(at_obj_t *obj);

/**
 * @brief AT receive match mask
 */
#define MATCH_MASK_PREFIX 0x01         
#define MATCH_MASK_SUFFIX 0x02         
#define MATCH_MASK_ERROR  0x04   


#define AT_DEBUG(ai, fmt, args...)                 \
    do                                             \
    {                                              \
        if (__get_adapter(ai)->debug)              \
            __get_adapter(ai)->debug(fmt, ##args); \
    } while (0)
 
#define AT_IS_TIMEOUT(start, time) (at_get_ms() - (start) > (time))
      

/**
 * @brief  Default command attributes.
 */
static const at_attr_t at_def_attr = {
    .params = NULL,    
    .prefix = NULL,
    .suffix = AT_DEF_RESP_OK,
    .cb     = NULL,
    .timeout= AT_DEF_TIMEOUT,
    .retry  = AT_DEF_RETRY,
};


static int (*const work_handler_table[WORK_TYPE_MAX])(at_info_t *) = {
   // [WORK_TYPE_GENERAL]   = do_work_handler,
    [WORK_TYPE_SINGLLINE] = do_cmd_handler,
   // [WORK_TYPE_MULTILINE] = send_multiline_handler,
   // [WORK_TYPE_CMD]       = do_cmd_handler,
    [WORK_TYPE_CUSTOM]    = do_cmd_handler,
    [WORK_TYPE_BUF]       = do_cmd_handler,
};




/**
 * @brief Custom work processing 
 */
//static int do_work_handler(at_info_t *ai)
//{
//    work_item_t *i = ai->item;
//    if (ai->next_delay > 0) {
//        if (!AT_IS_TIMEOUT(ai->delay_timer, ai->next_delay))
//            return 0;
//        ai->next_delay = 0;
//    }    
//    return ((int (*)(at_env_t * e)) i->work)(&ai->env);
//}


/**
 * @brief  at_obj_t * -> at_info_t *
 */
static inline at_info_t *obj_map(at_obj_t *at)
{
    return (at_info_t *)at;
}

/**
 * @brief   Send command with newline.
 */
static void send_cmdline(at_info_t *at, const char *cmd)
{
    int len;
    if (cmd == NULL)
        return;
    len = strlen(cmd);
    __get_adapter(at)->write(cmd, len);
    __get_adapter(at)->write("\r\n", 2);
    AT_DEBUG(at,"->\r\n%s\r\n", cmd);
}


/**
 * @brief   Indication timeout
 */
static bool at_is_timeout(at_env_t *env, unsigned int ms)
{
    return AT_IS_TIMEOUT(obj_map(env->obj)->timer, ms);
}

/**
 * @brief  Reset the currently working timer.
 */
static void at_reset_timer(at_env_t *env)
{
    obj_map(env->obj)->timer = at_get_ms();
}



/**
 * @brief       Send command line
 */
static void at_send_line(at_info_t *ai, const char *fmt, va_list args)
{
    int len;
    char *cmdline;
    cmdline = at_malloc(AT_MAX_CMD_LEN);
    if (cmdline == NULL) {
        AT_DEBUG(ai, "Malloc failed when send...\r\n");
        return;
    }
    len = vsnprintf(cmdline, AT_MAX_CMD_LEN, fmt, args);
    //Clear receive buffer.
    ai->recv_cnt = 0;
    ai->recvbuf[0] = '\0';
    send_data(ai, cmdline, len);
    send_data(ai, "\r\n", 2);
    AT_DEBUG(ai,"->\r\n%s\r\n", cmdline);

    at_free(cmdline);
}


/**
 * @brief Formatted print with newline.
 */
static void println(at_env_t *env, const char *cmd, ...)
{
    va_list args;
    va_start(args, cmd);
    at_send_line(obj_map(env->obj), cmd, args);
    va_end(args);
}

static unsigned int get_recv_count(at_env_t *env)
{
    return obj_map(env->obj)->recv_cnt;
}

static char *get_recvbuf(at_env_t *env)
{
    return obj_map(env->obj)->recvbuf;
}

static void recvbuf_clear(at_env_t *env)
{
    obj_map(env->obj)->recv_cnt = 0;
}


static inline void send_data(at_info_t *at, const void *buf, unsigned int len)
{
    __get_adapter(at)->write(buf, len);
}

/**
 * @brief   Indicates whether the current work was abort.
 */
static bool at_isabort(at_env_t *env)
{
    at_info_t *ai = obj_map(env->obj);
    if (ai->item == NULL)
        return true;
    return ai->item->state == AT_WORK_STAT_ABORT;
}

/**
 * @brief   End the currently running work.
 */
static void at_finish(struct at_env *env, at_resp_code code)
{
    work_item_t *it = obj_map(env->obj)->item;
    update_work_state(it, AT_WORK_STAT_FINISH, code);
}


/**
 * @brief  Create a basic work item.
 */
static work_item_t *work_item_create(int extend_size)
{
    work_item_t *it;
    it = at_malloc(sizeof(work_item_t) + extend_size);   
    if (it != NULL)
        memset(it, 0, sizeof(work_item_t) + extend_size);
    return it;
}


/**
 * @brief  Create and initialize a work item.
 * @param  type  Type of work item.
 * @param  attr  Item attributes
 * @param  info  additional information.
 * @param  size  the extended size.
 */
static work_item_t *create_work_item(at_info_t *ai, int type, const at_attr_t *attr, const void *info, int extend_size)
{
  work_item_t *it = work_item_create( extend_size );
  if (it == NULL) {
    AT_DEBUG(ai, "Insufficient memory, list count:%d\r\n", ai->list_cnt);
    return NULL;
  }
  if (attr == NULL)
    attr = &at_def_attr;
  
    if (type == WORK_TYPE_CMD || type == WORK_TYPE_BUF) {
        memcpy(it->buf, info, extend_size);
        it->bufsize = extend_size;
    } else {
        it->info = info;
    }        
    it->attr = *attr;
    it->type = type;
    it->state = AT_WORK_STAT_READY;
//#if AT_WORK_CONTEXT_EN    
//    if (attr->ctx) {
//        attr->ctx->code = AT_RESP_OK;
//        attr->ctx->work_state = AT_WORK_STAT_READY;
//    }
//#endif    
    
    at_env_t *env = &ai->env;
    if (ai->item == NULL) {
        //if (!list_empty(&ai->hlist))
        //    ai->clist = &ai->hlist;
       // else if (!list_empty(&ai->llist))
      //      ai->clist = &ai->llist;
      //  else
       //     return; //No work to do.

       // at_lock(ai);
        ai->next_delay = 0;
        env->obj    = (struct at_obj *)ai;
        env->i      = 0;
        env->j      = 0;
        env->state  = 0;
       // ai->item  = list_first_entry(ai->clist, work_item_t, node);
        ai->item =it;
        env->params = ai->item->attr.params;
        env->recvclr(env);
        env->reset_timer(env);
      //  at_unlock(ai);
    }
    else
    {
     return NULL;
    }
  
    
  return it;    
}



static void update_work_state(work_item_t *wi, at_work_state state, at_resp_code code)
{
    wi->state = state;
    wi->code  = code;
#if AT_WORK_CONTEXT_EN
    at_context_t  *ctx = wi->attr.ctx;
    if (ctx != NULL) {
        ctx->code       = (at_resp_code)wi->code;
        ctx->work_state = (at_work_state)wi->state;
    }
#endif
}


/**
 * @brief  Destroy work item.
 * @param  it Pointer to an item to destroy.
 */
static void work_item_destroy(work_item_t *it)
{
    if (it != NULL) {
        //it->magic = 0;
        at_free(it);
    }
}


/**
 * @brief  Work item recycling.
 */
static void work_item_recycle(at_info_t *ai, work_item_t *it)
{
    //at_lock(ai);    
   // if (ai->list_cnt) 
    //    ai->list_cnt--; 

    //list_del(&it->node);
    work_item_destroy(it);
    //at_unlock(ai);
}

static void urc_reset(at_info_t *ai)
{
    ai->urc_target = 0;
    ai->urc_cnt    = 0;
    ai->urc_item   = NULL;
    ai->urc_match  = 0;
}

/**
 * @brief  AT命令处理函数
 */
static void at_work_process(at_info_t *ai)
{
    if ( ai->item == NULL ) {
      return; //No work to do.
    }
    
  /*Enter running state*/
  if ( ai->item->state == AT_WORK_STAT_READY ) 
  {            
    update_work_state(ai->item, AT_WORK_STAT_RUN, (at_resp_code)ai->item->code);   
  }
  
  /* When the job execution is complete, put it into the idle work queue */
 // if (ai->item->state >= AT_WORK_STAT_FINISH || work_handler_table[ai->item->type](ai)) 
  if (  work_handler_table[ai->item->type](ai) ) 
  {
    //Marked the work as done.
    if (ai->item->state == AT_WORK_STAT_RUN)
    {            
      update_work_state(ai->item, AT_WORK_STAT_FINISH, (at_resp_code)ai->item->code);
    }            
    //Recycle Processed work item.
    work_item_recycle(ai, ai->item);
    ai->item = NULL;
    
    use_rate=my_mem_perused(SRAMIN);
    printf("RamUsed:%d\r\n", use_rate);
  }
}


/**
  * @brief Find a URC handler based on URC receive buffer information.
  */
static const urc_item_t *find_urc_item(at_info_t *ai, char *urc_buf, unsigned int size)
{
    const urc_item_t *tbl = ai->urc_tbl;
    int i;
    for (i = 0; i < ai->urc_tbl_size && tbl; i++, tbl++) {
       if (strstr(urc_buf, tbl->prefix))//It will need to be further optimized in the future.
            return tbl;  
    }
    return NULL;
}



/**
 * @brief       URC(unsolicited code) handler entry.
 * @param[in]   urc    - URC receive buffer
 * @param[in]   size   - URC receive buffer length
 * @return      none
 */
static void urc_handler_entry(at_info_t *ai, urc_recv_status status, char *urc, unsigned int size)
{
    int remain;
    at_urc_info_t ctx = {status, urc, size};
    if (ai->urc_target > 0)
        AT_DEBUG(ai, "<=\r\n%.5s..\r\n", urc);
    else 
        AT_DEBUG(ai, "<=\r\n%s\r\n", urc);    
    /* Send URC event notification. */
    remain = ai->urc_item ? ai->urc_item->handler(&ctx) : 0;
    if (remain == 0 && (ai->urc_item || ai->item == NULL)) 
    {
        urc_reset(ai);
    } 
    else 
    {
        AT_DEBUG(ai,"URC receives %d bytes remaining.\r\n", remain);
        ai->urc_target = ai->urc_cnt + remain;
        ai->urc_match = true;
    }
}

/**
 * @brief       Command response data processing
 * @return      none
 */
static void resp_recv_process(at_info_t *ai, const char *buf, unsigned int size)
{
    if (size == 0)
        return;    
    if (ai->recv_cnt + size >= ai->recv_bufsize) //Receive overflow, clear directly.
        ai->recv_cnt = 0;

    memcpy(ai->recvbuf + ai->recv_cnt, buf, size);    
    ai->recv_cnt += size;
    ai->recvbuf[ai->recv_cnt] = '\0';
}

static void urc_timeout_process(at_info_t *ai)
{
    //Receive timeout processing, default (MAX_URC_RECV_TIMEOUT).
    if (ai->urc_cnt > 0 && AT_IS_TIMEOUT(ai->urc_timer, AT_URC_TIMEOUT)) {        
        if (ai->urc_cnt > 2 && ai->urc_item != NULL) {
            ai->urcbuf[ai->urc_cnt] = '\0';
            AT_DEBUG(ai,"urc recv timeout=>%s\r\n", ai->urcbuf);  
            urc_handler_entry(ai, URC_RECV_TIMEOUT, ai->urcbuf, ai->urc_cnt);                      
        }
        urc_reset(ai);     
    }  
}


/**
 * @brief       URC receive processing
 * @param[in]   buf  - Receive buffer
 * @return      none
 */
static void urc_recv_process(at_info_t *ai, char *buf, unsigned int size)
{
    char *urc_buf;
    int ch;
    if (ai->urcbuf == NULL)
        return;
    
    if (size == 0)
    {
        urc_timeout_process(ai);
        return;
    }
    
    if (!ai->urc_enable) 
    {
        if (!AT_IS_TIMEOUT(ai->urc_timer, ai->urc_disable_time))
            return;
        ai->urc_enable = 1;
        AT_DEBUG(ai, "Enable the URC match handler\r\n");
    }    
    
    urc_buf  = ai->urcbuf;
    while (size--)
    {
        ch = *buf++;
        urc_buf[ai->urc_cnt++] = ch;
        
        if (ai->urc_cnt >= ai->urc_bufsize) /* Empty directly on overflow */
        {                   
            urc_reset(ai);
            AT_DEBUG(ai, "Urc buffer full.\r\n");
            continue;
        }
        
        if (ai->urc_match) 
        {
            if (ai->urc_cnt >= ai->urc_target)
            {
                urc_handler_entry(ai, URC_RECV_OK, urc_buf, ai->urc_cnt);
            }                                                    
            continue;
        }
        
        if (strchr(AT_URC_END_MARKS, ch) == NULL && ch != '\0')  // Find the URC end mark.
            continue;
        urc_buf[ai->urc_cnt] = '\0';
        
        if (ai->urc_item == NULL) 
        {                                           
            ai->urc_item = find_urc_item(ai, urc_buf, ai->urc_cnt); //Find the corresponding URC handler   
            if (ai->urc_item == NULL && ch == '\n')
            {
                if (ai->urc_cnt > 2 && ai->item == NULL) //Unrecognized URC message
                    AT_DEBUG(ai, "%s\r\n", urc_buf);
                urc_reset(ai);
                continue;
            }
        }     
        
        if (ai->urc_item != NULL && ch == ai->urc_item->endmark)
            urc_handler_entry(ai, URC_RECV_OK, urc_buf, ai->urc_cnt);  
    }
          
}

/**
 * @brief  透传数据出来函数
 */
static void at_raw_trans_process(at_obj_t *obj)
{
     unsigned char rbuf[32];
    int size;
    int i;
    at_info_t *ai = obj_map(obj);
    if (obj->raw_conf == NULL)
        return;
    size = obj->adap->read(rbuf);
    if (size > 0 ){
        obj->raw_conf->write(rbuf, size);
    }
    size = obj->raw_conf->read(rbuf, sizeof(rbuf));
    if (size > 0) {
        obj->adap->write(rbuf, size);
    } 
    //Exit command detection
    if (obj->raw_conf->exit_cmd != NULL) {
        for (i = 0; i < size; i++) {
            if (ai->recv_cnt >= ai->recv_bufsize)
                ai->recv_cnt = 0;
            ai->recvbuf[ai->recv_cnt] = rbuf[i];
            if (rbuf[i] == '\r' || rbuf[i] == 'n') {
                ai->recvbuf[ai->recv_cnt] = '\0';
                ai->recv_cnt = 0;
                if (strcasecmp(obj->raw_conf->exit_cmd, ai->recvbuf) != 0) {
                    continue;
                }
                if (obj->raw_conf->on_exit) {
                    obj->raw_conf->on_exit();
                }
            } else {
                ai->recv_cnt++;
            }
        }
    }
}

/**
 * @brief  AT work polling processing.
 */
void at_obj_process(at_obj_t *at)
{
  char rbuf[64] ={0};
  int read_size;
  register at_info_t *ai = obj_map(at);
  
//  if (ai->raw_trans) {
//    at_raw_trans_process(at);//透传模式处理函数
//    return;
//  }    
  
  read_size = __get_adapter(ai)->read(rbuf);
  urc_recv_process(ai, rbuf, read_size);//URC处理函数
  resp_recv_process(ai, rbuf, read_size);//AT命令RESP处理函数
  at_work_process(ai);//AT命令模式处理函数
}


static inline const  at_adapter_t *__get_adapter(at_info_t *ai)
{
    return ai->obj.adap;
}

//bool at_work_is_busy(at_context_t *ctx)
//{
//    return ctx->work_state == AT_WORK_STAT_RUN || ctx->work_state == AT_WORK_STAT_READY;
//}

bool at_work_is_busy( at_obj_t *at )
{
    at_info_t *ai = obj_map(at);
    return ai->item->state == AT_WORK_STAT_RUN || ai->item->state == AT_WORK_STAT_READY;
}


/**
 * @brief  Initialize matching info
 */
static void match_info_init(at_info_t *ai, at_attr_t *attr)
{
    ai->prefix = ai->suffix = NULL;
    ai->match_len = 0;
    ai->match_mask = 0;        
    if (attr->prefix == NULL || strlen(attr->prefix) == 0)
        ai->match_mask |= MATCH_MASK_PREFIX;
    if (attr->suffix == NULL || strlen(attr->suffix) == 0)
        ai->match_mask |= MATCH_MASK_SUFFIX;
}


/**
 * @brief  AT execution callback handler.
 */
static void do_at_callback(at_info_t *ai, work_item_t *wi, at_resp_code code)
{
    at_response_t r;
    AT_DEBUG(ai, "<-\r\n%s\r\n", ai->recvbuf);
    //Exception notification
    if ((code == AT_RESP_ERROR || code == AT_RESP_TIMEOUT) && __get_adapter(ai)->error != NULL) {
        __get_adapter(ai)->error(&r);
        ai->err_occur = 1;
        AT_DEBUG(ai, "AT Respose :%s\r\n", code == AT_RESP_TIMEOUT ? "timeout" : "error");
    } else {
        ai->err_occur = 0;
    }
//#if AT_WORK_CONTEXT_EN
//    at_context_t  *ctx = wi->attr.ctx;
//    if (ctx != NULL ) {
//        if (ctx->respbuf != NULL/* && ctx->bufsize */) {
//            ctx->resplen = ai->recv_cnt >= ctx->bufsize ? ctx->bufsize - 1 : ai->recv_cnt;
//            memcpy(ctx->respbuf, ai->recvbuf, ctx->resplen);
//        }
//    }
//#endif
    update_work_state(wi, AT_WORK_STAT_FINISH, code);
    //Submit response data and status.
    if (wi->attr.cb) {
        r.obj     = &ai->obj;
        r.params  = wi->attr.params;
        r.recvbuf = ai->recvbuf;
        r.recvcnt = ai->recv_cnt;
        r.code    = code;        
        r.prefix  = ai->prefix != NULL ? ai->prefix : ai->recvbuf;
        r.suffix  = ai->suffix != NULL ? ai->suffix : ai->recvbuf;
        wi->attr.cb(&r);
    }
}


/**
 * @brief  AT命令处理函数
 */
static int do_cmd_handler(at_info_t *ai)
{
    work_item_t *wi = ai->item;
    at_env_t   *env = &ai->env;
    at_attr_t  *attr = &wi->attr;
    switch (env->state)
    {
    case AT_STAT_SEND: 
        if (wi->type == WORK_TYPE_CUSTOM && wi->sender != NULL) {
            wi->sender(env);
        } else if (wi->type == WORK_TYPE_BUF) {
            __get_adapter(ai)->write(wi->buf, wi->bufsize);
        }  else if (wi->type == WORK_TYPE_SINGLLINE) {
            send_cmdline(ai, wi->singlline);
        } else {
            send_cmdline(ai, wi->buf);
        }
        env->state = AT_STAT_RECV;
        env->reset_timer(env);
        env->recvclr(env);
        match_info_init(ai, attr);        
        break;
    case AT_STAT_RECV: /*Receive information and matching processing.*/
        if (ai->match_len != ai->recv_cnt) {
            ai->match_len = ai->recv_cnt;
            //Matching response content prefix.
            if ( !(ai->match_mask & MATCH_MASK_PREFIX) ) {
                ai->prefix = strstr(ai->recvbuf, attr->prefix);
                ai->match_mask |= ai->prefix ? MATCH_MASK_PREFIX : 0x00;
            } 
            //Matching response content suffix.
            if ( ai->match_mask & MATCH_MASK_PREFIX ) {
                ai->suffix = strstr(ai->prefix ? ai->prefix : ai->recvbuf, attr->suffix);
                ai->match_mask |= ai->suffix ? MATCH_MASK_SUFFIX : 0x00;
            }
            ai->match_mask |= strstr(ai->recvbuf, AT_DEF_RESP_ERR) ? MATCH_MASK_ERROR : 0x00;
        }
        if (ai->match_mask & MATCH_MASK_ERROR) {  
			AT_DEBUG(ai, "<-\r\n%s\r\n", ai->recvbuf);
            if (env->i++ >= attr->retry) {
                do_at_callback(ai, wi, AT_RESP_ERROR);
                return true;
            }
            
            env->state = AT_STAT_RETRY; //If the command responds incorrectly, it will wait for a while and try again.
            env->reset_timer(env); 
        }
        if (ai->match_mask & MATCH_MASK_SUFFIX) {
            do_at_callback(ai, wi, AT_RESP_OK);
            return true;
        } else if (env->is_timeout(env, attr->timeout)) {
			AT_DEBUG(ai, "Command response timeout, retry:%d\r\n", env->i);
            if (env->i++ >= attr->retry) {
                do_at_callback(ai, wi, AT_RESP_TIMEOUT);
                return true;
            }            
            env->state = AT_STAT_SEND;            
        }
        break;        
    case AT_STAT_RETRY:
        if (env->is_timeout(env, 100))
            env->state = AT_STAT_SEND; /*Go back to the send state*/
        break;
    default:
        env->state = AT_STAT_SEND;
    }
    return false;
}


/**
 * @brief   Execute custom command
 * @param   attr AT attributes(NULL to use the default value)
 * @param   sender Command sending handler (such as sending any type of data through the env->obj->adap-write interface)
 * @retval  Indicates whether the asynchronous work was enqueued successfully
 */
bool at_custom_cmd(at_obj_t *at, const at_attr_t *attr, void (*sender)(at_env_t *env))
{
  work_item_t *it = create_work_item( obj_map(at), WORK_TYPE_CUSTOM, attr, (const void *)sender, 0 );
  if (it == NULL)
    return false;
  
  return true;
}

/**
 * @brief   Send a single-line command
 * @param   cb        Response callback handler, Fill in NULL if not required
 * @param   timeout   command execution timeout(ms)
 * @param   retry     command retries( >= 0)
 * @param   singlline command
 * @retval  Indicates whether the asynchronous work was enqueued successfully.
 * @note    Only the address is saved, so the 'singlline' can not be a local variable which will be destroyed.
 */
bool at_send_singlline(at_obj_t *at, const at_attr_t *attr, const char *cmd)
{
  use_rate=my_mem_perused(SRAMIN);
  printf("RamUsed:%d\r\n", use_rate);
  
  work_item_t *it = create_work_item( obj_map(at), WORK_TYPE_SINGLLINE, attr, cmd, 0);
  if (it == NULL)
    return false;
  
  use_rate=my_mem_perused(SRAMIN);
  printf("RamUsed:%d\r\n", use_rate);
  return true;
}

/**
 * @brief   Send (binary) data
 * @param   attr AT attributes(NULL to use the default value)
 * @param   databuf Binary data
 * @param   bufsize Binary data length
 * @retval  Indicates whether the asynchronous work was enqueued successfully
 */
bool at_send_data(at_obj_t *at, const at_attr_t *attr, const void *databuf, unsigned int bufsize)
{
    work_item_t *it = create_work_item( obj_map(at), WORK_TYPE_BUF, attr, databuf, bufsize );
    if (it == NULL)
    return false;
    
    return true;
}


/**
 * @brief   Execute custom work.
 * @param   params User parameter.
 * @param   work   AT custom polling work entry, specific usage ref@at_work_t
 * @retval  Indicates whether the asynchronous work was enqueued successfully.
 */
bool at_do_work(at_obj_t *at, void *params, at_work_t work)
{
   // at_attr_t attr;
   // at_attr_deinit(&attr);
   // attr.params = params;
    //return add_work_item(obj_map(at), WORK_TYPE_GENERAL, &attr, (const void *)work, 0) != NULL;
     //return create_work_item(obj_map(at), WORK_TYPE_GENERAL, &attr, (const void *)work, 0) != NULL;
    //at_work_abort_all(at);
    
    return true;
}


/**
 * @brief   Default attributes initialization.
 *          Default low priority, other reference AT_DEF_XXX definition.
 * @param   attr AT attributes
 */
void at_attr_deinit(at_attr_t *attr)
{
    *attr = at_def_attr;
}


/**
 * @brief Abort all AT work
 */
void at_work_abort_all(at_obj_t *at)
{
//    struct list_head *pos;
   // work_item_t *it;
    at_info_t *ai = obj_map(at);    
//    at_lock(ai);
//    list_for_each(pos, &ai->hlist) {
//        it = list_entry(pos, work_item_t, node);
//        update_work_state(it, AT_WORK_STAT_ABORT, AT_RESP_ABORT);
//    }
//    list_for_each(pos, &ai->llist) {
//        it = list_entry(pos, work_item_t, node);
//        update_work_state(it, AT_WORK_STAT_ABORT, AT_RESP_ABORT);
//    }
//    at_unlock(ai);
    
     update_work_state(ai->item, AT_WORK_STAT_ABORT, AT_RESP_ABORT);
}


/**
 * @brief  Create an AT object
 * @param  adap AT interface adapter (AT object only saves its pointer, it must be a global resident object)
 * @return Pointer to a new AT object
 */
at_obj_t *at_obj_create(const at_adapter_t *adap)
{
    at_env_t *e;
    at_info_t *ai = at_malloc(sizeof(at_info_t));
    if (ai == NULL)
        return NULL;
    memset(ai, 0, sizeof(at_info_t));
    ai->obj.adap = adap;
    /* Initialize high and low priority queues*/
   // INIT_LIST_HEAD(&ai->hlist);
  //  INIT_LIST_HEAD(&ai->llist);
    //Allocate at least 32 bytes to the buffer
    ai->recv_bufsize = adap->recv_bufsize < 32 ? 32 : adap->recv_bufsize;
    ai->recvbuf      = at_malloc(ai->recv_bufsize);
    if (ai->recvbuf == NULL) {
        at_obj_destroy(&ai->obj);
        return NULL;
    }
#if AT_URC_WARCH_EN    
    if (adap->urc_bufsize != 0) {
        ai->urc_bufsize  = adap->urc_bufsize < 32 ? 32 : adap->urc_bufsize;
        ai->urcbuf       = at_malloc(ai->urc_bufsize);
        if (ai->urcbuf == NULL) {
            at_obj_destroy(&ai->obj);
            return NULL;
        }         
    }
#endif
    e              = &ai->env;
    ai->recv_cnt   = 0;
    ai->urc_enable = 1;
    ai->enable     = 1;    
    //Initialization of public work environment.
    e->is_timeout  = at_is_timeout;
    e->println     = println;
    e->recvbuf     = get_recvbuf;
    e->recvclr     = recvbuf_clear;
    e->recvlen     = get_recv_count;
   // e->contains    = find_substr;
    e->disposing   = at_isabort;
    e->finish      = at_finish;
    e->reset_timer = at_reset_timer;
    //e->next_wait   = at_next_wait;
    return &ai->obj;
}

/**
 * @brief  Destroy a AT object.
 */
void at_obj_destroy(at_obj_t *obj)
{
    at_info_t *ai = obj_map(obj);

    if (obj == NULL)
        return;

   // work_item_destroy_all(ai, &ai->hlist);
   // work_item_destroy_all(ai, &ai->llist);

    if (ai->recvbuf != NULL)
        at_free(ai->recvbuf);
#if AT_URC_WARCH_EN        
    if (ai->urcbuf != NULL)
        at_free(ai->urcbuf);
#endif
    at_free(ai);

}


/**
 * @brief   Set the AT urc table.
 */
void at_obj_set_urc(at_obj_t *at, const urc_item_t *tbl, int count)
{
    at_info_t *ai = obj_map(at);
    ai->urc_tbl      = tbl;
    ai->urc_tbl_size = count;
}
