#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>

int id_client = 0;// Variable que lleva la cuenta de los clientes conectados
int fd_max = 0;// Variable para almacenar el valor del fd más grande
int cl_fd[4096] = {-1};// Array que guarda los fd de los clientes
int cl_id_fd[4096] = {-1};// Array que mapea cada fd a un ID único
char *buffer[4096] = {0};// Array para almacenar los mensajes que aún no han sido procesados

void erro_msg(char *str)//Manda mensajes de error
{
    write(2, str, strlen(str));// Escribe el mensaje en la salida de error, con el tamaño de ese mensaje
    exit(1);
}

void max_fd() //Actualiza el fd_max
{
    for (int i = 0; i < id_client; i++)// Recorre todos los clientes
        if (cl_fd[i] > fd_max)// Si el fd del cliente es mayor que el fd_max
            fd_max = cl_fd[i];//Ahora el fd_max es ese fd
}

void send_all(char *msg, int fd)// Envia un mensaje a todos los clientes, excepto al cliente cuyo descriptor es 'fd'
{
    for (int i = 0; i < id_client; i++)// Recorre todos los clientes
        if (cl_fd[i] != fd && cl_fd[i] != -1)//Si el fd del cliente no es el fd del sender, y no está vacío (-1)
            send(cl_fd[i], msg, strlen(msg), 0);//Mandar a cl_fd[i] el msg, con su tamaño, y opcion 0
}

int extract_message(char **buf, char **msg)
{
    char *newbuf;
    int i;

    *msg = 0;
    if (*buf == 0)
        return (0);
    i = 0;
    while ((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {  // Detecta el fin de un mensaje (salto de línea)
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
            if (newbuf == 0)
                return (-1);  // Error en la asignación de memoria
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;  // Finaliza el mensaje con el salto de línea
            *buf = newbuf;
            return (1);  // Mensaje completo
        }
        i++;
    }
    return (0);  // No se encontró un mensaje completo
}

char *str_join(char *buf, char *add)
{
    char *newbuf;
    int len;

    if (buf == 0)
        len = 0;
    else
        len = strlen(buf);
    newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
    if (newbuf == 0)
        return (0);  // Error en la asignación de memoria
    newbuf[0] = 0;
    if (buf != 0)
        strcat(newbuf, buf);
    free(buf);
    strcat(newbuf, add);
    return (newbuf);
}

int main(int argc, char **argv)
{
    if (argc != 2)// Verifica que el número de argumentos sea correcto
        erro_msg("Wrong number of arguments\n");

    int sockfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;

    // Creación del socket y verificación de error
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        erro_msg("Fatal error\n");

    bzero(&servaddr, sizeof(servaddr));
   
    fd_set readfds, writefds, cpyfds; // Inicialización de los sets de 'select'
    FD_ZERO(&cpyfds);  // Inicializa el conjunto de descriptores de archivo
    FD_SET(sockfd, &cpyfds);  // Agrega el socket principal al conjunto
    fd_max = sockfd;  // Establece fd_max al valor del socket
    
    // Asignación de IP y puerto
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433);  // 127.0.0.1
    servaddr.sin_port = htons(atoi(argv[1]));  // Puerto tomado como argumento

    // Enlace del socket a la IP y puerto
    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        erro_msg("Fatal error\n");
    if (listen(sockfd, 10) != 0)
        erro_msg("Fatal error\n");

    while (1)
    {
    	readfds = writefds = cpyfds; //Copia los conjuntos de fds para select
        if (select(fd_max + 1, &readfds, &writefds, 0, 0) >= 0)// Llama a select para ver si algun fd quiere hacer algo
        {
            for (int fd = sockfd; fd <= fd_max; fd++)//Recorrer todos los fd desde sockfd(incio) hasta el fd_max(final)
            {
                if (FD_ISSET(fd, &readfds))//Revisar si ese fd quiere hacer algo
                {  
                    if (fd == sockfd)// Si es una nueva conexión de cliente
                    {
                        len = sizeof(cli);
                        connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
                        if (connfd >= 0)
                        {
                            int id = id_client++;
			    cl_id_fd[connfd] = id;//Asociar el descriptor connfd con su ID
			    cl_fd[id] = connfd;//Guarda el fd en el array de clientes.
			    char msg[50] = {'\0'};
			    sprintf(msg, "server: client %d just arrived\n", id);//Almacenar en msg
			    send_all(msg, connfd);//Enviar el mensaje 
			    FD_SET(connfd, &cpyfds);//Agregar el nuevo fd a cpyfds para ser revisado en futuras iteraciones
			    max_fd();//Llamar a funcion max_fd para actualizar el fd_max
			    break;
                        }
                    }
                    else// Si es un cliente interactuando
                    {
                        int id = cl_id_fd[fd];//Obtener el ID del cliente asociado al descriptor fd
                        char buffer_tmp[4096] = {'\0'};//Para almacenar el mensaje del cliente
                        int readed = recv(fd, &buffer_tmp, 4096, MSG_DONTWAIT);//Almacenar el nº de bytes leídos por recv()
                        buffer_tmp[readed] = '\0';//Terminar buffer_tmp con \0
                        if (readed <= 0)// Si el cliente se desconecta
                        {
                            char msg[50] = {'\0'};
                            sprintf(msg, "server: client %d just left\n", id);//Almacenar en msg
                            send_all(msg, fd);//Enviar el mensaje
                            FD_CLR(fd, &cpyfds); // Eliminar el fd de cpyfds
                            close(fd);// Cerrar el fd de ese cliente
                            cl_fd[id] = -1;// Eliminar del array de fds
                            free(buffer[id]);// Liberar el buffer de ese cliente
                        }
                        else //Si el cliente manda un mensaje
                        {
                            buffer[id] = str_join(buffer[id], buffer_tmp);// Unir lo que hay en el buffer_tmp con lo que hay en el buffer del cliente
                            char *msg_to_send = 0;//Para almacenar el mensaje que se va a enviar
                            while (extract_message(&buffer[id], &msg_to_send))//Buscar \n en buffer[id] y almacenar el msj en msg_to_end
                            {
                                char msg[50] = {'\0'};
                                sprintf(msg, "client %d: ", id);//Almacenar en msg
                                send_all(msg, fd);//Enviar "client %d: "
                                send_all(msg_to_send, fd);//Enviar el mensaje que ha escrito el sender
                                free(msg_to_send);// Liberar la cadena
                            }
                        }
                    }
                }
            }
        }
    }
}
