#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdlib.h> 
#include <fstream>
using namespace std;
union HeaderUnion
{
	char cHearder[48];
	struct
	{
		char sqeSequenceNumber[6];
		char dataQualityIndicator;
		char reserved;
		char StationIdentifierCode[5];
		char LocationIdentifier[2];
		char ChannelIdentifier[3];
		char NetworkCode[2];
		char TIME[10];
		unsigned short NumofSamples;
		short SampleRate;
		short SampleRateMultiplier;
		unsigned char ActivityFlags;
		unsigned char IOFlags;
		unsigned char QualityFlags;
		unsigned char NumberBlockettesFollow;
		long TimeCorrection;
		unsigned short OffsetBeginData;
		unsigned short OffsetFirstBlockette;
	}sHearder;
};

union blocketteUnion
{
	char cBlockette[8];
	struct
	{
		unsigned short type;
		unsigned short OffsetNextBlockette;
		unsigned char EncodingFormat;
		unsigned char WordOrder;
		unsigned char DataRecordLength;
		unsigned char Reserved;
	}sBlockette;
};


union dataUnion
{
	char cData[4032];
	int Data[4032];
};

//对应int32大小的成员 的转换 范例   
short swapInt16(short value)
{
	return ((value & 0x00FF) << 8) |
		((value & 0xFF00) >> 8);
}
//采用CPP模式写二进制文件  
void DataWrite_CPPMode(char* p, int len)
{
	//写出数据  
	ofstream f("binary.miniseed", ios::binary);
	if (!f)
	{
		//cout << "创建文件失败" << endl;
		return;
	}
	f.write(p, len);      //fwrite以char *的方式进行写出，做一个转化  
	f.close();
}

void getminiseed(int* data, int len)
{
	HeaderUnion _hearder;
	blocketteUnion _blockette1000;
	blocketteUnion _blockette1001;
	dataUnion  _Data;

	int i,j,k,m,n,sbn,l;
	int d[28200];
	unsigned int dd,temp;
	char s[32];

	int bnib[7] = { 4, 5, 6, 8, 10, 15, 30 };	//number of bit
	int	nnib[7] = { 7, 6, 5, 4, 3, 2, 1 };		// number of differences
	int	cnib[7] = { 3, 3, 3, 1, 2, 2, 2 };      // Cnib value
	int dnib[7] = { 2, 1, 0, -1, 3, 2, 1 };	    // Dnib value


												//写出数据  
	ofstream f("binary.miniseed", ios::binary);
	if (!f)
	{
		//cout << "创建文件失败" << endl;
		return;
	}

	memset(&_hearder,0, 48);
	/*****************Header****************/
	_hearder.sHearder.sqeSequenceNumber[5] = 1;
	_hearder.sHearder.dataQualityIndicator = 'D';
	_hearder.sHearder.reserved = ' ';
	_hearder.sHearder.SampleRate = swapInt16(200);
	_hearder.sHearder.SampleRateMultiplier = swapInt16(1);
	_hearder.sHearder.NumofSamples = swapInt16(len);
	_hearder.sHearder.NumberBlockettesFollow = 1;
	_hearder.sHearder.OffsetBeginData = swapInt16(64);
	_hearder.sHearder.OffsetFirstBlockette = swapInt16(48);
	f.write(_hearder.cHearder, 48);
	/**************blockette 1000************/
	memset(&_blockette1000, 0, 8);
	_blockette1000.sBlockette.type = swapInt16(1000);
	_blockette1000.sBlockette.OffsetNextBlockette = 0;
	_blockette1000.sBlockette.EncodingFormat = 0x0B;
	_blockette1000.sBlockette.WordOrder = 0x0;
	_blockette1000.sBlockette.DataRecordLength =0x9;
	
	f.write(_blockette1000.cBlockette, 8);
	memset(&_blockette1001, 0, 8);
	f.write(_blockette1001.cBlockette, 8);
	/**************data************/
	n = 0;
	d[0] = 0;	
	for (i = 1; i < len ;i++ )
	{
		d[i] = data[i] - data[i - 1];
		printf("%d\n",d[i]);
	}
	for (i = 0; i < 63; i++)
	{
		_Data.Data[i * 16] = 0;
		for (j = 1; j < 16; j++)
		{
			if (i == 0)
			{
				if(j == 1)
				{
				  _Data.Data[j] = data[0];
				  printf("W1:%d \r\n", _Data.Data[j]);
				  continue;
				}
				else if (j == 2)
				{
					_Data.Data[j] = data[len-1];
					printf("W2:%d \r\n", _Data.Data[j]);
					continue;
				}
			}
			dd = 0;
			for (k = 0; k < 7; k++)
			{
				if (n + nnib[k] <= len)
				{
					sbn = pow(2, bnib[k] - 1);
					for (m = 0; m < nnib[k]; m++)
					{
						if ((d[n + m] >= -sbn) && (d[n + m] <= sbn - 1))
						{
							//dd = (dd << bnib[k]) + d[n + m] +(d[n + m]<0)*pow(2, bnib[k]);
							temp = d[n + m] + (d[n + m] < 0)*pow(2, bnib[k]);
							dd += temp << (bnib[k]*m);
						}
						else
						{
							dd = 0;
							break;
						}
					}
				}
				if (m == nnib[k])
				{	
					printf("m:%d \r\n", m);
					break;
				}
			}
			for (l = 0; l < m; l++)
			{
				printf("d=%d  ", d[n+l]);
			}
			
			if (dnib[k] > 0)
			{
				dd += (dnib[k] << 30);
			}
			_Data.Data[i*16+j] = dd;
			_Data.Data[i * 16] += cnib[k]<<(32-j*2-2);
			n += nnib[k];
			printf("  %2x  %d\r\n", _Data.Data[i * 16 + j], _Data.Data[i * 16 + j]);
			//printf("\n");
			if (n >= len)
			{
				break;
			}
		}
		printf("W%d:%d \r\n", i * 16,_Data.Data[i * 16]);
		if (n >= len)
		{
			break;
		}

	}

	f.write(_Data.cData, (i+1) * 16*4);
	f.close();
}



int main() 
{
	int _NumofSamples = 14,i=0;
	int d[1024] = {-5,-3,-2,0,-2,3,0,0,0,-1,-2,-4,-4,-5,-5,-9,-9,-12,-12,-14,-18,-17,-12};
	//for (i = 0; i < _NumofSamples; i++)
	//{
	////	d[i] = i;
	//}
	//DataWrite_CPPMode();
	/*fstream inStream;
	inStream.open("w2.txt", ios::in);
	int x;
	if (!inStream.is_open()) {
		printf("Open the file failure...\n");
		exit(0);
	}
	while (!inStream.eof())
	{
		inStream >> d[i++];
	}
		
	inStream.close();*/
	
	for (i = 0; i < 200; i++)
	{
		d[i] = (int)1000 * sin(i*3.14159 / 100);
	}
	d[i++] = 0;
	i++;
	
	getminiseed(d, i-1);
	//getminiseed(d, _NumofSamples);
	i = 1;
	i += 1 << 10;

	printf("%2x", i);
	printf("%c", "S");
	return 0;
}