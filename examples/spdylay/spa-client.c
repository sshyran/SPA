#include <stdio.h>
#include <strings.h>
#include <assert.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <spdylay/spdylay.h>

#include <spa/spaRuntime.h>

#define SPDY_VERSION		SPDYLAY_PROTO_SPDY3
#define REQUEST_METHOD		"GET"
#define REQUEST_SCHEME		"http"
#define REQUEST_PATH		"/"
#define REQUEST_HOST		"127.0.0.1"
#define REQUEST_PORT		6121
#define REQUEST_VERSION		"HTTP/1.1"
#define REQUEST_PRIORITY	3
#define RECEIVE_BUFFER_SIZE	1500

#define QUOTE( str ) #str
#define EXPAND_AND_QUOTE( str ) QUOTE( str )
#define REQUEST_HOSTPATH	REQUEST_HOST ":" EXPAND_AND_QUOTE( REQUEST_PORT )


int sock = -1;

ssize_t send_callback( spdylay_session *session, const uint8_t *data, size_t length, int flags, void *user_data ) {
	printf( "Sending %d bytes of data for %s\n", (int) length, (char *) user_data );

	spa_msg_output( data, length, 1024, "request" );
#ifndef ENABLE_KLEE
	assert( send( sock, data, length, 0 ) == length );
#endif // #ifndef ENABLE_KLEE	

	return length;
}

void ctrl_recv_callback( spdylay_session *session, spdylay_frame_type type, spdylay_frame *frame, void *user_data ) {
	switch ( type ) {
		case SPDYLAY_SYN_STREAM: { // The SYN_STREAM control frame.
			printf( "Received SYN_STREAM control frame on %s.\n", (char *) user_data );
			printf( "	Stream ID: %d.\n", ((spdylay_syn_stream *) frame)->stream_id );
			printf( "	Associated-to-stream ID: %d.\n", ((spdylay_syn_stream *) frame)->assoc_stream_id );
			printf( "	Priority: %d.\n", ((spdylay_syn_stream *) frame)->pri );
			printf( "	Headers:\n" );
			char **nv = ((spdylay_syn_stream *) frame)->nv;
			assert( nv );
			while ( nv[0] ) {
				assert( nv[1] );
				printf( "		%s: %s\n", nv[0], nv[1] );
				nv += 2;
			}
			break;
		}
		case SPDYLAY_SYN_REPLY: { // The SYN_REPLY control frame.
			printf( "Received SYN_REPLY control frame on %s.\n", (char *) user_data );
			printf( "	Stream ID: %d.\n", ((spdylay_syn_reply *) frame)->stream_id );
			printf( "	Headers:\n" );
			char **nv = ((spdylay_syn_reply *) frame)->nv;
			assert( nv );
			while ( nv[0] ) {
				assert( nv[1] );
				printf( "		%s: %s\n", nv[0], nv[1] );
				nv += 2;
			}
			break;
		}
		case SPDYLAY_RST_STREAM: // The RST_STREAM control frame.
			printf( "Received RST_STREAM control frame on %s.\n", (char *) user_data );
			break;
		case SPDYLAY_SETTINGS: // The SETTINGS control frame.
			printf( "Received SETTINGS control frame on %s.\n", (char *) user_data );
			break;
		case SPDYLAY_NOOP: // The NOOP control frame. This was deprecated in SPDY/3.
			printf( "Received NOOP control frame on %s.\n", (char *) user_data );
			break;
		case SPDYLAY_PING: // The PING control frame.
			printf( "Received PING control frame on %s.\n", (char *) user_data );
			break;
		case SPDYLAY_GOAWAY: // The GOAWAY control frame.
			printf( "Received GOAWAY control frame on %s.\n", (char *) user_data );
			break;
		case SPDYLAY_HEADERS: // The HEADERS control frame.
			printf( "Received HEADERS control frame on %s.\n", (char *) user_data );
			break;
		case SPDYLAY_WINDOW_UPDATE: // The WINDOW_UPDATE control frame. This first appeared in SPDY/3.
			printf( "Received WINDOW_UPDATE control frame on %s.\n", (char *) user_data );
			break;
		case SPDYLAY_CREDENTIAL: // The CREDENTIAL control frame. This first appeared in SPDY/3.
			printf( "Received CREDENTIAL control frame on %s.\n", (char *) user_data );
			break;
		default:
			printf( "Received unknown control frame type on %s.\n", (char *) user_data );
			break;
	}
}

void data_chunk_recv_callback( spdylay_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data ) {
	printf( "Received data on stream %d of %s: \"%s\"\n", stream_id, (char *) user_data, data );
}

void __attribute__((noinline,used)) spa_SendRequest() {
	spa_api_entry();
// 	spa_api_input_var();

	struct hostent *serverHost;
	struct sockaddr_in serverAddr;

	assert( (serverHost = gethostbyname( REQUEST_HOST )) != NULL && "Host not found." );
	bzero( &serverAddr, sizeof( serverAddr ) );
	serverAddr.sin_family = AF_INET;
	bcopy( serverHost->h_addr, &serverAddr.sin_addr.s_addr, serverHost->h_length );
	serverAddr.sin_port = htons( REQUEST_PORT );

	assert( (sock = socket( AF_INET, SOCK_STREAM, 0 )) >= 0 && "Error opening socket." );
	assert( connect( sock, (struct sockaddr *) &serverAddr, sizeof( serverAddr ) ) >= 0 && "Error connecting to host." );
// 	assert( fcntl( sock, F_SETFL, fcntl( sock, F_GETFL ) | O_NONBLOCK ) >= 0 && "Error setting non-blocking socket option." );
	printf( "Connected to remote host.\n" );

	spdylay_session_callbacks callbacks;
	callbacks.send_callback = send_callback; // Callback function invoked when the |session| wants to send data to the remote peer.
	callbacks.recv_callback = NULL; // Callback function invoked when the |session| wants to receive data from the remote peer.
	callbacks.on_ctrl_recv_callback = ctrl_recv_callback; // Callback function invoked by `spdylay_session_recv()` when a control frame is received.
	callbacks.on_invalid_ctrl_recv_callback = NULL; // Callback function invoked by `spdylay_session_recv()` when an invalid control frame is received.
	callbacks.on_data_chunk_recv_callback = data_chunk_recv_callback; // Callback function invoked when a chunk of data in DATA frame is received.
	callbacks.on_data_recv_callback = NULL; // Callback function invoked when DATA frame is received.
	callbacks.before_ctrl_send_callback = NULL; // Callback function invoked before the control frame is sent.
	callbacks.on_ctrl_send_callback = NULL; // Callback function invoked after the control frame is sent.
	callbacks.on_ctrl_not_send_callback = NULL; // The callback function invoked when a control frame is not sent because of an error.
	callbacks.on_data_send_callback = NULL; // Callback function invoked after DATA frame is sent.
	callbacks.on_stream_close_callback = NULL; // Callback function invoked when the stream is closed.
	callbacks.on_request_recv_callback = NULL; // Callback function invoked when request from the remote peer is received.
	callbacks.get_credential_proof = NULL; // Callback function invoked when the library needs the cryptographic proof that the client has possession of the private key associated with the certificate.
	callbacks.get_credential_ncerts = NULL; // Callback function invoked when the library needs the length of the client certificate chain.
	callbacks.get_credential_cert = NULL; // Callback function invoked when the library needs the client certificate.
	callbacks.on_ctrl_recv_parse_error_callback = NULL; // Callback function invoked when the received control frame octets could not be parsed correctly.
	callbacks.on_unknown_ctrl_recv_callback = NULL; // Callback function invoked when the received control frame type is unknown.

	spdylay_session *session;
	assert( spdylay_session_client_new( &session, SPDY_VERSION, &callbacks, "SPDY Client Session" ) == SPDYLAY_OK );

// 	assert( spdylay_session_set_initial_client_cert_origin( session, REQUEST_SCHEME, REQUEST_HOST, REQUEST_PORT ) == SPDYLAY_OK );

	const char *nv[] = {
		":method",	REQUEST_METHOD,
		":scheme",	REQUEST_SCHEME,
		":path",	REQUEST_PATH,
		":version",	REQUEST_VERSION,
		":host",	REQUEST_HOSTPATH,
		NULL
	};
	assert( spdylay_submit_request( session, REQUEST_PRIORITY, nv, NULL, "SPDY Client Stream 1" ) == SPDYLAY_OK );
	
	assert( spdylay_session_send( session ) == SPDYLAY_OK );

	unsigned char buf[RECEIVE_BUFFER_SIZE];
	ssize_t len = 0;
	do {
#ifndef ENABLE_KLEE
		len = recv( sock, buf, RECEIVE_BUFFER_SIZE, 0 /*MSG_DONTWAIT*/ );
#endif // #ifndef ENABLE_KLEE	
// 		spa_msg_input( buf, RECEIVE_BUFFER_SIZE, "response" );
		assert( len >= 0 && "Error receiving network data." );
		if ( len > 0 ) {
			printf( "Received %d bytes of data.\n", (int) len );
			assert( spdylay_session_mem_recv( session, buf, len) == len );
		}
#ifdef ENABLE_SPA
	} while ( 0 );
#else // #ifdef ENABLE_SPA
	} while ( len > 0 );
#endif // #ifdef ENABLE_SPA #else

	printf( "Closing down session.\n" );
	spdylay_session_del( session );
}

int main( int argc, char **argv ) {
	spa_SendRequest();

	return 0;
}
