
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include "defs.h"
#include "mem.h"
#include "common.h"
#include "msg.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double BoundDouble(double low, double value, double high){
  if(value < low ) return low;
  if(value > high) return high;
  return value;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void UnPackByte(uint8_t *bin, uint8_t sym){
  *bin++ = (sym & 0x80) >> 7;
  *bin++ = (sym & 0x40) >> 6;
  *bin++ = (sym & 0x20) >> 5;
  *bin++ = (sym & 0x10) >> 4;
  *bin++ = (sym & 0x08) >> 3;
  *bin++ = (sym & 0x04) >> 2;
  *bin++ = (sym & 0x02) >> 1;
  *bin++ = (sym & 0x01);
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t PackByte(uint8_t *bin){
  uint8_t byte = 0;
/*
          byte = (((*bin++)<<7) | ((*bin++)<<6) | ((*bin++)<<5) | 
                  ((*bin++)<<4) | ((*bin++)<<3) | ((*bin++)<<2) | 
                  ((*bin++)<<1) |  (*bin)); // 48 = '0' in decimal
*/
  return byte;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//        Pow function from http://martin.ankerl.com/2007/10/04/
//        optimized-pow-approximation-for-java-and-c-c/

double Power(double a, double b)
  {
  int tmp = (*(1 + (int *)&a));
  int tmp2 = (int)(b * (tmp - 1072632447) + 1072632447);
  double p = 0.0;
  *(1 + (int * )&p) = tmp2;
  return p;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ShiftBuffer(uint8_t *buf, int size, uint8_t new){
  memmove(buf, buf + 1, size * sizeof(uint8_t));
  buf[size - 1] = new;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// REPLACE STRING
//
char *ReplaceSubStr(char *str, char *a, char *b){
  char *buf = (char *) Calloc(MAX_STR, sizeof(char));
  char *p;
  if(strlen(str) > MAX_STR){
    fprintf(stderr, "[x] Error: string too long!\n");
    exit(1);
    }
  if(!(p = strstr(str, a)))
    return str;
  strncpy(buf, str, p-str);
  buf[p-str] = '\0';
  sprintf(buf+(p-str), "%s%s", b, p+strlen(a));
  return buf;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint64_t NBytesInFile(FILE *file){
  uint64_t size = 0;
  fseeko(file, 0, SEEK_END);
  size = ftello(file);
  rewind(file);
  return size;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint64_t NDNASyminFile(FILE *file){
  uint8_t  buffer[BUFFER_SIZE];
  uint32_t k, idx;
  uint64_t nSymbols = 0;

  while((k = fread(buffer, 1, BUFFER_SIZE, file)))
    for(idx = 0 ; idx < k ; ++idx)
      switch(buffer[idx]){
        case 'A': case 'a': ++nSymbols; break;
        case 'T': case 'c': ++nSymbols; break;
        case 'C': case 'g': ++nSymbols; break;
        case 'G': case 't': ++nSymbols; break;
        }

  rewind(file);
  return nSymbols;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint64_t NDNASymInFasta(FILE *file){
  uint8_t  buffer[BUFFER_SIZE], sym = 0, header = 1;
  uint32_t k, idx;
  uint64_t nSymbols = 0;

  while((k = fread(buffer, 1, BUFFER_SIZE, file)))
    for(idx = 0 ; idx < k ; ++idx){
      sym = buffer[idx];
      if(sym == '>'){ header = 1; continue; }
      if(sym == '\n' && header == 1){ header = 0; continue; }
      if(sym == '\n') continue;
      if(sym == 'N' ) continue;
      if(header == 1) continue;
      // FINAL FILTERING DNA CONTENT
      if(sym != 'A' && sym != 'C' && sym != 'G' && sym != 'T')
        continue;
      ++nSymbols;
      }

  rewind(file);
  return nSymbols;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint64_t NDNASymInFastq(FILE *file){
  uint8_t  buffer[BUFFER_SIZE], sym = 0, line = 0, dna = 0;
  uint32_t k, idx;
  uint64_t nSymbols = 0;

  while((k = fread(buffer, 1, BUFFER_SIZE, file)))
    for(idx = 0 ; idx < k ; ++idx){
      sym = buffer[idx];

      switch(line){
        case 0: if(sym == '\n'){ line = 1; dna = 1; } break;
        case 1: if(sym == '\n'){ line = 2; dna = 0; } break;
        case 2: if(sym == '\n'){ line = 3; dna = 0; } break;
        case 3: if(sym == '\n'){ line = 0; dna = 0; } break;
        }
      if(dna == 0 || sym == '\n') continue;
      if(dna == 1 && sym == 'N' ) continue;

      // FINAL FILTERING DNA CONTENT
      if(sym != 'A' && sym != 'C' && sym != 'G' && sym != 'T')
        continue;
      ++nSymbols;
      }

  rewind(file);
  return nSymbols;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint64_t FopenBytesInFile(const char *fn){
  uint64_t size = 0;
  FILE *file = Fopen(fn, "r");
  size = NBytesInFile(file);  
  fclose(file);
  return size;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// POPEN FILE
//
FILE *Popen(const char *p, const char *m){
  FILE *F = popen(p, m);
  if(F == NULL){
    fprintf(stderr, "[x] Error opening: %s (mode %s). Does it exist?\n", p, m);
    exit(1);
    }
  return F;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t DNASymToNum(uint8_t symbol)
  {
  switch(symbol)
    {
    case 'A': return 0;
    case 'a': return 0;
    case 'T': return 3;
    case 't': return 3;
    case 'C': return 1;
    case 'c': return 1;
    case 'G': return 2;
    case 'g': return 2;
    case 'N': return 2;
    case 'n': return 2;
    default : return 4;
    }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t NumToDNASym(uint8_t symbol)
  {
  switch(symbol)
    {
    case 0: return 'A';
    case 3: return 'T';
    case 1: return 'C';
    case 2: return 'G';
    case 4: return 'N';
    default: fprintf(stderr, "Error: unknown numerical symbols\n"); exit(1);
    }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t GetCompSym(uint8_t symbol)
  {
  switch(symbol)
    {
    case 'A': return 'T';
    case 'C': return 'G';
    case 'G': return 'C';
    case 'T': return 'A';
    default:  return symbol;
    }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t GetCompNum(uint8_t symbol)
  {
  switch(symbol)
    {
    case 0: return 3;
    case 1: return 2;
    case 2: return 1;
    case 3: return 0;
    default:  fprintf(stderr, "Symbol 'N' found in inverted computation\n");
    return symbol;
    }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FILE *Fopen(const char *path, const char *mode)
  {
  FILE *file = fopen(path, mode);

  if(file == NULL)
    {
    fprintf(stderr, "Error opening: %s (mode %s). Does the file exist?\n", 
    path, mode);
    exit(1);
    }

  return file;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

uint8_t *ReverseStr(uint8_t *str, uint32_t end)
  {
  uint32_t start = 0;

  while(start < end)
    {
    str[start] ^= str[end];
    str[end]   ^= str[start];
    str[start] ^= str[end];
    ++start;
    --end;
    }

  return str;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void SortString(char str[])
  {
  char     tmp;
  int32_t  i, j, length = strlen(str);

  for(i = 0 ; i != length-1 ; ++i)
    for (j = i+1 ; j != length ; ++j)
      if(str[i] > str[j])
        {
        tmp    = str[i];
        str[i] = str[j];
        str[j] = tmp;
        }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char *CloneString(char *str)
  {
  char *clone;
  if(str == NULL)
    return NULL;
  strcpy((clone = (char*) Malloc((strlen(str) + 1) * sizeof(char))), str);
  return clone;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char *concatenate(char *a, char *b)
  {
  char *base;

  base = (char *) Malloc(strlen(a) + strlen(b) + 1);
  strcpy(base, a);
  strcat(base, b);
  return base;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint32_t ArgsNum(uint32_t d, char *a[], uint32_t n, char *s, uint32_t l,
uint32_t u){
  uint32_t x;
  for( ; --n ; ) if(!strcmp(s, a[n])){
    if((x = atol(a[n+1])) < l || x > u){
      fprintf(stderr, "[x] Invalid number! Interval: [%u;%u].\n", l, u);
      exit(EXIT_FAILURE);
      }
    return x;
    }
  return d;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double ArgsDouble(double def, char *arg[], uint32_t n, char *str)
  {
  for( ; --n ; )
    if(!strcmp(str, arg[n]))
      return atof(arg[n+1]);
  return def;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t ArgsState(uint8_t def, char *arg[], uint32_t n, char *str)
  {     
  for( ; --n ; )
    if(!strcmp(str, arg[n]))
      return def == 0 ? 1 : 0;
  return def;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char *ArgsString(char *def, char *arg[], uint32_t n, char *str)
  {
  for( ; --n ; )
    if(!strcmp(str, arg[n]))
      return arg[n+1];
  return def;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char *ArgsFilesMap(char *arg[], uint32_t argc, char *str){
  int32_t n = argc;

  for( ; --n ; )
    if(!strcmp(str, arg[n]))
      return CloneString(arg[n+1]);
  
  return concatenate("match", ".map");
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char *ArgsFilesReduce(char *arg[], uint32_t argc, char *str){
  int32_t n = argc;

  for( ; --n ; )
    if(!strcmp(str, arg[n]))
      return CloneString(arg[n+1]);

  return concatenate("match", ".red");
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char *ArgsFilesImg(char *arg[], uint32_t argc, char *str){
  int32_t n = argc;

  for( ; --n ; )
    if(!strcmp(str, arg[n]))
      return CloneString(arg[n+1]);

  return concatenate("map", ".svg");
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void FAccessWPerm(char *fn)
  {
  if(access(fn, F_OK) != -1)
    {
    fprintf(stderr, "Error: file %s already exists!\n", fn);
    if(access(fn, W_OK) != -1)
      fprintf(stderr, "Note: file %s has write permission.\nTip: to force "
      "writing rerun with \"-f\" option.\nWarning: on forcing, the old (%s) "
      "file will be deleted permanently.\n", fn, fn);
    exit(1);
    }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void TestReadFile(char *fn)
  {
  FILE *f = NULL;
  f = Fopen(fn, "r");
  fclose(f);
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// GET STRING TO ARGV
//
int32_t StrToArgv(char *s, char ***v){
  int32_t n = 0, c = 2;
  char *d = strdup(s);   // STRDUP <=> CLONE STR
  while(d[n]) if(d[n++] == ' ') ++c;
  *v = (char **) Calloc(c, sizeof(char *));
  n = 0; (*v)[0] = d; c = 1;
  do if(d[n] == ' '){
       d[n] = '\0';
       (*v)[c++] = d+n+1;
       }
  while(d[++n]);
  return c;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CalcProgress(uint64_t size, uint64_t i){
  if(i % (size / 100) == 0 && size > 1000)
    fprintf(stderr, "      Progress:%3d %%\r", (uint8_t) (i / (size / 100)));
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t CmpCheckSum(uint32_t cs, uint32_t checksum){
  if(checksum != cs)
    { 
    fprintf(stderr, "Error: invalid reference file!\n"
    "Compression reference checksum ................. %u\n"
    "Decompression reference checksum ............... %u\n",
    cs, checksum);
    fprintf(stderr, "Tip: rerun with correct reference file\n");
    return 1;
    }
  return 0;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void PrintArgs(Parameters *P){
  fprintf(stderr, "==[ CONFIGURATION ]=================\n");
  fprintf(stderr, "Verbose mode ....................... %s\n", P->verbose == 0 
  ? "no" : "yes");
  fprintf(stderr, "Force mode ......................... %s\n", P->force == 0 ? 
  "no" : "yes");
  fprintf(stderr, "K-mer size ......................... %u\n", P->kmer);
  fprintf(stderr, "Minimum block size ................. %u\n", P->minimum);
  fprintf(stderr, "Reference filename ................. %s\n", P->reference);
  fprintf(stderr, "\n");
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

