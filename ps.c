#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "config.h"

#define NUM_AGENTS 1000
#define MAX_ITER 10000
#define INF 1000

#define RAND(a, b) (((a)-(b))*(2.0*rand()/RAND_MAX-1)+(b))
#define NORM ((double)rand()/RAND_MAX)

double g_weight_best[N_LAY-1][N_MAX][N_MAX];
double g_thresh_best[N_LAY][N_MAX];
double g_best_val = INF;

double input[N_IN];
double target[N_OUT];

typedef struct
{
  double weight_best[N_LAY-1][N_MAX][N_MAX];
  double weight_curr[N_LAY-1][N_MAX][N_MAX];
  double weight_velo[N_LAY-1][N_MAX][N_MAX];

  double thresh_best[N_LAY][N_MAX];
  double thresh_curr[N_LAY][N_MAX];
  double thresh_velo[N_LAY][N_MAX];

  double val_best;
} agent;

//int domain[NUM_DIM][2];

static double eval(agent         *);
static void   evaluate(agent     *);
static void   init_agents(agent  *);
static void   init_thresh(agent  *);
static void   init_weight(agent  *);
static void   store();
static void   update(agent       *);

double 
eval(agent* p)
{
  unsigned int i, k, n;
  double sum;
  static double output[N_LAY][N_MAX];
  for(k = 0; k < N_IN; k++)
    output[0][k] = NEURO(input[k] - p->thresh_curr[0][k]);
  for(i = 0; i < N_LAY-1; i++) //following layer
    for(n = 0; n < neuron_num[i+1]; n++)
    {
      sum = p->thresh_curr[i+1][n];
      for(k = 0; k < neuron_num[i]; k++)
	sum += p->weight_curr[i][n][k]*output[i][k];
      output[i+1][n] = NEURO(sum);
    }
  sum = 0;
  for(k = 0; k < N_OUT; k++)
    sum += fabs(output[N_LAY-1][k] - target[k]);
  return(sum);
}

void
evaluate(agent* array)
{
  unsigned int i, j;
  double val;
  agent* p;
  for(j = 0; j < NUM_AGENTS; j++)
    //for every agent
  {
    p = array+j;
    //val = 0;
    //Evaluate
    input[0] = 0;
    input[1] = 0;
    target[0] = 0;
    //target[1] = 0;
    val = eval(p);
    input[0] = 0;
    input[1] = 1;
    target[0] = 1;
    //target[1] = 1;
    val += eval(p);
    input[0] = 1;
    input[1] = 0;
    target[0] = 1;
    //target[1] = 1;
    val += eval(p);
    input[0] = 1;
    input[1] = 1;
    target[0] = 0;
    //target[1] = 0;
    val += eval(p);

    if(val < p->val_best)
    {
      p->val_best = val;
      memcpy(p->weight_best, p->weight_curr, sizeof(p->weight_best));
      memcpy(p->thresh_best, p->thresh_curr, sizeof(p->thresh_best));
      if(val < g_best_val)
      {
	g_best_val = val;
	memcpy(g_weight_best, p->weight_curr, sizeof(g_weight_best));
	memcpy(g_thresh_best, p->thresh_curr, sizeof(g_thresh_best));
      }
    }
  }
}

void
init_weight(agent* p)
{
  unsigned int i, n, k;
  for(i = 0; i < N_LAY-1; i++)
    for(n = 0; n < neuron_num[i+1]; n++)
      for(k = 0; k < neuron_num[i]; k++)
      {
	p->weight_curr[i][n][k] = RAND(10, 0);
	p->weight_velo[i][n][k] = RAND(1, 0);
      }
}

void
init_thresh(agent* p)
{
  unsigned int i, k;
  for(i = 0; i < N_LAY; i++)
    for(k = 0; k < neuron_num[i]; k++)
    {
      p->thresh_curr[i][k] = RAND(10, 0);
      p->thresh_velo[i][k] = RAND(1, 0);
    }
}

void
init_agents(agent* array)
{
  unsigned int j;
  for(j = 0; j < NUM_AGENTS; j++)
  {
    array[j].val_best = INF;
    init_weight(array+j);
    init_thresh(array+j);
  }
}

void
store()
{
  unsigned int i, n, k;
  FILE* fp = fopen(FILENAME, "w+");
  fprintf(fp, "<?xml version=\"1.0\" ?>\n");
  fprintf(fp, "<matrices>\n");
  fprintf(fp, "<weight>\n");
  for(i = 0; i < N_LAY-1; i++)
  {
    for(n = 0; n < N_MAX; n++)
    {
      for(k = 0; k < N_MAX; k++)
	fprintf(fp, " %lf  ", g_weight_best[i][n][k]);
      fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "</weight>\n\n");

    //threshold
  fprintf(fp, "<thresh>\n");
  for(i = 0; i < N_LAY; i++)
  {
      for(k = 0; k < N_MAX-1; k++)
	fprintf(fp, " %lf  ", g_thresh_best[i][k]);
    fprintf(fp, "\n");
  } 
  fprintf(fp, "</thresh>\n");
  fprintf(fp, "</matrices>\n");
  fclose(fp);
  printf("data successfully stored.\n");
}

void
update(agent* array)
{
  unsigned int i, j, k, n;
  static float inherit_weight = 1;
  float weight1;
  float weight2;
  agent* p;
  for(j = 0; j < NUM_AGENTS; j++)
  {
    weight1 = RAND(2, 0);
    weight2 = RAND(2, 0);
    p = array+j;
    for(i = 0; i < N_LAY-1; i++)
      for(n = 0; n < neuron_num[i+1]; n++)
	for(k = 0; k < neuron_num[i]; k++)
	{
	  p->weight_velo[i][n][k] *= inherit_weight;
	  p->weight_velo[i][n][k] += weight1*(p->weight_best[i][n][k] - p->weight_curr[i][n][k])
	    + weight2*(g_weight_best[i][n][k] - p->weight_curr[i][n][k]);
	  p->weight_curr[i][n][k] += p->weight_velo[i][n][k]; //update_position
	  if(fabs(p->weight_velo[i][n][k]) > INF || fabs(p->weight_curr[i][n][k]) > INF)
	  {
	    init_thresh(p);
	    init_weight(p);
	  }
	}
    for(i = 0; i < N_LAY; i++)
      for(k = 0; k < neuron_num[i]; k++)
      {
	p->thresh_velo[i][k] *= inherit_weight;
	p->thresh_velo[i][k] += weight1*(p->thresh_best[i][k] - p->thresh_curr[i][k])
	  + weight2*(g_thresh_best[i][k] - p->thresh_curr[i][k]);
	p->thresh_curr[i][k] = p->thresh_velo[i][k]; //update_position
	if(fabs(p->thresh_velo[i][k]) > INF || fabs(p->thresh_curr[i][k]) > INF)
	{
	  init_thresh(p);
	  init_weight(p);
	}

      }
  }
  //inherit_weight *= 0.9999;
}

int
main()
{
  agent agents[NUM_AGENTS];
  unsigned int i, counter = 0;
  init_agents(agents);
  evaluate(agents);
  while(counter++ < MAX_ITER && g_best_val > 0.001)
  {
    update(agents);
    evaluate(agents);
    printf("@iteration No.%d, best = %lf\n", counter, g_best_val);
  }
  store();
  exit(EXIT_SUCCESS);
}

