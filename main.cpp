#include <iostream>
#include <vector>
#include <string>
#include <locale>
#include <sys/socket.h>
#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>

#define OK 0
using namespace std;
string arr[] = { "PUT", "DEL", "GET", "LST", "MKD", "RMD"};
vector<string> commands (arr, arr + 6) ;
string arr2[] = {"-p", "-r"};
vector<string> args (arr2, arr2+2);

void errMsg(int err,const char *msg)
{
    perror(msg);
    exit(err);
}

struct arg{
    string rootDir;
    uint16_t portNum;
};

struct arg* getParams (int argc, char *argv[])
{
    //if (argc != 5 && argc != 3)
      //  errMsg(42, "ERROR: spatne argumetny");
    char temp[PATH_MAX];
    arg *str = new arg;
    str->portNum = 6677;
    str->rootDir = getcwd(temp, PATH_MAX);
    if (argc == 3)
    {
        if ( args[0].compare(argv[1]) == 0 && isdigit(*argv[2]))
        {
            char *t;
            str->portNum =   (uint16_t) strtol(argv[2], &t, 10);

            if (t == NULL)
                errMsg(42, "ERROR port neni numericky");

        }
    }
    else if (argc == 5)
    {
        if ( args[0].compare(argv[1]) == 0 && args[1].compare(argv[3]) == 0)
        {
            if ( args[0].compare(argv[1]) == 0 && isdigit(*argv[2]))
            {
                char *t;
                str->portNum =   (uint16_t) strtol(argv[2], &t, 10);

                if (t == NULL)
                    errMsg(42, "ERROR port neni numericky");

            } else errMsg(42, "ERROR port neni numericky");

            str->rootDir = argv[4];
        }
        else if (args[0].compare(argv[3]) == 0 && args[1].compare(argv[1]) == 0)
        {
            if ( args[0].compare(argv[3]) == 0 && isdigit(*argv[4]))
            {
                char *t;
                str->portNum =   (uint16_t) strtol(argv[4], &t, 10);

                if (t == NULL)
                    errMsg(42, "ERROR port neni numericky");

            } else errMsg(42, "ERROR port neni numericky");

            str->rootDir = argv[2];
        }
    }

    return str;

}

int reqForPut(int newsockfd, string type, string path, string fsize)
{
    char buff[1024];
    int size = 0;
    int n;
    FILE *file;
    DIR * dir;
    cout << path << endl;
    if (type == "file")
    {
        file = fopen(path.c_str(), "wb");
        if (file == NULL ) errMsg(42, "ERROR could not open the path."); //todo error handling to the client
    }
    else if ( type == "folder")
    {
        //todo mkdir, close socket, free?, exit, user handling
        mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        n = (int) write(newsockfd, "HTTP/1.1 200 OK", 1024);
        if (n < 0) errMsg(42, "ERROR neodeslano");
        close(newsockfd);
        return OK;
    }

    n = (int) write(newsockfd, "HTTP/1.1 200 OK", 1024);
    if (n < 0) errMsg(42, "ERROR neodeslano");

    while (atoi(fsize.c_str()) > size)
    {
        bzero(buff, 1024);
        int rec = (int) recv(newsockfd, buff, 1024, 0);
        if (rec < 0) errMsg(42, "ERROR nic neprijato nebo disconnect.");

        if (fwrite(buff, rec, 1, file) != 1) errMsg(42, "ERROR nelze zapsat do souboru.");
        size += rec;

    }

    if (atoi(fsize.c_str()) != size) errMsg(42, "ERROR wrong file");
    n = (int ) write(newsockfd, "HTTP/1.1 200 OK", 1024);
    if (n < 0) errMsg(42, "ERROR neodeslano");
    if (file != NULL)
        fclose(file);
    close(newsockfd);
    return OK;
}
int reqForGet()
{

}

int reqForDel()
{

}


void serveMe(int newsockfd) {
    int n;
    string operation;
    string path = "/home";
    string type;
    string fsize;
    FILE *file = NULL;
    long size = 0;

    char buff[1024];
    bzero(buff, 1024);
    n = (int) read(newsockfd, buff, 1024);


    if (n < 0) errMsg(42, "ERROR cteni ze socketu");
    string rest = buff;
    //cout << rest << endl;
    int flag = 0;
    int countOfN = 0;
    for (string::size_type i = 0; i < rest.size(); i++) {
        if (i < 3) operation += rest[i];
        else if (i != 3 && rest[i] != '?' && flag == 0) path += rest[i]; //todo \t atd.
        else if (rest[i] == '?') {
            flag = 1;
        } else if (flag == 1 && rest[i] == '=') flag = 2;
        else if (flag == 2 && rest[i] != ' ') type += rest[i];
        else if (rest[i] == ' ' && flag == 2) flag = 3;
        else if (rest[i] == '\n') countOfN++;
        else if (countOfN == 5) {
            if (isdigit(rest[i]))
                fsize += rest[i];
        }
    }
    cout << operation << endl;
    cout << type << endl;
    cout << path << endl;
    cout << fsize << endl;

    if (operation == "PUT") {
        //path += "/text.txt";
        reqForPut(newsockfd, type, path, fsize);
    } else if (operation == "GET") {
        //path += "/k.deb";
        reqForGet();
    } else if (operation == "DEL") {
        //path += "/k.deb";

    } else if (operation == "MKD") {
        reqForPut(newsockfd, type, path, fsize);
    } else if (operation == "RMD")
    {

    }
    else if ( operation == "LST" )
    {

    }
    close(newsockfd);
    exit(0);
}
void createSock(struct arg * str)
{
    int sockfd, newsockfd, pid, n;

    socklen_t clientLeng;
    sockaddr_in servAddr, clientAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        errMsg(42, "ERROR otevirani socketu");

    bzero((char*) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port   =   htons(str->portNum);

    if (bind(sockfd, (sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        errMsg(42, "ERROR binding");

    listen (sockfd, 5);

    clientLeng = sizeof(clientAddr);

    while (42)
    {
        newsockfd = accept(sockfd, (sockaddr * ) &clientAddr, &clientLeng );

        if (newsockfd < 0)
            errMsg(42, "ERRR accept");

       pid = fork();
       if (pid == 0)
       {
            serveMe(newsockfd);
       } else close(newsockfd); //todo SIGCHILD
    }
}



int main(int argc, char *argv[]) {

    arg * str = getParams(argc, argv);

    createSock(str);

    std::cout << "Hello, World!" << std::endl;
    return 0;
}
