/* TGA File REader Classs Implementation File
   This Implementation Allows the reading of TGA (Targa) Files
   into an RGB buffer. Also the class allows an RGB Buffer to be 
   written to a TGA File. There is also a function to determine 
   the dimensions of a TGA file.

  Created By: Timothy A. Bish
  Created On: 08/17/98

*/

#ifndef TGAFILEH
#define TGAFILEH

unsigned char * LoadTGAFile
(
	const char * filename,
	int * width,
	int * height
);

#endif