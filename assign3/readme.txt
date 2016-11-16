Assignment 3 - CS 525 Advanced Database Organization

Record Manager Implementation:
-------------------------------

Group Members:
--------------

1) Gautham Giridharan - ggiridh1@hawk.iit.edu - A20359074

2) Pavithra Kuttiyandi Chandrakasu - pkuttiya@hawk.iit.edu - A20353191  

3) Hatim Shabbir Dhuliawala - hdhuliaw@hawk.iit.edu - A20351495 

Contents:
---------

1) Instructions to compile and run the code
2) Description of new Data structures used 
3) Description of additional functionalties implemented
4) Additional Test cases 
5) Additional Error Checks
6) Memory leaks Checks 

1) Instructions to compile and run the code
----------------------------------------------

i) In the terminal, navigate to the assign3 directory.

ii) Type: 
	make -f makefile1 
	make -f makefile2
	make -f makefile3 

iii) Output: 
	./recordManager1
	./recordManager2
	./recordManager3


2) Description of the functions used
---------------------------------------

***** Record Manager Interface Table and Manager ******

initRecordManager Function:
 	1)Initializes record manager and all the temporary attributes are intialized to zero. 
 
shutdownRecordManager Function:
	1)Shutdown an existing record manager.
 
createTable Function:
	1) This function creates a table.
	2) It creates the underlying page file and stores information about the schema.
	3) Assign other table attributes to the relation.  

openTable Function:
 	1) Opens the table from a page file.
	2) Deserialize the data that is read from the page file.  
 	3) Before inserting or scanning operations are performed,table should be opened 

closeTable Funtion:
	1)Closes the table.
	2)Changes are written back to the page file before closing the table.

deleteTable Function:
	1)Deletes the table from a page file.
 
getNumTuples Function:
 	1)Get the size of each tuple
	2)Returns the count of tuples in the table.


*********** Record Manager Interface Handling Record in a Table ***************

insertRecord Function:
 	1)Insert a new record in the available page and slot 
	2)Assign the inserted record with a RID i.e., (page number and slot) and the record parameter passed is updated.
 
deleteRecord Function:
 	1) Search the table for the record to be deleted and then deletes that record based on the given RID parameter.

updateRecord Function:
	1) Based on the given page number and slot number, find the record that needs to be updated. 
	2)Updates an existing record with a new value on the slot and page that has been passed.
 
getRecord Function:
 	1)Fetches an existing record value based on RID parameter 	
	2)Assings the value to record and copy that RID to record Data structure
 

**************** Record Manager Interface Scan ***************

startscan Function:
	1) We have created new Data structure ScanInfo and attribute details are passed as arguments to it. 
 	2) Initializes a scan and load the active scan to a linked list.

next Function:
	1)This function is used to find the next record that satifies the condition based on the given slot number.
	2)The given slot number is acutally get stored in ScanInfo data structure.

closeScan Function:
	1) This Function will clean up all the allocated resources with respective scan function.

***** Record Manager Interface Dealing with the Schemas ******

getRecordSize Function:

	1) Returns the Size of the records
	2) The purpose is to get the size of a record in a given schema
	3)It finds the number of attributes from the schema
  	4)Finds the datatype for each attribute and sum the size occupied by it in bytes.
 	5) Returns the total size occupied by all the attribute datatypes in byte
 
createSchema Function:
 
 	1) It creates a new schema
	2) The created schema is returned
 	3) Finds the sum of the total size occupied by the schema
	4) Calculate size to allocate a schema 
 	5) Assign values to the schema and returns the pointer. 

freeSchema Function:
 
 	1) Frees a schemas memory.
 	2) Free the space occupied by each schema.
 	


********* Record Manager Interface Dealing with the Schemas dealing with records and attribute values ****************

createRecord Function:
 	1) This function is used to create a record
	2) Allocates memory to the record based on given record size
	3) Returns status of record created.
 
freeRecord Function:
 	1)Free the memory space occupied by a record and return the status

getAttr Function:
 	1) This function returns the attribute value which is set already by setAttr function.
	2) Firstly modifies the record based on the schema and returns the record value

setAttr Function:
 	1)Get the value from the parameter to set a attribute.
	2) This function sets the values and also appends the required delimiters in between the data. 

3) Description of additional functionalties implemented
-------------------------------------------------------

1) Tombstones :
	For this implementation, we are trying to create of group of RID's of the records. Once the group is created, we will delete these
group of RID's in large while closing the table

2) Menu Based Interface:
	We have created a menu based interface for all the table operations. We can use this interface to perform each table operations like create table, open table, insert record inside a table, delete table etc., individually as required.

3) Primary key constraints:
	Before insertion of record to a table, we are checking the value of the key whether it exists in the table or not before inserting the record. It will throw an error if record exists or else it will insert the record.

4) Additional Functionalities Implemented
------------------------------------------

We have implemented three additional functionalities as follows:
	
	1) Tombstones
	2) Interactive interface - Menu Based Interface
	3) Primary key constraints check

5) Additional Test cases 
---------------------------

We have tested the newly added functionalities:
	
	1) Tombstones
	2) Interactive interface - Menu Based Interface
	3) Primary key constraints check

6) Additional Error Checks 
----------------------------

	1) RC_OPEN_TABLE_FAILED 
	2) RC_INSERT_ERROR 
	3) RC_CREATE_FAILED 
	4) RC_SET_ATTR_FAILED 

7) Memory leaks Checks 
-----------------------

	We have some many memory leaks. Some errors due to the test cases provided and some were due to written code. We tried to fix many of these memory issues existed in the test cases as well as in other provided files. For instance, in file test_assign3_1.c, freeing of records and freeing of rids were missing like freeRecord(r),free(rids),free(sc1),free(sc2) etc,. Some of them where to due to values intitalized in test file and not being freed at the end. We have added extra variables to consider these leaks as well. 

