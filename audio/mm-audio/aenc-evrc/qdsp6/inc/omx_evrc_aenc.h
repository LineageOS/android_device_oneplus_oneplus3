/*--------------------------------------------------------------------------
Copyright (c) 2010,2014 The Linux Foundation. All rights reserved.

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
#ifndef _EVRC_ENC_H_
#define _EVRC_ENC_H_
/*============================================================================
                    Audio Encoder

@file omx_evrc_aenc.h
This module contains the class definition for openMAX encoder component.



============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

/* Uncomment out below line #define LOG_NDEBUG 0 if we want to see
 *  all DEBUG_PRINT or LOGV messaging */
#include<stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include "QOMX_AudioExtensions.h"
#include "QOMX_AudioIndexExtensions.h"
#include "OMX_Core.h"
#include "OMX_Audio.h"
#include "aenc_svr.h"
#include "qc_omx_component.h"
#include "Map.h"
#include <semaphore.h>
#include <linux/msm_audio.h>
#include <linux/msm_audio_qcp.h>
extern "C" {
    void * get_omx_component_factory_fn(void);
}


//////////////////////////////////////////////////////////////////////////////
//                       Module specific globals
//////////////////////////////////////////////////////////////////////////////



#define OMX_SPEC_VERSION  0x00000101
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (x >= y?x:y)

//////////////////////////////////////////////////////////////////////////////
//               Macros
//////////////////////////////////////////////////////////////////////////////
//


#define PrintFrameHdr(i,bufHdr) \
                           DEBUG_PRINT("i=%d OMX bufHdr[%p]buf[%p]size[%d]TS[%lld]nFlags[0x%x]\n",\
                           i,\
                           bufHdr,                                     \
                           ((OMX_BUFFERHEADERTYPE *)bufHdr)->pBuffer,   \
                           (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nFilledLen,\
                           ((OMX_BUFFERHEADERTYPE *)bufHdr)->nTimeStamp, \
                           (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nFlags)


// BitMask Management logic
#define BITS_PER_BYTE 8
#define BITMASK_SIZE(mIndex) \
            (((mIndex) + BITS_PER_BYTE - 1)/BITS_PER_BYTE)
#define BITMASK_OFFSET(mIndex)\
            ((mIndex)/BITS_PER_BYTE)
#define BITMASK_FLAG(mIndex) \
            (1 << ((mIndex) % BITS_PER_BYTE))
#define BITMASK_CLEAR(mArray,mIndex)\
            (mArray)[BITMASK_OFFSET(mIndex)] &=  ~(BITMASK_FLAG(mIndex))
#define BITMASK_SET(mArray,mIndex)\
            (mArray)[BITMASK_OFFSET(mIndex)] |=  BITMASK_FLAG(mIndex)
#define BITMASK_PRESENT(mArray,mIndex)\
            ((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex))
#define BITMASK_ABSENT(mArray,mIndex)\
            (((mArray)[BITMASK_OFFSET(mIndex)] & \
            BITMASK_FLAG(mIndex)) == 0x0)

#define OMX_CORE_NUM_INPUT_BUFFERS    2
#define OMX_CORE_NUM_OUTPUT_BUFFERS   16

#define OMX_CORE_INPUT_BUFFER_SIZE    8160 // Multiple of 160
#define OMX_CORE_CONTROL_CMDQ_SIZE   100
#define OMX_AENC_VOLUME_STEP         0x147
#define OMX_AENC_MIN                 0
#define OMX_AENC_MAX                 100
#define NON_TUNNEL                   1
#define TUNNEL                       0
#define IP_PORT_BITMASK                 0x02
#define OP_PORT_BITMASK                 0x01
#define IP_OP_PORT_BITMASK              0x03

#define OMX_EVRC_DEFAULT_SF            8000
#define OMX_EVRC_DEFAULT_CH_CFG        1
#define OMX_EVRC_DEFAULT_VOL         25
// 14 bytes for input meta data
#define OMX_AENC_SIZEOF_META_BUF     (OMX_CORE_INPUT_BUFFER_SIZE+14)

#define TRUE 1
#define FALSE 0

#define NUMOFFRAMES                   1
#define MAXFRAMELENGTH                25
#define OMX_EVRC_OUTPUT_BUFFER_SIZE    ((NUMOFFRAMES * (sizeof(ENC_META_OUT) + MAXFRAMELENGTH) \
                        + 1))

#define OMX_EVRC_DEFAULT_MINRATE 4
#define OMX_EVRC_DEFAULT_MAXRATE 4

class omx_evrc_aenc;

// OMX EVRC audio encoder class
class omx_evrc_aenc: public qc_omx_component
{
public:
    omx_evrc_aenc();                             // constructor
    virtual ~omx_evrc_aenc();                    // destructor

    OMX_ERRORTYPE allocate_buffer(OMX_HANDLETYPE             hComp,
                                  OMX_BUFFERHEADERTYPE **bufferHdr,
                                  OMX_U32                     port,
                                  OMX_PTR                  appData,
                                  OMX_U32                    bytes);


    OMX_ERRORTYPE component_deinit(OMX_HANDLETYPE hComp);

    OMX_ERRORTYPE component_init(OMX_STRING role);

    OMX_ERRORTYPE component_role_enum(OMX_HANDLETYPE hComp,
                                      OMX_U8         *role,
                                      OMX_U32        index);

    OMX_ERRORTYPE component_tunnel_request(OMX_HANDLETYPE             hComp,
                                           OMX_U32                     port,
                                           OMX_HANDLETYPE     peerComponent,
                                           OMX_U32                 peerPort,
                                           OMX_TUNNELSETUPTYPE *tunnelSetup);

    OMX_ERRORTYPE empty_this_buffer(OMX_HANDLETYPE         hComp,
                                    OMX_BUFFERHEADERTYPE *buffer);


    OMX_ERRORTYPE empty_this_buffer_proxy(OMX_HANDLETYPE         hComp,
                                          OMX_BUFFERHEADERTYPE *buffer);


    OMX_ERRORTYPE fill_this_buffer(OMX_HANDLETYPE         hComp,
                                   OMX_BUFFERHEADERTYPE *buffer);


    OMX_ERRORTYPE free_buffer(OMX_HANDLETYPE         hComp,
                              OMX_U32                 port,
                              OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE get_component_version(OMX_HANDLETYPE              hComp,
                                        OMX_STRING          componentName,
                                        OMX_VERSIONTYPE *componentVersion,
                                        OMX_VERSIONTYPE *     specVersion,
                                        OMX_UUIDTYPE       *componentUUID);

    OMX_ERRORTYPE get_config(OMX_HANDLETYPE      hComp,
                             OMX_INDEXTYPE configIndex,
                             OMX_PTR        configData);

    OMX_ERRORTYPE get_extension_index(OMX_HANDLETYPE     hComp,
                                      OMX_STRING     paramName,
                                      OMX_INDEXTYPE *indexType);

    OMX_ERRORTYPE get_parameter(OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE paramIndex,
                                OMX_PTR paramData);

    OMX_ERRORTYPE get_state(OMX_HANDLETYPE hComp,
                            OMX_STATETYPE *state);

    static void process_in_port_msg(void          *client_data,
                                    unsigned char id);

    static void process_out_port_msg(void          *client_data,
                                     unsigned char id);

    static void process_command_msg(void          *client_data,
                                    unsigned char id);

    static void process_event_cb(void          *client_data,
                                   unsigned char id);


    OMX_ERRORTYPE set_callbacks(OMX_HANDLETYPE hComp,
                                OMX_CALLBACKTYPE *callbacks,
                                OMX_PTR appData);

    OMX_ERRORTYPE set_config(OMX_HANDLETYPE hComp,
                             OMX_INDEXTYPE configIndex,
                             OMX_PTR configData);

    OMX_ERRORTYPE set_parameter(OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE paramIndex,
                                OMX_PTR paramData);

    OMX_ERRORTYPE use_buffer(OMX_HANDLETYPE             hComp,
                             OMX_BUFFERHEADERTYPE **bufferHdr,
                             OMX_U32                     port,
                             OMX_PTR                  appData,
                             OMX_U32                    bytes,
                             OMX_U8                  *buffer);

    OMX_ERRORTYPE use_EGL_image(OMX_HANDLETYPE             hComp,
                                OMX_BUFFERHEADERTYPE **bufferHdr,
                                OMX_U32                     port,
                                OMX_PTR                  appData,
                                void *                  eglImage);

    bool post_command(unsigned int p1, unsigned int p2,
        unsigned char id);

    // Deferred callback identifiers
    enum
    {
        //Event Callbacks from the component thread context
        OMX_COMPONENT_GENERATE_EVENT       = 0x1,
        //Buffer Done callbacks from component thread context
        OMX_COMPONENT_GENERATE_BUFFER_DONE = 0x2,
        OMX_COMPONENT_GENERATE_ETB         = 0x3,
        //Command
        OMX_COMPONENT_GENERATE_COMMAND     = 0x4,
        OMX_COMPONENT_GENERATE_FRAME_DONE  = 0x05,
        OMX_COMPONENT_GENERATE_FTB         = 0x06,
        OMX_COMPONENT_GENERATE_EOS         = 0x07,
        OMX_COMPONENT_PORTSETTINGS_CHANGED = 0x08,
        OMX_COMPONENT_SUSPEND              = 0x09,
        OMX_COMPONENT_RESUME               = 0x0a
    };
private:

    ///////////////////////////////////////////////////////////
    // Type definitions
    ///////////////////////////////////////////////////////////
    // Bit Positions
    enum flags_bit_positions
    {
        // Defer transition to IDLE
        OMX_COMPONENT_IDLE_PENDING            =0x1,
        // Defer transition to LOADING
        OMX_COMPONENT_LOADING_PENDING         =0x2,

        OMX_COMPONENT_MUTED                   =0x3,

        // Defer transition to Enable
        OMX_COMPONENT_INPUT_ENABLE_PENDING    =0x4,
        // Defer transition to Enable
        OMX_COMPONENT_OUTPUT_ENABLE_PENDING   =0x5,
        // Defer transition to Disable
        OMX_COMPONENT_INPUT_DISABLE_PENDING   =0x6,
        // Defer transition to Disable
        OMX_COMPONENT_OUTPUT_DISABLE_PENDING  =0x7
    };


    typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
    input_buffer_map;

    typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
    output_buffer_map;

    enum port_indexes
    {
        OMX_CORE_INPUT_PORT_INDEX        =0,
        OMX_CORE_OUTPUT_PORT_INDEX       =1
    };

    struct omx_event
    {
        unsigned long param1;
        unsigned long param2;
        unsigned char id;
    };

    struct omx_cmd_queue
    {
        omx_event m_q[OMX_CORE_CONTROL_CMDQ_SIZE];
        unsigned m_read;
        unsigned m_write;
        unsigned m_size;

        omx_cmd_queue();
        ~omx_cmd_queue();
        bool insert_entry(unsigned long p1, unsigned long p2, unsigned char id);
        bool pop_entry(unsigned long *p1,unsigned long *p2, unsigned char *id);
        bool get_msg_id(unsigned char *id);
        bool get_msg_with_id(unsigned *p1,unsigned *p2, unsigned char id);
    };

    typedef struct TIMESTAMP
    {
        unsigned int LowPart;
        unsigned int HighPart;
    }__attribute__((packed)) TIMESTAMP;

    typedef struct metadata_input
    {
        unsigned short   offsetVal;
        TIMESTAMP      nTimeStamp;
        unsigned int   nFlags;
    }__attribute__((packed)) META_IN;

    typedef struct enc_meta_out
    {
        unsigned int offset_to_frame;
        unsigned int frame_size;
        unsigned int encoded_pcm_samples;
        unsigned int lsw_ts;
        unsigned int msw_ts;
        unsigned int nflags;
    } __attribute__ ((packed))ENC_META_OUT;

    typedef struct
    {
        OMX_U32 tot_in_buf_len;
        OMX_U32 tot_out_buf_len;
        OMX_TICKS tot_pb_time;
        OMX_U32 fbd_cnt;
        OMX_U32 ftb_cnt;
        OMX_U32 etb_cnt;
        OMX_U32 ebd_cnt;
    }EVRC_PB_STATS;

    ///////////////////////////////////////////////////////////
    // Member variables
    ///////////////////////////////////////////////////////////
    OMX_U8                         *m_tmp_meta_buf;
    OMX_U8                         *m_tmp_out_meta_buf;
    OMX_U8                         m_flush_cnt ;
    OMX_U8                         m_comp_deinit;

    // the below var doesnt hold good if combo of use and alloc bufs are used
    OMX_S32                        m_volume;//Unit to be determined
    OMX_PTR                        m_app_data;// Application data
    int                            nNumInputBuf;
    int                            nNumOutputBuf;
    int                            m_drv_fd;   // Kernel device node file handle
    bool                           bFlushinprogress;
    bool                           is_in_th_sleep;
    bool                           is_out_th_sleep;
    unsigned int                   m_flags;      //encapsulate the waiting states.
    OMX_TICKS                      nTimestamp;
    unsigned int                   pcm_input; //tunnel or non-tunnel
    unsigned int                   m_inp_act_buf_count;    // Num of Input Buffers
    unsigned int                   m_out_act_buf_count;    // Numb of Output Buffers
    unsigned int                   m_inp_current_buf_count;    // Num of Input Buffers
    unsigned int                   m_out_current_buf_count;    // Numb of Output Buffers
    unsigned int                   output_buffer_size;
    unsigned int                   input_buffer_size;
    unsigned short                 m_session_id;
    // store I/P PORT state
    OMX_BOOL                       m_inp_bEnabled;
    // store O/P PORT state
    OMX_BOOL                       m_out_bEnabled;
    //Input port Populated
    OMX_BOOL                       m_inp_bPopulated;
    //Output port Populated
    OMX_BOOL                       m_out_bPopulated;
    sem_t                          sem_States;
    sem_t                          sem_read_msg;
    sem_t                          sem_write_msg;

    volatile int                   m_is_event_done;
    volatile int                   m_is_in_th_sleep;
    volatile int                   m_is_out_th_sleep;
    input_buffer_map               m_input_buf_hdrs;
    output_buffer_map              m_output_buf_hdrs;
    omx_cmd_queue                  m_input_q;
    omx_cmd_queue                  m_input_ctrl_cmd_q;
    omx_cmd_queue                  m_input_ctrl_ebd_q;
    omx_cmd_queue                  m_command_q;
    omx_cmd_queue                  m_output_q;
    omx_cmd_queue                  m_output_ctrl_cmd_q;
    omx_cmd_queue                  m_output_ctrl_fbd_q;
    pthread_mutexattr_t            m_outputlock_attr;
    pthread_mutexattr_t            m_commandlock_attr;
    pthread_mutexattr_t            m_lock_attr;
    pthread_mutexattr_t            m_state_attr;
    pthread_mutexattr_t            m_flush_attr;
    pthread_mutexattr_t            m_in_th_attr_1;
    pthread_mutexattr_t            m_out_th_attr_1;
    pthread_mutexattr_t            m_event_attr;
    pthread_mutexattr_t            m_in_th_attr;
    pthread_mutexattr_t            m_out_th_attr;
    pthread_mutexattr_t            out_buf_count_lock_attr;
    pthread_mutexattr_t            in_buf_count_lock_attr;
    pthread_cond_t                 cond;
    pthread_cond_t                 in_cond;
    pthread_cond_t                 out_cond;
    pthread_mutex_t                m_lock;
    pthread_mutex_t                m_commandlock;
    pthread_mutex_t                m_outputlock;
    // Mutexes for state change
    pthread_mutex_t                m_state_lock;
    // Mutexes for  flush acks from input and output threads
    pthread_mutex_t                m_flush_lock;
    pthread_mutex_t                m_event_lock;
    pthread_mutex_t                m_in_th_lock;
    pthread_mutex_t                m_out_th_lock;
    pthread_mutex_t                m_in_th_lock_1;
    pthread_mutex_t                m_out_th_lock_1;
    pthread_mutex_t                out_buf_count_lock;
    pthread_mutex_t                in_buf_count_lock;

    OMX_STATETYPE                  m_state;      // OMX State
    OMX_STATETYPE                  nState;
    OMX_CALLBACKTYPE               m_cb;         // Application callbacks
    EVRC_PB_STATS                  m_evrc_pb_stats;
    struct evrc_ipc_info           *m_ipc_to_in_th;    // for input thread
    struct evrc_ipc_info           *m_ipc_to_out_th;    // for output thread
    struct evrc_ipc_info           *m_ipc_to_cmd_th;    // for command thread
    struct evrc_ipc_info          *m_ipc_to_event_th;    //for txco event thread
    OMX_PRIORITYMGMTTYPE           m_priority_mgm ;
    OMX_AUDIO_PARAM_EVRCTYPE m_evrc_param; // Cache EVRC encoder parameter
    OMX_AUDIO_PARAM_PCMMODETYPE    m_pcm_param;  // Cache pcm  parameter
    OMX_PARAM_COMPONENTROLETYPE    component_Role;
    OMX_PARAM_BUFFERSUPPLIERTYPE   m_buffer_supplier;

    ///////////////////////////////////////////////////////////
    // Private methods
    ///////////////////////////////////////////////////////////
    OMX_ERRORTYPE allocate_output_buffer(OMX_HANDLETYPE       hComp,
                                         OMX_BUFFERHEADERTYPE **bufferHdr,
                                         OMX_U32 port,OMX_PTR appData,
                                         OMX_U32              bytes);

    OMX_ERRORTYPE allocate_input_buffer(OMX_HANDLETYPE       hComp,
                                        OMX_BUFFERHEADERTYPE **bufferHdr,
                                        OMX_U32              port,
                                        OMX_PTR              appData,
                                        OMX_U32              bytes);

    OMX_ERRORTYPE use_input_buffer(OMX_IN OMX_HANDLETYPE          hComp,
                                   OMX_INOUT OMX_BUFFERHEADERTYPE **bufHdr,
                                   OMX_IN OMX_U32                 port,
                                   OMX_IN OMX_PTR                 appData,
                                   OMX_IN OMX_U32                 bytes,
                                   OMX_IN OMX_U8*                 buffer);

    OMX_ERRORTYPE use_output_buffer(OMX_IN OMX_HANDLETYPE          hComp,
                                    OMX_INOUT OMX_BUFFERHEADERTYPE **bufHdr,
                                    OMX_IN OMX_U32                 port,
                                    OMX_IN OMX_PTR                 appData,
                                    OMX_IN OMX_U32                 bytes,
                                    OMX_IN OMX_U8*                 buffer);

    OMX_ERRORTYPE fill_this_buffer_proxy(OMX_HANDLETYPE       hComp,
                                         OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE send_command_proxy(OMX_HANDLETYPE  hComp,
                                     OMX_COMMANDTYPE cmd,
                                     OMX_U32         param1,
                                     OMX_PTR         cmdData);

    OMX_ERRORTYPE send_command(OMX_HANDLETYPE hComp,
                               OMX_COMMANDTYPE  cmd,
                               OMX_U32       param1,
                               OMX_PTR      cmdData);

    bool allocate_done(void);

    bool release_done(OMX_U32         param1);

    bool execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl=true);

    bool execute_input_omx_flush(void);

    bool execute_output_omx_flush(void);

    bool search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer);

    bool search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer);

    bool post_input(unsigned long p1, unsigned long p2,
                    unsigned char id);

    bool post_output(unsigned long p1, unsigned long p2,
                     unsigned char id);

    void process_events(omx_evrc_aenc *client_data);

    void buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);

    void frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);

    void wait_for_event();

    void event_complete();

    void in_th_goto_sleep();

    void in_th_wakeup();

    void out_th_goto_sleep();

    void out_th_wakeup();

    void flush_ack();
    void deinit_encoder();

};
#endif
