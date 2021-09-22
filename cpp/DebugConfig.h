#ifndef _DEBUG_CONFIG_H
#define _DEBUG_CONFIG_H

#include "ubiquitous/ConfigHelper.h"
#include "ubiquitous/PrintfWriter.h"

TRACE_WRITER(pet::PrintfWriter)
GLOBAL_TRACE_POLICY(All)

#endif