#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "mem.h"
#include "seq.h"
#include "pos.h"
#include "time.h"
#include "defs.h"
#include "param.h"
#include "msg.h"
#include "parser.h"
#include "reads.h"
#include "buffer.h"
#include "common.h"
#include "rmodel.h"

HASH *Hash; // HASH MEMORY IS PUBLIC

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - R A N D   S T R I N G - - - - - - - - - - - - - -
char *RandString(int size){
  int x;

  srand(time(NULL));               // SEED WITH TIME
  char *label = (char *) Malloc((size + 1) * sizeof(char));
  for(x = 0 ; x < size ; ++x)
    label[x] = rand() % 26 + 65;
  label[x] = 0; // null terminator

  return label;
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - S O R T I N G - - - - - - - - - - - - - - - -
void MaxHeapify(int64_t a[], int i, int size){
  int64_t tmp, largest;
  int64_t l = (2 * i) + 1;
  int64_t r = (2 * i) + 2;
  if((l <= size) && (a[l] > a[i])) largest = l;
  else                             largest = i;
  if((r <= size) && (a[r] > a[largest])) largest = r;
  if(largest != i){
    tmp        = a[i];
    a[i]       = a[largest];
    a[largest] = tmp;
    MaxHeapify(a, largest, size);
    }
  }

void BuildMaxHeap(int64_t a[], int size){
  int i;
  for(i = size/2; i >= 0; i--)
    MaxHeapify(a, i, size);
  }

void HeapSort(int64_t a[], int size){
  int64_t tmp;
  int i;
  BuildMaxHeap(a, size);
  for(i = size; i > 0; i--){
    tmp  = a[i];
    a[i] = a[0];
    a[0] = tmp;
    --size;
    MaxHeapify(a, 0, size);
    }
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - O R D E R   A N D   W R I T E   R E A D S - - - - - - - -
void OrderReads(uint32_t mem){
  char fname[MAX_LINE_SIZE];
  char buffer[MAX_LINE_SIZE];

  if(!P->order)
    sprintf(fname, "sort -n -T . -S %uM %s", mem, P->label); // -n numeric sort
  else
    sprintf(fname, "cat %s", P->label);

  FILE *F = Popen(fname, "r");

  while(fgets(buffer, MAX_LINE_SIZE, F)){

    if(!P->header)
      fprintf(stdout, "@owl ");

    int32_t n = 0, x = 0, step = 0, init = 0;

    while(buffer[x] != '\n'){

      if(buffer[x] == 20 && step == 0){ // CUT THE PREFIX NUMBER
        init = x + 1;
        step = 1;
        ++x;
        continue;
        }

      if(buffer[x] == 20 && step == 1){
        if(P->header){
          fprintf(stdout, "@");
          for(n = init ; n < x ; ++n)
            fprintf(stdout, "%c", buffer[n]);
          fprintf(stdout, "\n");
          }
        else{
          fprintf(stdout, "%c",   buffer[x-2]);        
          fprintf(stdout, "%c\n", buffer[x-1]);        
          }
        init = x + 1;
        step = 2;
        ++x;
        continue;
        }

      if(buffer[x] == 20 && step == 2){
        for(n = init ; n < x ; ++n)
          fprintf(stdout, "%c", buffer[n]);
        fprintf(stdout, "\n+\n");        
        init = x + 1;
        step = 3;
        ++x;
        continue;
        }
 
      if(buffer[x] == 20 && step == 3){
        for(n = init ; n < x ; ++n)
          fprintf(stdout, "%c", buffer[n]);
        fprintf(stdout, "\n");     
        init = x + 1;
        step = 0;
        ++x;
        continue;
        }

      ++x;
      }

    }
  
  pclose(F);
  if(!P->delete)
    remove(P->label);
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - - E L A S T I C - - - - - - - - - - - - - - -
int64_t CumulativeElastic(int64_t a[], int64_t size){
  int64_t x;
  int64_t pivot   = a[0];
  int64_t elastic = 0;
  int64_t best    = 0;

  for(x = 1 ; x < size ; ++x){

    #ifdef NON_RANDOM_MODE
    if(a[x] == -1) continue; // SKIP IF UNDEFINED (-1)
    #endif

    if(a[x] < pivot + P->minimum){
      ++elastic;
      best = pivot;
      }
    else{
      pivot = a[x];
      x     = x + elastic;
      }
    }

  return best;
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - W R I T E   R E A D - - - - - - - - - - - - -
void WriteRead(Read *Read, int64_t pos, FILE *F){
  int64_t x;

  //fprintf(F, "%li\t%c", pos, 20);
  fprintf(F, "%li%c", pos, 20);
  x = 0;
  while(Read->header1[1][x] != '\n')
    fprintf(F, "%c", Read->header1[1][x++]); 
  fprintf(F, "%c", 20); // SPLITTER
  x = 0;
  while(Read->bases[x] != '\n')
    fprintf(F, "%c", Read->bases[x++]);
  fprintf(F, "%c", 20); // SPLITTER
  x = 0;
  while(Read->scores[x] != '\n')
    fprintf(F, "%c", Read->scores[x++]);
  fprintf(F, "%c", 20); // SPLITTER
  fprintf(F, "\n");
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - C R E A T E   L A B E L - - - - - - - - - - - -
void CreateLabel(void){
  char *extension = ".owl";
  P->label = concatenate(RandString(12), extension);
  fprintf(stderr, "      Label: %s\n", P->label);
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - - - - M A P - - - - - - - - - - - - - - - - - 
void MapTarget(void){
  int64_t     nBase = 0;
  uint32_t    n = 0, idxPos;
  PARSER      *PA = CreateParser();
  CBUF        *symBuf = CreateCBuffer(BUFFER_SIZE, BGUARD);
  uint8_t     sym, *pos;
  RMODEL      *RM = CreateRModel(P->minimum, P->kmer);
  FILE        *F = Fopen(P->label, "w"); 

  FileType(PA, stdin);
  if(PA->type != 2){
    fprintf(stderr, "Error: input file must be in FASTQ format!\n");
    exit(1);
    }

  srand(0); // ENSURE THE SAME RUN PRODUCES THE SAME RANDOM RESULTS

  Read *Read = CreateRead(10000, 40000);
  while((Read = GetRead(stdin, Read)) != NULL){

    nBase = strlen(Read->bases) - 1; // IT ALSO LOADS '\n' AT THE END
    int64_t positions[nBase+1], idx = 0;
    uint64_t base = 0;

    for(idxPos = 0 ; idxPos < nBase ; ++idxPos){

      sym = Read->bases[idxPos];
      if(sym == 'N') sym = rand() % 4; 
      else           sym = DNASymToNum(sym);
      symBuf->buf[symBuf->idx] = sym;

      n = 0;
      pos = &symBuf->buf[symBuf->idx-1];
      GetIdxRM(pos, RM);
      if(++base > P->kmer){
        #ifdef NON_RANDOM_MODE
        positions[idx++] = GetPositionRM(RM, Hash);
        #elif
        int64_t pos_idx = GetPositionRM(RM, Hash);
        if(pos_idx == -1)
          positions[idx++] = rand() % 2300000000;  // RANDOMIZE by number of chars
        else
          positions[idx++] = pos_idx;
        #endif
        }
      UpdateCBuffer(symBuf);
      }

    HeapSort(positions, idx-1);
    int64_t read_idx = CumulativeElastic(positions, idx-1);
    WriteRead(Read, read_idx, F);

    PA->nRead++;
    }

  fclose(F);
  RemoveRModel(RM);
  RemoveCBuffer(symBuf);
  RemoveParser(PA);
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - R E F E R E N C E - - - - - - - - - - - - - -
void LoadReference(void){
  FILE      *Reader = Fopen(P->reference, "r");
  uint64_t  idx = 0;
  uint64_t  k, idxPos, pos = 0, i = 0;
  PARSER    *PA = CreateParser();
  CBUF      *symBuf  = CreateCBuffer(BUFFER_SIZE, BGUARD);
  uint8_t   *readBuf = Calloc(BUFFER_SIZE, sizeof(uint8_t));
  uint8_t   sym;
  FileType(PA, Reader);
  uint64_t  file_length = NBytesInFile(Reader);
  RMODEL    *RM = CreateRModel(P->minimum, P->kmer);

  idx = 0;
  while((k = fread(readBuf, 1, BUFFER_SIZE, Reader)))
    for(idxPos = 0 ; idxPos < k ; ++idxPos){

      if(ParseSym(PA, (sym = readBuf[idxPos])) == -1){
        idx = 0;
        continue;
        }
 
      symBuf->buf[symBuf->idx] = sym = DNASymToNum(sym);
  
      if(sym != 4){
        symBuf->buf[symBuf->idx] = sym;
        GetIdxRM(symBuf->buf+symBuf->idx-1, RM);
        UpdateRM(RM, Hash, pos++);
        UpdateCBuffer(symBuf);
        }

      CalcProgress(file_length, ++i);
      }

  RemoveRModel(RM);
  RemoveCBuffer(symBuf);
  Free(readBuf);
  RemoveParser(PA);
  fclose(Reader);
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - - M A P P E R - - - - - - - - - - - - - - - -
void MapAction(){

  fprintf(stderr, "  [+] Creating random label ...\n");
  CreateLabel();
  fprintf(stderr, "      Done!                \n");

  fprintf(stderr, "  [+] Building hash ...\n");
  Hash = CreateHash();
  fprintf(stderr, "      Done!                \n");

  fprintf(stderr, "  [+] Loading reference ...\n");
  LoadReference();
  fprintf(stderr, "      Done!                \n");

  fprintf(stderr, "  [+] Mapping reads ... \n");
  MapTarget();
  fprintf(stderr, "      Done!                \n");

  fprintf(stderr, "  [+] Order reads ... \n");
  OrderReads(32768); // 32768 -> 32 GB of RAM
  fprintf(stderr, "      Done!                \n");
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - M A I N - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int32_t main(int argc, char *argv[]){
  char **p = *&argv;

  P = (Parameters *) Malloc(1 * sizeof(Parameters));
  if((P->help = ArgsState(DEF_HELP, p, argc, "-h")) == 1 || argc < 2){
    PrintMenu();
    return EXIT_SUCCESS;
    }

  if(ArgsState(DEF_VERSION, p, argc, "-V")){
    PrintVersion();
    return EXIT_SUCCESS;
    }

  P->verbose    = ArgsState (DEF_VERBOSE, p, argc, "-v" );
  P->force      = ArgsState (DEF_FORCE,   p, argc, "-F" );
  P->order      = ArgsState (DEF_ORDER,   p, argc, "-N" );
  P->delete     = ArgsState (DEF_DELETE,  p, argc, "-D" );
  P->header     = ArgsState (DEF_HEADER,  p, argc, "-W" );
  P->kmer       = ArgsNum   (DEF_KMER,    p, argc, "-k", MIN_KMER, MAX_KMER);
  P->minimum    = ArgsNum   (DEF_MINI,    p, argc, "-m", MIN_MINI, MAX_MINI);
  P->reference  = argv[argc-1];

  fprintf(stderr, "\n");
  if(P->verbose) PrintArgs(P);

  fprintf(stderr, "==[ PROCESSING ]====================\n");
  TIME *Time = CreateClock(clock());
  MapAction();
  StopTimeNDRM(Time, clock());
  fprintf(stderr, "\n");

  fprintf(stderr, "==[ STATISTICS ]====================\n");
  StopCalcAll(Time, clock());
  fprintf(stderr, "\n");
  RemoveClock(Time);

  return EXIT_SUCCESS;
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
