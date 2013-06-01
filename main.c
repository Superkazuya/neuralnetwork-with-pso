#include <math.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "config.h"

int
main()
{
  unsigned int i, n, j, m, k;
  double       sum;

  double       weight[N_LAY-1][N_MAX][N_MAX];
  double       output[N_LAY][N_MAX];
  double       thresh[N_LAY][N_MAX];
  double       input[N_IN];

  xmlDoc*  doc;
  xmlNode* matrices = NULL;
  xmlNode* node;
  xmlNode* layer;

  char* matrix, rest;
  //xml parse
  if((doc = xmlReadFile(FILENAME, NULL, 0)) == NULL)
  {
    printf("Error opening XML file.\n");
    return(EXIT_FAILURE);
  }
  
  if((matrices = xmlDocGetRootElement(doc)) == NULL)
    goto XML_ERROR;
#ifdef DEBUG
  printf("root name: %s\n", matrices->name);
#endif

  //find node weight
  node = matrices->xmlChildrenNode->next;
#ifdef DEBUG
  printf("node1 name: %s\n", node->name);
#endif
  if(xmlStrcmp(node->name, (const xmlChar*)"weight"))
    goto XML_ERROR;

  //print weight
  matrix = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
  for(i = 0; i < N_LAY-1; i++)
    for(n = 0; n < N_MAX; n++)
      for(k = 0; k < N_MAX; k++)
	weight[i][n][k] = strtod(matrix, &matrix);

  node = node->next->next;
#ifdef DEBUG
  printf("node2 name: %s\n", node->name);
#endif
  if(xmlStrcmp(node->name, (const xmlChar*)"thresh"))
    goto XML_ERROR;
  matrix = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
  for(i = 0; i < N_LAY; i++)
    for(k = 0; k < N_MAX; k++)
      thresh[i][k] = strtod(matrix, &matrix);
  printf("XML file loaded successfully.\n");

  for(m = 0; m < 2; m++)
    for(j = 0; j < 2; j++)
    {
      input[0] = m;
      input[1] = j;
      printf("%lf, %lf:", input[0], input[1]);
      for(k = 0; k < N_IN; k++) //first layer
	output[0][k] = NEURO(input[k] - thresh[0][k]);
      for(i = 0; i < N_LAY-1; i++) //following layer
	for(n = 0; n < neuron_num[i+1]; n++)
	{
	  sum = thresh[i+1][n];
	  for(k = 0; k < neuron_num[i]; k++)
	    sum += weight[i][n][k]*output[i][k];
	  output[i+1][n] = NEURO(sum);
	}
      //printf("%lf, %lf\n", output[N_LAY-1][0], output[N_LAY-1][1]);
      printf("%lf\n", output[N_LAY-1][0]);
    }

#ifdef DEBUG
    printf("\n");
    for(i = 0; i < N_LAY-1; i++)
    {
      for(n = 0; n < N_MAX; n++)
      {
	for(k = 0; k < N_MAX; k++)
	  printf("%lf", weight[i][n][k]);
	printf("\n");
      }
      printf("\n");
    }
#endif

    return(EXIT_SUCCESS);
XML_ERROR:
    printf("XML empty or corrupted.\n");
    //xmlFreeDoc(doc);
    xmlCleanupParser();
    xmlMemoryDump();
    return(EXIT_FAILURE);
}
