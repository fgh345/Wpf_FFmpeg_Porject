#include <stdio.h>
#include "FFmpegReadCameraZ.h"



extern "C"
{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libswscale/swscale.h"
	#include "libavdevice/avdevice.h"
	#include "libavutil/imgutils.h"
	#include "SDL.h"
};

//命名前缀 frcz_

//我们要渲染到的窗口
SDL_Window* frcz_window = NULL;

int frcz_screen_w = 0, frcz_screen_h = 0;

//The surface contained by the window
//SDL_Surface* frcz_screenSurface = NULL;

//The window renderer
SDL_Renderer* frcz_Renderer = NULL;

//Current displayed texture
SDL_Texture* frcz_Texture = NULL;

//循环标记
bool frcz_isQuit = false;

//事件句柄
SDL_Event frcz_event;

AVFrame* frcz_aVframe = NULL;
AVFrame* frcz_aVframeYUV = av_frame_alloc();
AVCodec* frcz_aVCodec = NULL;
AVCodecContext* frcz_aVCodecContext = NULL;
AVFormatContext* frcz_aVFormatContext = NULL;
AVDictionary* frcz_options = NULL;
AVPacket* frcz_packet = NULL;
struct SwsContext* frcz_swsContext = NULL;

//流队列中，视频流所在的位置
int frcz_video_index = -1;

SDL_Rect frcz_sdlRect;

int frcz_video_out_buffer_size = 0; //视频输出buffer 大小
uint8_t* frcz_video_out_buffer;//视频输出buffer

//char frcz_filepath[] = "C:\\Users\\Youzh\\Videos\\jxyy.mp4"; //读取本地文件地址
FILE* frcz_YUV_FILE;

void initReadCameraZ() {

	printf("start!\n");

	if (initFFmpeg()!=0)
	{
		printf("initFFmpeg error!");
		return;
	}

	
	//SDL配置开始----------------------------

	//Start up SDL and create window
	if (!initSDLWindow())
	{
		printf("Failed to initialize!\n");
	}
	else
	{

		//While application is running
		while (!frcz_isQuit)
		{
			//Handle events on queue
			while (SDL_PollEvent(&frcz_event) != 0)
			{
				//User requests quit
				if (frcz_event.type == SDL_QUIT)
				{
					frcz_isQuit = true;
				}
			}

			read_frame_by_dshow();

			//Clear screen
			SDL_RenderClear(frcz_Renderer);

			//Render texture to screen
			SDL_RenderCopy(frcz_Renderer, frcz_Texture, NULL, NULL);

			//Update screen
			SDL_RenderPresent(frcz_Renderer);

			//Update the surface
			//SDL_UpdateWindowSurface(frcz_window);
		}
	}
	closeWindow();

	printf("end!");
}

int initFFmpeg() {

	avformat_network_init();

	avdevice_register_all();

	frcz_aVFormatContext = avformat_alloc_context();
	frcz_aVCodecContext = avcodec_alloc_context3(NULL);

	//Show Dshow Device
	//show_dshow_device();
	//Show Device Options
	//show_dshow_device_option();
	//Show VFW Options
	//show_vfw_device();

	AVInputFormat* frcz_aVInputFormat = av_find_input_format("dshow");

	av_dict_set_int(&frcz_options, "rtbufsize", 3041280 * 10, 0);//默认大小3041280

	//Set own video device's name              Surface Camera Front   USB2.0 Camera
	if (avformat_open_input(&frcz_aVFormatContext, "video=USB2.0 Camera", frcz_aVInputFormat, &frcz_options) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}

	//if (avformat_open_input(&frcz_aVFormatContext, frcz_filepath, NULL, NULL) != 0) {
	//	printf("Couldn't open input stream.\n");
	//	return -1;
	//}

	if (avformat_find_stream_info(frcz_aVFormatContext, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}


	for (int i = 0; i < frcz_aVFormatContext->nb_streams; i++)
		if (frcz_aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			frcz_video_index = i;
			break;
		}

	if (frcz_video_index == -1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}

	avcodec_parameters_to_context(frcz_aVCodecContext, frcz_aVFormatContext->streams[frcz_video_index]->codecpar);

	if (frcz_aVCodecContext == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}

	frcz_aVCodec = avcodec_find_decoder(frcz_aVCodecContext->codec_id);
	if (frcz_aVCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(frcz_aVCodecContext, frcz_aVCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	
	printf("pix_format %d\n", frcz_aVCodecContext->pix_fmt);

	frcz_screen_w = frcz_aVCodecContext->width;
	frcz_screen_h = frcz_aVCodecContext->height;


	//计算输出缓存
	frcz_video_out_buffer_size = av_image_get_buffer_size(
		AV_PIX_FMT_YUV420P,
		frcz_aVCodecContext->width,
		frcz_aVCodecContext->height,
		1);

	//输出缓存
	frcz_video_out_buffer = new uint8_t[frcz_video_out_buffer_size];

	//准备一些参数，在视频格式转换后，参数将被设置值  这里从源码看 是将 video_out_buffer 填入 video_out_frame->data 里面 所以后面操作 video_out_frame->data 会影响video_out_buffer 
	av_image_fill_arrays(
		frcz_aVframeYUV->data,
		frcz_aVframeYUV->linesize,
		frcz_video_out_buffer,
		AV_PIX_FMT_YUV420P,
		frcz_aVCodecContext->width,
		frcz_aVCodecContext->height,
		1);

	frcz_swsContext = sws_getContext(
		frcz_aVCodecContext->width,
		frcz_aVCodecContext->height,
		frcz_aVCodecContext->pix_fmt,
		frcz_aVCodecContext->width,
		frcz_aVCodecContext->height,
		AV_PIX_FMT_YUV420P,//转码为RGB像素
		SWS_BICUBIC,
		NULL, NULL, NULL);

	frcz_packet = av_packet_alloc();
	av_init_packet(frcz_packet);

	if (!frcz_packet)
		return -1;

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
	AVInputFormat* iformat = av_find_input_format("vfwcap");
	printf("========VFW Device Info======\n");
	avformat_open_input(&pFormatCtx, "list", iformat, NULL);
	printf("=============================\n");
}

//初始化SDL窗口
bool initSDLWindow()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
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
	}

	return success;
}


bool loadCamera()
{

	sws_scale(frcz_swsContext,
		(const uint8_t* const*)frcz_aVframe->data,
		frcz_aVframe->linesize,
		0,
		frcz_aVCodecContext->height,
		frcz_aVframeYUV->data,
		frcz_aVframeYUV->linesize);
	
	
	//SDL_UpdateYUVTexture(frcz_Texture, &frcz_sdlRect,
	//	frcz_aVframeYUV->data[0], frcz_aVframeYUV->linesize[0],
	//	frcz_aVframeYUV->data[1], frcz_aVframeYUV->linesize[1],
	//	frcz_aVframeYUV->data[2], frcz_aVframeYUV->linesize[2]);
	
	//printf("data:%6s --- buffer:%6s \n", frcz_aVframeYUV->data[0], frcz_video_out_buffer);

	SDL_UpdateTexture(frcz_Texture, &frcz_sdlRect, frcz_aVframeYUV->data[0], frcz_aVframeYUV->linesize[0]);

	return true;
}

void saveYUVFile() {

	int err = fopen_s(&frcz_YUV_FILE, "CameraFrame.yuv", "wb");//打开文件流

	int i = 0;

	uint8_t* tempptr = frcz_aVframe->data[0];

	//printf("height:%d\n", original_video_frame->height);
	//printf("width:%d\n", original_video_frame->width);
	//printf("linesize0:%d\n", original_video_frame->linesize[0]);
	//printf("linesize1:%d\n", original_video_frame->linesize[1]);
	//printf("linesize2:%d\n", original_video_frame->linesize[2]);

	for (i = 0; i < frcz_aVframe->height; i++) {
		fwrite(tempptr, 1, frcz_aVframe->width, frcz_YUV_FILE);     //Y 
		tempptr += frcz_aVframe->linesize[0];
	}
	tempptr = frcz_aVframe->data[1];
	for (i = 0; i < frcz_aVframe->height / 2; i++) {
		fwrite(tempptr, 1, frcz_aVframe->width / 2, frcz_YUV_FILE);   //U
		tempptr += frcz_aVframe->linesize[1];
	}
	tempptr = frcz_aVframe->data[2];
	for (i = 0; i < frcz_aVframe->height / 2; i++) {
		fwrite(tempptr, 1, frcz_aVframe->width / 2, frcz_YUV_FILE);   //V
		tempptr += frcz_aVframe->linesize[2];
	}
}

void closeWindow()
{
	//Deallocate surface
	//SDL_FreeSurface(gHelloWorld);
	//gHelloWorld = NULL;

	//Destroy window
	SDL_DestroyWindow(frcz_window);
	frcz_window = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

//读取一帧
int read_frame_by_dshow() {

	int ret = -1;

	//从packet中解出来的原始视频帧
	frcz_aVframe = av_frame_alloc();//返回一个填充默认值的AVFrame

	if (av_read_frame(frcz_aVFormatContext, frcz_packet) >= 0) {
		//av_read_frame:  根据AVFormatContext 读取packet信息  

		if (frcz_packet->stream_index == frcz_video_index)//对比packet->stream_index 的流类型
		{

			//解码。输入为packet，输出为original_video_frame
			if (avcodec_send_packet(frcz_aVCodecContext, frcz_packet) == 0)//向解码器(AVCodecContext)发送需要解码的数据包(packet),0 表示解码成功
			{
				ret = avcodec_receive_frame(frcz_aVCodecContext, frcz_aVframe);//AVCodecContext* video_codec_ctx;
				//avcodec_receive_frame: 从解码器获取解码后的一帧,0表是解释成功
				if (ret ==0)
				{

					//loadCamera();
					saveYUVFile();
				}
				else {
					printf("没读取到!");
				}
				
			}
		}
	}

	av_packet_unref(frcz_packet);
	av_free(frcz_aVframe);
	return ret;
}
