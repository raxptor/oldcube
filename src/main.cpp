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
uv_pipe_t cube_pipe;
bool cube_connected = false;

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

void tmp_write_complete(uv_write_t *req, int status)
{
    printf("write completed with status %d\n", status);
    if (status < 0)
    {
        uv_close((uv_handle_t*)req->handle, 0);
    }
    
    free(req);
}

template<typename T, int bufsize>
void send_packet(uv_stream_t *handle, T *pkt)
{
    struct tmp {
        uv_write_t req;
        char data[bufsize];
    };
    
    tmp *req = (tmp*)malloc(sizeof(tmp));

    netki::bitstream::buffer buf;
    netki::bitstream::init_buffer(&buf, req->data);
    write_packet(pkt, &buf);
    netki::bitstream::flip_buffer(&buf);
    
    printf("writing %d bytes\n", buf.bufsize);
    uv_buf_t uvb;
    uvb.base = req->data;
    uvb.len = buf.bufsize;
    uv_write(&req->req, handle, &uvb, 1, tmp_write_complete);
}

template<typename T>
inline void send_packet(uv_stream_t *stream, T *pkt)
{
    send_packet<T, 1024>(stream, pkt);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf) 
{
    *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

void on_read_pipe(uv_stream_t *client, ssize_t nread, const uv_buf_t* buf) 
{
    if (nread > 0)
    {
        /*
        netki::CubeClientData data;
        data.ConnectionId = cc->connection_id;
        data.Data = (uint8_t*)buf->base;
        data.Data_size = nread;
        send_packet<netki::CubeClientData, 65536>((uv_stream_t*)&cube_pipe, &data);
        */
        printf("turbo: Got pipe data, %d bytes\n", (int)nread);
    }
    else
    {
        printf("turbo: Disconnected from pipe!\n");
        uv_close((uv_handle_t*)client, 0);
    }
    
    free(buf->base);
}


void on_pipe_connected(uv_connect_t *handle, int status)
{
    if (status == 0)
    {
        netki::CubeTurboInfo cti;
        cti.Version = "turbo-0.01";
        send_packet((uv_stream_t*)&cube_pipe, &cti);
        cube_connected = true;
    }
}

void on_close_tcp(uv_handle_t *handle)
{
    cube_tcp_client *cc = (cube_tcp_client *)handle;
    netki::CubeClientDisconnect disco;
    disco.ConnectionId = cc->connection_id;
    send_packet((uv_stream_t*)&cube_pipe, &disco);
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
        send_packet<netki::CubeClientData, 65536>((uv_stream_t*)&cube_pipe, &data);
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
        send_packet((uv_stream_t*)&cube_pipe, &connect);
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
    
    printf("connecting on [%s]\n", argv[1]);

    // connect    
    loop = uv_default_loop();
    uv_connect_t connreq;
    uv_pipe_init(loop, &cube_pipe, 0);
    uv_pipe_connect(&connreq, &cube_pipe, argv[1], on_pipe_connected);
    uv_run(loop, UV_RUN_DEFAULT);
    
    if (!cube_connected)
    { 
        printf("turbo: failed to make initial connection\n");
        return 0;
    }
    
    uv_read_start((uv_stream_t*)&cube_pipe, alloc_buffer, on_read_pipe);
    
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
