#include "FFmpegPushRtmp.h"

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


//char ffpr_filepath[] = "..\\..\\Test_vedio.mp4"; //读取本地文件地址
//char ffpr_filepath[] = "E:\\Download\\GOPR4656.MP4"; //读取本地文件地址
char ffpr_filepath[] = "C:\\Users\\Youzh\\Videos\\rain.mp4"; //读取本地文件地址
//char ffpr_filepath[] = "C:\\Users\\Youzh\\Videos\\jxyy.mp4"; //读取本地文件地址


//char ffpr_rtmp_url[] = "rtmp://192.168.30.20/live/livestream"; //推流地址
char ffpr_rtmp_url[] = "result_file_push_rtmp.flv"; //推流地址

struct SwrContext* ffpr_swrContext = NULL;//音频转换

AVFormatContext* ffpr_aVFormatContext = NULL;

AVPacket* ffpr_packet = NULL;

int ffpr_video_index = -1;
int ffpr_audio_index = -1;

void ffpr_start()
{
	ffpr_initFFmpeg();
	ffpr_open_rtmp_fun();
}

int ffpr_initFFmpeg()
{

	ffpr_aVFormatContext = avformat_alloc_context();

	if (avformat_open_input(&ffpr_aVFormatContext, ffpr_filepath, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if (avformat_find_stream_info(ffpr_aVFormatContext, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	for (int i = 0; i < ffpr_aVFormatContext->nb_streams; i++) {
		if (ffpr_aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ffpr_video_index = i;
			continue;
		}

		if (ffpr_audio_index == -1 && ffpr_aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ffpr_audio_index = i;
			continue;
		}
	}

	ffpr_packet = av_packet_alloc();
	av_init_packet(ffpr_packet);

    return 0;
}

int ffpr_open_rtmp_fun()
{
	AVFormatContext* ffpr_aVFormatContext_rtmp = NULL;

	avformat_alloc_output_context2(&ffpr_aVFormatContext_rtmp, NULL, "flv", NULL); //RTMP

	if (!ffpr_aVFormatContext_rtmp) {
		printf("Could not create output context\n");
		return -1;
	}

	//处理视频流
	if (ffpr_video_index >= 0)
	{
		//Create output AVStream according to input AVStream
		AVStream* in_stream = ffpr_aVFormatContext->streams[ffpr_video_index];

		AVStream* out_stream = avformat_new_stream(ffpr_aVFormatContext_rtmp, NULL);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			return -1;
		}
		//Copy the settings of AVCodecContext
		AVCodecContext* codecContext = avcodec_alloc_context3(NULL);
		//从in_stream读取数据到dest里面
		int ret = avcodec_parameters_to_context(codecContext, in_stream->codecpar);
		
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			return -1;
		}
		codecContext->codec_tag = 0;
		if (ffpr_aVFormatContext_rtmp->oformat->flags & AVFMT_GLOBALHEADER)
			codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		//将dest应用到out_stream里面
		ret = avcodec_parameters_from_context(out_stream->codecpar, codecContext);

		if (ret < 0) {
			printf("Failed to copy codec context to out_stream codecpar context\n");
			return -1;
		}
	}

	//处理音频流
	if (ffpr_audio_index >= 0) {
		//Create output AVStream according to input AVStream
		AVStream* in_stream = ffpr_aVFormatContext->streams[ffpr_audio_index];

		AVStream* out_stream = avformat_new_stream(ffpr_aVFormatContext_rtmp, NULL);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			return -1;
		}
		//Copy the settings of AVCodecContext
		AVCodecContext* codecContext = avcodec_alloc_context3(NULL);

		int ret = avcodec_parameters_to_context(codecContext, in_stream->codecpar);

		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			return -1;
		}

		codecContext->codec_tag = 0;
		if (ffpr_aVFormatContext_rtmp->oformat->flags & AVFMT_GLOBALHEADER)
			codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		//将dest应用到out_stream里面
		ret = avcodec_parameters_from_context(out_stream->codecpar, codecContext);

		if (ret < 0) {
			printf("Failed to copy codec context to out_stream codecpar context\n");
			return -1;
		}
	}

	//Dump Format------------------ 打印有关输入或输出格式的详细信息，例如
	//持续时间，比特率，流，容器，程序，元数据，边数据，编解码器和时基
	printf("====av_dump_format====\n");
	av_dump_format(ffpr_aVFormatContext_rtmp, 0, ffpr_rtmp_url, 1);

	//Open output URL
	printf("====Open output URL====\n");

	printf("oformat->flags: %d \n", (ffpr_aVFormatContext_rtmp->oformat->flags & AVFMT_NOFILE));

	if (!(ffpr_aVFormatContext_rtmp->oformat->flags & AVFMT_NOFILE)) {
		int ret = avio_open(&ffpr_aVFormatContext_rtmp->pb, ffpr_rtmp_url, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Could not open output URL '%s'\n", ffpr_rtmp_url);
			return -1;
		}
		else {
			printf("avio_open success\n");
		}
	}

	//Write file header 分配流私有数据并将流头写入输出媒体文件。
	int ret = avformat_write_header(ffpr_aVFormatContext_rtmp, NULL);
	if (ret == AVSTREAM_INIT_IN_WRITE_HEADER) {
		printf("avformat_write_header success !\n");
		
	}
	else if (ret == AVSTREAM_INIT_IN_INIT_OUTPUT) {
		printf("AVSTREAM_INIT_IN_INIT_OUTPUT!\n");
	}
	else
	{
		printf("avformat_write_header fail\n");
		return -1;
	}

	//return -1;

	int64_t start_time = av_gettime();

	printf("start_time:%d", start_time);

	int frame_index = 0;//推送的帧计数

	while (1) {

		AVStream* in_stream, * out_stream;
		//Get an AVPacket
		ret = av_read_frame(ffpr_aVFormatContext, ffpr_packet);
		if (ret < 0)
			break;

		//if (!(ffpr_packet->stream_index == ffpr_audio_index || ffpr_packet->stream_index == ffpr_video_index))
		//{
		//	continue;
		//}

		if (ffpr_packet->stream_index != ffpr_video_index)
		{
			continue;
		}

		//if (ffpr_packet->stream_index != ffpr_audio_index)
		//{
		//	continue;
		//}

		//FIX：No PTS (Example: Raw H.264)
		//Simple Write PTS
		if (ffpr_packet->pts == AV_NOPTS_VALUE) {

			printf("AV_NOPTS_VALUE:没有PTS");

			//Write PTS
			AVRational time_base1 = ffpr_aVFormatContext->streams[ffpr_video_index]->time_base;

			//Duration between 2 frames (us)
			int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ffpr_aVFormatContext->streams[ffpr_video_index]->r_frame_rate);
			//Parameters
			ffpr_packet->pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
			ffpr_packet->dts = ffpr_packet->pts;
			ffpr_packet->duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
		}

		//Important:Delay 解码延迟 否则视频会瞬间推送完成
		if (ffpr_packet->stream_index == ffpr_video_index) {

			AVRational time_base = ffpr_aVFormatContext->streams[ffpr_video_index]->time_base;
			AVRational time_base_q = { 1,AV_TIME_BASE };
			int64_t dts_time = av_rescale_q(ffpr_packet->dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (dts_time > now_time)
				av_usleep(dts_time - now_time);
		}

		//printf("ffpr_packet->stream_index:%d\n", ffpr_packet->stream_index);

		in_stream = ffpr_aVFormatContext->streams[ffpr_packet->stream_index];
		out_stream = ffpr_aVFormatContext_rtmp->streams[ffpr_packet->stream_index];


		//printf("*** old-> pts:%d---dts:%d---duration:%d ***", ffpr_packet->pts, ffpr_packet->dts, ffpr_packet->duration);

		/* copy packet */
		//Convert PTS/DTS
		ffpr_packet->pts = av_rescale_q_rnd(ffpr_packet->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_DOWN | AV_ROUND_PASS_MINMAX));
		ffpr_packet->dts = av_rescale_q_rnd(ffpr_packet->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_DOWN | AV_ROUND_PASS_MINMAX));
		ffpr_packet->duration = av_rescale_q(ffpr_packet->duration, in_stream->time_base, out_stream->time_base);
		ffpr_packet->pos = -1;

		//printf("pts:%d---dts:%d---duration:%d \n", ffpr_packet->pts, ffpr_packet->dts, ffpr_packet->duration);

		//Print to Screen
		if (ffpr_packet->stream_index == ffpr_video_index) {
			printf("Send %8d video frames to output URL\n", frame_index);
			frame_index++;
		}

		//ret = av_interleaved_write_frame(ffpr_aVFormatContext_rtmp, ffpr_packet);
		ret = av_write_frame(ffpr_aVFormatContext_rtmp, ffpr_packet);

		if (ret == 0) {
			printf("av_write_frame success! \n");

		}

		av_packet_unref(ffpr_packet);
	}

	//Write file trailer 将流尾写入输出媒体文件并释放 文件私有数据。 只能在成功调用avformat_write_头之后调用。
	av_write_trailer(ffpr_aVFormatContext_rtmp);

	printf("推送完毕!");

	return 0;
}
