#include <uv.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <netki/types/cube.h>
#include <netki/bitstream.h>

uv_loop_t *loop = 0;
int cubesock = 0;
int connection_id_counter = 0;

struct cube_tcp_client
{
    uv_tcp_t stream;
    int connection_id;
};

template<typename T>
bool write_packet(T *pkt, netki::bitstream::buffer *buf)
{
    netki::bitstream::insert_bits<16>(buf, T::type_id());
    if (buf->error)
        return false;
    return T::write_into_bitstream(pkt, buf);
}

template<typename T, int bufsize>
void send_packet(T *pkt)
{
    char data[bufsize];
    netki::bitstream::buffer buf;
    netki::bitstream::init_buffer(&buf, data);
    write_packet(pkt, &buf);
    netki::bitstream::flip_buffer(&buf);
    write(cubesock, buf.buf, buf.bufsize);
}

template<typename T>
inline void send_packet(T *pkt)
{
    send_packet<T, 1024>(pkt);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf) 
{
    *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

void on_close_tcp(uv_handle_t *handle)
{
    cube_tcp_client *cc = (cube_tcp_client *)handle;
    netki::CubeClientDisconnect disco;
    disco.ConnectionId = cc->connection_id;
    send_packet(&disco);
    delete cc;
}

void on_read_tcp(uv_stream_t *client, ssize_t nread, const uv_buf_t* buf) 
{
    cube_tcp_client *cc = (cube_tcp_client *)client;
    
    if (nread > 0)
    {
        netki::CubeClientData data;
        data.ConnectionId = cc->connection_id;
        data.Data = (uint8_t*)buf->base;
        data.Data_size = nread;
        send_packet<netki::CubeClientData, 65536>(&data);
    }
    else
    {
        uv_close((uv_handle_t*)client, on_close_tcp);
    }
    
    free(buf->base);
}

void on_new_connection(uv_stream_t *server, int status) 
{
    if (status == -1) 
    {
        fprintf(stderr, "failed!\n");
        return;
    }
    
    cube_tcp_client *client = new cube_tcp_client();
    client->connection_id = ++connection_id_counter;
    
    uv_stream_t *stream = (uv_stream_t *) &client->stream;
    uv_tcp_init(loop, &client->stream);
    if (uv_accept(server, stream) == 0) 
    {
        netki::CubeClientConnect connect;
        connect.Host = "<unknown>";
        connect.ConnectionId = client->connection_id;
        send_packet(&connect);
        uv_read_start(stream, alloc_buffer, on_read_tcp);
    }
    else
    {
        uv_close((uv_handle_t*)stream, on_close_tcp);
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
        
    cubesock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cubesock < 0)
    {
        printf("turbo: error creating socket\n");
        return -1;
    }
    
    struct sockaddr_un saddr;
    saddr.sun_family = AF_UNIX;
    strcpy(saddr.sun_path, argv[1]);
    if (connect(cubesock, (struct sockaddr*) &saddr, sizeof(struct sockaddr_un)))
    {
        printf("turbo: error connecting\n");
        return -1;
    }
    
    char data[1024];
    netki::bitstream::buffer buf;
    netki::bitstream::init_buffer(&buf, data);
    
    // welcome message.
    netki::CubeTurboInfo cti;
    cti.Version = "turbo-0.01";
    send_packet(&cti);
    
    loop = uv_default_loop();
    
    const int port = 9860;
    
    uv_tcp_t server;
    uv_tcp_init(loop, &server);
    
    struct sockaddr_in bind_addr;
    int ret = uv_ip4_addr("0.0.0.0", port, &bind_addr);
    
    uv_tcp_bind(&server, (const struct sockaddr*)&bind_addr, 0);
    
    int r = uv_listen((uv_stream_t*) &server, port, on_new_connection);
    if (r) 
    {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }

    printf("turbonet: entering main loop\n");
    uv_run(loop, UV_RUN_DEFAULT);   
    
    return 0;
}
