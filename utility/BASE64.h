/* ========================================================================== */
/*                                                                            */
/*   base64 coding/decoding function                                          */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#ifndef BASE64_h
#define BASE64_h

#include <avr/pgmspace.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


class BASE64
{
public:
     char* base64_encode(char* string);
     char* base64_decode(char* string);
     
private:
     static inline bool is_base64(unsigned char c) 
     { return (isalnum(c) || (c == '+') || (c == '/'));}
     int search(prog_char* pstring,char c);
     char* encoded;
     char* decoded;     
};


#endif