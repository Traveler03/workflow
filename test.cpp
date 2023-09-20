// 头文件
#include <fstream>
#include <netinet/in.h> // sockaddr_in, htonl, htons
#include <sys/socket.h> // socket, setsockopt, bind, listen, accept, recv, send
#include <cstdio>       // perror
#include <cstdlib>      // exit, EXIT_FAILURE
#include <cstring>      // bzero, strncpy
#include <unistd.h>     // close
#include <sys/wait.h>   // waitpid
#include <ctime>        // time
#include <iostream>     // std::cout, std::cerr
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string
#include <fcntl.h>      // flock


// 宏定义
#define SERV_PORT 3333     // 服务端端口号
#define LISTEN_MAX_COUNT 1 // 所监听的最大连接数
#define BUFF_SIZE 512      // 传递消息缓冲区大小

// 获取当前的时间 ctime非线程安全 如果需要修改不能使用ctime函数
std::string get_time() {
    time_t current_time;
    char* c_time_string;
    c_time_string = ctime(&current_time);
    return std::string(c_time_string);
}

// 将内容写进文件
void write_to_file(const std::string& filepath, const std::string& content){
    // 打开文件
    int fd = open(filepath.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        std::cerr << "Failed to open " + filepath + " \n";
        throw std::runtime_error("Failed to open " + filepath + " \n");
    }

    // 创建一个文件锁
    struct flock fl;
    fl.l_type   = F_WRLCK;  // 写锁
    fl.l_whence = SEEK_SET; // 锁定整个文件
    fl.l_start  = 0;
    fl.l_len    = 0;

    // 尝试加锁
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            std::cerr << "File is locked by another process\n";
            throw std::runtime_error("File is locked by another process\n");
        }
        std::cerr << "Failed to lock file\n";
        throw std::runtime_error("Failed to lock file\n");
    }

    // 写入数据
    std::string data = content + "\n";
    if (write(fd, data.c_str(), data.size()) != data.size()) {
        std::cerr << "Failed to write to file\n";
        throw std::runtime_error("Failed to write to file\n");
    }

    // 解锁文件
    fl.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        std::cerr << "Failed to unlock file\n";
        throw std::runtime_error("Failed to unlock file\n");
    }

    close(fd);
}

// 主函数
int main(int arg, char *argv[]) {

    int listen_fd; // 监听套接字文件描述符

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "Failed to create the server's socket\n";
        throw std::runtime_error("Failed to create the server's socket");
    }

    int reuse = 1;

    if ((setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) == -1) {
        close(listen_fd);
        std::cerr << "Failed to set the socket's options\n";
        throw std::runtime_error("Failed to set the socket's options");
    }

    struct sockaddr_in serv_addr; // 服务端网络信息结构体

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    if ((bind(listen_fd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr))) == -1) {
        close(listen_fd);
        std::cerr << "Failed to bind the socket\n";
        throw std::runtime_error("Failed to bind the socket");
    }

    if ((listen(listen_fd, LISTEN_MAX_COUNT)) == -1) {
        close(listen_fd);
        std::cerr << "Failed to configure the socket's listening status\n";
        throw std::runtime_error("Failed to configure the socket's listening status");
    }

    struct sockaddr_in clie_addr; // 客户端网络信息结构体
    socklen_t addr_size;          // 网络信息结构体大小

    bzero(&clie_addr, sizeof(clie_addr));
    addr_size = sizeof(struct sockaddr);

	while (1) {
		int connect_fd;               // 连接套接字文件描述符

		if ((connect_fd = accept(listen_fd, (struct sockaddr *)(&clie_addr), &addr_size)) == -1) {
			close(connect_fd);
			close(listen_fd);
			std::cerr << "Failed to accept connection\n";
			throw std::runtime_error("Failed to accept connection");
		}

		pid_t pid = fork();
		if (pid < 0) {
			std::cerr << "Failed to fork process\n";
			throw std::runtime_error("Failed to fork process");
		}
		else if (pid == 0) {  // 子进程
			close(listen_fd);

			std::string msg_recv; // 接收客户端消息缓冲区
			std::string msg_send; // 发送客户端消息缓冲区

			msg_recv.resize(BUFF_SIZE);
			msg_send.resize(BUFF_SIZE);

			if ((recv(connect_fd, &msg_recv[0], BUFF_SIZE, 0)) < 0) { // 接收数据
				close(connect_fd);
				std::cerr << "Failed to receive messages from the client\n";
				throw std::runtime_error("Failed to receive messages from the client");
			}

			std::cout << msg_recv << "\n"; // 接收的消息

			msg_send = "test2@211.71.149.249";
			std::cout << msg_send << "\n"; // 发送的消息

			if ((send(connect_fd, msg_send.c_str(), BUFF_SIZE, 0)) == -1) { // 发送数据
				std::cerr << "Failed to send messages to the client\n";
				throw std::runtime_error("Failed to send messages to the client");
			}

			close(connect_fd);
			exit(EXIT_SUCCESS);
		}
		else {  // 父进程
			close(connect_fd);
		}
	}
    return 0;
}
