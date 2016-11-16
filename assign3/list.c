#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include "list.h"
#include "buffer_mgr.h"
#include "dberror.h"


/* creation of linked list and intialising the required values */

PageFrame* linked_list(int numPages)
{
  PageFrame *new_node, *current, *start;
  int i;
  int count =0;
  while ( count < numPages)
  {
    new_node = (PageFrame *) malloc(sizeof(PageFrame)*numPages); /* allocating the size for the linked list */
    new_node->dirty = FALSE;
    new_node->fixCount =0;
    new_node->handle = FALSE;
    new_node->ph.pageNum = -1;
    new_node->ph.data = NULL;
    new_node->position = 0;
    if (count==0)
    {
	start =new_node;
	current = new_node;
    }
    else
    {
        current->next = new_node;
	current = new_node;
    }
    current->next = start;
    count ++;
}
return start;

}

/* displays the contents of the linked list */

void display_list(BM_BufferPool *const bm,int numPages)
{
  PoolInfo *pi;
  PageFrame *pf;
  pi = (PoolInfo*) bm->mgmtData;
  pf = pi->pf;
  int i = 0;
  do 
  {
    if (i < numPages) 
    {
	printf("\nfix count = [%d] and FRAME [%d] and dirty [%d] and position [%d] and Handle[%d] and Pageno[%d]\n",pf->fixCount, i, pf->dirty,pf->position,pf->handle,pf->ph.pageNum );
	i = i + 1;
	pf = pf->next;
    } 
    else
	break;
  } while (pf != NULL);
}



/************ Record Manager related functions*********************/



/* helper functions */

// calculate each page can hold how many slot
int integer_adding(char *data, int length, int value) {
    memcpy(data + length, &value, sizeof(int));
    return sizeof(int)+length; 
}

int integer_getting(char *data, int length, int *value) {
    memcpy(value, data + length, sizeof(int));
    return sizeof(int)+length;
}

int float_addition(char *data, int length, float value) {
    memcpy(data + length, &value, sizeof(float));
    return sizeof(float) + length;
}

int float_getting(char *data, int length, float *value) {
    memcpy(value, data + length, sizeof(float));
    return sizeof(float) + length ;
}

int string_addition(char *data, int length, char *value) {
    int str_len = strlen(value);
    memcpy(data + length, &str_len, sizeof(int));
    memcpy(data + length + sizeof(int), value, strlen(value));
    return sizeof(int)+ length + str_len;
}

int string_getting(char *data, int length, char *value) {
    int str_len = 0;
    memcpy(&str_len, data + length, sizeof(int));
    memcpy(value, data + length + sizeof(int), str_len);
    value[str_len] = '\0';
    return sizeof(int)+ length + str_len ;
}

int slot_per_page(int size) {
    int r = PAGE_SIZE / size;
    int v = PAGE_SIZE / size;
    while (r * (size + 1) >= PAGE_SIZE) {
        r = r - 1;
    }
    return r;
}


int serialize_schema(char *page_data, int length, Schema *schema) {
    int i, temp_size = length;
    temp_size = integer_adding(page_data, temp_size, schema->numAttr);
    temp_size = integer_adding(page_data, temp_size, schema->keySize);
    for (i = 0; i < schema->keySize; i++) {
        temp_size = integer_adding(page_data, temp_size, schema->keyAttrs[i]);
    }
    for (i = 0; i < schema->numAttr; i++) {
        temp_size = integer_adding(page_data, temp_size, (int)schema->dataTypes[i]);
    }
    for (i = 0; i < schema->numAttr; i++) {
        temp_size = integer_adding(page_data, temp_size, schema->typeLength[i]);
    }
    for (i = 0; i < schema->numAttr; i++) {
        temp_size = string_addition(page_data, temp_size, schema->attrNames[i]);
    }
    return temp_size;
}

int deserialize_schema(char *page_data, int length, Schema **schema) {
    int i, total = 0, key = 0, temp_size = length,dType;
    int *keys = (int *) malloc(sizeof(int) * key);
    DataType *dt = (DataType *) malloc(sizeof(DataType) * total);
    int *tl = (int *) malloc(sizeof(int) * total);
    char **names = (char **) malloc(sizeof(char*) * total);

    temp_size = integer_getting(page_data, temp_size, &total);
    temp_size = integer_getting(page_data, temp_size, &key);

    for (i = 0; i < key; i++) {
        temp_size = integer_getting(page_data, temp_size, keys + i);
    }
    for (i = 0; i < total; i++) {
        temp_size = integer_getting(page_data, temp_size, &dType);
        dt[i] = (DataType) dType;
        // printf("get datatype : %d\n", dt[i]);
    }
    for (i = 0; i < total; i++) {
        temp_size = integer_getting(page_data, temp_size, tl + i);
    }
    for (i = 0; i < total; i++) {
        names[i] = (char *) malloc(sizeof(char) * tl[i]);
        temp_size = string_getting(page_data, temp_size, names[i]);
        // printf("get string : %s\n", names[i]);
    }

    *schema = createSchema(total, names, dt, tl, key, keys);
    free(keys);
    free(names);
    return temp_size;
}


int calculateOffset (Schema *schema, int attrNum, int *result)
{
  int temp = 0;
  int i = 0;
  
  for (i = 0; i < attrNum; i++) {
        switch (schema->dataTypes[i]) {
            case DT_INT :
                temp += sizeof(int);
                break;
            case DT_STRING :
                temp += schema->typeLength[i] + sizeof(int);
                break;
            case DT_FLOAT :
                temp += sizeof(float);
                break;
            case DT_BOOL :
                temp += sizeof(int);
                break;
        }
    }
  
  *result = temp;
  return RC_OK;
}


