#include "FFMpegAudioPlay.h"

#include <stdio.h>
//播放音频

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
#include "SDL.h"
};

//流队列中，音频流所在的位置
int ffap_audio_index = -1;

AVCodecContext* ffap_aVAudioCodecContext = NULL;

AVCodec* ffap_aVaudioCodec = NULL;

AVPacket* ffap_packet = NULL;
AVFormatContext* ffap_aVFormatContext = NULL;

unsigned char* outBuff = NULL;
int out_buffer_size = -1;   //输出buff
Uint32  audio_len;
Uint8* audio_pos;

struct SwrContext* au_convert_ctx = NULL;

char ffap_filepath[] = "..\\..\\Test_vedio.mp4"; //读取本地文件地址

FILE* ffap_file_out_pcm;

void  fill_audio(void* udata, Uint8* stream, int len);

void ffap_start()
{
	
	ffap_initFFmpeg();

	if (SDL_Init(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return;
	}
	else {

		//SDL_AudioSpec
		SDL_AudioSpec wanted_spec;
		wanted_spec.freq = 44100;
		wanted_spec.format = AUDIO_S16SYS;
		wanted_spec.channels = 2;
		wanted_spec.silence = 0;
		wanted_spec.samples = 1024;
		wanted_spec.callback = fill_audio;

		if (SDL_OpenAudio(&wanted_spec, NULL) < 0) {
			printf("can't open audio.\n");
			return;
		}

		//Play
		SDL_PauseAudio(0);

	}

	
	fopen_s(&ffap_file_out_pcm, "result_file_audio.pcm", "wb+");


	while (true)
	{
		ffap_load_frame();
	}
}

void ffap_load_frame() {

	//从packet中解出来的原始视频帧
	AVFrame* ffap_aVframe = av_frame_alloc();//返回一个填充默认值的AVFrame

	if (av_read_frame(ffap_aVFormatContext, ffap_packet) >= 0) {
		//av_read_frame:  根据AVFormatContext 读取packet信息
		if (ffap_packet->stream_index == ffap_audio_index) {
			//解码。输入为packet，输出为original_video_frame
			if (avcodec_send_packet(ffap_aVAudioCodecContext, ffap_packet) == 0)//向解码器(AVCodecContext)发送需要解码的数据包(packet),0 表示解码成功
			{
				int ret = avcodec_receive_frame(ffap_aVAudioCodecContext, ffap_aVframe);//AVCodecContext* video_codec_ctx;
				//avcodec_receive_frame: 从解码器获取解码后的一帧,0表是解释成功
				if (ret == 0)
				{
					int ret = swr_convert(au_convert_ctx, &outBuff, out_buffer_size, (const uint8_t**)ffap_aVframe->data, ffap_aVframe->nb_samples);

					printf("实际samples:%d 原:%d \n", ret, ffap_aVframe->nb_samples);

					//fwrite(outBuff, 1, out_buffer_size, ffap_file_out_pcm);

					audio_len = out_buffer_size;
					audio_pos = (Uint8*)outBuff;

					printf("outBuff:%d", *outBuff);

					while (audio_len > 0) {//Wait until finish
						SDL_Delay(1);
					}
				}

			}
		}
	}

	av_packet_unref(ffap_packet);
	av_free(ffap_aVframe);
	ffap_aVframe = NULL;
	printf("释放!\n");
}

void ffap_initFFmpeg() {
	avformat_network_init();

	avdevice_register_all();

	ffap_aVFormatContext = avformat_alloc_context();
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

		if (ffap_aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ffap_audio_index = i;
			continue;
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

	ffap_packet = av_packet_alloc();
	av_init_packet(ffap_packet);

	if (!ffap_packet)
		return;


	int64_t in_chn_layout = av_get_default_channel_layout(ffap_aVAudioCodecContext->channels);


	out_buffer_size = av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO), ffap_aVAudioCodecContext->frame_size, AV_SAMPLE_FMT_S16, 1);

	//outBuff = (unsigned char*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2); //双声道

	outBuff = new uint8_t[out_buffer_size];

	//Swr
	au_convert_ctx = swr_alloc_set_opts(NULL,
		AV_CH_LAYOUT_STEREO,                                /*out*/
		AV_SAMPLE_FMT_S16,                              /*out*/
		44100,                                         /*out*/
		in_chn_layout,                                  /*in*/
		ffap_aVAudioCodecContext->sample_fmt,               /*in*/
		ffap_aVAudioCodecContext->sample_rate,               /*in*/
		0,
		NULL);

	swr_init(au_convert_ctx);

	SDL_PauseAudio(0);
}

void  fill_audio(void* udata, Uint8* stream, int len) {
	//SDL 2.0

	SDL_memset(stream, 0, len);

	if (audio_len == 0)		/*  Only  play  if  we  have  data  left  */
		return;

	printf("len:%d --- audio_len:%d", len,audio_len);

	len = (len > audio_len ? audio_len : len);	/*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;

	//printf("audio_len:%d\n", audio_len);
}