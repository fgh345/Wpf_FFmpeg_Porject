#include <iostream>
#include "FFmpegReadCameraZ.h"
#include "FFmpegPushRtmp.h"
#include "SDL_LoadImg.h"
#include "ConsoleFFmpegLib.h"
#include "FFMpegAudioPlay.h"
#include "PCMTest.h"
#include "MemoryTest.h"
#include "FFmpegEncodeYUV.h"
#include "FFmpegEncodePCM.h"
#include "MakeUP_DshowToH264.h"
#include "MakeUP_DshowToAAC.h"

int main() {

	//startReadCameraZ();

	//initSDL_LoadImg();

	//cffStart();

	//������Ƶ
	//ffap_start();

	//ת��YUV
	//ffecyuv_start();

	//ת��PCM  ʧ��!
	//ffecpcm_start();

	//����rtmp
	//ffpr_start();

	//PCMTest pc = PCMTest();

	//pc.simplest_pcm16le_to_pcm8((char*)"..\\..\\Test_44.1k_s16le.pcm");
	//pc.simplest_pcm16le_cut_singlechannel((char*)"..\\..\\Test_drum.pcm", 2360, 120);
	//pc.simplest_pcm16le_cut_singlechannel2((char*)"..\\..\\Test_drum.pcm", 2360, 120);

	//short twoB = 4367;
	//short* ptwoB = &twoB;

	//char* oneB = (char*)ptwoB;

	//printf("AA:%d\n", ptwoB);
	//printf("BB:%d\n", oneB[1]);
	//printf("CC:%d\n", *(oneB + 1));
	//printf("DD:%d\n",oneB);

	//mt_start();

	//��ȡdshow�豸����Ϊh264
	//mp_dth264_start();

	//��ȡdshow�豸����Ϊaac
	mp_dtaac_start();

	return 0;
}

