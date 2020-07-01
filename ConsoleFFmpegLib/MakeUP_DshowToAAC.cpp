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


void mp_dtaac_start() {

	char device_in_url[] = "audio=virtual-audio-capturer";//HKZ (Realtek High Definition Audio(SST)) | ub570 (TC-UB570, Audio Capture) | virtual-audio-capturer
	char file_out_path[] = "result_file_microphone_to_aac.aac";
	//char file_out_path[] = "rtmp://192.168.30.20/live/livestream"; //������ַ
	//��������

	avdevice_register_all();

	//void* i = 0;
	//const AVInputFormat* aa = NULL;
	//������ӡ��װ��ʽ��
	//while ((aa = av_demuxer_iterate(&i))) {
	//	printf("cccc:%s \n", aa->name);
	//}

	AVDictionary* options = NULL;
	//av_dict_set(&options, "list_devices", "true", 0);
	//av_dict_set(&options, "list_options", "true", 0);

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

	AVFormatContext* formatContext_output;
	avformat_alloc_output_context2(&formatContext_output, av_guess_format("", ".aac", NULL), NULL, NULL);

	//Open output URL
	if (avio_open(&formatContext_output->pb, file_out_path, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Failed to open output file! \n");
		return;
	}

	AVStream* stream_output = avformat_new_stream(formatContext_output, NULL);
	stream_output->time_base = stream_input->time_base;
	
	AVCodecContext* codecContext_output = avcodec_alloc_context3(NULL);
	codecContext_output->codec_id = formatContext_output->oformat->audio_codec;//������id
	codecContext_output->codec_type = AVMEDIA_TYPE_AUDIO;//����������
	codecContext_output->sample_fmt = out_sample_fmt;
	codecContext_output->sample_rate = out_sample_rate;
	codecContext_output->channel_layout = out_channel_layout;
	codecContext_output->channels = av_get_channel_layout_nb_channels(out_channel_layout);
	codecContext_output->bit_rate = 128000;
	//codecContext_output->frame_size = 1024;
	//codecContext_output->codec_tag = 0;
	//codecContext_output->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


	avcodec_parameters_from_context(stream_output->codecpar, codecContext_output);

	//��ӡ�������Ϣ
	av_dump_format(formatContext_output, 0, file_out_path, 1);

	//���������

	//printf("������id:%d\n", codecContext_output->codec_id);

	AVCodec* encoder = avcodec_find_encoder(codecContext_output->codec_id);
	if (avcodec_open2(codecContext_output, encoder, NULL) < 0)
		printf("����������ʧ��!");



	int pst_p = 0;

	//׼������
	AVPacket* avpkt_in = av_packet_alloc();
	av_init_packet(avpkt_in);

	AVPacket* avpkt_out = av_packet_alloc();
	av_init_packet(avpkt_out);

	AVFrame* frameOriginal = av_frame_alloc();
	AVFrame* frameAAC = av_frame_alloc();
	
	frameAAC->nb_samples = 1024;
	frameAAC->format = out_sample_fmt;
	frameAAC->channel_layout = out_channel_layout;
	frameAAC->channels = av_get_channel_layout_nb_channels(out_channel_layout);
	frameAAC->sample_rate = out_sample_rate;


	

	//av_frame_get_buffer(frameAAC, 1);


	int audio_out_buffer_size = av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO), 1024, AV_SAMPLE_FMT_FLTP, 1);
	uint8_t* audio_out_buffer = (uint8_t*)av_malloc(audio_out_buffer_size);

	
	int cc = avcodec_fill_audio_frame(frameAAC, av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO), AV_SAMPLE_FMT_FLTP, audio_out_buffer, audio_out_buffer_size,1);

	//int dd = sizeof(*audio_out_buffer);

	uint8_t** ddd = &audio_out_buffer;
	uint8_t** eee = (uint8_t**)audio_out_buffer;

	printf("\n audio_out_buffer_size: %p\n", eee);
	printf("\n audio_out_buffer_size: %p\n", eee+1);
	//printf("\n audio_out_buffer_size: %d\n", *(audio_out_buffer + 2));

	


	//������Ƶ����ת��

	//Swr
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

	//int64_t time_start= av_gettime();


	while (av_read_frame(formatContext_input, avpkt_in) == 0)
	{
		if (avcodec_send_packet(codecContext_input, avpkt_in) == 0)
			if (avcodec_receive_frame(codecContext_input, frameOriginal) == 0)
			{

				//ת�����ݸ�ʽ
				int ret = swr_convert(swrContext, (uint8_t**)audio_out_buffer, 15, (const uint8_t**)frameOriginal->data, frameOriginal->nb_samples);
				if (ret < 0)
					break;
				
				printf("ʵ��samples:%d\n", ret);

				//frameAAC->data[0] = audio_out_buffer;

				//Encode
				frameAAC->pts = av_rescale_q(pst_p, codecContext_output->time_base, stream_output->time_base);
				if (avcodec_send_frame(codecContext_output, frameAAC) == 0) {
					if (avcodec_receive_packet(codecContext_output, avpkt_out) == 0)
					{

						printf("pts:%d--dts:%d--duration:%d--pst_p:%d\n", avpkt_out->pts, avpkt_out->dts, avpkt_out->duration, pst_p++);

						int ret = av_write_frame(formatContext_output, avpkt_out);

						if (ret != 0)
							printf("write fail %d!\n", ret);

						av_packet_unref(avpkt_out);

					}
				}
			}
		av_packet_unref(avpkt_in);


		if (pst_p > 1000)
			break;

		
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