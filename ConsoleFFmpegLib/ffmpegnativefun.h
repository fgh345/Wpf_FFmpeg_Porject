#pragma once

//url���ļ���ַ
extern "C" _declspec(dllexport) int init_ffmpeg(char* url);

//��ȡһ֡ -1��δȡ�� 1����Ƶ 2����Ƶ 
extern "C" _declspec(dllexport) int read_frame();

//��ȡ��Ƶ֡
extern "C" _declspec(dllexport) char* get_audio_frame();

//��ȡ��Ƶ֡
extern "C" _declspec(dllexport) char* get_video_frame();

//��ȡ��Ƶ�����С
extern "C" _declspec(dllexport) int get_audio_buffer_size();

//��ȡ��Ƶ�����С
extern "C" _declspec(dllexport) int get_video_buffer_size();

//��ȡ��Ƶ���
extern "C" _declspec(dllexport) int get_video_width();

//��ȡ��Ƶ�߶�
extern "C" _declspec(dllexport) int get_video_height();

//�ͷ���Դ
extern "C" _declspec(dllexport) void release();