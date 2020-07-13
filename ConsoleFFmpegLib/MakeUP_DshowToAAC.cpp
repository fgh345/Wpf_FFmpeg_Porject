#include "MakeUP_DshowToAAC.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include "libavutil/time.h"
#include "SDL.h"
};

#include <iostream>

int XError(int errNum)
{
	char buf[1024] = { 0 };
	av_strerror(errNum, buf, sizeof(buf));
	std::cout << buf << std::endl;

	return -1;
}

void mp_dtaac_start() {

	char device_in_url[] = "audio=virtual-audio-capturer";//HKZ (Realtek High Definition Audio(SST)) | ub570 (TC-UB570, Audio Capture) | virtual-audio-capturer
	char file_out_path[] = "result_file_microphone_to_aac.aac";
	//char file_out_path[] = "rtmp://192.168.30.20/live/livestream"; //������ַ
	//��������

	avdevice_register_all();

	//avformat_network_init();

	//void* i = 0;
	//const AVInputFormat* aa = NULL;
	//������ӡ��װ��ʽ��
	//while ((aa = av_demuxer_iterate(&i))) {
	//	printf("cccc:%s \n", aa->name);
	//}

	AVDictionary* options = NULL;
	//av_dict_set(&options, "list_devices", "true", 0);
	/*av_dict_set(&options, "list_options", "true", 0);*/


	AVFormatContext* formatContext_input = avformat_alloc_context();

	AVInputFormat* fmt = av_find_input_format("dshow");

	avformat_open_input(&formatContext_input, device_in_url, fmt, &options);

	avformat_find_stream_info(formatContext_input, NULL);

	AVStream* stream_input = formatContext_input->streams[0];

	//��ӡ��������Ϣ
	av_dump_format(formatContext_input, 0, device_in_url, 0);

	//���������

	AVCodecContext* codecContext_input = avcodec_alloc_context3(NULL);
	codecContext_input->channel_layout = AV_CH_LAYOUT_STEREO;
	avcodec_parameters_to_context(codecContext_input, stream_input->codecpar);
	AVCodec* codec_input = avcodec_find_decoder(codecContext_input->codec_id);

	if (avcodec_open2(codecContext_input, codec_input, NULL) != 0)
		printf("����������ʧ��!");

	//�������

	int out_sample_rate = 48000;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLTP;
	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;

	AVFormatContext* formatContext_output;//av_guess_format("", ".ac3", NULL)
	avformat_alloc_output_context2(&formatContext_output, NULL, NULL, file_out_path);

	//Open output URL
	if (avio_open(&formatContext_output->pb, file_out_path, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Failed to open output file! \n");
		return;
	}

	AVStream* stream_output = avformat_new_stream(formatContext_output, NULL);
	stream_output->codecpar->codec_tag = 0;

	AVCodecContext* codecContext_output = avcodec_alloc_context3(NULL);
	codecContext_output->codec_id = formatContext_output->oformat->audio_codec;//������id
	codecContext_output->codec_type = AVMEDIA_TYPE_AUDIO;//����������
	codecContext_output->sample_fmt = out_sample_fmt;
	codecContext_output->sample_rate = out_sample_rate;
	codecContext_output->channel_layout = out_channel_layout;
	codecContext_output->channels = av_get_channel_layout_nb_channels(out_channel_layout);
	codecContext_output->bit_rate = 128000;
	codecContext_output->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


	avcodec_parameters_from_context(stream_output->codecpar, codecContext_output);

	//��ӡ�������Ϣ
	av_dump_format(formatContext_output, 0, file_out_path, 1);

	//���������

	AVCodec* encoder = avcodec_find_encoder(codecContext_output->codec_id);
	if (avcodec_open2(codecContext_output, encoder, NULL) < 0)
		printf("����������ʧ��!");

	int pst_p = 0;

	//׼������
	AVPacket avpkt_in;

	AVPacket* avpkt_out = av_packet_alloc();
	//av_init_packet(avpkt_out);

	AVFrame* frameOriginal = av_frame_alloc();
	AVFrame* frameAAC = av_frame_alloc();

	frameAAC->nb_samples = 1024;
	frameAAC->format = out_sample_fmt;
	frameAAC->channel_layout = out_channel_layout;
	frameAAC->channels = av_get_channel_layout_nb_channels(out_channel_layout);
	//frameAAC->sample_rate = out_sample_rate;

	av_frame_get_buffer(frameAAC, 0);

	//������Ƶ����ת��
	SwrContext* swrContext = swr_alloc_set_opts(
		NULL,
		out_channel_layout,                            /*out*/
		out_sample_fmt,                                 /*out*/
		out_sample_rate,                                /*out*/
		av_get_default_channel_layout(codecContext_input->channels),/*in Ĭ����� �������ֿ����ǿյ� ����ͨ�������� ��ȡһ��Ĭ�ϵ���������*/
		codecContext_input->sample_fmt,                 /*in*/
		codecContext_input->sample_rate,                /*in*/
		0,
		NULL);

	swr_init(swrContext);


	//д���ļ�ͷ
	avformat_write_header(formatContext_output, NULL);

	for (;;) {
		if (av_read_frame(formatContext_input, &avpkt_in) == 0)
		{
			if (avcodec_send_packet(codecContext_input, &avpkt_in) == 0)

				if (avcodec_receive_frame(codecContext_input, frameOriginal) == 0)
				{

					//ת�����ݸ�ʽ
					int ret = swr_convert(swrContext, frameAAC->data, frameOriginal->nb_samples, (const uint8_t**)frameOriginal->data, frameOriginal->nb_samples);
					
					//pts ����
					// second = nb_samples/sample_rate   һ֡��Ƶ������
					// pts = second/timebase 

					//Encode

					int rpts = av_rescale_q(frameOriginal->nb_samples, codecContext_output->time_base, stream_output->time_base);

					frameAAC->pts = pst_p++ + rpts;

					printf("ԭʼpts: %d!\n", frameAAC->pts);

					if (avcodec_send_frame(codecContext_output, frameAAC) == 0) {
						if (avcodec_receive_packet(codecContext_output, avpkt_out) == 0)
						{

							if (avpkt_out->size > 0) {

								avpkt_out->pts = av_rescale_q(avpkt_out->pts, codecContext_output->time_base, stream_output->time_base);// ǰ����ԭʼ�� ,������Ҫת����Ϊ��
								avpkt_out->dts = av_rescale_q(avpkt_out->dts, codecContext_output->time_base, stream_output->time_base);
								avpkt_out->duration = av_rescale_q(avpkt_out->duration, codecContext_output->time_base, stream_output->time_base);

								printf("���pts: %d!\n", avpkt_out->pts);

								ret = av_interleaved_write_frame(formatContext_output, avpkt_out);
								//if (!ret)
								//{
								//	XError(ret);
								//}
							}
						}
					}
				}
			av_packet_unref(&avpkt_in);


			if (pst_p++ > 3000)
				break;


		}
	}


	//Write file trailer
	av_write_trailer(formatContext_output);

	//Clean
	if (stream_output) {
		avcodec_close(codecContext_output);
		av_free(frameAAC);
	}
	avio_close(formatContext_output->pb);
	avformat_free_context(formatContext_output);
	swr_free(&swrContext);
}