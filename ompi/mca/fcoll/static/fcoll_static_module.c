/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008-2011 University of Houston. All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"
#include "fcoll_static.h"

#include <stdio.h>

#include "mpi.h"
#include "ompi/mca/fcoll/fcoll.h"
#include "ompi/mca/fcoll/base/base.h"


/*
 * *******************************************************************
 * ************************ actions structure ************************
 * *******************************************************************
 */
static mca_fcoll_base_module_1_0_0_t static_t =  {
    mca_fcoll_static_module_init,
    mca_fcoll_static_module_finalize,
    mca_fcoll_static_file_read_all,
    mca_fcoll_static_file_read_all_begin,
    mca_fcoll_static_file_read_all_end,
    mca_fcoll_static_file_write_all,
    mca_fcoll_static_file_write_all_begin,
    mca_fcoll_static_file_write_all_end
};

int
mca_fcoll_static_component_init_query(bool enable_progress_threads,
                                      bool enable_mpi_threads)
{
    /* Nothing to do */

    return OMPI_SUCCESS;
}

mca_fcoll_base_module_1_0_0_t *
mca_fcoll_static_component_file_query (mca_io_ompio_file_t *fh, int *priority)
{
    *priority = mca_fcoll_static_priority;
    if (0 >= mca_fcoll_static_priority) {
        return NULL;
    }

    if (mca_fcoll_base_query_table (fh, "static")) {
        if (*priority < 50) {
            *priority = 50;
        }
    }

    return &static_t;
}

int mca_fcoll_static_component_file_unquery (mca_io_ompio_file_t *file)
{    
   /* This function might be needed for some purposes later. for now it
    * does not have anything to do since there are no steps which need 
    * to be undone if this module is not selected */

   return OMPI_SUCCESS;
}

int mca_fcoll_static_module_init (mca_io_ompio_file_t *file)
{
    return OMPI_SUCCESS;
}

   
int mca_fcoll_static_module_finalize (mca_io_ompio_file_t *file) 
{
    return OMPI_SUCCESS;
}
