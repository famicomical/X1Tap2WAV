#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma pack(push, 1)

//note: In original code, long meant 32-bits. On 64bit linux, long is 64-bits. Using int type instead
//Modified Oct 2016 By famicomical

struct WAB_CNK {
	unsigned char name[4];
	unsigned int size;
} WCNK;

struct WAB_FMT {
	unsigned short id;      // フォーマットID
	unsigned short ch_cnt;  // チャネル数
	unsigned int  hz;      // サンプリング周波数
	unsigned int  spd;     // 平均データ速度
	unsigned short blk_siz; // ブロックサイズ
	unsigned short bit_siz; // １サンプル当たりのビット数
//	unsigned short headext;	// ?
} WFMT;

unsigned char TMP0[0x1000];
unsigned char TMP1[0x4000];

short tap2wav(unsigned char *tap_name,unsigned char *wav_name)
{
	unsigned int flng,freq;
	unsigned short siz,lng;
	unsigned short off,err;
	unsigned char  bit,dmy;
	FILE *des;
	FILE *src;
	err=0;
	src=des=NULL;

	src=fopen(tap_name,"rb");
	des=fopen(wav_name,"wb");
	if(src==NULL || des==NULL){
		if(src!=NULL) fclose(src);
		if(des!=NULL) fclose(des);
		remove(wav_name);
		return 1;
	}

	//-----
	fseek(src, 0L, SEEK_END);
	flng = ftell(src);
	rewind(src);
	fread(&freq,1,4,src);

	//-----

	strncpy(WCNK.name,"RIFF",4);
	// ファイル全体のサイズ−８
	WCNK.size=(flng-4l)*8l;
	WCNK.size+=46l-8l;
	fwrite(&WCNK,1,8,des);
	fwrite("WAVE",1,4,des);


	//-----

	strncpy(WCNK.name,"fmt ",4);
	WCNK.size=sizeof(struct WAB_FMT);
	fwrite(&WCNK,1,8,des);
	if(strcmp(&freq, "TAPE")){
		fseek(src, 28l, SEEK_SET);
		fread(&freq,1,4,src);
		rewind(src);
		// printf("Sampling rate is %dHz\n", freq );
	}
	WFMT.id=1;
	WFMT.ch_cnt=1;
	WFMT.hz =freq;
	WFMT.spd=freq;
	WFMT.blk_siz=1;
	WFMT.bit_siz=8;
//	WFMT.headext=0;
	fwrite(&WFMT,1,sizeof(struct WAB_FMT),des);

	//-----

	strncpy(WCNK.name,"data",4);
	WCNK.size=(flng-4l)*8l; // dataのサイズは必ず偶数
	fwrite(&WCNK,1,8,des);
	while(!err && flng){
		if(flng>0x800l){ lng=0x800; flng-=0x800l; }
		else{           lng=(short)flng; flng=0l; }

		fread(TMP0,1,lng,src); if(ferror(src)){ err=1; break; }

		siz=0; off=0; bit=0x80;
		while(off<lng){
			TMP1[siz++]=((TMP0[off]&bit)?0xff:0x00);
			bit>>=1; if(!bit){ bit=0x80; off++; }
		}

		fwrite(TMP1,1,siz,des); if(ferror(des)){ err=2; break; }
	}

	//-----

	fclose(src);
	fclose(des);
	if(err) remove(wav_name);

	return err;
}

void main(int argc,unsigned char *argv[])
{
	unsigned char *src;
	unsigned char *des;
	short i,err;

	err=0; src=des=NULL;
	for(i=1; i<argc && !err; i++){
		if(argv[i][0]=='/' || argv[i][0]=='-'){
			err=1;
		} else {
			if(     src==NULL) src=argv[i];
			else if(des==NULL) des=argv[i];
			else err=1;
		}
	}
	if(src==NULL || des==NULL || err){
		printf("TAP2WAV (Ver. 0.2)\n");
		printf("  TAP to WAV Conversion\n");
		printf(" Usage:\nTAP2WAV [sourcefile.tap] [targetfile.wav]\n");
		exit(1);
	}

	if(tap2wav(src,des)) printf("Error!\n");
	else                 printf("Conversion completed successfully!\n");
}

