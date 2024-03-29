#include "socket_helper.h"

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define FLAGS 0
#define BUFFER_SIZE 1024
#define MAX_PENDING_QUEUE 20
int numOccurencesInString(char* string, char* substring)
{
    int num_apperences = 0;
    char* found = string;
    while ((found = strstr(found, substring)) != NULL) {
        found += strlen(substring);
        num_apperences++;
    }
    return num_apperences;
}

void writeNumberToFile(char* file_name, int num)
{
    FILE* fp = fopen(file_name, "w+");
    fprintf(fp, "%d", num);
    fclose(fp);
}

void recieveAndForward(int source_connection, int destination_connection, int num_end_string_occurences)
{

    char* buffer = (char*)calloc(BUFFER_SIZE, sizeof(char));
    int buffer_size = BUFFER_SIZE;
    int bytes_read = 0;

    while (true) {
        bytes_read += recv(source_connection, buffer + bytes_read, buffer_size - bytes_read, 0);
        // Check if the buffer needs to be reallocated
        if (bytes_read == buffer_size) {
            buffer_size *= 2;
            printf("Reallocating...\n");
            buffer = realloc(buffer, buffer_size);
        }

        if (numOccurencesInString(buffer, "\r\n\r\n") == num_end_string_occurences) {
            break;
        }
    }

    send(destination_connection, buffer, strlen(buffer), FLAGS);
    free(buffer);
}

void loadBalance(int lb_client_socket, int lb_server_socket)
{
    int client_connection, server_connection;

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    struct sockaddr_in server_addr;
    socklen_t server_addr_size = sizeof(server_addr);

    while (true) {

        client_connection = accept(lb_client_socket, (struct sockaddr*)&client_addr, &client_addr_size);
        server_connection = accept(lb_server_socket, (struct sockaddr*)&server_addr, &server_addr_size);

        if (client_connection == -1 || server_connection == -1) {
            printf("accept Error...\n");
            exit(1);
        }

        if (SetTimeOutValues(client_connection) == false || SetTimeOutValues(server_connection) == false) {
            printf("Error setting timeout values..\n");
            exit(1);
        }

        recieveAndForward(client_connection, server_connection, 1);  // read untill 1 \r\n\r\n in the request -- note- 1 Should be replaced with a Define - "NUM_DELIMS_IN_REQUEST"
        recieveAndForward(server_connection, client_connection, 2);  // read untill 2 \r\n\r\n in the response -- note- 2 Should be replaced with a Define - "NUM_DELIMS_IN_RESPONSE"

        close(client_connection);
        close(server_connection);
    }
}

int main()
{
    srand(time(NULL));

    int lb_client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int lb_client_port_number = BindSocketRandomPort(lb_client_socket);
    writeNumberToFile("http_port", lb_client_port_number);

    int lb_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int lb_server_port_number = BindSocketRandomPort(lb_server_socket);
    writeNumberToFile("server_port", lb_server_port_number);

    listen(lb_client_socket, MAX_PENDING_QUEUE);
    listen(lb_server_socket, MAX_PENDING_QUEUE);

    loadBalance(lb_client_socket, lb_server_socket);

    close(lb_client_socket);
    close(lb_server_socket);
    return 0;
}
