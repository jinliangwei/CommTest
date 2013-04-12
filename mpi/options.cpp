/* -*- mode: C; mode: folding; fill-column: 70; -*- */
/* Copyright 2010,  Georgia Institute of Technology, USA. */
/* See COPYING for license. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* getopt should be in unistd.h */
#if HAVE_UNISTD_H
#include <unistd.h>
#else
#if !defined(__MTA__)
#include <getopt.h>
#endif
#endif
#include "options.hpp"
#include "getinput.hpp"

using namespace std;

void get_options (int argc, char **argv, struct problem *pcfg, struct context *ctx, struct experiments *expt) {

  extern char *optarg;
  int c, err = 0;
  
  expt->maxiter = DEFAULT_MAX_ITER;
  expt->timelimit = DEFAULT_MAX_TIME_SEC;

  while ((c = getopt (argc, argv, "V?hd:o:m:t:l:ie")) != -1)
    
    switch (c) {
    case 'h':
    case '?':
      printf ("Usuage: exetuable -dX.txt -oY.txt\n");      
      exit (EXIT_SUCCESS);
      break;
    case 'd':
      pcfg->xfile  = strdup (optarg);
      if (!pcfg->xfile) {
	fprintf (stderr, "can not copy x file name.\n");
	err = 1;
      }
      break;
    case 'o':
      pcfg->yfile  = strdup (optarg);
      if (!pcfg->yfile) {
	fprintf (stderr, "can not copy y file name\n");
	err = 1;
      }
      break;
    case 'm':
      ctx->machines  = strtol (optarg, NULL, 10);
      if (ctx->machines < 1) {
	fprintf (stderr, "machine must be larger than 1\n");
	err = 1;
      }
      break;
    case 't':
      ctx->thrdpm  = strtol (optarg, NULL, 10);
      if (ctx->thrdpm < 1) {
	fprintf (stderr, "thread per machine must be larger than 1\n");
	err = 1;
      }
      break;
    case 'l':
      ctx->lamda  = strtod (optarg, NULL);
      if (ctx->lamda <= 0) {
	fprintf (stderr, "lamda  must be larger than 0\n");
	err = 1;
      }
      break;


    case 'i':
      expt->maxiter  = strtol (optarg, NULL, 10);
      if (expt->maxiter < 1) {
	fprintf (stderr, "max iteration must be larger than 1\n");
	err = 1;
      }
      break;

    case 'e':
      expt->timelimit  = strtol (optarg, NULL, 10);
      if (expt->timelimit < 1) {
	fprintf (stderr, "time limit must be larger than 1\n");
	err = 1;
      }
      break;

    default:
      fprintf (stderr, "Unrecognized option [%c]\n", c);
      err = -1;
    }

  ctx->thrds = ctx->machines*ctx->thrdpm;

  if (err)
    exit (EXIT_FAILURE);
}
