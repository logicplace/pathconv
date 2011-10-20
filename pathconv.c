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

#define ERROR_CANNOT_GUESS_PATH  1
#define ERROR_INVALID_CHAR       2

typedef enum { false,true } bool;
typedef unsigned int uint;

typedef struct _path {
	bool relative;
	char **entity;
	int entities;
} path;

/* Globals and helper functions for conversion */
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

/* Conversion functions */
#define UNIX_TYPE 1
bool isUnixPath(char *pathStr){
	return pathStr[0] == '/';
}

int fromUnixPath(char *pathStr, path *pathObj, bool abs){
	uint pieces,cpiece=0;
	char *wholePathStr, *tk;
	bool parent = false;
	bool first = true;
	
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
	pathObj->relative = (wholePathStr[0] != '/');
	
	pieces = strcntc(wholePathStr,'/')
	- strcnts(wholePathStr,"/../") * 2
	- strcnts(wholePathStr,"/./")
	- strcnts(wholePathStr,"//");
	pathObj->entity = (char**)safe_malloc(pieces * sizeof(char*));
	
	for(tk = strtok(wholePathStr,"/");
	tk != NULL; tk = strtok(NULL, "/")){
		//TODO: Deal with escapes
		//Ignore blanks and current dirs
		if(!strcmp(tk,"") || !strcmp(tk,"."))continue;
		//Step back on parent dirs
		if(!strcmp(tk,"..") && !(abs && first)){
			--cpiece;
			parent = true;
		} else {
			first = false;
			//Create new entity
			if(parent){
				pathObj->entity[cpiece] = (char*)safe_realloc(pathObj->entity[cpiece],(strlen(tk)+1) * sizeof(char));
				parent = false;
			} else {
				pathObj->entity[cpiece] = (char*)safe_malloc((strlen(tk)+1) * sizeof(char));
			}
			strcpy(pathObj->entity[cpiece++],tk);
		}
	}
	pathObj->entities = cpiece;
	
	free(wholePathStr);
	return 0;
}

int toUnixPath(path *pathObj, char **pathStr){
	char *piece;
	uint i; int err=0;
	size_t pieces = pathObj->entities, len = 0, app = 0;
	for(i=0;i<pieces;++i){
		len += 1 + strlen(pathObj->entity[i]);
	}
	if(pathObj->relative)--len;
	*pathStr = (char*)safe_malloc((len+1) * sizeof(char));
	if(!pathObj->relative){ (*pathStr)[app++] = '/'; }
	
	for(i=0;i<pieces;++i){
		if(app > 1)(*pathStr)[app++] = '/';
		for(piece=pathObj->entity[i]; *piece; ++piece){
			if(*piece == '/'){
				err = ERROR_INVALID_CHAR;
				break;
			}
			(*pathStr)[app++] = *piece;
		}
	}
	
	if(err)pathStr[0] = '\0';
	else{ (*pathStr)[app] = '\0'; }
	
	return err;
}

#define URL_TYPE 2
bool isURL(char *pathStr){
	return !strncmp(pathStr,"file://",7);
}

int toURL(path *pathObj, char **pathStr){
	char *piece;
	uint i;
	size_t pieces = pathObj->entities, len = 7, app;
	for(i=0;i<pieces;++i){
		len += 1 + strlen(pathObj->entity[i]);
	}
	*pathStr = (char*)safe_malloc(((len*3)+1) * sizeof(char));
	strcpy(*pathStr,"file://");
	app = strlen(*pathStr);
	for(i=0;i<pieces;++i){
		(*pathStr)[app++] = '/';
		for(piece=pathObj->entity[i]; *piece; ++piece){
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
bool isDosPath(char *pathStr){
	return !strncmp(pathStr+1,":\\",2);
}

/* End conversion functions */

static bool needsAbsolute[] = {
	false, //placeholder
	false, //UNIX_TYPE
	true,  //URL_TYPE
	false, //DOS_TYPE
};

int defaultOut(path *pathObj, char **pathStr){
	int pieces = pathObj->entities-1,i;
	for(i=0;i<pieces;++i){
		printf("%s\n",pathObj->entity[i]);
	}
	printf("%s",pathObj->entity[i]); //No trailing newline
	return 0;
}

int main(int argc, char **argv){
	uint i,intype=0,outtype=0;
	bool help = false;
	char *pathstr = NULL, *outpath = NULL;
	path pathObj;
	int (*infunc)(char*,path*,bool);
	int (*outfunc)(path*,char**);
	
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
		else return ERROR_CANNOT_GUESS_PATH;
	}
	
	if(intype == outtype){
		printf(pathstr);
	} else {
		switch(intype){
			case UNIX_TYPE: infunc = fromUnixPath; break;
		}
		infunc(pathstr,&pathObj,needsAbsolute[outtype]);

		switch(outtype){
			case URL_TYPE: outfunc = toURL; break;
			default: outfunc = defaultOut; break;
		}
		outfunc(&pathObj,&outpath);
	
		if(outpath != NULL){
			printf("%s",outpath);
		}
	}
	
	safe_freeall();
	return 0;
}
