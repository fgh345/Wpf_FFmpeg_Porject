#include "MemoryTest.h"


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"  
#include "libswscale/swscale.h"  
#include "libavutil/imgutils.h"  
#include "libavdevice/avdevice.h"
}



void mt_start() {
	avformat_network_init();//֧��������
	avdevice_register_all();//��ʹ��libavdevice֮ǰ������������avdevice_register_all()���豸����ע��


	while (1)//��ȡһ֡��ֱ����ȡ������
	{
		AVFrame* original_video_frame = av_frame_alloc();//����һ�����Ĭ��ֵ��AVFrame
		av_free(original_video_frame);
	}
}