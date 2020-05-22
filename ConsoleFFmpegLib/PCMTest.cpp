#include "PCMTest.h"

	int PCMTest::simplest_pcm16le_to_pcm8(char* url) {
		FILE* fp;
		fopen_s(&fp,url, "rb+");
		FILE* fp1;
		fopen_s(&fp1,"output_8.pcm", "wb+");

		int cnt = 0;

		unsigned char* sample = (unsigned char*)malloc(4);

		while (!feof(fp)) {

			short* samplenum16 = NULL;
			char samplenum8 = 0;
			unsigned char samplenum8_u = 0;
			fread(sample, 1, 4, fp);
			//(-32768-32767)
			samplenum16 = (short*)sample;
			samplenum8 = (*samplenum16) >> 8;
			//(0-255)
			samplenum8_u = samplenum8 + 128;
			//L
			fwrite(&samplenum8_u, 1, 1, fp1);

			samplenum16 = (short*)(sample + 2);
			samplenum8 = (*samplenum16) >> 8;
			samplenum8_u = samplenum8 + 128;
			//R
			fwrite(&samplenum8_u, 1, 1, fp1);
			cnt++;
		}
		printf("Sample Cnt:%d\n", cnt);

		free(sample);
		fclose(fp);
		fclose(fp1);
		return 0;
	}


	int PCMTest::simplest_pcm16le_cut_singlechannel(char* url, int start_num, int dur_num) {
		int ret = -1;

		FILE* fp;
		ret = fopen_s(&fp,url, "rb+");

		if (ret != 0)
		{
			printf("fpÊ§°Ü!");
		}


		FILE* fp1;
		ret = fopen_s(&fp1,"output_cut.pcm", "wb+");

		if (ret != 0)
		{
			printf("fp1Ê§°Ü!");
		}

		FILE* fp_stat;
		ret = fopen_s(&fp_stat,"output_char.txt", "wb+");

		if (ret != 0)
		{
			printf("fp_statÊ§°Ü!");
		}

		unsigned char* sample = (unsigned char*)malloc(2);

		int cnt = 0;
		while (!feof(fp)) {
			fread(sample, 1, 2, fp);
			if (cnt > start_num && cnt <= (start_num + dur_num)) {
				fwrite(sample, 1, 2, fp1);

				short samplenum = sample[1];

				//printf("sample[0]:%d ", sample[0]);
				//printf("sample[1]:%d ", sample[1]);

				samplenum = samplenum * 256;//°ÑÒ»¸öchar×óÎ»ÒÆ°ËÎ»

				printf("samplenum2:%d ", samplenum);

				samplenum = samplenum + sample[0];//È»ºó¼ÓÉÏµÚ¶ş¸ö×Ö½ÚÖµ

				printf("samplenum3:%d \n", samplenum);

				fprintf(fp_stat, "%6d,", samplenum);
				if (cnt % 10 == 0)
					fprintf(fp_stat, "\n", samplenum);
			}
			cnt++;
		}

		free(sample);
		fclose(fp);
		fclose(fp1);
		fclose(fp_stat);
		return 0;
	}


	int PCMTest::simplest_pcm16le_cut_singlechannel2(char* url, int start_num, int dur_num) {
		int ret = -1;

		FILE* fp;
		ret = fopen_s(&fp, url, "rb+");

		if (ret != 0)
		{
			printf("fpÊ§°Ü!");
		}


		FILE* fp1;
		ret = fopen_s(&fp1, "output_cut.pcm", "wb+");

		if (ret != 0)
		{
			printf("fp1Ê§°Ü!");
		}

		FILE* fp_stat;
		ret = fopen_s(&fp_stat, "output_short.txt", "wb+");

		if (ret != 0)
		{
			printf("fp_statÊ§°Ü!");
		}

		short* sample = (short*)malloc(2);

		int cnt = 0;
		while (!feof(fp)) {
			fread(sample, 2, 1, fp);
			if (cnt > start_num && cnt <= (start_num + dur_num)) {
				fwrite(sample, 2, 1, fp1);

				printf("sample:%d \n", *sample);

				fprintf(fp_stat, "%6d,", *sample);
				if (cnt % 10 == 0)
					fprintf(fp_stat, "\n");
			}
			cnt++;
		}

		free(sample);
		fclose(fp);
		fclose(fp1);
		fclose(fp_stat);
		return 0;
	}
