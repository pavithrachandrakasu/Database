#include <stdlib.h>
#include "dberror.h"
#include "expr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"
#include "storage_mgr.h"
#include "tables.h"
#include "test_helper.h"

#define ASSERT_EQUALS_RECORDS(_l,_r, schema, message)			\
  do {									\
    Record *_lR = _l;   						 \
    Record *_rR = _r;                                                    \
    ASSERT_TRUE(memcmp(_lR->data,_rR->data,getRecordSize(schema)) == 0, message); \
    int i;							\
    for(i = 0; i < schema->numAttr; i++)				\
      {										\
        Value *lVal, *rVal;          	                                   \
		char *lSer, *rSer; 					\
        getAttr(_lR, schema, i, &lVal); 	                                 \
        getAttr(_rR, schema, i, &rVal);                                  \
		lSer = serializeValue(lVal); \
		rSer = serializeValue(rVal); \
        ASSERT_EQUALS_STRING(lSer, rSer, "attr same");	\
		free(lVal); \
		free(rVal); \
		free(lSer); \
		free(rSer); \
      }									\
  } while(0)

/* test methods */
static void testPrimaryKey(void);


typedef struct TestRecord {
	int a;
	char *b;
	int c;
} TestRecord;


Record *testRecord(Schema *schema, int a, char *b, int c);
Schema *testSchema(void);
Record *fromTestRecord(Schema *schema, TestRecord in);


char *testName;
char *table_name_temp;

int main(void) {
	testName = "";

	testPrimaryKey();

	return 0;
}


void testPrimaryKey(void) {
RM_TableData *table = (RM_TableData *) malloc(sizeof(RM_TableData));
  TestRecord inserts[] = { 
    {1, "aaaa", 3}, 
    {2, "bbbb", 2},
    {3, "cccc", 1},
    {4, "dddd", 3},
    {1, "eeee", 5},
    {2, "ffff", 1},
    {3, "gggg", 3},
    {4, "hhhh", 3},
    {9, "iiii", 2},
    {10, "jjjj", 5},
  };
  TestRecord newTest[] = {
    {1, "aaaa", 3}, 
    {2, "bbbb", 2},
    {3, "cccc", 1},
    {4, "dddd", 3},
  };
  int numInserts = 10, numKey = 4, i,j,primaryFlag=0,inc=0;
  Record *r;
  RID *rids;
  Schema *schema;
  testName = "test primary key check";
  schema = testSchema();
  rids = (RID *) malloc(sizeof(RID) * numKey);
  
  TEST_CHECK(initRecordManager(NULL));
  TEST_CHECK(createTable("test_table_r",schema));
  TEST_CHECK(openTable(table, "test_table_r"));
  
  for(i = 0; i < numInserts; i++)
    {
	if(i>0){
	primaryFlag =0;
	for(j=i-1;j>=0;j--){
	if(inserts[i].a == inserts[j].a)
       { 
	primaryFlag=1;
        break;
	}
	}
	}
	if(primaryFlag!=1){

    r = fromTestRecord(schema, inserts[i]);  
    TEST_CHECK(insertRecord(table,r)); 
    rids[inc] = r->id;
    inc++;
	}
    }
  TEST_CHECK(closeTable(table));
  TEST_CHECK(openTable(table, "test_table_r"));

  for(i = 0; i < numKey; i++)
    {
      RID rid = rids[i];
      TEST_CHECK(getRecord(table, rid, r));
      ASSERT_EQUALS_RECORDS(fromTestRecord(schema, newTest[i]), r, schema, "compare records");
    }

  TEST_CHECK(closeTable(table));
  TEST_CHECK(deleteTable("test_table_r"));
  TEST_CHECK(shutdownRecordManager());

  free(table);
  TEST_DONE();

}

Schema *
testSchema (void)
{
  Schema *result;
  char *names[] = { "a", "b", "c" };
  DataType dt[] = { DT_INT, DT_STRING, DT_INT };
  int sizes[] = { 0, 4, 0 };
  int keys[] = {0};
  int i;
  char **cpNames = (char **) malloc(sizeof(char*) * 3);
  DataType *cpDt = (DataType *) malloc(sizeof(DataType) * 3);
  int *cpSizes = (int *) malloc(sizeof(int) * 3);
  int *cpKeys = (int *) malloc(sizeof(int));

  for(i = 0; i < 3; i++)
    {
      cpNames[i] = (char *) malloc(2);
      strcpy(cpNames[i], names[i]);
    }
  memcpy(cpDt, dt, sizeof(DataType) * 3);
  memcpy(cpSizes, sizes, sizeof(int) * 3);
  memcpy(cpKeys, keys, sizeof(int));

  result = createSchema(3, cpNames, cpDt, cpSizes, 1, cpKeys);

  return result;
}

Record *
fromTestRecord (Schema *schema, TestRecord in)
{
  return testRecord(schema, in.a, in.b, in.c);
}

Record *
testRecord(Schema *schema, int a, char *b, int c)
{
  Value *value;
  Record *result;
  TEST_CHECK(createRecord(&result, schema));
  MAKE_VALUE(value, DT_INT, a);
  TEST_CHECK(setAttr(result, schema, 0, value));
  freeVal(value);
  MAKE_STRING_VALUE(value, b);
  TEST_CHECK(setAttr(result, schema, 1, value));
  freeVal(value);
  MAKE_VALUE(value, DT_INT, c);
  TEST_CHECK(setAttr(result, schema, 2, value));
  freeVal(value);

  return result;
}
