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
#include <sstream>
#include <fstream>
#include <signal.h>

#define OK 0
#define ERR 1

#define PUT 1
#define DEL 2
#define GET 3
#define LST 4
#define MKD 5
#define RMD 6

#define FI 0
#define FO 1
#define NOTH 2
using namespace std;
string arr[] = { "PUT", "DEL", "GET", "LST", "MKD", "RMD"};
vector<string> commands (arr, arr + 6) ;
string arr2[] = {"-p", "-r"};
vector<string> args (arr2, arr2+2);

string ok = "HTTP/1.1 200 OK\n\r";
string notF = "HTTP/1.1 404 Not Found\n\r";
string badR = "HTTP/1.1 400 Bad Request\n\r";

void errMsg(const char *msg)
{
    perror(msg);
}

struct arg{
    string rootDir;
    uint16_t portNum;
};

string getMIME(string localPath)
{
    FILE *mime;
    char b[200];
    string mi;
    string a = "file -b --mime-type " + localPath;
    if ((mime = popen(a.c_str(), "r")) == NULL)
    {
        errMsg("ERROR: popen neziskal mime, typ bude: application/octet-stream");
        pclose(mime);

        mi = "application/octet-stream";
        return mi;
    }
    while(fgets(b, 200, mime))
        mi += b;
    pclose(mime);
    return mi;
}


int fileOrFolder(string path)
{
    struct stat s;

    if( stat(path.c_str(),&s) == 0 )
    {
        if( s.st_mode & S_IFDIR )
        {
            return FO;
        }
        else if( s.st_mode & S_IFREG )
        {
            return FI;
        } else
            return NOTH;

    } else
        return NOTH;
}

struct arg* getParams (int argc, char *argv[])
{
    //if (argc != 5 && argc != 3)
    //  errMsg(42, "ERROR:: spatne argumetny");
    char temp[PATH_MAX];
    arg *str = new arg;
    str->portNum = 6677;
    str->rootDir = getcwd(temp, PATH_MAX);
    if (argc == 3)
    {
        if ( args[0].compare(argv[1]) == 0 && isdigit(*argv[2]))
        {
            char *t = NULL;
            str->portNum =   (uint16_t) strtol(argv[2], &t, 10);

            if (*t != '\0')
            {
                errMsg( "ERROR: port neni numericky");
                delete str;
                return NULL;
            }
        }
    }
    else if (argc == 5)
    {
        if ( args[0].compare(argv[1]) == 0 && args[1].compare(argv[3]) == 0)
        {
            if ( args[0].compare(argv[1]) == 0 )
            {
                char *t = NULL;
                str->portNum =   (uint16_t) strtol(argv[2], &t, 10);

                if (*t != '\0')
                {
                    errMsg( "ERROR: port neni numericky");
                    delete str;
                    return NULL;
                }

            }
            str->rootDir = "";
            str->rootDir = argv[4];
            if (fileOrFolder(str->rootDir) != FO )
            {
                delete str;
                errMsg("ERROR:: zadany parametr neni slozka");
                return NULL;
            }
        }
        else if (args[0].compare(argv[3]) == 0 && args[1].compare(argv[1]) == 0)
        {
            if ( args[0].compare(argv[3]) == 0 )
            {
                char *t = NULL;
                str->portNum =   (uint16_t) strtol(argv[4], &t, 10);

                if (*t != '\0')
                {
                    errMsg( "ERROR: port neni numericky");
                    delete str;
                    return NULL;
                }
            }
            str->rootDir = "";
            str->rootDir = argv[2];
            if (fileOrFolder(str->rootDir) != FO )
            {
                delete str;
                errMsg("ERROR:: zadany parametr neni slozka");
                return NULL;
            }

        }
    }
    //cout << str->rootDir<< end;
    return str;

}
string readSock(int socket){
    char buf[1024];
    bzero(buf, 1024);
    int n;
    string rest;
    n = (int) read(socket, buf, 1024);
    rest = buf;
    return rest;
}

int writeSock(int socket, string buf)
{
    int n = (int) write(socket, buf.c_str(), 1024);
    if (n < 0) return ERR;
    return OK;
}
string createHeader(int type)
{
    char buf[1024];
    string rest;
    time_t now = time(0);
    tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    switch (type)
    {
        case DEL:
            rest.append(ok);
            rest += "Date: ";
            rest.append(buf);
            rest += "\n\rContent-Type: text/plain\n\rContent-Length: 0\n\rContent-Encoding: identity\n\r";
            break;

        case RMD:
            rest.append(ok);
            rest += "Date: ";
            rest.append(buf);
            rest += "\n\rContent-Type: text/plain\n\rContent-Length: 0\n\rContent-Encoding: identity\n\r";
            break;
        case GET:
            rest.append(ok);
            rest += "Date: ";
            rest.append(buf);
            rest += "\n\rContent-Type: ";
            break;
        case LST:
            rest.append(ok);
            rest += "Date: ";
            rest.append(buf);
            rest += "\n\rContent-Type: text/plain\n\rContent-Length: ";

            break;
        case PUT:
            rest.append(ok);
            rest += "Date: ";
            rest.append(buf);
            rest += "\n\rContent-Type: text/plain\n\rContent-Length: 0\n\rContent-Encoding: identity\n\r";
            break;

        case MKD:
            rest.append(ok);
            rest += "Date: ";
            rest.append(buf);
            rest += "\n\rContent-Type: text/plain\n\rContent-Length: 0\n\rContent-Encoding: identity\n\r";
            break;
    }

    return rest;
}
int reqForPut(int sock, int type, string path, string fsize)
{
    char buff[1024];
    int size = 0;
    int n;
    FILE *file;
    DIR * dir;
    string rest;
    cout << path << endl;
    if (type == PUT ) {
        file = fopen(path.c_str(), "wb");
        if (file == NULL)
        {
            errMsg("ERROR: could not open the path.");
            writeSock(sock, "Unknown error\n");
            return ERR;
        }

        while (atoi(fsize.c_str()) != size) {
            bzero(buff, 1024);
            int rec = (int) recv(sock, buff, 1024, 0);
            if (rec < 0) {

                errMsg("ERROR: nic neprijato nebo disconnect.");
                remove(path.c_str());
                fclose(file);
                writeSock(sock, "Unknown error\n");
                return ERR;
            }

            if (fwrite(buff, rec, 1, file) != 1) {
                errMsg("ERROR: nelze zapsat do souboru");
                remove(path.c_str());
                fclose(file);
                writeSock(sock, "Unknown error\n");
                return ERR;
            }
            size += rec;
        }

        if (atoi(fsize.c_str()) != size)
        {
            errMsg("ERROR: wrong file");
            remove(path.c_str());
            fclose(file);
            writeSock(sock, "Unknown error\n");
            return ERR;
        }

        writeSock(sock, createHeader(type));

        fclose(file);
    }
    else if ( type == MKD )
    {
        //todo mkdir, close socket, free?, exit, user handling

        mkdir(path.c_str(), S_IRWXU | S_IRWXG );
        writeSock(sock, createHeader(type));

    }
    else {
        //todo error handle for PUT && MKDIR
    }
    return OK;

}
int reqForGet(int sock, string path, int type)
{
    FILE *file, *mime;
    int n;
    long fsize, save;
    char buf[1024];
    char *content = NULL;
    string rest = createHeader(type);

    if(fileOrFolder(path) == FO && type == LST)
    {
        string a = "ls " + path;
        if ((mime = popen(a.c_str(), "r")) == NULL) {
            errMsg("ERROR popen neziskal ls");
            writeSock(sock, "Unknown error\n");
            return ERR;
        }

        char b[200];
        string mi;
        while(fgets(b, 200, mime)) {
            mi += b;
        }

        fsize = mi.length();
        stringstream ss;
        ss << fsize;
        string str = ss.str();
        rest += str + "\n\rContent-Encoding: identity\n\r";

        writeSock(sock, rest);
        writeSock(sock, mi);

    } else if (fileOrFolder(path) == FI && type == GET){

        file = fopen(path.c_str(), "rb");
        if ( file == NULL )
        {
            errMsg("ERROR otevirani souboru");
            writeSock(sock, "Unknown error\n");
            return ERR;
        }

        rest += getMIME(path);
        rest += "\rContent-Length: ";

        if (fseek(file, 0, SEEK_END) == -1)
        {
            errMsg("ERROR nelze dojit na konec souboru");
            writeSock(sock, "Unknown error\n");
            return ERR;
        }

        fsize = ftell(file);
        if (fsize == -1)
        {
            errMsg("ERROR zadna velikost souboru");
            writeSock(sock, "Unknown error\n");
            return ERR;
        }

        stringstream ss;
        ss << fsize;
        string str = ss.str();
        rest += str + "\n\rContent-Encoding: identity\n\r";

        rewind(file);

        content = (char*) malloc((size_t) fsize);
        if (!content)
        {
            errMsg("ERROR nenaalokoval se prostor pro file");
            fclose(file);
            writeSock(sock, "Unknown error\n");
            return ERR;
        }
        if (fread(content, fsize, 1, file) != 1)
        {
            errMsg("ERROR soubor nebyl nacten");
            free(content);
            fclose(file);
            writeSock(sock, "Unknown error\n");
            return ERR;
        }
        writeSock(sock, rest);

        bzero(buf, 1024);

        while (fsize > 0) {
            int sent = (int) send(sock, content, fsize, 0);
            if (sent <= 0) {
                if (sent == 0)
                {
                    errMsg("ERROR disconnected");
                    fclose(file);
                    free(content-save);
                    writeSock(sock, "Unknown error\n");
                    return ERR;
                }
                else
                {
                    errMsg("ERROR nic nebylo zapsano");
                    fclose(file);
                    free(content-save);
                    writeSock(sock, "Unknown error\n");
                    return ERR;
                }
            }
            content += sent;
            fsize -= sent;
            save += sent;
        }
        fclose(file);
        //free(content-save);
        if (fsize != 0)
        {
            writeSock(sock, "Unknown error\n");
            return ERR;
        }
    }
    else
    {
        cout <<  path << endl;

        if (fileOrFolder(path) == 0 && type == GET)
        {
            writeSock(sock, badR);
            writeSock(sock, "Not a file.\n");
        }
        else if (fileOrFolder(path) == 1 && type == LST)
        {
            writeSock(sock, badR);
            writeSock(sock, "Not a directory.\n");
        }
        else if (fileOrFolder(path) == 2 && type == LST)
        {
            writeSock(sock, notF);
            writeSock(sock, "Directory not found.\n");

        }
        else if (fileOrFolder(path) == 2 && type == GET)
        {
            writeSock(sock, notF);
            writeSock(sock, "File not found.\n");
        }
        return ERR;
    }

    return OK;
}
int reqForDel(int sock, string path, int type)
{
    string rest = createHeader(type);

    if (type == DEL && fileOrFolder(path) == FI)
    {
        remove(path.c_str());
        writeSock(sock, rest);
    }
    else if (type == RMD && fileOrFolder(path) == FO)
    {

        if (rmdir(path.c_str()) != 0)
        {
            errMsg("ERROR slozku se nepodarilo smazat");
            writeSock(sock, badR);
            return ERR;
        }
        else
        {
            writeSock(sock, rest);
        }
    } else{
        //todo error handle
    }
    return OK;
}

void serveMe(int sock, struct arg * arg) {
    int n = 0;
    string operation;
    string path = arg->rootDir;
    string type;
    string fsize;
    string rest = readSock(sock);
    //cout << rest << endl;
    int flag = 0;
    int countOfN = 0;
    for (string::size_type i = 0; i < rest.size(); i++) {
        if (i < 3) {

            operation += rest[i];
            if (operation == "DEL")
            {
                i += 4;
            }
        }
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

    if (operation == "PUT" && type == "file") {
        //path += "/text.txt";
        n = reqForPut(sock, PUT, path, fsize);
    } else if (operation == "GET" && type == "file") {
        //path += "/k.deb";
        n = reqForGet(sock, path, GET);
    } else if (operation == "DEL" && type == "file") {
        //path += "/k.deb";
        n = reqForDel(sock, path, DEL);
    } else if (operation == "PUT" && type == "folder") {
        n = reqForPut(sock, MKD, path, fsize);
    } else if (operation == "DEL" && type == "folder")
    {
        n = reqForDel(sock, path, RMD);
    }
    else if ( operation == "GET" && type == "folder")
    {
        n = reqForGet(sock, path, LST);
    }
    close(sock);
    delete arg;
    exit(n);
}
void createSock(struct arg * str)
{
    //close(12345);
    int sockfd, newsockfd, pid, n;

    socklen_t clientLeng;
    sockaddr_in servAddr, clientAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        errMsg("ERROR otevirani socketu");
        delete str;
        exit(ERR);
    }

    bzero((char*) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port   =   htons(str->portNum);
    n   = bind(sockfd, (sockaddr *) &servAddr, sizeof(servAddr));
    if (n < 0)
    {
        errMsg("ERROR binding");
        delete str;
        exit(ERR);
    }

    listen (sockfd, 5);

    clientLeng = sizeof(clientAddr);

    while (42)
    {
        newsockfd = accept(sockfd, (sockaddr * ) &clientAddr, &clientLeng );

        if (newsockfd < 0) {
            errMsg("ERRR creating new socket descriptor");
            delete str;
            exit(42);
        }
        signal(SIGCHLD, SIG_IGN);
        pid = fork();
        if (pid == 0)
        {
            serveMe(newsockfd, str);
        } else close(newsockfd); //todo SIGCHILD
    }
}



int main(int argc, char *argv[]) {
    arg * str = getParams(argc, argv);
    if (str == NULL)
        exit(ERR);
    createSock(str);

    return 0;
}
