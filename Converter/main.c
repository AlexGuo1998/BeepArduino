#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

#include "decode.h"
#include "generator.h"

#define TRACK_MAX 1

#define _STR(s) #s
#define STR(s) _STR(s)

void printHelp(void) {
	fprintf(stderr,
		"BeepArduino-Converter - A program to convert music scopes to Arduino programs.\n"
		"Usage: beeparduino [options] outfile\n"
		"\n"
		"Options:\n"
		"-h\t\tshow this help\n"
		"-i infile\tmusic scope to load\n"
		//"\t\t(up to " STR(TRACK_MAX) " tracks are supported)\n"
		"\t\t(only 1 track is supported in this version)\n"
		"\n"
		"Project website: <https://github.com/AlexGuo1998/BeepArduino>\n"
	);
}

#ifdef _WIN32
//Get attached console count. if count==1, we are running standalone, show pause.
void exitpause(void) {
	DWORD proc;
	DWORD ret = GetConsoleProcessList(&proc, 1);
	if (ret == 1) {
		system("pause");
	}
}
#else // !_WIN32
#define exitpause() ((void)0)
#endif // _WIN32


int main(int argc, char *argv[]) {
	size_t trackcount = 0;
	char filelist[TRACK_MAX][FILENAME_MAX];
	if (argc <= 1) {
		//printHelp();
		fprintf(stderr, "Please select a output file!\ntype: \"beeparduino -h\" for help\n");
		exitpause();
		return 1;
	}
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0) {
			printHelp();
			exitpause();
			return 0;
		} else if (strcmp(argv[i], "-i") == 0) {
			if (trackcount >= TRACK_MAX) {
				fprintf(stderr, "\nYou can open at most " STR(TRACK_MAX) " files!\n");
				exitpause();
				return 1;
			}
			i++;
			strncpy(filelist[trackcount], argv[i], FILENAME_MAX);
			filelist[trackcount][FILENAME_MAX - 1] = '\0';
			trackcount++;
		}
	}

	if (trackcount == 0) {
		fprintf(stderr, "No input file!\ntype: \"beeparduino -h\" for help\n");
		exitpause();
		return 1;
	}

	char *notestr;

	{
		FILE *pf = fopen(filelist[0], "rb");
		if (pf == NULL) {
			fprintf(stderr, "Can't open file \"%s\"!\n", filelist[0]);//TODO
			return 1;
		}

		fseek(pf, 0, SEEK_END);
		int filelen;
		filelen = ftell(pf);
		notestr = (char *)malloc(filelen + 1);
		if (notestr == NULL) {
			fprintf(stderr, "Run out of memory!\n");
			return 1;
		}
		rewind(pf);
		size_t ret = fread(notestr, 1, filelen, pf);
		notestr[ret] = '\0';
		fclose(pf);
	}

	note_t *notelist;
	char *lyric;

	decodenote(notestr, &notelist, &lyric);
	free(notestr);

	size_t notecount = 0;
	while (notelist[notecount].time != 0) notecount++;
	
	unsigned long *sleeptime = (unsigned long *)malloc(notecount * sizeof(unsigned long));
	unsigned long *beeptime = (unsigned long *)malloc(notecount * sizeof(unsigned long));
	unsigned int *freq = (unsigned int *)malloc(notecount * sizeof(unsigned int));
	if (sleeptime == NULL || beeptime == NULL || freq == NULL) {
		fprintf(stderr, "Run out of memory!\n");
		return 1;
	}

	size_t notecount_optimized = 0;

	{
		double beeptime_last = 0, sleeptime_last = 0, freq_last = 0;
		double beeptime_next, sleeptime_next, freq_next;

		for (size_t i = 0; i < notecount; i++) {
			sleeptime_next = notelist[i].time * 1000;
			if (notelist[i].height == 0) {
				sleeptime_last += sleeptime_next;
			} else {
				beeptime_next = sleeptime_next * (100 - notelist[i].staccato) / 100;
				freq_next = 440 * pow(2, ((float)(notelist[i].height - 34) / 12));
				if (freq_last == freq_next && beeptime_last == sleeptime_last) {
					beeptime_last += beeptime_next;
					sleeptime_last += sleeptime_next;
				} else {
					if (freq_last != 0) {
						beeptime[notecount_optimized] = (unsigned long)(beeptime_last + .5);
						sleeptime[notecount_optimized] = (unsigned long)(sleeptime_last + .5);
						freq[notecount_optimized] = (unsigned int)(freq_last + .5);
						notecount_optimized++;
					}
					beeptime_last = beeptime_next;
					sleeptime_last = sleeptime_next;
					freq_last = freq_next;
				}
			}
		}
		beeptime[notecount_optimized] = (unsigned long)(beeptime_last + .5);
		sleeptime[notecount_optimized] = (unsigned long)(sleeptime_last + .5);
		freq[notecount_optimized] = (unsigned int)(freq_last + .5);
		notecount_optimized++;
	}

	bool ret = output(argv[argc - 1], freq, beeptime, sleeptime, notecount_optimized);

	if (ret == false) {
		fprintf(stderr, "Output failed!\n");
		return 1;
	}

	return 0;
}
