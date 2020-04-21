#include <stdio.h>
#include "FFmpegReadCameraZ.h"


extern "C"
{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libswscale/swscale.h"
	#include "libavdevice/avdevice.h"
	#include "SDL.h"
};


void initReadCameraZ() {

	printf("start!\n");

	AVFormatContext* aVFormatContext = avformat_alloc_context();
	int				i, videoindex;
	AVCodecContext* aVCodecContext = avcodec_alloc_context3(NULL);
	AVCodec* aVCodec;

	avformat_network_init();

	//Register Device
	avdevice_register_all();

	//Show Dshow Device
	show_dshow_device();
	//Show Device Options
	//show_dshow_device_option();
	//Show VFW Options
	//show_vfw_device();

	AVInputFormat* ifmt = av_find_input_format("dshow");
	//Set own video device's name              Surface Camera Front
	if (avformat_open_input(&aVFormatContext, "video=USB2.0 Camera", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return;
	}

	if (avformat_find_stream_info(aVFormatContext, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return;
	}

	videoindex = -1;
	for (i = 0; i < aVFormatContext->nb_streams; i++)
		if (aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		return;
	}

	avcodec_parameters_to_context(aVCodecContext, aVFormatContext->streams[videoindex]->codecpar);

	if (aVCodecContext == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return;
	}

	aVCodec = avcodec_find_decoder(aVCodecContext->codec_id);
	if (aVCodec == NULL)
	{
		printf("Codec not found.\n");
		return;
	}
	if (avcodec_open2(aVCodecContext, aVCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return;
	}

	AVFrame* pFrame, * pFrameYUV;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	//unsigned char *out_buffer=(unsigned char *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	//avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	
	//SDL配置开始----------------------------

	int screen_w = 0, screen_h = 0;
	//SDL_Surface* screen;
	screen_w = aVCodecContext->width;
	screen_h = aVCodecContext->height;

	//将要渲染的窗口
	SDL_Window* window = NULL;

	//窗口含有的surface
	SDL_Surface* screenSurface = NULL;

	//初始化SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return;
	}

	//创建 window
	window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return;
	}

	//获取 window surface
	screenSurface = SDL_GetWindowSurface(window);

	//用白色填充surface
	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));

	//更新surface
	SDL_UpdateWindowSurface(window);

	//延迟两秒
	SDL_Delay(2000);

	//销毁 window
	SDL_DestroyWindow(window);

	//退出 SDL subsystems
	SDL_Quit();

	printf("end!");
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
	AVInputFormat* iformat = av_find_input_format("vfwcap");
	printf("========VFW Device Info======\n");
	avformat_open_input(&pFormatCtx, "list", iformat, NULL);
	printf("=============================\n");
}