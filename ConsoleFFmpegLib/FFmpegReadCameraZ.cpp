#include <stdio.h>
#include "FFmpegReadCameraZ.h"

//��ȡdshow��� ���ѻ�����SDL2չʾ����,�������洢����

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

//����ǰ׺ frcz_

//����Ҫ��Ⱦ���Ĵ���
SDL_Window* frcz_window = NULL;

int frcz_screen_w = 0;
int frcz_screen_h = 0;

//The window renderer
SDL_Renderer* frcz_Renderer = NULL;

//Current displayed texture
SDL_Texture* frcz_Texture = NULL;

//ѭ�����
bool frcz_isQuit = false;

//�¼����
SDL_Event frcz_event;

#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

AVPacket* frcz_packet_video = NULL;
//AVPacket* frcz_packet_audio = NULL;

AVFormatContext* frcz_aVFormatContext_camera = NULL;
//AVFormatContext* frcz_aVFormatContext_audio = NULL;

AVFrame* frcz_aVframe_video = NULL;
//AVFrame* frcz_aVframe_audio = NULL;
AVFrame* frcz_aVframeYUV = av_frame_alloc();

AVCodecContext* frcz_aVCodecContext_video = NULL;
//AVCodecContext* frcz_aVCodecContext_audio = NULL;

struct SwsContext* frcz_swsContext = NULL;//����ת��
//struct SwrContext* frcz_swrContext = NULL;//��Ƶת��

AVPixelFormat frcz_pix_format = AV_PIX_FMT_YUV420P;

int frcz_video_index = -1;
//int frcz_audio_index = -1;

SDL_Rect frcz_sdlRect;

int frcz_video_out_buffer_size = 0; //��Ƶ���buffer ��С
uint8_t* frcz_video_out_buffer;//��Ƶ���buffer

int frcz_audio_out_buffer_size = -1;   //��Ƶ���buffer ��С
//uint8_t* frcz_audio_out_buffer;//��Ƶ���buffer

FILE* frcz_YUV_FILE;//�洢ΪYUV��ʽ�ļ�

//Uint32  frcz_audio_len;//��Ҫ���ŵ����ݳ���
//Uint8* frcz_audio_pos;//��Ƶ�Ѿ����ŵ���λ��


//��Ƶ���Żص�
void  frcz_fill_audio_back(void* udata, Uint8* stream, int len);


void startReadCameraZ() {

	printf("start!\n");

	if (frcz_initFFmpeg()!=0)
	{
		printf("initFFmpeg error!");
		return;
	}

	int err = fopen_s(&frcz_YUV_FILE, "CameraFrame.yuv", "wb+");//���ļ���

	frcz_open_window_fun();

	frcz_release();
	printf("end!");
}

int frcz_initFFmpeg() {

	avdevice_register_all();

	frcz_aVFormatContext_camera = avformat_alloc_context();
	//frcz_aVFormatContext_audio = avformat_alloc_context();

	frcz_aVCodecContext_video = avcodec_alloc_context3(NULL);
	//frcz_aVCodecContext_audio = avcodec_alloc_context3(NULL);

	//Show Dshow Device
	//show_dshow_device();
	//Show Device Options
	//show_dshow_device_option();
	//Show VFW Options
	//show_vfw_device();

	AVInputFormat* frcz_aVInputFormat = av_find_input_format("dshow");

	AVDictionary* frcz_options_v = NULL;
	AVDictionary* frcz_options_a = NULL;
	av_dict_set_int(&frcz_options_v, "rtbufsize", 3041280 * 20, 0);//Ĭ�ϴ�С3041280  ���ú�����ӳ� ��Ϊ�����ٶȸ�����
	av_dict_set_int(&frcz_options_a, "rtbufsize", 3041280 * 20, 0);//Ĭ�ϴ�С3041280  ���ú�����ӳ� ��Ϊ�����ٶȸ�����

	//Set own video device's name              Surface Camera Front  USB2.0 Camera
	if (avformat_open_input(&frcz_aVFormatContext_camera, "video=USB2.0 Camera", frcz_aVInputFormat, &frcz_options_v) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}

	//if (avformat_open_input(&frcz_aVFormatContext_audio, "audio=HKZ (Realtek(R) Audio)", frcz_aVInputFormat, &frcz_options_a) != 0) {
	//	printf("Couldn't open input stream.\n");
	//	return -1;
	//}

	if (frcz_aVFormatContext_camera ==NULL || avformat_find_stream_info(frcz_aVFormatContext_camera, NULL) < 0)
	{
		printf("Couldn't find stream information v.\n");
		return -1;
	}

	//if (frcz_aVFormatContext_audio == NULL || avformat_find_stream_info(frcz_aVFormatContext_audio, NULL) < 0)
	//{
	//	printf("Couldn't find stream information a.\n");
	//	return -1;
	//}


	for (int i = 0; i < frcz_aVFormatContext_camera->nb_streams; i++) {
		if (frcz_aVFormatContext_camera->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			frcz_video_index = i;
			break;
		}
	}

	//for (int i = 0; i < frcz_aVFormatContext_audio->nb_streams; i++) {
	//	if (frcz_aVFormatContext_audio->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
	//	{
	//		frcz_audio_index = i;
	//		break;
	//	}
	//}


	//if (frcz_video_index == -1 || frcz_audio_index == -1)
	//{
	//	printf("Couldn't find a video or audio stream.\n");
	//	return -1;
	//}

	avcodec_parameters_to_context(frcz_aVCodecContext_video, frcz_aVFormatContext_camera->streams[frcz_video_index]->codecpar);
	//avcodec_parameters_to_context(frcz_aVCodecContext_audio, frcz_aVFormatContext_audio->streams[frcz_audio_index]->codecpar);

	if (frcz_aVCodecContext_video == NULL)
	{
		printf("Could not allocate CodecContext_video\n");
		return -1;
	}

	//if (frcz_aVCodecContext_audio == NULL)
	//{
	//	printf("Could not allocate CodecContext_audio\n");
	//	return -1;
	//}

	AVCodec* frcz_aVCodec_video = avcodec_find_decoder(frcz_aVCodecContext_video->codec_id);
	//AVCodec* frcz_aVCodec_audio = avcodec_find_decoder(frcz_aVCodecContext_audio->codec_id);


	if (frcz_aVCodec_video == NULL || avcodec_open2(frcz_aVCodecContext_video, frcz_aVCodec_video, NULL) < 0)
	{
		printf("Could not open video codec.\n");
		return -1;
	}

	//if (frcz_aVCodec_audio == NULL || avcodec_open2(frcz_aVCodecContext_audio, frcz_aVCodec_audio, NULL) < 0)
	//{
	//	printf("Could not open audio codec.\n");
	//	return -1;
	//}


	//frcz_screen_w = frcz_aVCodecContext_video->width;
	//frcz_screen_h = frcz_aVCodecContext_video->height;

	frcz_screen_w = 200;
	frcz_screen_h = 200;

#pragma region ���ظ�ʽת��

	int width_out = 200;
	int height_out = 200;

	int width = frcz_aVCodecContext_video->width > width_out ? frcz_aVCodecContext_video->width : width_out;
	int height = frcz_aVCodecContext_video->height > height_out ? frcz_aVCodecContext_video->height : height_out;

	//�����������
	frcz_video_out_buffer_size = av_image_get_buffer_size(
		frcz_pix_format,//out pix 614400  420p 460800
		200,
		200,
		1);

	//�������
	frcz_video_out_buffer = new uint8_t[frcz_video_out_buffer_size];

	//׼��һЩ����������Ƶ��ʽת���󣬲�����������ֵ  �����Դ�뿴 �ǽ� video_out_buffer ���� video_out_frame->data ���� ���Ժ������ video_out_frame->data ��Ӱ��video_out_buffer 
	av_image_fill_arrays(
		frcz_aVframeYUV->data,//out
		frcz_aVframeYUV->linesize,//out
		frcz_video_out_buffer,//out
		frcz_pix_format,//out
		200,//in
		200,//in
		1);

	frcz_swsContext = sws_getContext(
		frcz_aVCodecContext_video->width,//in
		frcz_aVCodecContext_video->height,//in
		frcz_aVCodecContext_video->pix_fmt,//in
		200,//out
		200,//out
		frcz_pix_format,//out
		SWS_BICUBIC,
		NULL, NULL, NULL);

#pragma endregion

#pragma region ��Ƶ��ʽת��

	//int64_t in_chn_layout = av_get_default_channel_layout(frcz_aVCodecContext_audio->channels);

	//frcz_audio_out_buffer_size = av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO), 1024, AV_SAMPLE_FMT_S16, 1);

	////outBuff = (unsigned char*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2); //˫����

	//frcz_audio_out_buffer = new uint8_t[frcz_audio_out_buffer_size];

	////Swr
	//frcz_swrContext = swr_alloc_set_opts(
	//	NULL,
	//	AV_CH_LAYOUT_STEREO,                                /*out*/
	//	AV_SAMPLE_FMT_S16,                              /*out*/
	//	44100,                                         /*out*/
	//	in_chn_layout,                                  /*in*/
	//	frcz_aVCodecContext_audio->sample_fmt,               /*in*/
	//	frcz_aVCodecContext_audio->sample_rate,               /*in*/
	//	0,
	//	NULL);

	//swr_init(frcz_swrContext);
#pragma endregion

	frcz_packet_video = av_packet_alloc();
	//frcz_packet_audio = av_packet_alloc();
	av_init_packet(frcz_packet_video);
	//av_init_packet(frcz_packet_audio);

	return 0;
}

//Show Dshow Device
void show_dshow_device() {
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat* iformat = av_find_input_format("dshow");
	printf("========Device Info=============\n");
	avformat_open_input(&pFormatCtx, "video=USB2.0 Camera", iformat, &options);
	printf("================================\n");
}

//Show Dshow Device Option
void show_dshow_device_option() {
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_options", "true", 0);
	AVInputFormat* iformat = av_find_input_format("dshow");
	printf("========Device Option Info======\n");
	avformat_open_input(&pFormatCtx, "video=USB2.0 Camera", iformat, &options);
	printf("================================\n");
}

//Show VFW Device
void show_vfw_device() {
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat* iformat = av_find_input_format("avfoundation");
	printf("==AVFoundation Device Info===\n");
	avformat_open_input(&pFormatCtx, "", iformat, &options);
	printf("=============================\n");
}

//��ʼ��SDL����
bool frcz_initSDLWindow()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Create window
		frcz_window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, frcz_screen_w, frcz_screen_h, SDL_WINDOW_SHOWN);
		if (frcz_window == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{

			//Create renderer for window
			frcz_Renderer = SDL_CreateRenderer(frcz_window, -1, SDL_RENDERER_ACCELERATED);
			if (frcz_Renderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				frcz_Texture = SDL_CreateTexture(frcz_Renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, frcz_screen_w, frcz_screen_h);

				frcz_sdlRect.x = 0;
				frcz_sdlRect.y = 0;
				frcz_sdlRect.w = frcz_screen_w;
				frcz_sdlRect.h = frcz_screen_h;
			}

		}

		//SDL_AudioSpec
		SDL_AudioSpec wanted_spec;
		wanted_spec.freq = 44100;
		wanted_spec.format = AUDIO_S16SYS;
		wanted_spec.channels = 2;
		wanted_spec.silence = 0;
		wanted_spec.samples = 1024;
		wanted_spec.callback = frcz_fill_audio_back;

		if (SDL_OpenAudio(&wanted_spec, NULL) < 0) {
			printf("can't open audio.\n");
			return success;
		}

		//Play
		SDL_PauseAudio(0);
	}

	return success;
}

bool loadCamera()
{

	//printf("linesize0:%d\n", frcz_aVframeYUV->linesize[0]);
	//printf("linesize1:%d\n", frcz_aVframeYUV->linesize[1]);
	//printf("linesize2:%d\n", frcz_aVframeYUV->linesize[2]);

	//SDL_UpdateYUVTexture(frcz_Texture, &frcz_sdlRect,
	//	frcz_aVframeYUV->data[0], frcz_aVframeYUV->linesize[0],
	//	frcz_aVframeYUV->data[1], frcz_aVframeYUV->linesize[1],
	//	frcz_aVframeYUV->data[2], frcz_aVframeYUV->linesize[2]);
	
	//printf("data:%6s --- buffer:%6s \n", frcz_aVframeYUV->data[0], frcz_video_out_buffer);

	SDL_UpdateTexture(frcz_Texture, NULL, frcz_aVframeYUV->data[0], frcz_aVframeYUV->linesize[0]);

	return true;
}

void saveYUVFile() {

	switch (frcz_pix_format)
	{
	case AV_PIX_FMT_YUV420P:
		toSaveYUV420PFile();
		break;
	case AV_PIX_FMT_YUV422P:
		toSaveYUV422PFile();
		break;
	case AV_PIX_FMT_YUYV422:
		printf("YUYV422");
		break;
	default:
		printf("AV_PIX_FMT:%d", frcz_pix_format);
		break;
	}

}

void frcz_closeWindow()
{

	//Destroy window
	SDL_DestroyWindow(frcz_window);
	frcz_window = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

//�ͷ���Դ
void frcz_release() {

	sws_freeContext(frcz_swsContext);
	frcz_swsContext = NULL;

	//av_free(frcz_aVframe);
	av_free(frcz_aVframeYUV);

	avcodec_close(frcz_aVCodecContext_video);
	//avcodec_close(frcz_aVCodecContext_audio);

	avformat_close_input(&frcz_aVFormatContext_camera);
	//avformat_close_input(&frcz_aVFormatContext_audio);
}

//��ȡһ֡ �����
int read_frame_by_camera() {

	int ret = -1;

	//��packet�н������ԭʼ��Ƶ֡
	frcz_aVframe_video = av_frame_alloc();//����һ�����Ĭ��ֵ��AVFrame

	if (av_read_frame(frcz_aVFormatContext_camera, frcz_packet_video) >= 0) {
		//av_read_frame:  ����AVFormatContext ��ȡpacket��Ϣ
		if (frcz_packet_video->stream_index == frcz_video_index)//�Ա�packet->stream_index ��������
		{

			//���롣����Ϊpacket�����Ϊoriginal_video_frame
			if (avcodec_send_packet(frcz_aVCodecContext_video, frcz_packet_video) == 0)//�������(AVCodecContext)������Ҫ��������ݰ�(packet),0 ��ʾ����ɹ�
			{
				ret = avcodec_receive_frame(frcz_aVCodecContext_video, frcz_aVframe_video);//AVCodecContext* video_codec_ctx;
				//avcodec_receive_frame: �ӽ�������ȡ������һ֡,0���ǽ��ͳɹ�
				if (ret ==0)
				{
					//ת���������ݸ�ʽ
					sws_scale(frcz_swsContext,
						(const uint8_t* const*)frcz_aVframe_video->data,
						frcz_aVframe_video->linesize,
						0,
						frcz_aVCodecContext_video->height,
						frcz_aVframeYUV->data,
						frcz_aVframeYUV->linesize);

					loadCamera();
					//saveYUVFile();
				}
				else {
					printf("û��ȡ��!");
				}
				
			}
		}
	}

	av_packet_unref(frcz_packet_video);
	av_free(frcz_aVframe_video);
	return ret;
}

//��ȡһ֡ ����˷�
int read_frame_by_micphone() {

	int ret = -1;

	////��packet�н������ԭʼ��Ƶ֡
	//frcz_aVframe_audio = av_frame_alloc();//����һ�����Ĭ��ֵ��AVFrame

	//if (av_read_frame(frcz_aVFormatContext_audio, frcz_packet_audio) >= 0) {
	//	//av_read_frame:  ����AVFormatContext ��ȡpacket��Ϣ
	//	if (frcz_packet_audio->stream_index == frcz_audio_index)//�Ա�packet->stream_index ��������
	//	{
	//		//���롣����Ϊpacket�����Ϊoriginal_video_frame
	//		if (avcodec_send_packet(frcz_aVCodecContext_audio, frcz_packet_audio) == 0)//�������(AVCodecContext)������Ҫ��������ݰ�(packet),0 ��ʾ����ɹ�
	//		{
	//			ret = avcodec_receive_frame(frcz_aVCodecContext_audio, frcz_aVframe_audio);//AVCodecContext* video_codec_ctx;
	//			//avcodec_receive_frame: �ӽ�������ȡ������һ֡,0���ǽ��ͳɹ�
	//			if (ret == 0)
	//			{
	//				swr_convert(frcz_swrContext, &frcz_audio_out_buffer, frcz_audio_out_buffer_size, (const uint8_t**)frcz_aVframe_audio->data, frcz_aVframe_audio->nb_samples);
	//			
	//				frcz_audio_len = frcz_audio_out_buffer_size;
	//				frcz_audio_pos = (Uint8*)frcz_audio_out_buffer;

	//				while (frcz_audio_len > 0)//Wait until finish
	//					SDL_Delay(1);
	//			}
	//			else {
	//				printf("û��ȡ��!");
	//			}

	//		}
	//	}
	//}

	//av_packet_unref(frcz_packet_audio);
	//av_free(frcz_aVframe_audio);
	return ret;
}

void toSaveYUV420PFile() {

	int i = 0;

	uint8_t* tempptr = frcz_aVframeYUV->data[0];

	for (i = 0; i < frcz_screen_h; i++) {
		fwrite(tempptr, 1, frcz_screen_w, frcz_YUV_FILE);     //Y 
		tempptr += frcz_aVframeYUV->linesize[0];
	}
	tempptr = frcz_aVframeYUV->data[1];
	for (i = 0; i < frcz_screen_h/2; i++) {
		fwrite(tempptr, 1, frcz_screen_w / 2, frcz_YUV_FILE);   //U
		tempptr += frcz_aVframeYUV->linesize[1];
	}
	tempptr = frcz_aVframeYUV->data[2];
	for (i = 0; i < frcz_screen_h/2; i++) {
		fwrite(tempptr, 1, frcz_screen_w / 2, frcz_YUV_FILE);   //V
		tempptr += frcz_aVframeYUV->linesize[2];
	}
}

void toSaveYUV422PFile() {

	//printf("err:%d \n",err);

	int i = 0;

	uint8_t* tempptr = frcz_aVframeYUV->data[0];

	//printf("linesize0:%d ", frcz_aVframeYUV->linesize[0]);
	//printf("linesize1:%d ", frcz_aVframeYUV->linesize[1]);
	//printf("linesize2:%d\n", frcz_aVframeYUV->linesize[2]);

	for (i = 0; i < frcz_screen_h; i++) {
		fwrite(tempptr, 1, frcz_screen_w, frcz_YUV_FILE);     //Y 
		printf("Y:%d,", frcz_screen_w);
		tempptr += frcz_aVframeYUV->linesize[0];
	}
	printf("\n");
	tempptr = frcz_aVframeYUV->data[1];
	for (i = 0; i < frcz_screen_h; i++) {
		fwrite(tempptr, 1, frcz_screen_w / 2, frcz_YUV_FILE);   //U
		printf("U:%d,", frcz_screen_w/2);
		tempptr += frcz_aVframeYUV->linesize[1];
	}
	printf("\n");
	tempptr = frcz_aVframeYUV->data[2];
	for (i = 0; i < frcz_screen_h; i++) {
		fwrite(tempptr, 1, frcz_screen_w / 2, frcz_YUV_FILE);   //V
		printf("V:%d,", frcz_screen_w / 2);
		tempptr += frcz_aVframeYUV->linesize[2];
	}
	printf("\n");
	printf("�������\n");
}

//����SDL����
int frcz_open_window_fun() {

//Start up SDL and create window
	if (!frcz_initSDLWindow())
	{
		printf("Failed to initialize!\n");
	}
	else
	{

		//�������߳�
		SDL_CreateThread(frcz_vidio_thread, NULL, NULL);
		//SDL_CreateThread(frcz_audio_thread, NULL, NULL);

		//While application is running
		while (!frcz_isQuit)
		{
			//Handle events on queue

			//SDL_WaitEvent(&frcz_event);
			//User requests quit
			if (frcz_event.type == SDL_QUIT)
			{
				frcz_isQuit = true;
			}
			else if (frcz_event.type == SFM_REFRESH_EVENT)
			{

				//Clear screen
				SDL_RenderClear(frcz_Renderer);

				//Render texture to screen
				SDL_RenderCopy(frcz_Renderer, frcz_Texture, NULL, NULL);

				//Update screen
				SDL_RenderPresent(frcz_Renderer);

			}
		}
	}
	frcz_closeWindow();
	return 0;
}

//��Ƶ����
int frcz_vidio_thread(void* opaque) {
	while (frcz_event.type != SDL_QUIT) {
		read_frame_by_camera();
		frcz_event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&frcz_event);
		SDL_Delay(40);
	}
	return 0;
}

//��Ƶ����
int frcz_audio_thread(void* opaque) {

	while (frcz_event.type != SDL_QUIT) {
		read_frame_by_micphone();
	}

	return 0;
}

void  frcz_fill_audio_back(void* udata, Uint8* stream, int len) {
	//SDL 2.0
	

	//SDL_memset(stream, 0, len);
	//if (frcz_audio_len == 0)		/*  Only  play  if  we  have  data  left  */
	//	return;
	//len = (len > frcz_audio_len ? frcz_audio_len : len);	/*  Mix  as  much  data  as  possible  */

	//

	//SDL_MixAudio(stream, frcz_audio_pos, len, SDL_MIX_MAXVOLUME);
	//frcz_audio_pos += len;
	//frcz_audio_len -= len;

}
