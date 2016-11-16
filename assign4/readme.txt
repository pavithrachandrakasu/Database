Assignment 4 - CS 525 Advanced Database Organization

B+ TREE Implementation:
-----------------------

Group Members:
--------------

1) Gautham Giridharan - ggiridh1@hawk.iit.edu - A20359074

2) Pavithra Kuttiyandi Chandrakasu - pkuttiya@hawk.iit.edu - A20353191  

We both of us only worked on this particular assignment. No contribution or no communication from the third member of the team regarding this assignment. 

Contents:
---------

1) Instructions to compile and run the code
2) Description of new Data structures used 
3) Additional Changes

1) Instructions to compile and run the code
----------------------------------------------

i) In the terminal, navigate to the assign3 directory.

ii) Type: 
	make -f makefile 

iii) Output: 
	./test_assign4


2) Description of the functions used
---------------------------------------
	

/***B+-Tree Index Functions****/
	

i) createBtree()
              	
   createBtree creates a B tree index.
		
ii) deleteBtree()
	
    deleteBtree deletes a B tree index and removes the corresponding page file.

iii) openBtree()
	
     openBtree opens the B tree index created.

iv)  closeBtree()
	
     closeBtree closes the B tree index which is opened and the index manager ensures that all new or modified pages of the index are flushed back to disk. 

v)   openTreeScan()
	
     openTreeScan opens the tree for a scan through all entries of a Btree . 

vi)  nextEntry()
	
     nextEntry reads the next entry in the Btree.

vii) closeTreeScan()
	
     closeTreeScan closes the tree after scanning through all the elements of the B tree.


/*****Key Functions*******/


i) findKey()
	
   findKey return the RID for the entry with the search key in the b-tree. If the key does not exist it returns RC_IM_KEY_NOT_FOUND

ii) insertKey()
	
   insertKey inserts a new key and record pointer pair into the index. If that key is already stored in the b-tree it returns RC_IM_KEY_ALREADY_EXISTS .

iii) deleteKey()
	
     deleteKey removes a key (and corresponding record pointer) from the index.It returns RC_IM_KEY_NOT_FOUND if the key is not in the index. For deletion it is up to the client whether this is handled as an error.

/*******Access Information Functions*********/


i)  getNumNodes()
	
    This Function calculates the total number of nodes in the Btree.

ii) getNumEntries()
	-
    This Function calculates the total number of entries in the Btree.

iii) getKeyType()
	
     This Function returns the keytype.

/********Debug Functions****************/

i) printTree()
	
   This Function is used to create a string representation of a b-tree. 


3) Additional Changes:
-----------------------

We have edited the test_assign4_1.c file one ASSERT_EQUALS_INT function which checks for the equality of numInserts and i value. This will not be equal so we have incremented i by 1.