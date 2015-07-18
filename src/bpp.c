#include <stdio.h>//Error reporting
#include <stdint.h>//Because uint8_t makes purpose more obvious than char in a memory block and makes for a handy boolean variable
#include <unistd.h>//System calls to gcc
#include <stdlib.h>//For malloc and realloc
#include <string.h>//For memcmp
#include <errno.h>//For errors with strtol

void createFileHead(FILE *outFile, uint64_t tapeLen);
void createFileBody(FILE *sourceFile, FILE *outFile);
void createFileFoot(FILE *outFile);




int main(int argc, char *argv[]){
	if(argc < 2 || (argv[1][0] == '-' && argv[1][1] != 'h')){//If you're not supplying a source file or asking for help
		printf("Invalid usage. Use %s -h for help.\n", argv[0]);
		return 1;
	}else if(!memcmp(argv[1], "-h", 3)){//Always checking to 3 for standalone options to ensure they're not part of a block
		printf("Usage: %s SOURCE/-h [-options]\n", argv[0]);
		puts("Options: -o NAME : Name of output file, must be isolated");
		puts("         -c : Keep translated C file");
		puts("         -h : Print this text (Must be only argument)");
		puts("         -m NUMBER : Length of tape /bytes (default: 30000), must be isolated");
		puts("         -g OPTIONS : Designate all arguments after this point as options for gcc");
		return 0;
	}
	//Variables defining the compiler's behaviour
	uint8_t keepC = 0;
	char *outName = malloc(1);//Malloc 1s just to put the values on the heap rather than the stack
	char *gccArgs = malloc(1);
	uint8_t defaultOutName = 1;
	uint64_t tapeLen = 30000;//Because it can be reassigned with strtol, it must be able to fit a long value. Can store values greater than available
				 //memory, so if you want to run brainf**k++ on a supercomputer, that's your problem, not mine



	//Parse command line arguments
	uint8_t reservedForGcc = 0;//So that when -g comes up, further commands are not parsed by this program. Also useful to see if additional gcc args are present
	for(int i = 2; i < argc; i++){//Find out which arguments are in use, starts at 2 because 0 is program call, and 1 is either help or the source file
		if(!reservedForGcc){
			if(argv[i][0] == '-'){//If we're looking at an option block
				if(!memcmp(argv[i], "-o", 3)){
					defaultOutName = 0;
					if(argv[i+1]){
						outName = realloc(outName, sizeof(argv[i+1]));
						outName = strcpy(outName, argv[i+1]);
						i++;//Skip over the next argument as it is a file name
					}
					else{
						puts("Error: Invalid file name");
						free(outName);
						free(gccArgs);
						return 1;
					}
				}
				else if(!memcmp(argv[i], "-m", 3)){
					char *storageChar;//For strto to store stuff
					tapeLen = strtol(argv[i+1], &storageChar, 10);
					if(!tapeLen || errno){
						puts("Error: Invalid tape length");
						free(outName);
						free(gccArgs);
						return 1;
					}
					i++;//Skip over the next argument as it is the memory size

				}else if (memchr(argv[i], 'c', 5)) keepC = 1;//Only checks 5 bytes as there are only 5 valid args and most shouldn't be together
				else if (!memcmp(argv[i], "-g", 3)){
					reservedForGcc = 1;//If you've called -g, the rest of the args must belong to gcc
					if(argv[i+1]){
						gccArgs = realloc(gccArgs, sizeof(argv[i+1]));
						i++;
					}
					else{
						puts("Error: No gcc arguments following -g option");
						free(outName);
						free(gccArgs);
						return 1;
					}
				}
			}

		}else{
			gccArgs = realloc(gccArgs, sizeof(gccArgs) + sizeof(argv[i]) + 1);//+1 for the space
			gccArgs = strcat(gccArgs, " ");//Put spaces between each arg so that gcc understands. Valgrind gets angry here.
							//Reason: conditional move... depends on uninitialised value(s), no actual problem
			gccArgs = strcat(gccArgs, argv[i]);//Then stick the latest arg on the end

		}
	}


	FILE *sourceFile = fopen(argv[1], "r");
	FILE *outFile = fopen(defaultOutName? strcat(argv[1], ".c") : outName, "w+");
	if(!sourceFile){
		puts("Error: Invalid source file");
		fclose(outFile);
		free(outName);
		free(gccArgs);
		return 1;
	}
	createFileHead(outFile, tapeLen);
	createFileBody(sourceFile, outFile);
	createFileFoot(outFile);

	//We're done with the files. Close source to reduce memory usage, close out to commit changes
	fclose(sourceFile);
	fclose(outFile);
	char *gccCall = malloc(3 + sizeof(gccArgs));
	gccCall = "gcc";
	gccCall = strcat(gccCall, gccArgs);//Segfault here

	free(outName);
	free(gccArgs);
	return 0;
}



void createFileHead(FILE *outFile, uint64_t tapeLen){
	fputs("#include <stdio.h>\n", outFile);//So bf can print
	fputs("#include <stdint.h>\n", outFile);//So bf can use uint8_t
	fputs("#include <stdlib.h>\n", outFile);//So the tape can be calloc-ed rather than kept in the call stack - less dangerous in case of overflow
						//calloc is used here to initialise to 0
	//Globally visible tape start, end, length and current cell
	fputs("static uint8_t *tapeStart;\n", outFile);
	fputs("static uint8_t *tapeEnd;\n", outFile);
	fprintf(outFile, "static uint64_t tapeLen = %lu;\n", tapeLen);
	fputs("static uint8_t *activeCell;\n", outFile);
	//Function prototypes just because it's good practise
	fputs("static void incPoint();\n", outFile);//>
	fputs("static void decPoint();\n", outFile);//<
	fputs("static void incVal();\n", outFile);//+
	fputs("static void decVal();\n", outFile);//-
	//[ ] , and . will be handled in main as while , getchar and putchar
	fputs("int main(int argc, char *argv[]){\n", outFile);
		fputs("\ttapeStart = calloc(tapeLen, 1);\n", outFile);
		fputs("\ttapeEnd = tapeStart + tapeLen-1;\n", outFile);
		fputs("\tactiveCell = tapeStart;\n", outFile);

}


void createFileBody(FILE *sourceFile, FILE *outFile){
	char c = fgetc(sourceFile);
	while(c != EOF){
		switch(c){
			case '>':
				fputs("\tincPoint();\n", outFile);
				break;
			case '<':
				fputs("\tdecPoint();\n", outFile);
				break;
			case '+':
				fputs("\tincVal();\n", outFile);
				break;
			case '-':
				fputs("\tdecVal();\n", outFile);
				break;
			case ',':
				fputs("\t*activeCell = getchar();\n", outFile);
				break;
			case '.':
				fputs("\tputchar(*activeCell);\n", outFile);
				break;
			case '[':
				fputs("\twhile(*activeCell){\n", outFile);
				break;
			case ']':
				fputs("\t}\n", outFile);
			default:
				break;

		}

		c = fgetc(sourceFile);
	}




}


void createFileFoot(FILE *outFile){
	fputs("\tfree(tapeStart);\n", outFile);
	fputs("\treturn 0;\n", outFile);
	fputs("}\n", outFile);//Ends main


	fputs("static void incPoint(){\n", outFile);
		fputs("\tif (activeCell == tapeEnd) activeCell = tapeStart;\n", outFile);//So the tape wraps, as per the specification
		fputs("\telse ++activeCell;\n", outFile);
	fputs("}\n", outFile);


	fputs("static void decPoint(){\n", outFile);
		fputs("\tif (activeCell == tapeStart) activeCell = tapeEnd;\n", outFile);//So the tape wraps
		fputs("\telse --activeCell;\n", outFile);
	fputs("}\n", outFile);

	fputs("static void incVal(){\n", outFile);
		fputs("\t++*activeCell;\n", outFile);
	fputs("}\n", outFile);

	fputs("static void decVal(){\n", outFile);
		fputs("\t--*activeCell;\n", outFile);
	fputs("}\n", outFile);

}
