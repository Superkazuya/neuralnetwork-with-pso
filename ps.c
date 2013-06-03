#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include <pthread.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define NUM_AGENTS 3
#define MAX_ITER 100000
#define MAX_ERR  0.000000000001
#define INF 999

#define RAND(a, b) (((a)-(b))*(2.0*rand()/RAND_MAX-1)+(b))
#define NORM ((double)rand()/RAND_MAX)

//global
double g_weight_best[N_LAY-1][N_MAX][N_MAX];
double g_thresh_best[N_LAY][N_MAX];
double g_best_val = INF;

pthread_mutex_t mutex_g;

typedef struct
{
  double weight_best[N_LAY-1][N_MAX][N_MAX];
  double weight_curr[N_LAY-1][N_MAX][N_MAX];
  double weight_velo[N_LAY-1][N_MAX][N_MAX];

  double thresh_best[N_LAY][N_MAX];
  double thresh_curr[N_LAY][N_MAX];
  double thresh_velo[N_LAY][N_MAX];

  double val_best;

  double input[N_IN];
  double target[N_OUT];
} agent;

//int domain[NUM_DIM][2];

static double eval(agent         *);
static void   evaluate(agent     *);
static void   init(agent  *);
static void   init_thresh(agent  *);
static void   init_weight(agent  *);
static void   store();
static void   update(agent       *);

double 
eval(agent* p)
{
  unsigned int i, k, n;
  double sum;
  double output[N_LAY][N_MAX];
  for(k = 0; k < N_IN; k++)
    output[0][k] = NEURO(p->input[k] - p->thresh_curr[0][k]);
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
    sum += fabs(output[N_LAY-1][k] - p->target[k]);
  return(sum);
}

void
evaluate(agent* p)
{
  double val;
  //Evaluate
  p->input[0] = 0;
  p->input[1] = 0;
  p->target[0] = 0;
  //target[1] = 0;
  val = eval(p);
  p->input[0] = 0;
  p->input[1] = 1;
  p->target[0] = 1;
  //target[1] = 1;
  val += eval(p);
  p->input[0] = 1;
  p->input[1] = 0;
  p->target[0] = 1;
  //target[1] = 1;
  val += eval(p);
  p->input[0] = 1;
  p->input[1] = 1;
  p->target[0] = 0;
  //target[1] = 0;
  val += eval(p);

  if(val < p->val_best)
  {
    p->val_best = val;
    memcpy(p->weight_best, p->weight_curr, sizeof(p->weight_best));
    memcpy(p->thresh_best, p->thresh_curr, sizeof(p->thresh_best));
    if(val < g_best_val)
    {
      pthread_mutex_lock(&mutex_g);
      g_best_val = val;
      printf("New global best found! %.10lf\n", val);
      memcpy(g_weight_best, p->weight_curr, sizeof(g_weight_best));
      memcpy(g_thresh_best, p->thresh_curr, sizeof(g_thresh_best));
      if(g_best_val > MAX_ERR) //result satisfactory, lock global value
	pthread_mutex_unlock(&mutex_g);
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
init(agent* array)
{
  array->val_best = INF;
  init_weight(array);
  init_thresh(array);
}

void
store()
{
  unsigned int i, n, k;
  xmlDoc* doc = NULL;
  xmlNode* matrices = NULL;
  xmlNode* node = NULL;
  char buff[4096] = "\0";
  char tmp[64];

  doc = xmlNewDoc(BAD_CAST "1.0");
  matrices = xmlNewNode(NULL, BAD_CAST "matrices");
  xmlDocSetRootElement(doc, matrices);

  for(i = 0; i < N_LAY-1; i++)
  {
    for(n = 0; n < N_MAX; n++)
    {
      for(k = 0; k < N_MAX; k++)
      {
	sprintf(tmp, " %lf  ", g_weight_best[i][n][k]);
	strcat(buff, tmp);
      }
      strcat(buff, "\n");
    }
    strcat(buff, "\n");
  }
  xmlNewChild(matrices, NULL, BAD_CAST "weight", BAD_CAST buff);

    //threshold
  strcpy(buff, "\0");
  for(i = 0; i < N_LAY; i++)
  {
      for(k = 0; k < N_MAX-1; k++)
      {
	sprintf(tmp, " %lf  ", g_thresh_best[i][k]);
	strcat(buff, tmp);
      }
    strcat(buff, "\n");
  } 
  xmlNewChild(matrices, NULL, BAD_CAST "thresh", BAD_CAST buff);

  xmlKeepBlanksDefault(0) ;//libxml2 global variable .
  xmlIndentTreeOutput = 1 ;// indent .with \n 
  xmlSaveFormatFile(FILENAME, doc, 1);
  printf("data successfully stored.\n");
  xmlFreeDoc(doc);
  xmlCleanupParser();
  xmlMemoryDump();
}

void
update(agent* p)
{
  unsigned int i, k, n;
  static float inherit_weight = 1;
  float weight1;
  float weight2;
  weight1 = RAND(2, 0);
  weight2 = RAND(2, 0);
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
  inherit_weight *= 0.9999;
}

void*
thread_agent(void* argv)
{
  agent smith;
  unsigned int counter = 0;
  init(&smith);
  printf("Thread %d ready to go!\n", *(int*)argv);
  *(int*)argv = 0;
  while(counter++ < MAX_ITER)
  {
    evaluate(&smith);
    if(g_best_val <= MAX_ERR)
      pthread_exit(argv);
    update(&smith);
  }
  *(int*)argv = 1;
  pthread_exit(argv);
}

int
main()
{
  unsigned int i, counter = 0;
  int array[NUM_AGENTS];
  void* status;
  pthread_t pthread_id[NUM_AGENTS];
  pthread_attr_t pthread_attr;
  pthread_mutex_init(&mutex_g, NULL);
  pthread_attr_init(&pthread_attr);
  pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_JOINABLE);
  for(i = 0; i < NUM_AGENTS; i++)
  {
    array[i] = i;
    pthread_create(pthread_id+i, &pthread_attr, thread_agent, (void *)(array+i));
  }
  pthread_attr_destroy(&pthread_attr);

  for(i = 0; i < NUM_AGENTS; i++)
  {
    pthread_join(pthread_id[i], &status);
    printf("Thread return: %d.\n", *(int*)status);
    if(*(int*)status == 0)
      goto STORE;
  }
  printf("BAD ENDING.\n");
  exit(EXIT_FAILURE);
STORE:
  printf("GOOD ENDING.\n");
  store();
  exit(EXIT_SUCCESS);
}


