/*************************
*   Vaibhav Kashyap      *     
*    2012A3PS143P        *   
**************************
**************************
*   Bhavin Senjaliya     *     
*    2012A8PS274P        *   
*************************/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#define max_arguments 10
#define NORMAL        0
#define OUTPUT        1
#define INPUT         2
#define PIPELINE      3
#define BG            4
#define APPE          5
char spaces[] = " \t\r\n\v";
char symb[] = "<|>";
struct cmd {
  int type;        
};
struct redir_cmd 
{
  int type;          
  struct cmd *cmd;   
  char *file;        
  int mode;          
  int fd;            
};
struct npipe_cmd {
  int type;          
  struct cmd *left;  
  struct cmd *right1,*right2; 
};
struct pipe_cmd 
{
  int type;          
  struct cmd *left;  
  struct cmd *right; 
};
struct exec_cmd 
{
  int type;             
  char *argv[max_arguments];  
};



void set_redir(struct redir_cmd*);
int forking(void);  
struct cmd *parse_cmd(char*);
void runcmd(struct cmd*);
void dup2_wrap(int, int);
char *find_path(char*); 
 

int main(void)
{
  
  while(1)
  {    
      printf("Enter your command and Ctl+C to exit.\n");
      int r;
      int i, mode = NORMAL, cmdArgc;
      char input[100],commands[100][10],operators[100][3],buf[100];
      int l,j,comm=0,cnt1=0,pipes=0,cnt[100][100],jump=0,cnt2;
      int k=0;char st[120][120];
      char Current_Dir[100];
      char *cpt, *inp_str, *CmdArgv[100], *SuppPtr = NULL;
      int flag=0,flg2=0,flg3=0;
      getcwd(Current_Dir, 100);
      printf("%s@%s$ ",getlogin(), Current_Dir);
      fgets(input,1024,stdin);
      l = strlen(input);
      input[l-1]='\0';
      if(strcmp(input, "exit") == 0)
        exit(0);
      for(i=0;i<l;i++)
      {
        commands[comm][cnt1] = input[i];
        if(input[i]=='|' || input[i] ==',' )
        {
          if(input[i]==',')
          {
            commands[comm][cnt1]='\0';
            operators[comm][0] = ',';
            operators[comm][1] = '\0';
            if(flg2==0)
              {
                  strcpy(st[k],commands[comm]);
                  k++;
              }
            comm++;
            cnt1=-1;
          }
          else if(input[i]=='|'&&input[i+1]=='|')
          {
            flg2 = 1;
            commands[comm][cnt1]='\0';
            operators[comm][0] = '|';
            operators[comm][1] = '|';
            operators[comm][2] = '\0';
            pipes++;
            pipes++;
            i++;
            if(input[i]=='|'&&input[i+1]=='|')
            {
              strcpy(operators[comm] , "|||");
              pipes++;
              i++;
            }
            comm++;
            cnt1=-1;
          }
        }
        cnt1++;
      }
      if(flg2==1) 
      {
        
        int opera = comm;
        for(i=0;i<opera;i++)
        {
          for(j=0;j<2;j++)
          {
            if(operators[i][j]==' ')
            {
              operators[i][j] = operators[i][j+1];
            }
          }
          for(j=3;j>0;j--)
          {
            if(operators[i][j]==' ')
            {
              operators[i][j] = '\0';
              break;
            }
          }
          //printf("operators are (%s)[%d]\n",operators[i],i);
        }

        for(i=0;i<comm;i++)
        {
            if(strcmp(operators[i],"|||")==0||strcmp(operators[i],"|||,")==0)
            {
              cnt2 = 3;flag=1;
            }
            if(strcmp(operators[i],"||")==0)
            {
              cnt2 = 2;flag=1;
            }
            if(flag==1)
            {
              for(j=i+1;j<comm+1;j++)
              {
                cnt[j+jump-i][j-1] = i;
                cnt2--;
                if(cnt2==0)
                  flag=2;
                if(strcmp(operators[j],"||")==0)
                  jump=2;
                if(strcmp(operators[j],"|||")==0)
                  jump=3;

              }
            }

        }
        //printf("3\n");

        for(i=0;i<18;i++)
        {
            for(j=0;j<18;j++)
            {
                if(cnt[i][j]>100)
                    cnt[i][j]=-1;
            }
        }

          for(j=0;j<comm+1;j++)
          {
              st[k][0]='\0';
              for(i=comm+1;i>0;i--)
              {
                  if(cnt[i][j]!=-1)
                  {
                     strcat(st[k],commands[cnt[i][j]]);
                     strcat(st[k],"|");
                  }
                  if(i==1)
                  {
                      strcat(st[k],commands[j+1]);
                      k++;
                  }
              }

          }

      if(k!=0)
        {
            flg3 = 1;
            for(i=0;i<k-1;i++)   
            { 
              int z;
              for(z=0;z<strlen(st[i]);z++)
              {
                if(st[i][z]==',')
                {
                  st[i][z]='\0';
                  break;
                }
                if(st[i][z]=='.')
                {
                  st[i][z+1]='t';
                  st[i][z+3]='t';
                  st[i][z+2]='x';
                  st[i][z+4]='\0';
                  break;
                }
              }
              mode = NORMAL;
              inp_str= strstr(st[i],"|");
              if(strcmp(operators[0],"|||")==0||strcmp(operators[0],"|||,")==0)
                inp_str =strstr(inp_str+1,"|");
              inp_str =strstr(inp_str+1,"|");
              // st[i][0] = '\0';
              if(forking() == 0)
                  runcmd(parse_cmd(st[i]));
                wait(&r);
            }
        }

      }
      else
      {
        if(forking() == 0)
          runcmd(parse_cmd(input));
        wait(&r);
      }
  }
}

int forking(void)
{
  int process_id;
  process_id = fork();
  if(process_id == -1)
    perror("Error In Forking.");
  return process_id;
}

struct cmd* exec_cmd(void)
{
  struct exec_cmd *cmd;
  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd* redir_cmd(struct cmd *temp_cmd, char *file, int type)
{
  struct redir_cmd *cmd;
  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = temp_cmd;
  cmd->file = file;
  cmd->mode = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd* pipe_cmd(struct cmd *left, struct cmd *right)
{
  struct pipe_cmd *cmd;
  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

int get_operator(char **string_pt, char *es, char **q, char **en_arg)
{
  char *s;
  int return_val;
  s = *string_pt;
  while(s < es && strchr(spaces, *s))
      s++;
  if(q)
      *q = s;
  return_val = *s;
  switch(*s)
  {
    case 0:
      break;
    case '|':
    case '<':
      s++;
      break;
    case '>':
      s++;
      break;
    default:
      return_val = 'a';
    while(s < es && !strchr(spaces, *s) && !strchr(symb, *s))
        s++;
    break;
  }
  if(en_arg)
    *en_arg = s;
  
  while(s < es && strchr(spaces, *s))
    s++;
  *string_pt = s;
  return return_val;
}

int peek_action(char **string_pt, char *es, char *toks)
{
  char *s;
  
  s = *string_pt;
  while(s < es && strchr(spaces, *s))
    s++;
  *string_pt = s;
  return *s && strchr(toks, *s);
}

struct cmd *parse_line(char**, char*);
struct cmd *parse_pipe(char**, char*);
struct cmd *parce_exec(char**, char*);

char  *make_copy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd* parse_cmd(char *s)
{
  char *es;
  struct cmd *cmd;
  es = s + strlen(s);
  cmd = parse_line(&s, es);
  peek_action(&s, es, "");
  if(s != es){
    fprintf(stderr, "The Leftovers is: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd* parse_line(char **string_pt, char *es)
{
  struct cmd *cmd;
  cmd = parse_pipe(string_pt, es);
  return cmd;
}

struct cmd* parse_pipe(char **string_pt, char *es)
{
  struct cmd *cmd;
  cmd = parce_exec(string_pt, es);
  if(peek_action(string_pt, es, "|"))
  {
    get_operator(string_pt, es, 0, 0);
    cmd = pipe_cmd(cmd, parse_pipe(string_pt, es));
  }
  return cmd;
}

struct cmd* parse_dir(struct cmd *cmd, char **string_pt, char *es)
{
  int tok;
  char *q, *en_arg;

  while(peek_action(string_pt, es, "<>"))
  {
    tok = get_operator(string_pt, es, 0, 0);
    if(get_operator(string_pt, es, &q, &en_arg) != 'a') 
    {
      fprintf(stderr, "File not Found For Rediraction.\n");
      exit(-1);
    }
    switch(tok)
    {
      case '<':
        cmd = redir_cmd(cmd, make_copy(q, en_arg), '<');
        break;
      case '>':
        cmd = redir_cmd(cmd, make_copy(q, en_arg), '>');
        break;
    }
  }
  return cmd;
}

struct cmd* parce_exec(char **string_pt, char *es)
{
  char *q, *en_arg;
  int tok, argc;
  struct exec_cmd *cmd;
  struct cmd *return_val;
  return_val = exec_cmd();
  cmd = (struct exec_cmd*)return_val;
  argc = 0;
  return_val = parse_dir(return_val, string_pt, es);
  while(!peek_action(string_pt, es, "|"))
  {
    if((tok=get_operator(string_pt, es, &q, &en_arg)) == 0)
      break;
    if(tok != 'a') 
    {
      fprintf(stderr, "Error In Syntax\n");
      exit(-1);
    }
    cmd->argv[argc] = make_copy(q, en_arg);
    argc++;
    if(argc >= max_arguments) 
    {
      fprintf(stderr, "Too many Arguments.\n");
      exit(-1);
    }
    return_val = parse_dir(return_val, string_pt, es);
  }
  cmd->argv[argc] = 0;
  return return_val;
}


void runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct exec_cmd *ecmd;
  struct pipe_cmd *pcmd;
  struct redir_cmd *rcmd;
  struct npipe_cmd *npcmd;
  if ((cmd->type));
  if(cmd == 0)
    exit(EXIT_SUCCESS);
  errno = 0;
  switch(cmd->type){
  default:
    fprintf(stderr, "Unknown Command\n");
    exit(EXIT_FAILURE);

  case ' ':
    ecmd = (struct exec_cmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(EXIT_SUCCESS);
    execv(find_path(ecmd->argv[0]), ecmd->argv);
    fprintf(stderr, "Error In Execv %s\n", strerror(errno));
    exit(EXIT_FAILURE);

  case '>':
  case '<':
    rcmd = (struct redir_cmd*)cmd;
    set_redir(rcmd);
    runcmd(rcmd->cmd);

  case '|':
    pcmd = (struct pipe_cmd*)cmd;
    int ans = pipe(p);
    if (ans == -1) 
    {
      fprintf(stderr, "Error In Pipe: %s\n",strerror(errno));
      exit(EXIT_FAILURE);
    }

    if (forking() == 0)
    {
      close(p[0]);
      dup2_wrap(p[1], STDOUT_FILENO);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if (forking() == 0) 
    {
      close(p[1]);
      dup2_wrap(p[0], STDIN_FILENO);
      close(p[0]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait(&r);
    wait(&r);
    exit(EXIT_SUCCESS);
  }    
  exit(EXIT_FAILURE); // should never reach here
}

void set_redir(struct redir_cmd *cmd)
{
  int redir_fd = 0;
  if (cmd->type == '>') 
  { 
    redir_fd = open(cmd->file, cmd->mode, S_IRWXU);
  } 
  else 
  { 
    redir_fd = open(cmd->file, cmd->mode);
  }
  if (redir_fd < 0) 
  {
    fprintf(stderr, "Error In Rediraction: %s\n",strerror(errno));
    exit(EXIT_FAILURE);
  }
  dup2_wrap(redir_fd, cmd->fd);
}

void dup2_wrap(int old_fd, int new_fd)
{
  int ans = dup2(old_fd, new_fd);
  if (ans < 0) 
  {
    fprintf(stderr, "Error in Dup: %s\n",strerror(errno));
    exit(EXIT_FAILURE);
  }
}

char * find_path(char *exe)
{
  DIR *d;
  struct dirent *dir;
  char *paths = getenv("PATH");
  char *path_dir = strtok(paths, ":");
  while (path_dir != NULL) 
  {
    d = opendir(path_dir);
    if (d == NULL) 
    {
      fprintf(stderr, "Cannot Open Dir: %s", strerror(errno));
    } 
    else 
    {
      while ((dir = readdir(d)) != NULL) 
      {
        if (strcmp(dir->d_name, exe) == 0) 
        {
          char *fin_path = malloc(strlen(path_dir) + strlen(exe) + 2);
          fin_path = strcat(fin_path, path_dir);
          fin_path = strcat(fin_path, "/");
          fin_path = strcat(fin_path, exe);
          return fin_path;
        }
      }
    }

    path_dir = strtok(NULL, ":");
  }
  return exe;
}
