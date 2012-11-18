
#include <stdio.h>
#include <string.h>  /* memset() */
#include <lightning.h>

static const int maxLoopDepth = 32;
  
static jit_insn codeBuffer[1024*64]; /* This is the max number of instructions. */
static int      dataBuffer[30000];

/* Return the number of times *c is repeated. */
int opCount(FILE* fp, int* c) {
  int n  = 1;  /* Seen it at least 1 time. */
  int cr = *c;

  while ((*c = fgetc(fp)) != EOF) {
    /* Skip invalid characters. */
    if ((*c == '>') ||
        (*c == '<') ||
        (*c == '+') ||
        (*c == '-') ||
        (*c == '.') ||
        (*c == ',') ||
        (*c == '[') ||
        (*c == ']')) {
      if (*c == cr) {
        ++n;
      } else {
        return n;
      }
    }
  }

  return n;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: %s [infile]\n", argv[0]);
    return 0;
  }

  FILE* fp = fopen(argv[1], "r");

  if (!fp) {
    fprintf(stderr, "Error: could not open %s\n", argv[1]);
    return 1;
  }

  memset(dataBuffer, 0, sizeof(dataBuffer));

  /* We use these as a simple stack for the loops. */
  jit_insn* loopStart[maxLoopDepth];
  jit_insn* loopEnd[maxLoopDepth];
  int loop = -1;

  void (*code)();
  /* Writing: code = (void (*)())jit_set_ip(codeBuffer).vptr;
     would seem more natural, but the C99 standard leaves
     casting from "void *" to a function pointer undefined.
     The assignment used below is the POSIX.1-2003 (Technical
     Corrigendum 1) workaround. */
  *(void **)(&code) = jit_set_ip(codeBuffer).vptr;

  /* Save the start of the code buffer. */
  char* start = jit_get_ip().ptr;

  jit_prolog(0);
  jit_movi_p(JIT_V0, dataBuffer);

  int n;
  int c = fgetc(fp);
  do {
    if (jit_get_ip().ptr > (char*)(codeBuffer + sizeof(codeBuffer))) {
      fprintf(stderr, "Error: the program is to big!\n");
      return 4;
    }

    switch (c) {
      case '>': {
        n = opCount(fp, &c);
        jit_addi_i(JIT_V0, JIT_V0, n);
        continue;
      }

      case '<': {
        n = opCount(fp, &c);
        jit_subi_i(JIT_V0, JIT_V0, n);
        continue;
      }

      case '+': {
        n = opCount(fp, &c);
        jit_ldr_c(JIT_R1, JIT_V0);
        jit_addi_i(JIT_R1, JIT_R1, n);
        jit_str_c(JIT_V0, JIT_R1);
        continue;
      }

      case '-': {
        n = opCount(fp, &c);
        jit_ldr_c(JIT_R1, JIT_V0);
        jit_subi_i(JIT_R1, JIT_R1, n);
        jit_str_c(JIT_V0, JIT_R1);
        continue;
      }

      case '.': {
        jit_ldr_c(JIT_R1, JIT_V0);
        jit_prepare_i(1);
        jit_pusharg_i(JIT_R1);
        jit_finish(putchar);
        break;
      }

      case ',': {
        jit_prepare(0);
        jit_finish(getchar);
        jit_retval_c(JIT_R1);
        jit_str_c(JIT_V0, JIT_R1);
        break;
      }

      case '[': {
        ++loop;

        if (loop >= maxLoopDepth) {
          fprintf(stderr, "Error: nesting level to deep (max %d)!\n", maxLoopDepth);
          return 2;
        }

        jit_ldr_c(JIT_R1, JIT_V0);
        loopEnd[loop]   = jit_beqi_i(jit_forward(), JIT_R1, 0);
        loopStart[loop] = jit_get_label();
        break;
      }

      case ']': {
        if (loop < 0) {
          fprintf(stderr, "Error: invalid ] (missing [)\n");
          return 3;
        }

        jit_ldr_c(JIT_R1, JIT_V0);
        jit_bnei_i(loopStart[loop], JIT_R1, 0);
        jit_patch(loopEnd[loop]);

        --loop;
        break;
      }
    }
        
    c = fgetc(fp);
  } while (c != EOF);

  fclose(fp);

  if (loop > -1) {
    fprintf(stderr, "Error: %d unclosed loops!\n", loop + 1);
    return 5;
  }

  jit_ret();
  jit_flush_code(start, jit_get_ip().ptr);


  int s = jit_get_ip().ptr - start;
  fp = fopen("code.bin", "wb");
  int i;
  for (i = 0; i < s; ++i) {
    fputc(start[i], fp);
  }
  fclose(fp);


  code();

  return 0;
}

