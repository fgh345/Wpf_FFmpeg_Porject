#include "FFMpegAudioPlay.h"

#include <stdio.h>
//播放音视频

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
#include "SDL.h"
};


//ffap_

//流队列中，视频流所在的位置
int ffap_video_index = -1;
//流队列中，音频流所在的位置
int ffap_audio_index = -1;

AVCodecContext* ffap_aVideoCodecContext = NULL;
AVCodecContext* ffap_aVAudioCodecContext = NULL;

AVCodec* ffap_aVideoCodec = NULL;
AVCodec* ffap_aVaudioCodec = NULL;


AVFormatContext* ffap_aVFormatContext = NULL;


char ffap_filepath[] = "C:\\Users\\Youzh\\Videos\\jxyy.mp4"; //读取本地文件地址

void ffap_start()
{
	
	avformat_network_init();

	avdevice_register_all();

	ffap_aVFormatContext = avformat_alloc_context();
	ffap_aVideoCodecContext = avcodec_alloc_context3(NULL);
	ffap_aVAudioCodecContext = avcodec_alloc_context3(NULL);


	if (avformat_open_input(&ffap_aVFormatContext, ffap_filepath, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return;
	}

	if (avformat_find_stream_info(ffap_aVFormatContext, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return;
	}

	for (int i = 0; i < ffap_aVFormatContext->nb_streams; i++) {
		if (ffap_aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ffap_video_index = i;
			continue;
		}

		if (ffap_aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ffap_audio_index = i;
			continue;
		}
	}

	if (ffap_video_index >= 0)
	{
		//获取视频解码器
		avcodec_parameters_to_context(ffap_aVideoCodecContext, ffap_aVFormatContext->streams[ffap_video_index]->codecpar);

		if (ffap_aVideoCodecContext == NULL)
		{
			printf("Could not allocate ffap_aVideoCodecContext\n");
			return;
		}

		ffap_aVideoCodec = avcodec_find_decoder(ffap_aVideoCodecContext->codec_id);
	}

	if (ffap_audio_index >= 0) {
		//获取音频解码器
		avcodec_parameters_to_context(ffap_aVAudioCodecContext, ffap_aVFormatContext->streams[ffap_audio_index]->codecpar);

		if (ffap_aVAudioCodecContext == NULL)
		{
			printf("Could not allocate ffap_aVAudioCodecContext\n");
			return;
		}


	}

	
	



		
}


