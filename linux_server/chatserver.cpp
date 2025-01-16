// compile: -lcrypto -lpthread
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <map>
#include <fstream>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <sstream>
#include <iomanip>
#include <thread>

// sha1 sha256 sha512 md5
// EPOLL for IO intensive computing
// Thread for CPU intensive computing
// EPOLL(hanlding connection) + Thread(handle cpu like file transmission)
// EPOLL(Single) + Thread(Multi)
// EPOLL(Multi) + Thread(Multi)

#define MAX_EVENTS 1000
#define BUF_SIZE 1024
#define PORT 9999
#define EPOLL_SIZE 50

pthread_mutex_t mutex;
// client socket fd, client user,name(ip)
std::map<int, std::string> *map_clients;

void error_handling(const char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

// Level Trigger, set fd to non-blocking mode
void setnonblockingmode(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

bool is_fd_valid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

/**
 * Send message to all clients, Broadcast
 */
void *send_msg_all(void *msg, int len)
{
    pthread_mutex_lock(&mutex);
    for (auto it = map_clients->begin(); it != map_clients->end(); it++)
    {
        if (is_fd_valid(it->first))
            write(it->first, msg, len);
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

// -lcrypto
std::string generate_sha256(const std::string &input)
{
    std::string suffix = input.substr(input.find_last_of('.'));
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        throw std::runtime_error("EVP_MD_CTX_new() failed");
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int hash_len;

    // Initialize the digest context with SHA256 Algorithm
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestInit_ex() failed");
    }

    // Update the context with the input data
    if (EVP_DigestUpdate(ctx, input.c_str(), input.size()) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestUpdate() failed");
    }

    // Finalize the hash
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestFinal_ex() failed");
    }
    EVP_MD_CTX_free(ctx);
    char hex_hash[2 * SHA256_DIGEST_LENGTH + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(hex_hash + 2 * i, "%02x", hash[i]);
    }
    hex_hash[2 * SHA256_DIGEST_LENGTH] = '\0';
    return std::string(hex_hash) + suffix;
}

/**
 * md5sum hello.txt | cut -d ' ' -f1
 */
std::string md5(const std::string filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file for MD5 computation");
    }
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        throw std::runtime_error("EVP_MD_CTX_new() failed");
    }
    if (EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestInit_ex() failed");
    }
    char buf[4096];
    while (file.read(buf, sizeof(buf)) || file.gcount() > 0)
    {
        if (EVP_DigestUpdate(ctx, buf, file.gcount()) != 1)
        {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("EVP_DigestUpdate() failed");
        }
    }

    unsigned char hash[MD5_DIGEST_LENGTH];
    unsigned int hash_length = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_length) != 1)
    {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestFinal_ex() failed");
    }
    EVP_MD_CTX_free(ctx);
    std::ostringstream oss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

/**
 * This is a simple way to handle file upload.
 * The server receives the file in chunks and writes it to the file.
 * In real-world applications, you should consider using a more robust protocol like `FTP` or HTTP.
 * And it's more complicated than this.
 * You should consider, duplicate name, interrupted-resume, md5 checksum, virus scan, etc.
 * cancel, pause, resume, progress, etc.
 */
void handle_file_upload(int client_sock, std::string filename, size_t file_size)
{
    // std::ifstream infile(filename);
    // if (infile.good())
    // {
    //     std::cout << "File already exists: replace? " << filename << std::endl;
    //     return;
    // }

    char buf[4096];
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        perror("fopen() error");
        return;
    }
    size_t bytes_received = 0;
    while (bytes_received < file_size)
    {
        ssize_t bytes = recv(client_sock, buf, 4096, 0);
        if (bytes > 0)
        {
            file.write(buf, bytes);
            bytes_received += bytes;
        }
        else if (bytes < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                // std::cout << "No data available to read, try again later" << std::endl;
                continue;
            std::cout << "Recv failed: " << strerror(errno) << std::endl;
            break;
        }
        else
        {
            // bytes == 0
            // disconnected
            std::cout << "Disconnected Connection closed by the peer" << std::endl;
            break;
        }
    }
    file.close();
#if 0
    char buf[BUF_SIZE];
    size_t total_recv = 0;
    FILE *fp = fopen(filename.c_str(), "wb");
    if (fp == NULL)
    {
        perror("fopen() error");
        return;
    }

    while (total_recv < file_size)
    {
        ssize_t recv_len = read(client_sock, buf, BUF_SIZE);
        if (recv_len > 0)
        {
            fwrite(buf, 1, recv_len, fp);
            total_recv += recv_len;
        }
        else
        {
            break;
        }
    }
    fclose(fp);
#endif
    std::cout << "File upload complete: " << filename << " Received bytes: "
                << bytes_received << std::endl;
    // Broadcast to all clients about the new file and it's size
    std::string msg = "FILE " + filename + " " + std::to_string(file_size);
    send_msg_all((void *)msg.c_str(), msg.size() + 1);
    std::string notification = u8"上传了文件: " + filename;
    send_msg_all((void *)notification.c_str(), notification.size() + 1);
}

void handle_file_download(int client_sock, const std::string filename)
{
#if 1
    char buf[BUF_SIZE];
    size_t bytes_sent = 0;
    FILE *fp = fopen(filename.c_str(), "rb");
    if (fp == NULL)
    {
        perror("fopen() error");
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    while (bytes_sent < file_size)
    {
        ssize_t bytes_read = fread(buf, 1, BUF_SIZE, fp);
        if (bytes_read > 0)
        {
            size_t bytes_to_send = bytes_read;
            char *send_ptr = buf;
            if (bytes_to_send > 0)
            {
                ssize_t sent = send(client_sock, send_ptr, bytes_to_send, 0);
                if (sent < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        std::cout << "No data available to read, try again later" << std::endl;
                        continue;
                    }
                    puts("send() error");
                    std::cout << "Send failed: " << strerror(errno) << std::endl;
                    break;
                }
                bytes_to_send -= sent;
                send_ptr += sent;
            }
            bytes_sent += bytes_read;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        else if (bytes_read < 0)
        {
            puts("fread() error");
            break;
        }
        else
        {
            break; // EOF
        }
    }
    fclose(fp);
#endif
#if 0
    std::ifstream file(filename, std::ios::binary);
    if (!file.good())
    {
        printf("File Name compare: filename:[%s], [FTNN_14.38.16058.exe] %d\n", filename.c_str(), filename.compare("FTNN_14.38.16058.exe"));
        std::cerr << "File not found: " << filename << std::endl;
        send(client_sock, "File not found", 14, 0);
        return;
    }
    char buf[BUF_SIZE];
    size_t bytes_sent = 0;
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    printf("File size: %ld\n", file_size);
    while (file.read(buf, sizeof(buf)))
    {
        send(client_sock, buf, file.gcount(), 0);
        bytes_sent += file.gcount();
    }
    file.close();
#endif
    std::cout << "File download complete: " << filename << "Bytes Send: "<< bytes_sent << std::endl;
}

void broadcast_userlist()
{
    std::string userlist = "USERLIST ";
    for (auto it = map_clients->begin(); it != map_clients->end(); it++)
    {
        userlist += it->second + "\n";
    }
    printf("Broadcasting user list: \n%s\n", userlist.c_str());
    send_msg_all((void *)userlist.c_str(), userlist.size() + 1);
}

/***
 * Handle client message in thread
 * @arg client_sock: client socket fd
 */
void *handle_client(void *arg)
{
    int client_sock = *((int *)arg);
    setnonblockingmode(client_sock);
    int epfd = epoll_create(1);
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    event.data.fd = client_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &event);
    char msg[BUF_SIZE];
    bool name_saved = false;
    while (true)
    {
        int n = epoll_wait(epfd, &event, 1, -1);
        if (n == 1 && event.events & EPOLLIN)
        {
            // Client IO ready, read.
            ssize_t bytes_read = recv(client_sock, msg, BUF_SIZE - 1, 0);
            if (bytes_read == 0)
            {
                // Disconnected
                break;
            }
            else if (bytes_read < 0)
            {
                // read finished
                if (errno == EAGAIN)
                    continue;
                break;
            }
            else
            {
                msg[bytes_read] = '\0';
                std::string message(msg);
                if (message.substr(0, 6) == "UPLOAD")
                {
                    puts("Upload file message");
                    // UPLOAD filename, there is a space
                    std::istringstream iss(msg);
                    std::string cmd, filename;
                    size_t filesize;
                    iss >> cmd >> filename >> filesize;
                    std::cout << "Filename: " << filename << " Filesize: " << filesize << std::endl;
                    handle_file_upload(client_sock, filename, filesize);
                    break;
                }
                else if (message.substr(0, 8) == "DOWNLOAD")
                {
                    puts("Download file message");
                    std::string filename = message.substr(strlen("DOWNLOAD") + 1);
                    printf("Download filename: [%s]\n", filename.c_str());
                    handle_file_download(client_sock, std::string(filename));
                    puts("Download file Finished");
                    break;
                }
                else if (message.compare("USERLIST") == 0)
                {
                    // new user enter will request user list
                    // it will broadcast the user list to all clients
                    // to update the user list.
                    puts("User list request");
                    broadcast_userlist();
                }
                else
                {
                    printf("Normal message: %s\n", msg);
                    // greeting message first, then request user list.
                    if (!name_saved)
                    {
                        name_saved = true;
                        std::string str(msg);
                        std::string name = str.substr(0, str.find(' '));
                        std::cout << "Client Username: " << name << std::endl;
                        map_clients->at(client_sock) += ":" + name;
                        // name saved.
                        printf("Client Username Saved: %d %s\n", client_sock, map_clients->at(client_sock).c_str());
                    }
                    send_msg_all(msg, bytes_read);
                }
            }
        }
        if (n == -1 || event.events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
        {
            printf("client: %d %s epoll_wait() error\n", client_sock, map_clients->at(client_sock).c_str());
            break;
        }
    }
    // Remove
    pthread_mutex_lock(&mutex);
    map_clients->erase(client_sock);
    epoll_ctl(epfd, EPOLL_CTL_DEL, client_sock, NULL);
    close(client_sock);
    printf("Closed client: %d\n", client_sock);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void test_sha256()
{
    std::string filename = "hello.txt";
    std::string sha256 = generate_sha256(filename);
    std::string md = md5(filename);
    std::cout << "SHA256: " << sha256 << "\nMD5: " << md << std::endl;
}

int run_server(int port)
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    // init mutex
    pthread_mutex_init(&mutex, NULL);

    // prepare server socket
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        error_handling("bind() error");
    if (listen(server_sock, 5) == -1)
        error_handling("listen() error");

    // epoll instance fd
    int epfd = epoll_create(1);
    // struct epoll_event *ep_events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * EPOLL_SIZE);
    struct epoll_event event;
    // epoll, non blocking io event monitoring
    event.events = EPOLLIN | EPOLLET,
    event.data.fd = server_sock,
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &event);

    int str_len;
    char buf[BUF_SIZE];
    map_clients = new std::map<int, std::string>();
    while (true)
    {
        int ret = epoll_wait(epfd, &event, EPOLL_SIZE, -1);
        if (ret == -1)
        {
            puts("epoll_wait() error");
            break;
        }
        // server sock IO ready
        if (event.data.fd == server_sock && (event.events & EPOLLIN))
        {
            // new connection, wait/block until accept
            client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size);
            map_clients->insert(std::pair<int, std::string>(client_sock, inet_ntoa(client_addr.sin_addr)));
            pthread_mutex_lock(&mutex);
            printf("New Connected client: %d\n", client_sock);
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, (void *)&client_sock);
            pthread_detach(tid);
            pthread_mutex_unlock(&mutex);
        }
    }
    close(server_sock);
    close(epfd);
    delete map_clients;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    run_server(atoi(argv[1]));
    return 0;
}
