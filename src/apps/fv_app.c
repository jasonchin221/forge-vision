#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "fv_types.h"
#include "fv_app.h"
#include "fv_track.h"

static fv_app_proc_file_t fv_app_proc[] = {
    {"image", 1, fv_cv_detect_img},
    {"video", 1, fv_cv_detect_video},
    {"camera", 0, fv_cv_detect_camera},
};

#define fv_app_proc_size (sizeof(fv_app_proc)/sizeof(fv_app_proc_file_t))

static fv_app_algorithm_t fv_app_algorithm[] = {
    {"lk_optical_flow", {NULL, fv_cv_lk_optical_flow}},
};

#define fv_app_alg_num (sizeof(fv_app_algorithm)/sizeof(fv_app_algorithm_t))

static const char *
fv_program_version = "1.0.0";//PACKAGE_STRING;

static const struct option 
fv_long_opts[] = {
	{"help", 0, 0, 'H'},
	{"debug", 0, 0, 'D'},
	{"type", 0, 0, 'T'},
	{"list", 0, 0, 'L'},
	{"image_file", 1, 0, 'f'},
	{"code_file", 1, 0, 'c'},
	{"output_dir", 1, 0, 'o'},
	{"right_sample_dir", 1, 0, 'r'},
	{"wrong_sample_dir", 1, 0, 'w'},
	{"mode", 1, 0, 'm'},
	{"num", 1, 0, 'n'},
	{"algorithm", 1, 0, 'a'},
	{"check_per", 1, 0, 'p'},
	{"err_per", 1, 0, 'e'},
	{"thread", 1, 0, 't'},

	{0, 0, 0, 0}
};

static const char *
fv_options[] = {
	"--type         -T	file type(image, video, camera)\n",
	"--debug        -D	enable debug mode\n",	
	"--list         -L	list algorithm name\n",	
	"--algorithm    -a	algorithm name\n",	
	"--right        -r	right sample directive\n",	
	"--file         -f	file location\n",
	"--code         -c	code file to store strong classifier\n",
	"--wrong        -w	wrong sample directive\n",	
	"--sequence     -s	sequence number of the strong classifier\n",	
	"--output_dir   -o	test data output dir\n",	
	"--mode         -m	mode of algorithm, 0 for forgevision, 1 for opencv\n",	
	"--number       -n	the number of strong classifiers\n",	
	"--check_per    -p	check percent of one strong classifier\n",	
	"--error_per    -e	error percent of one strong classifier\n",	
	"--thread_num   -t	thread number to run adaboost\n",	
	"--help         -H	Print help information\n",	
};

static void 
fv_help()
{
	int i;

	fprintf(stdout, "Version: %s\n", fv_program_version);

	fprintf(stdout, "\nOptions:\n");
	for(i = 0; i < sizeof(fv_options)/sizeof(char *); i ++) {
		fprintf(stdout, "  %s", fv_options[i]);
	}

	return;
}

static const char *
fv_optstring = "HDLT:a:f:o:r:w:n:c:m:t:e:";

int main(int argc, char **argv)  
{
    int                         c;
    char                        *type = NULL;
    char                        *im_file = NULL;
    char                        *name = NULL;
    fv_app_algorithm_t          *alg = NULL;
    fv_proc_func                func;
    fv_bool                     list = 0;
    fv_u8                       mode = 0;
    fv_u32                      i;
#if 0
    char                        *code_file = NULL;
    char                        *r = NULL;
    char                        *w = NULL;
    char                        learn = 0;
    fv_s8                       check_per = 0;
    fv_s8                       error_per = -1;
    fv_u32                      thread_num = 4;
    fv_u32                      clf_num = 0;
#endif

    while((c = getopt_long(argc, argv, 
                    fv_optstring,  fv_long_opts, NULL)) != -1) {
        switch(c) {
            case 'H':
                fv_help();
                return FV_ERROR;

            case 'D':
                break;

            case 'T':
                type = optarg;
                break;

            case 'a':
                name = optarg;
                break;

            case 'L':
                list = 1;
                break;

            case 'f':
                im_file = optarg;
                break;

#if 0
            case 'c':
                code_file = optarg;
                break;

            case 'o':
                break;

            case 'r':
                r = optarg;
                break;

            case 'w':
                w = optarg;
                break;

            case 'n':
                clf_num = atoi(optarg);
                break;

            case 't':
                thread_num = atoi(optarg);
                break;

            case 'p':
                check_per = atoi(optarg);
                break;

            case 'e':
                error_per = atoi(optarg);
                break;
#endif
            case 'm':
                mode = atoi(optarg);
                break;

            default:
                fv_help();
                return FV_ERROR;
        }
    }

    if (list) {
        alg = &fv_app_algorithm[0];
        for (i = 0; i < fv_app_alg_num; i++, alg++) {
            fprintf(stdout, "%s\n", alg->ag_name);
        }
        return FV_OK;
    }

    if (type == NULL) {
        fprintf(stderr, "Please input type with -T!\n");
        return FV_ERROR;
    }

    if (name == NULL) {
        fprintf(stderr, "Please input algorithm name with -a!\n");
        return FV_ERROR;
    }

    for (i = 0; i < fv_app_alg_num; i++) {
        if (strcmp(name, fv_app_algorithm[i].ag_name) == 0) {
            alg = &fv_app_algorithm[i];
            break;
        }
    }

    if (alg == NULL) {
        fprintf(stderr, "Unknow algorithm name %s!\n", name);
        return FV_ERROR;
    }

    if (mode < 0 || mode > FV_PROC_MODE_MAX) {
        fprintf(stderr, "Unknow mode %d\n", mode);
        return FV_ERROR;
    }

    func = alg->ag_func[mode];
    if (func == NULL) {
        fprintf(stderr, "Mode %d have no algorithm %s!\n", mode, name);
        return FV_ERROR;
    }

    for (i = 0; i < fv_app_proc_size; i++) {
        if (strcmp(fv_app_proc[i].pf_type, type) == 0) {
            if (fv_app_proc[i].pf_use_file && im_file == NULL) {
                fprintf(stderr, "Please input im_file with -f!\n");
                return FV_ERROR;
            }
            return fv_app_proc[i].pf_proc_func(im_file, func);
        }
    }

    return FV_ERROR;
}
