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
#include "buffer.h"
#include "common.h"
#include "rmodel.h"

HASH     *Hash; // HASH MEMORY IS PUBLIC

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - C O M P R E S S I N G - - - - - - - - - - - - - 
void MapTarget(void){

/*
  FILE      *Reader = Fopen(P->Con.name, "r");
  char      name[MAX_FILENAME];
  sprintf(name, ".map%u", T.id+1);
  FILE      *Writter = Fopen(concatenate(P->positions, name), "w");
  int64_t   nBaseRelative = 0, nBaseAbsolute = 0, idxPos = 0;
  uint32_t  k, r = 0;
  int32_t   action;
  PARSER    *PA = CreateParser();
  CBUF      *symBuf = CreateCBuffer(BUFFER_SIZE, BGUARD);
  uint8_t   *readBuf = (uint8_t *) Calloc(BUFFER_SIZE, sizeof(uint8_t)), sym,
            *conName = (uint8_t *) Calloc(MAX_CONTIG_NAME, sizeof(uint8_t));
  RCLASS    *Mod = CreateRClass(P->repeats, P->minimum, P->kmer, P->inversion);

  Mod->nBases = P->Ref.nBases;
  while((k = fread(readBuf, 1, BUFFER_SIZE, Reader)))
    for(idxPos = 0 ; idxPos < k ; ++idxPos){
      sym = readBuf[idxPos];
      if((action = ParseSym(PA, sym)) < 0){
        switch(action){
          case -1: // IT IS THE BEGGINING OF THE HEADER
            if(PA->nRead > 1 && Mod->nRM > 0)
              ResetAllRMs(Mod, Head, nBaseRelative, nBaseAbsolute, conName, 
              Writter);
            nBaseRelative = 0;
            r = 0;
          break;
          case -2: // IT IS THE '\n' HEADER END
            conName[r] = '\0';
          break;
          case -3: // IF IS A SYMBOL OF THE HEADER
            if(r >= MAX_CONTIG_NAME-1)
              conName[r] = '\0';
            else{ 
              if(sym == ' '){
                if(r == 0) continue;
                else       sym = '_'; // PROTECT SPACES WITH UNDERL
                }
              conName[r++] = sym;        
              }
          break;
          case -99: // IF IS A SIMPLE FORMAT BREAK
          break;
          default:
            fprintf(stderr, "ERROR: Unknown action!\n");
            exit(1);
          }
        continue; // GO TO NEXT SYMBOL
        }

      if((sym = DNASymToNum(sym)) == 4){
        if(Mod->nRM > 0 && PA->nRead % P->nThreads == T.id) 
          ResetAllRMs(Mod, Head, nBaseRelative, nBaseAbsolute, conName, 
          Writter);
        ++nBaseRelative;
        ++nBaseAbsolute;
        continue;
        }
      
      symBuf->buf[symBuf->idx] = sym;
      GetIdxRM   (symBuf->buf+symBuf->idx, Mod);
      GetIdxRevRM(symBuf->buf+symBuf->idx, Mod);

      if(PA->nRead % P->nThreads == T.id){
        if(nBaseRelative >= Mod->kmer){  // PROTECTING THE BEGGINING OF K-SIZE
          UpdateRMs(Mod, Seq->buf, nBaseRelative, sym);
          StopRMs(Mod, Head, nBaseRelative, nBaseAbsolute, conName, Writter);
          StartMultipleRMs(Mod, Hash, nBaseRelative, nBaseAbsolute);
          }
        }

      UpdateCBuffer(symBuf);
      ++nBaseRelative;
      ++nBaseAbsolute;
      }

  P->Con.nBases = nBaseAbsolute;
  Free(readBuf);
  Free(conName);
  RemoveCBuffer(symBuf);
  RemoveParser(PA);
  RemoveRClass(Mod);
  fclose(Writter);
  fclose(Reader);
*/
  }

//////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - R E F E R E N C E - - - - - - - - - - - - - -
void LoadReference(void){
  FILE      *Reader = Fopen(P->reference, "r");
  uint32_t  n;
  uint64_t  idx = 0;
  uint64_t  k, idxPos, pos = 0, i = 0;
  PARSER    *PA = CreateParser();
  CBUF      *symBuf  = CreateCBuffer(BUFFER_SIZE, BGUARD);
  uint8_t   *readBuf = Calloc(BUFFER_SIZE, sizeof(uint8_t));
  uint8_t   sym;
  FileType(PA, Reader);
  uint64_t  file_length = NBytesInFile(Reader);
  RMODEL    *RM = CreateRModel(P->minimum, P->kmer);

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
  uint32_t n;
  pthread_t t[P->nThreads];
  Threads  *T = (Threads *) Calloc(P->nThreads, sizeof(Threads));
  for(n = 0 ; n < P->nThreads ; ++n) T[n].id = n; 

  fprintf(stderr, "  [+] Building hash ...\n");
  Hash = CreateHash();
  fprintf(stderr, "      Done!                \n");

  fprintf(stderr, "  [+] Loading reference ...\n");
  LoadReference();
  fprintf(stderr, "      Done!                \n");

  fprintf(stderr, "  [+] Map contigs ... \n");
  MapTarget();
  fprintf(stderr, "\r      Done!                   \n");
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
  P->kmer       = ArgsNum   (DEF_KMER,    p, argc, "-k", MIN_KMER, MAX_KMER);
  P->minimum    = ArgsNum   (DEF_MINI,    p, argc, "-m", MIN_MINI, MAX_MINI);
  P->nThreads   = ArgsNum   (DEF_THRE,    p, argc, "-n", MIN_THRE, MAX_THRE);
  P->reference  = argv[argc-1];

  if(P->minimum < P->kmer){
    fprintf(stderr, "  [x] Error: minimum block size must be >= than k-mer!\n");
    exit(1);
    }

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
