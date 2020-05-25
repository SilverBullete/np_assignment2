#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
/* You will to add includes here */

// Included to get the support library
#include <calcLib.h>

#include "protocol.h"

#define PORT 1234
#define MAXDATASIZE 100
using namespace std;

int main(int argc, char *argv[])
{
  printf("start!!!\n");
  int sockfd, num;
  char buf[MAXDATASIZE];
  int count = 0;

  struct hostent *he;
  struct sockaddr_in server, peer;
  if (argc != 3)
  {
    printf("Usage: %s <IP Address><sleepTime>\n", argv[0]);
    exit(1);
  }
  int sleepTime = atoi(argv[2]);
  if ((he = gethostbyname(argv[1])) == NULL)
  {
    printf("gethostbyname()error\n");
    exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
    printf("socket() error\n");
    exit(1);
  }
  calcMessage message;
  calcProtocol protocol;

  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
  server.sin_addr = *((struct in_addr *)he->h_addr);
  socklen_t addrlen;
  addrlen = sizeof(server);
  struct timeval timeout = {2, 0};
  int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  while (1)
  {
    printf("Start to send message\n");
    message.version = htons(2);
    message.type = htons(22);
    message.message = htonl(0);
    message.protocol = htons(17);
    message.major_version = htons(1);
    message.minor_version = htons(0);
    message.id = htonl(1);
    sendto(sockfd, (char *)&message, sizeof(calcMessage), 0, (struct sockaddr *)&server, sizeof(server));
    printf("Wait for server response\n");
    int num;
    while (count < 3)
    {
      num = recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&peer, &addrlen);
      if (num < 0)
      {
        count++;
        printf("Server no response. Resend %d times\n", count);
        sendto(sockfd, (char *)&message, sizeof(calcMessage), 0, (struct sockaddr *)&server, sizeof(server));
      }
      else
      {
        break;
      }
    }
    if (count == 3)
    {
      printf("Server timeout :(\n");
      break;
    }
    uint16_t version;
    memcpy(&version, buf, sizeof(version));
    version = ntohs(version);
    if (version == 1 && num == sizeof(calcProtocol))
    {
      memcpy(&protocol, buf, num);
      switch (ntohl(protocol.arith))
      {
      case 1:
        protocol.inResult = htonl(ntohl(protocol.inValue1) + ntohl(protocol.inValue1));
        break;
      case 2:
        protocol.inResult = htonl(ntohl(protocol.inValue1) - ntohl(protocol.inValue1));
        break;
      case 3:
        protocol.inResult = htonl(ntohl(protocol.inValue1) * ntohl(protocol.inValue1));
        break;
      case 4:
        protocol.inResult = htonl(ntohl(protocol.inValue1) / ntohl(protocol.inValue1));
        break;
      case 5:
        protocol.flResult = htonl(ntohl(protocol.flValue1) + ntohl(protocol.flValue2));
        break;
      case 6:
        protocol.flResult = htonl(ntohl(protocol.flValue1) - ntohl(protocol.flValue2));
        break;
      case 7:
        protocol.flResult = htonl(ntohl(protocol.flValue1) * ntohl(protocol.flValue2));
        break;
      case 8:
        protocol.flResult = htonl(ntohl(protocol.flValue1) / ntohl(protocol.flValue2));
        break;
      }
      sleep(sleepTime);
      printf("Send back a protocol to server\n");
      sendto(sockfd, (char *)&protocol, sizeof(calcProtocol), 0, (struct sockaddr *)&server, sizeof(server));
      printf("Wait for server response\n");
      count = 0;
      while (count < 3)
      {
        num = recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&peer, &addrlen);
        if (num < 0)
        {
          count++;
          printf("Server no response. Resend %d times\n", count);
          sendto(sockfd, (char *)&protocol, sizeof(calcProtocol), 0, (struct sockaddr *)&server, sizeof(server));
        }
        else
        {
          break;
        }
      }
      if (count == 3)
      {
        printf("Server timeout :(\n");
        break;
      }
      count = 0;
      uint16_t version;
      memcpy(&version, buf, sizeof(version));
      version = ntohs(version);
      printf("Get server response\n");
      if (version == 2 && num == sizeof(calcMessage))
      {
        memcpy(&message, buf, num);
        if (ntohl(message.message) == 1)
        {
          printf("Congratulations on your calculation :)\n");
        }
        else if (ntohl(message.message) == 2)
        {
          printf("It's a pity that you made a wrong calculation :(\n");
        }
        else
        {
          printf("Server error :(\n");
        }
      }
      else
      {
        printf("Error server response :(\n");
      }
    }
    else
    {
      printf("Error server response :(\n");
    }
    sleep(sleepTime);
  }
  close(sockfd);
}