#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
void wc(FILE *ofile, FILE *infile, char *inname) {
  char *code;
  int c;
  size_t n = 0;
  code = malloc(1000);
  int char_count = 0;
  int line_count = 0;
  int word_count = 0;
  char path[1024];
  char result[1024];
  int fd = fileno(ofile);
  while ((c = fgetc(ofile)) != EOF)
  {
    code[n++] = (char) c;
  }
    
  code[n] = '\0';        
  char_count = count_characters(code);
  line_count = count_lines(code);
  word_count = count_words(code);
  sprintf(path, "/proc/self/fd/%d",fd); 
  memset(result, 0, sizeof(result));
  readlink(path, result, sizeof(result)-1);
  
 
  printf(" %d\t%d\t%d\t%s\t%s\n", line_count, word_count, char_count, result, path);
}

int count_characters(char *source)
{
  int characters = 0;
  for( int i = 0; i < strlen(source); i++ )
  {
    characters++;
  }
  return characters;
}

int count_lines(char *source)
{
  int lines = 0;
  for( int i = 0; i<strlen(source) ; i++)
  {
    if(source[i] == '\n')
      lines++;
  }
  return lines;
}

int count_words(char *source)
{
  int words = 0;
  int inword = 0;
  printf("%s\n", source);
  do switch(*source){
    case '\0':
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      if(inword)
      {
        inword = 0;
        words++;
      }
      break;
    default:
      inword = 1;
  }while(*source++);
  /*for( int i = 0; i<strlen(source) ; i++)
  {
    if(source[i] == " ")
      words++;
  }*/
  return words;
}


int main (int argc, char *argv[]) {
    int char_count = 0;
    int line_count = 0;
    int word_count = 0;
    if(argv[2])
    {
      if(access(argv[2],F_OK) != -1)
      {
        wc(fopen(argv[2], "r"),"","");
      }
      else
      {
        char_count = count_characters(argv[2]);
        line_count = count_lines(argv[2]);
        word_count = count_words(argv[2]);
        printf(" %d\t%d\t%d\t%s\n", line_count, char_count, word_count, argv[2]);
      }
    }
    else
    {
      printf("No argument passed");
    }
    return 0;
}
