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


//流队列中，视频流所在的位置
int ffap_video_index = -1;
//流队列中，音频流所在的位置
int ffap_audio_index = -1;

AVCodecContext* ffap_aVideoCodecContext = NULL;
AVCodecContext* ffap_aVAudioCodecContext = NULL;

AVCodec* ffap_aVideoCodec = NULL;
AVCodec* ffap_aVaudioCodec = NULL;

AVPacket* ffap_packet = NULL;
AVFormatContext* ffap_aVFormatContext = NULL;


char ffap_filepath[] = "C:\\Users\\Youzh\\Videos\\jxyy.mp4"; //读取本地文件地址

void ffap_start()
{
	
	ffap_initFFmpeg();

	//ffap_load_frame();

	SDL_CreateThread(sfp_refresh_thread, NULL, NULL);


	while (true)
	{
		printf("主线程.....\n");
		SDL_Delay(500);
	}
}

int thread_exit = 0;

int sfp_refresh_thread(void* opaque) {
	thread_exit = 0;
	while (!thread_exit) {
		printf("qwerty\n");
		SDL_Event event;
		//event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(400);
	}
	thread_exit = 0;
	//Break
	SDL_Event event;
	//event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);
	printf("SDL_PushEvent\n");
	return 0;
}

void ffap_load_frame() {

	//从packet中解出来的原始视频帧
	AVFrame* ffap_aVframe = av_frame_alloc();//返回一个填充默认值的AVFrame

	if (av_read_frame(ffap_aVFormatContext, ffap_packet) >= 0) {
		//av_read_frame:  根据AVFormatContext 读取packet信息
		if (ffap_packet->stream_index == ffap_video_index)//对比packet->stream_index 的流类型
		{

			//解码。输入为packet，输出为original_video_frame
			if (avcodec_send_packet(ffap_aVideoCodecContext, ffap_packet) == 0)//向解码器(AVCodecContext)发送需要解码的数据包(packet),0 表示解码成功
			{
				int ret = avcodec_receive_frame(ffap_aVideoCodecContext, ffap_aVframe);//AVCodecContext* video_codec_ctx;
				//avcodec_receive_frame: 从解码器获取解码后的一帧,0表是解释成功
				if (ret == 0)
				{

				}
				else {
					printf("没读取到!");
				}

			}
		}else if (ffap_packet->stream_index == ffap_audio_index) {

		}
	}

	av_packet_unref(ffap_packet);
	av_free(ffap_aVframe);

}

void ffap_initFFmpeg() {
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
		}

		ffap_aVideoCodec = avcodec_find_decoder(ffap_aVideoCodecContext->codec_id);

		if (ffap_aVideoCodec == NULL)
		{
			printf("VideoCodec not found.\n");
		}

		if (avcodec_open2(ffap_aVideoCodecContext, ffap_aVideoCodec, NULL) < 0)
		{
			printf("Could not open VideoCodec.\n");
		}
	}

	if (ffap_audio_index >= 0) {
		//获取音频解码器
		avcodec_parameters_to_context(ffap_aVAudioCodecContext, ffap_aVFormatContext->streams[ffap_audio_index]->codecpar);

		if (ffap_aVAudioCodecContext == NULL)
		{
			printf("Could not allocate ffap_aVAudioCodecContext\n");
		}

		ffap_aVaudioCodec = avcodec_find_decoder(ffap_aVAudioCodecContext->codec_id);

		if (ffap_aVaudioCodec == NULL)
		{
			printf("AudioCodec not found.\n");
		}

		if (avcodec_open2(ffap_aVAudioCodecContext, ffap_aVaudioCodec, NULL) < 0)
		{
			printf("Could not open AudioCodec.\n");
		}
	}
}


