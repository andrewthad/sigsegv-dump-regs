#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/ucontext.h>
#include <sys/types.h>
#include <ucontext.h>

// Resources:
//
// Figuring out base address of executable:
//   https://stackoverflow.com/questions/24664961/how-to-detect-executable-or-shared-object-in-proc-self-maps-on-linux

volatile sig_atomic_t segfaultCounter = 0;

// Precondition: buffer has space for three digits
void threeDigitDecimal(char* dst, int n) {
  int clipped;
  if (n > 999) {
    clipped = 999;
  } else if (n < 0) {
    clipped = 0;
  } else {
    clipped = n;
  }
  
  int x = clipped / 10;
  int y = x / 10;
  dst[0] = (char)(y + 0x30);
  dst[1] = (char)((x % 10) + 0x30);
  dst[2] = (char)((clipped % 10) + 0x30);
}

// Precondition: argument is less than 16
char fourBitToHexDigit(uint64_t n) {
  if(n < 10) {
    return (char)(n + 0x30);
  } else {
    return (char)((n - 10) + 0x61);
  }
}

// This is safe to use in a signal handler
void writeRegisterToStderr(char* name, uint64_t v) {
  int z;
  z = write(2, name, strlen(name));
  z = write(2, ": 0x", 4);
  char buf[16];
  for(int i = 0; i < 16; i++) {
    uint64_t w = (v >> (60 - (i * 4))) & 0xF;
    buf[i] = fourBitToHexDigit(w);
  }
  z = write(2, buf, 16);
  z = write(2, "\n", 1);
}

void fault_handler(int signo, siginfo_t *info, void *extra) 
{
  ucontext_t *p=(ucontext_t *)extra;
  // int val;
  char signalSpace[] = "Signal ";
  char spaceReceivedNewline[] = " received\n";
  char signoBuf[3];
  int z;
  threeDigitDecimal(signoBuf,signo);
  // Note: we expect to see signal number 11 since that is
  // what SIGSEGV is.
  z = write(2, signalSpace, strlen(signalSpace));
  z = write(2, signoBuf, 3);
  z = write(2, spaceReceivedNewline, strlen(spaceReceivedNewline));
  // printf("Signal %d received\n", signo);
  // printf("siginfo address=%x\n",info->si_addr);
  // val= p->uc_mcontext.gregs[REG_RIP];
  // printf("address = %x\n",val);
  writeRegisterToStderr("REG_RIP",p->uc_mcontext.gregs[REG_RIP]);
  writeRegisterToStderr("REG_R8",p->uc_mcontext.gregs[REG_R8]);
  writeRegisterToStderr("REG_R9",p->uc_mcontext.gregs[REG_R9]);
  writeRegisterToStderr("REG_R10",p->uc_mcontext.gregs[REG_R10]);
  writeRegisterToStderr("REG_R11",p->uc_mcontext.gregs[REG_R11]);
  writeRegisterToStderr("REG_R12",p->uc_mcontext.gregs[REG_R12]);
  writeRegisterToStderr("REG_R13",p->uc_mcontext.gregs[REG_R13]);
  writeRegisterToStderr("REG_R14",p->uc_mcontext.gregs[REG_R14]);
  writeRegisterToStderr("REG_R15",p->uc_mcontext.gregs[REG_R15]);
  writeRegisterToStderr("REG_RDI",p->uc_mcontext.gregs[REG_RDI]);
  writeRegisterToStderr("REG_RSI",p->uc_mcontext.gregs[REG_RSI]);
  writeRegisterToStderr("REG_RBP",p->uc_mcontext.gregs[REG_RBP]);
  writeRegisterToStderr("REG_RBX",p->uc_mcontext.gregs[REG_RBX]);
  writeRegisterToStderr("REG_RDX",p->uc_mcontext.gregs[REG_RDX]);
  writeRegisterToStderr("REG_RAX",p->uc_mcontext.gregs[REG_RAX]);
  writeRegisterToStderr("REG_RCX",p->uc_mcontext.gregs[REG_RCX]);
  writeRegisterToStderr("REG_RSP",p->uc_mcontext.gregs[REG_RSP]);

  // write(2, regRipName, strlen(regRipName));
  // write(2, "\n", 1);
  if(segfaultCounter < 1) {
    segfaultCounter = segfaultCounter + 1;
    if (p->uc_mcontext.gregs[REG_RDX] == 0x1000 && p->uc_mcontext.gregs[REG_RAX] == 0x1000) {
      p->uc_mcontext.gregs[REG_RDX] = 0x2000;
      p->uc_mcontext.gregs[REG_RAX] = 0x2000;
    } else {
      _exit(2);
    }
  } else {
    _exit(2);
  }
}

void printSelfMaps() {
  FILE *fptr; 
  char c;
  fptr = fopen("/proc/self/maps", "r"); 
  if (fptr == NULL) 
  { 
      printf("Cannot open file \n"); 
      _exit(0); 
  } 
  // Read contents from file 
  c = fgetc(fptr); 
  while (c != EOF) 
  { 
      printf ("%c", c); 
      c = fgetc(fptr); 
  } 
  fclose(fptr); 
}

int main () {
  size_t kilobyte = 1024;
  uint8_t* ptr = mmap(NULL, kilobyte * 12, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(ptr == NULL) {
    puts("mmap failed with NULL\n");
    return 1;
  }
  if(ptr == MAP_FAILED) {
    perror("mmap");
    puts("mmap failed with MAP_FAILED\n");
    return 1;
  }
  if (mprotect(ptr, 4096, PROT_READ | PROT_WRITE) == (-1)) {
    perror("mprotect");
    return 1;
  }
  if (mprotect(ptr + 8192, 4096, PROT_READ | PROT_WRITE) == (-1)) {
    perror("mprotect");
    return 1;
  }
  uint64_t* manySixThousands = mmap(NULL, kilobyte * 128, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(ptr == NULL) {
    puts("mmap failed with NULL\n");
    return 1;
  }
  if(ptr == MAP_FAILED) {
    perror("mmap");
    puts("mmap failed with MAP_FAILED\n");
    return 1;
  }
  for(int i = 0; i < 1024 * 16; i++) {
    manySixThousands[i] = 6000;
  }

  printSelfMaps();
  
  struct sigaction action;
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = fault_handler;
  if (sigaction(SIGSEGV, &action, NULL) == -1) {
    perror("sigaction");
    _exit(1);
  }
  // Now we have 4KB of virtual address space that we can write to followed
  // by 4KB of address space that will trigger SIGSEGV when accessed.
  for(int i = 0; i < manySixThousands[i]; i++) {
    uint8_t a = (uint8_t) i;
    ptr[i] = a;
  }
  printf("Magic number: %d\n", (int)(ptr[1]) + (int)(ptr[200]) + (int)(ptr[4000]));
  return 0;
}
