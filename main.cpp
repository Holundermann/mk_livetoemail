//============================================================================
// Name        : main.cpp
// Author      : Clemens Feuerstein
// E-Mail      : cfeuerstein@scc.co.at
// Version     : 0.3
// Copyright   : GPL
// Description : sends an notification from mk_live with an given query file
//               to email addresses specified in email.txt in the same folder
//============================================================================

#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <vector>

#include "ReadConfigFile.h"

using namespace std;

const static int WARNING_STATE  = 1;
const static int ERROR_STATE    = 2;
const static int UNKNOWN_STATE  = 3;
const static int NO_ERROR       = 0;
const static int ERROR          = -1;
const static int NO_QUERY_FILE  = -1;

/**
 * sends an status mail to the email adresses which are stored in the email.txt
 * file
 */
bool sendMail(string& message, ReadConfigFile* readConfig)
{
  FILE *myMail;

  vector<string> mails = readConfig->getMailAdresses();

  if(mails.size() == 0)
  {
    return false;
  }

  // via sendmail we send the statusmail to the mailadresses stored in email.txt
  for (vector<string>::iterator it = mails.begin(); it != mails.end(); ++it)
  {
    if (NULL != (myMail = popen("/usr/sbin/sendmail -t", "w")))
    {
      cout << "sending mail to " << (*it) << endl;
      fprintf(myMail, "To: %s \n", (*it).c_str());
      fprintf(myMail, "From: %s \n", readConfig->getFrom().c_str());
      fprintf(myMail, "Subject: %s\n", readConfig->getSubject().c_str());
      fprintf(myMail, "\n");
      fprintf(myMail, "%s", message.c_str());
      fprintf(myMail, "\n.\n");
      pclose(myMail);
      cout << "Mail Sent..." << endl;
    }
    else
    {
      cout << "Error sending mail" << endl;
      return false;
    }
  }
  return true;
}

/**
 * writes status of the service
 */
void writeStatus(string& message, string& host, string& previous_host,
    string& service, uint& counter, int status, int previous_status)
{
  stringstream tmp;
  ++counter;
  if (host == previous_host && status == previous_status)
  {
    tmp << ", " << service;
  }
  else if (message.size())
  {
    tmp << "; " << host << ": " << service;
  }
  else
    tmp << host << ": " << service;
  message += tmp.str();
}

/**
 * translates char state to int state
 */
int getStatus(string& message, size_t& current_pos)
{
  if (message.at(current_pos) == '1')
    return WARNING_STATE;
  else if (message.at(current_pos) == '2')
    return ERROR_STATE;
  else if (message.at(current_pos) == '3') return UNKNOWN_STATE;
  return ERROR;
}

/**
 * gets the hostname of the current line
 */
string getHost(string& message, size_t current_line, size_t host_pos)
{
  string host;
  host.assign(message.c_str(), current_line, (host_pos - current_line));
  return host;
}

/**
 * gets the service of the current line
 */
string getService(string& message, size_t& current_pos)
{  // opens the query file and saves it to buff_str
  string service;
  size_t service_pos = message.find(";", current_pos + 1);
  service.assign(message.c_str(), (current_pos + 1),
      (service_pos - (current_pos + 1)));
  current_pos = ++service_pos;

  return service;
}

/**
 * formats the output message
 */
void formatMessage(string& message)
{
  // speichert die Anfangsposition des auszuschneidenten strings
  size_t pos1 = 0;
  // speichert die Endposition des auszuschneidenten strings
  size_t pos2 = 0;
  int status = 0;
  int previous_status = 0;
  uint warning_counter = 0;
  uint critical_counter = 0;
  uint unknown_counter = 0;
  // aktueller Host
  string host;
  // vorhergehender Host
  string previous_host;
  // service
  string service;
  // warning, critical, unknown
  //host1: service, service, service; host2: service, service...
  string warning;
  string critical;
  string unknown;
  stringstream final_message;

  // solange host = host vorhergehende Zeile ist
  /*"even.eht;HDD C;1\n
   * gucose;HDD C;1\n
   * gucose;HDD D;2\n
   * ippt;Apache;2\n
   * ippt;CPU Load;2\n
   * ippt;HDD\n */
  while (1)
  {
    if (pos2 != 0) pos1 = pos2 + 1;
    pos2 = message.find(';', pos2);
    if (pos2 != string::npos)
    {
      host = getHost(message, pos1, pos2);
    }
    else
      break;
    service = getService(message, pos2);
    status = getStatus(message, pos2);

    switch (status)
    {
    case WARNING_STATE:
      writeStatus(warning, host, previous_host, service, warning_counter,
          status, previous_status);
      break;
    case ERROR_STATE:
      writeStatus(critical, host, previous_host, service, critical_counter,
          status, previous_status);
      break;
    case UNKNOWN_STATE:
      writeStatus(unknown, host, previous_host, service, unknown_counter,
          status, previous_status);
      break;
    default:
      break;
    }
    previous_host = host;
    previous_status = status;
    ++pos2;
  }

  if (critical_counter)
  {
    final_message << "Critical: " << critical_counter << "; ";
  }
  if (warning_counter)
  {
    final_message << "Warning: " << warning_counter << "; ";
  }
  if (unknown_counter)
  {
    final_message << "Unknown: " << unknown_counter << "; ";
  }

  final_message << endl;
  if (critical_counter) final_message << "Criticals: " << critical << endl;
  if (warning_counter) final_message << "Warnings: " << warning << endl;
  if (unknown_counter) final_message << "Unknown: " << unknown << endl;
  message = final_message.str();
}

/**
 * checks the size of argc
 */
bool checkArguments(int argc, char* argv[])
{
  if (argc != 2)
  {
    return false;
  }
  else return true;
}

/**
 * reads data from mk_live via the query file and pass it on to parse the
 * received data
 */
int main(int argc, char* argv[])
{
  if (!checkArguments(argc, argv))
  {
    cout << "Error - no config file was specified!" << endl;
    return ERROR;
  }
  // read config file
  ReadConfigFile* readConfig = new ReadConfigFile(std::string(argv[1]));
  // tmp buffer for reading from socket
  char* buffer = NULL;
  // LQL query file
  std::ifstream is;
  // length of the query file
  int length = 0;

  // opens the query file and saves it to buff_str
  cout << "readConfig->getQueryFilename(): " << readConfig->getQueryFilename() << endl;
  is.open(readConfig->getQueryFilename().c_str(), ios::in);
  if (is.good())
  {
    is.seekg(0, std::ios::end);
    length = is.tellg();
    is.seekg(0, ios::beg);
    buffer = new char[length + 1];
    is.read(buffer, length - 1);
    buffer[length] = '\0';
    is.close();
  }
  else
  {
    cout << "Error reading query file, filename was: "<< readConfig->getQueryFilename() << endl;
    return NO_QUERY_FILE;
  }

  string buff_str(buffer);  //buff_str = buffer;
  cout << buff_str << endl;
//  string receive = "even.eht;HDD C;1\ngucose;HDD C;1\ngucose;HDD D;2\nippt;Apache;2\nippt;"
//      "CPU Load;2\nippt;HDD /;2\nippt;HDD bigone;2\nippt;HDD boot;2\nippt;HDD server;2\nippt;HDD var;2\nippt;MailQ;2\n"
//      "ippt;MySQL;2\nippt;Processes;2\nippt;Users;2\ntokyo.scc.local;Raid-Platten;2\ntokyo.scc.local;Users;2\n";
  string receive;

  //send query to the socket and receive Data
  if (buff_str.length())
  {
    int s, t, len;
    struct sockaddr_un remote;
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
      perror("socket");
      exit(ERROR);
    }

    cout << "Trying to connect..." << endl;
    cout << "readConfig->getSocketPathname().c_str(): " << readConfig->getSocketPathname().c_str() << endl;

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, readConfig->getSocketPathname().c_str());
    cout << "remote.sun_path: " << remote.sun_path << "strlen is: " << strlen(remote.sun_path) << endl;
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *) &remote, len) == -1)
    {
      cout << "could not connect to socket! Socket pathname was: " << readConfig->getSocketPathname() << endl;
      exit(ERROR);
    }

    cout << "connected" << endl;
    buff_str += "\n";

    if (send(s, buff_str.c_str(), buff_str.size(), 0) == -1)
    {
      cout << "could not send query to socket" << endl;
      exit(ERROR);
    }

    while ((t = recv(s, str, 100, 0)) > 0)
    {
      str[t] = '\0';
      receive += str;
    }
    cout << "received: " << endl << receive << endl;
  }

  delete[] buffer;
  buffer = NULL;
  formatMessage(receive);
  cout << receive << endl;

  int error = ERROR;
  if (receive.size() && sendMail(receive, readConfig))
  {
    error = NO_ERROR;
  }
  // cleanup
  delete readConfig;
  readConfig = NULL;
  return error;
}
