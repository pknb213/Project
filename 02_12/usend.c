/*********************************************************************/
/*	2018_01_10   usend.c    code									 */												
/*	Posco Cloude - Socket Communication								 */												
/*	by CYJ	v.0214												 */
/*********************************************************************/
#include <EAIAdapter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h> /* strftime() */
#include <unistd.h> /* sleep() */

/*********************************************************************/

// Added new header
#include <sys/socket.h>		// socket
#include <arpa/inet.h>		// inet_addr

// Added new macro define
#define _CRT_SECURE_NO_WARNINGS

/****************************  Value  ***********************************/
//  Added the IFD,URL path url
const char* _IFD = "S25K-1A01-01";
const char* _URL = "/home/eai2/uCUBE/config/cfg/KLFRM01L_TS01_YJ.cfg";
const char* _Position1 = "A177         ";
const char* _Position2 = "A178         ";
const char* _Position3 = "A179         ";
const char* _Position4 = "A180         ";
const char* _Position5 = "A181         ";
const char* _Position6 = "A182         ";
const char* _Position7 = "A183         ";
const char* _Position8 = "A184         ";

/*********************************************************************/


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
	// Header 100 byte leng
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

	char spc_attr[66];

	/*********************************************************************/

} Mes_data;

// Change parameter
//char* editSendData(char*,char*);
char* editSendData(char*, char*, char*);

// Not using 
/*
void usage();

int sCount = 1;
char *progname;
*/

/*********************************************************************/

// Added the array
char *sArr[16] = { NULL, };

/*********************************************************************/

int main(int argc, char *argv[])
{
    int ret;
    int i = 0, flag = 1;
    char *pSendData = NULL;
    //char *cfg;
    char ifd[13];
    //char *tmp;
    int len = 0;
	int length = 0;
    //int ch, cnt;

	/*********************************************************************
	
	// Not use the macro 
#ifdef __VMS
    if ((progname = strrchr(argv[0], ']')) != NULL)
        progname++;
    else
        progname = argv[0];

    tmp = strrchr(progname,';');
    *tmp='\0';
#else
    if ((progname = strrchr(argv[0], '/')) != NULL)
        progname++;
    else
        progname = argv[0];
#endif

    if(argc != 2) {
        usage();
    }
    
    cfg = argv[1];
    len = strlen(cfg);
    if(127 < len) {
        usage();
    }

	********************************************************************/
	/*********************************************************************/

	// Added socket

	/*
	Create socket : int socket ( int domain, int type, int protocol )
	domain		->	RF_INET : IPv4, AF_INET : TCP,UDP
	type		->	SOCKET_STREAM, SOCK_DGRAM, SOCK_RAW
	protocol	->	Basic : 0, TCP : IPPROTO_TCP, UDP : IPPROTO_UDP

	return		-> success : socket handle, fail : INVALD_SOCKET

	1st	:	socket create (called the socket function)
	2st	:	IP address , port number mapping (called the bind function)
	3st	:	make the enable connected state (called the listen function)
	4st	:	accept the connect request (called accept fucntion)

	int send(int s, const void *msg, size_t len, int flags)
		explain		:	sent the data to a linked server or client
		header		:	<sys/types.h>, <sys/socket.h>
		parameter	:	
			s		:	socket descriptor
			msg		:	data
			len		:	length
			flag	:	MSG_DONTWAIT, MSG_NOSIGNAL

		return		
			-1		:	fail
			another	:	number of trasmit byte

	int recv(int s, void *buf, size_t len, int flags)
		explain		:	send a request to the send() to get a response.
		return		
			0		:	Not value
			another	:	respond size
	*/

	int sock;
	struct sockaddr_in server;
	char message[1000], server_reply[2000];

	//Create socket
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


	/*********************************************************************/

	// !!!!!!!		Change the initalize cfg file		!!!!!!!!
    //ret = initialize(cfg);
	ret = initialize(_URL);
    if (ret == 0) {
        printf ("[UAP]INITIALIZE SUCCESS.\n");
    }
    else {
        printf ("[UAP]INITIALIZE FAILED.[%d]\n", ret);
	return(0);
    }

    pSendData = (char *)malloc(1024); //more the MesHeadLength + DataLength

    while(flag)
	{
		/*********************************************************************/

		// Added the received socket reply
		if (recv(sock, server_reply, 2000, 0) < 0)
		{
			puts("recv failed");
			break;
		}
		//puts("Server reply :");
		//puts(server_reply);

		/*********************************************************************/

        //cnt = 0;
		// !!!!!!!!		Change the ifd		!!!!!!!!
        //memset(ifd, '\0', sizeof(ifd));
		memset(ifd, _IFD, sizeof(ifd));
        printf("Input Interface ID(eXit:x): ");

		/*********************************************************************

		// Not use the ch
        while((ch = fgetc(stdin)) != '\n') 
        {
            if(ch==' ') 
                continue;
            if(cnt > 12) 
                continue;
            ifd[cnt++] = ch;
        }

		if(ifd[0]=='x' || ifd[0]=='X') 
				break;

		*********************************************************************/

		/*********************************************************************/

		// !!!!!!!!!		Added the memcpy		!!!!!!!!
		memcpy(ifd, _IFD, sizeof(ifd));

		/*********************************************************************/

		len = strlen(ifd);
        if(len != 12) {
            printf("[%s]Check Interface ID!!!!\n",ifd);
            continue;
        }

		// !!!!!!!!		Change the pSendData		!!!!!!!!!
        //pSendData = editSendData(pSendData, ifd);
		pSendData = editSendData(pSendData, _IFD, server_reply);
        printf("[%s]\n",pSendData);
        /*send Transaction Data to EAI */
		// !!!!!!!!		Change the ret			!!!!!!!!
        //ret = sendTC(ifd, pSendData, 0);
		ret = sendTC(_IFD, pSendData, 0);
        /*ret = sendFile("TEST-0001-11", sendFilePath, sendFileName);*/
		length = strlen(pSendData);
		printf("Ret : %d			Length : %d\n", ret, length);

        switch (ret)
        {
            case 0  : 
                printf ("sendTC success.length:%d,Data [%.40s]\n", strlen(pSendData), pSendData);
                /* printf ("sendFile success. count::%d,Data [%s]\n", sCount, "C:\\EAI\\zpss\\fileout\\sendfile3.txt"); */
                break;
            case 1  : 
                printf ("sendTC saved to Local Queue...Data [%.40s]\n", pSendData);
                break;
            case -1 : 
                printf ("sendTC failed...Invalied Interface ID. Check Interface ID\n");
            flag = 0;
                break;
            case -2 :
                printf ("sendTC failed...Maybe sendTC is non-persistent...\n");
                sleep(5);
                break;
            case -3 :  
                printf ("sendFile failed...Maybe File does not exsist...\n");
                sleep(5);
                break;
            case -8 :
                printf ("please call initialize()...\n");
                break;
            default :
                printf ("sendTC failed...Error code::%d\n", ret);
                flag = 0;
                break;
        }
    }
    
    if (pSendData != NULL)
        free(pSendData);

    onClose();
	/*********************************************************************/

	// Added close socket
	close(sock);

	/*********************************************************************/

    return 0;
}

// Change the function define because received_data parameter
//char* editSendData(char* msg,char* ifd)
char* editSendData(char* msg, char* ifd, char* recv_data)
{

    time_t now;
    struct tm *stmp;
    char tmpbuf[32];

	/*********************************************************************

	// Not use wcode, smpthead, smpdata etc parameter
    int len = 0;
    char *smphead="KEAITESTK                               TEST-0001-01        000963POSMAST                           ";
    char *smpdata="               ñéÏÐÇÑ±¹¨Ï¨ç¢â¢Î¡É¢Þ";
    Mes_data *sdata = (Mes_data *)msg;

    memcpy(msg, smphead, mes_header_len);

    switch (*(ifd+3)) { 
    case 'P':
        memcpy(sdata->tc,"PEAITEST",sizeof(sdata->tc));
        sdata->wcode = 'P';
        break;
    case 'K':
        memcpy(sdata->tc,"KEAITEST",sizeof(sdata->tc));
        sdata->wcode = 'K';
        break;
    default:
        memcpy(sdata->tc,"PEAITEST",sizeof(sdata->tc));
        sdata->wcode = 'Z';
        break;
    }

	*********************************************************************/

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
	memcpy(sdata->sndr_date, tmpbuf, sizeof(sdata->sndr_date));
	//memcpy(sdata->sndr_date, "20171114090000", sizeof(sdata->sndr_date));
	memcpy(sdata->sndr_pgmid, "              ", sizeof(sdata->sndr_pgmid));
	//memcpy(sdata->if_id, ifd, sizeof(sdata->if_id));
	memcpy(sdata->if_id, _IFD, sizeof(sdata->if_id));
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

	memcpy(sdata->position_1, _Position1, sizeof(sdata->position_1));
	memcpy(sdata->temperature_1, sArr[0], sizeof(sdata->temperature_1));
	memcpy(sdata->vibe_1, "000000", sizeof(sdata->vibe_1));
	memcpy(sdata->noise_1, "0000", sizeof(sdata->noise_1));
	memcpy(sdata->date_1, sArr[1], sizeof(sdata->date_1));

	memcpy(sdata->position_2, _Position2, sizeof(sdata->position_2));
	memcpy(sdata->temperature_2, sArr[2], sizeof(sdata->temperature_2));
	memcpy(sdata->vibe_2, "000000", sizeof(sdata->vibe_2));
	memcpy(sdata->noise_2, "0000", sizeof(sdata->noise_2));
	memcpy(sdata->date_2, sArr[3], sizeof(sdata->date_2));

	memcpy(sdata->position_3, _Position3, sizeof(sdata->position_3));
	memcpy(sdata->temperature_3, sArr[4], sizeof(sdata->temperature_3));
	memcpy(sdata->vibe_3, "000000", sizeof(sdata->vibe_3));
	memcpy(sdata->noise_3, "0000", sizeof(sdata->noise_3));
	memcpy(sdata->date_3, sArr[5], sizeof(sdata->date_3));

	memcpy(sdata->position_4, _Position4, sizeof(sdata->position_4));
	memcpy(sdata->temperature_4, sArr[6], sizeof(sdata->temperature_4));
	memcpy(sdata->vibe_4, "000000", sizeof(sdata->vibe_4));
	memcpy(sdata->noise_4, "0000", sizeof(sdata->noise_4));
	memcpy(sdata->date_4, sArr[7], sizeof(sdata->date_4));

	memcpy(sdata->position_5, _Position5, sizeof(sdata->position_5));
	memcpy(sdata->temperature_5, sArr[8], sizeof(sdata->temperature_5));
	memcpy(sdata->vibe_5, "000000", sizeof(sdata->vibe_5));
	memcpy(sdata->noise_5, "0000", sizeof(sdata->noise_5));
	memcpy(sdata->date_5, sArr[9], sizeof(sdata->date_5));

	memcpy(sdata->position_6, _Position6, sizeof(sdata->position_6));
	memcpy(sdata->temperature_6, sArr[10], sizeof(sdata->temperature_6));
	memcpy(sdata->vibe_6, "000000", sizeof(sdata->vibe_6));
	memcpy(sdata->noise_6, "0000", sizeof(sdata->noise_6));
	memcpy(sdata->date_6, sArr[11], sizeof(sdata->date_6));

	memcpy(sdata->position_7, _Position7, sizeof(sdata->position_7));
	memcpy(sdata->temperature_7, sArr[12], sizeof(sdata->temperature_7));
	memcpy(sdata->vibe_7, "000000", sizeof(sdata->vibe_7));
	memcpy(sdata->noise_7, "0000", sizeof(sdata->noise_7));
	memcpy(sdata->date_7, sArr[13], sizeof(sdata->date_7));

	memcpy(sdata->position_8, _Position8, sizeof(sdata->position_8));
	memcpy(sdata->temperature_8, sArr[14], sizeof(sdata->temperature_8));
	memcpy(sdata->vibe_8, "000000", sizeof(sdata->vibe_8));
	memcpy(sdata->noise_8, "0000", sizeof(sdata->noise_8));
	memcpy(sdata->date_8, sArr[15], sizeof(sdata->date_8));

	memcpy(sdata->spc_attr, "                                                                  ", sizeof(sdata->spc_attr));

	/*********************************************************************/

	/*********************************************************************

	// Not use the code about len parameter
    memcpy(sdata->sndr_date, tmpbuf,sizeof(sdata->sndr_date));
    memcpy(sdata->if_id, ifd, sizeof(sdata->if_id));
    len=strlen(progname);
    if(len > 14) { len=14; }
    memcpy(sdata->sndr_pgmid, progname,len);
    sprintf(tmpbuf,"%05d",sCount);
    memcpy(sdata->if_sendseq, tmpbuf, sizeof(sdata->if_sendseq));
    sprintf(tmpbuf,"%06d", strlen(smpdata) + mes_header_len);
    memcpy(sdata->if_datalen, tmpbuf, sizeof(sdata->if_datalen));
    strcpy(&(sdata->data), smpdata);

    sCount++;

	*********************************************************************/

    return (char*)sdata;
}

/*********************************************************************

// Not use the function
void usage()
{
    printf("\n\nUsage:\n");
    printf("\t%s cfgfile\n\n",progname);
    exit(0);
}

/*********************************************************************/

int handleData(char *data, char *ifd, int ret) { }
