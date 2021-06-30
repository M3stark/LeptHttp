#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <libgen.h>
#include <signal.h>

#include "locker.h"
#include "threadpool.h"
#include "http_conn.h"


#define MAX_FD 65535            // 最大文件描述符个数
#define MAX_EVENT_NUMBER 10000  // 监听最大的事件数量


// 添加信号捕捉
void addsig(int sig, void(handler)(int)) {
    struct sigaction sa;
    std::memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask); // 临时阻塞信号集
    sigaction(sig, &sa, nullptr);
}

// 添加文件描述符到epoll中
extern int addfd(int epollfd, int fd, bool one_shot);

// epoll中删除文件描述符
extern void removefd(int epollfd, int fd);

// 修改文件描述符
extern void modfd(int epollfd, int fd, int ev);

int main(int argc, char* argv[]) {

    if(argc <= 1) {
        // 最少要传递一个端口号
        std::cout << "Usage: " << basename(argv[0]) << " port_name" << std::endl;
        exit(-1);
    }

    // 获取端口号
    int port = atoi(argv[1]);

    // 对SIGPIE信号作处理
    addsig(SIGPIPE, SIG_IGN);

    // 创建线程池
    threadpool<http_conn> * pool = nullptr;
    try {
        pool = new threadpool<http_conn>;
    } catch(...) {
        return 1;
    }

    // 创建一个数组，用于保存所有的客户端信息
    http_conn * users = new http_conn[ MAX_FD ];

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);

    // 设置端口复用（设置在服务器绑定端口之前！
    int reuse = 1;  // 1. 可以复用 0. 不可以复用
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 绑定
    struct sockaddr_in address;
    address.sin_family = PF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port); // port 通过输入参数获得
    bind(listenfd, (struct sockaddr*) & address, sizeof(address));

    // 监听
    listen(listenfd, 5);

    // 使用epoll检测事件发生
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);

    // 将监听的文件描述符添加到epoll
    addfd(epollfd, listenfd, false);    // 监听的fd 不需要设置 ONESHOT
    http_conn::m_epollfd = epollfd;

    while(true) {
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if( (num < 0) && (errno != EINTR)) {
            std::cout << "epoll failure!" << std::endl;
            break;
        }

        // 循环遍历事件数组
        for(int i = 0; i < num; ++i) {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd) {
                // 有客户端连接进来
                struct sockaddr_in client_address;
                socklen_t client_addlen = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)& client_address, &client_addlen);

                if(http_conn::m_user_count >= MAX_FD) {
                    // 目前连接数满了
                    // 给客户端写一个信息，服务器正忙
                    close(connfd);
                    continue;
                }

                // 将新的客户的数据初始化，放在数组中
                users[connfd].init(connfd, client_address);
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 对方异常断开或者错误等事件
                users[sockfd].close_conn();
            }
            else if(events[i].events & EPOLLIN) {
                if(users[sockfd].read()) {
                    // 一次性读完所有数据
                    pool->append(users + sockfd);
                }
                else {
                    users[sockfd].close_conn();
                }
            }
            else if(events[i].events & EPOLLOUT) {
                if( !users[sockfd].write()) {
                    users[sockfd].close_conn();
                }
            }
        }

    }

    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;

    return 0;
}