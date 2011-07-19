#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sql.h>
#include <sqlext.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <windows.h>

#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define ABS(x)   ( (x) < 0 ? (-(x)) : (x) )


typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;

typedef struct
{
        short       version;                        
        short       sqldaid[8];                     
        long        sqldabc;                        
        short       sqln;                           
        short       sqld;                           
        char sqlvar[1];                      
} XSQLDA;


// SQL_ERROR �̏��\����
typedef struct {
    char       msg[512+32];          // �����͖���
    BOOL       is_server;            // �G���[�̏o��
    int        native;               // �l�C�e�B�u�G���[�R�[�h
} IB_ERRINFO;



//
// XSQLDA �ƃA�v���P�[�V�����f�[�^��Ƃ̌������
//
//   �Eparam_type �́A�o�̓p�����[�^�Ŏg�����H
//
typedef struct {
    SQLUSMALLINT param_type;        // �p�����[�^�^�C�v�i���g�p�j
    SQLSMALLINT  odbc_type;         // ODBC SQL �^�C�v
    SQLUINTEGER  odbc_precision;    // ODBC Precision
    SQLSMALLINT  odbc_scale;        // ODBC Scale
    SQLSMALLINT  odbc_nullable;     // ODBC null ��
    BOOL         is_bind;           // C �ϐ��o�C���h�ς݂�
    SQLSMALLINT  ctype;             // C ����^�C�v
    void         *data;             // C �ϐ��f�[�^
    SQLINTEGER   *pind;             // �f�[�^�������NULL(��):���o�͂Ŏg�p
    SDWORD       limit;             // C �ϐ��Ɋi�[���̌��x�i�o�́j
    BOOL         is_at_exec;        // ���s���p�����[�^
    BOOL         is_at_exec_end;    // ���s���p�����[�^��put����
    long         data_pos;          // CHUNK �����i���o�́j�̎��̃o�C�g�ʒu
} IB_XCHG;

//
//    HSTMT �̈�
//
typedef struct {
    void          *next_stmt;     // next stmt ( list �\�� )
    void          *pare_dbc;      // �e�� dbc
    char          *data;          // data area (use sqlda)
    char          *in_data;       // parameter data area (use in_sqlda)
    BOOL          prepared;       // already prepared
    BOOL          is_result;      // select or other(insert/update/delete...)
    XSQLDA        *in_sqlda;      // use input parameter
    XSQLDA        *sqlda;         // use fetch data 
    IB_XCHG       *xchgs;         // output    sqlvar <-> app variables
    IB_XCHG       *in_xchgs;      // parameter sqlvar <-> app variables
    int           in_xchgs_size;  // number of in_chages[]
    IB_ERRINFO    err;            // stmt ���x���̃G���[�\����
    int           catalog_opt;    // CATALOG Option(1=tables,2=columns...)
    ushort        isc_stmt_type;  // SQL �X�e�[�g�����g�̎��
    int           need_data_no;   // ���ݏ���������s���p�����[�^
    BOOL          is_need_data;   // ���s���p�����[�^������
    char          cursor_name[32];// �J�[�\����
    BOOL          is_cancel;      // SQLCancel ���쒆
} IB_STMT;

//
//    HDBC �̈�
//
typedef struct {
    IB_STMT       *stmt;          // �ŏ��� hstmt �\���ւ̎Q��
    BOOL          is_open;
    BOOL          is_noautocommit; // AUTOCOMMIT ���[�h
    BOOL          is_trans;       // �g�����U�N�V������(htrans �ł͔��f�o���Ȃ�)
    char          *dpb;           // DPB buffer
    int           dpb_size;
    long          sqlcode;        // ���g�p
    long          odbc_cursors;   // SQLConnectOption
    void          *next_dbc;      // next dbc ( list �\�� )
    void          *pare_env;      // �e�� henv
    ushort        dialect;        // InterBase �̌݊����x��
    int           isolation_level;  // �ݒ肷�� ODBC �̕������x��
    int           current_isolation;// ���� ODBC �̕������x��
    BOOL          is_nowait;      // NO WAIT
    IB_ERRINFO    err;            // dbc ���x���̃G���[�\����
} IB_DBC;

//
//    HENV �̈�
//
typedef struct {
    IB_DBC        *dbc;          // �ŏ��� hdbc �i list �\�� �j
    IB_ERRINFO    err;           // env ���x���̃G���[�\����
} IB_ENV;


//
//   ���W�X�g�����
//
typedef struct {
    char	driver[ MAX_PATH ];
    char	database[ MAX_PATH ];
    char	host[ MAX_PATH ];
	char	framed[ MAX_PATH ];
	char	logfile[ MAX_PATH ];
    short	port;
	int		debug_level;
} REGISTER_INFO;


/*
�v���g�^�C�v�錾
*/
void debuglog( char *fmt, ... );
int DBExecute(int sockfd,char *query);
int DBFetchOne(int sockfd,char *ou);
int DBFetchN(int sockfd,char *ou);
int DBConnect(char *svr,short port);
int DBDisconnect();
int tcp_connect(char *server_name,short server_port);
int tcp_select(int type, int tm);
int tcp_write(char *buf, int buf_len);
int tcp_read(char *buf, int buf_len);
int tcp_disconnect();
char *utl_strSepValue(char *s1,char sep1,int idx1);
char *utl_strCut(char *in, char c);
int utl_GetRegistory();
int func_init(char *func);


/*
�O���[�o���ϐ�
*/
extern int sockfd;
extern int errflg;
extern REGISTER_INFO reginfo; 


