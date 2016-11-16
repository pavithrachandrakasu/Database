#include "btree_mgr.h"
#include "tables.h"
#include "storage_mgr.h"
#include "record_mgr.h"
#include <stdlib.h>
#include <string.h>
#include "buffer_mgr.h"
#include "list.h"



/*** Global Variable Initialisation ******/
SM_FileHandle fh;
int numKeys;
BTree *root;
BTree *scan;
int ind= 0;


/*** Initialization of B+ TREE ***/
RC initIndexManager (void *mgmtData)
{
    initStorageManager();
    return RC_OK;
}

/*** Shutdown B+ TREE *******/
RC shutdownIndexManager ()
{
    return RC_OK;
}

/*** Creates a B+ tree structure for the given order *****/
RC createBtree (char *idxId, DataType keyType, int n)
{
    
    BTreeHandle *tree;
    char data[PAGE_SIZE];
    char *offset= data;
    offset+= sizeof(int);
    tree=(BTreeHandle *)malloc(sizeof(BTreeHandle)*3); // Tree data structure initialization
    root = (BTree*)malloc(sizeof(BTree));
    tree->idxId=idxId;
    tree->keyType=keyType;
    root->key = malloc(sizeof(int) * n);
    root->id = malloc(sizeof(int) * n);
    root->next = malloc(sizeof(BTree) * (n+1));
    BT_Info *bi= (BT_Info*) malloc( sizeof(BT_Info) ); // additional data structure initialization
    bi->rootPage = 1;
    bi->leaf = 0;
    bi->numKeys = n + 2;
    bi->entryKeys =*(int*)offset;
    bi->order = n;
    numKeys = bi->order;
    tree->mgmtData =bi;
    int i;
    for (i = 0; i < bi->order ; i ++)
        root->next[i] = NULL;		/* for the given order of the tree, initialize null values */
    createPageFile (idxId);		/* create new page file for the with the given name for the tree */
    return RC_OK;
}


/*** Opens a B+ tree to do different operations *****/
RC openBtree (BTreeHandle **tree, char *idxId)
{
  
   BT_Info *bi= (BT_Info*) malloc( sizeof(BT_Info) );
   openPageFile (idxId, &fh);
   return RC_OK;
}

/*** Close a B+ tree *****/
RC closeBtree (BTreeHandle *tree)
{
    closePageFile(&fh);
    free(root); /* free resources allocated */
    free(scan); 
    return RC_OK;
}


/*** Deletes a B+ tree respective details ****/
RC deleteBtree (char *idxId)
{
    destroyPageFile(idxId);
    
    return RC_OK;
}


/*** Returns the number of nodes in a B+ TREE  *****/
RC getNumNodes (BTreeHandle *tree, int *result)
{
    
    int nodeCount = 0,i=0;
    do{
	nodeCount ++;
        i++;
       }while(i < numKeys + 2);
    *result = nodeCount;
    return RC_OK;
}

/*** Returns the number of elements in a B+ TREE  *****/
RC getNumEntries (BTreeHandle *tree, int *result)
{
    int totalElements = 0;
    totalElements = calcElements();
    *result = totalElements;
    return RC_OK;
}

/** gets the key type of the key to be inserted in the BTREE ****/
RC getKeyType (BTreeHandle *tree, DataType *result)
{
   BT_Info *bi= (BT_Info*) malloc( sizeof(BT_Info));
   *result = bi->keyType;
   return RC_OK;
}


/** finds a key in the BTREE ****/
RC findKey (BTreeHandle *tree, Value *key, RID *result)
{
     BTree *btree = (BTree*)malloc(sizeof(BTree));
    int flag = 0, i;


 /*** key found condition *******/

    btree = root;
    do
    {
 	 for (i = 0; i < numKeys; i ++) {
            if (btree->key[i] == key->v.intV) {
                (*result).page = btree->id[i].page;
                (*result).slot = btree->id[i].slot;
                flag = 1;
                break;
            }}
         btree = btree->next[numKeys];
    }while(btree != NULL);
    if (flag!=1)
        return RC_IM_KEY_NOT_FOUND; /*** key not found ******/
    else
        return RC_OK;
}


/**Insert a key in BTREE ***/
RC insertKey (BTreeHandle *tree, Value *key, RID rid)
{
    insert(tree,key,rid);
    return RC_OK;
}

/** Deletes a key from the BTREE ***/
RC deleteKey (BTreeHandle *tree, Value *key)
{
    BTree *btree = (BTree*)malloc(sizeof(BTree));
    int i;

    for (btree = root; btree != NULL; btree = btree->next[numKeys]) 
    {
        for (i = 0; i < numKeys; i ++) 
        {
            if (btree->key[i] == key->v.intV) /*** if key found for deletion *******/
	    {  
                btree->id[i].slot = 0;
                btree->key[i] = 0;
                btree->id[i].page = 0;   
		return RC_OK;
	    }
	    else
            {
		RC_IM_KEY_NOT_FOUND;   /*** if key is not found for deletion *******/
	    }    
        }
	
    }  
    
}

/*** opens a scan on B+ TREE to search for an element ******/

RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle)
{
    scan = (BTree*)malloc(sizeof(BTree));
    scan = root;
    ind = 0;
    BTree *tmp = (BTree*)malloc(sizeof(BTree));
    int totalKeys, i,j,k = 0;
    totalKeys = calcElements();
    int key[totalKeys];
    int nodes[numKeys][totalKeys];
    int change,first, second;
    tmp=scan;
    do
    {
       for(j=0;j<numKeys;j++)
       {
        nodes[0][k] = tmp->id[j].page;
           nodes[1][k] = tmp->id[j].slot;
           key[k] = tmp->key[j];
           k++;
       }
       tmp = tmp->next[numKeys];
    }while(tmp!=NULL);

/**** swapping elements in B+ TREE *********/

    for (i = 0 ; i < k ; i++)
    {
        for (j = 0 ; j < k-i ; j++)
        {
            if (key[j+1] < key[j])
            {

                first = nodes[0][j];
                second = nodes[1][j];
                change = key[j];              
                nodes[0][j] = nodes[0][j + 1];
                nodes[1][j] = nodes[1][j + 1];
                nodes[0][j + 1] = first;
                nodes[1][j + 1] = second;
                key[j]   = key[j + 1];              
                key[j + 1] = change;

            }
        }
    }
   
   k = 0;
   tmp=scan;
    do
    {
       for(j=0;j<numKeys;j++)
       {
         tmp->id[j].page = nodes[0][k];
         tmp->id[j].slot = nodes[1][k];
         tmp->key[j] =key[k];
         k++;
       }
       tmp = tmp->next[numKeys];
    }while(tmp!=NULL);
    return RC_OK;
}

/**** Scans for the next entry in the B+ TREE *********/

RC nextEntry (BT_ScanHandle *handle, RID *result)
{

     if (scan->next[numKeys]==NULL)
     {
    return RC_IM_NO_MORE_ENTRIES;
     }

      else if (ind == numKeys)
     {
            ind = 0;
            scan = scan->next[numKeys];
     }
        (*result).page = scan->id[ind].page;
        (*result).slot = scan->id[ind].slot;
        ind ++;
    return RC_OK;
}


/***** Closes the B+ TREE scan *********/
RC closeTreeScan (BT_ScanHandle *handle)
{
    free(handle);
    return RC_OK;
}


/**** Helper function used for B+ TREE Insertion ********/
RC insert(BTreeHandle *tree, Value *key, RID rid){
    int i = 0;
    int nodeStatus = 0;
    BTree *temp = (BTree*)malloc(sizeof(BTree));
    BTree *btree = (BTree*)malloc(sizeof(BTree));
    btree->key = malloc(sizeof(int) * numKeys);
    btree->next = malloc(sizeof(BTree) * (numKeys + 1));
    btree->id = malloc(sizeof(int) * numKeys);

 /*** checks if the key is already present , if its there it returns RC_IM_KEY_ALREADY_EXISTS *********/
  
 for (temp = root; temp != NULL; temp = temp->next[numKeys]) {
        for (i = 0; i < numKeys; i ++) {
            if (temp->key[i] == 0 && temp->key[i] == key->v.intV) 
			return RC_IM_KEY_ALREADY_EXISTS;

		}
	}
    
    for (temp = root; temp != NULL; temp = temp->next[numKeys]) {
        for (i = 0; i < numKeys; i ++) {
            if (temp->key[i] == 0) {
                temp->id[i].page = rid.page;
                temp->id[i].slot = rid.slot;
                temp->key[i] = key->v.intV;   /*** Only Integer Keys are supported ***/
                temp->next[i] = NULL;
                nodeStatus ++;
                break;
            }
        }
   /*** if the node status is empty and and the tree's next node is also null ****/
        if ((nodeStatus == 0) && (temp->next[numKeys] == NULL)) {
            btree->next[numKeys] = NULL;
            temp->next[numKeys] = btree;
        }
    } 
}

/*** debug function to print the BTREE structure ********/
char *printTree (BTreeHandle *tree)
{
    return RC_OK;
}

/*** Helper function to calculate the number of elements ****/
RC calcElements(){
 BTree *btree = (BTree*)malloc(sizeof(BTree));
    int numElements = 0, i;
    for (btree = root; btree != NULL; btree = btree->next[numKeys])
        for (i = 0; i < numKeys; i++)
            if (btree->key[i] != 0)
                numElements ++;
 return numElements;
}

