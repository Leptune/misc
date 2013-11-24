#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DATA "DATA\r\n"
#define QUIT "QUIT\r\n"

#define IN_BUF_SIZE 4096
#define OUT_BUF_SIZE (((IN_BUF_SIZE + 2) / 3) * 4 + 1)

#define HOST_IP "smtp.exmail.qq.com" /* 邮件服务器 */
#define FROM_IP "your email"         /* 改为你自己的邮箱 */
#define TO_IP   "her email"          /* 改为要发往的邮箱 */
#define SUBJECT "email test"         /* 邮件标题 */
#define USER    FROM_IP              /* 用户名(一般和邮箱名一样) */
#define PASSWD  "your password"      /* 改为你的密码 */

#define HTML_FILE "test.html"
#define DOC_FILE  "test.doc"

int g_sock;

void send_socket(const char *out_buf, int size)
{
    int count = 0;
    int ret = 0;
    int len = 0;

    count = 0;
    len = (-1 == size) ? strlen(out_buf) : size;
    while (count < len)
    {
        ret = write(g_sock, out_buf + count, len - count);
        if (ret <= 0)
        {
            perror("write");
            exit(-1);
        }
        count += ret;
    }
}

void read_socket()
{
    int len = 0;
    char buf[BUFSIZ + 1] = "\0";

    len = read(g_sock, buf, BUFSIZ);
    buf[len] = '\0';
    printf("%s\n", buf);
}


int base64_encode(const char *in, int ilen, char *out, int olen)
{/*{{{*/
    int convlen = 0;
    const char base64tab[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    convlen = ((ilen + 2) / 3) * 4;
    if (convlen >= olen)
    {
        printf("outlen should be ((%d+2)/3)*4+1=%d,but it is %d\n", ilen, convlen + 1, olen);
        return -1;
    }

    /** we don't need to check olen anymore */
    while (ilen >= 3)
    {
        out[0] = base64tab[(((unsigned char)in[0]) >> 2)];
        out[1] = base64tab[(((unsigned char)in[0] & 0x03) << 4) | (((unsigned char)in[1]) >> 4)];
        out[2] = base64tab[(((unsigned char)in[1] & 0x0F) << 2) | (((unsigned char)in[2]) >> 6)];
        out[3] = base64tab[(((unsigned char)in[2] & 0x3F))];
        out += 4;
        in += 3;
        ilen -= 3;
    }

    if (!ilen)
    {
        out[0] = '\0';
    } else
    {
        out[0] = base64tab[((unsigned char)in[0]) >> 2];
        if (ilen == 1)
        {
            out[1] = base64tab[((unsigned char)in[0] & 0x03) << 4];
            out[2] = '=';
        } else
        {
            out[1] = base64tab[(((unsigned char)in[0] & 0x03) << 4) |
                (((unsigned char)in[1]) >> 4)];
            out[2] = base64tab[((unsigned char)in[1] & 0x0F) << 2];
        }
        out[3] = '=';
        out[4] = '\0';
    }

    return convlen;
}/*}}}*/

int main(int argc, const char* argv[])
{
    struct sockaddr_in server;
    struct hostent *hp  = NULL;
    FILE *fp = NULL;
    long length = 0L;
    long count = 0L;
    int ret = 0;
    int tmp_ret = 0;
    char in_buf[IN_BUF_SIZE+ 1] = "\0";
    char out_buf[OUT_BUF_SIZE + 1] = "\0";


    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == g_sock)
    {
        perror("socket");
        return -1;
    }

    hp = gethostbyname(HOST_IP);
    if (NULL == hp)
    {
        perror("gethostbyname");
        return -1;
    }

    server.sin_family = AF_INET;
    memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);
    server.sin_port=htons(25);
    if (connect(g_sock, (struct sockaddr *)&server, sizeof (server)) < 0)
    {
        perror("connect");
        return -1;
    }

    /* login email */
    read_socket();
    send_socket("HELO ", -1);
    send_socket(FROM_IP, -1);
    send_socket("\r\n", -1);
    read_socket();
    send_socket("AUTH LOGIN\r\n", -1);
    read_socket();
    base64_encode(USER, strlen(USER), out_buf, OUT_BUF_SIZE);
    send_socket(out_buf, -1); //用户名的base64编码
    send_socket("\r\n", -1);
    read_socket();
    base64_encode(PASSWD, strlen(PASSWD), out_buf, OUT_BUF_SIZE);
    send_socket(out_buf, -1);//密码的base64编码
    send_socket("\r\n", -1);
    read_socket();

    /* mail head */
    send_socket("MAIL FROM <", -1);
    send_socket(FROM_IP, -1);
    send_socket(">\r\n", -1);
    read_socket();
    send_socket("RCPT TO <", -1);
    send_socket(TO_IP, -1);
    send_socket(">\r\n", -1);
    read_socket();

    /* mail info */
    send_socket(DATA, -1);
    read_socket();
    send_socket("FROM:<", -1);
    send_socket(FROM_IP, -1);
    send_socket(">\r\n", -1);
    send_socket("TO:<\r\n", -1);
    send_socket(TO_IP, -1);
    send_socket(">\r\n", -1);
    send_socket("SUBJECT:", -1);
    send_socket(SUBJECT, -1);
    send_socket("\r\n", -1);
    send_socket("MIME-VERSION: 1.0\r\n", -1);
    send_socket("CONTENT-TYPE: MULTIPART/MIXED; BOUNDARY=\"#BOUNDARY#\"\r\n\r\n", -1);

    /* 正文 */
    send_socket("--#BOUNDARY#\r\n", -1);
    send_socket("CONTENT-TYPE: TEXT/HTML\r\n\r\n", -1);
    //send_socket("Date: Fri, 8 Jan 2010 16:12:30\r\n", -1);
    //send_socket("Content-transfer-encoding:base64\r\n", -1);

    fp = fopen(HTML_FILE, "rb");
    if (NULL == fp)
    {
        perror("fopen");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    count = 0L;
    while (count < length)
    {
        ret = fread(in_buf, 1, IN_BUF_SIZE, fp);
        if (ret <= 0)
        {
            perror("fread");
            return -1;
        }
        send_socket(in_buf, ret);
        count += ret;
    }
    fclose(fp);
    send_socket("\r\n\r\n", -1);

    send_socket("--#BOUNDARY#\r\n", -1);
    send_socket("CONTENT-TYPE: APPLICATION/MSWORD; NAME=", -1); /* msword为word类型的 */
    send_socket(DOC_FILE, -1);
    send_socket("\r\n", -1);
    send_socket("CONTENT-DISPOSITION: ATTACHMENT; FILENAME=", -1); /* attachment表示只有当用户点击时文档才会播放(即预览) */
    send_socket(DOC_FILE, -1);
    send_socket("\r\n", -1);
    send_socket("CONTENT-TRANSFER-ENCODING:BASE64\r\n\r\n", -1); /* 附件传输要用base64编码 */

    fp = fopen(DOC_FILE, "rb");
    if (NULL == fp)
    {
        perror("fopen");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    count = 0L;
    while (count < length)
    {
        ret = fread(in_buf, 1, IN_BUF_SIZE, fp);
        tmp_ret = base64_encode(in_buf, ret, out_buf, OUT_BUF_SIZE);
        if (-1 == tmp_ret)
        {
            perror("base64_encode");
            return -1;
        }
        send_socket(out_buf, tmp_ret);
        count += ret;
    }
    fclose(fp);

    send_socket("\r\n.\r\n", -1);
    read_socket();
    send_socket(QUIT, -1);
    read_socket();

    close(g_sock);
    return 0;
}
