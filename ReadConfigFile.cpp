/*
 * ReadConfigFile.cpp
 *
 *  Created on: Oct 29, 2013
 *      Author: user
 */

#include <stdio.h>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "ReadConfigFile.h"

ReadConfigFile::ReadConfigFile(std::string filename) :
    filename_(filename)
{
  readConfig();
}

ReadConfigFile::~ReadConfigFile()
{
}

bool ReadConfigFile::parseType(std::string line, std::string type)
{
  // line could look like "email=" or "email =" or " email =" or "   email="
  size_t pos = line.find_first_not_of(" ");
  if(pos != std::string::npos && (!line.compare(pos, line.find(" ", 0), type) ||
     !line.compare(pos, line.find("=", 0), type)))
    return true;
  return false;
}

void ReadConfigFile::readConfig()
{
  std::string line;
  int counter = 0;
  std::ifstream infile(filename_.c_str());
  while (std::getline(infile, line))
  {
    ++counter;
    std::cout << "line is: " << line << std::endl;
    // see what kind of line: if it starts with # its comment,
    //                        email = xxxxx email addresses, can be comma separated
    //                        subject = xxxx subject
    //                        from = xxxx from address
    if (parseType(line, "#"))
    {
      // comment, do nothing...
      continue;
    }
    else if (line.find_first_not_of(" ",0) == line.find("\n",0))
    {
      // only new line, do nothing!! can contain blanks... nothing else!
      continue;
    }
    else if (parseType(line, "email"))
    {
      // email, forward to parseAddress!
      parseAddress(line);
    }
    else if (parseType(line, "subject"))
    {
      // subject, forward to parseSubject!
      parseSubject(line);
    }
    else if (parseType(line, "from"))
    {
      // from address, forward to parseFrom!
      parseFrom(line);
    }
    else if(parseType(line, "query_file"))
    {
      // query filename
      parseOption(line, query_filename_);
    }
    else if(parseType(line, "socket_path"))
    {
      // query filename
      parseOption(line, socket_pathname_);
    }
    else
    {
      // not a valid config parameter!
      std::cerr
          << "invalid configuration file - check your configuration! Error occured in line" << counter << ", Filename was: "
          << filename_ << std::endl;
      break;
    }
  }
}

void ReadConfigFile::parseAddress(std::string line)
{
  //email = hans@gmx.at, wurst@gmx.at
  //email = hans@gmx.at
  size_t pos2 = 0;
  size_t pos = line.find("=", 0);
  std::string tmp;
  bool loop = true;
  while(loop)
  {
    // beliebige leerzeichen zwischen mails...
    pos = line.find_first_not_of(" ", pos + 1);
    pos2 = line.find(",", pos);
    if(pos2 == std::string::npos)
    {
      // could be last address: email = hans@gmx.at, wurst@gmx.at\n or
      //                        email = hans@gmx.at,         wurst@gmx.at\n
      pos2 = line.find("\n", pos);
      loop = false;
    }

    mailAddresses_.push_back(line.substr(pos, pos2 - pos));
    pos = pos2;
  }
}

void ReadConfigFile::parseOption(std::string line, std::string &param)
{
  size_t pos2 = 0;
  size_t pos = line.find("=", 0);
  pos = line.find_first_not_of(" ", pos + 1);
  //std::cout << "pos is: " << pos << std::endl;
  pos2 = line.find("\n", pos);
  if(pos2 == std::string::npos)
  {
    pos2 = line.size();
  }
  //std::cout << "pos2 is: " << pos2 << std::endl;
  param = line.substr(pos, pos2 - pos);
}

void ReadConfigFile::parseSubject(std::string line)
{
  parseOption(line, subject_);
}

void ReadConfigFile::parseFrom(std::string line)
{
  parseOption(line, from_);
}

