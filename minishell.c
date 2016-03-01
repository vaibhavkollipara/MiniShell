#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<signal.h>
#include<fcntl.h>

struct command {
	char line[100];
	char *arguments[50];
	int iflag;
	int oflag;
};
struct command cmds[20];
int noOfCommands;
char cmd[100];
char ofile[20] = "no_file"; //output file name should be less tha 20 chars
char ifile[20] = "no_file"; //input filename should be less than 20 chars.
int in = -1;
int out = -1;
pid_t pid;
int cidStack[50];
int stackTop;
int bg;

void push(int cid);
int pop();
void argsbreak(int a);
void breakWithAmp();
void argsBreakWithGreaterThan(int i);
void argsBreakWithLessThan(int i);
void commandsbreak();
void pipeExecution();
void execute();
void sig_handler(int signo);
void tokenize();
void minish();


//stack operations
void push(int cid){
	if(stackTop+1<50){
	stackTop++;
	cidStack[stackTop]=cid;
	}else{
	//	printf("Child Process Stack Full !");
	}
}

int pop(){
	int x;
	if(stackTop>=0){
		x = cidStack[stackTop];
		stackTop--;
		return x;
	}
	else{
		//printf("No Child Processes");
		return -1;
	}
}

void argsbreak(int a) {

	char *arg;
	int i = -1;
	int j;
	char *line = (char *) malloc(1 + strlen(cmds[a].line));
	strcpy(line, cmds[a].line);
	arg = strtok(line, " ");
	while (arg != NULL) {
		cmds[a].arguments[++i] = (char *) malloc(1 + strlen(arg));
		strcpy(cmds[a].arguments[i], arg);
		arg = strtok(NULL, " ");
	}
	cmds[a].arguments[++i] = NULL;

}

void breakWithAmp(){
	char *token;
	int len = (int)strlen(cmd);
	token = strtok(cmd, "&");
	strcpy(cmd, token);
	if(len!=strlen(cmd))
		bg=1;
}


void argsBreakWithGreaterThan(int i) {
	//printf("check >: %s\n",cmds[i].line);
	char *token;
	token = strtok(cmds[i].line, ">");
	strcpy(cmds[i].line, token);
	if (token != NULL) {
		token = strtok(NULL, ">");
		if (token != NULL)
			strcpy(ofile, token);
	}
	if (!strcmp(ofile, "no_file") == 0)
		cmds[i].oflag = 1;
}

void argsBreakWithLessThan(int i) {
	//printf("check < : %s\n",cmds[i].line);
	char *token;
	token = strtok(cmds[i].line, "<");
	strcpy(cmds[i].line, token);
	if (token != NULL) {
		token = strtok(NULL, "<");
		if (token != NULL)
			strcpy(ifile, token);
	}
	if (!strcmp(ifile, "no_file") == 0)
		cmds[i].iflag = 1;
}

void commandsbreak() {
	char *arg;
	noOfCommands = -1;
	int j;
	char *line = (char *) malloc(1 + strlen(cmd));
	strcpy(ifile, "no_file");
	strcpy(ofile, "no_file");
	strcpy(line, cmd);
	arg = strtok(line, "|");
	while (arg != NULL) {
		strcpy(cmds[++noOfCommands].line, arg);
		arg = strtok(NULL, "|");
	}
	for (j = 0; j <= noOfCommands; j++) {
		//printf("cmd %d : %s\n",j,cmds[j].line);
		cmds[j].iflag = cmds[j].oflag = 0;
		argsBreakWithGreaterThan(j);
		argsBreakWithLessThan(j);
		argsbreak(j);
	}
}

void pipeExecution() {
	int fd_in = 0;
	int i = 0;
	int p[2];
	int a, b;
	int status;
	close(in);
	close(out);
	if (!strcmp(ifile, "no_file") == 0) {
		//trim spaces from input filenames
		for (a = 0; a < strlen(ifile); a++) {
			if (ifile[a] == ' ') {
				for (b = a; b < strlen(ifile) - 1; b++)
					ifile[b] = ifile[b + 1];
				ifile[b] = '\0';
			}
		}
		in = open(ifile, O_RDONLY);
	}

	if (!strcmp(ofile, "no_file") == 0) {
		//trim spaces from output filenames
		for (a = 0; a < strlen(ofile); a++) {
			if (ofile[a] == ' ') {
				for (b = a; b < strlen(ofile) - 1; b++)
					ofile[b] = ofile[b + 1];
				ofile[b] = '\0';
			}
		}
		out = open(ofile, O_WRONLY | O_TRUNC | O_CREAT,
				S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);

	}
	while (i <= noOfCommands) {

		pipe(p);
		if ((pid = fork()) == -1) {
			exit(EXIT_FAILURE);
		}
		if (pid == 0) { //child
			// printf("\nChild...\n");
			if (cmds[i].iflag == 1) {
				//printf("in : %d\n", in);
				dup2(in, 0); //ipf from file
			} else {
				dup2(fd_in, 0);
			}

			if (i < noOfCommands)
				dup2(p[1], 1);
			if (cmds[i].oflag == 1) {
				//printf("out : %d\n", out);
				dup2(out, 1); //o/p file redirection
			}
			close(p[0]);
			execvp(cmds[i].arguments[0], cmds[i].arguments);
			printf("Invalid Command !\n");
			exit(EXIT_FAILURE);
		} else { //parent
			if(i==noOfCommands && bg==1){// if background process
			push(pid);
			 printf("[%d] %d\n",stackTop,pid);
			}else{ // if not background process
			 wait(pid);
			}

			//printf("\nParent...\n");
			close(p[1]);
			// waitid(pid,&status,0);
			//if(status==0){
			fd_in = p[0]; //save the input for the next command
			if (i == noOfCommands)
				close(fd_in);
			i++;
			// }
		}
	}

}

void execute() {
	if (strcmp(cmds[0].arguments[0], "cd") == 0) {
		chdir(cmds[0].arguments[1]);
	} else if (strcmp(cmds[0].arguments[0], "exit") == 0) {
		//printf("group id : %d\n",getgrp());
		int cid=pop();
		while(cid!=-1){
			if(cid!=0){
			   if(cid>=0)
				kill(cid, SIGTERM);//background process
			   else
				kill(cid*(-1),SIGTERM);//suspended process

			}
			cid=pop();
		}
		exit(0);
	}else if (strcmp(cmds[0].arguments[0], "jobs") == 0) {
		int cid;
		for(cid=0;cid<=stackTop;cid++){
			if(cidStack[cid]!=0){
			if(cidStack[cid]>0)
			  printf("[%d] %d Background\n",cid,cidStack[cid]);
			else
			  printf("[%d] %d Suspended\n",cid,cidStack[cid]*-1);

			  }
		}
	}else if(strcmp(cmds[0].arguments[0], "fg") == 0){
			int cid;
		/* Put the job into the foreground.  */
		cid = cidStack[atoi(cmds[0].arguments[1])];
		cidStack[atoi(cmds[0].arguments[1])]=0;
		if(cid!=0){
			if(cid>0){//background process
			printf("starting %d\n",cid);
				cid = getpgid(cid);
				tcsetpgrp (0, cid);
		  		wait(cid);
			}else{//suspended process
				printf("starting %d\n",cid*(-1));
				cid = getpgid(cid*(-1));
				kill(cid,SIGCONT);
			}


		}

//////
	}else {
		pipeExecution();
	}
}

void sig_handler(int signo){
	int cid,i;

  switch(signo){
	case SIGINT : printf("\nPressed Ctrl + C\n");
		      printf("Terminated %d\n",pid);
		      kill(pid, SIGTERM);
		      break;
	case SIGTSTP : printf("\nPressed Ctrl + Z\n");
		       kill(pid, SIGSTOP);
		       push(pid*(-1));
		       printf("[%d] %d\n",stackTop,pid);
		       minish();
		       break;
	default :printf("Unrecognized signal");
  }
}

void tokenize(){
	breakWithAmp();
	commandsbreak();
}

void minish() {
	printf("VaibhavShell> ");
	gets(cmd);
	bg=0;
	tokenize();
	execute();
	minish();
}

int main() {
	if (signal(SIGINT, sig_handler) == SIG_ERR)
  printf("\ncan't catch SIGINT\n");
	if (signal(SIGTSTP, sig_handler) == SIG_ERR)
  printf("\ncan't catch SIGTSTP\n");
	stackTop=-1;
	minish();
	return 0;
}
