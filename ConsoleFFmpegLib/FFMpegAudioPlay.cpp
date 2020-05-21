#include "FFMpegAudioPlay.h"

#include <stdio.h>
//��������Ƶ

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

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio  

//�������У���Ƶ�����ڵ�λ��
int ffap_video_index = -1;
//�������У���Ƶ�����ڵ�λ��
int ffap_audio_index = -1;

AVCodecContext* ffap_aVideoCodecContext = NULL;
AVCodecContext* ffap_aVAudioCodecContext = NULL;

AVCodec* ffap_aVideoCodec = NULL;
AVCodec* ffap_aVaudioCodec = NULL;

AVPacket* ffap_packet = NULL;
AVFormatContext* ffap_aVFormatContext = NULL;

unsigned char* outBuff = NULL;
int out_buffer_size = -1;   //���buff
Uint32  audio_len;
Uint8* audio_pos;

struct SwrContext* au_convert_ctx = NULL;

char ffap_filepath[] = "C:\\Users\\Youzh\\Videos\\jxyy.mp4"; //��ȡ�����ļ���ַ

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

	}

	while (true)
	{
		ffap_load_frame();
	}
}

void ffap_load_frame() {

	//��packet�н������ԭʼ��Ƶ֡
	AVFrame* ffap_aVframe = av_frame_alloc();//����һ�����Ĭ��ֵ��AVFrame

	if (av_read_frame(ffap_aVFormatContext, ffap_packet) >= 0) {
		//av_read_frame:  ����AVFormatContext ��ȡpacket��Ϣ
		if (ffap_packet->stream_index == ffap_video_index)//�Ա�packet->stream_index ��������
		{

			//���롣����Ϊpacket�����Ϊoriginal_video_frame
			if (avcodec_send_packet(ffap_aVideoCodecContext, ffap_packet) == 0)//�������(AVCodecContext)������Ҫ��������ݰ�(packet),0 ��ʾ����ɹ�
			{
				int ret = avcodec_receive_frame(ffap_aVideoCodecContext, ffap_aVframe);//AVCodecContext* video_codec_ctx;
				//avcodec_receive_frame: �ӽ�������ȡ������һ֡,0���ǽ��ͳɹ�
				if (ret == 0)
				{

				}
				else {
					printf("û��ȡ��!");
				}

			}
		}else if (ffap_packet->stream_index == ffap_audio_index) {
			//���롣����Ϊpacket�����Ϊoriginal_video_frame
			if (avcodec_send_packet(ffap_aVAudioCodecContext, ffap_packet) == 0)//�������(AVCodecContext)������Ҫ��������ݰ�(packet),0 ��ʾ����ɹ�
			{
				int ret = avcodec_receive_frame(ffap_aVAudioCodecContext, ffap_aVframe);//AVCodecContext* video_codec_ctx;
				//avcodec_receive_frame: �ӽ�������ȡ������һ֡,0���ǽ��ͳɹ�
				if (ret == 0)
				{
					swr_convert(au_convert_ctx, &outBuff, MAX_AUDIO_FRAME_SIZE, (const uint8_t**)ffap_aVframe->data, ffap_aVframe->nb_samples);
					//fwrite(outBuff, 1, out_buffer_size, outFile); 
					while (audio_len > 0)
						SDL_Delay(1);

					unsigned char*  audioChunk = (unsigned char*)outBuff;
					audio_pos = audioChunk;
					
					audio_len = out_buffer_size;
					printf("������");
				}
				else {
					printf("û��ȡ��!");
				}

			}
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
		//��ȡ��Ƶ������
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
		//��ȡ��Ƶ������
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


	outBuff = (unsigned char*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2); //˫����
	out_buffer_size = av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO), ffap_aVAudioCodecContext->frame_size, AV_SAMPLE_FMT_S16, 1);

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
	printf("ִ������");

	SDL_memset(stream, 0, len);
	if (audio_len == 0)		/*  Only  play  if  we  have  data  left  */
		return;
	len = (len > audio_len ? audio_len : len);	/*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}