#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <iostream>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/uio.h>
#include <string.h>
#include "locker.h"

class http_conn {
public:
    http_conn() {};
    ~http_conn() {};

public:
    static const int FILENAME_LEN       = 200;       // 文件名的最大长度
    static const int READ_BUFFER_SIZE   = 2048;      // 读缓冲区大小
    static const int WRITE_BUFFER_SIZE  = 1024;      // 写缓冲区大小

    /**
     * HTTP请求方法
     */
    enum METHOD { GET = 0,      // 暂时只支持 GET
                  POST, 
                  HEAD, 
                  PUT, 
                  DELETE, 
                  TRACE, 
                  OPTIONS, 
                  CONNECT };

    /**
     * 有限状态机
     * 解析客户端请求时，主状态机的状态
     */
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0,   // 当前正在分析请求行
                       CHECK_STATE_HEADER,            // 当前正在分析头部字段
                       CHECK_STATE_CONTENT };         // 当前正在解析请求体
    
    /*
        从状态机的三种可能状态（行的读取状态）
    */
    enum LINE_STATUS { LINE_OK = 0,                   // 读取到一个完整的行
                       LINE_BAD,                      // 行出错
                       LINE_OPEN };                   // 行数据尚且不完整
    
    /*
        服务器处理HTTP请求的可能结果，报文解析的结果
    */
    enum HTTP_CODE { NO_REQUEST,            // 请求不完整，需要继续读取客户数据
                     GET_REQUEST,           // 表示获得了一个完成的客户请求
                     BAD_REQUEST,           // 表示客户请求语法错误
                     NO_RESOURCE,           // 表示服务器没有资源
                     FORBIDDEN_REQUEST,     // 表示客户对资源没有足够的访问权限
                     FILE_REQUEST,          // 文件请求,获取文件成功
                     INTERNAL_ERROR,        // 表示服务器内部错误
                     CLOSED_CONNECTION };   // 表示客户端已经关闭连接了

public:
    void init(int sockfd, const sockaddr_in & addr);    // 初始化新接收的连接
    void close_conn();  // 关闭连接
    void process();     // 处理客户端请求
    bool read();        // 非阻塞的读
    bool write();       // 非阻塞的写

private:
    void init();                                // 初始化连接其他的信息
    HTTP_CODE process_read();                   // 解析HTTP请求
    bool process_write( HTTP_CODE ret );        // 只是判断有没有完整读入

    /*  被process_read()调用以分析HTTP请求的请求首行、请求头和请求体  */
    HTTP_CODE parse_request_line(char* text);   // 解析请求首行
    HTTP_CODE parse_headers(char* text);        // 解析请求头
    HTTP_CODE parse_content(char* text);        // 解析请求体
    HTTP_CODE do_request();
    char* get_line() {
        return m_read_buf + m_start_line;
    }
    LINE_STATUS parse_line();                   // 从状态机，解析具体的一行

    /*  被process_write()调用以写上HTTP回应  */
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_content_type();
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;                   // 所有socket上的事件都被注册到同一个epoll内核事件中，所以设置成静态的
    static int m_user_count;                // 统计用户的数量

private:
    int m_sockfd;                           // 该HTTP连接的socket
    sockaddr_in m_address;                  // 通信的socket地址
    char m_read_buf[READ_BUFFER_SIZE];      // 读缓冲区
    int m_read_idx;                       // 读缓冲区中已经读入的客户端数据的最后一个字节的下一个位置
    int m_checked_idx;                      // 当前正在分析的字符在读缓冲区的位置
    int m_start_line;                       // 当前正在解析的行的起始位置

    CHECK_STATE m_check_state;              // 主状态机当前所处的状态
    METHOD m_method;                        // 请求方法
    char* m_url;                            // 客户请求的目标文件的文件名
    char* m_version;                        // HTTP版本
    char* m_host;                           // 主机名
    int m_content_length;                   // HTTP请求的消息总长度
    bool m_linger;                          // HTTP请求是否保持连接
    char m_real_file[ FILENAME_LEN ];       // 客户请求的目标文件的完整路径，
                                            // 其内容等于 doc_root + m_url, doc_root是网站根目录

    char m_write_buf[ WRITE_BUFFER_SIZE ];  // 写缓冲区
    int m_write_idx;                        // 写缓冲区中待发送的字节数
    char* m_file_address;                   // 户请求的目标文件被mmap到内存中的起始位置
    struct stat m_file_stat;                // 目标文件的状态。通过它我们可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息
    struct iovec m_iv[2];                   // 我们将采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示被写内存块的数量。
    int m_iv_count;

    
};

#endif