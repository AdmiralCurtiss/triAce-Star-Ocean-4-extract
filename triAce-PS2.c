//
// triAce-PSE.c
// simple tri-Ace PS2 unpacker
// (c) CUE
//

// removed "triAce-PS2.h" (no reason)

// includes
#include <stdio.h>
#include <stdlib.h>

// types
typedef unsigned char      U08;
typedef unsigned short int U16;
typedef unsigned int       U32;
typedef unsigned long long U64;

// EXIT macro
#define STOP(msg) { printf("%s\n", msg); return(1); }

// DVD sector size (2048 bytes)
#define SECTOR_LEN    0x800

// needed to check/decode the games:
// - initial seed
// - first 32-bit value in the table
// - ISO table offset
// - number of entries
//
// decoded table format:
// - a first 32-bits table with the LBA (file offset)
// - a second 32-bits table with the number of sectors (file length)
// - a third 32-bits table (not used)

// "Star Ocean 3" data
#define SEED_SO3      0x13578642
#define SIGNATURE_SO3 0x27D51556
#define TABLE_SO3     0x00200000
#define TOTAL_SO3     0x1800

// "Radiata Stories" data
#define SEED_RS       0x13578642
#define SIGNATURE_RS  0x27D51556
#define TABLE_RS      0x3C6C1800
#define TOTAL_RS      0x1200

// "Valkyrie Profile 2" data
#define SEED_VP2      0x49287491
#define SIGNATURE_VP2 0x516F6699
#define TABLE_VP2     0x00200000
#define TOTAL_VP2     0x0C00

// 4 first bytes of the file to put the extension
#define HEADER_ELF    0x464C457F
#define HEADER_SLZ0   0x005A4C53
#define HEADER_SLZ1   0x015A4C53
#define HEADER_SLZ2   0x025A4C53
#define HEADER_SLZ3   0x035A4C53
#define HEADER_SLE0   0x00454C53
#define HEADER_SLE1   0x01454C53
#define HEADER_SLE2   0x02454C53
#define HEADER_SLE3   0x03454C53
#define HEADER_ZLS    0x00534C5A
#define HEADER_SEQ    0x57514553
#define HEADER_PAC    0x4B434150
#define HEADER_TXT    0x73696854
#define HEADER_DMY    0x00594D44
#define HEADER_010    0x00000010
#define HEADER_020    0x00000020
#define HEADER_MC     0x6D336F73
#define HEADER_IDX    0x27D51556
#define HEADER_UNK    0x67225277
#define HEADER_000    0x00000000
#define HEADER_KOD    0x73646F4B
#define HEADER_RCP    0x00504352
#define HEADER_FIS    0x00534946
#define HEADER_IDX3   0x516F6699
#define HEADER_MC3    0x7370636D
#define HEADER_CRC    0x00435243

// extensions
#define EXTENSION_ELF ".elf"
#define EXTENSION_SLZ ".slz"
#define EXTENSION_SLE ".sle"
#define EXTENSION_ZLS ".zls"
#define EXTENSION_SEQ ".seq"
#define EXTENSION_PAC ".pac"
#define EXTENSION_TXT ".txt"
#define EXTENSION_DMY ".dmy"
#define EXTENSION_010 ".010"
#define EXTENSION_020 ".020"
#define EXTENSION_MC  ".mc"
#define EXTENSION_IDX ".idx"
#define EXTENSION_UNK ".unk"
#define EXTENSION_000 ".000"
#define EXTENSION_KOD ".kod"
#define EXTENSION_RCP ".rcp"
#define EXTENSION_FIS ".fis"
#define EXTENSION_CRC ".crc"

// BIN is the generic extension
#define EXTENSION_BIN ".bin"

// extensions for packed files
// i have no time to search more packed types in the BIN files
#define EXTENSION_PK1 ".pk1"
#define EXTENSION_PK2 ".pk2"
#define EXTENSION_PK3 ".pk3"

// all
int main (int argc, char **argv) {
  U32 cue;
  FILE *fp, *fp1;
  U08 *fb;
  U32 *table, seed, key, max, len;
  U64 pos;
  U08 *buffer, fn[256];
  U32 hdr, tmp1, tmp2, tmp3;
  U32 i, j;

  printf(
         "\n"
         "triAce-PS2 - tri-Ace PS2 UnPacker - (c) CUE 2011\n"
         "\n"
         "- Star Ocean 3: Till the End of Time\n"
         "- Radiata Stories\n"
         "- Valkyrie Profile 2: Silmeria\n"
         "\n"
        );

  // parameter to save the decoded data
  if (argc == 4) {
    cue = !strcmpi(argv[3], "-cue") ? 1 : 0;
    if (cue) argc--;
  } else {
    cue = 0;
  }

  if (argc != 3) STOP("Sintaxis: triAce-PS2 imagen carpeta");

  // open the ISO
  if ((fp = fopen(argv[1], "rb")) == NULL) STOP("Error abriendo la imagen");

  // check the game
  pos = TABLE_SO3;
  max = TOTAL_SO3;
  seed = SEED_SO3;
  if (fseeko64(fp, pos, SEEK_SET)) STOP("Error posicionando la imagen");
  if (fread(&tmp1, 4, 1, fp) != 1) STOP("Error leyendo la imagen");
  if (tmp1 != SIGNATURE_SO3) {
    pos = TABLE_RS;
    max = TOTAL_RS;
    seed = SEED_RS;
    if (fseeko64(fp, pos, SEEK_SET)) STOP("Error posicionando la imagen");
    if (fread(&tmp1, 4, 1, fp) != 1) STOP("Error leyendo la imagen");
    if (tmp1 != SIGNATURE_RS) {
      pos = TABLE_VP2;
      max = TOTAL_VP2;
      seed = SEED_VP2;
      if (fseeko64(fp, pos, SEEK_SET)) STOP("Error posicionando la imagen");
      if (fread(&tmp1, 4, 1, fp) != 1) STOP("Error leyendo la imagen");
      if (tmp1 != SIGNATURE_VP2) STOP("Tabla no encontrada");
    }
  }

  // read the encoded data
  len = max * 3;
  if ((fb = (U08 *) malloc(len * 4)) == NULL) STOP("Error de memoria");
  if (fseeko64(fp, pos, SEEK_SET)) STOP("Error posicionando la imagen");
  if (fread(fb, 4, len, fp) != len) STOP("Error leyendo la tabla");

  // 32-bits pointer
  table = (U32 *)fb;

  // decode start
  key = seed;
  for (i = 0; i < max; i++) {
    table[max*0+i] ^= key; key ^= (key << 1);
    table[max*1+i] ^= key; key ^= ~seed;
    table[max*2+i] ^= key; key ^= (key << 2) ^ seed;
  }
  table[0] = pos / SECTOR_LEN;
  // decode end (yes, very simple!!!)

  // save the decoded data
  if (cue) {
    if ((fp1 = fopen("decoded.idx", "wb")) == NULL) STOP("Error creando la tabla");
    if (fwrite(fb, 4, len, fp1) != len) STOP("Error grabando la tabla");
    if (fclose(fp1) == EOF) STOP("Error cerrando la tabla");
  }

  // make folder
  if (mkdir(argv[2]) > 0) STOP("Error creando la carpeta");

  // extract each file
  for (i = 0; i < max; i++) {
    pos = table[max*0+i]; // LBA
    len = table[max*1+i]; // sectors

    // extract not empty files only
    if (len) {
      pos *= (U64) SECTOR_LEN; // real position
      len *= (U32) SECTOR_LEN; // length in bytes

      // read the file
      if ((buffer = (U08 *) malloc(len)) == NULL) STOP("Error de memoria");
      if (fseeko64(fp, pos, SEEK_SET)) STOP("Error posicionando la imagen");
      if (fread(buffer, 1, len, fp) != len) STOP("Error leyendo la imagen");

      // simple check of the 4 first bytes to put the extension
      hdr = *(U32 *)buffer;
      if (!hdr) {
        tmp1 = *(U32 *)(buffer + 0x4);
        tmp2 = *(U32 *)(buffer + 0x8);
        if (!tmp1 && (tmp2 == HEADER_010)) {
          sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_010);
          hdr = 0xFFFFFFFF;
        } else if (!tmp1 && (tmp2 == HEADER_000)) {
          sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_000);
          hdr = 0xFFFFFFFF;
        } else if (16 * (tmp1 + 1) == tmp2) {
          tmp3 = *(U32 *)(buffer + 0x1C);
          if (tmp3 == tmp2) {
            sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_PK1);
            hdr = 0xFFFFFFFF;
          }
        }
      } else if (hdr == HEADER_020) {
        sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_020);
        hdr = 0xFFFFFFFF;
      } else if (hdr < 0x100) {
        tmp1 = *(U32 *)(buffer + 0x10);
        tmp2 = *(U32 *)(buffer + 0x14);
        for (j = 0; j < hdr; j++) {
          tmp1 = *(U16 *)(buffer + 4 + 4 * j + 0);
          tmp2 = *(U16 *)(buffer + 4 + 4 * j + 2);
          tmp3 = *(U16 *)(buffer + 4 + 4 * j + 4);
          if (tmp1 + tmp2 != tmp3) break;
        }
        if ((tmp1 + tmp2) * SECTOR_LEN == len) {
          sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_PK2);
          hdr = 0xFFFFFFFF;
        }
      } else {
        tmp1 = *(U32 *)(buffer + 0x14);
        if (tmp1 == len) {
          sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_PK3);
          hdr = 0xFFFFFFFF;
        }
      }

      // file name with the extension
      if (hdr != 0xFFFFFFFF) {
        switch (hdr) {
          case HEADER_ELF : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_ELF); break;
          case HEADER_SLZ0:
          case HEADER_SLZ1:
          case HEADER_SLZ2:
          case HEADER_SLZ3: sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_SLZ); break;
          case HEADER_SLE0:
          case HEADER_SLE1:
          case HEADER_SLE2:
          case HEADER_SLE3: sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_SLE); break;
          case HEADER_ZLS : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_ZLS); break;
          case HEADER_SEQ : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_SEQ); break;
          case HEADER_PAC : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_PAC); break;
          case HEADER_TXT : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_TXT); break;
          case HEADER_DMY : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_DMY); break;
          case HEADER_MC  :
          case HEADER_MC3 : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_MC);  break;
          case HEADER_IDX :
          case HEADER_IDX3: sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_IDX); break;
          case HEADER_UNK : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_UNK); break;
          case HEADER_KOD : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_KOD); break;
          case HEADER_RCP : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_RCP); break;
          case HEADER_FIS : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_FIS); break;
          case HEADER_CRC : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_CRC); break;
          default         : sprintf(fn, "%s/%04d%s", argv[2], i, EXTENSION_BIN); break;
        }
      }

      // show the info
      if (cue) {
        printf("pos=%010I64X  len=%08X  ->  ", pos, len);
      }
      printf("%s\n", fn);

      // save the file
      if ((fp1 = fopen(fn, "wb")) == NULL) STOP("Error creando el fichero");
      if (fwrite(buffer, 1, len, fp1) != len) STOP("Error grabando el fichero");
      if (fclose(fp1) == EOF) STOP("Error cerrando el fichero");

      // free the memory used by the file
      free(buffer);
    }
  }

  // free the memory used by the encoded data
  free(fb);

  // close the ISO
  if (fclose(fp) == EOF) STOP("Error cerrando la iso");

  // normal exit
  return(0);
}

// eof
