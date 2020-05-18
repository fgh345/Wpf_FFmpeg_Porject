#pragma once
#include <string>

//���
void startReadCameraZ();

// ��ʼ��ffmpeg
int frcz_initFFmpeg();

//��ӡ�����е���������
void show_dshow_device();

void show_dshow_device_option();

void show_vfw_device();

//��ʼ��SDL���� 
bool frcz_initSDLWindow();

//�ر�SDL����
void frcz_closeWindow();

//�������
bool loadCamera();

//����ΪYUV��ʽ�ļ�
void saveYUVFile();

//����ΪYUV420P��ʽ�ļ�
void toSaveYUV420PFile();

//����ΪYUV422P��ʽ�ļ�
void toSaveYUV422PFile();


//��ȡһ֡
int read_frame_by_dshow();

//�ͷ���Դ
void frcz_release();

//��������
int frcz_open_rtmp_fun();

//����SDL����
int frcz_open_window_fun();

//���߳�
int frcz_refresh_thread(void* opaque);
