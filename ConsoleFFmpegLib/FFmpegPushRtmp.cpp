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


char ffpr_filepath[] = "C:\\Users\\Youzh\\Videos\\rain.mp4"; //读取本地文件地址
//char ffpr_filepath[] = "E:\\BaiduNetdiskDownload\\借东西的小人阿莉埃缇.mkv"; //读取本地文件地址
//char ffpr_filepath[] = "C:\\Users\\Youzh\\Videos\\jxyy.mp4"; //读取本地文件地址
//char ffpr_filepath[] = "C:\\Users\\Youzh\\Videos\\jxyy.mp4"; //读取本地文件地址


//char ffpr_rtmp_url[] = "rtmp://192.168.30.20/live/livestream"; //推流地址
char ffpr_rtmp_url[] = "E:\\flvpull.flv"; //推流地址

struct SwrContext* ffpr_swrContext = NULL;//音频转换

AVFormatContext* ffpr_aVFormatContext = NULL;

AVCodecContext* ffpr_aVideoCodecContext;
AVCodecContext* ffpr_aVAudioCodecContext;

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
	//avformat_network_init();

	//avdevice_register_all();

	ffpr_aVFormatContext = avformat_alloc_context();

	ffpr_aVideoCodecContext = avcodec_alloc_context3(NULL);
	ffpr_aVAudioCodecContext = avcodec_alloc_context3(NULL);


	if (avformat_open_input(&ffpr_aVFormatContext, ffpr_filepath, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}

	//ffpr_aVFormatContext->pb

	if (avformat_find_stream_info(ffpr_aVFormatContext, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}


	for (int i = 0; i < ffpr_aVFormatContext->nb_streams; i++) {
		if (ffpr_aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ffpr_video_index = i;
			break;
		}
	}

	for (int i = 0; i < ffpr_aVFormatContext->nb_streams; i++) {
		if (ffpr_aVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ffpr_audio_index = i;
			break;
		}
	}


	if (ffpr_video_index == -1 || ffpr_audio_index == -1)
	{
		printf("Couldn't find a video or audio stream.\n");
		return -1;
	}

	if (ffpr_video_index >= 100)
	{
		//获取视频解码器 AVCodec* ffpr_aVideoCodec
		avcodec_parameters_to_context(ffpr_aVideoCodecContext, ffpr_aVFormatContext->streams[ffpr_video_index]->codecpar);

		//printf("ffpr_aVideoCodecContext->codec_id:%d\n", ffpr_aVideoCodecContext->codec_id);

		if (ffpr_aVideoCodecContext == NULL)
		{
			printf("Could not allocate ffap_aVideoCodecContext\n");
		}

		//AVCodec* ffpr_aVideoCodec = avcodec_find_decoder(ffpr_aVideoCodecContext->codec_id);

		//if (ffpr_aVideoCodec == NULL)
		//{
		//	printf("VideoCodec not found.\n");
		//	return -1;
		//}

		//if (avcodec_open2(ffpr_aVideoCodecContext, ffpr_aVideoCodec, NULL) < 0)
		//{
		//	printf("Could not open VideoCodec.\n");
		//	return -1;
		//}
	}

	if (ffpr_audio_index >= 0) {
		//获取音频解码器
		avcodec_parameters_to_context(ffpr_aVAudioCodecContext, ffpr_aVFormatContext->streams[ffpr_audio_index]->codecpar);

		if (ffpr_aVAudioCodecContext == NULL)
		{
			printf("Could not allocate ffap_aVAudioCodecContext\n");
		}

		AVCodec* ffpr_aVaudioCodec = avcodec_find_decoder(ffpr_aVAudioCodecContext->codec_id);

		AVCodec* ffpr_aVaudioEnCodec = avcodec_find_encoder(ffpr_aVAudioCodecContext->codec_id);

		if (ffpr_aVaudioCodec == NULL)
		{
			printf("AudioCodec not found.\n");
			return -1;
		}

		if (avcodec_open2(ffpr_aVAudioCodecContext, ffpr_aVaudioCodec, NULL) < 0)
		{
			printf("Could not open AudioCodec.\n");
			return -1;
		}

		if (avcodec_open2(ffpr_aVAudioCodecContext, ffpr_aVaudioEnCodec, NULL) < 0)
		{
			printf("Could not open AudioCodec.\n");
			return -1;
		}
	}

	ffpr_packet = av_packet_alloc();
	av_init_packet(ffpr_packet);


	//Swr
	ffpr_swrContext = swr_alloc_set_opts(NULL,
		AV_CH_LAYOUT_STEREO,                                /*out*/
		AV_SAMPLE_FMT_S16,                              /*out*/
		44100,                                         /*out*/
		av_get_default_channel_layout(ffpr_aVAudioCodecContext->channels),                                  /*in*/
		ffpr_aVAudioCodecContext->sample_fmt,               /*in*/
		ffpr_aVAudioCodecContext->sample_rate,               /*in*/
		0,
		NULL);

	swr_init(ffpr_swrContext);
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
		AVCodecContext* dest = avcodec_alloc_context3(NULL);
		//从in_stream读取数据到dest里面
		int ret = avcodec_parameters_to_context(dest, in_stream->codecpar);
		
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			return -1;
		}
		dest->codec_tag = 0;
		if (ffpr_aVFormatContext_rtmp->oformat->flags & AVFMT_GLOBALHEADER)
			dest->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		//将dest应用到out_stream里面
		ret = avcodec_parameters_from_context(out_stream->codecpar, dest);

		out_stream->time_base = in_stream->time_base;

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
		AVCodecContext* dest = avcodec_alloc_context3(NULL);

		dest->codec_tag = 0;
		if (ffpr_aVFormatContext_rtmp->oformat->flags & AVFMT_GLOBALHEADER)
			dest->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		dest->sample_rate = 44100;
		dest->codec = avcodec_find_decoder_by_name("aac");
		dest->codec_id = AV_CODEC_ID_AAC;
		dest->codec_type = AVMEDIA_TYPE_AUDIO;

		//将dest应用到out_stream里面
		int ret = avcodec_parameters_from_context(out_stream->codecpar, dest);

		out_stream->time_base = in_stream->time_base;

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
	if (ret < 0) {
		printf("avformat_write_header fail\n");
		return -1;
	}
	else
	{
		printf("avformat_write_header success !\n");
	}

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
		//	printf("没有流????");
		//	continue;
		//}

		if (ffpr_packet->stream_index != ffpr_video_index)
		{
			continue;
		}


		//尝试转换流
		//if (ffpr_packet->stream_index == ffpr_audio_index)
		//{
		//	AVFrame* frame_in = av_frame_alloc();
		//	AVFrame* frame_out = av_frame_alloc();
		//	
		//	if (avcodec_send_packet(ffpr_aVAudioCodecContext, ffpr_packet) == 0)//向解码器(AVCodecContext)发送需要解码的数据包(packet),0 表示解码成功
		//	{
		//		ret = avcodec_receive_frame(ffpr_aVAudioCodecContext, frame_in);//AVCodecContext* video_codec_ctx;
		//		//avcodec_receive_frame: 从解码器获取解码后的一帧,0表是解释成功
		//		if (ret == 0)
		//		{
		//			//av_frame_copy_props(frame_out, frame_in);
		//			//av_frame_copy(frame_out, frame_in);
		//			frame_out->sample_rate = 44100;
		//			frame_out->format = frame_in->format;
		//			frame_out->nb_samples = frame_in->nb_samples;
		//			int ret = swr_convert_frame(ffpr_swrContext, frame_out, frame_in);
		//			if (ret == 0)
		//			{
		//				printf("转码成功!\n");
		//			}
		//			else {
		//				printf("转码失败!%d\n", ret);
		//			}
		//			/*if (avcodec_send_frame(ffpr_aVAudioCodecContext, frame_out)==0)
		//			{
		//				avcodec_receive_packet(ffpr_aVAudioCodecContext, ffpr_packet);
		//				printf("封装完成!\n");
		//			}
		//			else
		//			{
		//				printf("封装失败!\n");
		//			}*/
		//		}
		//		else {
		//			printf("没读取到!");
		//		}
		//	}
		//	else {
		//		printf("解码失败!");
		//	}
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


		printf("*** old-> pts:%d---dts:%d---duration:%d ***", ffpr_packet->pts, ffpr_packet->dts, ffpr_packet->duration);

		/* copy packet */
		//Convert PTS/DTS
		ffpr_packet->pts = av_rescale_q_rnd(ffpr_packet->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_DOWN | AV_ROUND_PASS_MINMAX));
		ffpr_packet->dts = av_rescale_q_rnd(ffpr_packet->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_DOWN | AV_ROUND_PASS_MINMAX));
		ffpr_packet->duration = av_rescale_q(ffpr_packet->duration, in_stream->time_base, out_stream->time_base);
		ffpr_packet->pos = -1;

		printf("pts:%d---dts:%d---duration:%d \n", ffpr_packet->pts, ffpr_packet->dts, ffpr_packet->duration);

		//Print to Screen
		if (ffpr_packet->stream_index == ffpr_video_index) {
			printf("Send %8d video frames to output URL\n", frame_index);
			frame_index++;
		}
		//ret = av_write_frame(ofmt_ctx, &pkt);
		ret = av_interleaved_write_frame(ffpr_aVFormatContext_rtmp, ffpr_packet);

		if (ret < 0) {
			printf("Error muxing packet\n");
			break;
		}

		av_packet_unref(ffpr_packet);
	}

	//Write file trailer 将流尾写入输出媒体文件并释放 文件私有数据。 只能在成功调用avformat_write_头之后调用。
	av_write_trailer(ffpr_aVFormatContext_rtmp);

	return 0;
}
