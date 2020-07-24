// ConsoleFFmpegLib.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "ConsoleFFmpegLib.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"  
#include "libswscale/swscale.h"  
#include "libavutil/imgutils.h"  
#include "libavdevice/avdevice.h"
}

AVFormatContext* aVFormatContext;

//AVDictionary* options = NULL;

//流队列中，视频流所在的位置
int video_index = -1;
int audio_index = -1;

//视频解码上下文
AVCodecContext* video_AVCodecContext;
AVCodecContext* audio_AVCodecContext;

//输出缓存大小
int video_out_buffer_size;
int audio_out_buffer_size;

//输出缓存
uint8_t* video_out_buffer;
uint8_t* audio_out_buffer;

//转码后输出的视频帧（如yuv转rgb24）
AVFrame* video_out_AVFrame = av_frame_alloc();

//格式转换上下文
struct SwsContext* video_SwsContext;
struct SwrContext* audio_SwrContext;

//解码前数据包
AVPacket* aVPacket = (AVPacket*)malloc(sizeof(AVPacket));

//音频声道数量
int nb_channels;
//AVSampleFormat aVSampleFormat = ;//输出的采样格式 16bit PCM
int sample_rate;//采样率
uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;//输出的声道布局：立体声

//FILE* out_FILE;


int cffStart()
{
    std::cout << "Start!\n";

	//char url[] = "rtmp://live.hkstv.hk.lxdns.com/live/hks";

	//char url[] = "video=Intel(R) AVStream Camera";
	char url[] = "..\\..\\result_file.flv";
	int init_ret = init_ffmpeg(url);
	if (init_ret >= 0)
	{
		while (1)//读取一帧，直到读取到数据
		{
			int ret = read_frame();
			//if (ret==0)
			//{
			//	std::cout << get_video_frame();
			//	//break;
			//}
			//else
			//{
			//	printf("未读取到!");
			//}
			
		}


		//printf("高度:%d  宽度:%d  缓存大小:%d", get_video_height(), get_video_width(), get_video_buffer_size());
		//std::cout << "\n=================================\n";
		//std::cout << get_video_frame();//打印出来，虽然打印出来的东西看不懂，但是证明已经获取到一帧的数据了
		std::cout << "\nEND";
	}
	else
	{
		std::cout << "初始化失败!";
	}
	return 0;

}

//初始化FFmpeg 
//@param *url 媒体地址（本地/网络地址）
int init_ffmpeg(char* url) {
	avformat_network_init();//支持网络流
	avdevice_register_all();//在使用libavdevice之前，必须先运行avdevice_register_all()对设备进行注册
	aVFormatContext = avformat_alloc_context();//AVFormatContext

	//char filepath_out[] = "oneframe.yuv";//文件输出地址
	//int err = fopen_s(&out_FILE, filepath_out, "wb");//打开文件流


	//打开文件
	int res = avformat_open_input(&aVFormatContext, url, NULL, NULL);//打开一个输入流,读取资源的header 0表示成功
	if (res != 0) {
		return -1;
	}

	//查找流信息
	if (avformat_find_stream_info(aVFormatContext, NULL) < 0)//读取媒体文件的流信息
	{
		return -1;
	}

	//找到流队列中，视频流所在位置
	for (unsigned int i = 0; i < aVFormatContext->nb_streams; i++) {
		if (aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_index = i;
		}
		if (aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_index = i;
		}
	}

	//音视频流都没有找到
	if (video_index == -1 && audio_index == -1)
	{
		return -1;
	}

	//查找解码器
	AVCodec* video_codec = avcodec_find_decoder(aVFormatContext->streams[video_index]->codecpar->codec_id);//codecpar 编码标准 查找具有匹配ID的解码器。
	video_AVCodecContext = avcodec_alloc_context3(video_codec);//初始化AVCodecContext 其值全是默认
	if (video_AVCodecContext == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(video_AVCodecContext, aVFormatContext->streams[video_index]->codecpar);//用提供的参数填充AVCodecContext


		//计算输出缓存
	video_out_buffer_size = av_image_get_buffer_size(
		AV_PIX_FMT_RGB24,
		video_AVCodecContext->width,
		video_AVCodecContext->height,
		1);

	//输出缓存
	video_out_buffer = new uint8_t[video_out_buffer_size];

	//准备一些参数，在视频格式转换后，参数将被设置值  这里从源码看 是将 video_out_buffer 填入 video_out_frame->data 里面
	//所以后面操作 video_out_frame->data 会影响video_out_buffer 
	av_image_fill_arrays(
		video_out_AVFrame->data,//转换后的数据
		video_out_AVFrame->linesize,
		video_out_buffer, //视频buffer
		AV_PIX_FMT_RGB24,//像素格式
		video_AVCodecContext->width,
		video_AVCodecContext->height,
		1);


	video_SwsContext = sws_getContext(//图片格式转换上下文
		video_AVCodecContext->width,
		video_AVCodecContext->height,
		video_AVCodecContext->pix_fmt,
		video_AVCodecContext->width,
		video_AVCodecContext->height,
		AV_PIX_FMT_RGB24,//转码为RGB像素
		SWS_BICUBIC,
		NULL, NULL, NULL);


	//下面是音频的准备工作  跟视频类似
	AVCodecParameters* aa = aVFormatContext->streams[audio_index]->codecpar;
	AVCodec* audio_codec = avcodec_find_decoder(aVFormatContext->streams[audio_index]->codecpar->codec_id);
	audio_AVCodecContext = avcodec_alloc_context3(audio_codec);
	if (audio_AVCodecContext == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(audio_AVCodecContext, aVFormatContext->streams[audio_index]->codecpar);


	//解码器没有找到
	if (video_codec == NULL && audio_codec == NULL)
	{
		return -1;
	}

	//打开解码器
	if ((avcodec_open2(video_AVCodecContext, video_codec, NULL) < 0 &
		avcodec_open2(audio_AVCodecContext, audio_codec, NULL) < 0))
	{
		return -1;
	}


	//======音频转码准备======start======
	enum AVSampleFormat in_sample_fmt = audio_AVCodecContext->sample_fmt;//输入的采样格式
	sample_rate = audio_AVCodecContext->sample_rate;//输入的采样率
	uint64_t in_ch_layout = audio_AVCodecContext->channel_layout;//输入的声道布局

	//audio_SwrContext = swr_alloc();
	//swr_alloc_set_opts(audio_SwrContext, out_ch_layout, AVSampleFormat::AV_SAMPLE_FMT_S16, sample_rate, in_ch_layout, in_sample_fmt, sample_rate, 0, NULL);
	//swr_init(audio_SwrContext);

	nb_channels = av_get_channel_layout_nb_channels(out_ch_layout);//获取声道个数
	audio_out_buffer = (uint8_t*)av_malloc(sample_rate * 2);//存储pcm数据
	//======音频转码准备======end======


	av_init_packet(aVPacket);


	return 0;
}

//读取一帧
int read_frame() {
	int ret = -1;

	//int got_frame;

	//从packet中解出来的原始视频帧
	AVFrame* original_video_frame = av_frame_alloc();//返回一个填充默认值的AVFrame
	AVFrame* original_audio_frame = av_frame_alloc();

	if (av_read_frame(aVFormatContext, aVPacket) >= 0) {//AVFormatContext* fmt_ctx; 返回下一帧的stream
		//av_read_frame:  应用 AVFormatContext 从 packet 读取下一个 AVFrame 的数据流


		if (aVPacket->stream_index == video_index)//对比packet->stream_index 的流类型
		{

			//解码。输入为packet，输出为original_video_frame
			if (avcodec_send_packet(video_AVCodecContext, aVPacket) == 0)//向解码器(AVCodecContext)发送需要解码的数据包(packet),0 表示解码成功
			{
				ret = avcodec_receive_frame(video_AVCodecContext, original_video_frame);//AVCodecContext* video_codec_ctx;
				//avcodec_receive_frame: 从解码器获取解码后的一帧,0表是解释成功

				if (ret == 0)//0表是解释成功
				{
					/*图片格式转换（上面图片转换准备的参数，在这里使用）*/
					//sws_scale(video_SwsContext,//图片转码上下文
					//	(const uint8_t* const*)original_video_frame->data,//原始数据
					//	original_video_frame->linesize,//原始参数
					//	0,//转码开始游标，一般为0
					//	video_AVCodecContext->height,//行数
					//	video_out_AVFrame->data,//转码后的数据
					//	video_out_AVFrame->linesize);
				}
			}
		}
		else if (aVPacket->stream_index == audio_index)
		{

			if (avcodec_send_packet(audio_AVCodecContext, aVPacket) >= 0) {
				fprintf(stderr, "Error submitting the packet to the decoder\n");
				return 0;
			}

			ret = avcodec_receive_frame(audio_AVCodecContext, original_audio_frame);
			if (ret == 0) {
				//swr_convert(audio_SwrContext,//音频转换上下文
				//	&audio_out_buffer,//输出缓存
				//	sample_rate * 2,//每次输出大小
				//	(const uint8_t**)original_audio_frame->data,//输入数据
				//	original_audio_frame->nb_samples);//输入

				//audio_out_buffer_size = av_samples_get_buffer_size(NULL, nb_channels, original_audio_frame->nb_samples, AVSampleFormat::AV_SAMPLE_FMT_S16, 1);
				ret = 1;
				
			}
		}
	}

	av_packet_unref(aVPacket);
	av_free(original_audio_frame);
	av_free(original_video_frame);
	//original_video_frame = NULL;
	//original_audio_frame = NULL;
	printf("释放了!");
	return ret;
}

//释放资源
void release() {
	sws_freeContext(video_SwsContext);
	//swr_free(&audio_SwrContext);

	av_free(video_out_AVFrame);

	av_free(audio_out_buffer);
	av_free(video_out_buffer);

	avcodec_close(video_AVCodecContext);
	avcodec_close(audio_AVCodecContext);

	avformat_close_input(&aVFormatContext);
}


//获取音频缓存大小
int get_audio_buffer_size() {
	return audio_out_buffer_size;
}

//获取视频缓存大小
int get_video_buffer_size() {
	return video_out_buffer_size;
}

//获取音频帧
char* get_audio_frame() {
	return (char*)audio_out_buffer;
}

//获取视频帧
char* get_video_frame() {
	return (char*)video_out_buffer;
}

//获取视频宽度
int get_video_width() {
	return video_AVCodecContext->width;
}

//获取视频高度
int get_video_height() {
	return video_AVCodecContext->height;
}

