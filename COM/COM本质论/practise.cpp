struct tagVARIANT
    {   
    union 
        {
        struct __tagVARIANT  
            {
            VARTYPE vt;
            WORD wReserved1;
            WORD wReserved2;
            WORD wReserved3;
            union 
                {
                LONGLONG llVal;
                LONG lVal;
                BYTE bVal;
                SHORT iVal;
                FLOAT fltVal;
                DOUBLE dblVal;
                VARIANT_BOOL boolVal;
                _VARIANT_BOOL bool;
                SCODE scode;  ddd
                CY cyVal;
                DATE date;
                BSTR bstrVal;
                IUnknown *punkVal;
                IDispatch *pdispVal;
                SAFEARRAY *parray;
                BYTE *pbVal;
                SHORT *piVal;
                LONG *plVal;
                LONGLONG *pllVal;
                FLOAT *pfltVal;
                DOUBLE *pdblVal;
                VARIANT_BOOL *pboolVal;
                _VARIANT_BOOL *pbool;
                SCODE *pscode;
                CY *pcyVal;
                DATE *pdate;
                BSTR *pbstrVal;
                IUnknown **ppunkVal;
                IDispatch **ppdispVal;
                SAFEARRAY **pparray;
                VARIANT *pvarVal;
                PVOID byref;
                CHAR cVal;
                USHORT uiVal;
                ULONG ulVal;
                ULONGLONG ullVal;
                INT intVal;
                UINT uintVal;
                DECIMAL *pdecVal;
                CHAR *pcVal;
                USHORT *puiVal;
                ULONG *pulVal;
                ULONGLONG *pullVal;
                INT *pintVal;
                UINT *puintVal;
                struct __tagBRECORD
                    {
                    PVOID pvRecord;
                    IRecordInfo *pRecInfo;
                    } 	__VARIANT_NAME_4;
                } 	__VARIANT_NAME_3;
            } 	__VARIANT_NAME_2;
        DECIMAL decVal;
        } 	__VARIANT_NAME_1;
    } ;
typedef VARIANT *LPVARIANT;





#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define DEFAULT_FAMILY  AF_UNSPEC
#define DEFAULT_SOCKTYPE SOCK_STREAM
#define DEFAULT_PORT  "1234"
#define BUFFER_SIZE   23    // length of "WinCE Echo Test Packet"

void
Print(
    TCHAR *pFormat, 
...)
{
    va_list ArgList;
    TCHAR Buffer[256];
    va_start (ArgList, pFormat);
    (void)StringCchPrintf(Buffer, 256, pFormat, ArgList);

#ifndef UNDER_CE
    _putts(Buffer);
#else
    printf("%s",Buffer);
#endif

    va_end(ArgList);
}

int _tmain (int argc, TCHAR* argv[])
{
    SOCKET sock, SockServ[FD_SETSIZE];
    int nFamily = DEFAULT_FAMILY;
    int nSockType = DEFAULT_SOCKTYPE;
    char *szPort = DEFAULT_PORT;
    SOCKADDR_STORAGE ssRemoteAddr;
    int i, nNumSocks, cbRemoteAddrSize, cbXfer, cbTotalRecvd;
    WSADATA wsaData;
    ADDRINFO Hints, *AddrInfo = NULL, *AI;
    fd_set fdSockSet;
    char pBuf[BUFFER_SIZE];
    char szRemoteAddrString[128];

    if(WSAStartup(MAKEWORD(2,2), &wsaData))
    {
        // WSAStartup failed
        return 1;
    }

    sock = INVALID_SOCKET;

    for(i = 0; i < FD_SETSIZE; i++)
        SockServ[i] = INVALID_SOCKET;

    //
    // Get a list of available addresses to serve on
    //

    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = nFamily;
    Hints.ai_socktype = nSockType;
    Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;

    if(getaddrinfo(NULL, szPort, &Hints, &AddrInfo))
    {
        Print(TEXT("ERROR: getaddrinfo failed with error %d\r\n"), WSAGetLastError());
        goto Cleanup;
    }

    //
    // Create a list of serving sockets, one for each address
    //

    i = 0;
    for(AI = AddrInfo; AI != NULL; AI = AI->ai_next) 
    {
        if (i == FD_SETSIZE) 
        {
            // getaddrinfo returned more addresses than we could use
            break;
        }

        if((AI->ai_family == PF_INET) || (AI->ai_family == PF_INET6)) // only want PF_INET or PF_INET6
        {
            SockServ[i] = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
            if (SockServ[i] != INVALID_SOCKET)
            {
                if (bind(SockServ[i], AI->ai_addr, AI->ai_addrlen) == SOCKET_ERROR)
                    closesocket(SockServ[i]);
                else 
                {
                    if(nSockType == SOCK_STREAM)
                    {
                        if (listen(SockServ[i], 5) == SOCKET_ERROR)
                        {
                            closesocket(SockServ[i]);
                            continue;
                        }
                    }

                    Print( 
                        TEXT("Socket 0x%08x ready for connection with %hs family, %hs type, on port %hs\r\n"), 
                        SockServ[i], 
                        (AI->ai_family == AF_INET) ? "AF_INET" : ((AI->ai_family == AF_INET6) ? "AF_INET6" : "UNKNOWN"),
                        (AI->ai_socktype == SOCK_STREAM) ? "TCP" : ((AI->ai_socktype == SOCK_DGRAM) ? "UDP" : "UNKNOWN"),
                        szPort);
                    i++;
                }
            }
        }
    }
    
    freeaddrinfo(AddrInfo);
    
    if (i == 0) 
    {
        Print(TEXT("ERROR: Unable to serve on any address. Error = %d\r\n"), WSAGetLastError());
        goto Cleanup;
    }

    //
    // Wait for incomming data/connections
    //

    nNumSocks = i;

    FD_ZERO(&fdSockSet);

    for (i = 0; i < nNumSocks; i++)    // want to check all available sockets
        FD_SET(SockServ[i], &fdSockSet);

    if (select(nNumSocks, &fdSockSet, 0, 0, NULL) == SOCKET_ERROR)
    {
        Print(TEXT("ERROR: select() failed with error = %d\r\n"), WSAGetLastError());
        goto Cleanup;
    }

    for (i = 0; i < nNumSocks; i++)    // check which socket is ready to process
    {
        if (FD_ISSET(SockServ[i], &fdSockSet))    // proceed for connected socket
        {
            FD_CLR(SockServ[i], &fdSockSet);
            if(nSockType == SOCK_STREAM)
            {
                cbRemoteAddrSize = sizeof(ssRemoteAddr);
                sock = accept(SockServ[i], (SOCKADDR*)&ssRemoteAddr, &cbRemoteAddrSize);
                if(sock == INVALID_SOCKET) 
                {
                    Print(TEXT("ERROR: accept() failed with error = %d\r\n"), WSAGetLastError());
                    goto Cleanup;
                }

                Print(TEXT("Accepted TCP connection from socket 0x%08x\r\n"), sock);
            }
            else
            {
                sock = SockServ[i];
                Print(TEXT("UDP data available on socket 0x%08x\r\n"), sock);
            }
            break;        // Only need one socket
        }
    }

    //
    // Receive data from a client
    //

    cbTotalRecvd = 0;
    do
    {
        cbRemoteAddrSize = sizeof(ssRemoteAddr);
        cbXfer = recvfrom(sock, pBuf + cbTotalRecvd, sizeof(pBuf) - cbTotalRecvd, 0,
            (SOCKADDR *)&ssRemoteAddr, &cbRemoteAddrSize);
        cbTotalRecvd += cbXfer;
    } while(cbXfer > 0 && cbTotalRecvd < sizeof(pBuf));

    if(cbXfer == SOCKET_ERROR)
    {
        Print(TEXT("ERROR: Couldn't receive the data! Error = %d\r\n"), WSAGetLastError());
        goto Cleanup;
    }
    else if(cbXfer == 0)
    {
        Print(TEXT("ERROR: Didn't get all the expected data from the client!\r\n"));
        goto Cleanup;
    }

    if(nSockType == SOCK_STREAM)
    {
        cbRemoteAddrSize = sizeof(ssRemoteAddr);
        getpeername(sock, (SOCKADDR *)&ssRemoteAddr, &cbRemoteAddrSize);
    }
    
    if (getnameinfo((SOCKADDR *)&ssRemoteAddr, cbRemoteAddrSize,
        szRemoteAddrString, sizeof(szRemoteAddrString), NULL, 0, NI_NUMERICHOST) != 0)
        strcpy(szRemoteAddrString, "");

    Print(TEXT("SUCCESS - Received %d bytes from client %hs\r\n"), cbTotalRecvd, szRemoteAddrString);

    //
    // Echo the data back to the client
    //

    cbXfer = 0;
    cbXfer = sendto(sock, pBuf, cbTotalRecvd, 0, (SOCKADDR *)&ssRemoteAddr, cbRemoteAddrSize);

    if(cbXfer != cbTotalRecvd)
        Print(TEXT("ERROR: Couldn't send the data! error = %d\r\n"), WSAGetLastError());
    else
        Print(TEXT("SUCCESS - Echo'd %d bytes back to the client\r\n"), cbXfer);

Cleanup:

    for(i = 0; i < nNumSocks && SockServ[i] != INVALID_SOCKET; i++)
        closesocket(SockServ[i]);

    if(sock != INVALID_SOCKET)
    {
        shutdown(sock, SD_BOTH);
        closesocket(sock);
    }

    WSACleanup();

    return 0;
}


#define sysmon_if(__ret__, __func__, ...) \
	__ret__ sysmon_decl __func__(__VA_ARGS__); \
	typedef __ret__ (sysmon_decl * __func__ ## _ptr)(__VA_ARGS__);
	
	pid_t * 

static define_event_handler(exec_handler)
{
        INFO(MON_exec_monitor) *info;
        info = (INFO(MON_exec_monitor) *)header;
        printf("pid=%d, parent_pid=%d, pathname=%ws, cmdline=%ws\n",
               (int)info->pid, (int)info->parent_pid,
               info->pathname, info->cmdline);
        return UV_pass;
}

void handle_image_exec()
{
        struct monitor *m = get_monitor(MON_exec_monitor);

        assert(monitor_reg_event_handler(m, exec_handler, &proc_count) >= 0);
        assert(monitor_set_state(m, MS_on) >= 0);

        // wait...

        monitor_set_state(m, MS_off);
}












