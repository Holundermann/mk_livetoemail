/*
 * ReadConfigFile.h
 *
 *  Created on: Oct 29, 2013
 *      Author: user
 */

#ifndef READCONFIGFILE_H_
#define READCONFIGFILE_H_

#include <vector>
#include <string.h>
#include <iostream>

class ReadConfigFile
{
public:
  ReadConfigFile(std::string filename);

  virtual ~ReadConfigFile();

  __inline std::string getSubject() {return subject_;};

  __inline std::string getFrom() {return from_;};

  __inline std::string getQueryFilename() {return query_filename_;};

  __inline std::string getSocketPathname() {return socket_pathname_;};

  __inline std::vector<std::string> getMailAdresses() {return mailAddresses_;};

private:
  std::vector<std::string> mailAddresses_;
  std::string subject_;
  std::string from_;
  std::string query_filename_;
  std::string socket_pathname_;
  std::ifstream inFile_;
  std::string filename_;

  /**
   * parses given string and checks weather its an email address, subject or from
   * field
   */
  void readConfig();

  /**
   * parses given string for mail addresses
   */
  void parseAddress(std::string line);

  /**
   * parse given string for subject
   */
  void parseSubject(std::string line);

  /**
   * parse given string for from
   */
  void parseFrom(std::string line);

  /**
   * parses the given line for the given option
   */
  void parseOption(std::string line, std::string &param);

  /**
   * parse line for type
   * @return true if line is of given type, false otherwise
   */
  bool parseType(std::string line, std::string type);
};

#endif /* READCONFIGFILE_H_ */
