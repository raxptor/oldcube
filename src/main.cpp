#include <uv.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

uv_loop_t *loop = 0;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf) 
{
    *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

void echo_read(uv_stream_t *server, ssize_t nread, const uv_buf_t* buf) 
{
    if (nread == -1) 
    {
        fprintf(stderr, "error echo_read");
        return;
    }
             
    printf("result: %ld\n", nread);
}

void on_new_connection(uv_stream_t *server, int status) 
{
    if (status == -1) 
    {
        fprintf(stderr, "failed!\n");
        return;
    }
    
    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) 
    {
        uv_read_start((uv_stream_t*) client, alloc_buffer, echo_read);
    }
    else
    {
        uv_close((uv_handle_t*)client, 0);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("turbo: missing socket argument\n");
        return 1;
    }
    
    printf("connectig on [%s]\n", argv[1]);
        
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("turbo: error creating socket\n");
        return -1;
    }
    
    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, argv[1]);
    if (connect(sock, (struct sockaddr*) &server, sizeof(struct sockaddr_un)))
    {
        printf("turbo: error connecting\n");
        return -1;
    }
    
    write(sock, "apa", 3);
    printf("wrote 3 bytes\n");
    
    close(sock);
    
/*
    loop = uv_default_loop();
    
    const int port = 9860;
    
    uv_tcp_t server;
    uv_tcp_init(loop, &server);
    
    struct sockaddr_in bind_addr;
    int ret = uv_ip4_addr("0.0.0.0", 7000, &bind_addr);
    
    uv_tcp_bind(&server, (const struct sockaddr*)&bind_addr, 0);
    
    int r = uv_listen((uv_stream_t*) &server, port, on_new_connection);
    if (r) 
    {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }

    printf("turbonet: entering main loop\n");
    uv_run(loop, UV_RUN_DEFAULT);   
*/
    return 0;
}
