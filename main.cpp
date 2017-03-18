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
#include <errno.h>

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

string ok           =   "HTTP/1.1 200 OK\r\n";
string notF         =   "HTTP/1.1 404 Not Found\r\n";
string badR         =   "HTTP/1.1 400 Bad Request\r\n";
string forbidden    =   "HTTP/1.1 400 Bad Request\r\n";

string intSerErr    =   "HTTP/1.1 500 Internal Server Error\r\n";
string notADir      =   "Not a directory\n";
string dirNotFound  =   "Directory not found\n";
string dirNotEmpty  =   "Directory not empty\n";
string alrExists    =   "Already exists\n";
string notAFile     =   "Not a file\n";
string fiNotFound   =   "File not found\n";
string userNotFound =   "User Account Not Found\n";
string unknownErr   =   "Unknown error.\n";


void errMsg(const char *msg)
{
    perror(msg);
}

struct arg{
    string rootDir;
    uint16_t portNum;
};

struct content {
    string fsize;
    string operation;
    string path;
    string type;
};

struct content *cont = new content;

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
        argv[1][1] = tolower(argv[1][1]);
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
        argv[1][1] = tolower(argv[1][1]);
        argv[3][1] = tolower(argv[3][1]);
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

string getHead(string rest)
{
    string fsize;
    string operation;
    string path;
    string type;
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
        else if (i != 3 && rest[i] != '?' && flag == 0)
        {
            if (rest[i] == '%' && rest[i+1] == '2' && rest[i+2] == '0')
            {
                path += " ";
                i += 2;
                continue;
            }
            else
                path += rest[i];
        } //todo \t atd.
        else if (rest[i] == '?' && flag == 0) {
            flag = 1;
        } else if (flag == 1 && rest[i] == '=') flag = 2;
        else if (flag == 2 && rest[i] != ' ') type += rest[i];
        else if (rest[i] == ' ' && flag == 2) flag = 3;
        else if (rest[i] == '\n') countOfN++;
        else if (countOfN == 5) {
            if (isdigit(rest[i]))
                fsize += rest[i];
        }
        else if (rest[i-3] == '\r' && rest[i-2] == '\n' && rest[i-1] == '\r' && rest[i] == '\n')
        {
            rest.erase(0,i+1);
            break;
        }
    }
    cont->fsize     =   fsize;
    cont->operation =   operation;
    cont->path      =   path;
    cont->type      =   type;

    return rest;
}

string readSock(int socket){

    char buff[1024];
    string rcv;
    int n = 0;
    while(42) {
        bzero(buff, 1024);
        n = recv(socket, buff, 1024,0);
        if (n > 0) {
            rcv += string(buff,n);
        }
        else if (n == 0) {
            break;
        } else {
            fprintf(stderr, "UNKNOWN ERROR\n");
            exit(ERR);
        }
    }
    return rcv;
}

int writeSock(int socket, string buf)
{
    long size = buf.size();

    while (size > 0)
    {
        int n = (int) send(socket, (void*)buf.data(), buf.size(),0);
        shutdown(socket, SHUT_WR);
        if (n <= 0) {
            if (n == 0) {
                errMsg("ERROR: disconnected");
                return ERR;
            }
            else
            {
                errMsg("ERROR: nic nebylo zapsano");
                return ERR;
            }
        }
        size -= n;
    }

    return OK;
}
string createHeader(int type, string state, int size, string ctype)
{
    char buf[1024];
    string rest;
    time_t now = time(0);
    tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    stringstream ss;
    ss << size;
    string str = ss.str();
    switch (type)
    {
        case DEL:
            rest.append(state);
            rest += "Date: ";
            rest.append(buf);
            rest += "\r\nContent-Type: text/plain\r\nContent-Length: " + str + "\r\nContent-Encoding: identity\r\n\r\n";
            break;

        case RMD:
            rest.append(state);
            rest += "Date: ";
            rest.append(buf);
            rest += "\r\nContent-Type: text/plain\r\nContent-Length: " + str +  "\r\nContent-Encoding: identity\r\n\r\n";
            break;
        case GET:
            rest.append(state);
            rest += "Date: ";
            rest.append(buf);
            rest += "\r\nContent-Type: " + ctype + "\r\nContent-Length: " + str +  "\r\nContent-Encoding: identity\r\n\r\n";
            break;
        case LST:
            rest.append(state);
            rest += "Date: ";
            rest.append(buf);
            rest += "\r\nContent-Type: text/plain\r\nContent-Length: " + str+  "\r\nContent-Encoding: identity\r\n\r\n";

            break;
        case PUT:
            rest.append(state);
            rest += "Date: ";
            rest.append(buf);
            rest += "\r\nContent-Type: text/plain\r\nContent-Length: "+ str + "\r\nContent-Encoding: identity\r\n\r\n";
            break;

        case MKD:
            rest.append(state);
            rest += "Date: ";
            rest.append(buf);
            rest += "\r\nContent-Type: text/plain\r\nContent-Length: "+ str + "\r\nContent-Encoding: identity\r\n\r\n";
            break;
    }

    return rest;
}
int reqForPut(int sock, int type, string path, string fsize, string data)
{
    FILE *file;
    string err;
    if (type == PUT ) {
        if (fileOrFolder(path) == FI)
        {
            err = createHeader(type, forbidden, alrExists.length(), forbidden);
            err += alrExists;
            writeSock(sock, err);
            return OK;
        }
       //cout << path << endl;
        file = fopen(path.c_str(), "wb");
        if (file == NULL)
        {
            err = createHeader(type, intSerErr, unknownErr.length(), intSerErr);
            err += unknownErr;
            writeSock(sock, err);
            return ERR;
        }
        int size = (int) strtol(fsize.c_str(), NULL, 10);
        fwrite(data.c_str(),1,size, file);
        fclose(file);
        writeSock(sock, createHeader(type, ok, 0, ok));

    }
    else if ( type == MKD )
    {
        //todo mkdir, close socket, free?, exit, user handling

        int errH = mkdir(path.c_str(), S_IRWXU | S_IRWXG );
        if (errH == 0)
            writeSock(sock, createHeader(type, ok, 0, ok));
        else if ( errno == EEXIST ) {
            err = createHeader(type, badR, alrExists.length(), badR);
            err += alrExists;
            writeSock(sock, err);
        }
        else if ( errno == ENOENT)
        {
            err = createHeader(type, badR, dirNotFound.length(), badR);
            err += dirNotFound;
            writeSock(sock, err);
        }
        else
        {
            err = createHeader(type, badR, unknownErr.length(), badR);
            err += unknownErr;
            writeSock(sock, err);
        }

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
    string rest;
    string err;

    if(fileOrFolder(path) == FO && type == LST)
    {
        string a = "ls " + path;
        if ((mime = popen(a.c_str(), "r")) == NULL) {
            errMsg("ERROR popen neziskal ls");
            err = createHeader(type, forbidden, unknownErr.length(), "text/plain");
            err += unknownErr;
            writeSock(sock, err);
            return ERR;
        }

        char b[200];
        string mi;
        while(fgets(b, 200, mime)) {
            mi += b;
        }

        fsize = mi.length();
        rest = createHeader(type, ok, fsize, ok);
        rest += mi;
        writeSock(sock, rest);

    } else if (fileOrFolder(path) == FI && type == GET){

        file = fopen(path.c_str(), "rb");
        if ( file == NULL )
        {

            err = createHeader(type, forbidden, unknownErr.length(), "text/plain");
            err += unknownErr;
            writeSock(sock, err);

            return ERR;
        }

        if (fseek(file, 0, SEEK_END) == -1)
        {
            errMsg("ERROR nelze dojit na konec souboru");
            err = createHeader(type, forbidden, unknownErr.length(), "text/plain");
            err += unknownErr;
            writeSock(sock, err);
            return ERR;
        }

        fsize = ftell(file);
        if (fsize == -1)
        {
            errMsg("ERROR zadna velikost souboru");
            err = createHeader(type, forbidden, unknownErr.length(), "text/plain");
            err += unknownErr;
            writeSock(sock, err);
            return ERR;
        }



        rewind(file);

        content = (char*) malloc((size_t) fsize);
        if (!content)
        {
            errMsg("ERROR nenaalokoval se prostor pro file");
            fclose(file);
            err = createHeader(type, forbidden, unknownErr.length(), "text/plain");
            err += unknownErr;
            writeSock(sock, err);
            return ERR;
        }
        if (fread(content, fsize, 1, file) != 1)
        {
            errMsg("ERROR soubor nebyl nacten");
            free(content);
            fclose(file);
            err = createHeader(type, forbidden, unknownErr.length(), "text/plain");
            err += unknownErr;
            writeSock(sock, err);
            return ERR;
        }

        rest = createHeader(type, ok, fsize, getMIME(path));
        rest += string(content, fsize);
        free(content);
        writeSock(sock, rest);

        fclose(file);
    }
    else
    {
        if (fileOrFolder(path) == FI && type == LST)
        {
            err = createHeader(type, badR, notADir.length(), badR);
            err += notADir;
            writeSock(sock, err);
        }
        else if (fileOrFolder(path) == NOTH && type == LST)
        {
            err = createHeader(type, forbidden, dirNotFound.length(), forbidden);
            err += dirNotFound;
            writeSock(sock, err);
        }
        else if ( fileOrFolder(path) == FO && type == GET)
        {
            err = createHeader(type, badR, notAFile.length(), "text/plain");
            err += notAFile;
            writeSock(sock, err);
        }
        else if (fileOrFolder(path) == NOTH && type == GET)
        {
            err = createHeader(type, forbidden, fiNotFound.length(), "text/plain");
            err += fiNotFound;
            writeSock(sock, err);
        }
        else
        {
            err = createHeader(type, forbidden, unknownErr.length(), forbidden);
            err += unknownErr;
            writeSock(sock, err);
        }
    }

    return OK;
}
int reqForDel(int sock, string path, int type)
{
    string rest = createHeader(type, ok, 0 , ok);
    string err;
    int errH;
    if (type == DEL)
    {
        if (fileOrFolder(path) == FI){
            errH = remove(path.c_str());
            writeSock(sock, rest);
            return OK;
        }
        else if (fileOrFolder(path) == FO)
        {
            err = createHeader(type, badR, notAFile.length(), badR);
            err += notAFile;
            writeSock(sock, err);

        }
        else if (fileOrFolder(path) == NOTH)
        {
            err = createHeader(type, notF, fiNotFound.length(), notF);
            err += fiNotFound;
            writeSock(sock, err);
        }
        else
        {
            err = createHeader(type, forbidden, unknownErr.length(), forbidden);
            err += unknownErr;
            writeSock(sock, err);
        }
    }
    else if (type == RMD )
    {
        errH = rmdir(path.c_str());

        if (errH == 0)
        {
            writeSock(sock, rest);
            return OK;
        }
        else if (fileOrFolder(path) == NOTH)
        {
            err = createHeader(type, notF, dirNotFound.length(), notF);
            err += dirNotFound;
            writeSock(sock, err);
        }
        else if (fileOrFolder(path) == FI)
        {
            err = createHeader(type, badR, notADir.length(), badR);
            err += notADir;
            writeSock(sock,err);
        }
        else if (errno == ENOTEMPTY)
        {
            err = createHeader(type, forbidden, dirNotEmpty.length(), forbidden);
            err += dirNotEmpty;
            writeSock(sock, err);
        }

    }
    return OK;
}

void serveMe(int sock, struct arg * arg) {
    int n = 0;
    string operation;
    string path = arg->rootDir;
    string type;
    string fsize;
    string err;
    string rest = readSock(sock);
    rest = getHead(rest);
    string testUser = arg->rootDir;
    for (string::size_type i = 0; i < cont->path.length() ; i++)
    {
        if (i==0 || cont->path[i] != '/')
            testUser += cont->path[i];
        else
            break;
    }

    if (fileOrFolder(testUser) == NOTH )
    {
        if (cont->operation == "PUT" && cont->type == "file") {
            n = PUT;
        } else if (cont->operation == "GET" && cont->type == "file") {
            n = GET;
        } else if (cont->operation == "DEL" && cont->type == "file") {
            n = DEL;
        } else if (cont->operation == "PUT" && cont->type == "folder") {
            n = MKD;
        } else if (cont->operation == "DEL" && cont->type == "folder")
            n = RMD;
        else if ( cont->operation == "GET" && cont->type == "folder")
            n = LST;

        err = createHeader(n , notF, userNotFound.length(), "text/plain" );
        err += userNotFound;
        writeSock(sock, err);
        close(sock);
        delete arg;
        exit(OK);
    }

    path += cont->path;

    if (cont->operation == "PUT" && cont->type == "file") {
        //path += "/text.txt";
        n = reqForPut(sock, PUT, path, cont->fsize, rest);
    } else if (cont->operation == "GET" && cont->type == "file") {
        //path += "/k.deb";
        n = reqForGet(sock, path, GET);
    } else if (cont->operation == "DEL" && cont->type == "file") {
        //path += "/k.deb";
        n = reqForDel(sock, path, DEL);
    } else if (cont->operation == "PUT" && cont->type == "folder") {
        n = reqForPut(sock, MKD, path, cont->fsize, rest);
    } else if (cont->operation == "DEL" && cont->type == "folder")
    {
        n = reqForDel(sock, path, RMD);
    }
    else if ( cont->operation == "GET" && cont->type == "folder")
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
