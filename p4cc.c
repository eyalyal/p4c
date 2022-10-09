#include <stdio.h>
#include <winsock2.h>		//윈도우 소켓 사용

#pragma comment (lib,"ws2_32.lib")		//#pragma comment 전처리기 구문을 이용해 라이브러리 참조하게 해줌

static void s_handle_client(int client_sock, struct sockaddr_in * cli_addr, int cli_size);

#ifndef __HTTP_PARSER_H__		
#define __HTTP_PARSER_H__

typedef struct _http_request_line_t {		//메소드,uri,프로토콜 담을 구조체 형성
	char method[100];
	char uri[4096];
	char protocol[512];
} http_request_line_t;

typedef struct _http_header_field_t {
	char name[100];
	char value[512];
	struct _http_header_field_t * next;
} http_header_field_t;

typedef struct _http_header_fields_t {
	http_header_field_t * first;
} http_header_fields_t;

extern const char * parseHttpRequestLine(const char * header, http_request_line_t * line);
extern void parseHttpHeader(const char * header, http_header_fields_t * fields);
extern char * getHeaderField(http_header_fields_t * fields, const char * name);
extern void releaseHeaderFields(http_header_fields_t * fields);

#endif

int main(){
	SOCKET		s,cs;	//서버와 클라이언트의 소켓 핸들(시스템으로부터 할당 받은 파일이나 소켓을 대표하는 정수) 구조체
	WSADATA		wsaData;	//윈속 정보를 담을 구조체
	SOCKADDR_IN		sin, cli_addr;		//서버와 클라이언트의 소켓 주소정보를 담을 구조체


	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){	
													
		printf("WSAStartup 실패, 에러코드: %d\n",WSAGetLastError());	
		return 0;
	}		


	s=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//윈도우 소켓 생성이 실패했을 때
	if (s==INVALID_SOCKET){
		printf("소켓 생성 실패, 에러코드 : %d\n", WSAGetLastError());
		WSACleanup();		
		return 0;
	}

	
	sin.sin_family = AF_INET;		//사용하는 주소체계 명시
	sin.sin_port = htons(8080);		//서버 포트번호 설정
	sin.sin_addr.S_un.S_addr = htonl(ADDR_ANY);		//ADDR_ANY:모든 클라이언트 접속 허용

	
	if (bind(s, (SOCKADDR*)&sin, sizeof(sin))==SOCKET_ERROR){
		printf("bind 실패, 에러코드:%d\n", WSAGetLastError());
		closesocket(s);		//방금 생성했던 소켓 제거
		WSACleanup();
		return 0;
	}

	
	if (listen(s, 10) !=0){
		printf("listen 모드 설정 실패, 에러코드:%d\n", WSAGetLastError());
		closesocket(s);
		WSACleanup();
		return 0;
	}
	printf("서버를 시작합니다\n");
	printf("클라이언트로부터의 접속을 기다리고 있습니다\n");

	int cli_size = sizeof(cli_addr);	

	
	while(1){
		cs = accept(s, (SOCKADDR*)&cli_addr, &cli_size);
		if (cs == INVALID_SOCKET){
			printf("접속 승인 실패, 에러코드:%d\n", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			return 0;
		}
		printf("클라이언트와 연결되었습니다\n");
		s_handle_client(s, &cli_addr, cli_size);		//http 구현 함수를 불러온다.
	}
	

}
//http
static void s_handle_client(int client_sock, struct sockaddr_in * cli_addr, int cli_size) {
	int read_len = -1;
	char header_buffer[2048] = {0,};
	size_t p = 0;
	http_request_line_t request = {0,};
	http_header_fields_t fields = {0,};
	char response_header[4096] = {0,};
	char response_content[4096] = {0,};
	char client_ip_address[128] = {0,};


	while (p < sizeof(header_buffer) && (read_len = recv(client_sock, header_buffer + p, 1, 0)) > 0) {
		if (p > 4 &&
			header_buffer[p - 3] == '\r' && header_buffer[p - 2] == '\n' &&
			header_buffer[p - 1] == '\r' && header_buffer[p] == '\n') {
			break;
		}
		p++;
		read_len = -1;
	}

	if (read_len < 0) {
		printf("socket closed\n");
		return;
	}

	inet_ntop(cli_addr->sin_family, &cli_addr->sin_addr, client_ip_address, sizeof(client_ip_address));		
	
	parseHttpHeader(parseHttpRequestLine(header_buffer, &request), &fields);

	snprintf(response_content, sizeof(response_content), "<h1>Your Request Information</h1>"
			 "<p>You IP Address: %s:%d</p>"
			 "<ul>"
			 "<li>Method: %s</li>"
			 "<li>Path: %s</li>"
			 "<li>Protocol: %s</li>"
			 "</ul>", client_ip_address, ntohs(cli_addr->sin_port), request.method, request.uri, request.protocol);

	snprintf(response_header, sizeof(response_header), "HTTP/1.1 200 OK\r\n"
			 "Content-Type: text/html\r\n"
			 "Content-Length: %ld\r\n"
			 "\r\n", strlen(response_content));



	send(client_sock, response_header, strlen(response_header), 0);
	send(client_sock, response_content, strlen(response_content), 0);

	releaseHeaderFields(&fields);

	closesocket(client_sock);
}