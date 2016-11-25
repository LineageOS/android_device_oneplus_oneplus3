/*--------------------------------------------------------------------------
Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.

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


/*
    An Open max test application ....
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/ioctl.h>
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "pthread.h"
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
#include "QOMX_AudioExtensions.h"
#include "QOMX_AudioIndexExtensions.h"
#ifdef AUDIOV2 
#include "control.h" 
#endif
#include <linux/ioctl.h>

typedef unsigned char uint8;
typedef unsigned char byte;
typedef unsigned int  uint32;
typedef unsigned int  uint16;
#define AUDAAC_MAX_ADIF_HEADER_LENGTH 64
/* ADTS variable frame header, frame length field  */
#define AUDAAC_ADTS_FRAME_LENGTH_SIZE    13
QOMX_AUDIO_STREAM_INFO_DATA streaminfoparam;
void audaac_rec_install_bits
(
  uint8   *input,
  byte    num_bits_reqd,
  uint32  value,
  uint16  *hdr_bit_index
);

/* maximum ADTS frame header length                */
#define AUDAAC_MAX_ADTS_HEADER_LENGTH 7
void audaac_rec_install_adts_header_variable (uint16  byte_num);
void Release_Encoder();

#ifdef AUDIOV2
unsigned short session_id;
int device_id;
int control = 0;
const char *device="handset_tx";
#define DIR_TX 2
#endif

#define AACHDR_LAYER_SIZE             2
#define AACHDR_CRC_SIZE               1
#define AAC_PROFILE_SIZE              2
#define AAC_SAMPLING_FREQ_INDEX_SIZE  4
#define AAC_ORIGINAL_COPY_SIZE        1
#define AAC_HOME_SIZE                 1

#define MIN(A,B)    (((A) < (B))?(A):(B))

uint8   audaac_header[AUDAAC_MAX_ADTS_HEADER_LENGTH];
unsigned int audaac_hdr_bit_index;


FILE *F1 = NULL;

uint32_t samplerate = 44100;
uint32_t channels = 2;
uint32_t bitrate = 128000;
uint32_t pcmplayback = 0;
uint32_t tunnel      = 0;
uint32_t rectime     = 0;
uint32_t format = 1;
uint32_t profile = OMX_AUDIO_AACObjectLC;
#define DEBUG_PRINT printf
unsigned to_idle_transition = 0;

typedef enum adts_sample_index__ {

ADTS_SAMPLE_INDEX_96000=0x0,
ADTS_SAMPLE_INDEX_88200,
ADTS_SAMPLE_INDEX_64000,
ADTS_SAMPLE_INDEX_48000,
ADTS_SAMPLE_INDEX_44100,
ADTS_SAMPLE_INDEX_32000,
ADTS_SAMPLE_INDEX_24000,
ADTS_SAMPLE_INDEX_22050,
ADTS_SAMPLE_INDEX_16000,
ADTS_SAMPLE_INDEX_12000,
ADTS_SAMPLE_INDEX_11025,
ADTS_SAMPLE_INDEX_8000,
ADTS_SAMPLE_INDEX_7350,
ADTS_SAMPLE_INDEX_MAX

}adts_sample_index;
/************************************************************************/
/*                #DEFINES                            */
/************************************************************************/
#define false 0
#define true 1

#define CONFIG_VERSION_SIZE(param) \
    param.nVersion.nVersion = CURRENT_OMX_SPEC_VERSION;\
    param.nSize = sizeof(param);

#define FAILED(result) (result != OMX_ErrorNone)

#define SUCCEEDED(result) (result == OMX_ErrorNone)

/************************************************************************/
/*                GLOBAL DECLARATIONS                     */
/************************************************************************/

pthread_mutex_t lock;
pthread_cond_t cond;
pthread_mutex_t elock;
pthread_cond_t econd;
pthread_cond_t fcond;
pthread_mutex_t etb_lock;
pthread_mutex_t etb_lock1;
pthread_cond_t etb_cond;
FILE * inputBufferFile;
FILE * outputBufferFile;
OMX_PARAM_PORTDEFINITIONTYPE inputportFmt;
OMX_PARAM_PORTDEFINITIONTYPE outputportFmt;
OMX_AUDIO_PARAM_AACPROFILETYPE aacparam;
OMX_AUDIO_PARAM_PCMMODETYPE    pcmparam;
OMX_PORT_PARAM_TYPE portParam;
OMX_ERRORTYPE error;




#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
  uint32_t riff_id;
  uint32_t riff_sz;
  uint32_t riff_fmt;
  uint32_t fmt_id;
  uint32_t fmt_sz;
  uint16_t audio_format;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;       /* sample_rate * num_channels * bps / 8 */
  uint16_t block_align;     /* num_channels * bps / 8 */
  uint16_t bits_per_sample;
  uint32_t data_id;
  uint32_t data_sz;
};
struct enc_meta_out{
        unsigned int offset_to_frame;
        unsigned int frame_size;
        unsigned int encoded_pcm_samples;
        unsigned int msw_ts;
        unsigned int lsw_ts;
        unsigned int nflags;
} __attribute__ ((packed));

static int totaldatalen = 0;
/************************************************************************/
/*                GLOBAL INIT                    */
/************************************************************************/

unsigned int input_buf_cnt = 0;
unsigned int output_buf_cnt = 0;
int used_ip_buf_cnt = 0;
volatile int event_is_done = 0;
volatile int ebd_event_is_done = 0;
volatile int fbd_event_is_done = 0;
volatile int etb_event_is_done = 0;
int ebd_cnt;
int bInputEosReached = 0;
int bOutputEosReached = 0;
int bInputEosReached_tunnel = 0;
static int etb_done = 0;
int bFlushing = false;
int bPause    = false;
const char *in_filename;
const char *out_filename;

int timeStampLfile = 0;
int timestampInterval = 100;

//* OMX Spec Version supported by the wrappers. Version = 1.1 */
const OMX_U32 CURRENT_OMX_SPEC_VERSION = 0x00000101;
OMX_COMPONENTTYPE* aac_enc_handle = 0;

OMX_BUFFERHEADERTYPE  **pInputBufHdrs = NULL;
OMX_BUFFERHEADERTYPE  **pOutputBufHdrs = NULL;

/************************************************************************/
/*                GLOBAL FUNC DECL                        */
/************************************************************************/
int Init_Encoder(char*);
int Play_Encoder();
OMX_STRING aud_comp;
/**************************************************************************/
/*                STATIC DECLARATIONS                       */
/**************************************************************************/

static int open_audio_file ();
static int Read_Buffer(OMX_BUFFERHEADERTYPE  *pBufHdr );
static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *aac_enc_handle,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       unsigned int bufCntMin, unsigned int bufSize);


static OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData,
                                  OMX_IN OMX_EVENTTYPE eEvent,
                                  OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                                  OMX_IN OMX_PTR pEventData);
static OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_PTR pAppData,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE FillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_PTR pAppData,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE  parse_pcm_header();
void wait_for_event(void)
{
    pthread_mutex_lock(&lock);
    DEBUG_PRINT("%s: event_is_done=%d", __FUNCTION__, event_is_done);
    while (event_is_done == 0) {
        pthread_cond_wait(&cond, &lock);
    }
    event_is_done = 0;
    pthread_mutex_unlock(&lock);
}

void event_complete(void )
{
    pthread_mutex_lock(&lock);
    if (event_is_done == 0) {
        event_is_done = 1;
        pthread_cond_broadcast(&cond);
    }
    pthread_mutex_unlock(&lock);
}

void etb_wait_for_event(void)
{
    pthread_mutex_lock(&etb_lock1);
    DEBUG_PRINT("%s: etb_event_is_done=%d", __FUNCTION__, etb_event_is_done);
    while (etb_event_is_done == 0) {
        pthread_cond_wait(&etb_cond, &etb_lock1);
    }
    etb_event_is_done = 0;
    pthread_mutex_unlock(&etb_lock1);
}

void etb_event_complete(void )
{
    pthread_mutex_lock(&etb_lock1);
    if (etb_event_is_done == 0) {
        etb_event_is_done = 1;
        pthread_cond_broadcast(&etb_cond);
    }
    pthread_mutex_unlock(&etb_lock1);
}


OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                           OMX_IN OMX_PTR pAppData,
                           OMX_IN OMX_EVENTTYPE eEvent,
                           OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                           OMX_IN OMX_PTR pEventData)
{
    DEBUG_PRINT("Function %s \n", __FUNCTION__);
    /* To remove warning for unused variable to keep prototype same */
    (void)hComponent;
    (void)pAppData;
    (void)pEventData;

    switch(eEvent) {
        case OMX_EventCmdComplete:
        DEBUG_PRINT("\n OMX_EventCmdComplete event=%d data1=%u data2=%u\n",(OMX_EVENTTYPE)eEvent,
                                                                               nData1,nData2);
            event_complete();
        break;
        case OMX_EventError:
        DEBUG_PRINT("\n OMX_EventError \n");
        break;
         case OMX_EventBufferFlag:
             DEBUG_PRINT("\n OMX_EventBufferFlag \n");
             bOutputEosReached = true;
             event_complete();
             break;
        case OMX_EventPortSettingsChanged:
        DEBUG_PRINT("\n OMX_EventPortSettingsChanged \n");
        break;
        default:
        DEBUG_PRINT("\n Unknown Event \n");
        break;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE FillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_PTR pAppData,
                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    size_t bytes_writen = 0;
    size_t total_bytes_writen = 0;
    size_t len = 0;
    struct enc_meta_out *meta = NULL;
    OMX_U8 *src = pBuffer->pBuffer;
    unsigned int num_of_frames = 1;

    /* To remove warning for unused variable to keep prototype same */
    (void)pAppData;

        if(((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
            DEBUG_PRINT("FBD::EOS on output port\n ");
            bOutputEosReached = true;
            return OMX_ErrorNone;
        }
        if(bInputEosReached_tunnel || bOutputEosReached)
        {
            DEBUG_PRINT("EOS REACHED NO MORE PROCESSING OF BUFFERS\n");
            return OMX_ErrorNone;
        }
        if(num_of_frames != src[0]){

            printf("Data corrupt\n");
            return OMX_ErrorNone;
        }
        /* Skip the first bytes */



        src += sizeof(unsigned char);
        meta = (struct enc_meta_out *)src;
        while (num_of_frames > 0) {
            meta = (struct enc_meta_out *)src;
            /*printf("offset=%d framesize=%d encoded_pcm[%d] msw_ts[%d]lsw_ts[%d] nflags[%d]\n",
                                                                       meta->offset_to_frame,
                                                                       meta->frame_size,
                          meta->encoded_pcm_samples, meta->msw_ts, meta->lsw_ts, meta->nflags);*/
            len = meta->frame_size;

            if(format == 6)
            {
                audaac_rec_install_adts_header_variable((uint16_t)(len + AUDAAC_MAX_ADTS_HEADER_LENGTH));
                bytes_writen = fwrite(audaac_header,1,AUDAAC_MAX_ADTS_HEADER_LENGTH,outputBufferFile);
                if(bytes_writen < AUDAAC_MAX_ADTS_HEADER_LENGTH)
                {
                    DEBUG_PRINT("error: invalid adts header length\n");
                    return OMX_ErrorNone;
                }
            }
            bytes_writen = fwrite(pBuffer->pBuffer + sizeof(unsigned char) + meta->offset_to_frame,1,len,outputBufferFile);
            if(bytes_writen < len)
            {
                DEBUG_PRINT("error: invalid AAC encoded data \n");
                return OMX_ErrorNone;
            }
            src += sizeof(struct enc_meta_out);
            num_of_frames--;
            total_bytes_writen += len;
        }
        DEBUG_PRINT(" FillBufferDone size writen to file  %zu\n",total_bytes_writen);
        totaldatalen = totaldatalen + (int)total_bytes_writen;

        DEBUG_PRINT(" FBD calling FTB\n");
        OMX_FillThisBuffer(hComponent,pBuffer);

        return OMX_ErrorNone;
}

OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_PTR pAppData,
                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    int readBytes =0;

    /* To remove warning for unused variable to keep prototype same */
    (void)pAppData;

    ebd_cnt++;
    used_ip_buf_cnt--;
    pthread_mutex_lock(&etb_lock);
    if(!etb_done)
    {
        DEBUG_PRINT("\n*********************************************\n");
        DEBUG_PRINT("Wait till first set of buffers are given to component\n");
        DEBUG_PRINT("\n*********************************************\n");
        etb_done++;
        pthread_mutex_unlock(&etb_lock);
        etb_wait_for_event();
    }
    else
    {
        pthread_mutex_unlock(&etb_lock);
    }


    if(bInputEosReached)
    {
        DEBUG_PRINT("\n*********************************************\n");
        DEBUG_PRINT("   EBD::EOS on input port\n ");
        DEBUG_PRINT("*********************************************\n");
        return OMX_ErrorNone;
    }else if (bFlushing == true) {
      DEBUG_PRINT("omx_aac_adec_test: bFlushing is set to TRUE used_ip_buf_cnt=%d\n",used_ip_buf_cnt);
      if (used_ip_buf_cnt == 0) {
        bFlushing = false;
      } else {
        DEBUG_PRINT("omx_aac_adec_test: more buffer to come back used_ip_buf_cnt=%d\n",used_ip_buf_cnt);
        return OMX_ErrorNone;
      }
    }

    if((readBytes = Read_Buffer(pBuffer)) > 0) {
        pBuffer->nFilledLen = (OMX_U32)readBytes;
        used_ip_buf_cnt++;
        OMX_EmptyThisBuffer(hComponent,pBuffer);
    }
    else{
        pBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
        used_ip_buf_cnt++;
        bInputEosReached = true;
        pBuffer->nFilledLen = 0;
        OMX_EmptyThisBuffer(hComponent,pBuffer);
        DEBUG_PRINT("EBD..Either EOS or Some Error while reading file\n");
    }
    return OMX_ErrorNone;
}

void signal_handler(int sig_id) {

  /* Flush */
  if (sig_id == SIGUSR1) {
    DEBUG_PRINT("%s Initiate flushing\n", __FUNCTION__);
    bFlushing = true;
    OMX_SendCommand(aac_enc_handle, OMX_CommandFlush, OMX_ALL, NULL);
  } else if (sig_id == SIGUSR2) {
    if (bPause == true) {
      DEBUG_PRINT("%s resume playback\n", __FUNCTION__);
      bPause = false;
      OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    } else {
      DEBUG_PRINT("%s pause playback\n", __FUNCTION__);
      bPause = true;
      OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StatePause, NULL);
    }
  }
}

int main(int argc, char **argv)
{
     unsigned int bufCnt=0;
     OMX_ERRORTYPE result;

    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &signal_handler;
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

   (void) signal(SIGINT, Release_Encoder);

    pthread_cond_init(&cond, 0);
    pthread_mutex_init(&lock, 0);
    pthread_cond_init(&etb_cond, 0);
    pthread_mutex_init(&etb_lock, 0);
    pthread_mutex_init(&etb_lock1, 0);

    if (argc >= 9) {
      in_filename = argv[1];
      out_filename = argv[2];
      samplerate = (uint32_t)atoi(argv[3]);
      channels = (uint32_t)atoi(argv[4]);
      tunnel  = (uint32_t)atoi(argv[5]);
      rectime = (uint32_t)atoi(argv[6]);
      bitrate = (uint32_t)atoi(argv[7]);
      format =  (uint32_t)atoi(argv[8]);
      profile = (uint32_t)atoi(argv[9]);

	  DEBUG_PRINT("Input parameters: samplerate = %d, channels = %d, tunnel = %d,"
				  " rectime = %d, bitrate = %d, format = %d, profile = %d\n",
				  samplerate, channels, tunnel, rectime, bitrate, format, profile);

	  if (!((profile == 2) || (profile == 5) || (profile == 29))) {
		  DEBUG_PRINT("profile = %d, not supported. Supported "
					  "profile values are AAC_LC(2), AAC+(5), EAAC+(29)\n", profile);
		  return 0;
	  }
	  if (!((format == 1) || (format == 6)))  {
		  DEBUG_PRINT("format = %d, not supported. Supported "
					  "formats are ADTS(1), RAW(6)\n", format);
		  return 0;
	  }
	  if ((channels > 2) || (channels <= 0)) {
		  DEBUG_PRINT("channels = %d, not supported. Supported "
					  "number of channels are 1 and 2\n", channels);
		  return 0;
	  }
	  if ((samplerate < 8000) && (samplerate > 48000)) {
		  DEBUG_PRINT("samplerate = %d, not supported, Supported "
					  "samplerates are 8000, 11025, 12000, 16000, 22050, "
					  "24000, 32000, 44100, 48000\n", samplerate);
		  return 0;
	  } else {
		  if ((profile == 5) || (profile == 29)) {
			  if (samplerate < 24000) {
				  DEBUG_PRINT("samplerate = %d, not supported for AAC+/EAAC+."
							  " Supported samplerates are 24000, 32000,"
							  " 44100, 48000\n", samplerate);
				  return 0;
			  }
		  }
	  }
    } else {
        DEBUG_PRINT(" invalid format: \n");
        DEBUG_PRINT("ex: ./mm-aenc-omxaac INPUTFILE AAC_OUTPUTFILE SAMPFREQ CHANNEL TUNNEL RECORDTIME BITRATE FORMAT PROFILE\n");
        DEBUG_PRINT("FOR TUNNEL MOD PASS INPUT FILE AS ZERO\n");
        DEBUG_PRINT("RECORDTIME in seconds for AST Automation ...TUNNEL MODE ONLY\n");
        DEBUG_PRINT("FORMAT::ADTS(1), RAW(6)\n");
        DEBUG_PRINT("BITRATE in bits/sec \n");
        DEBUG_PRINT("PROFILE::AAC_LC(2), AAC+(5), EAAC+(29)\n");
        return 0;
    }
    if(tunnel == 0)
        aud_comp = "OMX.qcom.audio.encoder.aac";
    else
        aud_comp = "OMX.qcom.audio.encoder.tunneled.aac";
    if(Init_Encoder(aud_comp)!= 0x00)
    {
        DEBUG_PRINT("Decoder Init failed\n");
        return -1;
    }

    fcntl(0, F_SETFL, O_NONBLOCK);

    if(Play_Encoder() != 0x00)
    {
        DEBUG_PRINT("Play_Decoder failed\n");
        return -1;
    }

    // Wait till EOS is reached...
        if(rectime && tunnel)
        {
            sleep(rectime);
            rectime = 0;
            bInputEosReached_tunnel = 1;
            DEBUG_PRINT("\EOS ON INPUT PORT\n");
        }
        else
        {
            wait_for_event();
        }

        if((bInputEosReached_tunnel) || ((bOutputEosReached) && !tunnel))
        {

            DEBUG_PRINT("\nMoving the decoder to idle state \n");
            OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateIdle,0);
            wait_for_event();
            DEBUG_PRINT("\nMoving the encoder to loaded state \n");
            OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateLoaded,0);
            sleep(1);
            if (!tunnel)
            {
                DEBUG_PRINT("\nFillBufferDone: Deallocating i/p buffers \n");
                for(bufCnt=0; bufCnt < input_buf_cnt; ++bufCnt) {
                    OMX_FreeBuffer(aac_enc_handle, 0, pInputBufHdrs[bufCnt]);
                }
            }

            DEBUG_PRINT ("\nFillBufferDone: Deallocating o/p buffers \n");
            for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt) {
                OMX_FreeBuffer(aac_enc_handle, 1, pOutputBufHdrs[bufCnt]);
            }
            wait_for_event();

            result = OMX_FreeHandle(aac_enc_handle);
            if (result != OMX_ErrorNone) {
                DEBUG_PRINT ("\nOMX_FreeHandle error. Error code: %d\n", result);
            }
            /* Deinit OpenMAX */
            if(tunnel)
            {
                #ifdef AUDIOV2
                if (msm_route_stream(DIR_TX,session_id,device_id, 0))
                {
                    DEBUG_PRINT("\ncould not set stream routing\n");
                    return -1;
                }
                if (msm_en_device(device_id, 0))
                {
                    DEBUG_PRINT("\ncould not enable device\n");
                    return -1;
                }
                msm_mixer_close();
                #endif
            }
            OMX_Deinit();
            ebd_cnt=0;
            bOutputEosReached = false;
            bInputEosReached_tunnel = false;
            bInputEosReached = 0;
            aac_enc_handle = NULL;
            pthread_cond_destroy(&cond);
            pthread_mutex_destroy(&lock);
            fclose(outputBufferFile);
            DEBUG_PRINT("*****************************************\n");
            DEBUG_PRINT("******...AAC ENC TEST COMPLETED...***************\n");
            DEBUG_PRINT("*****************************************\n");
        }
        return 0;
}

void Release_Encoder()
{
    static int cnt=0;
    OMX_ERRORTYPE result;

    DEBUG_PRINT("END OF AAC ENCODING: EXITING PLEASE WAIT\n");
    bInputEosReached_tunnel = 1;
    event_complete();
    cnt++;
    if(cnt > 1)
    {
        /* FORCE RESET  */
        aac_enc_handle = NULL;
        ebd_cnt=0;
        bInputEosReached_tunnel = false;

        result = OMX_FreeHandle(aac_enc_handle);
        if (result != OMX_ErrorNone) {
            DEBUG_PRINT ("\nOMX_FreeHandle error. Error code: %d\n", result);
        }

        /* Deinit OpenMAX */

        OMX_Deinit();

        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&lock);
            DEBUG_PRINT("*****************************************\n");
            DEBUG_PRINT("******...AAC ENC TEST COMPLETED...***************\n");
            DEBUG_PRINT("*****************************************\n");
        exit(0);
    }
}

int Init_Encoder(OMX_STRING audio_component)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE omxresult;
    OMX_U32 total = 0;
    typedef OMX_U8* OMX_U8_PTR;
    char *role ="audio_encoder";

    static OMX_CALLBACKTYPE call_back = {
        &EventHandler,&EmptyBufferDone,&FillBufferDone
    };

    /* Init. the OpenMAX Core */
    DEBUG_PRINT("\nInitializing OpenMAX Core....\n");
    omxresult = OMX_Init();

    if(OMX_ErrorNone != omxresult) {
        DEBUG_PRINT("\n Failed to Init OpenMAX core");
          return -1;
    }
    else {
        DEBUG_PRINT("\nOpenMAX Core Init Done\n");
    }

    /* Query for audio decoders*/
    DEBUG_PRINT("Aac_test: Before entering OMX_GetComponentOfRole");
    OMX_GetComponentsOfRole(role, &total, 0);
    DEBUG_PRINT ("\nTotal components of role=%s :%u", role, total);


    omxresult = OMX_GetHandle((OMX_HANDLETYPE*)(&aac_enc_handle),
                        (OMX_STRING)audio_component, NULL, &call_back);
    if (FAILED(omxresult)) {
        DEBUG_PRINT("\nFailed to Load the component:%s\n", audio_component);
    return -1;
    }
    else
    {
        DEBUG_PRINT("\nComponent %s is in LOADED state\n", audio_component);
    }

    /* Get the port information */
    CONFIG_VERSION_SIZE(portParam);
    omxresult = OMX_GetParameter(aac_enc_handle, OMX_IndexParamAudioInit,
                                (OMX_PTR)&portParam);

    if(FAILED(omxresult)) {
        DEBUG_PRINT("\nFailed to get Port Param\n");
    return -1;
    }
    else
    {
        DEBUG_PRINT("\nportParam.nPorts:%u\n", portParam.nPorts);
    DEBUG_PRINT("\nportParam.nStartPortNumber:%u\n",
                                             portParam.nStartPortNumber);
    }
    return 0;
}

int Play_Encoder()
{
    unsigned int i;
    int Size=0;
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE ret;
    OMX_INDEXTYPE index;
#ifdef __LP64__
    DEBUG_PRINT("sizeof[%ld]\n", sizeof(OMX_BUFFERHEADERTYPE));
#else
    DEBUG_PRINT("sizeof[%d]\n", sizeof(OMX_BUFFERHEADERTYPE));
#endif

    /* open the i/p and o/p files based on the video file format passed */
    if(open_audio_file()) {
        DEBUG_PRINT("\n Returning -1");
    return -1;
    }

    /* Query the encoder input min buf requirements */
    CONFIG_VERSION_SIZE(inputportFmt);

    /* Port for which the Client needs to obtain info */
    inputportFmt.nPortIndex = portParam.nStartPortNumber;

    OMX_GetParameter(aac_enc_handle,OMX_IndexParamPortDefinition,&inputportFmt);
    DEBUG_PRINT ("\nEnc Input Buffer Count %u\n", inputportFmt.nBufferCountMin);
    DEBUG_PRINT ("\nEnc: Input Buffer Size %u\n", inputportFmt.nBufferSize);

    if(OMX_DirInput != inputportFmt.eDir) {
        DEBUG_PRINT ("\nEnc: Expect Input Port\n");
    return -1;
    }

    pcmparam.nPortIndex   = 0;
    pcmparam.nChannels    =  channels;
    pcmparam.nSamplingRate = samplerate;
    OMX_SetParameter(aac_enc_handle,OMX_IndexParamAudioPcm,&pcmparam);


    /* Query the encoder outport's min buf requirements */
    CONFIG_VERSION_SIZE(outputportFmt);
    /* Port for which the Client needs to obtain info */
    outputportFmt.nPortIndex = portParam.nStartPortNumber + 1;

    OMX_GetParameter(aac_enc_handle,OMX_IndexParamPortDefinition,&outputportFmt);
    DEBUG_PRINT ("\nEnc: Output Buffer Count %u\n", outputportFmt.nBufferCountMin);
    DEBUG_PRINT ("\nEnc: Output Buffer Size %u\n", outputportFmt.nBufferSize);

    if(OMX_DirOutput != outputportFmt.eDir) {
        DEBUG_PRINT ("\nEnc: Expect Output Port\n");
    return -1;
    }


    CONFIG_VERSION_SIZE(aacparam);


    aacparam.nPortIndex   =  1;
    aacparam.nChannels    =  channels; //2 ; /* 1-> mono 2-> stereo*/
    aacparam.nBitRate     =  bitrate;
    aacparam.nSampleRate  =  samplerate;
    aacparam.eChannelMode =  OMX_AUDIO_ChannelModeStereo;
    aacparam.eAACStreamFormat    =  (OMX_AUDIO_AACSTREAMFORMATTYPE)format;
    aacparam.eAACProfile = (OMX_AUDIO_AACPROFILETYPE)profile;
    OMX_SetParameter(aac_enc_handle,OMX_IndexParamAudioAac,&aacparam);
    OMX_GetExtensionIndex(aac_enc_handle,"OMX.Qualcomm.index.audio.sessionId",&index);
    OMX_GetParameter(aac_enc_handle,index,&streaminfoparam);
    if(tunnel)
    {
    #ifdef AUDIOV2
    session_id = streaminfoparam.sessionId;
    control = msm_mixer_open("/dev/snd/controlC0", 0);
    if(control < 0)
    printf("ERROR opening the device\n");
    device_id = msm_get_device(device);
    DEBUG_PRINT ("\ndevice_id = %d\n",device_id);
    DEBUG_PRINT("\nsession_id = %d\n",session_id);
    if (msm_en_device(device_id, 1))
    {
        perror("could not enable device\n");
        return -1;
    }

    if (msm_route_stream(DIR_TX,session_id,device_id, 1))
    {
        perror("could not set stream routing\n");
        return -1;
    }
    #endif
    }
    DEBUG_PRINT ("\nOMX_SendCommand Encoder -> IDLE\n");
    OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateIdle,0);
    /* wait_for_event(); should not wait here event complete status will
       not come until enough buffer are allocated */
    if (tunnel == 0)
    {
        input_buf_cnt = inputportFmt.nBufferCountActual; //  inputportFmt.nBufferCountMin + 5;
        DEBUG_PRINT("Transition to Idle State succesful...\n");
        /* Allocate buffer on decoder's i/p port */
        error = Allocate_Buffer(aac_enc_handle, &pInputBufHdrs, inputportFmt.nPortIndex,
                            input_buf_cnt, inputportFmt.nBufferSize);
        if (error != OMX_ErrorNone || pInputBufHdrs == NULL) {
            DEBUG_PRINT ("\nOMX_AllocateBuffer Input buffer error\n");
        return -1;
    }
    else {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Input buffer success\n");
    }
    }
    output_buf_cnt = outputportFmt.nBufferCountMin ;

    /* Allocate buffer on encoder's O/Pp port */
    error = Allocate_Buffer(aac_enc_handle, &pOutputBufHdrs, outputportFmt.nPortIndex,
                            output_buf_cnt, outputportFmt.nBufferSize);
    if (error != OMX_ErrorNone || pOutputBufHdrs == NULL) {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer error\n");
    return -1;
    }
    else {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer success\n");
    }

    wait_for_event();


    if (tunnel == 1)
    {
        DEBUG_PRINT ("\nOMX_SendCommand to enable TUNNEL MODE during IDLE\n");
        OMX_SendCommand(aac_enc_handle, OMX_CommandPortDisable,0,0); // disable input port
        wait_for_event();
    }

    DEBUG_PRINT ("\nOMX_SendCommand encoder -> Executing\n");
    OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateExecuting,0);
    wait_for_event();

    DEBUG_PRINT(" Start sending OMX_FILLthisbuffer\n");

    for(i=0; i < output_buf_cnt; i++) {
        DEBUG_PRINT ("\nOMX_FillThisBuffer on output buf no.%d\n",i);
        pOutputBufHdrs[i]->nOutputPortIndex = 1;
        pOutputBufHdrs[i]->nFlags = pOutputBufHdrs[i]->nFlags & (unsigned)~OMX_BUFFERFLAG_EOS;
        ret = OMX_FillThisBuffer(aac_enc_handle, pOutputBufHdrs[i]);
        if (OMX_ErrorNone != ret) {
            DEBUG_PRINT("OMX_FillThisBuffer failed with result %d\n", ret);
    }
        else {
            DEBUG_PRINT("OMX_FillThisBuffer success!\n");
    }
    }

if(tunnel == 0)
{
    DEBUG_PRINT(" Start sending OMX_emptythisbuffer\n");
    for (i = 0;i < input_buf_cnt;i++) {
        DEBUG_PRINT ("\nOMX_EmptyThisBuffer on Input buf no.%d\n",i);
        pInputBufHdrs[i]->nInputPortIndex = 0;
        Size = Read_Buffer(pInputBufHdrs[i]);
        if(Size <=0 ){
          DEBUG_PRINT("NO DATA READ\n");
          bInputEosReached = true;
          pInputBufHdrs[i]->nFlags= OMX_BUFFERFLAG_EOS;
        }
        pInputBufHdrs[i]->nFilledLen = (OMX_U32)Size;
        pInputBufHdrs[i]->nInputPortIndex = 0;
        used_ip_buf_cnt++;
        ret = OMX_EmptyThisBuffer(aac_enc_handle, pInputBufHdrs[i]);
        if (OMX_ErrorNone != ret) {
            DEBUG_PRINT("OMX_EmptyThisBuffer failed with result %d\n", ret);
        }
        else {
            DEBUG_PRINT("OMX_EmptyThisBuffer success!\n");
        }
        if(Size <=0 ){
            break;//eos reached
        }
    }
    pthread_mutex_lock(&etb_lock);
    if(etb_done)
{
        DEBUG_PRINT("Component is waiting for EBD to be released.\n");
        etb_event_complete();
    }
    else
    {
        DEBUG_PRINT("\n****************************\n");
        DEBUG_PRINT("EBD not yet happened ...\n");
        DEBUG_PRINT("\n****************************\n");
        etb_done++;
    }
    pthread_mutex_unlock(&etb_lock);
}

    return 0;
}



static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *avc_enc_handle,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       unsigned int bufCntMin, unsigned int bufSize)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE error=OMX_ErrorNone;
    unsigned int bufCnt=0;
    /* To remove warning for unused variable to keep prototype same */
    (void)avc_enc_handle;

    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)
                   malloc(sizeof(OMX_BUFFERHEADERTYPE*)*bufCntMin);

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
        DEBUG_PRINT("\n OMX_AllocateBuffer No %d \n", bufCnt);
        error = OMX_AllocateBuffer(aac_enc_handle, &((*pBufHdrs)[bufCnt]),
                                   nPortIndex, NULL, bufSize);
    }

    return error;
}




static int Read_Buffer (OMX_BUFFERHEADERTYPE  *pBufHdr )
{

    size_t bytes_read=0;


    pBufHdr->nFilledLen = 0;
    pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;

     bytes_read = fread(pBufHdr->pBuffer, 1, pBufHdr->nAllocLen , inputBufferFile);

      pBufHdr->nFilledLen = (OMX_U32)bytes_read;
        if(bytes_read == 0)
        {

          pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
          DEBUG_PRINT ("\nBytes read zero\n");
        }
        else
        {
            pBufHdr->nFlags = pBufHdr->nFlags & (unsigned)~OMX_BUFFERFLAG_EOS;
        }

    return (int)bytes_read;
}



//In Encoder this Should Open a PCM or WAV file for input.

static int open_audio_file ()
{
    int error_code = 0;

    if (!tunnel)
    {
        DEBUG_PRINT("Inside %s filename=%s\n", __FUNCTION__, in_filename);
        inputBufferFile = fopen (in_filename, "rb");
        if (inputBufferFile == NULL) {
            DEBUG_PRINT("\ni/p file %s could NOT be opened\n",
                                         in_filename);
        error_code = -1;
        }
        if(parse_pcm_header() != 0x00)
        {
            DEBUG_PRINT("PCM parser failed \n");
            return -1;
        }
    }

    DEBUG_PRINT("Inside %s filename=%s\n", __FUNCTION__, out_filename);
    outputBufferFile = fopen (out_filename, "wb");
    if (outputBufferFile == NULL) {
        DEBUG_PRINT("\ni/p file %s could NOT be opened\n",
                                         out_filename);
    error_code = -1;
    }
    return error_code;
}


void audaac_rec_install_bits
(
  uint8   *input,
  byte    num_bits_reqd,
  uint32  value,
  uint16  *hdr_bit_index
)
{
  uint32 byte_index;
  byte   bit_index;
  byte   bits_avail_in_byte;
  byte   num_to_copy;
  byte   byte_to_copy;

  byte   num_remaining = num_bits_reqd;
  uint8  bit_mask;

  bit_mask = 0xFF;

  while (num_remaining) {

    byte_index = (*hdr_bit_index) >> 3;
    bit_index  = (*hdr_bit_index) &  0x07;

    bits_avail_in_byte = (uint8)(8 - bit_index);

    num_to_copy = MIN(bits_avail_in_byte, num_remaining);

    byte_to_copy = (uint8)(((value >> (num_remaining - num_to_copy)) & 0xFF) <<
                    (bits_avail_in_byte - num_to_copy));

    input[byte_index] &= ((uint8)(bit_mask << bits_avail_in_byte));
    input[byte_index] |= byte_to_copy;

    *hdr_bit_index += num_to_copy;

    num_remaining = (uint8)(num_remaining - num_to_copy);
  } /* while (num_remaining) */
} /* audaac_rec_install_bits */

adts_sample_index  map_adts_sample_index(uint32 srate)
{
  adts_sample_index ret;

  switch(srate){

     case 96000:
     ret= ADTS_SAMPLE_INDEX_96000;
     break;
     case 88200:
     ret= ADTS_SAMPLE_INDEX_88200;
     break;
     case 64000:
     ret= ADTS_SAMPLE_INDEX_64000;
     break;
     case 48000:
     ret=ADTS_SAMPLE_INDEX_48000;
     break;
     case 44100:
     ret=ADTS_SAMPLE_INDEX_44100;
     break;
     case 32000:
     ret=ADTS_SAMPLE_INDEX_32000;
     break;
     case 24000:
     ret=ADTS_SAMPLE_INDEX_24000;
     break;
     case 22050:
     ret=ADTS_SAMPLE_INDEX_22050;
     break;
     case 16000:
     ret=ADTS_SAMPLE_INDEX_16000;
     break;
     case 12000:
     ret=ADTS_SAMPLE_INDEX_12000;
     break;
     case 11025:
     ret=ADTS_SAMPLE_INDEX_11025;
     break;
     case 8000:
     ret=ADTS_SAMPLE_INDEX_8000;
     break;
     case 7350:
     ret=ADTS_SAMPLE_INDEX_7350;
     break;
     default:
     ret=ADTS_SAMPLE_INDEX_44100;
     break;
    }
  return ret;
}

void audaac_rec_install_adts_header_variable (uint16  byte_num)
{
  //uint16  bit_index=0;

  adts_sample_index srate_enum;
  uint32  value;

  uint32   sample_index = samplerate;
  uint8   channel_config = (uint8)channels;

  /* Store Sync word first */
  audaac_header[0] = 0xFF;
  audaac_header[1] = 0xF0;

  audaac_hdr_bit_index = 12;

  if ((format == OMX_AUDIO_AACStreamFormatRAW) &&
      ((profile == OMX_AUDIO_AACObjectHE) ||
       (profile == OMX_AUDIO_AACObjectHE_PS))){
      if (samplerate >= 24000)
          sample_index = samplerate/2;
  }

  /* ID field, 1 bit */
  value = 1;
  audaac_rec_install_bits(audaac_header,
                          1,
                          value,
                          &(audaac_hdr_bit_index));

  /* Layer field, 2 bits */
  value = 0;
  audaac_rec_install_bits(audaac_header,
                          AACHDR_LAYER_SIZE,
                          value,
                          &(audaac_hdr_bit_index));

  /* Protection_absent field, 1 bit */
  value = 1;
  audaac_rec_install_bits(audaac_header,
                          AACHDR_CRC_SIZE,
                          value,
                          &(audaac_hdr_bit_index));

  /* profile_ObjectType field, 2 bit */
  value = 1;
  audaac_rec_install_bits(audaac_header,
                          AAC_PROFILE_SIZE,
                          value,
                          &(audaac_hdr_bit_index));

  /* sampling_frequency_index field, 4 bits */
  srate_enum = map_adts_sample_index(sample_index);
  audaac_rec_install_bits(audaac_header,
                          AAC_SAMPLING_FREQ_INDEX_SIZE,
                          (uint32)srate_enum,
                          &(audaac_hdr_bit_index));

  DEBUG_PRINT("%s: sample_index=%d; srate_enum = %d \n",
			  __FUNCTION__, sample_index, srate_enum);

  /* pravate_bit field, 1 bits */
  audaac_rec_install_bits(audaac_header,
                          1,
                          0,
                          &(audaac_hdr_bit_index));

  /* channel_configuration field, 3 bits */
  audaac_rec_install_bits(audaac_header,
                          3,
                          channel_config,
                          &(audaac_hdr_bit_index));


  /* original/copy field, 1 bits */
  audaac_rec_install_bits(audaac_header,
                          AAC_ORIGINAL_COPY_SIZE,
                          0,
                          &(audaac_hdr_bit_index));


  /* home field, 1 bits */
  audaac_rec_install_bits(audaac_header,
                          AAC_HOME_SIZE,
                          0,
                          &(audaac_hdr_bit_index));

 // bit_index = audaac_hdr_bit_index;
 // bit_index += 2;

    /* copyr. id. bit, 1 bits */
  audaac_rec_install_bits(audaac_header,
                          1,
                          0,
                          &(audaac_hdr_bit_index));

    /* copyr. id. start, 1 bits */
  audaac_rec_install_bits(audaac_header,
                          1,
                          0,
                          &(audaac_hdr_bit_index));

  /* aac_frame_length field, 13 bits */
  audaac_rec_install_bits(audaac_header,
                          AUDAAC_ADTS_FRAME_LENGTH_SIZE,
                          byte_num,
                          &audaac_hdr_bit_index);

  /* adts_buffer_fullness field, 11 bits */
  audaac_rec_install_bits(audaac_header,
                          11,
                          0x660,/*0x660 = CBR,0x7FF = VBR*/
                          &audaac_hdr_bit_index);

  /* number_of_raw_data_blocks_in_frame, 2 bits */
  audaac_rec_install_bits(audaac_header,
                          2,
                          0,
                          &audaac_hdr_bit_index);

} /* audaac_rec_install_adts_header_variable */

static OMX_ERRORTYPE parse_pcm_header()
{
    struct wav_header hdr;

    DEBUG_PRINT("\n***************************************************************\n");
    if(fread(&hdr, 1, sizeof(hdr),inputBufferFile)!=sizeof(hdr))
    {
        DEBUG_PRINT("Wav file cannot read header\n");
        return -1;
    }

    if ((hdr.riff_id != ID_RIFF) ||
        (hdr.riff_fmt != ID_WAVE)||
        (hdr.fmt_id != ID_FMT))
    {
        DEBUG_PRINT("Wav file is not a riff/wave file\n");
        return -1;
    }

    if (hdr.audio_format != FORMAT_PCM)
    {
        DEBUG_PRINT("Wav file is not adpcm format %d and fmt size is %d\n",
                      hdr.audio_format, hdr.fmt_sz);
        return -1;
    }

    DEBUG_PRINT("Samplerate is %d\n", hdr.sample_rate);
    DEBUG_PRINT("Channel Count is %d\n", hdr.num_channels);
    DEBUG_PRINT("\n***************************************************************\n");

    samplerate = hdr.sample_rate;
    channels = hdr.num_channels;

    return OMX_ErrorNone;
}
