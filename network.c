#include "stdafx.h"
#include "ttd.h"
#include "command.h"

#if defined(WIN32)
#	include <windows.h>
#	include <winsock.h>

# pragma comment (lib, "wsock32.lib")
# define ENABLE_NETWORK
#endif

#if defined(UNIX)
// Make compatible with WIN32 names
#	define ioctlsocket ioctl
#	define SOCKET int
#	define INVALID_SOCKET -1

// Need this for FIONREAD on solaris
#	define BSD_COMP
#	include <unistd.h>
#	include <sys/ioctl.h>

// Socket stuff
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#endif

typedef struct NetworkPacket {
	uint16 x;
	uint16 y;
	uint32 p1,p2;
	byte flags;
	byte procc;
	byte num_extra;
	byte unused;
} NetworkPacket;

#if defined(ENABLE_NETWORK)
static byte _packet_buf[sizeof(NetworkPacket) + 32];
static int _packet_buf_len;
static int _socket = -1;
#endif

static FILE *_recorder; 
static FILE *_playback;

typedef struct TotalNetworkPacket {
	NetworkPacket np;
	uint32 dp[8];
} TotalNetworkPacket;

void NetworkSend(int x, int y, uint32 p1, uint32 p2, uint flags, uint procc)
{
#if defined(ENABLE_NETWORK)
	TotalNetworkPacket t;
	int packet_size;

	if (_socket == -1 && _recorder == NULL)
		return;

	t.np.x = x;
	t.np.y = y;
	t.np.p1 = p1;
	t.np.p2 = p2;
	t.np.flags = (byte)flags;
	t.np.procc = (byte)procc;
	t.np.unused = 0;
	
	packet_size = 8;
	while ( packet_size != 0 && ((uint32*)_decode_parameters)[packet_size-1] == 0) packet_size--;
	t.np.num_extra = packet_size;

	if (packet_size != 0) {
		packet_size *= sizeof(uint32);
		memcpy(t.dp, _decode_parameters, packet_size);
	}

	packet_size += sizeof(NetworkPacket);

	if (_socket != -1)
		if (send(_socket, (const char*)&t, packet_size, 0) != packet_size)
			error("Network error. Good bye, houston!");

	if (_recorder != NULL)
		fwrite(&t, packet_size, 1, _recorder);
#endif
}

void HandlePacket(NetworkPacket *np)
{
	assert(np->num_extra <= 8);

	memset(_decode_parameters, 0, sizeof(_decode_parameters));
	if (np->num_extra)
		memcpy(_decode_parameters, (byte*)np + sizeof(*np), np->num_extra * sizeof(uint32));
			
	if (DoCommand(np->x, np->y, np->p1, np->p2, np->flags, np->procc) == CMD_ERROR)
		printf("Recieved packet that failed.\n");
}


#define NETWORK_BUFFER_SIZE 1024

void NetworkPoll()
{
#if defined(ENABLE_NETWORK)
	byte network_buffer[NETWORK_BUFFER_SIZE];
	uint pos;
	unsigned long read_count, recv_bytes;
	uint size;
	uint packet_size;

	if (_socket == -1)
		return;

	if (ioctlsocket(_socket, FIONREAD, &read_count) != 0)
		error("ioctlsocket failed.");

	if (read_count == 0)
		return;

	size = 0;
	if (_packet_buf_len != 0) {
		size = _packet_buf_len;
		memcpy(network_buffer, _packet_buf, size);
	}

	if (read_count > NETWORK_BUFFER_SIZE - size)
		read_count = NETWORK_BUFFER_SIZE - size;

	recv_bytes = recv(_socket, (char*)network_buffer + size, read_count, 0);

	if ( recv_bytes == -1) {
		char buf[256];
		sprintf(buf, "recv failed recv_bytes=%ld, read_count=%ld.", recv_bytes, read_count);
		error(buf);
	}

	size += read_count;

	_current_player = 1; // TODO

	pos = 0;

	for(;;) {
		NetworkPacket *packet;

		if (size < sizeof(NetworkPacket))
			break;

		packet = ((NetworkPacket*) (network_buffer + pos));
		packet_size = sizeof(NetworkPacket) + packet->num_extra * sizeof(uint32);

		if (size < packet_size)
			break;
	
		size -= packet_size;
		pos += packet_size;

		HandlePacket( packet );
	}

	_current_player = 0;

	assert(size>=0 && size < sizeof(_packet_buf));

	_packet_buf_len = size;
	memcpy(_packet_buf, network_buffer + pos, size);
#else
	return;
#endif
}


void RecorderPoll()
{
	TotalNetworkPacket t;

	if (_playback == NULL)
		return;

	for(;;) {
		if (fread(&t.np, sizeof(NetworkPacket), 1, _playback) != 1 ||
				(t.np.num_extra != 0 && fread(&t.dp, t.np.num_extra*4, 1, _playback) != 1)) {
			fclose(_playback);
			_playback = NULL;
			return;
		}
		HandlePacket(&t.np);
	}
}

void NetworkConnect(const char *hostname, int port)
{
#if defined(ENABLE_NETWORK)
	SOCKET s;
	struct sockaddr_in sin;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		error("socket() failed");
	
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(hostname);
	sin.sin_port = htons(port);

	if (connect(s, (struct sockaddr*) &sin, sizeof(sin)) != 0)
		error("connect() failed");

	_socket = s;
#endif
}

void NetworkListen(int port)
{
#if defined(ENABLE_NETWORK)
	SOCKET ls, s;
	struct sockaddr_in sin;
	int sin_len;

	ls = socket(AF_INET, SOCK_STREAM, 0);
	if (ls == -1)
		error("socket() on listen socket failed");

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(port);

	if (bind(ls, (struct sockaddr*)&sin, sizeof(sin)) != 0)
		error("bind() failed");

	if (listen(ls, 1) != 0)
		error("listen() failed");

	sin_len = sizeof(sin);
	s = accept(ls, (struct sockaddr*)&sin, &sin_len);
	if (s == INVALID_SOCKET)
		error("accept() failed");

	_socket = s;
#endif
}


void NetworkInitialize()
{
#if defined(ENABLE_NETWORK)
#if !defined(__GNUC__)
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,0), &wsa) != 0)
		error("WSAStartup failed");
#endif
#endif
}


bool RecorderStart(const char *file)
{
	if (_recorder != NULL)
		return false;

	_recorder = fopen(file, "wb");
	if (_recorder == NULL)
		return false;
	return true;
}

bool RecorderStop()
{
	if (_recorder == NULL)
		return false;

	fclose(_recorder);
	_recorder = NULL;
	return true;
}

bool RecorderPlayback(const char *file)
{
	if (_playback != NULL || _recorder != NULL)
		return false;

	_playback = fopen(file, "rb");
	if (_playback == NULL)
		return false;
	return true;
}

