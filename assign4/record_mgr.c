#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "tables.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "list.h"

/* global initialisation for tombstone */

int tombstone[1000];

/* intialization of record manager and addition of tombstone characteristics */
 
RC initRecordManager(void *mgmtData)  
{
   initStorageManager();
   int i = 0;
   for(;i<1000;i++)
   	tombstone[i] = -99;
   return RC_OK;
}

/* Shutdown an existing record manager */
RC shutdownRecordManager() 
{
   return RC_OK;
}

/* Creates a table in the page file and stores information about the schema */

RC createTable(char *name, Schema *schema) 
{
    SM_FileHandle fh;
    SM_PageHandle ph;
    createPageFile(name); /* create new page file with filename as table name to add the table data */
    openPageFile(name, &fh); /* open the page file */
    ph = (char *) malloc(sizeof(char) * PAGE_SIZE);
    memset(ph, 0, sizeof(char) * PAGE_SIZE); /* write information to pagehandle */
    int pos = 0;
    pos = integer_adding(ph, pos, 0);    /* add total record */
    pos = serialize_schema(ph, pos, schema);   
    writeBlock (0, &fh, ph); 
    appendEmptyBlock(&fh); /* append empty page if required */
    closePageFile(&fh);
    free(ph);
    return RC_OK;
}

/* Open the existing table before performing any table related operations */

RC openTable(RM_TableData *rel, char *name) 
{
    SM_FileHandle fh;
    SM_PageHandle ph;
    ph = (char *) malloc(sizeof(char) * PAGE_SIZE);
    memset(ph, 0, sizeof(char) * PAGE_SIZE);
    TableInfo *t = (TableInfo *) malloc(sizeof(TableInfo));
    Schema *schema;
    openPageFile(name, &fh); /* open the page file and read the table information */
    readBlock(0, &fh, ph);
    int pos = 0, tr = 0;
    pos = integer_getting(ph, pos, &tr);
    pos = deserialize_schema(ph, pos, &schema); /* get the position */
    t->name = (char *) malloc(sizeof(char) * strlen(name));
    strcpy(t->name, name);
    t->totalRecords = tr;
    t->recordLength = getRecordSize(schema);
    t->numSlots = slot_per_page(t->recordLength);
    t->end = 1;
    rel->name = name;
    rel->schema = schema;
    rel->mgmtData = (void *)t;
    if (rel->schema !=NULL)
	return RC_OK;
    else 
	return RC_OPEN_TABLE_FAILED;
    closePageFile(&fh);
    free(ph);
    
}

/* close the requested table and perform all write operations accordingly before close is performed */

RC closeTable(RM_TableData *rel) 
{
    SM_FileHandle fh;
    SM_PageHandle ph;
    ph = (char *) malloc(sizeof(char) * PAGE_SIZE);
    memset(ph, 0, sizeof(char) * PAGE_SIZE);
    char freePagesList[PAGE_SIZE];
    memset(freePagesList,'\0',PAGE_SIZE);
    TableInfo *t = (TableInfo *)rel->mgmtData;
    openPageFile(t->name, &fh);
    readBlock(0, &fh, ph);
    integer_adding(ph, 0, t->totalRecords); /* update number of records */
    writeBlock (0, &fh, ph);
    int x;
    char FreePageNumber[10];
    memset(FreePageNumber,'\0',10);
    char nullString[PAGE_SIZE];
    memset(nullString,'\0',PAGE_SIZE);
    for(x= 0;tombstone[x] != -99 ;x++)
    {
	strcat(freePagesList,ph);
	writeBlock(tombstone[x],&fh,nullString); /* write deleted block */
	memset(FreePageNumber,'\0',10);
    }
    closePageFile(&fh);
    free(rel->mgmtData);
    free(ph);
    return RC_OK;
}

/* deletes the existing table */

RC deleteTable(char *name) 
{
    destroyPageFile(name);
    return RC_OK;
}

/* returns the number of tuples present in the table */

int getNumTuples(RM_TableData *rel) 
{
    return ((TableInfo *) rel->mgmtData)->totalRecords;
}

/* receive all the parameters and malloc space to store schema */

Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) 
{
    Schema *schema;
    schema = (Schema *) malloc(sizeof(Schema));
    schema->numAttr = numAttr;
    schema->attrNames = attrNames;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keyAttrs = keys;
    schema->keySize = keySize;
    return schema;
}

/* free all the resources allocated respective to the schema */
RC freeSchema(Schema *schema) 
{
    free(schema->attrNames);
    free(schema->dataTypes);
    free(schema->typeLength);
    free(schema->keyAttrs);
    free(schema);
    return RC_OK;
}

/* dealing with records and attribute values */

RC createRecord(Record **record, Schema *schema) 
{
    *record = (Record *)malloc(sizeof(Record));
    (*record)->data = (char *)malloc(PAGE_SIZE);
    memset((*record)->data,'\0',PAGE_SIZE);
    if(record != NULL)
	 return RC_OK;
    else
	 return RC_CREATE_FAILED;
}

/* free the specified record */

RC freeRecord(Record *record) 
{
    free(record->data);
    free(record);
    return RC_OK;
}

/* gets the attribute values of a record */

RC getAttr(Record *record, Schema *schema, int attrNum, Value **value) 
{
    int offset = 0; 
    calculateOffset(schema, attrNum, &offset); /* to offset to the givent attribute number */
    Value *rec;
    int a;
    float f;
    char *c;
    switch (schema->dataTypes[attrNum]) 
    {
        case DT_INT :
            integer_getting(record->data, offset, &a);
            MAKE_VALUE(rec, DT_INT, a);
            break;
        case DT_STRING :
            c = (char *) malloc(sizeof(char) * schema->typeLength[attrNum]);
            string_getting(record->data, offset, c);
            MAKE_STRING_VALUE(rec, c);
            free(c);
            break;
        case DT_FLOAT :
            float_getting(record->data, offset, &f);
            MAKE_VALUE(rec, DT_FLOAT, f);
            break;
        case DT_BOOL :
            integer_getting(record->data, offset, &a);
            MAKE_VALUE(rec, DT_BOOL, a);
            break;
    }
    *value = rec;
    return RC_OK;
}

/* sets the attributes value of record */

RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
    int offset = 0; 
    calculateOffset(schema, attrNum, &offset); /* to offset to the givent attribute number */
    switch (schema->dataTypes[attrNum]) {
        case DT_INT :
            integer_adding(record->data, offset, value->v.intV);
            break;
        case DT_STRING :
            string_addition(record->data, offset, value->v.stringV);
            break;
        case DT_FLOAT :
            float_addition(record->data, offset, value->v.floatV);
            break;
        case DT_BOOL :
            integer_adding(record->data, offset, (int)(value->v.boolV));
            break;
    }
    return RC_OK;
}

// handling records in a table
RC insertRecord(RM_TableData *rel, Record *record) 
{
    int slot, size, end, newslot;
    TableInfo *t = (TableInfo *)rel->mgmtData;
    size = t->recordLength;
    end = t->end;
    slot = t->numSlots;
    char *flag = (char *) malloc(sizeof(char) * slot);
    memset(flag, 0, sizeof(char) * slot);
    SM_FileHandle fh;
    SM_PageHandle ph;
    ph = (char *) malloc(sizeof(char) * PAGE_SIZE);
    memset(ph, 0, sizeof(char) * PAGE_SIZE);
    openPageFile(t->name, &fh);
    readBlock(end, &fh, ph);
    memcpy(flag, ph, sizeof(char) * slot);
    for (newslot = slot - 1; newslot >=0; newslot--) 
    {
        if (flag[newslot] == 1) 
 	{
            break;
        }
    }
    /* if no available slot */ 
    if (newslot == slot - 1) 
    {
        end++;
        t->end = end;
        newslot = -1;
        memset(flag, 0, sizeof(char) * slot);
    }
    newslot = newslot + 1;
    flag[newslot] = 1;
    readBlock(end, &fh, ph);
    memcpy(ph, flag, sizeof(char) * slot);
    memcpy(ph + newslot * size + sizeof(char) * slot, record->data, size);
    writeBlock(end, &fh, ph);
    closePageFile(&fh);
    record->id.page = end;
    record->id.slot = newslot;
    ((TableInfo *) rel->mgmtData)->totalRecords += 1;
    free(flag);
    free(ph);
    return RC_OK;

}

/* deletes the record with a certain record id given as argument */

RC deleteRecord(RM_TableData *rel, RID id) {
    int x;
	for(x = 0;tombstone[x] != -99 ;x++)
	{
		tombstone[x] = id.page;
	}
    return RC_OK;
}

/* updates the existing record with the given value */

RC updateRecord(RM_TableData *rel, Record *record) {
    
    int slot, size, last, newslot;
    TableInfo *t = (TableInfo *)rel->mgmtData;
    size = t->recordLength;
    last = record->id.page;
    slot = t->numSlots;
    newslot = record->id.slot;
    SM_FileHandle fh;
    SM_PageHandle ph;
    ph = (char *) malloc(sizeof(char) * PAGE_SIZE);
    memset(ph, 0, sizeof(char) * PAGE_SIZE);
    openPageFile(t->name, &fh);
    readBlock(last, &fh, ph); /*reads the entire page and updates the required record*/
    memcpy(ph + newslot * size + sizeof(char) * slot, record->data, size);
    writeBlock(last, &fh, ph);
    closePageFile(&fh);
    free(ph);
    return RC_OK;
}

/* Retrieves a record with certain record id given as argument */

RC getRecord(RM_TableData *rel, RID id, Record *record)
{
    TableInfo *t = (TableInfo *)rel->mgmtData;
    SM_FileHandle fh;
    SM_PageHandle ph;
    ph = (char *) malloc(sizeof(char) * PAGE_SIZE);
    memset(ph, 0, sizeof(char) * PAGE_SIZE);
    openPageFile(t->name, &fh);
    readBlock(id.page, &fh, ph);
    closePageFile(&fh);
    /* get string and covert to records */
    memcpy(record->data, ph + sizeof(char) * t->numSlots + id.slot * t->recordLength, t->recordLength);
    record->id.page = id.page;
    record->id.slot = id.slot;
    free(ph);
    return RC_OK;
}


/* dealing with schemas */
int getRecordSize(Schema *schema) 
{
    int i, sum;
    sum = 0;
    for (i = 0; i < schema->numAttr; i++) 
    {
        switch (schema->dataTypes[i]) 
        {
            case DT_INT :
                sum += sizeof(int);
                break;
            case DT_STRING :
                /* plus a size of int means the length of string */
                sum += schema->typeLength[i] + sizeof(int);
                break;
            case DT_FLOAT :
                sum += sizeof(float);
                break;
            case DT_BOOL :
                sum += sizeof(bool);
                break;
        }
    }
    return sum;
}


/* Initializes the ScanInfo datastructure with the passed parameter values */ 
RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *condition)
{
   ScanInfo *si = (ScanInfo *) malloc(sizeof(ScanInfo));
    /* start search at page 1 */ 
    si->page = 1;
    si->slot = 0;
    si->condition = condition;
    scan->rel = rel;
    scan->mgmtData = (void *)si;
    return RC_OK;
}

/* Returns the next record value once the scan condition is fulfilled. It returns the next tuple that fullfills the scan condition */


RC next(RM_ScanHandle *scan, Record *record) 
{
    ScanInfo *si = (ScanInfo *)scan->mgmtData;
    TableInfo *m = (TableInfo *)scan->rel->mgmtData;
    Schema *sc = scan->rel->schema;
    Value *val;
    SM_FileHandle fh;
    SM_PageHandle ph;
    int tpage = fh.totalNumPages+2;
    int curp = si->page;
    int curs = si->slot;
    int slots = m->numSlots;
    int length = m->recordLength;
    char *flag = (char *) malloc(sizeof(char) * slots);
    int i;
    ph = (char *) malloc(sizeof(char) * PAGE_SIZE);
    memset(ph, 0, sizeof(char) * PAGE_SIZE);
    openPageFile(m->name, &fh);
    while (curp < tpage) 
   {
       	readBlock(curp, &fh, ph); /* read curp */
        memcpy(flag, ph, sizeof(char) * slots); 	/* get flags */
        for (i = curs; i < slots; i++) 
	{
    	    /* if flag available then */
            if (flag[i] == 1) 
	     {
                /* get string, string to record */
                memcpy(record->data, ph + sizeof(char) * slots + i * length, length);
		/* evaluate the  record to check the condition */
                evalExpr(record, sc, si->condition, &val);
                if (val->v.boolV == 1) 
		{

		    /* if this is the end of slot, then next will be a new page */
                    if (i == slots) 
		    {
                       si->page = curp + 1;
                        si->slot = 0;
                    } 
		    else 
                    { 
                        si->page = curp;
                        si->slot = i + 1;
                     }
                    closePageFile(&fh);
                    return RC_OK;
                }
                
            }
        }
    
        curs = 0;
        curp += 1;      
        break;
    }   
    closePageFile(&fh);
    free(flag);
    free(ph);
    return RC_RM_NO_MORE_TUPLES;

}

/* cleans the resources associated with the scan functions */
RC closeScan(RM_ScanHandle *scan) {
    free(scan->mgmtData);
    scan->rel = NULL;
    scan->mgmtData = NULL;
    return RC_OK;
}


