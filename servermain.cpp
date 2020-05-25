#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <exception>
#include <map>
#include <mutex>

#define PORT 1234
#define MAXDATASIZE 100

/* You will to add includes here */

// Included to get the support library
#include <calcLib.h>

#include "protocol.h"

using namespace std;
std::mutex mtx;
/* Needs to be global, to be rechable by callback and main */
int loopCount = 0;
int id = 486;
int flag = 0;
map<int, int> session;

/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum)
{
  // As anybody can call the handler, its good coding to check the signal number that called it.
  if (flag == 0)
  {
    loopCount++;
  }
  mtx.lock();
  for (auto it = session.begin(); it != session.end();)
  {
    it->second++;
    if (it->second >= 10)
    {
      it = session.erase(it);
      printf("Client timeout\n");
    }
    else
      ++it;
  }
  mtx.unlock();
  if (loopCount >= 10)
  {
    printf("Wait for client`s message.\n");
    loopCount = 0;
  }
  return;
}

int main(int argc, char *argv[])
{

  /* Do more magic */

  /* 
     Prepare to setup a reoccurring event every 10s. If it_interval, or it_value is omitted, it will be a single alarm 10s after it has been set. 
  */
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec = 1;
  alarmTime.it_interval.tv_usec = 1;
  alarmTime.it_value.tv_sec = 1;
  alarmTime.it_value.tv_usec = 1;

  /* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
  signal(SIGALRM, checkJobbList);
  setitimer(ITIMER_REAL, &alarmTime, NULL); // Start/register the alarm.
  int sockfd;
  struct sockaddr_in server;
  struct sockaddr_in client;
  socklen_t addrlen;
  int num;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
    perror("Creatingsocket failed.\n");
    exit(1);
  }

  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1)
  {
    perror("Bind()error.\n");
    exit(1);
  }
  char buf[MAXDATASIZE];
  calcMessage message;
  calcProtocol protocol;
  calcProtocol protocolRes;
  addrlen = sizeof(client);
  char *ptr;
  int session_id;

  while (num = recvfrom(sockfd, buf, MAXDATASIZE, 0, (struct sockaddr *)&client, &addrlen))
  {
    if (num < 0)
    {
      printf("Bad receive :(\n");
      break;
    }
    else
    {
      flag = 1;
      printf("Get a message form client\n");
      uint16_t version;
      memcpy(&version, buf, sizeof(version));
      version = ntohs(version);
      if (version == 2 && num == sizeof(calcMessage))
      {
        printf("The message type is calcMessage\n");
        memcpy(&message, buf, num);
        if (ntohs(message.type) == 22 && ntohl(message.message) == 0)
        {
          session_id = id;
        }
        else
        {
          continue;
          flag = 0;
          loopCount = 0;
        }
        printf("Generate an id %d\n", id);
        session.insert(make_pair(session_id, 0));
        initCalcLib();
        loopCount = 0;
        ptr = randomType();
        if (ptr[0] == 'f')
        {
          protocol.arith = htonl(5 + rand() % 4);
          protocol.flValue1 = htonl(randomFloat());
          protocol.flValue2 = htonl(randomFloat());
        }
        else
        {
          protocol.arith = htonl(1 + rand() % 4);
          protocol.inValue1 = htonl(randomInt());
          protocol.inValue2 = htonl(randomInt());
        }
        protocol.version = htons(1);
        protocol.type = htons(1);
        protocol.major_version = htons(1);
        protocol.minor_version = htons(0);
        protocol.id = htonl(id);
        id++;
        printf("Send protocol to the client\n");
        sendto(sockfd, (char *)&protocol, sizeof(calcProtocol), 0, (struct sockaddr *)&client, addrlen);
        printf("Wait for client response\n");
      }
      else if (version == 1 && num == sizeof(calcProtocol))
      {
        printf("The message type is calcProtocol\n");
        memcpy(&protocolRes, buf, num);
        printf("Get a protocal result\n");
        session_id = htonl(protocolRes.id);
        if (session.count(session_id) == 0)
        {
          printf("The task has been abandoned\n");
          flag = 0;
          loopCount = 0;
          continue;
        }
        session[session_id] = 0;
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
        printf("Start checking");
        if ((ntohl(protocolRes.arith) <= 4 && ntohl(protocolRes.inResult) == ntohl(protocol.inResult)) ||
            (ntohl(protocolRes.arith) > 4 && ntohl(protocolRes.flResult) == ntohl(protocol.flResult)))
        {
          message.type = htons(2);
          message.message = htonl(1);
          message.major_version = htons(1);
          message.minor_version = htons(0);
          message.protocol = htons(17);
          sendto(sockfd, (char *)&message, sizeof(calcMessage), 0, (struct sockaddr *)&client, addrlen);
          printf("Task success\n");
        }
        else
        {
          message.type = htons(2);
          message.message = htonl(2);
          message.major_version = htons(1);
          message.minor_version = htons(0);
          message.protocol = htons(17);
          sendto(sockfd, (char *)&message, sizeof(calcMessage), 0, (struct sockaddr *)&client, addrlen);
          printf("Task fail\n");
        }
        mtx.lock();
        session.erase(id - 1);
        mtx.unlock();
        printf("One task finished :)\n\n");
        flag = 0;
        loopCount = 0;
      }
      else
      {
        message.type = htons(2);
        message.message = htonl(2);
        message.major_version = htons(1);
        message.minor_version = htons(0);
        message.protocol = htons(17);
        sendto(sockfd, (char *)&message, sizeof(calcMessage), 0, (struct sockaddr *)&client, addrlen);
        printf("Error response type :(\n");
        flag = 0;
        loopCount = 0;
      }
    }
    flag = 0;
    loopCount = 0;
  }
}