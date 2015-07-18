#include <stdio.h>//Error reporting
#include <stdint.h>//Because uint8_t makes purpose more obvious than char in a memory block and make handy boolean variables
#include <unistd.h>//System calls to gcc
#include <stdlib.h>//For malloc and realloc
#include <string.h>//For memcmp
#include <errno.h>//For errors with strtol









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
	char *outName;
	char *gccArgs = malloc(0);
	uint8_t defaultOutName = 1;
	uint64_t tapeLen = 30000;//Because it can be reassigned with strtol, it must be able to fit a long value. Can store values greater than available
				 //memory, so if you want to run brainf**k++ on a supercomputer, that's your problem, not mine



	//Parse command line arguments
	uint8_t reservedForGcc = 0;//So that when -g comes up, further commands are not parsed by this program.
	for(int i = 2; i < argc; i++){//Find out which arguments are in use, starts at 2 because 0 is program call, and 1 is either help or the source file
		if(!reservedForGcc){
			if(argv[i][0] == '-'){//If we're looking at an option block
				if(!memcmp(argv[i], "-o", 3)){
					defaultOutName = 0;
					outName = argv[i+1];
					i++;//Skip over the next argument as it is a file name
				}
				else if(!memcmp(argv[i], "-m", 3)){
					char *storageChar;//For strto to store stuff
					tapeLen = strtol(argv[i+1], &storageChar, 10);
					if(!tapeLen || errno){
						puts("Error: Invalid tape length");
						return 1;
					}
					i++;//Skip over the next argument as it is the memory size

				}else if (memchr(argv[i], 'c', 5)) keepC = 1;//Only checks 5 bytes as there are only 5 valid args and most shouldn't be together
				else if (!memcmp(argv[i], "-g", 3)) reservedForGcc = 1;//If you've called -g, the rest of the args must belong to gcc

			}

		}else{
			gccArgs = realloc(gccArgs, sizeof(*gccArgs) + sizeof(*argv[i]) + 1);//+1 for the space
			gccArgs = strcat(gccArgs, " ");//Put spaces between each arg so that gcc understands
			gccArgs = strcat(gccArgs, argv[i]);//Then stick the latest arg on the end

		}
	}



	FILE *sourceFile = fopen(argv[1], "r");











	free(gccArgs);
	return 0;
}
