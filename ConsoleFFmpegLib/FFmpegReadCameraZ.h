#pragma once
#include <string>

//���
void initReadCameraZ();

// ��ʼ��ffmpeg
int initFFmpeg();

//��ӡ�����е���������
void show_dshow_device();

void show_dshow_device_option();

void show_vfw_device();

//��ʼ��SDL���� 
bool initSDLWindow();

//�ر�SDL����
void closeWindow();

//Loads individual image as texture
void loadTexture();

//�������
bool loadCamera();

//��ȡһ֡
int read_frame_by_dshow();
