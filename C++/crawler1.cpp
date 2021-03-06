#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <utility>
#include <regex>
#include <fstream>
#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void startupWSA()
{
    WSADATA wsadata;
    WSAStartup( MAKEWORD(2,0), &wsadata);
}

inline void cleanupWSA()
{
    WSACleanup();
}

inline pair<string, string> binaryString(const string &str, const string &dilme)
{
    pair<string, string> result(str, "");
    auto pos = str.find(dilme);
    if ( pos != string::npos )
    {
        result.first = str.substr(0, pos);
        result.second = str.substr(pos + dilme.size());
    }
    return result;
}

inline string getIpByHostName(const string &hostName)
{
    hostent* phost = gethostbyname( hostName.c_str() );
    return phost? inet_ntoa(*(in_addr *)phost->h_addr_list[0]): "";
}

inline SOCKET connect(const string &hostName)
{
    auto ip = getIpByHostName(hostName);
    if ( ip.empty() )
        return 0;
    auto sock = socket(AF_INET, SOCK_STREAM, 0);
    if ( sock == INVALID_SOCKET )
        return 0;
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if ( connect(sock, (const sockaddr *)&addr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR )
        return 0;
    return sock;
}

inline bool sendRequest(SOCKET sock, const string &host, const string &get)
{
    string http 
        = "GET " + get + " HTTP/1.1\r\n"
        + "HOST: " + host + "\r\n"
        + "Connection: close\r\n\r\n";
    return http.size() == send(sock, &http[0], http.size(), 0);
}

inline string recvRequest(SOCKET sock)
{
    static timeval wait = {2, 0};
    static auto buffer = string(2048 * 100, '\0');
    auto len = 0, reclen = 0;
    do {
        fd_set fd = {0};
        FD_SET(sock, &fd);
        reclen = 0;
        if ( select(0, &fd, nullptr, nullptr, &wait) > 0 )
        {
            reclen = recv(sock, &buffer[0] + len, 2048 * 100 - len, 0);
            if (reclen > 0)
                len += reclen;
        }
        FD_ZERO(&fd);
    } while (reclen > 0);

    return len > 11
        ? buffer[9] == '2' && buffer[10] == '0' && buffer[11] == '0'
        ? buffer.substr(0, len)
        : ""
        : "";
}

inline void extUrl(const string &buffer, queue<string> &urlQueue)
{
    if (buffer.empty())
    {
        return ;
    }
    smatch result;
    auto curIter = buffer.begin();
    auto endIter = buffer.end();
    while ( regex_search(curIter, endIter, result, regex("href=\"(https?:)?//\\S+\"") ) )
    {
        urlQueue.push(regex_replace(
            result[0].str(), 
            regex("href=\"(https?:)?//(\\S+)\""),
            "$2") );
        curIter = result[0].second;
    }
}

void Go(const string &url, int count)
{
    queue<string> urls;
    urls.push(url);
    for (auto i = 0; i != count; ++i)
    {
        if ( !urls.empty() )
        {
            auto &url = urls.front();
            auto pair = binaryString( url, "/" );
            auto sock = connect(pair.first);
            if ( sock && sendRequest(sock, pair.first, "/" + pair.second) )
            {
                auto buffer = move( recvRequest(sock) );
                extUrl(buffer, urls);
            }
　　　　　　　closesocket(sock);
            cout << url << ": count=> " << urls.size() <<  endl;
            urls.pop();

        }
    }
}

int main()
{
    startupWSA();
    Go("www.hao123.com", 200);
    cleanupWSA();
    return 0;
}