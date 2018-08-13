/*********************************************************************/
/*Copyright(c) 2006 POSDATA                                          */
/*@File           : client.c                                         */
/*@FileName       : CUB Client                                       */
/*Open Issues     :                                                  */
/*Change history                                                     */
/*@LastModifyDate : 20060609                                         */
/*@LastModifier   : Shin Joo Youn                                    */
/*@LastVersion    : 1.0                                              */
/*2006-06-09        Shin Joo Youn                                    */
/*1.0																 */
/*-------------------------------------------------------------------*/														
/*	2018_01_11   usend.c    code									 */												
/*	Posco Cloude - Socket Communication								 */	
/*	Test in the Neuromeka									     	 */		
/*	Detail impormations are in the document (01_11_Weekly.ppt)		 */	
/*	by CYJ	v.0124													 */
/*********************************************************************/
#include <EAIAdapter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h> /* strftime() */
#include <unistd.h> /* sleep() */

// Added new header
#include <sys/socket.h>		// socket
#include <arpa/inet.h>		// inet_addr

// Added new macro define
#define _CRT_SECURE_NO_WARNINGS

#define mes_header_len 100
/* MES Interface Header 100 Byte 
    1     TRANSACTION_CODE                 8    VARCHAR2
    2     WORKS_CODE                       1    VARCHAR2
    3     OPER_FLAG                        1    VARCHAR2
    4     FAC_OP_CD                        2    VARCHAR2
    5     SNDR_INFORM_EDIT_DATE            14   VARCHAR2
    6     SNDR_INFORM_EDIT_PGM_ID          14   VARCHAR2
    7     EAI_INTERFACE_ID                 12   VARCHAR2
    8     INTERFACE_DATA_DIR_ACTUAL_TYPE   1    VARCHAR2
    9     INTERFACE_DATA_OCR_RES_FLAG      1    VARCHAR2
    10    INTERFACE_DATA_SEND_SEQ          5    NUMBER
    11    INTERFACE_DATA_UPD_TP            1    VARCHAR2
    12    INTERFACE_DATA_T_LEN             6    NUMBER
    13    ATTRIBUTE                        24   VARCHAR2
    14    BSC_GW_DATA_ATTR                 10   VARCHAR2
    15    USER DATA
*/
typedef struct mes_data
{
    char tc[8];
    char wcode;
    char oper_flag;
    char fac_cd[2];
    char sndr_date[14];
    char sndr_pgmid[14];
    char if_id[12];
    char if_type;
    char if_flag;
    char if_sendseq[5];
    char if_updtp;
    char if_datalen[6];
    char attribute[24];
    char bsc_attr[10];
	// Change the date struct
    //char data; //Data start point

	/*********************************************************************/

	// Added detail data struct
	// 1 position is 42 byte
	char position_1[13];
	char temperature_1[5];
	char vibe_1[6];
	char noise_1[4];
	char date_1[14];

	char position_2[13];
	char temperature_2[5];
	char vibe_2[6];
	char noise_2[4];
	char date_2[14];

	char position_3[13];
	char temperature_3[5];
	char vibe_3[6];
	char noise_3[4];
	char date_3[14];

	char position_4[13];
	char temperature_4[5];
	char vibe_4[6];
	char noise_4[4];
	char date_4[14];

	char position_5[13];
	char temperature_5[5];
	char vibe_5[6];
	char noise_5[4];
	char date_5[14];

	char position_6[13];
	char temperature_6[5];
	char vibe_6[6];
	char noise_6[4];
	char date_6[14];

	char position_7[13];
	char temperature_7[5];
	char vibe_7[6];
	char noise_7[4];
	char date_7[14];

	char position_8[13];
	char temperature_8[5];
	char vibe_8[6];
	char noise_8[4];
	char date_8[14];

	//char spc_attr[150];
	char spc_attr[66];

	/*********************************************************************/

} Mes_data;

// Change parameter
//char* editSendData(char*,char*);
char* editSendData(char*, char*, char*);
void usage();

int sCount = 1;
char *progname;

// Added the array
char *sArr[16] = { NULL, };

int main(int argc, char *argv[])
{
    int ret;
    int i = 0, flag = 1;
    char *pSendData = NULL;
    char *cfg;
    char ifd[13];
    char *tmp;
    int len = 0;
    int ch, cnt;

	// Added socket
	int sock;
	struct sockaddr_in server;
	char message[1000], server_reply[2000];

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(9999);

	//Connect to remote server
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return 1;
	}

	puts("Connected\n");

	puts("Working Start");

    while(flag)
	{
		// recv ( sock, Message, Buffer_size, 0 )
		if (recv(sock, server_reply, 2000, 0) < 0)
		{
			puts("recv failed");
			break;
		}

		char *ptr = strtok(server_reply, ",");
		int j = 0;
		while (ptr != NULL){
			sArr[j] = ptr;
			j++;
			ptr = strtok(NULL, ",");
		}

		j = 0;
		puts("Received Array : sArr[16]");
		for (j = 0; j <= 15; j++){
			if (j % 2 == 0 )
				printf(" Temp [%d]: ", j);
			else
				printf(" Time [%d]: ", j);
			puts(sArr[j]);
		}

    }
    
    if (pSendData != NULL)
        free(pSendData);

    onClose();

	// Added close socket
	close(sock);

    return 0;
}

// Change the function define because received_data parameter
//char* editSendData(char* msg,char* ifd)
char* editSendData(char* msg, char* ifd, char* recv_data)
{

    time_t now;
    struct tm *stmp;
    char tmpbuf[32];

    time(&now);
    stmp = localtime(&now);    
    strftime(tmpbuf, sizeof(tmpbuf), "%Y%m%d%H%M%S", stmp);

	/*********************************************************************/

	// Added received_data and devide the data frame by the ","
	char *ptr = strtok(recv_data, ",");
	int j = 0;
	while (ptr != NULL){
		sArr[j] = ptr;
		j++;
		ptr = strtok(NULL, ",");
	}

	// !!!!!!!       *sdata (Message_data) memory copy     !!!!!!!!
	Mes_data *sdata = (Mes_data *)msg;

	memcpy(sdata->tc, "KLM25S01", sizeof(sdata->tc));
	sdata->wcode = 'K';
	sdata->oper_flag = '8';
	memcpy(sdata->fac_cd, "4A", sizeof(sdata->fac_cd));
	//memcpy(sdata->sndr_date, tmpbuf, sizeof(sdata->sndr_date));
	memcpy(sdata->sndr_date, "20171114090000", sizeof(sdata->sndr_date));
	memcpy(sdata->sndr_pgmid, "              ", sizeof(sdata->sndr_pgmid));
	//memcpy(sdata->if_id, ifd, sizeof(sdata->if_id));
	memcpy(sdata->if_id, "S25K-1A23-01", sizeof(sdata->if_id));
	sdata->if_type = '2';
	sdata->if_flag = '1';
	memcpy(sdata->if_sendseq, "00000", sizeof(sdata->if_sendseq));
	sdata->if_updtp = '1';
	memcpy(sdata->if_datalen, "000500", sizeof(sdata->if_datalen));
	memcpy(sdata->attribute, "                        ", sizeof(sdata->attribute));
	memcpy(sdata->bsc_attr, "          ", sizeof(sdata->bsc_attr));

	/*********************************************************************/

	// Position is explaned the Accel Sheet 1  ( A101 ~ A212 )

	/*********************************************************************/

	memcpy(sdata->position_1, "A177         ", sizeof(sdata->position_1));
	memcpy(sdata->temperature_1, sArr[0], sizeof(sdata->temperature_1));
	memcpy(sdata->vibe_1, "000000", sizeof(sdata->vibe_1));
	memcpy(sdata->noise_1, "0000", sizeof(sdata->noise_1));
	memcpy(sdata->date_1, sArr[1], sizeof(sdata->date_1));

	memcpy(sdata->position_2, "A178         ", sizeof(sdata->position_2));
	memcpy(sdata->temperature_2, sArr[2], sizeof(sdata->temperature_2));
	memcpy(sdata->vibe_2, "000000", sizeof(sdata->vibe_2));
	memcpy(sdata->noise_2, "0000", sizeof(sdata->noise_2));
	memcpy(sdata->date_2, sArr[3], sizeof(sdata->date_2));

	memcpy(sdata->position_3, "A179         ", sizeof(sdata->position_3));
	memcpy(sdata->temperature_3, sArr[4], sizeof(sdata->temperature_3));
	memcpy(sdata->vibe_3, "000000", sizeof(sdata->vibe_3));
	memcpy(sdata->noise_3, "0000", sizeof(sdata->noise_3));
	memcpy(sdata->date_3, sArr[5], sizeof(sdata->date_3));

	memcpy(sdata->position_4, "A180         ", sizeof(sdata->position_4));
	memcpy(sdata->temperature_4, sArr[6], sizeof(sdata->temperature_4));
	memcpy(sdata->vibe_4, "000000", sizeof(sdata->vibe_4));
	memcpy(sdata->noise_4, "0000", sizeof(sdata->noise_4));
	memcpy(sdata->date_4, sArr[7], sizeof(sdata->date_4));

	memcpy(sdata->position_5, "A181         ", sizeof(sdata->position_5));
	memcpy(sdata->temperature_5, sArr[8], sizeof(sdata->temperature_5));
	memcpy(sdata->vibe_5, "000000", sizeof(sdata->vibe_5));
	memcpy(sdata->noise_5, "0000", sizeof(sdata->noise_5));
	memcpy(sdata->date_5, sArr[9], sizeof(sdata->date_5));

	memcpy(sdata->position_6, "A182         ", sizeof(sdata->position_6));
	memcpy(sdata->temperature_6, sArr[10], sizeof(sdata->temperature_6));
	memcpy(sdata->vibe_6, "000000", sizeof(sdata->vibe_6));
	memcpy(sdata->noise_6, "0000", sizeof(sdata->noise_6));
	memcpy(sdata->date_6, sArr[11], sizeof(sdata->date_6));

	memcpy(sdata->position_7, "A183         ", sizeof(sdata->position_7));
	memcpy(sdata->temperature_7, sArr[12], sizeof(sdata->temperature_7));
	memcpy(sdata->vibe_7, "000000", sizeof(sdata->vibe_7));
	memcpy(sdata->noise_7, "0000", sizeof(sdata->noise_7));
	memcpy(sdata->date_7, sArr[13], sizeof(sdata->date_7));

	memcpy(sdata->position_8, "A184         ", sizeof(sdata->position_8));
	memcpy(sdata->temperature_8, sArr[14], sizeof(sdata->temperature_8));
	memcpy(sdata->vibe_8, "000000", sizeof(sdata->vibe_8));
	memcpy(sdata->noise_8, "0000", sizeof(sdata->noise_8));
	memcpy(sdata->date_8, sArr[15], sizeof(sdata->date_8));

	//memcpy(sdata->spc_attr, "                                                                                                                                                      ", sizeof(sdata->spc_attr));
	memcpy(sdata->spc_attr, "                                                                  ", sizeof(sdata->spc_attr));
	/*********************************************************************/

    return (char*)sdata;
}

int handleData(char *data, char *ifd, int ret) { }
