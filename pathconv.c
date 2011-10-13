//MIT Licensed

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WINDOWS
	#include <Windows.h>
#else
	#include <unistd.h>
#endif

#include "safealloc.h"

#define CANNOT_GUESS_PATH  1

typedef enum { false,true } bool;
typedef unsigned int uint;

char cCurrentPath[FILENAME_MAX];

int strcntc(char *str, char chr){
	int total=0;
	for(;*str;++str)total += (*str == chr);
	return total;
}

//NOTE: This overlaps matches
int strcnts(char *str, char *str2){
	int total=0;
	size_t len = strlen(str2);
	for(;*(str+len);++str)total += !strncmp(str,str2,len);
	return total;
}

#define UNIX_TYPE 1
bool isUnixPath(char *pathStr){
	return pathStr[0] == '/';
}

int fromUnixPath(char *pathStr, char ***path, bool abs){
	uint pieces,cpiece=0;
	char *wholePathStr, *tk;
	bool parent = false;
	
	//Fix relative path
	if(abs && pathStr[0] != '/'){
		wholePathStr = malloc((strlen(cCurrentPath)+1+strlen(pathStr)+1) * sizeof(char));
		strcpy(wholePathStr,cCurrentPath);
		strcat(wholePathStr,"/");
		strcat(wholePathStr,pathStr);
	} else {
		wholePathStr = malloc((strlen(pathStr)+1) * sizeof(char));
		strcpy(wholePathStr,pathStr);
	}
	
	pieces = strcntc(wholePathStr,'/')
	- strcnts(wholePathStr,"/../") * 2
	- strcnts(wholePathStr,"/./")
	- strcnts(wholePathStr,"//");
	*path = (char**)safe_malloc(pieces * sizeof(char*));
	
	for(tk = strtok(wholePathStr,"/");
	tk != NULL; tk = strtok(NULL, "/")){
		//Ignore blanks and current dirs
		if(!strcmp(tk,"") || !strcmp(tk,"."))continue;
		//Step back on parent dirs
		if(!strcmp(tk,"..")){
			--cpiece;
			parent = true;
		} else {
			//Create new entity
			if(parent){
				(*path)[cpiece] = (char*)safe_realloc(*path[cpiece],(strlen(tk)+1) * sizeof(char));
				parent = false;
			} else {
				(*path)[cpiece] = (char*)safe_malloc((strlen(tk)+1) * sizeof(char));
			}
			strcpy((*path)[cpiece++],tk);
		}
	}
	
	free(wholePathStr);
	return 0;
}

#define URL_TYPE 2
bool isURL(char *pathStr){
	return !strncmp(pathStr,"file://",7);
}

int toURL(char **path, char **pathStr){
	char *piece;
	uint i;
	size_t pieces = safe_length(path,sizeof(char*)), len = 7, app;
	for(i=0;i<pieces;++i){
		len += 1 + strlen(path[i]);
	}
	*pathStr = (char*)safe_malloc(((len*3)+1) * sizeof(char));
	strcpy(*pathStr,"file://");
	app = strlen(*pathStr);
	for(i=0;i<pieces;++i){
		(*pathStr)[app++] = '/';
		for(piece=path[i]; *piece; ++piece){
			if((*piece >= 'A' && *piece <= 'Z')
			|| (*piece >= 'a' && *piece <= 'z')
			|| (*piece >= '0' && *piece <= '9')
			|| *piece == '-' || *piece == '_'
			|| *piece == '.' || *piece == '~'){
				(*pathStr)[app++] = *piece;
			} else {
				sprintf((*pathStr)+app,"%%%02x",*piece);
				app += 3;
			}
		}
		(*pathStr)[app] = '\0';
	}
	return 0;
}

#define DOS_TYPE 3

static bool needsAbsolute[] = {
	false, //placeholder
	false, //UNIX_TYPE
	true,  //URL_TYPE
	false, //DOS_TYPE
};

int defaultOut(char **path, char **pathStr){
	int pieces = safe_length(path,sizeof(char*))-1,i;
	for(i=0;i<pieces;++i){
		printf("%s\n",path[i]);
	}
	printf("%s",path[i]); //No trailing newline
	return 0;
}

int main(int argc, char **argv){
	uint i,intype=0,outtype=0;
	bool help = false;
	char **path, *pathstr = NULL, *outpath = NULL;
	int (*infunc)(char*,char***,bool);
	int (*outfunc)(char**,char**);
	
	for(i=1;i<argc;++i){
		if(!strncmp(argv[i],"--",2)){
			char* flag = argv[i]+2;
			if(!strcmp(flag,"help"))help = true;
			else if(!strcmp(flag,"to-url"))outtype = URL_TYPE;
			else if(!strcmp(flag,"from-unix"))intype = UNIX_TYPE;
		} else if(argv[i][0] == '-'){
			uint j,len = strlen(argv[i]);
			for(j=1;j<len;++j){
				switch(argv[i][j]){
					case 'h': case '?': help = true; break;
					case 'l': intype = UNIX_TYPE; break;
					case 'U': outtype = URL_TYPE; break;
				}
			}
		} else if(pathstr == NULL){
			pathstr = argv[i];
		}
	}
	
	if(help || pathstr == NULL){
		printf("Path Conversion Utility (C) Wa <logicplace.com>\n"
			"If no from is given it will guess. Can only guess if it's an absolute path.\n"
			"Relative paths will be resolved to an absolute path as necessary (the cwd).\n"
			"%s options path\n"
			"Options:\n"
			"  -h -?  --help    Show this help and exit.\n"
			"  -l  --from-unix  Treat given path as unix-style.\n"
			//"  -L --to-unix  ...\n"
			//"  -u  --from-url   Treat given path as a file:// url.\n"
			"  -U  --to-url     Output path as a file:// url.\n"
		,argv[0]);
		return 0;
	}
	
	#ifdef WINDOWS
		GetCurrentDirectory(sizeof(cCurrentPath), cCurrentPath)
	#else
		getcwd(cCurrentPath, sizeof(cCurrentPath));
	#endif
	
	if(intype == 0){
		if(isURL(pathstr))intype = URL_TYPE;
		else if(isUnixPath(pathstr))intype = UNIX_TYPE;
		else return CANNOT_GUESS_PATH;
	}
	
	if(intype == outtype){
		printf(pathstr);
	} else {
		switch(intype){
			case UNIX_TYPE: infunc = fromUnixPath; break;
		}
		infunc(pathstr,&path,needsAbsolute[outtype]);

		switch(outtype){
			case URL_TYPE: outfunc = toURL; break;
			default: outfunc = defaultOut; break;
		}
		outfunc(path,&outpath);
	
		if(outpath != NULL){
			printf("%s",outpath);
		}
	}
	
	safe_freeall();
	return 0;
}
