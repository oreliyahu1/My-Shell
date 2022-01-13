#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

//HW1 - Operating Systems
//Arik skigin 
//Or Eliyahu 

///////      Setting     /////////
#define UserInput_CMDARGS_SIZE 30+1
	//CMD + 3 ARGS
#define UserInput_MAXARG 4
#define BG_MANGAER_PRINT_SIZE 10
#define FOREGROUND_DIR "/bin/"
#define Mysh_prompt "mysh**"
#define Mysh_bye "bye"
#define Mysh_bgjobs "bgjobs"
#define Mysh_kill "kill"
#define Mysh_mysh "mysh"
/////////////////////////////////


enum BOOL {TRUE = 1, FALSE = 0};
//ID of Internal functions
enum INTRLFUNC {INTRL_BYE = 1, INTRL_BGJOBS = 2, INTRL_KILL = 3, INTRL_MYSH = 4, INTRL_NOTITRL = -1};
//ID of errors program
enum ERRORS_MNG {INTRLERR_TOOMANY = 1, INTRLERR_PID_NOTRUN = 2, INTRLERR_FME_READ = 3, ITRLERR_UNEXCEPARG = 4,
ITRLERR_NOARGS = 6 ,BASHUNIX_UNEXCPTERR = 5, INTRLERR_FME_CLOSE = 7, INTRLERR_FME_LINEOVERFLOW = 8, INTRLERR_PID_NOTFOUND = 9, MYSHLL_NOARGS = 10,
INTRLERR_FME_RUNERR = 11, INTRL_NOBG = 12, BGJ_MEMALLOCERR = 13};

//background PID node
typedef struct prc_node{
	char bg[UserInput_CMDARGS_SIZE];
	int pid;
	enum BOOL isKilled;
	struct prc_node* next;
} prc_node;

//background PID manager
typedef struct BG_MNG {
	prc_node *head;
	int size;
} BG_MNG;

//BG PID MANAGER FUNCTIONS
prc_node* createProcessBGstack(int pid, char *bg, enum BOOL isKilled); //create new process node
enum ERRORS_MNG BGMNG_addPID(BG_MNG *bgj, int pid, char *bg, enum BOOL isKilled); //add pid to bg manager
void BGMNG_freeANDkillALL(BG_MNG *bgj); //kill all pid in bgjobs
void BGMNG_print(BG_MNG bgj); //print all bgjobs
enum ERRORS_MNG BGMNG_killPID(BG_MNG *bgj, int pid); //kill pid of bgjobs

int stringTOusrARGV(char *str, char usr_argv[][UserInput_CMDARGS_SIZE]);//get string from user and make string arr to arguments
enum ERRORS_MNG cmd_MNG(BG_MNG *bgj, char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc); //sets the action before running
enum BOOL ERROR_PRINT_MAGNER(enum ERRORS_MNG err, char* fname); //print error
enum BOOL isBG(char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc); //if need to run as BG
enum INTRLFUNC idInternalFunc(char *cmd); //if command is internal function

enum ERRORS_MNG processRUN(BG_MNG *bgj, enum BOOL BG, char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc); //run BG/FG command
enum ERRORS_MNG internalFuncManager(BG_MNG *bgj, enum INTRLFUNC func, char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc); //internal functions running
void execve_BASHUNIXRUN(char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc); //run arguments on unix bash

//MYSH Internal functions
//void BGMNG_freeANDkillALL(BG_MNG *bgj);					//bye - without exit
//void BGMNG_print(BG_MNG bgj);								//bgjobs
//enum ERRORS_MNG BGMNG_killPID(BG_MNG *bgj, int pid);		//kill
enum ERRORS_MNG MYSH_mysh(BG_MNG *bgj, char* filename);		//mysh


int main(int argc, char* argv[]){
	BG_MNG bgj;//BG MANAGER
	char USR_ARGV[UserInput_MAXARG][UserInput_CMDARGS_SIZE], usrtmpstr[UserInput_MAXARG*UserInput_CMDARGS_SIZE];
	bgj.size=0;
	bgj.head=NULL;
	if(argc > 1){
		ERROR_PRINT_MAGNER(MYSHLL_NOARGS, Mysh_prompt);
		exit(EXIT_FAILURE);
	}
	//dont worry internalFuncManager management all
	while(TRUE){
		printf("%s ", Mysh_prompt);
		fgets(usrtmpstr, UserInput_MAXARG*UserInput_CMDARGS_SIZE, stdin);
		usrtmpstr[strlen(usrtmpstr)-1]='\0'; //'\n' to null
		cmd_MNG(&bgj, &USR_ARGV[0], stringTOusrARGV(usrtmpstr, &USR_ARGV[0]));
	}

	return 0;
}

//stringTOusrARGV - Fill the arguments in array(usr_argv)
//get (char *str) - potential command
//get (char usr_argv[][UserInput_CMDARGS_SIZE]) - string arr to arguments
//return (int) size of arguments
int stringTOusrARGV(char *str, char usr_argv[][UserInput_CMDARGS_SIZE]){
	char *tcpy;
	int i;
	//split string to string arr
	for(i=0; i<UserInput_MAXARG; i++){
		tcpy = strchr(str,' '); //find first cmd/arg
		if(tcpy)
			*tcpy = '\0';	
		if(strlen(str)){
			strcpy(usr_argv[i],str);
			if(!tcpy)
				return i+1;
		}
		else
			return i;
		str = tcpy+1;
	}
	return i;
}

//cmd_MNG - run command and print error(if have)
//get (BG_MNG *bgj) - background manager
//get (char usr_argv[][UserInput_CMDARGS_SIZE]) - string arr to arguments
//get (int usr_argc) - size of arguments
//return (enum ERRORS_MNG) error that printed
enum ERRORS_MNG cmd_MNG(BG_MNG *bgj, char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc){
	enum ERRORS_MNG err = 0;
	enum BOOL bgRUN = FALSE;

	if(usr_argc){
		bgRUN=isBG(&usr_argv[0], usr_argc); //check if BG & remove & on last arg
		if(idInternalFunc(usr_argv[0]) == INTRL_NOTITRL)
			err = processRUN(bgj, bgRUN, &usr_argv[0], usr_argc);
		else if (!bgRUN)
			err = internalFuncManager(bgj, idInternalFunc(usr_argv[0]), &usr_argv[0], usr_argc);
		else
			err = INTRL_NOBG;  //internal cannot run as BG!!!
	}
	if(err) if(ERROR_PRINT_MAGNER(err, usr_argv[0])) internalFuncManager(bgj, INTRL_BYE, NULL, 1);
	return err;
}

//isBG - check if need to be BG
//get (char usr_argv[][UserInput_CMDARGS_SIZE]) - string arr to arguments
//get (int usr_argc) - size of arguments
//return (enum BOOL) - true if that BG
enum BOOL isBG(char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc){
	if(usr_argc == 0)
		return FALSE;
	if(usr_argv[usr_argc-1][strlen(usr_argv[usr_argc-1])-1] == '&'){
		usr_argv[usr_argc-1][strlen(usr_argv[usr_argc-1])-1]='\0';
		return TRUE;
	}
	return FALSE;
}

//idInternalFunc - Internal command ID
//get (char *cmd) - command
//return (enum INTRLFUNC) - the cmd enum of internal mysh
enum INTRLFUNC idInternalFunc(char *cmd){
	if(!strcmp(cmd,Mysh_bye))
		return INTRL_BYE;
	else if (!strcmp(cmd,Mysh_bgjobs))
		return INTRL_BGJOBS;
	else if (!strcmp(cmd,Mysh_kill))
		return INTRL_KILL;
	else if (!strcmp(cmd,Mysh_mysh))
		return INTRL_MYSH;
	return INTRL_NOTITRL;
}

//processRUN - check if need to be BG
//get (BG_MNG *bgj) - background manager
//get (enum BOOL BG) - if need BG run
//get (char usr_argv[][UserInput_CMDARGS_SIZE]) - string arr to arguments
//get (int usr_argc) - size of arguments
//return (enum ERRORS_MNG) - false(no problem) / some error
enum ERRORS_MNG processRUN(BG_MNG *bgj, enum BOOL BG, char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc){
	int pid = fork();

	if((pid) && (!BG)) waitpid(pid, NULL, 0); //if FR need wait to child

	if(pid == 0){
		execve_BASHUNIXRUN(&usr_argv[0],usr_argc); //child run
		ERROR_PRINT_MAGNER(BASHUNIX_UNEXCPTERR, usr_argv[0]); //if error in exec is return to here(return if error to load program)
		exit(EXIT_FAILURE);	//bye child
	}else if (BG)
		return BGMNG_addPID(bgj, pid, usr_argv[0], FALSE); //add to BG MANAGER

	return FALSE;
}

//execve_BASHUNIXRUN - get cmd/args arr and run bash unix
//get (char usr_argv[][UserInput_CMDARGS_SIZE]) - string arr to arguments
//get (int usr_argc) - size of arguments
//return (void)
void execve_BASHUNIXRUN(char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc){
	char* args[usr_argc + 1];	//char* to execv
	char cmd[strlen(FOREGROUND_DIR) + strlen(usr_argv[0]) + 1];
	int i;

	strcpy(cmd, FOREGROUND_DIR);
	strcpy((cmd + strlen(FOREGROUND_DIR)),usr_argv[0]);	//make bin app string
	for(i=0; i<usr_argc; i++)
		args[i] = usr_argv[i];
	args[i] = (char*)0;

	execv(cmd, args);
}

//BGMNG_addPID - add the new bg job to bg manager
//get (BG_MNG *bgj) - background manager
//get (int pid) - pid number
//get (char *bg) - bg name
//get (enum BOOL isKilled) - flag killed
//return (enum ERRORS_MNG) - if have error at memory allocation
enum ERRORS_MNG BGMNG_addPID(BG_MNG *bgj, int pid, char *bg, enum BOOL isKilled){
	prc_node* prn = createProcessBGstack(pid, bg, isKilled);
	if(prn){
		bgj->size++;
		prn->next = bgj->head;
		bgj->head = prn;
		return FALSE;
	}
	return BGJ_MEMALLOCERR;
}

//createProcessBGstack - create new process node
//get (int pid) - background pid number
//get (char *bg) - background name
//get (enum BOOL isKilled) - flag killed
//return (prc_node*) - pointer of prc_node node
prc_node* createProcessBGstack(int pid, char *bg, enum BOOL isKilled){
	prc_node* prn = (prc_node*)malloc(sizeof(prc_node));
	if(prn){
		prn->pid = pid;
		strcpy(prn->bg, bg);
		prn->isKilled = isKilled;
		prn->next=NULL;
	}
	return prn;
}

//BGMNG_freeANDkillALL - kill all bg jobs that runs and free nodes
//return (void)
void BGMNG_freeANDkillALL(BG_MNG *bgj){
	prc_node* tmp;
	while(bgj->size){
		tmp = bgj->head;
		if(!tmp->isKilled){	//have run flag
			tmp->isKilled = TRUE;
			kill(tmp->pid, SIGKILL);
		}
		bgj->head = bgj->head->next;
		free(tmp);
		bgj->size--;
	}
	bgj->size=0;
	bgj->head=NULL;
}

//BGMNG_addPID - print all bg jobs
//return (void)
void BGMNG_print(BG_MNG bgj){
	int printCount = 0; //print last 10 bg process
	if(bgj.size){
		printf("pid	bgjobs\n");
		while((bgj.head) && (printCount < BG_MANGAER_PRINT_SIZE)){
			printf("%d	%s\n", bgj.head->pid, bgj.head->bg);
			bgj.head = bgj.head->next;
			printCount++;
		}
	}else
		printf("empty background jobs\n");
}

//BGMNG_addPID - kill pid in bgjobs
//get (BG_MNG *bgj) - background manager
//get (int pid) - pid number
//return (enum ERRORS_MNG) - kill exception
enum ERRORS_MNG BGMNG_killPID(BG_MNG *bgj, int pid){
	prc_node* prn = bgj->head;
	while(prn){
		if(prn->pid == pid){ //found pid
			if(!prn->isKilled){ //we kill only running pid
				prn->isKilled = TRUE;				
				kill(pid, SIGKILL);
				return FALSE;
			}else //cannot kill killed bg job
				return INTRLERR_PID_NOTRUN;
		}
		prn = prn->next;
	}
	return INTRLERR_PID_NOTFOUND;
}

//internalFuncManager - Manage and run internal commands
//get (BG_MNG *bgj) - background manager
//get (enum INTRLFUNC func) - id of internal cmd
//get (char usr_argv[][UserInput_CMDARGS_SIZE]) - string arr to arguments
//get (int usr_argc) - size of arguments
//return (enum ERRORS) - false(no problem) / some error
enum ERRORS_MNG internalFuncManager(BG_MNG *bgj, enum INTRLFUNC func, char usr_argv[][UserInput_CMDARGS_SIZE], int usr_argc){
	switch(func){
		case INTRL_BYE:
			if(usr_argc == 1){
				BGMNG_freeANDkillALL(bgj);
				exit(EXIT_SUCCESS);//bye cmd => need exit
			}
			else
				return INTRLERR_TOOMANY;
			break;
		case INTRL_BGJOBS:
			if(usr_argc == 1)
				BGMNG_print(*bgj);
			else
				return INTRLERR_TOOMANY;
			break;
		case INTRL_KILL:
			if(usr_argc == 2){
				if(atoi(usr_argv[1])==0)
					return ITRLERR_UNEXCEPARG;
				return BGMNG_killPID(bgj,atoi(usr_argv[1]));
			}else if(usr_argc != 1)
				return INTRLERR_TOOMANY;
			else
				return ITRLERR_NOARGS;
			break;
		case INTRL_MYSH:
			if(usr_argc == 2)
				return MYSH_mysh(bgj, usr_argv[1]);
			else if(usr_argc != 1)
				return INTRLERR_TOOMANY;
			else
				return ITRLERR_NOARGS;
			break;
		default:
			return FALSE;
			break;
	}
	return FALSE;
}

//ERROR_PRINT_MAGNER - print the function name and the error of running
//get (enum ERRORS_MNG) - error flag
//get (char* fname) - function name
//return (enum BOOL) - is fatal error
enum BOOL ERROR_PRINT_MAGNER(enum ERRORS_MNG err, char* fname){
	printf("Function \"%s\" ERROR:\n");
	switch(err){
		case INTRLERR_TOOMANY:
			printf("--too many arguments\n");
			break;
		case ITRLERR_NOARGS:
			printf("--the command need get argument\\s\n");
			break;
		case INTRLERR_PID_NOTFOUND:
			printf("--not found pid in bgjobs\n");
			break;
		case INTRLERR_PID_NOTRUN:
			printf("--pid is off process\n");
			break;
		case INTRLERR_FME_READ:
			printf("--cannot open-read file\n");
			break;
		case INTRLERR_FME_CLOSE:
			printf("--cannot close the file\n");
			break;
		case INTRLERR_FME_LINEOVERFLOW:
			printf("--read line overflow\n");
			break;
		case ITRLERR_UNEXCEPARG:
			printf("--cannot read pid number\n");
			break;
		case INTRLERR_FME_RUNERR:
			printf("--running command exception\n");
			break;
		case BASHUNIX_UNEXCPTERR:
			printf("--cannot run the function from bin folder\n");
			break;
		case MYSHLL_NOARGS:
			printf("-- PROGRAM DONT SUPPORT ARGUMENTS\n");
			break;
		case INTRL_NOBG:
			printf("-- Internal function cannot run as background process\n");
			break;
		case BGJ_MEMALLOCERR:
			printf("-- cannot create dynamic memory allocation\n");
			return TRUE;
			break;
		default:
			break;
	}
	return FALSE;
}

//MYSH_mysh - mysh internal file cmd
//get (BG_MNG *bgj) - background manager
//get (char* filename) - file to reads&runs commands
//return (enum ERRORS_MNG) - false(no problem) / some error
enum ERRORS_MNG MYSH_mysh(BG_MNG *bgj, char* filename){
	int runFILE, retin, indexfile = 0;
	char buffer[1], line[UserInput_MAXARG*UserInput_CMDARGS_SIZE], USR_ARGV[UserInput_MAXARG][UserInput_CMDARGS_SIZE];
	
	if((runFILE = open(filename, O_RDONLY)) == -1) //check read success
		return INTRLERR_FME_READ;

	do{
		if(indexfile == (UserInput_MAXARG*UserInput_CMDARGS_SIZE)){ //line overflow
			close(runFILE);
			return INTRLERR_FME_LINEOVERFLOW;
		}

		retin = read(runFILE, &buffer, 1);//read char
		if ((indexfile) && (((retin) && (buffer[0] == '\n')) || (!retin))){
			line[indexfile]='\0'; //end of string
			printf("%s\n",line); //print command to screen
			indexfile=0;
			if(cmd_MNG(bgj, &USR_ARGV[0], stringTOusrARGV(line, &USR_ARGV[0])))//if error at run command
				return INTRLERR_FME_RUNERR;
		}
		else
			line[indexfile++] = buffer[0];
	}while(retin);

	if(close(runFILE) == -1) //check close success
		return INTRLERR_FME_CLOSE;
	
	return FALSE;
}
