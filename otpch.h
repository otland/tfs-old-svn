#ifdef __OTSERV_OTPCH_H__
#error "Precompiled header should be included only once."
#endif

#define __OTSERV_OTPCH_H__
#if defined __WINDOWS__ || defined WIN32
#include <winerror.h>
#endif

//libxml
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/threads.h>

//boost
#include <boost/config.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

//otserv
#include "thing.h"
