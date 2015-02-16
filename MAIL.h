/* ========================================================================== */
/*   Derived class of WiFi                                                    */
/*   This class implements a simple version of the SMTP protocol              */
/*                                                                            */
/*  Note that the implemented protocol is not TSL (no STARTTLS or SSL/TSL )   */
/*  No port 587 or 465. Just port 25.                                         */
/*  For example smtp.gmail.com (173.194.70.108) is not usable                 */
/*                                                                            */
/* Three functions:                                                           */
/* sendMail (basic protocol) (seldom used)                                    */
/*  SmtpServer : any SmtpServer availlable ex.: 62.241.4.1 (relay.poste.it)   */
/*  dest :  receiver mail address  (null terminated string)                   */
/*  subject : subject of message  (null terminated string)                    */
/*  message : null terminated string                                          */
/*                                                                            */
/* sendMail (authenticate protocol: username and password)                    */
/*  SmtpServer : any SmtpServer availlable ex.: 62.241.4.1 (relay.poste.it)   */
/*  dest :  receiver mail address  (null terminated string)                   */
/*  user :  usually the complete mail address  (it will be coded base64)      */
/*  psw  :  password (it will be coded base64)                                */
/*  subject : subject of message  (null terminated string)                    */
/*  message : null terminated string                                          */
/*                                                                            */
/* setTimeout : millisec (default 500)                                        */
/*                                                                            */
/*   (c) 2014 Author Daniele Denaro                                           */
/*                                                                            */
/* Version 1.1                                                                */
/* ========================================================================== */
/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*/

#ifndef MAIL_h
#define MAIL_h

#include <MWiFi.h>
#include <utility/BASE64.h>

#define maildebug 0              //1 for debug



class MAIL : public MWiFi
{
public:
  MAIL();
  bool sendMail(char *SmtpServer,char *user,char *dest,char *subject,char *mess);
  bool sendMail(char *SmtpServer,char *user,char *psw,char *dest,char *subject,char *mess);
  void setTimeout(int millisec);

private:
  BASE64 B64;
  int timeout;
  char *rec;
  int code;
  char *user64;
  char *psw64;
  void sendToServ(int sk,char *buff);
  void sendToServP(int sk,prog_char *buff);
  void sendToServP(int sk,prog_char *buff,char *var);
  char* recFromServ(int sk,int timeout);
  void serialPM(prog_char *buff);
  void serialPM(prog_char *buff, int len);  
  void MailExit(int sk);
  int locVar(prog_char *buff);  
};

#endif