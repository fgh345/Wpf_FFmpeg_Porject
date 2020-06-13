#include "MakeUP_DshowToH264.h"




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

void mp_dth264_start() {

	char device_in_url[] = "video=Surface Camera Front";//Surface Camera Front | USB2.0 Camera | TC-UB570, Video Capture
	char file_out_path[] = "result_file_camera_to_h264.flv";
	//char file_out_path[] = "rtmp://192.168.30.20/live/livestream"; //推流地址
	//配置输入

	avdevice_register_all();

	AVDictionary* options = NULL;
	//av_dict_set(&options, "list_devices", "true", 0);
	//av_dict_set(&options, "list_options", "true", 0);

	AVFormatContext* formatContext_input = avformat_alloc_context();

	AVInputFormat* fmt = av_find_input_format("dshow");

	avformat_open_input(&formatContext_input, device_in_url, fmt, &options);

	//avformat_find_stream_info(formatContextInput, NULL);

	AVStream* stream_input = formatContext_input->streams[0];//我的相机只有一个输入流 简单写了

	//处理解码器

	AVCodecContext* codecContext_input = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(codecContext_input, stream_input->codecpar);
	AVCodec* codec_input = avcodec_find_decoder(codecContext_input->codec_id);

	if (avcodec_open2(codecContext_input, codec_input, NULL) != 0)
		printf("开启解码器失败!");

	AVPixelFormat pix_fmt_out = AV_PIX_FMT_YUV420P;
	int width_out = codecContext_input->width;
	int height_out = codecContext_input->height;

	//int width_out = 1280;
	//int height_out = 720;

	AVFrame* frame420p = av_frame_alloc();

	//计算输出缓存
	int image_buffer_size = av_image_get_buffer_size(
		pix_fmt_out,//out
		width_out,
		height_out,
		1);

	//输出缓存
	uint8_t* image_buffer = new uint8_t[image_buffer_size];

	av_image_fill_arrays(
		frame420p->data,//out
		frame420p->linesize,//out
		image_buffer,//out
		pix_fmt_out,//out
		width_out,//out
		height_out,//out
		1);

	SwsContext* swsContext_input = sws_getContext(
		codecContext_input->width,//in
		codecContext_input->height,//in
		codecContext_input->pix_fmt,//in
		width_out,//out
		height_out,//out
		pix_fmt_out,//out
		SWS_BICUBIC,
		NULL, NULL, NULL);

	//配置输出
	AVFormatContext* formatContext_output;
	avformat_alloc_output_context2(&formatContext_output, av_guess_format(NULL, ".mov", NULL), "flv", NULL);

	//Open output URL
	if (avio_open(&formatContext_output->pb, file_out_path, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Failed to open output file! \n");
		return;
	}

	AVStream* stream_output = avformat_new_stream(formatContext_output, NULL);
	stream_output->avg_frame_rate.num = 1;
	stream_output->avg_frame_rate.den = 20;//设置帧率

	AVCodecContext* codecContext_output = avcodec_alloc_context3(NULL);
	codecContext_output->codec_id = formatContext_output->oformat->video_codec;//编码器id
	codecContext_output->codec_type = AVMEDIA_TYPE_VIDEO;//编码器类型
	codecContext_output->pix_fmt = pix_fmt_out;//像素格式  Test_480x272.yuv必须和次类型相等 否则需要转码
	codecContext_output->width = width_out;//视频宽
	codecContext_output->height = height_out;//视频高
	codecContext_output->time_base.num = 1;//时间基 分子
	codecContext_output->time_base.den = 1;//时间基 分母 //这里必填 但是 设置又无效.....
	codecContext_output->gop_size = 10;// 关键帧 = 总帧数/gop_size
	codecContext_output->qmin = 4;//决定像素块大小 qmin越大  画面块状越明显
	codecContext_output->qmax = 20;
	codecContext_output->max_b_frames = 4;//设置b帧是p帧的倍数

	avcodec_parameters_from_context(stream_output->codecpar, codecContext_output);

	//打印输出流信息
	av_dump_format(formatContext_output, 0, file_out_path, 1);

	// Set Option
	AVDictionary* param = 0;
	//H.264
	if (codecContext_output->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&param, "preset", "superfast", 0);//superfast slow
		av_dict_set(&param, "tune", "zerolatency", 0);
	}
	//处理编码器
	AVCodec* encoder = avcodec_find_encoder(codecContext_output->codec_id);
	if (avcodec_open2(codecContext_output, encoder, &param) < 0)
		printf("开启编码器失败!");


	//写入文件头
	avformat_write_header(formatContext_output, NULL);

	int pst_p = 0;

	int64_t duration = av_q2d(stream_output->avg_frame_rate) / av_q2d(stream_output->time_base);
	

	//准备解码
	AVPacket* avpkt_in = av_packet_alloc();
	av_init_packet(avpkt_in);

	AVPacket* avpkt_out = av_packet_alloc();
	av_init_packet(avpkt_out);

	AVFrame* frameOriginal = av_frame_alloc();
	
	int64_t w_frame_time_start;

	while (av_read_frame(formatContext_input, avpkt_in)==0)
	{
		w_frame_time_start = av_gettime();

		if (avcodec_send_packet(codecContext_input, avpkt_in) == 0)
			if (avcodec_receive_frame(codecContext_input, frameOriginal)==0)
			{

				//转换像素数据格式
				sws_scale(swsContext_input,
					(const uint8_t* const*)frameOriginal->data,
					frameOriginal->linesize,
					0,
					codecContext_input->height,
					frame420p->data,
					frame420p->linesize);

				frame420p->format = pix_fmt_out;
				frame420p->width = width_out;
				frame420p->height = height_out;
				frame420p->pts = pst_p++ * duration;

				//Encode
				if (avcodec_send_frame(codecContext_output, frame420p) == 0) {
					if (avcodec_receive_packet(codecContext_output, avpkt_out) == 0)
					{
						//printf("Succeed to encode %d frame!\n", pst_p++);

						avpkt_out->duration = duration;

					    //printf("pts:%d--dts:%d--duration:%d\n", avpkt_out->pts, avpkt_out->dts, avpkt_out->duration);

						int ret = av_write_frame(formatContext_output, avpkt_out);

						if (ret != 0)
							printf("write fail %d!\n", ret);

						av_packet_unref(avpkt_out);
						

						//控制帧率
						int64_t duration_new = av_rescale_q(duration,  stream_output->time_base, { 1,AV_TIME_BASE });
						int64_t spanTime = duration_new - (av_gettime() - w_frame_time_start);

						printf("duration_new:%d --- %d  --- %d\n", duration_new, duration, spanTime);

						if (spanTime > 0)
							av_usleep(spanTime);


						int64_t dv = av_gettime() - w_frame_time_start;//大
						int real_frame_rate = stream_output->time_base.den/av_rescale_q(dv, { 1,AV_TIME_BASE }, stream_output->time_base);
						printf("实际帧率:%d\n", real_frame_rate);

					}
				}
			}
		av_packet_unref(avpkt_in);

		if (pst_p > 200)
			break;
	}
	
	//Write file trailer
	av_write_trailer(formatContext_output);

	//Clean
	if (stream_output) {
		avcodec_close(codecContext_output);
		av_free(frame420p);
	}
	avio_close(formatContext_output->pb);
	avformat_free_context(formatContext_output);

}


int mp_dth264_thread(void* opaque) {

	//while (true)
	//{
	//	SDL_Delay(mp_dth264_duration);
	//}
	
	return 0;
}