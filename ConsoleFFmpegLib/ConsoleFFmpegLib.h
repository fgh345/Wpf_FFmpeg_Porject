#pragma once

int cffStart();


//url���ļ���ַ extern "C" _declspec(dllexport) 
int init_ffmpeg(char* url);

//��ȡһ֡ -1��δȡ�� 1����Ƶ 2����Ƶ 
int read_frame();

//��ȡ��Ƶ֡
char* get_audio_frame();

//��ȡ��Ƶ֡
char* get_video_frame();

//��ȡ��Ƶ�����С
int get_audio_buffer_size();

//��ȡ��Ƶ�����С
int get_video_buffer_size();

//��ȡ��Ƶ���
int get_video_width();

//��ȡ��Ƶ�߶�
int get_video_height();

//�ͷ���Դ
void release();
