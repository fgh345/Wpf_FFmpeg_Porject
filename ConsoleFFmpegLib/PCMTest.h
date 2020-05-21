#pragma once
#include <iostream>

class PCMTest
{
public:
	int simplest_pcm16le_to_pcm8(char* url);
	int simplest_pcm16le_cut_singlechannel(char* url, int start_num, int dur_num);
	int simplest_pcm16le_cut_singlechannel2(char* url, int start_num, int dur_num);
};

