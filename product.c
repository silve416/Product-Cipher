#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define TRANS 1
#define NOTRANS 0
#define PADD 0

#ifndef DEBUG
	#define DEBUG 0
#endif 

#define ENCODE 0
#define DECODE 1

#ifndef MODE
	#define MODE ENCODE
#endif
/* Product Cipher created for CS 4821 Computer Security
 * @author Maxwell Silver
 */

int vigenere_buffer(char* cipherbuf, unsigned int* keybuf, unsigned int length, unsigned int position, int mode) {

  /* Do a vigenere encipher/decipher
   * Takes in cipherbuf and is shifted by keybuf
   * outputs to cipherbuf 
   * uses length and position for key wrapping
   * Mode determines encoding(1) or decoding(0)
   */

  unsigned int shift;
  
    for(int j=0; j < 16; j++){
      shift = keybuf[position];//Feed key into shift
      if (DEBUG) printf("SHIFT: %i\n", shift);

      if(mode){//If encoding, add shift
	cipherbuf[j] = (char)((cipherbuf[j] + shift)%256);
      }
      else{//If decoding, subtract shift
	cipherbuf[j] = (char)((cipherbuf[j] - shift)%256);
      }
      position++;
      if(position == length) position = 0;//If keybuf has reached it's end, reset

    }    
    
  return position;
}

int transpose_buffer(char *in, unsigned int dim) {
  
	/* do a columnar encipher/decipher
	 * from in to out
	 * using box of size dim*dim
	 * since it's a square, enciphering and deciphering is the same
	 */

  int i = 0;
  int j = 0;
  unsigned int index = 0;
  unsigned int data[dim][dim];
  
  //Enter buffer data into array
  for (i=0; i < dim; i++){
    for (j=0; j < dim; j++){
      data[i][j] = in[index];	
      if (DEBUG) printf("IN TRANSPOSE %c\n", in[index]);
      index++;
    }
  }
  
  //Enter transposed data into arrary pointer
  index = 0;
  for (j=0; j < dim; j++){
    for (i=0; i < dim; i++){
      in[index] = data[i][j];
      if (DEBUG) printf("IN WRITE %c\n", in[index]);
      index++;
    }
  }
  
  return 0;
}

int dump_buffer(char *buffer, unsigned int bufsize, FILE *output) {
  
	/* prints a buffer one character at a time to a file using %c
	 * takes in:
	 *  buffer -- pointer to a buffer
	 *  bufsize -- size of 'buffer'
	 *  output -- path to the file to open and output to
	 */
	
  /* print 'bytes' bytes from buffer to output file one char at a time */
  for(int i=0; i < bufsize; i++){
    fprintf(output, "%c", buffer[i]);    
  }
  
  /* wipe buffer using memset */
  memset(buffer, 0, 16);
  
  return 0;
}

int pad_buffer(char *buffer, unsigned int bufsize, unsigned int rbuf_index) {  
  
  /* pad_buffer pads the empty space in a buffer
   *  buffer -- pointer to buffer
   *  bufsize -- size of 'buffer'
   *  rbuf_index -- first "empty" spot in buffer, i.e.,
   *                put the 'X' at rbuf_index and fill the
   *                rest with 'Y' characters.
   */
  
  int i = rbuf_index;
  
  buffer[i] = 'X';
  i++;
  
  while(i < bufsize){
    buffer[i] = 'Y';
    i++;
  }	  
  
	return 1;	
}

int unpad_buffer(char *buffer, unsigned int bufsize) {
  
  /* unpads a buffer of a given size
   *  buffer -- buffer containing padded data
   *  bufsize -- size of 'buffer'
   */
  
  int unpadded = 0;
  int i = bufsize-1;
  
  if(DEBUG) printf("IN UNPAD FUNC:%c\n", buffer[i]);

  // Read buffer until it reaches X
  while(buffer[i] == 'Y'){
    
    if(DEBUG) printf("BUFF WAS: %c\n", buffer[i]);
    buffer[i] = ' ';
    if(DEBUG) printf("BUFF NOW: %c\n", buffer[i]); 
	    i--;
  }
  
  if(DEBUG) printf("END: %i\n", i);
  
  return i; //Return position of X  
}

int main(int argc, char *argv[]) {
  
  int i = 0; /* iterator we'll reuse */
  int j = 0;
  
  if (argc < 4) {
    printf("Missing arguments!\n\n");
    printf("Usage: encoder dim infile outfile ['notrans']\n\n");
    printf("Note: outfile will be overwritten.\n");
    printf("Optional '1' as last parameter will disable transposition.\n");
    return 1;
  }
  
  /* give input and output nicer names */
  unsigned int cycles = atoi(argv[1]);/* number of cycles */
  char *key = argv[2];/* key file path */
  char *input = argv[3];/* input file path */
  char *output = argv[4];/* output file path */

  unsigned int dim = 4;
  unsigned int bufsize = 16;	
  char read_buf[bufsize]; /* buffer for reading and padding */
  char write_buf[bufsize];/* buffer for transposition */
  
  unsigned int data[16];
  unsigned int length = 0;
  unsigned int symbol;//input bytes read into symbol

  FILE *INPUT;
  FILE *OUTPUT;
  
  /* open the key or quit on error */
  if ((INPUT = fopen(key, "r")) == NULL){
    printf("Problem opening key file '%s'; errno: %d\n", key, errno);
    exit(1);
  }

  //Read Key into data array
  while (length < (16) && (symbol = fgetc(INPUT)) != EOF){
    data[length] = (symbol);
    length++;
  }
  

  fclose(INPUT);

  if (DEBUG){
    /* reads out contents of key array */
    printf("KEY SIZE: %i\n", length);
    for (i=0; i<length; i++){
      printf("%c\n", data[i]);
    }
  }
  
  /* open the input or quit on error. */
  if ((INPUT = fopen(input, "r")) == NULL) {
    printf("Problem opening input file '%s'; errno: %d\n", input, errno);
    return 1;
  }  
  
  /* get length of input file */
  unsigned int filesize;		/* length of file in bytes */
  unsigned int bytesleft;		/* counter we reduce on reading */
  struct stat filestats;		/* struct for file stats */
  int err;
  
  if ((err = stat(input, &filestats)) < 0) { //Gets size of input
    printf("error statting file! Error: %d\n", err);
  }
  
  filesize = filestats.st_size;
  bytesleft = filesize;
  
  if (DEBUG) printf("Size of 'input' is: %u bytes\n", filesize); 
  
  /* truncate output file if it exists */ 
  if ((OUTPUT = fopen(output, "w+")) == NULL) {
    printf("Problem truncating output file '%s'; errno: %d\n", output, errno);
    return 1;
  }
 
  int rbuf_index = 0;//index into the input file
  int pad = 0;//checks if pad has been run
  int key_count = 0;

  
  ///////* ~Begin Cipher~ *//////
  if (MODE == ENCODE){    
    i = 0;
    rbuf_index = 0;
    
    while ((symbol = fgetc(INPUT)) != EOF){
      read_buf[i] = symbol;
      i++;
      rbuf_index++;
      
      if(i == bufsize || rbuf_index >= filesize){
	
	for(int k=0; k < cycles; k++){
	  
	  //Pad if encoding and last buffer isn't full
	  if (rbuf_index >= (filesize) && i < bufsize && !pad){ 
	  if (DEBUG) printf("IN BUF PAD\n");
	  pad = pad_buffer(read_buf, bufsize, i); //Set pad to 1
	  }
	  
	  // Perform Vigenere cipher
	  key_count = 0;
	  vigenere_buffer(read_buf, data, length, key_count, 1);
	  
	  // Transpose buffer     
	  if (DEBUG) printf("TRANSPOSING\n");      
	  transpose_buffer(read_buf, dim);      
	  
	  if (DEBUG){ for(int h=0; h<bufsize; h++) printf("POSITION %c, %d\n", read_buf[h], rbuf_index);}
	}
	dump_buffer(read_buf, bufsize, OUTPUT);      
	i = 0;
      }
    }
  if (rbuf_index >= (filesize) && !pad){
    if (DEBUG) printf("IN FULL PAD\n");
    pad = pad_buffer(read_buf, bufsize, 0);
    for(int k=0; k < cycles; k++){
      key_count = 0;
      vigenere_buffer(read_buf, data, length, key_count, 1);
      transpose_buffer(read_buf, dim);
    }
    dump_buffer(read_buf, bufsize, OUTPUT);
   }
  }

  ///////* ~Begin Decypher~ *///////
  if (MODE == DECODE){
    i = 0;
    rbuf_index = 0;
    
    while ((symbol = fgetc(INPUT)) != EOF){
      read_buf[i] = symbol;
      i++;
      rbuf_index++;
      
      if(i == bufsize || rbuf_index >= filesize){
	
	for(int k=0; k < cycles; k++){

	  //Perform Transposition
	  if (DEBUG) printf("TRANSPOSING\n");
	  transpose_buffer(read_buf, dim);	  	

	  //Perform Vigenere cipher
	  key_count = 0;
	  vigenere_buffer(read_buf, data, length, key_count, 0);	  

	  if (DEBUG){ for(int h=0; h<bufsize; h++) printf("POSITION %c, %d\n", read_buf[h], rbuf_index);}
	}
	if (rbuf_index >= filesize){//If at end of file, unpad last buffer
	  bufsize = unpad_buffer(read_buf, bufsize);//Returns position of pad start
	}      
	dump_buffer(read_buf, bufsize, OUTPUT);
	i = 0;
      }
    }
  }
  
  fclose(INPUT);
  fclose(OUTPUT);
  
  return 0;
}


//Future features:
//Shift cypher (Shifts characters up and down the string
//A insertion: inserts a character at every n place
