#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

enum ns {norm, sys};
enum kov_work {kov_open, kov_close, kov_error};
enum bg_work {bg_process, norm_process, bg_error}; 
enum add_work {add_mode, append_mode, add_error};
enum close_work {close_ok, close_error};
struct block {
	char *string;
	struct block *next;
	int lenght;
	enum ns status;	
};

struct block_pids{
	int pid_s;
	struct block_pids *next;
};

struct file_block {
	char *file_input, *file_output;
	int fd_input, fd_output, std_input, std_output;
};

void NewBlock(struct block **bl_now_ukaz)
{
	if ((*bl_now_ukaz) -> lenght != 0) 
       	{
		(*bl_now_ukaz) -> next = malloc(sizeof(struct block));
		(*bl_now_ukaz) -> next -> lenght = 0;
		(*bl_now_ukaz) = (*bl_now_ukaz)->next;
	}
}


void AddSysSymb(char c, struct block ***bl_now)
{		
	NewBlock(*bl_now);

	(**bl_now) -> lenght = 1;
	(**bl_now) -> string = malloc(sizeof(char)*2);
	((**bl_now) -> string)[0] = c;
	((**bl_now) -> string)[1] = '\0';
	(**bl_now) -> status = sys;
	NewBlock(*bl_now);
}

/* for stage2  */
char **argsmas(struct block *first)
{
	char **mas_of_strings;
	struct block *bl_now = first;
	int len = 0, i = 0;
	while ((bl_now -> lenght) != 0)
	{
		if ((bl_now -> status) != sys)
			len ++;
		bl_now = bl_now -> next;
	}
	mas_of_strings = malloc((len+1)*sizeof(mas_of_strings));
	bl_now = first;
	while ((bl_now -> lenght) != 0) 
	{
		if ((bl_now -> status) != sys)
		{
			mas_of_strings[i] = bl_now -> string;
			i ++;
		}
		bl_now = bl_now -> next;
	}
	mas_of_strings[i] = NULL;
	return mas_of_strings;
}

int cmp_strings (char *str1, char *str2) 
/* return 1 if strings are = and 0 if != */
{
	int  i = 0;
	while ((str1[i] == str2[i]) && (str1[i] != 0) && (str2[i] != 0))
	{
		i ++;
	}
	return ((str1[i] == 0) && (str2[i] == 0));
}


void create_proc(char **arg, enum bg_work bg_fl)
{
	int pid;
	pid = fork();
	if (pid == -1)
	{
		perror("fork");
		exit(1);
	}
	if (pid == 0)
	{
		execvp(arg[0], arg);
		perror(arg[0]);
		exit(1);
	}
	if (bg_fl == norm_process)
	{
		while(wait(NULL) != pid)
			{}
	}	
	
}

void start_cd(char **arg)
{
	int err;
	if ((arg[2] == NULL) && (arg[1] != NULL))
	{
		err = chdir(arg[1]);
		if (err == -1) 
		{
			perror(arg[1]);
		}
	} else
	{
		printf("Wrong arguments\n");
	}
}
	
void start(char **arg, enum bg_work bg_fl ) 
{
	int  equal;
	if (arg[0] != NULL)
	{
		equal = cmp_strings(arg[0], "cd");
		if (equal)
		{
			start_cd(arg);
		} else
		{
			create_proc(arg, bg_fl);
		}
	}
}


/*  for stage1  */
void AddSymb(char c, struct block *bl_now)
{
	int len;
	len = bl_now -> lenght;
	bl_now -> status = norm; 
	if (len == 0)
	{
		bl_now->string = malloc(2 * sizeof(char));
	}else
	{
		if (((bl_now->lenght)%32) == 1)
		{
			bl_now->string = realloc(bl_now->string,(len+32));
		}
	}
	bl_now->string[len] = c;
	bl_now->string[len + 1] = 0;
	bl_now->lenght++;
}

void NewSystem (struct block **first_ukaz)
{
	while ((*first_ukaz)->lenght != 0)
	{
		/*if ((*fl) == 0)
		{
			PrintStringOfBlock(*first_ukaz);
		}
		*/
		free((*first_ukaz)->string);
		free(*first_ukaz);
		(*first_ukaz) = (*first_ukaz)->next;
	}
	free(*first_ukaz);
	(*first_ukaz) = malloc(sizeof(struct block));
	(*first_ukaz)->lenght = 0;
}

void q_error (struct block ***bl_now, struct block ***first,\
						 enum kov_work  **fl)
{
	NewBlock(*bl_now);
	NewSystem(*first);
	(**bl_now) = (**first);
	printf("Not correct string.\n>");
	(**fl) = 0;
} 

enum bg_work bg_test(struct block *first)  
{
	struct block *begalka;
	begalka = first;
	while ((begalka -> lenght) != 0)
	{
		if ((begalka -> status) == sys)	
		{
			if ((begalka -> string)[0] == '&')
			{
				if (((begalka -> next) -> lenght) == 0)	
					return bg_process;
				else {
					return bg_error;
				}
			}
		}
		begalka = begalka -> next;	
	}
	return norm_process;
}

enum add_work analyze_input_symb(struct block **begalka,enum add_work *mode,\
							char **output_file)
{
	if ((((*begalka) -> next) -> lenght) == 0)
		return add_error;
	if ((((*begalka) -> next) -> status) == norm)
	{
		(*output_file) =  (*begalka) -> next -> string;
		(*begalka) -> next -> status = sys;
		(*mode) = add_mode;
	} else {
		if ((((*begalka) -> next)->string)[0] == '>')
		{
			struct block *begalka2;
			begalka2 = (*begalka) -> next -> next;
			if  (((begalka2->lenght)!=0)&&((begalka2->status)\
								==norm)) 
			{
				(*output_file) =  begalka2 -> string;
				begalka2  -> status = sys;
				(*mode) = append_mode;
			} else {
				return add_error;
			}
		}  else {
			if ((*mode) != append_mode)
			{
				return add_error;
			}
		}
	}
	return (*mode);

}

enum add_work file_test(struct block *first, char **in, char **out)
{
	char *input_file, *output_file;
	struct block *begalka;
	enum add_work mode;
	input_file = NULL;
	output_file = NULL;
	begalka = first;
	while ((begalka -> lenght) != 0)
	{
		if ((begalka -> status) == sys)
		{
			
			if ((begalka -> string)[0] =='>')
			{
				if (analyze_input_symb(&begalka,&mode,\
						&output_file)==add_error)
					return add_error;
			}			
			else if ((begalka -> string)[0] == '<')
			{
				if (((begalka->next) -> status) == norm)
				{
					input_file = begalka -> next -> string;	
					begalka -> next -> status = sys;
				}
				if (((begalka->next) -> lenght) == 0)
				{
					return add_error;
				}
			
				
			}
		}
		begalka = begalka -> next;
	}
	(*in) = input_file;
	(*out) = output_file;
	return mode;
}
				
				
					
							
struct file_block work_with_files(enum add_work *fileflag2,enum add_work mode,\
					char *input_file,char *output_file)
{
	int fd1 = -2, fd2 = -2;
	struct file_block file_w;
	(file_w.std_input) = dup(0);
	(file_w.std_output) = dup(1);
	file_w.file_output = NULL;
	file_w.file_input = NULL;
//	printf("\n %s, %s \n", output_file, input_file);
	if ((mode == add_mode)&&(output_file != NULL))
	{
		fd1 = open(output_file, O_WRONLY|O_CREAT|O_TRUNC, 0666);			
		if (fd1 == -1)
		{
			(*fileflag2) = add_error;
			return file_w;
		}
		(file_w.fd_output) = fd1;
		(file_w.file_output) = output_file;	
		dup2(fd1, 1);
	}
	if ((mode == append_mode)&&(output_file != NULL))
	{
		fd1 = open(output_file, O_WRONLY|O_APPEND);
		if (fd1 == -1)
		{
			(*fileflag2) = add_error;
			return file_w;
		}

		(file_w.fd_output) = fd1;
		(file_w.file_output) = output_file;	
		dup2(fd1, 1);
	}
	
	if (input_file != NULL)
	{	
		fd2 = open(input_file, O_RDONLY);
		if (fd2 == -1)
		{
			(*fileflag2) = add_error;
			return file_w;
		}

		(file_w.fd_input) = fd2;
		(file_w.file_input) = input_file;
		dup2(fd2, 0);	
	}
	return file_w;
}

enum close_work files_close(struct file_block ff )
{
	int close_reg = 0;
	dup2(ff.std_input, 0); 
	dup2(ff.std_output, 1);
	if (ff.file_input != NULL)
	{
		close_reg = close(ff.fd_input);
		if (close_reg == -1)
			return close_error;
		
	}
	if (ff.file_output != NULL)
	{
		close_reg = close(ff.fd_output);
		if (close_reg == -1)
			return close_error;

	}
	return close_ok;

}


int conv_test(struct block ***first)
{
	struct block *begalka;
	begalka = **first;
	int conv_col=0;
	while ((begalka -> lenght) != 0)
	{
		if (((begalka -> status) == sys)&&((begalka->string)[0]=='|'))
			conv_col++;
		begalka = begalka -> next;
	}
	return conv_col;
}

void add_to_mas(char *string, char ***mas, int fl)
{
	if (fl == 0)
	{
//		printf("create %s\n", string);
		(*mas) = malloc (2*sizeof(char *));
		(*mas)[0] = string;
		(*mas)[1] = NULL;
	}
	else if (fl == 1)
	{
		int i=0;
//		printf("add %s\n", string);
		while ((*mas)[i] !=NULL)
			i++;
		(*mas) = realloc((*mas), (i+2)*sizeof(char *));
		(*mas)[i] = string;
		(*mas)[i+1] = NULL;
	}
}



char  ***make_mas(struct block ***first, int conv_col)
{
	struct block *bl_now;
	char ***ar = malloc((conv_col+1)*(sizeof(char **)));
	int i=0;
	int fl=0;				//fl = 0 -> mas is empty
//	printf("CONV_COL: %d\n", conv_col);
	while (i<=(conv_col))
	{
		ar[i] = NULL;
		i++;
		
	}
	bl_now = **first;
	i = 0;
	while ((bl_now -> lenght) != 0)
	{
		if (bl_now->status == norm)
		{
			add_to_mas(bl_now->string, &ar[i], fl);
			fl = 1;			//fl = 1 -> we have smth in mas	
		}
		if ((bl_now->status == sys)&&((bl_now->string)[0]=='|'))
		{
			i++;
			fl = 0;
		}
		bl_now = bl_now -> next;
	}
	return ar;
}

void first_pipe(int **fd, pid_t **mas_of_pids, int *Q, char ****mas_of_args)
{
	int pid;
	pid = fork();
	if (pid == 0)
	{
		close((*fd)[0]);
		dup2((*fd)[1], 1);
		close((*fd)[1]);
		execvp(*(*mas_of_args)[0], (*mas_of_args)[0]);
		perror("exec error");
		exit(1);
	}
	if (pid < 0)
		perror("fork error");
	if (pid > 0)
	{
		(*Q)  = dup((*fd)[0]);
		close((*fd)[1]);
		close((*fd)[0]);
		(*mas_of_pids)[0] = pid;
	}
}

void last_pipe(pid_t **mas_of_pids, int *Q, char ****mas_of_args, int conv_col)
{
	int pid;
	pid = fork();
	if (pid == 0)
	{
		dup2((*Q), 0);
		close((*Q));
		execvp(*(*mas_of_args)[conv_col], (*mas_of_args)[conv_col]);
		perror("exec error");
		exit(1);
	}
	if (pid < 0)
		perror("fork error");
	if (pid > 0)
	{
		close((*Q));
		(*mas_of_pids)[conv_col] = pid;
	}

}

void change_output(int **fd)
{
	dup2((*fd)[1], 1);
	close((*fd)[1]);
	close((*fd)[0]);
}

void change_input(int *Q)
{
	dup2((*Q), 0);
	close((*Q));
}

void start_conv(char ***mas_of_args, int conv_col, enum bg_work bg_flag)
{
	int *buf, *fd;
	int i = 0, p = 0, Q;
	pid_t pid = 0;
	pid_t *mas_of_pids;
	buf = malloc(2*sizeof(int));
	buf[0] = dup(0); buf[1] = dup(1);
	fd = malloc(2*sizeof(int));
	mas_of_pids = malloc((conv_col+1)*sizeof(int));
	pipe(fd);
	first_pipe(&fd, &mas_of_pids, &Q, &mas_of_args);
	p = 1;
	for (i = 1; i<conv_col; i++)
	{
		pipe(fd);
		pid = fork();
		if (pid == 0)
		{
			change_input(&Q);
			change_output(&fd);
			execvp(*mas_of_args[i], mas_of_args[i]);
			perror("exec error");
			exit(1);
		}
		if (pid < 0)	
			perror("fork error");
		if (pid > 0)
		{
			close(Q);
			Q  = dup(fd[0]);
			close(fd[1]);
			close(fd[0]);
			mas_of_pids[p] = pid;
			p++;
		}
	}
	last_pipe(&mas_of_pids, &Q, &mas_of_args, conv_col);
	dup2(buf[0], 0);
       	dup2(buf[1], 1);
	if (bg_flag == norm_process)
	{
		i = conv_col;
		while (i >= 0)
		{
			pid = wait(NULL);
			for (p=0; p <= conv_col; p++)
			{
				if (pid == mas_of_pids[p])
					i --;
			}
		}
	}
}

void do_command(struct block ***bl_now,struct block ***first,enum kov_work **fl)
{
	char **args;
	char ***mas_of_args;
	char *input_file, *output_file;
	enum bg_work bg_fl;
	enum add_work file_flag, file_flag2;
	enum close_work close_flag;
	struct file_block file_block1;
	int conv_col;
	NewBlock(*bl_now);


	file_flag = file_test(**first, &input_file, &output_file);

	if (file_flag != add_error)
		file_block1=work_with_files(&file_flag2,file_flag,\
							input_file,output_file);
	
	(bg_fl) = bg_test(**first);

	conv_col = conv_test(first);


	if (((bg_fl)==bg_error)|(file_flag==add_error)|(file_flag2==add_error))
	{
		if ((bg_fl) == bg_error)
			printf("Error with &\n");
		if (file_flag == add_error)
			printf("Error with file syntax\n");
		if (file_flag2 == add_error)
			printf("Error with file open/close\n");
	}
	else {
		if (conv_col != 0)
		{
			mas_of_args = make_mas(first, conv_col);
			start_conv(mas_of_args, conv_col, bg_fl);
		} else { 
			args = argsmas(**first);
			start(args, bg_fl);
		}
		
	}
	if ((file_flag !=  add_error)&&(file_flag2 !=  add_error))
	{	
		close_flag = files_close(file_block1);
		if (close_flag == close_error)
			printf("Error with file closing\n");
	}
	NewSystem(*first);
	(**bl_now) = (**first);
	printf(">");
	(**fl) = kov_close;
}


void Analyze(char c,struct block**bl_now,struct block **first,enum kov_work *fl)
{
	if (c == '"')
	{
		if ((*fl) == kov_open)
			(*fl) = kov_close;
		if ((*fl) == kov_close)
			(*fl) = kov_open;
	}

	if (((c == '&')|(c == '<')|(c == '>')|(c == '|'))&&((*fl) == kov_close))
			{
				AddSysSymb(c, &bl_now);
			}
	if (((*fl) == kov_close) && (c != '"'))
	{
		if (c == ' ')
			NewBlock(bl_now);
		if (c == '\n')
		{
			while (wait4(-1, NULL, WNOHANG, NULL) > 0)
				{}
			do_command(&bl_now, &first, &fl);
		}
		if ((c!=' ')&&(c!='\n')&&(c!='&')&&(c!='>')&&(c!='<')&&(c!='|'))
			AddSymb(c, *bl_now);
	}	
	if ((*fl) == kov_open)		/* fl=1  -> kov is open */
	{
		if (c == '\n')
		{
			q_error(&bl_now, &first, &fl);
		} else
		{
			if (c != '"')
				AddSymb(c, *bl_now);
		}
	}
}
		
int main()
{
	struct block *first, *bl_now;
	char c; 
	enum kov_work fl;
	fl = kov_close;
	bl_now = malloc(sizeof(struct block));
	first = bl_now;
	bl_now -> string = NULL;
	bl_now -> lenght = 0;
	printf("Welcome!\n>");
	while ((c = getchar()) != EOF)
	{
		Analyze (c, &bl_now, &first, &fl);
	}
	NewBlock(&bl_now);
	NewSystem(&first);
	if ((fl) == kov_open)
	{
		printf("Not correct string.\n");
	}
	printf("\n Thanks for using! \n");
	return 0;
}
