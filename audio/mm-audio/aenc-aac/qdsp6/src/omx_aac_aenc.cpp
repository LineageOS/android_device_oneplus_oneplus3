/*--------------------------------------------------------------------------
Copyright (c) 2010-2016, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/
/*============================================================================
@file omx_aenc_aac.c
  This module contains the implementation of the OpenMAX core & component.

*//*========================================================================*/
//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////


#include<string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "omx_aac_aenc.h"
#include <errno.h>

using namespace std;

#define SLEEP_MS 100

// omx_cmd_queue destructor
omx_aac_aenc::omx_cmd_queue::~omx_cmd_queue()
{
    // Nothing to do
}

// omx cmd queue constructor
omx_aac_aenc::omx_cmd_queue::omx_cmd_queue(): m_read(0),m_write(0),m_size(0)
{
    memset(m_q,      0,sizeof(omx_event)*OMX_CORE_CONTROL_CMDQ_SIZE);
}

// omx cmd queue insert
bool omx_aac_aenc::omx_cmd_queue::insert_entry(unsigned long p1,
                                                unsigned long p2,
                                                unsigned char id)
{
    bool ret = true;
    if (m_size < OMX_CORE_CONTROL_CMDQ_SIZE)
    {
        m_q[m_write].id       = id;
        m_q[m_write].param1   = p1;
        m_q[m_write].param2   = p2;
        m_write++;
        m_size ++;
        if (m_write >= OMX_CORE_CONTROL_CMDQ_SIZE)
        {
            m_write = 0;
        }
    } else
    {
        ret = false;
        DEBUG_PRINT_ERROR("ERROR!!! Command Queue Full");
    }
    return ret;
}

bool omx_aac_aenc::omx_cmd_queue::pop_entry(unsigned long *p1,
                                             unsigned long *p2, unsigned char *id)
{
    bool ret = true;
    if (m_size > 0)
    {
        *id = m_q[m_read].id;
        *p1 = m_q[m_read].param1;
        *p2 = m_q[m_read].param2;
        // Move the read pointer ahead
        ++m_read;
        --m_size;
        if (m_read >= OMX_CORE_CONTROL_CMDQ_SIZE)
        {
            m_read = 0;

        }
    } else
    {
        ret = false;
        DEBUG_PRINT_ERROR("ERROR Delete!!! Command Queue Empty");
    }
    return ret;
}

// factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
    return(new omx_aac_aenc);
}
bool omx_aac_aenc::omx_cmd_queue::get_msg_id(unsigned char *id)
{
   if(m_size > 0)
   {
       *id = m_q[m_read].id;
       DEBUG_PRINT("get_msg_id=%d\n",*id);
   }
   else{
       return false;
   }
   return true;
}
/*=============================================================================
FUNCTION:
  wait_for_event

DESCRIPTION:
  waits for a particular event

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
   None
=============================================================================*/
void omx_aac_aenc::wait_for_event()
{
    int               rc = 0;
    struct timespec   ts;
    pthread_mutex_lock(&m_event_lock);
    while (0 == m_is_event_done)
    {
       clock_gettime(CLOCK_REALTIME, &ts);
       ts.tv_sec += (SLEEP_MS/1000);
       ts.tv_nsec += ((SLEEP_MS%1000) * 1000000);
       rc = pthread_cond_timedwait(&cond, &m_event_lock, &ts);
       if (rc == ETIMEDOUT && !m_is_event_done) {
         DEBUG_PRINT("Timed out waiting for flush");
         rc = ioctl(m_drv_fd, AUDIO_FLUSH, 0);
         if (rc == -1)
         {
           DEBUG_PRINT_ERROR("Flush:Input port, ioctl flush failed: rc:%d, %s, no:%d \n",
              rc, strerror(errno), errno);
         }
         else if (rc < 0)
         {
           DEBUG_PRINT_ERROR("Flush:Input port, ioctl failed error: rc:%d, %s, no:%d \n",
               rc, strerror(errno), errno);
           break;
         }
       }
    }
    m_is_event_done = 0;
    pthread_mutex_unlock(&m_event_lock);
}

/*=============================================================================
FUNCTION:
  event_complete

DESCRIPTION:
  informs about the occurance of an event

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
   None
=============================================================================*/
void omx_aac_aenc::event_complete()
{
    pthread_mutex_lock(&m_event_lock);
    if (0 == m_is_event_done)
    {
        m_is_event_done = 1;
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&m_event_lock);
}

// All this non-sense because of a single aac object
void omx_aac_aenc::in_th_goto_sleep()
{
    pthread_mutex_lock(&m_in_th_lock);
    while (0 == m_is_in_th_sleep)
    {
        pthread_cond_wait(&in_cond, &m_in_th_lock);
    }
    m_is_in_th_sleep = 0;
    pthread_mutex_unlock(&m_in_th_lock);
}

void omx_aac_aenc::in_th_wakeup()
{
    pthread_mutex_lock(&m_in_th_lock);
    if (0 == m_is_in_th_sleep)
    {
        m_is_in_th_sleep = 1;
        pthread_cond_signal(&in_cond);
    }
    pthread_mutex_unlock(&m_in_th_lock);
}

void omx_aac_aenc::out_th_goto_sleep()
{

    pthread_mutex_lock(&m_out_th_lock);
    while (0 == m_is_out_th_sleep)
    {
        pthread_cond_wait(&out_cond, &m_out_th_lock);
    }
    m_is_out_th_sleep = 0;
    pthread_mutex_unlock(&m_out_th_lock);
}

void omx_aac_aenc::out_th_wakeup()
{
    pthread_mutex_lock(&m_out_th_lock);
    if (0 == m_is_out_th_sleep)
    {
        m_is_out_th_sleep = 1;
        pthread_cond_signal(&out_cond);
    }
    pthread_mutex_unlock(&m_out_th_lock);
}
/* ======================================================================
FUNCTION
  omx_aac_aenc::omx_aac_aenc

DESCRIPTION
  Constructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_aac_aenc::omx_aac_aenc(): m_tmp_meta_buf(NULL),
        m_tmp_out_meta_buf(NULL),
        m_flush_cnt(255),
        m_comp_deinit(0),
        adif_flag(0),
        mp4ff_flag(0),
        m_app_data(NULL),
        nNumOutputBuf(0),
        m_drv_fd(-1),
        bFlushinprogress(0),
        is_in_th_sleep(false),
        is_out_th_sleep(false),
        m_flags(0),
        nTimestamp(0),
        ts(0),
        frameduration(0),
        m_inp_act_buf_count (OMX_CORE_NUM_INPUT_BUFFERS),
        m_out_act_buf_count (OMX_CORE_NUM_OUTPUT_BUFFERS),
        m_inp_current_buf_count(0),
        m_out_current_buf_count(0),
        output_buffer_size((OMX_U32)OMX_AAC_OUTPUT_BUFFER_SIZE),
        input_buffer_size(OMX_CORE_INPUT_BUFFER_SIZE),
        m_session_id(0),
        m_inp_bEnabled(OMX_TRUE),
        m_out_bEnabled(OMX_TRUE),
        m_inp_bPopulated(OMX_FALSE),
        m_out_bPopulated(OMX_FALSE),
        m_is_event_done(0),
        m_state(OMX_StateInvalid),
        m_ipc_to_in_th(NULL),
        m_ipc_to_out_th(NULL),
        m_ipc_to_cmd_th(NULL)
{
    int cond_ret = 0;
    component_Role.nSize = 0;
    memset(&m_cmp, 0, sizeof(m_cmp));
    memset(&m_cb, 0, sizeof(m_cb));
    memset(&m_aac_pb_stats, 0, sizeof(m_aac_pb_stats));
    memset(&m_pcm_param, 0, sizeof(m_pcm_param));
    memset(&m_aac_param, 0, sizeof(m_aac_param));
    memset(&m_buffer_supplier, 0, sizeof(m_buffer_supplier));
    memset(&m_priority_mgm, 0, sizeof(m_priority_mgm));

    pthread_mutexattr_init(&m_lock_attr);
    pthread_mutex_init(&m_lock, &m_lock_attr);
    pthread_mutexattr_init(&m_commandlock_attr);
    pthread_mutex_init(&m_commandlock, &m_commandlock_attr);

    pthread_mutexattr_init(&m_outputlock_attr);
    pthread_mutex_init(&m_outputlock, &m_outputlock_attr);

    pthread_mutexattr_init(&m_state_attr);
    pthread_mutex_init(&m_state_lock, &m_state_attr);

    pthread_mutexattr_init(&m_event_attr);
    pthread_mutex_init(&m_event_lock, &m_event_attr);

    pthread_mutexattr_init(&m_flush_attr);
    pthread_mutex_init(&m_flush_lock, &m_flush_attr);

    pthread_mutexattr_init(&m_event_attr);
    pthread_mutex_init(&m_event_lock, &m_event_attr);

    pthread_mutexattr_init(&m_in_th_attr);
    pthread_mutex_init(&m_in_th_lock, &m_in_th_attr);

    pthread_mutexattr_init(&m_out_th_attr);
    pthread_mutex_init(&m_out_th_lock, &m_out_th_attr);

    pthread_mutexattr_init(&m_in_th_attr_1);
    pthread_mutex_init(&m_in_th_lock_1, &m_in_th_attr_1);

    pthread_mutexattr_init(&m_out_th_attr_1);
    pthread_mutex_init(&m_out_th_lock_1, &m_out_th_attr_1);

    pthread_mutexattr_init(&out_buf_count_lock_attr);
    pthread_mutex_init(&out_buf_count_lock, &out_buf_count_lock_attr);

    pthread_mutexattr_init(&in_buf_count_lock_attr);
    pthread_mutex_init(&in_buf_count_lock, &in_buf_count_lock_attr);
    if ((cond_ret = pthread_cond_init (&cond, NULL)) != 0)
    {
       DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for cond\n");
       if (cond_ret == EAGAIN)
         DEBUG_PRINT_ERROR("The system lacked necessary \
				resources(other than mem)\n");
       else if (cond_ret == ENOMEM)
          DEBUG_PRINT_ERROR("Insufficient memory to \
				initialise condition variable\n");
    }
    if ((cond_ret = pthread_cond_init (&in_cond, NULL)) != 0)
    {
       DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for in_cond\n");
       if (cond_ret == EAGAIN)
         DEBUG_PRINT_ERROR("The system lacked necessary \
				resources(other than mem)\n");
       else if (cond_ret == ENOMEM)
          DEBUG_PRINT_ERROR("Insufficient memory to \
				initialise condition variable\n");
    }
    if ((cond_ret = pthread_cond_init (&out_cond, NULL)) != 0)
    {
       DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for out_cond\n");
       if (cond_ret == EAGAIN)
         DEBUG_PRINT_ERROR("The system lacked necessary \
				resources(other than mem)\n");
       else if (cond_ret == ENOMEM)
          DEBUG_PRINT_ERROR("Insufficient memory to \
				initialise condition variable\n");
    }

    sem_init(&sem_read_msg,0, 0);
    sem_init(&sem_write_msg,0, 0);
    sem_init(&sem_States,0, 0);
    return;
}


/* ======================================================================
FUNCTION
  omx_aac_aenc::~omx_aac_aenc

DESCRIPTION
  Destructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_aac_aenc::~omx_aac_aenc()
{
    DEBUG_PRINT_ERROR("AAC Object getting destroyed comp-deinit=%d\n",
			m_comp_deinit);
    if ( !m_comp_deinit )
    {
        deinit_encoder();
    }
    pthread_mutexattr_destroy(&m_lock_attr);
    pthread_mutex_destroy(&m_lock);

    pthread_mutexattr_destroy(&m_commandlock_attr);
    pthread_mutex_destroy(&m_commandlock);

    pthread_mutexattr_destroy(&m_outputlock_attr);
    pthread_mutex_destroy(&m_outputlock);

    pthread_mutexattr_destroy(&m_state_attr);
    pthread_mutex_destroy(&m_state_lock);

    pthread_mutexattr_destroy(&m_event_attr);
    pthread_mutex_destroy(&m_event_lock);

    pthread_mutexattr_destroy(&m_flush_attr);
    pthread_mutex_destroy(&m_flush_lock);

    pthread_mutexattr_destroy(&m_in_th_attr);
    pthread_mutex_destroy(&m_in_th_lock);

    pthread_mutexattr_destroy(&m_out_th_attr);
    pthread_mutex_destroy(&m_out_th_lock);

    pthread_mutexattr_destroy(&out_buf_count_lock_attr);
    pthread_mutex_destroy(&out_buf_count_lock);

    pthread_mutexattr_destroy(&in_buf_count_lock_attr);
    pthread_mutex_destroy(&in_buf_count_lock);

    pthread_mutexattr_destroy(&m_in_th_attr_1);
    pthread_mutex_destroy(&m_in_th_lock_1);

    pthread_mutexattr_destroy(&m_out_th_attr_1);
    pthread_mutex_destroy(&m_out_th_lock_1);
    pthread_mutex_destroy(&out_buf_count_lock);
    pthread_mutex_destroy(&in_buf_count_lock);
    pthread_cond_destroy(&cond);
    pthread_cond_destroy(&in_cond);
    pthread_cond_destroy(&out_cond);
    sem_destroy (&sem_read_msg);
    sem_destroy (&sem_write_msg);
    sem_destroy (&sem_States);
    DEBUG_PRINT_ERROR("OMX AAC component destroyed\n");
    return;
}

/**
  @brief memory function for sending EmptyBufferDone event
   back to IL client

  @param bufHdr OMX buffer header to be passed back to IL client
  @return none
 */
void omx_aac_aenc::buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
    if (m_cb.EmptyBufferDone)
    {
        PrintFrameHdr(OMX_COMPONENT_GENERATE_BUFFER_DONE,bufHdr);
        bufHdr->nFilledLen = 0;

        m_cb.EmptyBufferDone(&m_cmp, m_app_data, bufHdr);
        pthread_mutex_lock(&in_buf_count_lock);
        m_aac_pb_stats.ebd_cnt++;
        nNumInputBuf--;
        DEBUG_DETAIL("EBD CB:: in_buf_len=%d nNumInputBuf=%d ebd_cnd %d\n",\
                     m_aac_pb_stats.tot_in_buf_len,
                     nNumInputBuf, m_aac_pb_stats.ebd_cnt);
        pthread_mutex_unlock(&in_buf_count_lock);
    }

    return;
}

/*=============================================================================
FUNCTION:
  flush_ack

DESCRIPTION:


INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
void omx_aac_aenc::flush_ack()
{
    // Decrement the FLUSH ACK count and notify the waiting recepients
    pthread_mutex_lock(&m_flush_lock);
    --m_flush_cnt;
    if (0 == m_flush_cnt)
    {
        event_complete();
    }
    DEBUG_PRINT("Rxed FLUSH ACK cnt=%d\n",m_flush_cnt);
    pthread_mutex_unlock(&m_flush_lock);
}
void omx_aac_aenc::frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
    if (m_cb.FillBufferDone)
    {
        PrintFrameHdr(OMX_COMPONENT_GENERATE_FRAME_DONE,bufHdr);
        m_aac_pb_stats.fbd_cnt++;
        pthread_mutex_lock(&out_buf_count_lock);
        nNumOutputBuf--;
        DEBUG_PRINT("FBD CB:: nNumOutputBuf=%d out_buf_len=%u fbd_cnt=%u\n",\
                    nNumOutputBuf,
                    m_aac_pb_stats.tot_out_buf_len,
                    m_aac_pb_stats.fbd_cnt);
        m_aac_pb_stats.tot_out_buf_len += bufHdr->nFilledLen;
        m_aac_pb_stats.tot_pb_time     = bufHdr->nTimeStamp;
        DEBUG_PRINT("FBD:in_buf_len=%u out_buf_len=%u\n",
                    m_aac_pb_stats.tot_in_buf_len,
                    m_aac_pb_stats.tot_out_buf_len);
        pthread_mutex_unlock(&out_buf_count_lock);
        m_cb.FillBufferDone(&m_cmp, m_app_data, bufHdr);
    }
    return;
}

/*=============================================================================
FUNCTION:
  process_out_port_msg

DESCRIPTION:
  Function for handling all commands from IL client
IL client commands are processed and callbacks are generated through
this routine  Audio Command Server provides the thread context for this routine

INPUT/OUTPUT PARAMETERS:
  [INOUT] client_data
  [IN] id

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
void omx_aac_aenc::process_out_port_msg(void *client_data, unsigned char id)
{
    unsigned long p1 = 0;                            // Parameter - 1
    unsigned long p2 = 0;                            // Parameter - 2
    unsigned char ident = 0;
    unsigned      qsize     = 0;                 // qsize
    unsigned      tot_qsize = 0;
    omx_aac_aenc  *pThis    = (omx_aac_aenc *) client_data;
    OMX_STATETYPE state;

loopback_out:
    pthread_mutex_lock(&pThis->m_state_lock);
    pThis->get_state(&pThis->m_cmp, &state);
    pthread_mutex_unlock(&pThis->m_state_lock);
    if ( state == OMX_StateLoaded )
    {
        DEBUG_PRINT(" OUT: IN LOADED STATE RETURN\n");
        return;
    }
    pthread_mutex_lock(&pThis->m_outputlock);

    qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize += pThis->m_output_ctrl_fbd_q.m_size;
    tot_qsize += pThis->m_output_q.m_size;

    if ( 0 == tot_qsize )
    {
        pthread_mutex_unlock(&pThis->m_outputlock);
        DEBUG_DETAIL("OUT-->BREAK FROM LOOP...%d\n",tot_qsize);
        return;
    }
    if ( (state != OMX_StateExecuting) && !qsize )
    {
        pthread_mutex_unlock(&pThis->m_outputlock);
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
        if ( state == OMX_StateLoaded )
            return;

        DEBUG_DETAIL("OUT:1.SLEEPING OUT THREAD\n");
        pthread_mutex_lock(&pThis->m_out_th_lock_1);
        pThis->is_out_th_sleep = true;
        pthread_mutex_unlock(&pThis->m_out_th_lock_1);
        pThis->out_th_goto_sleep();

        /* Get the updated state */
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
    }

    if ( ((!pThis->m_output_ctrl_cmd_q.m_size) && !pThis->m_out_bEnabled) )
    {
        // case where no port reconfig and nothing in the flush q
        DEBUG_DETAIL("No flush/port reconfig qsize=%d tot_qsize=%d",\
            qsize,tot_qsize);
        pthread_mutex_unlock(&pThis->m_outputlock);
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
        if ( state == OMX_StateLoaded )
            return;

        if(pThis->m_output_ctrl_cmd_q.m_size || !(pThis->bFlushinprogress))
        {
            DEBUG_PRINT("OUT:2. SLEEPING OUT THREAD \n");
            pthread_mutex_lock(&pThis->m_out_th_lock_1);
            pThis->is_out_th_sleep = true;
            pthread_mutex_unlock(&pThis->m_out_th_lock_1);
            pThis->out_th_goto_sleep();
        }
        /* Get the updated state */
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
    }

    qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize += pThis->m_output_ctrl_fbd_q.m_size;
    tot_qsize += pThis->m_output_q.m_size;
    pthread_mutex_lock(&pThis->m_state_lock);
    pThis->get_state(&pThis->m_cmp, &state);
    pthread_mutex_unlock(&pThis->m_state_lock);
    DEBUG_DETAIL("OUT-->QSIZE-flush=%d,fbd=%d QSIZE=%d state=%d\n",\
        pThis->m_output_ctrl_cmd_q.m_size,
        pThis->m_output_ctrl_fbd_q.m_size,
        pThis->m_output_q.m_size,state);


    if (qsize)
    {
        // process FLUSH message
        pThis->m_output_ctrl_cmd_q.pop_entry(&p1,&p2,&ident);
    } else if ( (qsize = pThis->m_output_ctrl_fbd_q.m_size) &&
        (pThis->m_out_bEnabled) && (state == OMX_StateExecuting) )
    {
        // then process EBD's
        pThis->m_output_ctrl_fbd_q.pop_entry(&p1,&p2,&ident);
    } else if ( (qsize = pThis->m_output_q.m_size) &&
        (pThis->m_out_bEnabled) && (state == OMX_StateExecuting) )
    {
        // if no FLUSH and FBD's then process FTB's
        pThis->m_output_q.pop_entry(&p1,&p2,&ident);
    } else if ( state == OMX_StateLoaded )
    {
        pthread_mutex_unlock(&pThis->m_outputlock);
        DEBUG_PRINT("IN: ***in OMX_StateLoaded so exiting\n");
        return ;
    } else
    {
        qsize = 0;
        DEBUG_PRINT("OUT--> Empty Queue state=%d %d %d %d\n",state,
                     pThis->m_output_ctrl_cmd_q.m_size,
		     pThis->m_output_ctrl_fbd_q.m_size,
                     pThis->m_output_q.m_size);

        if(state == OMX_StatePause)
        {
            DEBUG_DETAIL("OUT: SLEEPING AGAIN OUT THREAD\n");
            pthread_mutex_lock(&pThis->m_out_th_lock_1);
            pThis->is_out_th_sleep = true;
            pthread_mutex_unlock(&pThis->m_out_th_lock_1);
            pthread_mutex_unlock(&pThis->m_outputlock);
            pThis->out_th_goto_sleep();
            goto loopback_out;
        }
    }
    pthread_mutex_unlock(&pThis->m_outputlock);

    if ( qsize > 0 )
    {
        id = ident;
        ident = 0;
        DEBUG_DETAIL("OUT->state[%d]ident[%d]flushq[%d]fbd[%d]dataq[%d]\n",\
            pThis->m_state,
            ident,
            pThis->m_output_ctrl_cmd_q.m_size,
            pThis->m_output_ctrl_fbd_q.m_size,
            pThis->m_output_q.m_size);

        if ( OMX_COMPONENT_GENERATE_FRAME_DONE == id )
        {
            pThis->frame_done_cb((OMX_BUFFERHEADERTYPE *)p2);
        } else if ( OMX_COMPONENT_GENERATE_FTB == id )
        {
            pThis->fill_this_buffer_proxy((OMX_HANDLETYPE)p1,
                (OMX_BUFFERHEADERTYPE *)p2);
        } else if ( OMX_COMPONENT_GENERATE_EOS == id )
        {
            pThis->m_cb.EventHandler(&pThis->m_cmp,
                pThis->m_app_data,
                OMX_EventBufferFlag,
                1, 1, NULL );

        }
        else if(id == OMX_COMPONENT_RESUME)
        {
             DEBUG_PRINT("RESUMED...\n");
        }
        else if(id == OMX_COMPONENT_GENERATE_COMMAND)
        {
            // Execute FLUSH command
            if ( OMX_CommandFlush == p1 )
            {
                DEBUG_DETAIL("Executing FLUSH command on Output port\n");
                pThis->execute_output_omx_flush();
            } else
            {
                DEBUG_DETAIL("Invalid command[%lu]\n",p1);
            }
        } else
        {
            DEBUG_PRINT_ERROR("ERROR:OUT-->Invalid Id[%d]\n",id);
        }
    } else
    {
        DEBUG_DETAIL("ERROR: OUT--> Empty OUTPUTQ\n");
    }

    return;
}

/*=============================================================================
FUNCTION:
  process_command_msg

DESCRIPTION:


INPUT/OUTPUT PARAMETERS:
  [INOUT] client_data
  [IN] id

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
void omx_aac_aenc::process_command_msg(void *client_data, unsigned char id)
{
    unsigned long p1 = 0;                             // Parameter - 1
    unsigned long p2 = 0;                            // Parameter - 2
    unsigned char ident = 0;
    unsigned     qsize  = 0;
    omx_aac_aenc *pThis = (omx_aac_aenc*)client_data;
    pthread_mutex_lock(&pThis->m_commandlock);

    qsize = pThis->m_command_q.m_size;
    DEBUG_DETAIL("CMD-->QSIZE=%d state=%d\n",pThis->m_command_q.m_size,
                 pThis->m_state);

    if (!qsize)
    {
        DEBUG_DETAIL("CMD-->BREAKING FROM LOOP\n");
        pthread_mutex_unlock(&pThis->m_commandlock);
        return;
    } else
    {
        pThis->m_command_q.pop_entry(&p1,&p2,&ident);
    }
    pthread_mutex_unlock(&pThis->m_commandlock);

    id = ident;
    DEBUG_DETAIL("CMD->state[%d]id[%d]cmdq[%d]n",\
                 pThis->m_state,ident, \
                 pThis->m_command_q.m_size);

    if (OMX_COMPONENT_GENERATE_EVENT == id)
    {
        if (pThis->m_cb.EventHandler)
        {
            if (OMX_CommandStateSet == p1)
            {
                pthread_mutex_lock(&pThis->m_state_lock);
                pThis->m_state = (OMX_STATETYPE) p2;
                pthread_mutex_unlock(&pThis->m_state_lock);
                DEBUG_PRINT("CMD:Process->state set to %d \n", \
                            pThis->m_state);

                if (pThis->m_state == OMX_StateExecuting ||
                    pThis->m_state == OMX_StateLoaded)
                {

                    pthread_mutex_lock(&pThis->m_in_th_lock_1);
                    if (pThis->is_in_th_sleep)
                    {
                        pThis->is_in_th_sleep = false;
                        DEBUG_DETAIL("CMD:WAKING UP IN THREADS\n");
                        pThis->in_th_wakeup();
                    }
                    pthread_mutex_unlock(&pThis->m_in_th_lock_1);

                    pthread_mutex_lock(&pThis->m_out_th_lock_1);
                    if (pThis->is_out_th_sleep)
                    {
                        DEBUG_DETAIL("CMD:WAKING UP OUT THREADS\n");
                        pThis->is_out_th_sleep = false;
                        pThis->out_th_wakeup();
                    }
                    pthread_mutex_unlock(&pThis->m_out_th_lock_1);
                }
            }
            if (OMX_StateInvalid == pThis->m_state)
            {
                pThis->m_cb.EventHandler(&pThis->m_cmp,
                                         pThis->m_app_data,
                                         OMX_EventError,
                                         OMX_ErrorInvalidState,
                                         0, NULL );
            } else if ((signed)p2 == OMX_ErrorPortUnpopulated)
            {
                pThis->m_cb.EventHandler(&pThis->m_cmp,
                                         pThis->m_app_data,
                                         OMX_EventError,
                                         (OMX_U32)p2,
                                         0,
                                         0);
            } else
            {
                pThis->m_cb.EventHandler(&pThis->m_cmp,
                                         pThis->m_app_data,
                                         OMX_EventCmdComplete,
                                         (OMX_U32)p1, (OMX_U32)p2, NULL );
            }
        } else
        {
            DEBUG_PRINT_ERROR("ERROR:CMD-->EventHandler NULL \n");
        }
    } else if (OMX_COMPONENT_GENERATE_COMMAND == id)
    {
        pThis->send_command_proxy(&pThis->m_cmp,
                                  (OMX_COMMANDTYPE)p1,
                                  (OMX_U32)p2,(OMX_PTR)NULL);
    } else if (OMX_COMPONENT_PORTSETTINGS_CHANGED == id)
    {
        DEBUG_DETAIL("CMD-->RXED PORTSETTINGS_CHANGED");
        pThis->m_cb.EventHandler(&pThis->m_cmp,
                                 pThis->m_app_data,
                                 OMX_EventPortSettingsChanged,
                                 1, 1, NULL );
    }
    else
    {
       DEBUG_PRINT_ERROR("CMD->state[%d]id[%d]\n",pThis->m_state,ident);
    }
    return;
}

/*=============================================================================
FUNCTION:
  process_in_port_msg

DESCRIPTION:


INPUT/OUTPUT PARAMETERS:
  [INOUT] client_data
  [IN] id

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
void omx_aac_aenc::process_in_port_msg(void *client_data, unsigned char id)
{
    unsigned long p1 = 0;                            // Parameter - 1
    unsigned long p2 = 0;                            // Parameter - 2
    unsigned char ident = 0;
    unsigned      qsize     = 0;
    unsigned      tot_qsize = 0;
    omx_aac_aenc  *pThis    = (omx_aac_aenc *) client_data;
    OMX_STATETYPE state;

    if (!pThis)
    {
        DEBUG_PRINT_ERROR("ERROR:IN--> Invalid Obj \n");
        return;
    }
loopback_in:
    pthread_mutex_lock(&pThis->m_state_lock);
    pThis->get_state(&pThis->m_cmp, &state);
    pthread_mutex_unlock(&pThis->m_state_lock);
    if ( state == OMX_StateLoaded )
    {
        DEBUG_PRINT(" IN: IN LOADED STATE RETURN\n");
        return;
    }
    // Protect the shared queue data structure
    pthread_mutex_lock(&pThis->m_lock);

    qsize = pThis->m_input_ctrl_cmd_q.m_size;
    tot_qsize = qsize;
    tot_qsize += pThis->m_input_ctrl_ebd_q.m_size;
    tot_qsize += pThis->m_input_q.m_size;

    if ( 0 == tot_qsize )
    {
        DEBUG_DETAIL("IN-->BREAKING FROM IN LOOP");
        pthread_mutex_unlock(&pThis->m_lock);
        return;
    }

    if ( (state != OMX_StateExecuting) && ! (pThis->m_input_ctrl_cmd_q.m_size))
    {
        pthread_mutex_unlock(&pThis->m_lock);
        DEBUG_DETAIL("SLEEPING IN THREAD\n");
        pthread_mutex_lock(&pThis->m_in_th_lock_1);
        pThis->is_in_th_sleep = true;
        pthread_mutex_unlock(&pThis->m_in_th_lock_1);
        pThis->in_th_goto_sleep();

        /* Get the updated state */
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
    }
    else if ((state == OMX_StatePause))
    {
        if(!(pThis->m_input_ctrl_cmd_q.m_size))
        {
           pthread_mutex_unlock(&pThis->m_lock);

           DEBUG_DETAIL("IN: SLEEPING IN THREAD\n");
           pthread_mutex_lock(&pThis->m_in_th_lock_1);
           pThis->is_in_th_sleep = true;
           pthread_mutex_unlock(&pThis->m_in_th_lock_1);
           pThis->in_th_goto_sleep();

           pthread_mutex_lock(&pThis->m_state_lock);
           pThis->get_state(&pThis->m_cmp, &state);
           pthread_mutex_unlock(&pThis->m_state_lock);
        }
    }

    qsize = pThis->m_input_ctrl_cmd_q.m_size;
    tot_qsize = qsize;
    tot_qsize += pThis->m_input_ctrl_ebd_q.m_size;
    tot_qsize += pThis->m_input_q.m_size;

    DEBUG_DETAIL("Input-->QSIZE-flush=%d,ebd=%d QSIZE=%d state=%d\n",\
        pThis->m_input_ctrl_cmd_q.m_size,
        pThis->m_input_ctrl_ebd_q.m_size,
        pThis->m_input_q.m_size, state);


    if ( qsize )
    {
        // process FLUSH message
        pThis->m_input_ctrl_cmd_q.pop_entry(&p1,&p2,&ident);
    } else if ( (qsize = pThis->m_input_ctrl_ebd_q.m_size) &&
        (state == OMX_StateExecuting) )
    {
        // then process EBD's
        pThis->m_input_ctrl_ebd_q.pop_entry(&p1,&p2,&ident);
    } else if ((qsize = pThis->m_input_q.m_size) &&
               (state == OMX_StateExecuting))
    {
        // if no FLUSH and EBD's then process ETB's
        pThis->m_input_q.pop_entry(&p1, &p2, &ident);
    } else if ( state == OMX_StateLoaded )
    {
        pthread_mutex_unlock(&pThis->m_lock);
        DEBUG_PRINT("IN: ***in OMX_StateLoaded so exiting\n");
        return ;
    } else
    {
        qsize = 0;
        DEBUG_PRINT("IN-->state[%d]cmdq[%d]ebdq[%d]in[%d]\n",\
                             state,pThis->m_input_ctrl_cmd_q.m_size,
                             pThis->m_input_ctrl_ebd_q.m_size,
				pThis->m_input_q.m_size);

        if(state == OMX_StatePause)
        {
            DEBUG_DETAIL("IN: SLEEPING AGAIN IN THREAD\n");
            pthread_mutex_lock(&pThis->m_in_th_lock_1);
            pThis->is_in_th_sleep = true;
            pthread_mutex_unlock(&pThis->m_in_th_lock_1);
            pthread_mutex_unlock(&pThis->m_lock);
            pThis->in_th_goto_sleep();
            goto loopback_in;
        }
    }
    pthread_mutex_unlock(&pThis->m_lock);

    if ( qsize > 0 )
    {
        id = ident;
        DEBUG_DETAIL("Input->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d]\n",\
            pThis->m_state,
            ident,
            pThis->m_input_ctrl_cmd_q.m_size,
            pThis->m_input_ctrl_ebd_q.m_size,
            pThis->m_input_q.m_size);
        if ( OMX_COMPONENT_GENERATE_BUFFER_DONE == id )
        {
            pThis->buffer_done_cb((OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_EOS)
        {
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                OMX_EventBufferFlag, 0, 1, NULL );
        } else if ( OMX_COMPONENT_GENERATE_ETB == id )
        {
            pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,
                (OMX_BUFFERHEADERTYPE *)p2);
        } else if ( OMX_COMPONENT_GENERATE_COMMAND == id )
        {
            // Execute FLUSH command
            if ( OMX_CommandFlush == p1 )
            {
                DEBUG_DETAIL(" Executing FLUSH command on Input port\n");
                pThis->execute_input_omx_flush();
            } else
            {
                DEBUG_DETAIL("Invalid command[%lu]\n",p1);
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("ERROR:IN-->Invalid Id[%d]\n",id);
        }
    } else
    {
        DEBUG_DETAIL("ERROR:IN-->Empty INPUT Q\n");
    }
    return;
}

/**
 @brief member function for performing component initialization

 @param role C string mandating role of this component
 @return Error status
 */
OMX_ERRORTYPE omx_aac_aenc::component_init(OMX_STRING role)
{

    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    m_state                   = OMX_StateLoaded;

    /* DSP does not give information about the bitstream
    randomly assign the value right now. Query will result in
    incorrect param */
    memset(&m_aac_param, 0, sizeof(m_aac_param));
    m_aac_param.nSize = (OMX_U32)sizeof(m_aac_param);
    m_aac_param.nChannels = DEFAULT_CH_CFG;
    m_aac_param.nSampleRate = DEFAULT_SF;
    m_aac_param.nBitRate = DEFAULT_BITRATE;
    m_volume = OMX_AAC_DEFAULT_VOL;             /* Close to unity gain */
    memset(&m_aac_pb_stats,0,sizeof(AAC_PB_STATS));
    memset(&m_pcm_param, 0, sizeof(m_pcm_param));
    m_pcm_param.nSize = (OMX_U32)sizeof(m_pcm_param);
    m_pcm_param.nChannels = DEFAULT_CH_CFG;
    m_pcm_param.nSamplingRate = DEFAULT_SF;

    nTimestamp = 0;
    ts = 0;
    m_frame_count = 0;
    frameduration = 0;
    nNumInputBuf = 0;
    nNumOutputBuf = 0;
    m_ipc_to_in_th = NULL;  // Command server instance
    m_ipc_to_out_th = NULL;  // Client server instance
    m_ipc_to_cmd_th = NULL;  // command instance
    m_is_out_th_sleep = 0;
    m_is_in_th_sleep = 0;
    is_out_th_sleep= false;

    is_in_th_sleep=false;
    adif_flag = 0;
    mp4ff_flag = 0;
    memset(&m_priority_mgm, 0, sizeof(m_priority_mgm));
    m_priority_mgm.nGroupID =0;
    m_priority_mgm.nGroupPriority=0;

    memset(&m_buffer_supplier, 0, sizeof(m_buffer_supplier));
    m_buffer_supplier.nPortIndex=OMX_BufferSupplyUnspecified;

    DEBUG_PRINT_ERROR(" component init: role = %s\n",role);

    DEBUG_PRINT(" component init: role = %s\n",role);
    component_Role.nVersion.nVersion = OMX_SPEC_VERSION;
    if (!strcmp(role,"OMX.qcom.audio.encoder.aac"))
    {
        pcm_input = 1;
        component_Role.nSize = (OMX_U32)sizeof(role);
        strlcpy((char *)component_Role.cRole, (const char*)role,
		sizeof(component_Role.cRole));
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
    } else if (!strcmp(role,"OMX.qcom.audio.encoder.tunneled.aac"))
    {
        pcm_input = 0;
        component_Role.nSize = (OMX_U32)sizeof(role);
        strlcpy((char *)component_Role.cRole, (const char*)role,
		sizeof(component_Role.cRole));
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
    } else
    {
        component_Role.nSize = (OMX_U32)sizeof("\0");
        strlcpy((char *)component_Role.cRole, (const char*)"\0",
		sizeof(component_Role.cRole));
        DEBUG_PRINT_ERROR("\ncomponent_init: Component %s LOADED is invalid\n",
				role);
        return OMX_ErrorInsufficientResources;
    }
    if(pcm_input)
    {
        m_tmp_meta_buf = (OMX_U8*) malloc(sizeof(OMX_U8) *
                         (OMX_CORE_INPUT_BUFFER_SIZE + sizeof(META_IN)));

        if (m_tmp_meta_buf == NULL) {
            DEBUG_PRINT_ERROR("Mem alloc failed for in meta buf\n");
            return OMX_ErrorInsufficientResources;
	}
    }
    m_tmp_out_meta_buf =
	(OMX_U8*)malloc(sizeof(OMX_U8)*OMX_AAC_OUTPUT_BUFFER_SIZE);
        if ( m_tmp_out_meta_buf == NULL ) {
            DEBUG_PRINT_ERROR("Mem alloc failed for out meta buf\n");
            return OMX_ErrorInsufficientResources;
	}

    if(0 == pcm_input)
    {
        m_drv_fd = open("/dev/msm_aac_in",O_RDONLY);
    }
    else
    {
        m_drv_fd = open("/dev/msm_aac_in",O_RDWR);
    }
    if (m_drv_fd < 0)
    {
        DEBUG_PRINT_ERROR("Component_init Open Failed[%d] errno[%d]",\
                                      m_drv_fd,errno);

        return OMX_ErrorInsufficientResources;
    }
    if(ioctl(m_drv_fd, AUDIO_GET_SESSION_ID,&m_session_id) == -1)
    {
        DEBUG_PRINT_ERROR("AUDIO_GET_SESSION_ID FAILED\n");
    }
    if(pcm_input)
    {
        if (!m_ipc_to_in_th)
        {
            m_ipc_to_in_th = omx_aac_thread_create(process_in_port_msg,
                this, (char *)"INPUT_THREAD");
            if (!m_ipc_to_in_th)
            {
                DEBUG_PRINT_ERROR("ERROR!!! Failed to start \
				Input port thread\n");
                return OMX_ErrorInsufficientResources;
            }
        }
    }

    if (!m_ipc_to_cmd_th)
    {
        m_ipc_to_cmd_th = omx_aac_thread_create(process_command_msg,
            this, (char *)"CMD_THREAD");
        if (!m_ipc_to_cmd_th)
        {
            DEBUG_PRINT_ERROR("ERROR!!!Failed to start "
                              "command message thread\n");
            return OMX_ErrorInsufficientResources;
        }
    }

        if (!m_ipc_to_out_th)
        {
            m_ipc_to_out_th = omx_aac_thread_create(process_out_port_msg,
                this, (char *)"OUTPUT_THREAD");
            if (!m_ipc_to_out_th)
            {
                DEBUG_PRINT_ERROR("ERROR!!! Failed to start output "
                                  "port thread\n");
                return OMX_ErrorInsufficientResources;
            }
        }
    return eRet;
}

/**

 @brief member function to retrieve version of component



 @param hComp handle to this component instance
 @param componentName name of component
 @param componentVersion  pointer to memory space which stores the
       version number
 @param specVersion pointer to memory sapce which stores version of
        openMax specification
 @param componentUUID
 @return Error status
 */
OMX_ERRORTYPE  omx_aac_aenc::get_component_version
(
    OMX_IN OMX_HANDLETYPE               hComp,
    OMX_OUT OMX_STRING          componentName,
    OMX_OUT OMX_VERSIONTYPE* componentVersion,
    OMX_OUT OMX_VERSIONTYPE*      specVersion,
    OMX_OUT OMX_UUIDTYPE*       componentUUID)
{
    if((hComp == NULL) || (componentName == NULL) ||
        (specVersion == NULL) || (componentUUID == NULL))
    {
        componentVersion = NULL;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Comp Version in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    componentVersion->nVersion = OMX_SPEC_VERSION;
    specVersion->nVersion = OMX_SPEC_VERSION;
    return OMX_ErrorNone;
}
/**
  @brief member function handles command from IL client

  This function simply queue up commands from IL client.
  Commands will be processed in command server thread context later

  @param hComp handle to component instance
  @param cmd type of command
  @param param1 parameters associated with the command type
  @param cmdData
  @return Error status
*/
OMX_ERRORTYPE  omx_aac_aenc::send_command(OMX_IN OMX_HANDLETYPE hComp,
                                           OMX_IN OMX_COMMANDTYPE  cmd,
                                           OMX_IN OMX_U32       param1,
                                           OMX_IN OMX_PTR      cmdData)
{
    int portIndex = (int)param1;

    if(hComp == NULL)
    {
        cmdData = cmdData;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (OMX_StateInvalid == m_state)
    {
        return OMX_ErrorInvalidState;
    }
    if ( (cmd == OMX_CommandFlush) && (portIndex > 1) )
    {
        return OMX_ErrorBadPortIndex;
    }
    post_command((unsigned)cmd,(unsigned)param1,OMX_COMPONENT_GENERATE_COMMAND);
    DEBUG_PRINT("Send Command : returns with OMX_ErrorNone \n");
    DEBUG_PRINT("send_command : recieved state before semwait= %u\n",param1);
    sem_wait (&sem_States);
    DEBUG_PRINT("send_command : recieved state after semwait\n");
    return OMX_ErrorNone;
}

/**
 @brief member function performs actual processing of commands excluding
  empty buffer call

 @param hComp handle to component
 @param cmd command type
 @param param1 parameter associated with the command
 @param cmdData

 @return error status
*/
OMX_ERRORTYPE  omx_aac_aenc::send_command_proxy(OMX_IN OMX_HANDLETYPE hComp,
                                                 OMX_IN OMX_COMMANDTYPE  cmd,
                                                 OMX_IN OMX_U32       param1,
                                                 OMX_IN OMX_PTR      cmdData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    //   Handle only IDLE and executing
    OMX_STATETYPE eState = (OMX_STATETYPE) param1;
    int bFlag = 1;
    nState = eState;

    if(hComp == NULL)
    {
        cmdData = cmdData;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (OMX_CommandStateSet == cmd)
    {
        /***************************/
        /* Current State is Loaded */
        /***************************/
        if (OMX_StateLoaded == m_state)
        {
            if (OMX_StateIdle == eState)
            {

                 if (allocate_done() ||
                        (m_inp_bEnabled == OMX_FALSE
                         && m_out_bEnabled == OMX_FALSE))
                 {
                       DEBUG_PRINT("SCP-->Allocate Done Complete\n");
                 }
                 else
                 {
                        DEBUG_PRINT("SCP-->Loaded to Idle-Pending\n");
                        BITMASK_SET(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                        bFlag = 0;
                 }

            } else if (eState == OMX_StateLoaded)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Loaded\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorSameState,
                                  0, NULL );
                eRet = OMX_ErrorSameState;
            }

            else if (eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->WaitForResources\n");
                eRet = OMX_ErrorNone;
            }

            else if (eState == OMX_StateExecuting)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Executing\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorIncorrectStateTransition,
                                  0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }

            else if (eState == OMX_StatePause)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Pause\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorIncorrectStateTransition,
                                  0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }

            else if (eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Invalid\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorInvalidState,
                                  0, NULL );
                m_state = OMX_StateInvalid;
                eRet = OMX_ErrorInvalidState;
            } else
            {
                DEBUG_PRINT_ERROR("SCP-->Loaded to Invalid(%d))\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

        /***************************/
        /* Current State is IDLE */
        /***************************/
        else if (OMX_StateIdle == m_state)
        {
            if (OMX_StateLoaded == eState)
            {
                if (release_done(-1))
                {
                   if (ioctl(m_drv_fd, AUDIO_STOP, 0) == -1)
                    {
                        DEBUG_PRINT_ERROR("SCP:Idle->Loaded,ioctl \
					stop failed %d\n", errno);
                    }
                    nTimestamp=0;
                    ts = 0; 
                    frameduration = 0;
                    DEBUG_PRINT("SCP-->Idle to Loaded\n");
                } else
                {
                    DEBUG_PRINT("SCP--> Idle to Loaded-Pending\n");
                    BITMASK_SET(&m_flags, OMX_COMPONENT_LOADING_PENDING);
                    // Skip the event notification
                    bFlag = 0;
                }
            }
            else if (OMX_StateExecuting == eState)
            {

                struct msm_audio_aac_enc_config drv_aac_enc_config;
                struct msm_audio_aac_config drv_aac_config;
                struct msm_audio_stream_config drv_stream_config;
                struct msm_audio_buf_cfg buf_cfg;
                struct msm_audio_config pcm_cfg;
                if(ioctl(m_drv_fd, AUDIO_GET_STREAM_CONFIG, &drv_stream_config)
			== -1)
                {
                    DEBUG_PRINT_ERROR("ioctl AUDIO_GET_STREAM_CONFIG failed, \
					errno[%d]\n", errno);
                }
                if(ioctl(m_drv_fd, AUDIO_SET_STREAM_CONFIG, &drv_stream_config)
			== -1)
                {
                    DEBUG_PRINT_ERROR("ioctl AUDIO_SET_STREAM_CONFIG failed, \
					errno[%d]\n", errno);
                }

                if(ioctl(m_drv_fd, AUDIO_GET_AAC_ENC_CONFIG,
			&drv_aac_enc_config) == -1)
                {
                    DEBUG_PRINT_ERROR("ioctl AUDIO_GET_AAC_ENC_CONFIG failed, \
					errno[%d]\n", errno);
                }
                drv_aac_enc_config.channels = m_aac_param.nChannels;
                drv_aac_enc_config.sample_rate = m_aac_param.nSampleRate;
                drv_aac_enc_config.bit_rate =
                get_updated_bit_rate(m_aac_param.nBitRate);
                DEBUG_PRINT("aac config %u,%u,%u %d updated bitrate %d\n",
                            m_aac_param.nChannels,m_aac_param.nSampleRate,
			    m_aac_param.nBitRate,m_aac_param.eAACStreamFormat,
                            drv_aac_enc_config.bit_rate);
                switch(m_aac_param.eAACStreamFormat)
                {

                    case 0:
                    case 1:
                    {
                        drv_aac_enc_config.stream_format = 65535;
                        DEBUG_PRINT("Setting AUDIO_AAC_FORMAT_ADTS\n");
                        break;
                    }
                    case 4:
                    case 5:
                    case 6:
                    {
                        drv_aac_enc_config.stream_format = AUDIO_AAC_FORMAT_RAW;
                        DEBUG_PRINT("Setting AUDIO_AAC_FORMAT_RAW\n");
                        break;
                    }
                    default:
                           break;
                }
                DEBUG_PRINT("Stream format = %d\n",
				drv_aac_enc_config.stream_format);
                if(ioctl(m_drv_fd, AUDIO_SET_AAC_ENC_CONFIG,
			&drv_aac_enc_config) == -1)
                {
                    DEBUG_PRINT_ERROR("ioctl AUDIO_SET_AAC_ENC_CONFIG failed, \
					errno[%d]\n", errno);
                }
                if (ioctl(m_drv_fd, AUDIO_GET_AAC_CONFIG, &drv_aac_config)
			== -1) {
                    DEBUG_PRINT_ERROR("ioctl AUDIO_GET_AAC_CONFIG failed, \
					errno[%d]\n", errno);
                }

                drv_aac_config.sbr_on_flag = 0;
                drv_aac_config.sbr_ps_on_flag = 0;
				/* Other members of drv_aac_config are not used,
				 so not setting them */
                switch(m_aac_param.eAACProfile)
                {
                    case OMX_AUDIO_AACObjectLC:
                    {
                        DEBUG_PRINT("AAC_Profile: OMX_AUDIO_AACObjectLC\n");
                        drv_aac_config.sbr_on_flag = 0;
                        drv_aac_config.sbr_ps_on_flag = 0;
                        break;
                    }
                    case OMX_AUDIO_AACObjectHE:
                    {
                        DEBUG_PRINT("AAC_Profile: OMX_AUDIO_AACObjectHE\n");
                        drv_aac_config.sbr_on_flag = 1;
                        drv_aac_config.sbr_ps_on_flag = 0;
                        break;
                    }
                    case OMX_AUDIO_AACObjectHE_PS:
                    {
                        DEBUG_PRINT("AAC_Profile: OMX_AUDIO_AACObjectHE_PS\n");
                        drv_aac_config.sbr_on_flag = 1;
                        drv_aac_config.sbr_ps_on_flag = 1;
                        break;
                    }
                    default:
                    {
                        DEBUG_PRINT_ERROR("Unsupported AAC Profile Type = %d\n",
						m_aac_param.eAACProfile);
                        break;
                    }
                }
                DEBUG_PRINT("sbr_flag = %d, sbr_ps_flag = %d\n",
				drv_aac_config.sbr_on_flag,
				drv_aac_config.sbr_ps_on_flag);

                if (ioctl(m_drv_fd, AUDIO_SET_AAC_CONFIG, &drv_aac_config)
			== -1) {
                    DEBUG_PRINT_ERROR("ioctl AUDIO_SET_AAC_CONFIG failed, \
					errno[%d]\n", errno);
                }

                if (ioctl(m_drv_fd, AUDIO_GET_BUF_CFG, &buf_cfg) == -1)
                {
                    DEBUG_PRINT_ERROR("ioctl AUDIO_GET_BUF_CFG, errno[%d]\n",
					errno);
                }
                buf_cfg.meta_info_enable = 1;
                buf_cfg.frames_per_buf = NUMOFFRAMES;
                if (ioctl(m_drv_fd, AUDIO_SET_BUF_CFG, &buf_cfg) == -1)
                {
                    DEBUG_PRINT_ERROR("ioctl AUDIO_SET_BUF_CFG, errno[%d]\n",
					errno);
                }
                if(pcm_input)
                {
                    if (ioctl(m_drv_fd, AUDIO_GET_CONFIG, &pcm_cfg) == -1)
                    {
                        DEBUG_PRINT_ERROR("ioctl AUDIO_GET_CONFIG, errno[%d]\n",
					errno);
                    }
                    pcm_cfg.channel_count = m_pcm_param.nChannels;
                    pcm_cfg.sample_rate  =  m_pcm_param.nSamplingRate;
                    pcm_cfg.buffer_size =  input_buffer_size;
                    pcm_cfg.buffer_count =  m_inp_current_buf_count;
                    DEBUG_PRINT("pcm config %u %u\n",m_pcm_param.nChannels,
				m_pcm_param.nSamplingRate);

                    if (ioctl(m_drv_fd, AUDIO_SET_CONFIG, &pcm_cfg) == -1)
                    {
                        DEBUG_PRINT_ERROR("ioctl AUDIO_SET_CONFIG, errno[%d]\n",
						errno);
                    }
                }
                if(ioctl(m_drv_fd, AUDIO_START, 0) == -1)
                {
                    DEBUG_PRINT_ERROR("ioctl AUDIO_START failed, errno[%d]\n",
					errno);
                    m_state = OMX_StateInvalid;
                    this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorInvalidState,
                                        0, NULL );
                    eRet = OMX_ErrorInvalidState;
                }
                DEBUG_PRINT("SCP-->Idle to Executing\n");
                nState = eState;
                frameduration = (1024*1000000)/m_aac_param.nSampleRate;
            } else if (eState == OMX_StateIdle)
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->Idle\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorSameState,
                                  0, NULL );
                eRet = OMX_ErrorSameState;
            } else if (eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->WaitForResources\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }

            else if (eState == OMX_StatePause)
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->Pause\n");
            }

            else if (eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->Invalid\n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp,
                                        this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            } else
            {
                DEBUG_PRINT_ERROR("SCP--> Idle to %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

        /******************************/
        /* Current State is Executing */
        /******************************/
        else if (OMX_StateExecuting == m_state)
        {
            if (OMX_StateIdle == eState)
            {
                DEBUG_PRINT("SCP-->Executing to Idle \n");
                if(pcm_input)
                    execute_omx_flush(-1,false);
                else
                    execute_omx_flush(1,false);


            } else if (OMX_StatePause == eState)
            {
                DEBUG_DETAIL("*************************\n");
                DEBUG_PRINT("SCP-->RXED PAUSE STATE\n");
                DEBUG_DETAIL("*************************\n");
                //ioctl(m_drv_fd, AUDIO_PAUSE, 0);
            } else if (eState == OMX_StateLoaded)
            {
                DEBUG_PRINT("\n OMXCORE-SM: Executing --> Loaded \n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            } else if (eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("\n OMXCORE-SM: Executing --> WaitForResources \n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            } else if (eState == OMX_StateExecuting)
            {
                DEBUG_PRINT("\n OMXCORE-SM: Executing --> Executing \n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorSameState,
                                        0, NULL );
                eRet = OMX_ErrorSameState;
            } else if (eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("\n OMXCORE-SM: Executing --> Invalid \n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            } else
            {
                DEBUG_PRINT_ERROR("SCP--> Executing to %d Not Handled\n",
				eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /***************************/
        /* Current State is Pause  */
        /***************************/
        else if (OMX_StatePause == m_state)
        {
            if( (eState == OMX_StateExecuting || eState == OMX_StateIdle) )
            {
                pthread_mutex_lock(&m_out_th_lock_1);
                if(is_out_th_sleep)
                {
                    DEBUG_DETAIL("PE: WAKING UP OUT THREAD\n");
                    is_out_th_sleep = false;
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
            }
            if ( OMX_StateExecuting == eState )
            {
                nState = eState;
            } else if ( OMX_StateIdle == eState )
            {
                DEBUG_PRINT("SCP-->Paused to Idle \n");
                DEBUG_PRINT ("\n Internal flush issued");
                pthread_mutex_lock(&m_flush_lock);
                m_flush_cnt = 2;
                pthread_mutex_unlock(&m_flush_lock);
                if(pcm_input)
                    execute_omx_flush(-1,false);
                else
                    execute_omx_flush(1,false);

            } else if ( eState == OMX_StateLoaded )
            {
                DEBUG_PRINT("\n Pause --> loaded \n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError,
					OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            } else if (eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("\n Pause --> WaitForResources \n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError,
					OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            } else if (eState == OMX_StatePause)
            {
                DEBUG_PRINT("\n Pause --> Pause \n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorSameState,
                                        0, NULL );
                eRet = OMX_ErrorSameState;
            } else if (eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("\n Pause --> Invalid \n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            } else
            {
                DEBUG_PRINT("SCP-->Paused to %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /**************************************/
        /* Current State is WaitForResources  */
        /**************************************/
        else if (m_state == OMX_StateWaitForResources)
        {
            if (eState == OMX_StateLoaded)
            {
                DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Loaded\n");
            } else if (eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("OMXCORE-SM: \
				WaitForResources-->WaitForResources\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorSameState,
                                        0, NULL );
                eRet = OMX_ErrorSameState;
            } else if (eState == OMX_StateExecuting)
            {
                DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Executing\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            } else if (eState == OMX_StatePause)
            {
                DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Pause\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            } else if (eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Invalid\n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            } else
            {
                DEBUG_PRINT_ERROR("SCP--> %d to %d(Not Handled)\n",
					m_state,eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /****************************/
        /* Current State is Invalid */
        /****************************/
        else if (m_state == OMX_StateInvalid)
        {
            if (OMX_StateLoaded == eState || OMX_StateWaitForResources == eState
                || OMX_StateIdle == eState || OMX_StateExecuting == eState
                || OMX_StatePause == eState || OMX_StateInvalid == eState)
            {
                DEBUG_PRINT("OMXCORE-SM: Invalid-->Loaded/Idle/Executing"
                            "/Pause/Invalid/WaitForResources\n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            }
        } else
        {
            DEBUG_PRINT_ERROR("OMXCORE-SM: %d --> %d(Not Handled)\n",\
                              m_state,eState);
            eRet = OMX_ErrorBadParameter;
        }
    } else if (OMX_CommandFlush == cmd)
    {
        DEBUG_DETAIL("*************************\n");
        DEBUG_PRINT("SCP-->RXED FLUSH COMMAND port=%u\n",param1);
        DEBUG_DETAIL("*************************\n");
        bFlag = 0;
        if ( param1 == OMX_CORE_INPUT_PORT_INDEX ||
             param1 == OMX_CORE_OUTPUT_PORT_INDEX ||
            (signed)param1 == -1 )
        {
            execute_omx_flush(param1);
        } else
        {
            eRet = OMX_ErrorBadPortIndex;
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventError,
                OMX_CommandFlush, OMX_ErrorBadPortIndex, NULL );
        }
    } else if ( cmd == OMX_CommandPortDisable )
    {
        bFlag = 0;
        if ( param1 == OMX_CORE_INPUT_PORT_INDEX || param1 == OMX_ALL )
        {
            DEBUG_PRINT("SCP: Disabling Input port Indx\n");
            m_inp_bEnabled = OMX_FALSE;
            if ( (m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
                && release_done(0) )
            {
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortDisable:\
                            OMX_CORE_INPUT_PORT_INDEX:release_done \n");
                DEBUG_PRINT("************* OMX_CommandPortDisable:\
                            m_inp_bEnabled=%d********\n",m_inp_bEnabled);

                post_command(OMX_CommandPortDisable,
                             OMX_CORE_INPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            }

            else
            {
                if (m_state == OMX_StatePause ||m_state == OMX_StateExecuting)
                {
                    DEBUG_PRINT("SCP: execute_omx_flush in Disable in "\
                                " param1=%u m_state=%d \n",param1, m_state);
                    execute_omx_flush(param1);
                }
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortDisable:\
                            OMX_CORE_INPUT_PORT_INDEX \n");
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
                // Skip the event notification

            }

        }
        if (param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL)
        {

            DEBUG_PRINT("SCP: Disabling Output port Indx\n");
            m_out_bEnabled = OMX_FALSE;
            if ((m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
                && release_done(1))
            {
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortDisable:\
                            OMX_CORE_OUTPUT_PORT_INDEX:release_done \n");
                DEBUG_PRINT("************* OMX_CommandPortDisable:\
                            m_out_bEnabled=%d********\n",m_inp_bEnabled);

                post_command(OMX_CommandPortDisable,
                             OMX_CORE_OUTPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            } else
            {
                if (m_state == OMX_StatePause ||m_state == OMX_StateExecuting)
                {
                    DEBUG_PRINT("SCP: execute_omx_flush in Disable out "\
                                "param1=%u m_state=%d \n",param1, m_state);
                    execute_omx_flush(param1);
                }
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
                // Skip the event notification

            }
        } else
        {
            DEBUG_PRINT_ERROR("OMX_CommandPortDisable: disable wrong port ID");
        }

    } else if (cmd == OMX_CommandPortEnable)
    {
        bFlag = 0;
        if (param1 == OMX_CORE_INPUT_PORT_INDEX  || param1 == OMX_ALL)
        {
            m_inp_bEnabled = OMX_TRUE;
            DEBUG_PRINT("SCP: Enabling Input port Indx\n");
            if ((m_state == OMX_StateLoaded
                 && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                || (m_state == OMX_StateWaitForResources)
                || (m_inp_bPopulated == OMX_TRUE))
            {
                post_command(OMX_CommandPortEnable,
                             OMX_CORE_INPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);


            } else
            {
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
                // Skip the event notification

            }
        }

        if (param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL)
        {
            DEBUG_PRINT("SCP: Enabling Output port Indx\n");
            m_out_bEnabled = OMX_TRUE;
            if ((m_state == OMX_StateLoaded
                 && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                || (m_state == OMX_StateWaitForResources)
                || (m_out_bPopulated == OMX_TRUE))
            {
                post_command(OMX_CommandPortEnable,
                             OMX_CORE_OUTPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            } else
            {
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortEnable:\
                            OMX_CORE_OUTPUT_PORT_INDEX:release_done \n");
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                // Skip the event notification

            }
            pthread_mutex_lock(&m_in_th_lock_1);
            if(is_in_th_sleep)
            {
                    is_in_th_sleep = false;
                    DEBUG_DETAIL("SCP:WAKING UP IN THREADS\n");
                    in_th_wakeup();
            }
            pthread_mutex_unlock(&m_in_th_lock_1);
            pthread_mutex_lock(&m_out_th_lock_1);
            if (is_out_th_sleep)
            {
                is_out_th_sleep = false;
                DEBUG_PRINT("SCP:WAKING OUT THR, OMX_CommandPortEnable\n");
                out_th_wakeup();
            }
            pthread_mutex_unlock(&m_out_th_lock_1);
        } else
        {
            DEBUG_PRINT_ERROR("OMX_CommandPortEnable: disable wrong port ID");
        }

    } else
    {
        DEBUG_PRINT_ERROR("SCP-->ERROR: Invali Command [%d]\n",cmd);
        eRet = OMX_ErrorNotImplemented;
    }
    DEBUG_PRINT("posting sem_States\n");
    sem_post (&sem_States);
    if (eRet == OMX_ErrorNone && bFlag)
    {
        post_command(cmd,eState,OMX_COMPONENT_GENERATE_EVENT);
    }
    return eRet;
}

/*=============================================================================
FUNCTION:
  execute_omx_flush

DESCRIPTION:
  Function that flushes buffers that are pending to be written to driver

INPUT/OUTPUT PARAMETERS:
  [IN] param1
  [IN] cmd_cmpl

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_aac_aenc::execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl)
{
    bool bRet = true;

    DEBUG_PRINT("Execute_omx_flush Port[%u]", param1);
    struct timespec abs_timeout;
    abs_timeout.tv_sec = 1;
    abs_timeout.tv_nsec = 0;

    if ((signed)param1 == -1)
    {
        bFlushinprogress = true;
        DEBUG_PRINT("Execute flush for both I/p O/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 2;
        pthread_mutex_unlock(&m_flush_lock);

        // Send Flush commands to input and output threads
        post_input(OMX_CommandFlush,
                   OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        post_output(OMX_CommandFlush,
                    OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        // Send Flush to the kernel so that the in and out buffers are released
        if (ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1)
            DEBUG_PRINT_ERROR("FLush:ioctl flush failed errno=%d\n",errno);
        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
                     is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");

        pthread_mutex_lock(&m_in_th_lock_1);
        if (is_in_th_sleep)
        {
            is_in_th_sleep = false;
            DEBUG_DETAIL("For FLUSH-->WAKING UP IN THREADS\n");
            in_th_wakeup();
        }
        pthread_mutex_unlock(&m_in_th_lock_1);

        pthread_mutex_lock(&m_out_th_lock_1);
        if (is_out_th_sleep)
        {
            is_out_th_sleep = false;
            DEBUG_DETAIL("For FLUSH-->WAKING UP OUT THREADS\n");
            out_th_wakeup();
        }
        pthread_mutex_unlock(&m_out_th_lock_1);

        // sleep till the FLUSH ACK are done by both the input and
        // output threads
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%d",param1);
        wait_for_event();

        DEBUG_PRINT("RECIEVED BOTH FLUSH ACK's param1=%u cmd_cmpl=%d",\
                    param1,cmd_cmpl);

        // If not going to idle state, Send FLUSH complete message 
	// to the Client, now that FLUSH ACK's have been recieved.
        if (cmd_cmpl)
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                              OMX_CommandFlush, OMX_CORE_INPUT_PORT_INDEX,
				NULL );
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                              OMX_CommandFlush, OMX_CORE_OUTPUT_PORT_INDEX,
				NULL );
            DEBUG_PRINT("Inside FLUSH.. sending FLUSH CMPL\n");
        }
        bFlushinprogress = false;
    }
    else if (param1 == OMX_CORE_INPUT_PORT_INDEX)
    {
        DEBUG_PRINT("Execute FLUSH for I/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 1;
        pthread_mutex_unlock(&m_flush_lock);
        post_input(OMX_CommandFlush,
                   OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        if (ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1)
            DEBUG_PRINT_ERROR("Flush:Input port, ioctl flush failed %d\n",
				errno);
        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
                     is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");

        if (is_in_th_sleep)
        {
            pthread_mutex_lock(&m_in_th_lock_1);
            is_in_th_sleep = false;
            pthread_mutex_unlock(&m_in_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP IN THREADS\n");
            in_th_wakeup();
        }

        if (is_out_th_sleep)
        {
            pthread_mutex_lock(&m_out_th_lock_1);
            is_out_th_sleep = false;
            pthread_mutex_unlock(&m_out_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP OUT THREADS\n");
            out_th_wakeup();
        }

        //sleep till the FLUSH ACK are done by both the input and output threads
        DEBUG_DETAIL("Executing FLUSH for I/p port\n");
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%d",param1);
        wait_for_event();
        DEBUG_DETAIL(" RECIEVED FLUSH ACK FOR I/P PORT param1=%d",param1);

        // Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if (cmd_cmpl)
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                              OMX_CommandFlush, OMX_CORE_INPUT_PORT_INDEX,
				NULL );
        }
    } else if (OMX_CORE_OUTPUT_PORT_INDEX == param1)
    {
        DEBUG_PRINT("Executing FLUSH for O/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 1;
        pthread_mutex_unlock(&m_flush_lock);
        DEBUG_DETAIL("Executing FLUSH for O/p port\n");
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%d",param1);
        post_output(OMX_CommandFlush,
                    OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        if (ioctl( m_drv_fd, AUDIO_FLUSH, 0) ==-1)
            DEBUG_PRINT_ERROR("Flush:Output port, ioctl flush failed %d\n",
				errno);
        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
                     is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");
        if (is_in_th_sleep)
        {
            pthread_mutex_lock(&m_in_th_lock_1);
            is_in_th_sleep = false;
            pthread_mutex_unlock(&m_in_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP IN THREADS\n");
            in_th_wakeup();
        }

        if (is_out_th_sleep)
        {
            pthread_mutex_lock(&m_out_th_lock_1);
            is_out_th_sleep = false;
            pthread_mutex_unlock(&m_out_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP OUT THREADS\n");
            out_th_wakeup();
        }

        // sleep till the FLUSH ACK are done by both the input
	// and output threads
        wait_for_event();
        // Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if (cmd_cmpl)
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                              OMX_CommandFlush, OMX_CORE_OUTPUT_PORT_INDEX,
				NULL );
        }
        DEBUG_DETAIL("RECIEVED FLUSH ACK FOR O/P PORT param1=%d",param1);
    } else
    {
        DEBUG_PRINT("Invalid Port ID[%u]",param1);
    }
    return bRet;
}

/*=============================================================================
FUNCTION:
  execute_input_omx_flush

DESCRIPTION:
  Function that flushes buffers that are pending to be written to driver

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_aac_aenc::execute_input_omx_flush()
{
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned long p1 = 0;                            // Parameter - 1
    unsigned long p2 = 0;                            // Parameter - 2
    unsigned char ident = 0;
    unsigned      qsize=0;                       // qsize
    unsigned      tot_qsize=0;                   // qsize

    DEBUG_PRINT("Execute_omx_flush on input port");

    pthread_mutex_lock(&m_lock);
    do
    {
        qsize = m_input_q.m_size;
        tot_qsize = qsize;
        tot_qsize += m_input_ctrl_ebd_q.m_size;

        DEBUG_DETAIL("Input FLUSH-->flushq[%d] ebd[%d]dataq[%d]",\
                     m_input_ctrl_cmd_q.m_size,
                     m_input_ctrl_ebd_q.m_size,qsize);
        if (!tot_qsize)
        {
            DEBUG_DETAIL("Input-->BREAKING FROM execute_input_flush LOOP");
            pthread_mutex_unlock(&m_lock);
            break;
        }
        if (qsize)
        {
            m_input_q.pop_entry(&p1, &p2, &ident);
            if ((ident == OMX_COMPONENT_GENERATE_ETB) ||
                (ident == OMX_COMPONENT_GENERATE_BUFFER_DONE))
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Flush:Input dataq=%p \n", omx_buf);
                omx_buf->nFilledLen = 0;
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
            }
        } else if (m_input_ctrl_ebd_q.m_size)
        {
            m_input_ctrl_ebd_q.pop_entry(&p1, &p2, &ident);
            if (ident == OMX_COMPONENT_GENERATE_BUFFER_DONE)
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                omx_buf->nFilledLen = 0;
                DEBUG_DETAIL("Flush:ctrl dataq=%p \n", omx_buf);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
            }
        } else
        {
        }
    }while (tot_qsize>0);
    DEBUG_DETAIL("*************************\n");
    DEBUG_DETAIL("IN-->FLUSHING DONE\n");
    DEBUG_DETAIL("*************************\n");
    flush_ack();
    pthread_mutex_unlock(&m_lock);
    return true;
}

/*=============================================================================
FUNCTION:
  execute_output_omx_flush

DESCRIPTION:
  Function that flushes buffers that are pending to be written to driver

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_aac_aenc::execute_output_omx_flush()
{
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned long p1 = 0;                            // Parameter - 1
    unsigned long p2 = 0;                            // Parameter - 2
    unsigned char ident = 0;
    unsigned      qsize=0;                       // qsize
    unsigned      tot_qsize=0;                   // qsize

    DEBUG_PRINT("Execute_omx_flush on output port");

    pthread_mutex_lock(&m_outputlock);
    do
    {
        qsize = m_output_q.m_size;
        DEBUG_DETAIL("OUT FLUSH-->flushq[%d] fbd[%d]dataq[%d]",\
                     m_output_ctrl_cmd_q.m_size,
                     m_output_ctrl_fbd_q.m_size,qsize);
        tot_qsize = qsize;
        tot_qsize += m_output_ctrl_fbd_q.m_size;
        if (!tot_qsize)
        {
            DEBUG_DETAIL("OUT-->BREAKING FROM execute_input_flush LOOP");
            pthread_mutex_unlock(&m_outputlock);
            break;
        }
        if (qsize)
        {
            m_output_q.pop_entry(&p1,&p2,&ident);
            if ( (OMX_COMPONENT_GENERATE_FTB == ident) ||
                 (OMX_COMPONENT_GENERATE_FRAME_DONE == ident))
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Ouput Buf_Addr=%p TS[0x%x] \n",\
                             omx_buf,nTimestamp);
                omx_buf->nTimeStamp = nTimestamp;
                omx_buf->nFilledLen = 0;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                DEBUG_DETAIL("CALLING FBD FROM FLUSH");
            }
        } else if ((qsize = m_output_ctrl_fbd_q.m_size))
        {
            m_output_ctrl_fbd_q.pop_entry(&p1, &p2, &ident);
            if (OMX_COMPONENT_GENERATE_FRAME_DONE == ident)
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Ouput Buf_Addr=%p TS[0x%x] \n", \
                             omx_buf,nTimestamp);
                omx_buf->nTimeStamp = nTimestamp;
                omx_buf->nFilledLen = 0;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                DEBUG_DETAIL("CALLING FROM CTRL-FBDQ FROM FLUSH");
            }
        }
    }while (qsize>0);
    DEBUG_DETAIL("*************************\n");
    DEBUG_DETAIL("OUT-->FLUSHING DONE\n");
    DEBUG_DETAIL("*************************\n");
    flush_ack();
    pthread_mutex_unlock(&m_outputlock);
    return true;
}

/*=============================================================================
FUNCTION:
  post_input

DESCRIPTION:
  Function that posts command in the command queue

INPUT/OUTPUT PARAMETERS:
  [IN] p1
  [IN] p2
  [IN] id - command ID
  [IN] lock - self-locking mode

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_aac_aenc::post_input(unsigned long p1,
                               unsigned long p2,
                               unsigned char id)
{
    bool bRet = false;
    pthread_mutex_lock(&m_lock);

    if((OMX_COMPONENT_GENERATE_COMMAND == id) || (id == OMX_COMPONENT_SUSPEND))
    {
        // insert flush message and ebd
        m_input_ctrl_cmd_q.insert_entry(p1,p2,id);
    } else if ((OMX_COMPONENT_GENERATE_BUFFER_DONE == id))
    {
        // insert ebd
        m_input_ctrl_ebd_q.insert_entry(p1,p2,id);
    } else
    {
        // ETBS in this queue
        m_input_q.insert_entry(p1,p2,id);
    }

    if (m_ipc_to_in_th)
    {
        bRet = true;
        omx_aac_post_msg(m_ipc_to_in_th, id);
    }

    DEBUG_DETAIL("PostInput-->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d] \n",\
                 m_state,
                 id,
                 m_input_ctrl_cmd_q.m_size,
                 m_input_ctrl_ebd_q.m_size,
                 m_input_q.m_size);

    pthread_mutex_unlock(&m_lock);
    return bRet;
}

/*=============================================================================
FUNCTION:
  post_command

DESCRIPTION:
  Function that posts command in the command queue

INPUT/OUTPUT PARAMETERS:
  [IN] p1
  [IN] p2
  [IN] id - command ID
  [IN] lock - self-locking mode

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_aac_aenc::post_command(unsigned int p1,
                                 unsigned int p2,
                                 unsigned char id)
{
    bool bRet  = false;

    pthread_mutex_lock(&m_commandlock);

    m_command_q.insert_entry(p1,p2,id);

    if (m_ipc_to_cmd_th)
    {
        bRet = true;
        omx_aac_post_msg(m_ipc_to_cmd_th, id);
    }

    DEBUG_DETAIL("PostCmd-->state[%d]id[%d]cmdq[%d]flags[%x]\n",\
                 m_state,
                 id,
                 m_command_q.m_size,
                 m_flags >> 3);

    pthread_mutex_unlock(&m_commandlock);
    return bRet;
}

/*=============================================================================
FUNCTION:
  post_output

DESCRIPTION:
  Function that posts command in the command queue

INPUT/OUTPUT PARAMETERS:
  [IN] p1
  [IN] p2
  [IN] id - command ID
  [IN] lock - self-locking mode

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_aac_aenc::post_output(unsigned long p1,
                                unsigned long p2,
                                unsigned char id)
{
    bool bRet = false;

    pthread_mutex_lock(&m_outputlock);
    if((OMX_COMPONENT_GENERATE_COMMAND == id) || (id == OMX_COMPONENT_SUSPEND)
        || (id == OMX_COMPONENT_RESUME))
    {
        // insert flush message and fbd
        m_output_ctrl_cmd_q.insert_entry(p1,p2,id);
    } else if ( (OMX_COMPONENT_GENERATE_FRAME_DONE == id) )
    {
        // insert flush message and fbd
        m_output_ctrl_fbd_q.insert_entry(p1,p2,id);
    } else
    {
        m_output_q.insert_entry(p1,p2,id);
    }
    if ( m_ipc_to_out_th )
    {
        bRet = true;
        omx_aac_post_msg(m_ipc_to_out_th, id);
    }
    DEBUG_DETAIL("PostOutput-->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d]\n",\
                 m_state,
                 id,
                 m_output_ctrl_cmd_q.m_size,
                 m_output_ctrl_fbd_q.m_size,
                 m_output_q.m_size);

    pthread_mutex_unlock(&m_outputlock);
    return bRet;
}
/**
  @brief member function that return parameters to IL client

  @param hComp handle to component instance
  @param paramIndex Parameter type
  @param paramData pointer to memory space which would hold the
        paramter
  @return error status
*/
OMX_ERRORTYPE  omx_aac_aenc::get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                            OMX_IN OMX_INDEXTYPE paramIndex,
                                            OMX_INOUT OMX_PTR     paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if (paramData == NULL)
    {
        DEBUG_PRINT("get_parameter: paramData is NULL\n");
        return OMX_ErrorBadParameter;
    }

    switch ((int)paramIndex)
    {
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
                portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

                DEBUG_PRINT("OMX_IndexParamPortDefinition " \
                            "portDefn->nPortIndex = %u\n",
				portDefn->nPortIndex);

                portDefn->nVersion.nVersion = OMX_SPEC_VERSION;
                portDefn->nSize = (OMX_U32)sizeof(portDefn);
                portDefn->eDomain    = OMX_PortDomainAudio;

                if (0 == portDefn->nPortIndex)
                {
                    portDefn->eDir       = OMX_DirInput;
                    portDefn->bEnabled   = m_inp_bEnabled;
                    portDefn->bPopulated = m_inp_bPopulated;
                    portDefn->nBufferCountActual = m_inp_act_buf_count;
                    portDefn->nBufferCountMin    = OMX_CORE_NUM_INPUT_BUFFERS;
                    portDefn->nBufferSize        = input_buffer_size;
                    portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
                    portDefn->format.audio.eEncoding = OMX_AUDIO_CodingPCM;
                    portDefn->format.audio.pNativeRender = 0;
                } else if (1 == portDefn->nPortIndex)
                {
                    portDefn->eDir =  OMX_DirOutput;
                    portDefn->bEnabled   = m_out_bEnabled;
                    portDefn->bPopulated = m_out_bPopulated;
                    portDefn->nBufferCountActual = m_out_act_buf_count;
                    portDefn->nBufferCountMin    = OMX_CORE_NUM_OUTPUT_BUFFERS;
                    portDefn->nBufferSize        = output_buffer_size;
                    portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
                    portDefn->format.audio.eEncoding = OMX_AUDIO_CodingAAC;
                    portDefn->format.audio.pNativeRender = 0;
                } else
                {
                    portDefn->eDir =  OMX_DirMax;
                    DEBUG_PRINT_ERROR("Bad Port idx %d\n",\
                                       (int)portDefn->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }

        case OMX_IndexParamAudioInit:
            {
                OMX_PORT_PARAM_TYPE *portParamType =
                (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT("OMX_IndexParamAudioInit\n");

                portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
                portParamType->nSize = (OMX_U32)sizeof(portParamType);
                portParamType->nPorts           = 2;
                portParamType->nStartPortNumber = 0;
                break;
            }

        case OMX_IndexParamAudioPortFormat:
            {
                OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
                (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
                DEBUG_PRINT("OMX_IndexParamAudioPortFormat\n");
                portFormatType->nVersion.nVersion = OMX_SPEC_VERSION;
                portFormatType->nSize = (OMX_U32)sizeof(portFormatType);

                if (OMX_CORE_INPUT_PORT_INDEX == portFormatType->nPortIndex)
                {

                    portFormatType->eEncoding = OMX_AUDIO_CodingPCM;
                } else if (OMX_CORE_OUTPUT_PORT_INDEX ==
				portFormatType->nPortIndex)
                {
                    DEBUG_PRINT("get_parameter: OMX_IndexParamAudioFormat: "\
                                "%u\n", portFormatType->nIndex);

                    portFormatType->eEncoding = OMX_AUDIO_CodingAAC;
                } else
                {
                    DEBUG_PRINT_ERROR("get_parameter: Bad port index %d\n",
                                      (int)portFormatType->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }

        case OMX_IndexParamAudioAac:
            {
                OMX_AUDIO_PARAM_AACPROFILETYPE *aacParam =
                (OMX_AUDIO_PARAM_AACPROFILETYPE *) paramData;
                DEBUG_PRINT("OMX_IndexParamAudioAac\n");
                if (OMX_CORE_OUTPUT_PORT_INDEX== aacParam->nPortIndex)
                {
                    memcpy(aacParam,&m_aac_param,
                    sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

                } else
                {
                    DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioAac "\
                                      "OMX_ErrorBadPortIndex %d\n", \
                                      (int)aacParam->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }
    case QOMX_IndexParamAudioSessionId:
    {
       QOMX_AUDIO_STREAM_INFO_DATA *streaminfoparam =
               (QOMX_AUDIO_STREAM_INFO_DATA *) paramData;
       streaminfoparam->sessionId = (OMX_U8)m_session_id;
       break;
    }

        case OMX_IndexParamAudioPcm:
            {
                OMX_AUDIO_PARAM_PCMMODETYPE *pcmparam =
                (OMX_AUDIO_PARAM_PCMMODETYPE *) paramData;

                if (OMX_CORE_INPUT_PORT_INDEX== pcmparam->nPortIndex)
                {
                    memcpy(pcmparam,&m_pcm_param,\
                        sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));

                    DEBUG_PRINT("get_parameter: Sampling rate %u",\
                                 pcmparam->nSamplingRate);
                    DEBUG_PRINT("get_parameter: Number of channels %u",\
                                 pcmparam->nChannels);
                } else
                {
                    DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioPcm "\
                                      "OMX_ErrorBadPortIndex %d\n", \
                                      (int)pcmparam->nPortIndex);
                     eRet = OMX_ErrorBadPortIndex;
                }
                break;
         }
        case OMX_IndexParamComponentSuspended:
        {
            OMX_PARAM_SUSPENSIONTYPE *suspend =
			(OMX_PARAM_SUSPENSIONTYPE *) paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamComponentSuspended %p\n",
			suspend);
            break;
        }
        case OMX_IndexParamVideoInit:
            {
                OMX_PORT_PARAM_TYPE *portParamType =
                    (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT("get_parameter: OMX_IndexParamVideoInit\n");
                portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
                portParamType->nSize = (OMX_U32)sizeof(portParamType);
                portParamType->nPorts           = 0;
                portParamType->nStartPortNumber = 0;
                break;
            }
        case OMX_IndexParamPriorityMgmt:
            {
                OMX_PRIORITYMGMTTYPE *priorityMgmtType =
                (OMX_PRIORITYMGMTTYPE*)paramData;
                DEBUG_PRINT("get_parameter: OMX_IndexParamPriorityMgmt\n");
                priorityMgmtType->nSize = (OMX_U32)sizeof(priorityMgmtType);
                priorityMgmtType->nVersion.nVersion = OMX_SPEC_VERSION;
                priorityMgmtType->nGroupID = m_priority_mgm.nGroupID;
                priorityMgmtType->nGroupPriority =
			m_priority_mgm.nGroupPriority;
                break;
            }
        case OMX_IndexParamImageInit:
            {
                OMX_PORT_PARAM_TYPE *portParamType =
                (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT("get_parameter: OMX_IndexParamImageInit\n");
                portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
                portParamType->nSize = (OMX_U32)sizeof(portParamType);
                portParamType->nPorts           = 0;
                portParamType->nStartPortNumber = 0;
                break;
            }

        case OMX_IndexParamCompBufferSupplier:
            {
                DEBUG_PRINT("get_parameter: \
				OMX_IndexParamCompBufferSupplier\n");
                OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType
                = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
                DEBUG_PRINT("get_parameter: \
				OMX_IndexParamCompBufferSupplier\n");

                bufferSupplierType->nSize = (OMX_U32)sizeof(bufferSupplierType);
                bufferSupplierType->nVersion.nVersion = OMX_SPEC_VERSION;
                if (OMX_CORE_INPUT_PORT_INDEX   ==
			bufferSupplierType->nPortIndex)
                {
                    bufferSupplierType->nPortIndex =
				OMX_BufferSupplyUnspecified;
                } else if (OMX_CORE_OUTPUT_PORT_INDEX ==
				bufferSupplierType->nPortIndex)
                {
                    bufferSupplierType->nPortIndex =
				OMX_BufferSupplyUnspecified;
                } else
                {
                    DEBUG_PRINT_ERROR("get_parameter:"\
                                      "OMX_IndexParamCompBufferSupplier eRet"\
                                      "%08x\n", eRet);
                    eRet = OMX_ErrorBadPortIndex;
                }
                 break;
            }

            /*Component should support this port definition*/
        case OMX_IndexParamOtherInit:
            {
                OMX_PORT_PARAM_TYPE *portParamType =
                    (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT("get_parameter: OMX_IndexParamOtherInit\n");
                portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
                portParamType->nSize = (OMX_U32)sizeof(portParamType);
                portParamType->nPorts           = 0;
                portParamType->nStartPortNumber = 0;
                break;
            }
	case OMX_IndexParamStandardComponentRole:
            {
                OMX_PARAM_COMPONENTROLETYPE *componentRole;
                componentRole = (OMX_PARAM_COMPONENTROLETYPE*)paramData;
                componentRole->nSize = component_Role.nSize;
                componentRole->nVersion = component_Role.nVersion;
                strlcpy((char *)componentRole->cRole,
			(const char*)component_Role.cRole,
			sizeof(componentRole->cRole));
                DEBUG_PRINT_ERROR("nSize = %d , nVersion = %d, cRole = %s\n",
				component_Role.nSize,
				component_Role.nVersion,
				component_Role.cRole);
                break;

            }
        default:
            {
                DEBUG_PRINT_ERROR("unknown param %08x\n", paramIndex);
                eRet = OMX_ErrorUnsupportedIndex;
            }
    }
    return eRet;

}

/**
 @brief member function that set paramter from IL client

 @param hComp handle to component instance
 @param paramIndex parameter type
 @param paramData pointer to memory space which holds the paramter
 @return error status
 */
OMX_ERRORTYPE  omx_aac_aenc::set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                            OMX_IN OMX_INDEXTYPE paramIndex,
                                            OMX_IN OMX_PTR        paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned int loop=0;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (m_state != OMX_StateLoaded)
    {
        DEBUG_PRINT_ERROR("set_parameter is not in proper state\n");
        return OMX_ErrorIncorrectStateOperation;
    }
    if (paramData == NULL)
    {
        DEBUG_PRINT("param data is NULL");
        return OMX_ErrorBadParameter;
    }

    switch (paramIndex)
    {
        case OMX_IndexParamAudioAac:
            {
                DEBUG_PRINT("OMX_IndexParamAudioAac");
                OMX_AUDIO_PARAM_AACPROFILETYPE *aacparam
                = (OMX_AUDIO_PARAM_AACPROFILETYPE *) paramData;
                memcpy(&m_aac_param,aacparam,
                                      sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

        for (loop=0; loop< sizeof(sample_idx_tbl) / \
            sizeof(struct sample_rate_idx); \
                loop++)
        {
            if(sample_idx_tbl[loop].sample_rate == m_aac_param.nSampleRate)
                 {
                sample_idx  = sample_idx_tbl[loop].sample_rate_idx;
            }
        }
                break;
            }
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
                portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

                if (((m_state == OMX_StateLoaded)&&
                     !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                    || (m_state == OMX_StateWaitForResources &&
                        ((OMX_DirInput == portDefn->eDir && 
				m_inp_bEnabled == true)||
                         (OMX_DirInput == portDefn->eDir &&
				m_out_bEnabled == true)))
                    ||(((OMX_DirInput == portDefn->eDir &&
				m_inp_bEnabled == false)||
                        (OMX_DirInput == portDefn->eDir &&
				m_out_bEnabled == false)) &&
                       (m_state != OMX_StateWaitForResources)))
                {
                    DEBUG_PRINT("Set Parameter called in valid state\n");
                } else
                {
                    DEBUG_PRINT_ERROR("Set Parameter called in \
					Invalid State\n");
                    return OMX_ErrorIncorrectStateOperation;
                }
                DEBUG_PRINT("OMX_IndexParamPortDefinition portDefn->nPortIndex "
                            "= %u\n",portDefn->nPortIndex);
                if (OMX_CORE_INPUT_PORT_INDEX == portDefn->nPortIndex)
                {
                    if ( portDefn->nBufferCountActual >
				OMX_CORE_NUM_INPUT_BUFFERS )
                    {
                        m_inp_act_buf_count = portDefn->nBufferCountActual;
                    } else
                    {
                        m_inp_act_buf_count =OMX_CORE_NUM_INPUT_BUFFERS;
                    }
                    input_buffer_size = portDefn->nBufferSize;

                } else if (OMX_CORE_OUTPUT_PORT_INDEX == portDefn->nPortIndex)
                {
                    if ( portDefn->nBufferCountActual >
				OMX_CORE_NUM_OUTPUT_BUFFERS )
                    {
                        m_out_act_buf_count = portDefn->nBufferCountActual;
                    } else
                    {
                        m_out_act_buf_count =OMX_CORE_NUM_OUTPUT_BUFFERS;
                    }
                    output_buffer_size = portDefn->nBufferSize;
                } else
                {
                    DEBUG_PRINT(" set_parameter: Bad Port idx %d",\
                                  (int)portDefn->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }
        case OMX_IndexParamPriorityMgmt:
            {
                DEBUG_PRINT("set_parameter: OMX_IndexParamPriorityMgmt\n");

                if (m_state != OMX_StateLoaded)
                {
                    DEBUG_PRINT_ERROR("Set Parameter called in \
					Invalid State\n");
                    return OMX_ErrorIncorrectStateOperation;
                }
                OMX_PRIORITYMGMTTYPE *priorityMgmtype
                = (OMX_PRIORITYMGMTTYPE*) paramData;
                DEBUG_PRINT("set_parameter: OMX_IndexParamPriorityMgmt %u\n",
                            priorityMgmtype->nGroupID);

                DEBUG_PRINT("set_parameter: priorityMgmtype %u\n",
                            priorityMgmtype->nGroupPriority);

                m_priority_mgm.nGroupID = priorityMgmtype->nGroupID;
                m_priority_mgm.nGroupPriority = priorityMgmtype->nGroupPriority;

                break;
            }
        case  OMX_IndexParamAudioPortFormat:
            {

                OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
                (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
                DEBUG_PRINT("set_parameter: OMX_IndexParamAudioPortFormat\n");

                if (OMX_CORE_INPUT_PORT_INDEX== portFormatType->nPortIndex)
                {
                    portFormatType->eEncoding = OMX_AUDIO_CodingPCM;
                } else if (OMX_CORE_OUTPUT_PORT_INDEX ==
				portFormatType->nPortIndex)
                {
                    DEBUG_PRINT("set_parameter: OMX_IndexParamAudioFormat:"\
                                " %u\n", portFormatType->nIndex);
                    portFormatType->eEncoding = OMX_AUDIO_CodingAAC;
                } else
                {
                    DEBUG_PRINT_ERROR("set_parameter: Bad port index %d\n", \
                                      (int)portFormatType->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }


        case OMX_IndexParamCompBufferSupplier:
            {
                DEBUG_PRINT("set_parameter: \
				OMX_IndexParamCompBufferSupplier\n");
                OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType
                = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
                DEBUG_PRINT("set_param: OMX_IndexParamCompBufferSupplier %d",\
                            bufferSupplierType->eBufferSupplier);

                if (bufferSupplierType->nPortIndex == OMX_CORE_INPUT_PORT_INDEX
                    || bufferSupplierType->nPortIndex == OMX_CORE_OUTPUT_PORT_INDEX)
                {
                    DEBUG_PRINT("set_parameter:\
				OMX_IndexParamCompBufferSupplier\n");
                    m_buffer_supplier.eBufferSupplier =
				bufferSupplierType->eBufferSupplier;
                } else
                {
                    DEBUG_PRINT_ERROR("set_param:IndexParamCompBufferSup\
					%08x\n", eRet);
                    eRet = OMX_ErrorBadPortIndex;
                }

                break; }

        case OMX_IndexParamAudioPcm:
            {
                DEBUG_PRINT("set_parameter: OMX_IndexParamAudioPcm\n");
                OMX_AUDIO_PARAM_PCMMODETYPE *pcmparam
                = (OMX_AUDIO_PARAM_PCMMODETYPE *) paramData;

                if (OMX_CORE_INPUT_PORT_INDEX== pcmparam->nPortIndex)
                {

                    memcpy(&m_pcm_param,pcmparam,\
                        sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));

                    DEBUG_PRINT("set_pcm_parameter: %u %u",\
                                m_pcm_param.nChannels,
				m_pcm_param.nSamplingRate);
                } else
                {
                    DEBUG_PRINT_ERROR("Set_parameter:OMX_IndexParamAudioPcm "
                                      "OMX_ErrorBadPortIndex %d\n",
                                      (int)pcmparam->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }
        case OMX_IndexParamSuspensionPolicy:
            {
                eRet = OMX_ErrorNotImplemented;
                break;
            }
        case OMX_IndexParamStandardComponentRole:
            {
                OMX_PARAM_COMPONENTROLETYPE *componentRole;
                componentRole = (OMX_PARAM_COMPONENTROLETYPE*)paramData;
                component_Role.nSize = componentRole->nSize;
                component_Role.nVersion = componentRole->nVersion;
                strlcpy((char *)component_Role.cRole,
                       (const char*)componentRole->cRole,
			sizeof(component_Role.cRole));
                break;
            }

        default:
            {
                DEBUG_PRINT_ERROR("unknown param %d\n", paramIndex);
                eRet = OMX_ErrorUnsupportedIndex;
            }
    }
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::GetConfig

DESCRIPTION
  OMX Get Config Method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::get_config(OMX_IN OMX_HANDLETYPE      hComp,
                                         OMX_IN OMX_INDEXTYPE configIndex,
                                         OMX_INOUT OMX_PTR     configData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Config in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    switch (configIndex)
    {
        case OMX_IndexConfigAudioVolume:
            {
                OMX_AUDIO_CONFIG_VOLUMETYPE *volume =
                (OMX_AUDIO_CONFIG_VOLUMETYPE*) configData;

                if (OMX_CORE_INPUT_PORT_INDEX == volume->nPortIndex)
                {
                    volume->nSize = (OMX_U32)sizeof(volume);
                    volume->nVersion.nVersion = OMX_SPEC_VERSION;
                    volume->bLinear = OMX_TRUE;
                    volume->sVolume.nValue = m_volume;
                    volume->sVolume.nMax   = OMX_AENC_MAX;
                    volume->sVolume.nMin   = OMX_AENC_MIN;
                } else
                {
                    eRet = OMX_ErrorBadPortIndex;
                }
            }
            break;

        case OMX_IndexConfigAudioMute:
            {
                OMX_AUDIO_CONFIG_MUTETYPE *mute =
                (OMX_AUDIO_CONFIG_MUTETYPE*) configData;

                if (OMX_CORE_INPUT_PORT_INDEX == mute->nPortIndex)
                {
                    mute->nSize = (OMX_U32)sizeof(mute);
                    mute->nVersion.nVersion = OMX_SPEC_VERSION;
                    mute->bMute = (BITMASK_PRESENT(&m_flags,
                                      OMX_COMPONENT_MUTED)?OMX_TRUE:OMX_FALSE);
                } else
                {
                    eRet = OMX_ErrorBadPortIndex;
                }
            }
            break;

        default:
            eRet = OMX_ErrorUnsupportedIndex;
            break;
    }
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::SetConfig

DESCRIPTION
  OMX Set Config method implementation

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.
========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::set_config(OMX_IN OMX_HANDLETYPE      hComp,
                                         OMX_IN OMX_INDEXTYPE configIndex,
                                         OMX_IN OMX_PTR        configData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Set Config in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if ( m_state == OMX_StateExecuting)
    {
        DEBUG_PRINT_ERROR("set_config:Ignore in Exe state\n");
        return OMX_ErrorInvalidState;
    }

    switch (configIndex)
    {
        case OMX_IndexConfigAudioVolume:
            {
                OMX_AUDIO_CONFIG_VOLUMETYPE *vol =
			(OMX_AUDIO_CONFIG_VOLUMETYPE*)configData;
                if (vol->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
                {
                    if ((vol->sVolume.nValue <= OMX_AENC_MAX) &&
                        (vol->sVolume.nValue >= OMX_AENC_MIN))
                    {
                        m_volume = vol->sVolume.nValue;
                        if (BITMASK_ABSENT(&m_flags, OMX_COMPONENT_MUTED))
                        {
                            /* ioctl(m_drv_fd, AUDIO_VOLUME,
                            m_volume * OMX_AENC_VOLUME_STEP); */
                        }

                    } else
                    {
                        eRet = OMX_ErrorBadParameter;
                    }
                } else
                {
                    eRet = OMX_ErrorBadPortIndex;
                }
            }
            break;

        case OMX_IndexConfigAudioMute:
            {
                OMX_AUDIO_CONFIG_MUTETYPE *mute = (OMX_AUDIO_CONFIG_MUTETYPE*)
                                                  configData;
                if (mute->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
                {
                    if (mute->bMute == OMX_TRUE)
                    {
                        BITMASK_SET(&m_flags, OMX_COMPONENT_MUTED);
                        /* ioctl(m_drv_fd, AUDIO_VOLUME, 0); */
                    } else
                    {
                        BITMASK_CLEAR(&m_flags, OMX_COMPONENT_MUTED);
                        /* ioctl(m_drv_fd, AUDIO_VOLUME,
                        m_volume * OMX_AENC_VOLUME_STEP); */
                    }
                } else
                {
                    eRet = OMX_ErrorBadPortIndex;
                }
            }
            break;

        default:
            eRet = OMX_ErrorUnsupportedIndex;
            break;
    }
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::GetExtensionIndex

DESCRIPTION
  OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::get_extension_index(
				OMX_IN OMX_HANDLETYPE      hComp,
				OMX_IN OMX_STRING      paramName,
				OMX_OUT OMX_INDEXTYPE* indexType)
{
    if((hComp == NULL) || (paramName == NULL) || (indexType == NULL))
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Extension Index in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
  if(strncmp(paramName,"OMX.Qualcomm.index.audio.sessionId",
	strlen("OMX.Qualcomm.index.audio.sessionId")) == 0)
  {
      *indexType =(OMX_INDEXTYPE)QOMX_IndexParamAudioSessionId;
      DEBUG_PRINT("Extension index type - %d\n", *indexType);

  }
  else
  {
      return OMX_ErrorBadParameter;

  }
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::GetState

DESCRIPTION
  Returns the state information back to the caller.<TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::get_state(OMX_IN OMX_HANDLETYPE  hComp,
                                        OMX_OUT OMX_STATETYPE* state)
{
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    *state = m_state;
    DEBUG_PRINT("Returning the state %d\n",*state);
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::ComponentTunnelRequest

DESCRIPTION
  OMX Component Tunnel Request method implementation. <TBD>

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::component_tunnel_request
(
    OMX_IN OMX_HANDLETYPE                hComp,
    OMX_IN OMX_U32                        port,
    OMX_IN OMX_HANDLETYPE        peerComponent,
    OMX_IN OMX_U32                    peerPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
    DEBUG_PRINT_ERROR("Error: component_tunnel_request Not Implemented\n");

    if((hComp == NULL) || (peerComponent == NULL) || (tunnelSetup == NULL))
    {
        port = port;
        peerPort = peerPort;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::AllocateInputBuffer

DESCRIPTION
  Helper function for allocate buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::allocate_input_buffer
(
    OMX_IN OMX_HANDLETYPE                hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                        port,
    OMX_IN OMX_PTR                     appData,
    OMX_IN OMX_U32                       bytes)
{
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE  *bufHdr;
    unsigned              nBufSize = MAX(bytes, input_buffer_size);
    char                  *buf_ptr;
  if(m_inp_current_buf_count < m_inp_act_buf_count)
  {
    buf_ptr = (char *) calloc((nBufSize + \
		sizeof(OMX_BUFFERHEADERTYPE)+sizeof(META_IN)) , 1);

    if(hComp == NULL)
    {
        port = port;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        free(buf_ptr);
        return OMX_ErrorBadParameter;
    }
    if (buf_ptr != NULL)
    {
        bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
        *bufferHdr = bufHdr;
        memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

        bufHdr->pBuffer           = (OMX_U8 *)((buf_ptr) + sizeof(META_IN)+
                                               sizeof(OMX_BUFFERHEADERTYPE));
        bufHdr->nSize             = (OMX_U32)sizeof(OMX_BUFFERHEADERTYPE);
        bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
        bufHdr->nAllocLen         = nBufSize;
        bufHdr->pAppPrivate       = appData;
        bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
        m_input_buf_hdrs.insert(bufHdr, NULL);

        m_inp_current_buf_count++;
        DEBUG_PRINT("AIB:bufHdr %p bufHdr->pBuffer %p m_inp_buf_cnt=%d \
		bytes=%u",bufHdr, bufHdr->pBuffer,m_inp_current_buf_count,
		bytes);

    } else
    {
        DEBUG_PRINT("Input buffer memory allocation failed 1 \n");
        eRet =  OMX_ErrorInsufficientResources;
    }
  }
  else
  {
     DEBUG_PRINT("Input buffer memory allocation failed 2\n");
    eRet =  OMX_ErrorInsufficientResources;
  }
    return eRet;
}

OMX_ERRORTYPE  omx_aac_aenc::allocate_output_buffer
(
    OMX_IN OMX_HANDLETYPE                hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                        port,
    OMX_IN OMX_PTR                     appData,
    OMX_IN OMX_U32                       bytes)
{
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE  *bufHdr;
    unsigned                   nBufSize = MAX(bytes,output_buffer_size);
    char                  *buf_ptr;

    if(hComp == NULL)
    {
        port = port;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (m_out_current_buf_count < m_out_act_buf_count)
    {
        buf_ptr = (char *) calloc( (nBufSize + sizeof(OMX_BUFFERHEADERTYPE)),1);

        if (buf_ptr != NULL)
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

            bufHdr->pBuffer           = (OMX_U8 *)((buf_ptr)+
					sizeof(OMX_BUFFERHEADERTYPE));

            bufHdr->nSize             = (OMX_U32)sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nOutputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX;
            m_output_buf_hdrs.insert(bufHdr, NULL);
            m_out_current_buf_count++;
            DEBUG_PRINT("AOB::bufHdr %p bufHdr->pBuffer %p m_out_buf_cnt=%d "\
                        "bytes=%u",bufHdr, bufHdr->pBuffer,\
                        m_out_current_buf_count, bytes);
        } else
        {
            DEBUG_PRINT("Output buffer memory allocation failed 1 \n");
            eRet =  OMX_ErrorInsufficientResources;
        }
    } else
    {
        DEBUG_PRINT("Output buffer memory allocation failed\n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}


// AllocateBuffer  -- API Call
/* ======================================================================
FUNCTION
  omx_aac_aenc::AllocateBuffer

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::allocate_buffer
(
    OMX_IN OMX_HANDLETYPE                hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                        port,
    OMX_IN OMX_PTR                     appData,
    OMX_IN OMX_U32                       bytes)
{

    OMX_ERRORTYPE eRet = OMX_ErrorNone;          // OMX return type

    if (m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Allocate Buf in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    // What if the client calls again.
    if (OMX_CORE_INPUT_PORT_INDEX == port)
    {
        eRet = allocate_input_buffer(hComp,bufferHdr,port,appData,bytes);
    } else if (OMX_CORE_OUTPUT_PORT_INDEX == port)
    {
        eRet = allocate_output_buffer(hComp,bufferHdr,port,appData,bytes);
    } else
    {
        DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",
                          (int)port);
        eRet = OMX_ErrorBadPortIndex;
    }

    if (eRet == OMX_ErrorNone)
    {
        DEBUG_PRINT("allocate_buffer:  before allocate_done \n");
        if (allocate_done())
        {
            DEBUG_PRINT("allocate_buffer:  after allocate_done \n");
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
            {
                BITMASK_CLEAR(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                post_command(OMX_CommandStateSet,OMX_StateIdle,
                             OMX_COMPONENT_GENERATE_EVENT);
                DEBUG_PRINT("allocate_buffer:  post idle transition event \n");
            }
            DEBUG_PRINT("allocate_buffer:  complete \n");
        }
        if (port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated)
        {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING))
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
                post_command(OMX_CommandPortEnable, OMX_CORE_INPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            }
        }
        if (port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated)
        {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING))
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                m_out_bEnabled = OMX_TRUE;

                DEBUG_PRINT("AllocBuf-->is_out_th_sleep=%d\n",is_out_th_sleep);
                pthread_mutex_lock(&m_out_th_lock_1);
                if (is_out_th_sleep)
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("AllocBuf:WAKING UP OUT THREADS\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                   is_in_th_sleep = false;
                   DEBUG_DETAIL("AB:WAKING UP IN THREADS\n");
                   in_th_wakeup();
                }
                pthread_mutex_unlock(&m_in_th_lock_1);
                post_command(OMX_CommandPortEnable, OMX_CORE_OUTPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            }
        }
    }
    DEBUG_PRINT("Allocate Buffer exit with ret Code %d\n", eRet);
    return eRet;
}

/*=============================================================================
FUNCTION:
  use_buffer

DESCRIPTION:
  OMX Use Buffer method implementation.

INPUT/OUTPUT PARAMETERS:
  [INOUT] bufferHdr
  [IN] hComp
  [IN] port
  [IN] appData
  [IN] bytes
  [IN] buffer

RETURN VALUE:
  OMX_ERRORTYPE

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
OMX_ERRORTYPE  omx_aac_aenc::use_buffer
(
    OMX_IN OMX_HANDLETYPE            hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                   port,
    OMX_IN OMX_PTR                   appData,
    OMX_IN OMX_U32                   bytes,
    OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if (OMX_CORE_INPUT_PORT_INDEX == port)
    {
        eRet = use_input_buffer(hComp,bufferHdr,port,appData,bytes,buffer);

    } else if (OMX_CORE_OUTPUT_PORT_INDEX == port)
    {
        eRet = use_output_buffer(hComp,bufferHdr,port,appData,bytes,buffer);
    } else
    {
        DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",(int)port);
        eRet = OMX_ErrorBadPortIndex;
    }

    if (eRet == OMX_ErrorNone)
    {
        DEBUG_PRINT("Checking for Output Allocate buffer Done");
        if (allocate_done())
        {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
            {
                BITMASK_CLEAR(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                post_command(OMX_CommandStateSet,OMX_StateIdle,
                             OMX_COMPONENT_GENERATE_EVENT);
            }
        }
        if (port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated)
        {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING))
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
                post_command(OMX_CommandPortEnable, OMX_CORE_INPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);

            }
        }
        if (port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated)
        {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING))
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                post_command(OMX_CommandPortEnable, OMX_CORE_OUTPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
                pthread_mutex_lock(&m_out_th_lock_1);
                if (is_out_th_sleep)
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("UseBuf:WAKING UP OUT THREADS\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                   is_in_th_sleep = false;
                   DEBUG_DETAIL("UB:WAKING UP IN THREADS\n");
                   in_th_wakeup();
                }
                pthread_mutex_unlock(&m_in_th_lock_1);
        }
    }
  }
    DEBUG_PRINT("Use Buffer for port[%u] eRet[%d]\n", port,eRet);
    return eRet;
}
/*=============================================================================
FUNCTION:
  use_input_buffer

DESCRIPTION:
  Helper function for Use buffer in the input pin

INPUT/OUTPUT PARAMETERS:
  [INOUT] bufferHdr
  [IN] hComp
  [IN] port
  [IN] appData
  [IN] bytes
  [IN] buffer

RETURN VALUE:
  OMX_ERRORTYPE

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
OMX_ERRORTYPE  omx_aac_aenc::use_input_buffer
(
    OMX_IN OMX_HANDLETYPE            hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                   port,
    OMX_IN OMX_PTR                   appData,
    OMX_IN OMX_U32                   bytes,
    OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE  *bufHdr;
    unsigned              nBufSize = MAX(bytes, input_buffer_size);
    char                  *buf_ptr;

    if(hComp == NULL)
    {
        port = port;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if(bytes < input_buffer_size)
    {
      /* return if i\p buffer size provided by client
       is less than min i\p buffer size supported by omx component*/
      return OMX_ErrorInsufficientResources;
    }
    if (m_inp_current_buf_count < m_inp_act_buf_count)
    {
        buf_ptr = (char *) calloc(sizeof(OMX_BUFFERHEADERTYPE), 1);

        if (buf_ptr != NULL)
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

            bufHdr->pBuffer           = (OMX_U8 *)(buffer);
            DEBUG_PRINT("use_input_buffer:bufHdr %p bufHdr->pBuffer %p \
			bytes=%u", bufHdr, bufHdr->pBuffer,bytes);
            bufHdr->nSize             = (OMX_U32)sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            input_buffer_size         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
            bufHdr->nOffset           = 0;
            m_input_buf_hdrs.insert(bufHdr, NULL);
            m_inp_current_buf_count++;
        } else
        {
            DEBUG_PRINT("Input buffer memory allocation failed 1 \n");
            eRet =  OMX_ErrorInsufficientResources;
        }
    } else
    {
        DEBUG_PRINT("Input buffer memory allocation failed\n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}

/*=============================================================================
FUNCTION:
  use_output_buffer

DESCRIPTION:
  Helper function for Use buffer in the output pin

INPUT/OUTPUT PARAMETERS:
  [INOUT] bufferHdr
  [IN] hComp
  [IN] port
  [IN] appData
  [IN] bytes
  [IN] buffer

RETURN VALUE:
  OMX_ERRORTYPE

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
OMX_ERRORTYPE  omx_aac_aenc::use_output_buffer
(
    OMX_IN OMX_HANDLETYPE            hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                   port,
    OMX_IN OMX_PTR                   appData,
    OMX_IN OMX_U32                   bytes,
    OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE  *bufHdr;
    unsigned              nBufSize = MAX(bytes,output_buffer_size);
    char                  *buf_ptr;

    if(hComp == NULL)
    {
        port = port;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (bytes < output_buffer_size)
    {
        /* return if o\p buffer size provided by client
        is less than min o\p buffer size supported by omx component*/
        return OMX_ErrorInsufficientResources;
    }

    DEBUG_PRINT("Inside omx_aac_aenc::use_output_buffer");
    if (m_out_current_buf_count < m_out_act_buf_count)
    {

        buf_ptr = (char *) calloc(sizeof(OMX_BUFFERHEADERTYPE), 1);

        if (buf_ptr != NULL)
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            DEBUG_PRINT("BufHdr=%p buffer=%p\n",bufHdr,buffer);
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

            bufHdr->pBuffer           = (OMX_U8 *)(buffer);
            DEBUG_PRINT("use_output_buffer:bufHdr %p bufHdr->pBuffer %p \
			len=%u\n", bufHdr, bufHdr->pBuffer,bytes);
            bufHdr->nSize             = (OMX_U32)sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            output_buffer_size        = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nOutputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX;
            bufHdr->nOffset           = 0;
            m_output_buf_hdrs.insert(bufHdr, NULL);
            m_out_current_buf_count++;

        } else
        {
            DEBUG_PRINT("Output buffer memory allocation failed\n");
            eRet =  OMX_ErrorInsufficientResources;
        }
    } else
    {
        DEBUG_PRINT("Output buffer memory allocation failed 2\n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}
/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_aac_aenc::search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{

    bool eRet = false;
    OMX_BUFFERHEADERTYPE *temp = NULL;

    //access only in IL client context
    temp = m_input_buf_hdrs.find_ele(buffer);
    if (buffer && temp)
    {
        DEBUG_DETAIL("search_input_bufhdr %p \n", buffer);
        eRet = true;
    }
    return eRet;
}

/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_aac_aenc::search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{

    bool eRet = false;
    OMX_BUFFERHEADERTYPE *temp = NULL;

    //access only in IL client context
    temp = m_output_buf_hdrs.find_ele(buffer);
    if (buffer && temp)
    {
        DEBUG_DETAIL("search_output_bufhdr %p \n", buffer);
        eRet = true;
    }
    return eRet;
}

// Free Buffer - API call
/**
  @brief member function that handles free buffer command from IL client

  This function is a block-call function that handles IL client request to
  freeing the buffer

  @param hComp handle to component instance
  @param port id of port which holds the buffer
  @param buffer buffer header
  @return Error status
*/
OMX_ERRORTYPE  omx_aac_aenc::free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                          OMX_IN OMX_U32                 port,
                                          OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT("Free_Buffer buf %p\n", buffer);
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (m_state == OMX_StateIdle &&
        (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
    {
        DEBUG_PRINT(" free buffer while Component in Loading pending\n");
    } else if ((m_inp_bEnabled == OMX_FALSE &&
		port == OMX_CORE_INPUT_PORT_INDEX)||
               (m_out_bEnabled == OMX_FALSE &&
		port == OMX_CORE_OUTPUT_PORT_INDEX))
    {
        DEBUG_PRINT("Free Buffer while port %u disabled\n", port);
    } else if (m_state == OMX_StateExecuting || m_state == OMX_StatePause)
    {
        DEBUG_PRINT("Invalid state to free buffer,ports need to be disabled:\
                    OMX_ErrorPortUnpopulated\n");
        post_command(OMX_EventError,
                     OMX_ErrorPortUnpopulated,
                     OMX_COMPONENT_GENERATE_EVENT);

        return eRet;
    } else
    {
        DEBUG_PRINT("free_buffer: Invalid state to free buffer,ports need to be\
                    disabled:OMX_ErrorPortUnpopulated\n");
        post_command(OMX_EventError,
                     OMX_ErrorPortUnpopulated,
                     OMX_COMPONENT_GENERATE_EVENT);
    }
    if (OMX_CORE_INPUT_PORT_INDEX == port)
    {
        if (m_inp_current_buf_count != 0)
        {
            m_inp_bPopulated = OMX_FALSE;
            if (true == search_input_bufhdr(buffer))
            {
                /* Buffer exist */
                //access only in IL client context
                DEBUG_PRINT("Free_Buf:in_buffer[%p]\n",buffer);
                m_input_buf_hdrs.erase(buffer);
                free(buffer);
                m_inp_current_buf_count--;
            } else
            {
                DEBUG_PRINT_ERROR("Free_Buf:Error-->free_buffer, \
                                  Invalid Input buffer header\n");
                eRet = OMX_ErrorBadParameter;
            }
        } else
        {
            DEBUG_PRINT_ERROR("Error: free_buffer,Port Index calculation \
                              came out Invalid\n");
            eRet = OMX_ErrorBadPortIndex;
        }
        if (BITMASK_PRESENT((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING)
            && release_done(0))
        {
            DEBUG_PRINT("INPUT PORT MOVING TO DISABLED STATE \n");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING);
            post_command(OMX_CommandPortDisable,
                         OMX_CORE_INPUT_PORT_INDEX,
                         OMX_COMPONENT_GENERATE_EVENT);
        }
    } else if (OMX_CORE_OUTPUT_PORT_INDEX == port)
    {
        if (m_out_current_buf_count != 0)
        {
            m_out_bPopulated = OMX_FALSE;
            if (true == search_output_bufhdr(buffer))
            {
                /* Buffer exist */
                //access only in IL client context
                DEBUG_PRINT("Free_Buf:out_buffer[%p]\n",buffer);
                m_output_buf_hdrs.erase(buffer);
                free(buffer);
                m_out_current_buf_count--;
            } else
            {
                DEBUG_PRINT("Free_Buf:Error-->free_buffer , \
                            Invalid Output buffer header\n");
                eRet = OMX_ErrorBadParameter;
            }
        } else
        {
            eRet = OMX_ErrorBadPortIndex;
        }

        if (BITMASK_PRESENT((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING)
            && release_done(1))
        {
            DEBUG_PRINT("OUTPUT PORT MOVING TO DISABLED STATE \n");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
            post_command(OMX_CommandPortDisable,
                         OMX_CORE_OUTPUT_PORT_INDEX,
                         OMX_COMPONENT_GENERATE_EVENT);

        }
    } else
    {
        eRet = OMX_ErrorBadPortIndex;
    }
    if ((OMX_ErrorNone == eRet) &&
        (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
    {
        if (release_done(-1))
        {
            if(ioctl(m_drv_fd, AUDIO_STOP, 0) < 0)
               DEBUG_PRINT_ERROR("AUDIO STOP in free buffer failed\n");
            else
               DEBUG_PRINT("AUDIO STOP in free buffer passed\n");

            DEBUG_PRINT("Free_Buf: Free buffer\n");

            // Send the callback now
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_LOADING_PENDING);
            DEBUG_PRINT("Before OMX_StateLoaded \
			OMX_COMPONENT_GENERATE_EVENT\n");
            post_command(OMX_CommandStateSet,
                         OMX_StateLoaded,OMX_COMPONENT_GENERATE_EVENT);
            DEBUG_PRINT("After OMX_StateLoaded OMX_COMPONENT_GENERATE_EVENT\n");

        }
    }
    return eRet;
}


/**
 @brief member function that that handles empty this buffer command

 This function meremly queue up the command and data would be consumed
 in command server thread context

 @param hComp handle to component instance
 @param buffer pointer to buffer header
 @return error status
 */
OMX_ERRORTYPE  omx_aac_aenc::empty_this_buffer(
				OMX_IN OMX_HANDLETYPE         hComp,
				OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT("ETB:Buf:%p Len %u TS %lld numInBuf=%d\n", \
                buffer, buffer->nFilledLen, buffer->nTimeStamp, (nNumInputBuf));
    if (m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT("Empty this buffer in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if (!m_inp_bEnabled)
    {
        DEBUG_PRINT("empty_this_buffer OMX_ErrorIncorrectStateOperation "\
                    "Port Status %d \n", m_inp_bEnabled);
        return OMX_ErrorIncorrectStateOperation;
    }
    if (buffer->nSize != sizeof(OMX_BUFFERHEADERTYPE))
    {
        DEBUG_PRINT("omx_aac_aenc::etb--> Buffer Size Invalid\n");
        return OMX_ErrorBadParameter;
    }
    if (buffer->nVersion.nVersion != OMX_SPEC_VERSION)
    {
        DEBUG_PRINT("omx_aac_aenc::etb--> OMX Version Invalid\n");
        return OMX_ErrorVersionMismatch;
    }

    if (buffer->nInputPortIndex != OMX_CORE_INPUT_PORT_INDEX)
    {
        return OMX_ErrorBadPortIndex;
    }
    if ((m_state != OMX_StateExecuting) &&
        (m_state != OMX_StatePause))
    {
        DEBUG_PRINT_ERROR("Invalid state\n");
        eRet = OMX_ErrorInvalidState;
    }
    if (OMX_ErrorNone == eRet)
    {
        if (search_input_bufhdr(buffer) == true)
        {
            post_input((unsigned long)hComp,
                       (unsigned long) buffer,OMX_COMPONENT_GENERATE_ETB);
        } else
        {
            DEBUG_PRINT_ERROR("Bad header %p \n", buffer);
            eRet = OMX_ErrorBadParameter;
        }
    }
    pthread_mutex_lock(&in_buf_count_lock);
    nNumInputBuf++;
    m_aac_pb_stats.etb_cnt++;
    pthread_mutex_unlock(&in_buf_count_lock);
    return eRet;
}
/**
  @brief member function that writes data to kernel driver

  @param hComp handle to component instance
  @param buffer pointer to buffer header
  @return error status
 */
OMX_ERRORTYPE  omx_aac_aenc::empty_this_buffer_proxy
(
    OMX_IN OMX_HANDLETYPE         hComp,
    OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_STATETYPE state;
    META_IN meta_in;
    //Pointer to the starting location of the data to be transcoded
    OMX_U8 *srcStart;
    //The total length of the data to be transcoded
    srcStart = buffer->pBuffer;
    OMX_U8 *data = NULL;
    ssize_t bytes = 0;

    PrintFrameHdr(OMX_COMPONENT_GENERATE_ETB,buffer);
    memset(&meta_in,0,sizeof(meta_in));
    if ( search_input_bufhdr(buffer) == false )
    {
        DEBUG_PRINT("ETBP: INVALID BUF HDR\n");
        buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
        return OMX_ErrorBadParameter;
    }
    if (m_tmp_meta_buf)
    {
        data = m_tmp_meta_buf;

        // copy the metadata info from the BufHdr and insert to payload
        meta_in.offsetVal  = (OMX_U16)sizeof(META_IN);
        meta_in.nTimeStamp.LowPart =
           (unsigned int)((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp)& 0xFFFFFFFF);
        meta_in.nTimeStamp.HighPart =
          (unsigned int) (((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp) >> 32) & 0xFFFFFFFF);
        meta_in.nFlags &= ~OMX_BUFFERFLAG_EOS;
        if(buffer->nFlags & OMX_BUFFERFLAG_EOS)
        {
            DEBUG_PRINT("EOS OCCURED \n");
            meta_in.nFlags  |= OMX_BUFFERFLAG_EOS;
        }
        memcpy(data,&meta_in, meta_in.offsetVal);
        DEBUG_PRINT("meta_in.nFlags = %d\n",meta_in.nFlags);
    }

    if (ts == 0) {
        DEBUG_PRINT("Anchor time %lld", buffer->nTimeStamp);
        ts = buffer->nTimeStamp;
    }

    memcpy(&data[sizeof(META_IN)],buffer->pBuffer,buffer->nFilledLen);
    bytes = write(m_drv_fd, data, buffer->nFilledLen+sizeof(META_IN));
    if (bytes <= 0) {
        frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);

        if (errno == ENETRESET)
        {
            ALOGE("In SSR, return error to close the session");
            m_cb.EventHandler(&m_cmp,
                  m_app_data,
                  OMX_EventError,
                  OMX_ErrorHardware,
                  0, NULL );
        }
        return OMX_ErrorNone;
    }

    pthread_mutex_lock(&m_state_lock);
    get_state(&m_cmp, &state);
    pthread_mutex_unlock(&m_state_lock);

    if (OMX_StateExecuting == state)
    {
        DEBUG_DETAIL("In Exe state, EBD CB");
        buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
    } else
    {
        /* Assume empty this buffer function has already checked
        validity of buffer */
        DEBUG_PRINT("Empty buffer %p to kernel driver\n", buffer);
        post_input((unsigned long) & hComp,(unsigned long) buffer,
                   OMX_COMPONENT_GENERATE_BUFFER_DONE);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE  omx_aac_aenc::fill_this_buffer_proxy
(
    OMX_IN OMX_HANDLETYPE         hComp,
    OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_STATETYPE state;
    ENC_META_OUT *meta_out = NULL;
    ssize_t nReadbytes = 0;
    int szadifhr = 0;
    int numframes = 0;
    int metainfo  = 0;
    OMX_U8 *src = buffer->pBuffer;

    pthread_mutex_lock(&m_state_lock);
    get_state(&m_cmp, &state);
    pthread_mutex_unlock(&m_state_lock);

    if (true == search_output_bufhdr(buffer))
    {
        if((m_aac_param.eAACStreamFormat == OMX_AUDIO_AACStreamFormatADIF)
                && (adif_flag == 0))
        {

            DEBUG_PRINT("\nBefore Read..m_drv_fd = %d,\n",m_drv_fd);
            nReadbytes = read(m_drv_fd,m_tmp_out_meta_buf,output_buffer_size );
            DEBUG_DETAIL("FTBP->Al_len[%lu]buf[%p]size[%d]numOutBuf[%d]\n",\
                         buffer->nAllocLen,m_tmp_out_meta_buf,
                         nReadbytes,nNumOutputBuf);
            if(*m_tmp_out_meta_buf <= 0)
                return OMX_ErrorBadParameter;
            szadifhr = AUDAAC_MAX_ADIF_HEADER_LENGTH; 
            numframes =  *m_tmp_out_meta_buf;
            metainfo  = (int)((sizeof(ENC_META_OUT) * numframes)+
			sizeof(unsigned char));
            audaac_rec_install_adif_header_variable(0,sample_idx,
				(OMX_U8)m_aac_param.nChannels);
            memcpy(buffer->pBuffer,m_tmp_out_meta_buf,metainfo);
            memcpy(buffer->pBuffer + metainfo,&audaac_header_adif[0],szadifhr);
            memcpy(buffer->pBuffer + metainfo + szadifhr,
            m_tmp_out_meta_buf + metainfo,(nReadbytes - metainfo));
            src += sizeof(unsigned char);
            meta_out = (ENC_META_OUT *)src;
            meta_out->frame_size += szadifhr;
            numframes--;
            while(numframes > 0)
            {
                 src += sizeof(ENC_META_OUT);
                 meta_out = (ENC_META_OUT *)src;
                 meta_out->offset_to_frame += szadifhr;
                 numframes--;
            }
            buffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
            adif_flag++;
        }
        else if((m_aac_param.eAACStreamFormat == OMX_AUDIO_AACStreamFormatMP4FF)
                &&(mp4ff_flag == 0))
        {
            DEBUG_PRINT("OMX_AUDIO_AACStreamFormatMP4FF\n");
            audaac_rec_install_mp4ff_header_variable(0,sample_idx,
						(OMX_U8)m_aac_param.nChannels);
            memcpy(buffer->pBuffer,&audaac_header_mp4ff[0],
			AUDAAC_MAX_MP4FF_HEADER_LENGTH);
            buffer->nFilledLen = AUDAAC_MAX_MP4FF_HEADER_LENGTH;
            buffer->nTimeStamp = 0;
            buffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
            frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
            mp4ff_flag++;
            return OMX_ErrorNone;

        }
        else
        {

            DEBUG_PRINT("\nBefore Read..m_drv_fd = %d,\n",m_drv_fd);
            nReadbytes = read(m_drv_fd,buffer->pBuffer,output_buffer_size );
            DEBUG_DETAIL("FTBP->Al_len[%d]buf[%p]size[%d]numOutBuf[%d]\n",\
                         buffer->nAllocLen,buffer->pBuffer,
                         nReadbytes,nNumOutputBuf);
           if(nReadbytes <= 0)
           {
               buffer->nFilledLen = 0;
               buffer->nOffset = 0;
               buffer->nTimeStamp = nTimestamp;
               frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);

               if (errno == ENETRESET)
               {
                   ALOGE("In SSR, return error to close the session");
                   m_cb.EventHandler(&m_cmp,
                       m_app_data,
                      OMX_EventError,
                      OMX_ErrorHardware,
                      0, NULL );
               }
               return OMX_ErrorNone;
           }
        }

         meta_out = (ENC_META_OUT *)(buffer->pBuffer + sizeof(unsigned char));
         buffer->nTimeStamp = ts + (frameduration * m_frame_count);
         ++m_frame_count;
         nTimestamp = buffer->nTimeStamp;
         buffer->nFlags |= meta_out->nflags;
         buffer->nOffset =  meta_out->offset_to_frame + 1;
         buffer->nFilledLen = (OMX_U32)(nReadbytes - buffer->nOffset + szadifhr);
         DEBUG_PRINT("nflags %d frame_size %d offset_to_frame %d \
		timestamp %lld\n", meta_out->nflags,
		meta_out->frame_size,
		meta_out->offset_to_frame,
		buffer->nTimeStamp);

         if ((buffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS )
         {
              buffer->nFilledLen = 0;
              buffer->nOffset = 0;
              buffer->nTimeStamp = nTimestamp;
              frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
              if ((buffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS )
              {
                  DEBUG_PRINT("FTBP: Now, Send EOS flag to Client \n");
                  m_cb.EventHandler(&m_cmp,
                                  m_app_data,
                                  OMX_EventBufferFlag,
                                  1, 1, NULL );
                  DEBUG_PRINT("FTBP: END OF STREAM m_eos_bm=%d\n",m_eos_bm);
              }

              return OMX_ErrorNone;
          }
          DEBUG_PRINT("nState %d \n",nState );

          pthread_mutex_lock(&m_state_lock);
          get_state(&m_cmp, &state);
          pthread_mutex_unlock(&m_state_lock);

          if (state == OMX_StatePause)
          {
              DEBUG_PRINT("FTBP:Post the FBD to event thread currstate=%d\n",\
                            state);
              post_output((unsigned long) & hComp,(unsigned long) buffer,
                            OMX_COMPONENT_GENERATE_FRAME_DONE);
          }
          else
          {
              frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
              DEBUG_PRINT("FTBP*******************************************\n");

          }


    }
    else
        DEBUG_PRINT("\n FTBP-->Invalid buffer in FTB \n");


    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::FillThisBuffer

DESCRIPTION
  IL client uses this method to release the frame buffer
  after displaying them.



PARAMETERS

  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::fill_this_buffer
(
    OMX_IN OMX_HANDLETYPE         hComp,
    OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if (buffer->nSize != sizeof(OMX_BUFFERHEADERTYPE))
    {
        DEBUG_PRINT("omx_aac_aenc::ftb--> Buffer Size Invalid\n");
        return OMX_ErrorBadParameter;
    }
    if (m_out_bEnabled == OMX_FALSE)
    {
        return OMX_ErrorIncorrectStateOperation;
    }

    if (buffer->nVersion.nVersion != OMX_SPEC_VERSION)
    {
        DEBUG_PRINT("omx_aac_aenc::ftb--> OMX Version Invalid\n");
        return OMX_ErrorVersionMismatch;
    }
    if (buffer->nOutputPortIndex != OMX_CORE_OUTPUT_PORT_INDEX)
    {
        return OMX_ErrorBadPortIndex;
    }
    pthread_mutex_lock(&out_buf_count_lock);
    nNumOutputBuf++;
    m_aac_pb_stats.ftb_cnt++;
    DEBUG_DETAIL("FTB:nNumOutputBuf is %d", nNumOutputBuf);
    pthread_mutex_unlock(&out_buf_count_lock);
    post_output((unsigned long)hComp,
                (unsigned long) buffer,OMX_COMPONENT_GENERATE_FTB);
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::SetCallbacks

DESCRIPTION
  Set the callbacks.

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                                            OMX_IN OMX_CALLBACKTYPE* callbacks,
                                            OMX_IN OMX_PTR             appData)
{
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    m_cb       = *callbacks;
    m_app_data =    appData;

    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::ComponentDeInit

DESCRIPTION
  Destroys the component and release memory allocated to the heap.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (OMX_StateLoaded != m_state && OMX_StateInvalid != m_state)
    {
        DEBUG_PRINT_ERROR("Warning: Rxed DeInit when not in LOADED state %d\n",
            m_state);
    }
  deinit_encoder();

  DEBUG_PRINT_ERROR("%s:COMPONENT DEINIT...\n", __FUNCTION__);
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::deinit_encoder

DESCRIPTION
  Closes all the threads and release memory allocated to the heap.

PARAMETERS
  None.

RETURN VALUE
  None.

========================================================================== */
void  omx_aac_aenc::deinit_encoder()
{
    DEBUG_PRINT("Component-deinit being processed\n");
    DEBUG_PRINT("********************************\n");
    DEBUG_PRINT("STATS: in-buf-len[%u]out-buf-len[%u] tot-pb-time[%lld]",\
                m_aac_pb_stats.tot_in_buf_len,
                m_aac_pb_stats.tot_out_buf_len,
                m_aac_pb_stats.tot_pb_time);
    DEBUG_PRINT("STATS: fbd-cnt[%u]ftb-cnt[%u]etb-cnt[%u]ebd-cnt[%u]",\
                m_aac_pb_stats.fbd_cnt,m_aac_pb_stats.ftb_cnt,
                m_aac_pb_stats.etb_cnt,
                m_aac_pb_stats.ebd_cnt);
   memset(&m_aac_pb_stats,0,sizeof(AAC_PB_STATS));

    if((OMX_StateLoaded != m_state) && (OMX_StateInvalid != m_state))
    {
        DEBUG_PRINT_ERROR("%s,Deinit called in state[%d]\n",__FUNCTION__,\
                                                                m_state);
        // Get back any buffers from driver
        if(pcm_input)
            execute_omx_flush(-1,false);
        else
            execute_omx_flush(1,false);
        // force state change to loaded so that all threads can be exited
        pthread_mutex_lock(&m_state_lock);
        m_state = OMX_StateLoaded;
        pthread_mutex_unlock(&m_state_lock);
        DEBUG_PRINT_ERROR("Freeing Buf:inp_current_buf_count[%d][%d]\n",\
        m_inp_current_buf_count,
        m_input_buf_hdrs.size());
        m_input_buf_hdrs.eraseall();
        DEBUG_PRINT_ERROR("Freeing Buf:out_current_buf_count[%d][%d]\n",\
        m_out_current_buf_count,
        m_output_buf_hdrs.size());
        m_output_buf_hdrs.eraseall();

    }
    if(pcm_input)
    {
        pthread_mutex_lock(&m_in_th_lock_1);
        if (is_in_th_sleep)
        {
            is_in_th_sleep = false;
            DEBUG_DETAIL("Deinit:WAKING UP IN THREADS\n");
            in_th_wakeup();
        }
        pthread_mutex_unlock(&m_in_th_lock_1);
    }
    pthread_mutex_lock(&m_out_th_lock_1);
    if (is_out_th_sleep)
    {
        is_out_th_sleep = false;
        DEBUG_DETAIL("SCP:WAKING UP OUT THREADS\n");
        out_th_wakeup();
    }
    pthread_mutex_unlock(&m_out_th_lock_1);
    if(pcm_input)
    {
        if (m_ipc_to_in_th != NULL)
        {
            omx_aac_thread_stop(m_ipc_to_in_th);
            m_ipc_to_in_th = NULL;
        }
    }

    if (m_ipc_to_cmd_th != NULL)
    {
        omx_aac_thread_stop(m_ipc_to_cmd_th);
        m_ipc_to_cmd_th = NULL;
    }
    if (m_ipc_to_out_th != NULL)
    {
         DEBUG_DETAIL("Inside omx_aac_thread_stop\n");
        omx_aac_thread_stop(m_ipc_to_out_th);
        m_ipc_to_out_th = NULL;
    }


    if(ioctl(m_drv_fd, AUDIO_STOP, 0) <0)
          DEBUG_PRINT_ERROR("De-init: AUDIO_STOP FAILED\n");

    if(pcm_input && m_tmp_meta_buf )
    {
        free(m_tmp_meta_buf);
    }

    if(m_tmp_out_meta_buf)
    {
        free(m_tmp_out_meta_buf);
    }
    nNumInputBuf = 0;
    nNumOutputBuf = 0;
    m_inp_current_buf_count=0;
    m_out_current_buf_count=0;
    m_out_act_buf_count = 0;
    m_inp_act_buf_count = 0;
    m_inp_bEnabled = OMX_FALSE;
    m_out_bEnabled = OMX_FALSE;
    m_inp_bPopulated = OMX_FALSE;
    m_out_bPopulated = OMX_FALSE;
    adif_flag = 0;
    mp4ff_flag = 0;
    ts = 0;
    nTimestamp = 0;
    frameduration = 0;
    if ( m_drv_fd >= 0 )
    {
        if(close(m_drv_fd) < 0)
        DEBUG_PRINT("De-init: Driver Close Failed \n");
        m_drv_fd = -1;
    }
    else
    {
        DEBUG_PRINT_ERROR(" AAC device already closed\n");
    }
    m_comp_deinit=1;
    m_is_out_th_sleep = 1;
    m_is_in_th_sleep = 1;
    DEBUG_PRINT("************************************\n");
    DEBUG_PRINT(" DEINIT COMPLETED");
    DEBUG_PRINT("************************************\n");

}

/* ======================================================================
FUNCTION
  omx_aac_aenc::UseEGLImage

DESCRIPTION
  OMX Use EGL Image method implementation <TBD>.

PARAMETERS
  <TBD>.

RETURN VALUE
  Not Implemented error.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::use_EGL_image
(
    OMX_IN OMX_HANDLETYPE                hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                        port,
    OMX_IN OMX_PTR                     appData,
    OMX_IN void*                      eglImage)
{
    DEBUG_PRINT_ERROR("Error : use_EGL_image:  Not Implemented \n");

    if((hComp == NULL) || (appData == NULL) || (eglImage == NULL))
    {
        bufferHdr = bufferHdr;
        port = port;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::ComponentRoleEnum

DESCRIPTION
  OMX Component Role Enum method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
                                                  OMX_OUT OMX_U8*        role,
                                                  OMX_IN OMX_U32        index)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    const char *cmp_role = "audio_encoder.aac";

    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (index == 0 && role)
    {
        memcpy(role, cmp_role, strlen(cmp_role));
        *(((char *) role) + strlen(cmp_role) + 1) = '\0';
    } else
    {
        eRet = OMX_ErrorNoMore;
    }
    return eRet;
}




/* ======================================================================
FUNCTION
  omx_aac_aenc::AllocateDone

DESCRIPTION
  Checks if entire buffer pool is allocated by IL Client or not.
  Need this to move to IDLE state.

PARAMETERS
  None.

RETURN VALUE
  true/false.

========================================================================== */
bool omx_aac_aenc::allocate_done(void)
{
    OMX_BOOL bRet = OMX_FALSE;
    if (pcm_input==1)
    {
        if ((m_inp_act_buf_count == m_inp_current_buf_count)
            &&(m_out_act_buf_count == m_out_current_buf_count))
        {
            bRet=OMX_TRUE;

        }
        if ((m_inp_act_buf_count == m_inp_current_buf_count) && m_inp_bEnabled )
        {
            m_inp_bPopulated = OMX_TRUE;
        }

        if ((m_out_act_buf_count == m_out_current_buf_count) && m_out_bEnabled )
        {
            m_out_bPopulated = OMX_TRUE;
        }
    } else if (pcm_input==0)
    {
        if (m_out_act_buf_count == m_out_current_buf_count)
        {
            bRet=OMX_TRUE;

        }
        if ((m_out_act_buf_count == m_out_current_buf_count) && m_out_bEnabled )
        {
            m_out_bPopulated = OMX_TRUE;
        }

    }
    return bRet;
}


/* ======================================================================
FUNCTION
  omx_aac_aenc::ReleaseDone

DESCRIPTION
  Checks if IL client has released all the buffers.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_aac_aenc::release_done(OMX_U32 param1)
{
    DEBUG_PRINT("Inside omx_aac_aenc::release_done");
    OMX_BOOL bRet = OMX_FALSE;

    if (param1 == OMX_ALL)
    {

        if ((0 == m_inp_current_buf_count)&&(0 == m_out_current_buf_count))
        {
            bRet=OMX_TRUE;
        }
    } else if (param1 == OMX_CORE_INPUT_PORT_INDEX )
    {
        if ((0 == m_inp_current_buf_count))
        {
            bRet=OMX_TRUE;
        }
    } else if (param1 == OMX_CORE_OUTPUT_PORT_INDEX)
    {
        if ((0 == m_out_current_buf_count))
        {
            bRet=OMX_TRUE;
        }
    }
    return bRet;
}

void  omx_aac_aenc::audaac_rec_install_adif_header_variable (OMX_U16  byte_num,
                            OMX_U32 sample_index,
                            OMX_U8 channel_config)
{
  OMX_U8   buf8;
  OMX_U32  value;
  OMX_U32  dummy = 0;
  OMX_U8    num_pfe, num_fce, num_sce, num_bce; 
  OMX_U8    num_lfe, num_ade, num_vce, num_com;
  OMX_U8    pfe_index;
  OMX_U8    i;
  OMX_BOOL variable_bit_rate = OMX_FALSE;

  (void)byte_num;
  (void)channel_config;
  num_pfe = num_sce = num_bce = 
  num_lfe = num_ade = num_vce = num_com = 0;
  audaac_hdr_bit_index = 32;
  num_fce = 1;
  /* Store Header Id "ADIF" first */
  memcpy(&audaac_header_adif[0], "ADIF", sizeof(unsigned int));

  /* copyright_id_present field, 1 bit */
  value = 0;
  audaac_rec_install_bits(audaac_header_adif, 
                          AAC_COPYRIGHT_PRESENT_SIZE, 
                          value,
                          &(audaac_hdr_bit_index));

  if (value) {
    /* Copyright present, 72 bits; skip it for now,
     * just install dummy value */
    audaac_rec_install_bits(audaac_header_adif, 
                            72,
                            dummy,
                            &(audaac_hdr_bit_index));
  }

  /* original_copy field, 1 bit */
  value = 0;
  audaac_rec_install_bits(audaac_header_adif,
                          AAC_ORIGINAL_COPY_SIZE,
                          0,
                          &(audaac_hdr_bit_index));

  /* home field, 1 bit */
  value = 0;
  audaac_rec_install_bits(audaac_header_adif,
                          AAC_HOME_SIZE, 
                          0,
                          &(audaac_hdr_bit_index));

  /* bitstream_type = 1, varibable bit rate, 1 bit */
  value = 0;
  audaac_rec_install_bits(audaac_header_adif,
                          AAC_BITSTREAM_TYPE_SIZE,
                          value,
                          &(audaac_hdr_bit_index));

  /* bit_rate field, 23 bits */
  audaac_rec_install_bits(audaac_header_adif,
                          AAC_BITRATE_SIZE, 
                          (OMX_U32)m_aac_param.nBitRate,
                          &(audaac_hdr_bit_index));

  /* num_program_config_elements, 4 bits */
  num_pfe = 0;
  audaac_rec_install_bits(audaac_header_adif,
                          AAC_NUM_PFE_SIZE, 
                          (OMX_U32)num_pfe,
                          &(audaac_hdr_bit_index));

  /* below is to install program_config_elements field,
   * for now only one element is supported */
  for (pfe_index=0; pfe_index < num_pfe+1; pfe_index++) {


     if (variable_bit_rate == OMX_FALSE) {
	/* impossible, put dummy value for now */
       audaac_rec_install_bits(audaac_header_adif, 
                               AAC_BUFFER_FULLNESS_SIZE, 
                               0,
                               &(audaac_hdr_bit_index));

     }

    dummy = 0;

    /* element_instance_tag field, 4 bits */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_ELEMENT_INSTANCE_TAG_SIZE, 
                            dummy,
                            &(audaac_hdr_bit_index));

    /* object_type, 2 bits, AAC LC is supported */
    value = 1;
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_PROFILE_SIZE,  /* object type */
                            value,
                            &(audaac_hdr_bit_index));

    /* sampling_frequency_index, 4 bits */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_SAMPLING_FREQ_INDEX_SIZE, 
                            (OMX_U32)sample_index,
                            &(audaac_hdr_bit_index));

    /* num_front_channel_elements, 4 bits */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_NUM_FRONT_CHANNEL_ELEMENTS_SIZE, 
                            num_fce,
                            &(audaac_hdr_bit_index));

    /* num_side_channel_elements, 4 bits */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_NUM_SIDE_CHANNEL_ELEMENTS_SIZE, 
                            dummy,
                            &(audaac_hdr_bit_index));

    /* num_back_channel_elements, 4 bits */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_NUM_BACK_CHANNEL_ELEMENTS_SIZE, 
                            dummy,
                            &(audaac_hdr_bit_index));

    /* num_lfe_channel_elements, 2 bits */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_NUM_LFE_CHANNEL_ELEMENTS_SIZE, 
                            dummy,
                            &(audaac_hdr_bit_index));

    /* num_assoc_data_elements, 3 bits */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_NUM_ASSOC_DATA_ELEMENTS_SIZE, 
                            num_ade,
                            &(audaac_hdr_bit_index));

    /* num_valid_cc_elements, 4 bits */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_NUM_VALID_CC_ELEMENTS_SIZE, 
                            num_vce,
                            &(audaac_hdr_bit_index));

    /* mono_mixdown_present, 1 bits */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_MONO_MIXDOWN_PRESENT_SIZE, 
                            dummy,
                            &(audaac_hdr_bit_index));

    if (dummy) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_MONO_MIXDOWN_ELEMENT_SIZE, 
                              dummy,
                              &(audaac_hdr_bit_index));
    }
   
    /* stereo_mixdown_present */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_STEREO_MIXDOWN_PRESENT_SIZE, 
                            dummy,
                            &(audaac_hdr_bit_index));

    if (dummy) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_STEREO_MIXDOWN_ELEMENT_SIZE, 
                              dummy,
                              &(audaac_hdr_bit_index));
    }

    /* matrix_mixdown_idx_present, 1 bit */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_MATRIX_MIXDOWN_PRESENT_SIZE, 
                            dummy,
                            &(audaac_hdr_bit_index));

    if (dummy) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_MATRIX_MIXDOWN_SIZE, 
                              dummy,
                              &(audaac_hdr_bit_index));
    }
    if(m_aac_param.nChannels  == 2)
        value = 16;
    else
        value = 0; 
    for (i=0; i<num_fce; i++) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_FCE_SIZE, 
                              value,
                              &(audaac_hdr_bit_index));
    }
    
    for (i=0; i<num_sce; i++) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_SCE_SIZE, 
                              dummy,
                              &(audaac_hdr_bit_index));
    }

    for (i=0; i<num_bce; i++) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_BCE_SIZE, 
                              dummy,
                              &(audaac_hdr_bit_index));
    }

    for (i=0; i<num_lfe; i++) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_LFE_SIZE, 
                              dummy,
                              &(audaac_hdr_bit_index));
    }

    for (i=0; i<num_ade; i++) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_ADE_SIZE, 
                              dummy,
                              &(audaac_hdr_bit_index));
    }

    for (i=0; i<num_vce; i++) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_VCE_SIZE, 
                              dummy,
                              &(audaac_hdr_bit_index));
    }

    /* byte_alignment() */
    buf8 = (OMX_U8)((audaac_hdr_bit_index) & (0x07));
    if (buf8) {
      audaac_rec_install_bits(audaac_header_adif,
                              buf8,
                              dummy,
                              &(audaac_hdr_bit_index));
    }

    /* comment_field_bytes, 8 bits,
     * skip the comment section */
    audaac_rec_install_bits(audaac_header_adif,
                            AAC_COMMENT_FIELD_BYTES_SIZE,
                            num_com,
                            &(audaac_hdr_bit_index));

    for (i=0; i<num_com; i++) {
      audaac_rec_install_bits(audaac_header_adif,
                              AAC_COMMENT_FIELD_DATA_SIZE,
                              dummy,
                              &(audaac_hdr_bit_index));
    }
  } /* for (pfe_index=0; pfe_index < num_pfe+1; pfe_index++) */

  /* byte_alignment() */
  buf8 = (OMX_U8)((audaac_hdr_bit_index) & (0x07)) ;
  if (buf8) {
      audaac_rec_install_bits(audaac_header_adif,
                              buf8,
                              dummy,
                              &(audaac_hdr_bit_index));
  }

}

void   omx_aac_aenc::audaac_rec_install_bits(OMX_U8 *input,
                              OMX_U8 num_bits_reqd,
                              OMX_U32  value,
                              OMX_U16 *hdr_bit_index)
{
  OMX_U32 byte_index;
  OMX_U8   bit_index;
  OMX_U8   bits_avail_in_byte;
  OMX_U8   num_to_copy;
  OMX_U8   byte_to_copy;

  OMX_U8   num_remaining = num_bits_reqd;
  OMX_U8  bit_mask;

  bit_mask = 0xFF;

  while (num_remaining) {

    byte_index = (*hdr_bit_index) >> 3;
    bit_index  = (*hdr_bit_index) &  0x07;

    bits_avail_in_byte = (OMX_U8)(8 - bit_index);

    num_to_copy = min(bits_avail_in_byte, num_remaining);

    byte_to_copy = (OMX_U8)((OMX_U8)((value >> (num_remaining - num_to_copy)) & 0xFF) <<
                    (bits_avail_in_byte - num_to_copy));

    input[byte_index] &= ((OMX_U8)(bit_mask << bits_avail_in_byte));
    input[byte_index] |= byte_to_copy;

    *hdr_bit_index = (OMX_U16)(*hdr_bit_index + num_to_copy);

    num_remaining = (OMX_U8)(num_remaining - num_to_copy);
  }
}
void  omx_aac_aenc::audaac_rec_install_mp4ff_header_variable (OMX_U16  byte_num,
                                                        OMX_U32 sample_index,
                                                        OMX_U8 channel_config)
{
        OMX_U16 audaac_hdr_bit_index;
        (void)byte_num;
        audaac_header_mp4ff[0] = 0;
        audaac_header_mp4ff[1] = 0;
        audaac_hdr_bit_index = 0;

        /* Audio object type, 5 bit */
        audaac_rec_install_bits(audaac_header_mp4ff,
                          AUDAAC_MP4FF_OBJ_TYPE,
                          2,
                          &(audaac_hdr_bit_index));

        /* Frequency index, 4 bit */
        audaac_rec_install_bits(audaac_header_mp4ff,
                          AUDAAC_MP4FF_FREQ_IDX,
                          (OMX_U32)sample_index,
                          &(audaac_hdr_bit_index));

        /* Channel config filed, 4 bit */
        audaac_rec_install_bits(audaac_header_mp4ff,
                          AUDAAC_MP4FF_CH_CONFIG,
                          channel_config,
                          &(audaac_hdr_bit_index));

}

int omx_aac_aenc::get_updated_bit_rate(int bitrate)
{
	int updated_rate, min_bitrate, max_bitrate;

        max_bitrate = m_aac_param.nSampleRate *
        MAX_BITRATE_MULFACTOR;
	switch(m_aac_param.eAACProfile)
	{
		case OMX_AUDIO_AACObjectLC:
		    min_bitrate = m_aac_param.nSampleRate;
		    if (m_aac_param.nChannels == 1) {
		       min_bitrate = min_bitrate/BITRATE_DIVFACTOR;
                       max_bitrate = max_bitrate/BITRATE_DIVFACTOR;
                    }
                break;
		case OMX_AUDIO_AACObjectHE:
		    min_bitrate = MIN_BITRATE;
		    if (m_aac_param.nChannels == 1)
                       max_bitrate = max_bitrate/BITRATE_DIVFACTOR;
		break;
		case OMX_AUDIO_AACObjectHE_PS:
		    min_bitrate = MIN_BITRATE;
		break;
                default:
                    return bitrate;
                break;
	}
        /* Update MIN and MAX values*/
        if (min_bitrate > MIN_BITRATE)
              min_bitrate = MIN_BITRATE;
        if (max_bitrate > MAX_BITRATE)
              max_bitrate = MAX_BITRATE;
        /* Update the bitrate in the range  */
        if (bitrate < min_bitrate)
            updated_rate = min_bitrate;
        else if(bitrate > max_bitrate)
            updated_rate = max_bitrate;
        else
             updated_rate = bitrate;
	return updated_rate;
}
