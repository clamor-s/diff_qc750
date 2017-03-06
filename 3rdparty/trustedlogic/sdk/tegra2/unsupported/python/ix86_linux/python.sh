#!/bin/sh

#
# Copyright (c) 2005-2008 Trusted Logic S.A.
# All Rights Reserved.
#
# This software is the confidential and proprietary information of
# Trusted Logic S.A. ("Confidential Information"). You shall not
# disclose such Confidential Information and shall use it only in
# accordance with the terms of the license agreement you entered
# into with Trusted Logic S.A.
#
# TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
# SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
# NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
# MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
#

SCRIPT_FILENAME=`which $0`
#SCRIPT_DIR=`dirname $SCRIPT_FILENAME`
SCRIPT_DIR=`dirname $0`

export PYTHONHOME=$SCRIPT_DIR
export PYTHONPATH=$PYTHONHOME/../lib/python25.zip
chmod -R 777 $PYTHONHOME
$PYTHONHOME/python $@
 