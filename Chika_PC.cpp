#include <cstdio>
#include <chrono>
#include "ChVM.hpp"

void ChVM_Harness::print (const char* output) {
  printf("%s\n", output);
}

void ChVM_Harness::printInt (const char* output, uint32_t number) {
  printf("%s 0x%X ; %d\n", output, number, number);
}

void ChVM_Harness::printMem (uint8_t* mem, uint8_t by) {
  uint8_t left = by / 2;
  for (uint8_t i = 0; i < left; ++i)
    printf(". ");
  printf("V ");
  for (uint8_t i = 1; i < by; ++i)
    printf(". ");
  printf("\n");
  uint8_t* mEnd = mem + by;
  for (uint8_t* m = mem - left; m < mEnd; ++m) {
    if (*m < 16) printf("0");
    printf("%X", *m);
  }
  printf("\n");
}

void ChVM_Harness::printItems (uint8_t* pItems, uint32_t n) {
  printf("Items: ");
  for (uint8_t it = 0; it < n; ++it) {
    Item* item = (Item*)(pItems - (it * sizeof(Item)));
    printf("%d#l%dt%X%c ", it, item->len, item->type(), item->isConst() ? 'c' : ' ');
  }
  printf("\n");
}

int fsize (FILE *fp) {
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    return sz;
}

//Reads from `offset` in the file for `count` bytes
//If `count` is 0 the rest of the file is read
int32_t ChVM_Harness::fileRead (const char* path, uint8_t* blob, uint32_t offset, uint32_t count) {
  FILE* fp = fopen(path, "rb");
  if (!fp) return 0;
  size_t fLen = fsize(fp);
  if (offset + count > fLen) {
    if (offset > fLen) return 0;
    count = 0;
  }
  if (!count) count = fLen - offset;
  fseek(fp, offset, SEEK_SET);
  count = fread(blob, sizeof(uint8_t), count, fp);
  fclose(fp);
  return count;
}

bool ChVM_Harness::fileWrite (const char* path, uint8_t* blob, uint32_t offset, uint32_t count) {
  FILE* fp = fopen(path, "wb");
  if (!fp) return false;
  fseek(fp, offset, SEEK_SET);
  fwrite(blob, sizeof(uint8_t), count, fp);
  fclose(fp);
  return true;
}

bool ChVM_Harness::fileAppend (const char* path, uint8_t* blob, uint32_t count) {
  FILE* fp = fopen(path, "ab");
  if (!fp) return false;
  fwrite(blob, sizeof(uint8_t), count, fp);
  fclose(fp);
  return true;
}

bool ChVM_Harness::fileDelete (const char* path) {
  return remove(path);
}

auto start_time = std::chrono::high_resolution_clock::now();
uint32_t ChVM_Harness::msNow () {
  auto current_time = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
}


ChVM machine = ChVM();
uint8_t mem[CHIKA_SIZE];
uint8_t pNum = 0;

bool ChVM_Harness::loadProg (const char* path) {
  FILE* fp = fopen(path, "rb");
  if (!fp) return false;
  size_t fLen = fsize(fp);
  bytenum ramLen;
  fread((char*)&ramLen, sizeof(bytenum), 1, fp);
  progs[pNum].ramLen = ramLen <= MAX_PROG_RAM ? ramLen : MAX_PROG_RAM;
  machine.setPNum(pNum);
  fread((char*)machine.pROM, sizeof(bytenum), fLen, fp);
  fclose(fp);
  machine.romLen(fLen - sizeof(bytenum));
  machine.entry();
  return ++pNum;
}

void ChVM_Harness::unloadProg (const char* path) {
  //TODO
  --pNum;
}

int main (int argc, char* argv[]) {
  ChVM_Harness harness = ChVM_Harness();
  machine.harness = &harness;
  machine.mem = mem;

  if (argc == 2) harness.loadProg(argv[1]);
  else           harness.loadProg("init.kua");

  //Beat all hearts once, check if all are dead for early exit
  {
    bool allDead = true;
    for (uint8_t p = 0; p < pNum; ++p)
      if (machine.heartbeat(p))
        allDead = false;
    if (allDead) return 0;
  }

  //Round-robin the heartbeats
  while (true)
    for (uint8_t p = 0; p < pNum; ++p)
      machine.heartbeat(p);
}
