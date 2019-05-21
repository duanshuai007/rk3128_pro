#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void delete_space(char *dst, char *src);
int getbssidpos(char *src, int len);

#define RFile	"/usr/share/nginx/html/tmp/wifimanager/result"
//par1: wifi列表 nmcli dev wifi list
//par2: wifi曾连接的列表 nmcli con show
int main(int argc, char *argv[])
{
	FILE *file;

	if(argc < 3)
	{
		printf("paramter too smail\r\n");
		return -1;
	}

	if(argv[1] == NULL || argv[2] == NULL)
	{
		printf("argv is NULL, please check!\r\n");
		return -1;
	}
    //printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
    //printf("%s\r\n", argv[1]);
    //printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
	char *wifilist[1024];
	char reme_wifi[40960];
	int list_number=0;
	int BSSID_POS, SIGNAL_POS, SECURITY_POS, ACTIVE_POS;
	char ssid[256], bssid[32], bssid4[20], signal[10], security[20], active[20];

	int i,j;
	char dststring[40960];

	wifilist[0] = strtok(argv[1], "\n");
	while(wifilist[list_number] != NULL)
	{
		list_number++;
		wifilist[list_number] = strtok(NULL, "\n");
	}

	//printf("argv[2]=%s\r\n", argv[2]);
	strcpy(reme_wifi, argv[2]);
	//printf("+++++D++D+D++D+D++D++D+D++D+D+D+D::::::len=%d\r\n", strlen(argv[2]));
	//printf("rememeber wifi:%s\r\n", reme_wifi);

    //开始数据分析
	char *ptr = strstr(wifilist[0], "BSSID");    
	BSSID_POS = ptr - wifilist[0];
	ptr = strstr(wifilist[0], "SIGNAL");
	SIGNAL_POS = ptr - wifilist[0];
	ptr = strstr(wifilist[0], "SECURITY");
	SECURITY_POS = ptr - wifilist[0];
	ptr = strstr(wifilist[0], "ACTIVE");
	ACTIVE_POS = ptr - wifilist[0];
	//printf("BSSID_POS=%d\r\n", BSSID_POS);
	//printf("SIGNAL_POS=%d\r\n", SIGNAL_POS);
	//printf("SECURITY_POS=%d\r\n", SECURITY_POS);
	//printf("ACTIVE_POS=%d\r\n", ACTIVE_POS);

	//printf("list[1]:%s\r\n", wifilist[1]);
	//strncpy(ssid, 		wifilist[1], 			BSSID_POS-1);
	//ssid[BSSID_POS-1] = '\0';
	//strncpy(bssid, 		wifilist[1] + BSSID_POS, 	SIGNAL_POS - BSSID_POS - 1);
	//bssid[SIGNAL_POS - BSSID_POS - 1] = '\0';
	//strncpy(bssid4,		bssid, 	11);
	//bssid4[11] = '\0';
	//strncpy(signal, 	wifilist[1] + SIGNAL_POS, 	SECURITY_POS - SIGNAL_POS - 1);
	//signal[SECURITY_POS - SIGNAL_POS - 1] = '\0';
	//strncpy(security, 	wifilist[1] + SECURITY_POS, 	ACTIVE_POS - SECURITY_POS - 1);
	//security[ACTIVE_POS - SECURITY_POS - 1] = '\0';
	//strncpy(active, 	wifilist[1] + ACTIVE_POS, 	4);
	//active[4] = '\0';
	//
	//printf("ssid=%s\r\n", ssid);
	//printf("bssid=%s\r\n", bssid);
	//printf("bssid4=%s\r\n", bssid4);
	//printf("signal=%s\r\n", signal);
	//printf("security=%s\r\n", security);
	//printf("active=%s\r\n", active);

	for(i = 1; i < list_number; i++)
	{
		//获取当前行的信息
		if(wifilist[i] == NULL)
			continue;

        int cur_bssid_pos=getbssidpos(wifilist[i], strlen(wifilist[i]));
        int index = 0;
        if( cur_bssid_pos >= BSSID_POS )
        {
            index = cur_bssid_pos - BSSID_POS;
        }
		strncpy(ssid, wifilist[i], BSSID_POS + index -1);
		ssid[BSSID_POS + index -1] = '\0';
		strncpy(bssid, wifilist[i] + BSSID_POS + index, SIGNAL_POS - BSSID_POS - 1);
		bssid[SIGNAL_POS - BSSID_POS - 1] = '\0';
		strncpy(bssid4, bssid, 11);
		bssid4[11] = '\0';
		strncpy(signal, wifilist[i] + SIGNAL_POS + index, SECURITY_POS - SIGNAL_POS - 1);
		signal[SECURITY_POS - SIGNAL_POS - 1] = '\0';
		strncpy(security, wifilist[i] + SECURITY_POS + index, ACTIVE_POS - SECURITY_POS - 1);
		security[ACTIVE_POS - SECURITY_POS - 1] = '\0';
		strncpy(active, wifilist[i] + ACTIVE_POS + index, 4);
		active[4] = '\0';

		for(j = i + 1; j < list_number; j++)
		{
			if(wifilist[j] == NULL)
				continue;

			if(strstr(wifilist[j], ssid) != NULL && strstr(wifilist[j], bssid4) != NULL)
			{
				//找到了同名的wifi
				char tmp_bssid[128], tmp_signal[10], tmp_active[20];
				
				//如果外循环的wifi是状态是yes，则删除内循环的这个同名wifi
				//如果外循环wifi状态不是yes
				if(!strstr(active, "yes"))
				{
					strncpy(tmp_bssid, wifilist[j] + BSSID_POS + index, SIGNAL_POS - BSSID_POS - 1);
					tmp_bssid[SIGNAL_POS - BSSID_POS - 1] = '\0';

					strncpy(tmp_active, wifilist[j] + ACTIVE_POS + index, 4);
					tmp_active[4] = '\0';
					
					strncpy(tmp_signal, wifilist[j] + SIGNAL_POS + index, SECURITY_POS - SIGNAL_POS - 1);
					tmp_signal[SECURITY_POS - SIGNAL_POS - 1] = '\0';

					if(strstr(tmp_active, "yes"))
					{
						//如果内循环的wifi是yes，则用内循环的signal,bssid,active替换外循环的wifi
						strcpy(bssid, tmp_bssid);
						strcpy(signal, tmp_signal);
						strcpy(active, tmp_active);
					}else
					{
						//内外循环的wifi状态都不是yes，那么就比较信号强度，信号强的显示。
						int signal_x, signal_y;
						signal_x = atoi(signal);
						signal_y = atoi(tmp_signal);
						//printf("signal_x=%d,signal_y=%d\r\n", signal_x, signal_y);
						if(signal_x < signal_y)
						{
							strcpy(bssid, tmp_bssid);
							strcpy(signal, tmp_signal);
							strcpy(active, tmp_active);
						}
					}
				}
				//删除内循环的同名wifi
				wifilist[j] = NULL;
			}
		}	

		delete_space(ssid, ssid);
		delete_space(bssid, bssid);
		delete_space(signal, signal);
		delete_space(security, security);
		delete_space(active, active);

		if(strstr(active, "no"))
		{
			//如果当前的wifi状态是no才判断是否是在连接列表中的wifi
			char realname[64];
			sprintf(realname, "%s+%s", ssid, bssid4);
			//printf("realname=%s\r\n", realname);

			if(strstr(reme_wifi, realname))
			{
				strcpy(active, "remembered");
			}
		}else if(strstr(active, "yes"))
		{
			strcpy(active, "connected");
		}

		strcat(dststring, ssid);
		strcat(dststring, "+++");
		strcat(dststring, bssid);
		strcat(dststring, "+++");
		strcat(dststring, signal);
		strcat(dststring, "+++");
		strcat(dststring, security);
		strcat(dststring, "+++");
		strcat(dststring, active);
		strcat(dststring, "\n");
	}

	//printf("%s", dststring);
	file=fopen(RFile, "w");
	if(file == NULL)
	{
	   printf("open file failed\r\n");
	   return -1;
	}

	fwrite(dststring, strlen(dststring), 1, file);

	fflush(file);

	fclose(file);

	return 0;
}

void delete_space(char *dst, char *src)
{
	int len;
	int i,j;

	len = strlen(src);

	while(src[len] == ' ' || src[len] == '\0')
	{
		len--;
	}

	len++;
	src[len] = '\0';
	
	dst = src;
}

int getbssidpos(char *src, int len)
{
    int i;

    for(i = 0; i < (len - 12); i++)
    {
        if((src[i] == ':') && (src[i+3] == ':') && (src[i+6]==':') && (src[i+9]==':') && (src[i+12]==':'))
        {
            return i-2;
        }
    }
    return -1;
}
