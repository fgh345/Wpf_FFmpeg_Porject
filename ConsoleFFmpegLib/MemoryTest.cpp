#include "MemoryTest.h"


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"  
#include "libswscale/swscale.h"  
#include "libavutil/imgutils.h"  
#include "libavdevice/avdevice.h"
}



void mt_start() {
	//avformat_network_init();//֧��������
	avdevice_register_all();//��ʹ��libavdevice֮ǰ������������avdevice_register_all()���豸����ע��

	//void* i = 0;
	//const AVCodec* p;

	////������ӡ������
	//while ((p = av_codec_iterate(&i))) {
	//	printf("cccc:%s \n", p->name);
	//}

	void* i = 0;
	const AVInputFormat* fmt = NULL;
	//������ӡ��װ��ʽ��
	while ((fmt = av_demuxer_iterate(&i))) {
		printf("cccc:%s \n", fmt->name);
	}
}