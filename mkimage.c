/*
 * (C) Copyright 2017
 * Texas Instruments Incorporated, <www.ti.com>
 *
 * Venkateswara Rao Mandela <venkat.mandela@ti.com>
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#define _BSD_SOURCE
#include <endian.h>

void usage(char *str) {
    printf("Usage is %s -a <load address> <input> <output>\n",
           str);
}
void * read_file(char *filename, size_t *f_size) {

  FILE *fp;
  void *mptr = NULL;
  size_t file_size=1024;
  size_t nread;


  fp=fopen(filename,"rb");
  if(fp==NULL) {
    printf("Could not open file %s\n\n",filename);
    goto err_ret;
    /*
      } else {
      printf("Loaded file %s\n\n",filename);
    */
  }
  fseek(fp,0,SEEK_END);
  file_size = ftell(fp);
  fseek(fp,0,SEEK_SET);

  /*
  printf("%s found; size is %d bytes\n",filename,file_size);
  */
  mptr = (void *)malloc(file_size);
  assert(mptr!=NULL);
  memset(mptr,0x00,file_size);

  nread = fread(mptr,sizeof(uint8_t),file_size,fp);

  if(nread != file_size){
    printf("file %s : partially read %lu/%lu\n",
           filename,
           nread, file_size);
  }

  fclose(fp);

  *f_size = file_size;
 err_ret:
  return mptr;
}

void print_bytes(void *ptr, uint32_t n) {
  uint32_t i = 0;
  uint8_t *p = (uint8_t *) ptr;
  for (i = 0; i < n; i++)
    printf("%02x ", p[i]);
  printf("\n");
}
/*
 * Add an 8 byte header to the binary.
 *
 * first four bytes size of the image including the GP HEADER
 * load address
 */
int main(int argc, char *argv[]) {

  uint32_t arg_cnt = 0;
  uint32_t load_addr = 0;
  uint32_t cnt = 0;
  size_t inp_size;
  void *iptr;
  FILE *fout;
  uint32_t out_val = 0;

  if (argc !=  5)
    usage(argv[0]);

  arg_cnt++;
  if (strcmp(argv[arg_cnt],"-a") != 0) {
    printf("load_addr should be first argument\n");
    usage(argv[0]);
    exit(1);
  }
  arg_cnt++;

  cnt =  sscanf(argv[arg_cnt],"%x", &load_addr);
  if (cnt != 1) {
    printf("unable to read load address in hex from %s\n",
           argv[arg_cnt]);
    usage(argv[0]);
    exit(1);
  }
  arg_cnt++;
  iptr = read_file(argv[arg_cnt], &inp_size);
  if(iptr == NULL){
    printf("Could not read %s\n",argv[arg_cnt]);
    usage(argv[0]);
    exit(1);
  }

  arg_cnt++;
  fout = fopen(argv[arg_cnt], "wb");
  if (fout == NULL) {
    printf("Could not open %s for output\n", argv[arg_cnt]);
    usage(argv[0]);
    exit(1);
  }

  out_val = inp_size + 8;
  print_bytes(&out_val,sizeof(out_val));
  cnt = fwrite(&out_val, sizeof(uint8_t),
               sizeof(out_val), fout);
  assert(cnt == 4);

  out_val = load_addr;
  print_bytes(&out_val,sizeof(out_val));
  cnt = fwrite(&out_val, sizeof(uint8_t),
               sizeof(out_val), fout);
  assert(cnt == 4);

  cnt = fwrite(iptr, sizeof(uint8_t),
               inp_size, fout);
  assert(cnt == inp_size);
  fclose(fout);

  free(iptr);
  return 0;
}
