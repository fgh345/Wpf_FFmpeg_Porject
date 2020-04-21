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

//命名前缀 frcz_

//我们要渲染到的窗口
SDL_Window* frcz_window = NULL;

int frcz_screen_w = 0, frcz_screen_h = 0;

//The surface contained by the window
SDL_Surface* frcz_screenSurface = NULL;

//The window renderer
SDL_Renderer* frcz_Renderer = NULL;

//Current displayed texture
SDL_Texture* frcz_Texture = NULL;

//循环标记
bool frcz_isQuit = false;

//事件句柄
SDL_Event frcz_event;

AVFrame* frcz_aVframe = NULL;
AVCodec* frcz_aVCodec = NULL;
AVCodecContext* frcz_aVCodecContext = NULL;
AVFormatContext* frcz_aVFormatContext = NULL;
AVDictionary* frcz_options = NULL;
AVPacket* frcz_packet = NULL;

//流队列中，视频流所在的位置
int frcz_video_index = -1;

void initReadCameraZ() {

	printf("start!\n");

	frcz_aVFormatContext = avformat_alloc_context();
	frcz_aVCodecContext = avcodec_alloc_context3(NULL);

	avformat_network_init();

	//Register Device
	avdevice_register_all();

	//Show Dshow Device
	//show_dshow_device();
	//Show Device Options
	//show_dshow_device_option();
	//Show VFW Options
	//show_vfw_device();

	if (initFFmpeg()!=0)
	{
		printf("initFFmpeg error!");
		return;
	}

	
	//SDL配置开始----------------------------


	//SDL_Surface* screen;
	frcz_screen_w = frcz_aVCodecContext->width;
	frcz_screen_h = frcz_aVCodecContext->height;


	//Start up SDL and create window
	if (!initSDLWindow())
	{
		printf("Failed to initialize!\n");
	}
	else
	{

		//Create renderer for window
		//frcz_Renderer = SDL_CreateRenderer(frcz_window, -1, SDL_RENDERER_ACCELERATED);
		//if (frcz_Renderer == NULL)
		//{
		//	printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		//	return;
		//}
		//else
		//{
		//	//Initialize renderer color
		//	SDL_SetRenderDrawColor(frcz_Renderer, 0xFF, 0xFF, 0xFF, 0xFF);


		//}


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


			if (read_frame_by_dshow() == 0) {
				loadCamera();
			}

			//Clear screen
			//SDL_RenderClear(frcz_Renderer);

			////Render texture to screen
			//SDL_RenderCopy(frcz_Renderer, frcz_Texture, NULL, NULL);

			////Update screen
			//SDL_RenderPresent(frcz_Renderer);

			//Update the surface
			SDL_UpdateWindowSurface(frcz_window);
		}
	}
	closeWindow();

	printf("end!");
}

int initFFmpeg() {

	AVInputFormat* frcz_aVInputFormat = av_find_input_format("dshow");

	av_dict_set_int(&frcz_options, "rtbufsize", 3041280 * 100, 0);//默认大小3041280

	//Set own video device's name              Surface Camera Front
	if (avformat_open_input(&frcz_aVFormatContext, "video=USB2.0 Camera", frcz_aVInputFormat, &frcz_options) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if (avformat_find_stream_info(frcz_aVFormatContext, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	int i = 0;
	for (i = 0; i < frcz_aVFormatContext->nb_streams; i++)
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

	frcz_aVframe = av_frame_alloc();

	frcz_packet = av_packet_alloc();

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
			//Get window surface
			frcz_screenSurface = SDL_GetWindowSurface(frcz_window);
			//Fill the surface white
			SDL_FillRect(frcz_screenSurface, NULL, SDL_MapRGB(frcz_screenSurface->format, 0xFF, 0xFF, 0xFF));
		}
	}

	return success;
}


bool loadCamera()
{
	printf("loadCamera\n");


	

	return true;
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

void loadTexture()
{

	if (frcz_screenSurface == NULL)
	{
		printf("Unable to load image SDL_image Error\n");
	}
	else
	{
		//Create texture from surface pixels
		frcz_Texture = SDL_CreateTextureFromSurface(frcz_Renderer, frcz_screenSurface);
		if (frcz_Texture == NULL)
		{
			printf("Unable to create texture from! SDL Error: %s\n", SDL_GetError());
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(frcz_screenSurface);
	}

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

				if (ret == 0)//0表是解析成功
				{

					//SDL_CreateTexture();
					//SDL_UpdateYUVTexture();

				}
			}
		}
	}

	av_packet_unref(frcz_packet);
	av_free(frcz_aVframe);

	return ret;
}
