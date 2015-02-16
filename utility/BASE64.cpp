/* ========================================================================== */
/*                                                                            */
/*   base64 coding/decoding function                                          */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#include <BASE64.h>

prog_char base64_chars[] PROGMEM = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

char* BASE64::base64_encode(char* bytes_to_encode) 
{
//  if (encoded!=NULL) free(encoded);
  int in_len=strlen(bytes_to_encode);
  char* ret=(char*)calloc((int)(in_len*1.5),sizeof(char));
  int p = 0; int i = 0;  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
	{ret[p]=pgm_read_byte(base64_chars+char_array_4[i]);p++;ret[p]=0;}
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      {ret[p]=pgm_read_byte(base64_chars+char_array_4[j]);p++;ret[p]=0;}

    while((i++ < 3)){ret[p]='=';p++;ret[p]=0;}
  }
  encoded=ret;
  return encoded;
}

char* BASE64::base64_decode(char* encoded_string) 
{
  int in_len = strlen(encoded_string);
  if (decoded!=NULL) free(decoded);
  char* ret=(char*)malloc((int)(in_len*0.75));
  int i = 0; int j = 0; int in_ = 0; int p=0;
  unsigned char char_array_4[4], char_array_3[3];

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++) {char_array_4[i] = search(base64_chars,char_array_4[i]);}

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++) {ret[p]= char_array_3[i];p++;ret[p]=0;}
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++) char_array_4[j] = 0;

    for (j = 0; j <4; j++) char_array_4[j] = search(base64_chars,char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
    
    for (j = 0; (j < i - 1); j++) {ret[p]= char_array_3[j];p++;ret[p]=0;}
  }
  decoded=ret;
  return decoded;
}

int BASE64::search(prog_char* pstring,char c)
{
  int i;
  for (i=0;i<64;i++) {if (pgm_read_byte(&base64_chars[i])==c) return i;}
  return 0;
}
