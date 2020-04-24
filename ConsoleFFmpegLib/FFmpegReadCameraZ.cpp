#include <stdio.h>
#include "FFmpegReadCameraZ.h"

//��ȡdshow��� ���ѻ�����SDL2չʾ����,�������洢����

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

//����ǰ׺ frcz_

//����Ҫ��Ⱦ���Ĵ���
SDL_Window* frcz_window = NULL;

int frcz_screen_w = 0, frcz_screen_h = 0;

//The window renderer
SDL_Renderer* frcz_Renderer = NULL;

//Current displayed texture
SDL_Texture* frcz_Texture = NULL;

//ѭ�����
bool frcz_isQuit = false;

//�¼����
SDL_Event frcz_event;

AVFrame* frcz_aVframe = NULL;
AVFrame* frcz_aVframeYUV = av_frame_alloc();
AVCodec* frcz_aVCodec = NULL;
AVCodecContext* frcz_aVCodecContext = NULL;
AVFormatContext* frcz_aVFormatContext = NULL;
AVPacket* frcz_packet = NULL;
struct SwsContext* frcz_swsContext = NULL;
AVPixelFormat frcz_pix_format = AV_PIX_FMT_YUV420P;

//�������У���Ƶ�����ڵ�λ��
int frcz_video_index = -1;

SDL_Rect frcz_sdlRect;

int frcz_video_out_buffer_size = 0; //��Ƶ���buffer ��С
uint8_t* frcz_video_out_buffer;//��Ƶ���buffer

char frcz_filepath[] = "C:\\Users\\Youzh\\Videos\\jxyy.mp4"; //��ȡ�����ļ���ַ
FILE* frcz_YUV_FILE;//�洢ΪYUV��ʽ�ļ�

char frcz_trmp_url[] = "rtmp://192.168.30.20/live/livestream"; //������ַ
AVFormatContext* frcz_aVFormatContext_rtmp = NULL;
int frcz_frame_index = 0;//���͵�֡����


void startReadCameraZ() {

	printf("start!\n");

	if (frcz_initFFmpeg()!=0)
	{
		printf("initFFmpeg error!");
		return;
	}

	int err = fopen_s(&frcz_YUV_FILE, "CameraFrame.yuv", "wb+");//���ļ���

	//frcz_open_window_fun();
	
	frcz_open_rtmp_fun();

	frcz_release();
	printf("end!");
}

int frcz_initFFmpeg() {

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

	AVDictionary* frcz_options = NULL;
	av_dict_set_int(&frcz_options, "rtbufsize", 3041280 * 20, 0);//Ĭ�ϴ�С3041280  ���ú�����ӳ� ��Ϊ�����ٶȸ�����

	//Set own video device's name              Surface Camera Front  USB2.0 Camera
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

	frcz_screen_w = frcz_aVCodecContext->width;
	frcz_screen_h = frcz_aVCodecContext->height;

#pragma region ��ʽת��

	//�����������
	frcz_video_out_buffer_size = av_image_get_buffer_size(
		frcz_pix_format,
		frcz_aVCodecContext->width,
		frcz_aVCodecContext->height,
		1);

	//�������
	frcz_video_out_buffer = new uint8_t[frcz_video_out_buffer_size];

	//׼��һЩ����������Ƶ��ʽת���󣬲�����������ֵ  �����Դ�뿴 �ǽ� video_out_buffer ���� video_out_frame->data ���� ���Ժ������ video_out_frame->data ��Ӱ��video_out_buffer 
	av_image_fill_arrays(
		frcz_aVframeYUV->data,
		frcz_aVframeYUV->linesize,
		frcz_video_out_buffer,
		frcz_pix_format,
		frcz_aVCodecContext->width,
		frcz_aVCodecContext->height,
		1);

	frcz_swsContext = sws_getContext(
		frcz_aVCodecContext->width,
		frcz_aVCodecContext->height,
		frcz_aVCodecContext->pix_fmt,
		frcz_aVCodecContext->width,
		frcz_aVCodecContext->height,
		frcz_pix_format,
		SWS_BICUBIC,
		NULL, NULL, NULL);

#pragma endregion

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

	//printf("linesize0:%d\n", frcz_aVframeYUV->linesize[0]);
	//printf("linesize1:%d\n", frcz_aVframeYUV->linesize[1]);
	//printf("linesize2:%d\n", frcz_aVframeYUV->linesize[2]);

	SDL_UpdateYUVTexture(frcz_Texture, &frcz_sdlRect,
		frcz_aVframeYUV->data[0], frcz_aVframeYUV->linesize[0],
		frcz_aVframeYUV->data[1], frcz_aVframeYUV->linesize[1],
		frcz_aVframeYUV->data[2], frcz_aVframeYUV->linesize[2]);
	
	//printf("data:%6s --- buffer:%6s \n", frcz_aVframeYUV->data[0], frcz_video_out_buffer);

	//SDL_UpdateTexture(frcz_Texture, &frcz_sdlRect, frcz_aVframeYUV->data[0], frcz_aVframeYUV->linesize[0]);

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

	av_free(frcz_aVframeYUV);

	avcodec_close(frcz_aVCodecContext);

	avformat_close_input(&frcz_aVFormatContext);
}

//��ȡһ֡
int read_frame_by_dshow() {

	int ret = -1;

	//��packet�н������ԭʼ��Ƶ֡
	frcz_aVframe = av_frame_alloc();//����һ�����Ĭ��ֵ��AVFrame

	if (av_read_frame(frcz_aVFormatContext, frcz_packet) >= 0) {
		//av_read_frame:  ����AVFormatContext ��ȡpacket��Ϣ  

		if (frcz_packet->stream_index == frcz_video_index)//�Ա�packet->stream_index ��������
		{

			//���롣����Ϊpacket�����Ϊoriginal_video_frame
			if (avcodec_send_packet(frcz_aVCodecContext, frcz_packet) == 0)//�������(AVCodecContext)������Ҫ��������ݰ�(packet),0 ��ʾ����ɹ�
			{
				ret = avcodec_receive_frame(frcz_aVCodecContext, frcz_aVframe);//AVCodecContext* video_codec_ctx;
				//avcodec_receive_frame: �ӽ�������ȡ������һ֡,0���ǽ��ͳɹ�
				if (ret ==0)
				{
					//ת���������ݸ�ʽ
					sws_scale(frcz_swsContext,
						(const uint8_t* const*)frcz_aVframe->data,
						frcz_aVframe->linesize,
						0,
						frcz_aVCodecContext->height,
						frcz_aVframeYUV->data,
						frcz_aVframeYUV->linesize);

					loadCamera();
					saveYUVFile();
				}
				else {
					printf("û��ȡ��!");
				}
				
			}
		}
	}

	av_packet_unref(frcz_packet);
	av_free(frcz_aVframe);
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

//��������
int frcz_open_rtmp_fun() {

	avformat_alloc_output_context2(&frcz_aVFormatContext_rtmp, NULL, "flv", frcz_trmp_url); //RTMP

	if (!frcz_aVFormatContext_rtmp) {
		printf("Could not create output context\n");
		return -1;
	}

	for (int i = 0; i < frcz_aVFormatContext->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream* in_stream = frcz_aVFormatContext->streams[i];
		AVStream* out_stream = avformat_new_stream(frcz_aVFormatContext_rtmp, frcz_aVCodec);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			return -1;
		}
		//Copy the settings of AVCodecContext
		AVCodecContext* dest = avcodec_alloc_context3(NULL);
		//��in_stream��ȡ���ݵ�dest����
		int ret = avcodec_parameters_to_context(dest, in_stream->codecpar);
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			return -1;
		}
		dest->codec_tag = 0;
		if (frcz_aVFormatContext_rtmp->oformat->flags & AVFMT_GLOBALHEADER)
			dest->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		//��destӦ�õ�out_stream����
		ret = avcodec_parameters_from_context(out_stream->codecpar, dest);
		if (ret < 0) {
			printf("Failed to copy codec context to out_stream codecpar context\n");
			return -1;
		}

	}

	//Dump Format------------------
	av_dump_format(frcz_aVFormatContext_rtmp, 0, frcz_trmp_url, 1);

	//Open output URL
	if (!(frcz_aVFormatContext_rtmp->oformat->flags & AVFMT_NOFILE)) {
		int ret = avio_open(&frcz_aVFormatContext_rtmp->pb, frcz_trmp_url, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Could not open output URL '%s'", frcz_trmp_url);
			return -1;
		}
	}

	//Write file header
	int ret = avformat_write_header(frcz_aVFormatContext_rtmp, NULL);
	if (ret < 0) {
		printf("Error occurred when opening output URL\n");
		return -1;
	}

	int64_t start_time = av_gettime();

	while (1) {
		AVStream* in_stream, * out_stream;
		//Get an AVPacket
		ret = av_read_frame(frcz_aVFormatContext, frcz_packet);
		if (ret < 0)
			break;
		//FIX��No PTS (Example: Raw H.264)
		//Simple Write PTS
		if (frcz_packet->pts == AV_NOPTS_VALUE) {
			//Write PTS
			AVRational time_base1 = frcz_aVFormatContext->streams[frcz_video_index]->time_base;
			//Duration between 2 frames (us)
			int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(frcz_aVFormatContext->streams[frcz_video_index]->r_frame_rate);
			//Parameters
			frcz_packet->pts = (double)(frcz_frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
			frcz_packet->dts = frcz_packet->pts;
			frcz_packet->duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
		}
		//Important:Delay
		if (frcz_packet->stream_index == frcz_video_index) {
			AVRational time_base = frcz_aVFormatContext->streams[frcz_video_index]->time_base;
			AVRational time_base_q = { 1,AV_TIME_BASE };
			int64_t pts_time = av_rescale_q(frcz_packet->dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time)
				av_usleep(pts_time - now_time);

		}

		in_stream = frcz_aVFormatContext->streams[frcz_packet->stream_index];
		out_stream = frcz_aVFormatContext_rtmp->streams[frcz_packet->stream_index];

		/* copy packet */
		//Convert PTS/DTS
		frcz_packet->pts = av_rescale_q_rnd(frcz_packet->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		frcz_packet->dts = av_rescale_q_rnd(frcz_packet->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		frcz_packet->duration = av_rescale_q(frcz_packet->duration, in_stream->time_base, out_stream->time_base);
		frcz_packet->pos = -1;
		//Print to Screen
		if (frcz_packet->stream_index == frcz_video_index) {
			printf("Send %8d video frames to output URL\n", frcz_frame_index);
			frcz_frame_index++;
		}
		//ret = av_write_frame(ofmt_ctx, &pkt);
		ret = av_interleaved_write_frame(frcz_aVFormatContext_rtmp, frcz_packet);

		if (ret < 0) {
			printf("Error muxing packet\n");
			break;
		}

		av_packet_unref(frcz_packet);
	}

	//Write file trailer
	av_write_trailer(frcz_aVFormatContext_rtmp);

	return 0;
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
	frcz_closeWindow();
	return 0;
}
