// ORIGINAL EDITED

/* http://www.brokestream.com/udp_redirect.html

  Build: gcc -o udp_redirect udp_redirect.c

  udp_redirect.c
  Version 2008-11-09

  Copyright (C) 2007 Ivan Tikhonov

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Ivan Tikhonov, kefeer@brokestream.com

*/
#include <stdio.h>
#include <stdlib.h>
			
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
    #include <winsock2.h>
    #include <Windows.h>
	#define sock_t SOCKET
#else
    #include <err.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
	#define sock_t int	
#endif

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 6) {
        printf("Usage: %s our-ip our-port send-to-ip send-to-port obfs-key\n", argv[0]);
        printf("Usage: %s our-ip our-port             # echo mode\n", argv[0]);
        exit(1);
    }
    
    int key_length = strlen(argv[5]);
    //char key[key_length];
    // Should really be 65535 bytes...
    char* key = argv[5];


#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }
#endif

    // Init socket
    sock_t os = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    struct sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = inet_addr(argv[1]);
    a.sin_port = htons(atoi(argv[2]));
    
    // Try to bind to local port
    if(bind(os, (struct sockaddr *)&a, sizeof(a)) == -1) {
#ifdef _WIN32
        printf("Can't bind our address (%s:%s). Error: %u\n", argv[1], argv[2], WSAGetLastError());
#else
        err(EXIT_FAILURE, NULL);
#endif
        exit(1);
    }

    // TODO: Remove the echo service - although it could be useful for internal application/scripting use
    if(argc == 5) {
        a.sin_addr.s_addr = inet_addr(argv[3]);
        a.sin_port = htons(atoi(argv[4]));
    }

    struct sockaddr_in sa; // The source address of the incoming packet
    struct sockaddr_in da; // The address of the connection initiator
    da.sin_addr.s_addr = 0;
    
    while(1) {
        char buf[65535];
        int sn = sizeof(sa);
        int n = recvfrom(os, buf, sizeof(buf), 0, (struct sockaddr *)&sa, &sn);
        if(n <= 0) continue;

		int i;
        for(i = 0; i < n; i++)
        {
            // Encrypt/decrypt (XOR) in place - no buffers
            buf[i] = buf[i] ^ key[i%key_length];
        }

        if(argc == 3) {
            // Echo mode
            sendto(os, buf, n, 0, (struct sockaddr *)&sa, sn);
        } else if(sa.sin_addr.s_addr == a.sin_addr.s_addr && sa.sin_port == a.sin_port) { // Packet coming from destination
            // Check if we actually know what the incoming/local address is
            if(da.sin_addr.s_addr) {
                // Send packet to connection initiator
                sendto(os, buf, n, 0, (struct sockaddr *)&da, sizeof(da));
            }
        } else { // Packet coming from source
            // Send packet to destination port
            sendto(os, buf, n, 0, (struct sockaddr *)&a, sizeof(a));
            // Save address of connection initiator
            da = sa;
        }
    }
}