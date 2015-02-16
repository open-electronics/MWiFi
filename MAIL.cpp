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

#include <avr/pgmspace.h>
#include <MAIL.h>


prog_char helo[] PROGMEM="HELO Arduino\r\n";
prog_char ehlo[] PROGMEM="EHLO Arduino\r\n";
prog_char auth[] PROGMEM="AUTH LOGIN\r\n";
prog_char from[] PROGMEM="MAIL FROM:<%s>\r\n";
prog_char to[] PROGMEM="RCPT TO:<%s>\r\n";
prog_char data[] PROGMEM="DATA\r\n";
prog_char head1[] PROGMEM="From: Arduino\r\n";
prog_char head2[] PROGMEM="Subject: %s\r\n";
prog_char endTXT[] PROGMEM="\r\n.\r\n";
prog_char quit[] PROGMEM="QUIT\r\n";


MAIL::MAIL(){timeout=500;if (maildebug) Serial.begin(9600);}

bool MAIL::sendMail(char *SmtpServer,char *user,char *dest,char *subject,char *mess)
{
  *rec=NULL;
  int sk=openSockTCP(SmtpServer,25);
  if (sk==255) return false;
  rec=recFromServ(sk,timeout);
  if (code!=220){closeSock(sk);return false;}

  sendToServ(sk,helo);
  rec=recFromServ(sk,timeout);
  if (code!=250) {MailExit(sk);return false;}

  sendToServP(sk,from,user);
  rec=recFromServ(sk,timeout);
  if (code!=250) {MailExit(sk);return false;}
  
  sendToServP(sk,to,dest);
  rec=recFromServ(sk,timeout);
  if (code!=250) {MailExit(sk);return false;}
  
  sendToServP(sk,data);
  rec=recFromServ(sk,timeout);
  if (code!=354) {MailExit(sk);return false;}
  
  sendToServP(sk,head1);
  sendToServP(sk,head2,subject);
  sendToServ(sk,"\r\n");
  
  writeData(sk,mess);
  sendToServP(sk,endTXT);
  rec=recFromServ(sk,timeout);
  cleanBuff(sk);
  if (code!=250) {MailExit(sk);return false;}
  else {MailExit(sk); return true;}
}

bool MAIL::sendMail(char *SmtpServer,char *user,char *psw,char *dest,char *subject,char *mess)
{
  *rec=NULL;
  int sk=openSockTCP(SmtpServer,25);
  if (sk==255) return false;
  rec=recFromServ(sk,timeout);
  if (code!=220) MailExit(sk);
  
  sendToServP(sk,ehlo);
  rec=recFromServ(sk,timeout);
  while (code==250) {rec=recFromServ(sk,timeout);}
  sendToServP(sk,auth);

  rec=recFromServ(sk,timeout);
  if (code!=334) MailExit(sk);
  user64=B64.base64_encode(user);
  sendToServ(sk,user64);sendToServ(sk,"\r\n");
  free(user64);

  rec=recFromServ(sk,timeout);
  if (code!=334) MailExit(sk);
  psw64=B64.base64_encode(psw); 
  sendToServ(sk,psw64);sendToServ(sk,"\r\n"); 
  free(psw64);
    
  rec=recFromServ(sk,timeout);
  if (code!=235) MailExit(sk);
  
  sendToServP(sk,from,user);
  rec=recFromServ(sk,timeout);
  if (code!=250) MailExit(sk); 

  sendToServP(sk,to,dest);
  rec=recFromServ(sk,timeout);
  if (code!=250) MailExit(sk); 
 
  sendToServP(sk,data);
  rec=recFromServ(sk,timeout);
  if (code!=354) MailExit(sk); 
  
  sendToServP(sk,head1);
  sendToServP(sk,head2,subject);
  sendToServ(sk,"\r\n");
  sendToServ(sk,mess);
  sendToServP(sk,endTXT);
  rec=recFromServ(sk,timeout);
  cleanBuff(sk);
  if(code!=250){MailExit(sk);return false;}
  else {MailExit(sk);return true;}            
}


void MAIL::setTimeout(int millisec){timeout=millisec;}


void MAIL::sendToServ(int sk,char *buff)
{
  writeData(sk,buff);
  if (maildebug) Serial.print(buff);
}

void MAIL::sendToServP(int sk,prog_char *buff)
{
  writeDataPM(sk,buff);
  if (maildebug) serialPM(buff);
}

void MAIL::sendToServP(int sk,prog_char *buff,char *var)
{
  int p=locVar(buff);
  writeDataPM(sk,buff,p);
  writeData(sk,var);
  writeDataPM(sk,buff+p+2);
  if (maildebug) {serialPM(buff,p);Serial.print(var);serialPM(buff+p+2);}
}

char* MAIL::recFromServ(int sk,int timeout)
{
  rec=NULL; code=0;
  rec=readDataLn(sk,timeout);
  if (rec!=NULL) code=atoi(rec);
  if (maildebug) {if (rec!=NULL) Serial.println(rec);else Serial.println("---");}
  return rec;
}

void MAIL::serialPM(prog_char *buff)
{int i;Serial.print("> ");for (i=0;i<strlen_P(buff);i++)Serial.write(pgm_read_byte(buff+i));}

void MAIL::serialPM(prog_char *buff, int len)
{int i;Serial.print("> ");for (i=0;i<len;i++)Serial.write(pgm_read_byte(buff+i));}


void MAIL::MailExit(int sk)
{
  
  sendToServP(sk,quit);
  closeSock(sk);
}

int MAIL::locVar(prog_char* buff)
{
  int i;char c;
  for (i=0;i<strlen_P(buff);i++) {c=pgm_read_byte(buff+i);if (c=='%') return i;}
  return -1;
}
