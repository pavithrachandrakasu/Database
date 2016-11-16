#ifndef CONTEST_H
#define CONTEST_H
#include "btree_mgr.h"
#include "tables.h"
#include "storage_mgr.h"
#include "record_mgr.h"
#include <stdlib.h>
#include <string.h>
#include "buffer_mgr.h"
#include "list.h"
#include "dberror.h"

extern RC setUpContest (int numPages);
extern RC shutdownContest (void);

extern long getContestIOs (void);

#endif
