/*************************************************************************************************
 * NVidia Corporation
 * Copyright (c) 2011
 * All rights reserved
 *************************************************************************************************
  * $Revision: #1 $
 * $Date: 2010/08/04 $
 * $Author: gheinrich $
 ************************************************************************************************/

/**
 * @addtogroup agps
 * @{
 */

/**
 * @file agps_transport.h NVidia aGPS RIL Transport layer
 *
 */

#ifndef AGPS_TRANSPORT_H
#define AGPS_TRANSPORT_H

/*************************************************************************************************
 * Project header files
 ************************************************************************************************/

#include "agps.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

/*************************************************************************************************
 * Macros
 ************************************************************************************************/

/*************************************************************************************************
 * Public Types
 ************************************************************************************************/

/*************************************************************************************************
 * Public variable declarations
 ************************************************************************************************/

/*************************************************************************************************
 * Public function declarations
 ************************************************************************************************/

/**
 * Open a device file
 *
 * @param pathname Path
 * @return int -1 on failure (and errno is set appropriately), 0
 *         or greater on success. Successfull return value known
 *         as file descriptor.
 */
int agps_TransportOpen(const char *pathname);

/**
 * Read data
 *
 * @param fd             File descriptor to read from
 * @param buffer         Pointer to buffer to read data into
 * @param len            Size of buffer pointed to by buffer
 * @param bytes_received Pointer to integer to which number of
 *                       bytes received will be written in case
 *                       of success
 * @return int 0 on success, value < 0 on failure
 */
int agps_TransportRead(int fd, char *buffer, int len);

/**
 * Close a device file that was previously opened using
 * agps_TransportOpen()
 *
 * @param fd File descriptor
 *
 * @return int -1 on failure, 0 on success and errno is set
 *         appropriately.
 */
int agps_TransportClose(int fd);

/**
 * Write to a file descriptor.
 *
 * @param fd    file descriptor get from a successfull call to
 *              fild_Open.
 * @param buf   src buffer of the write.
 * @param count length to write in bytes.
 *
 * @return int  0 on success, -1 on failure
 *         and errno is set appropriately.
 */
int agps_TransportWrite(int fd, const char *buf, int count);


#endif /* AGPS_TRANSPORT_H */

