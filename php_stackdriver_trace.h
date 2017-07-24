/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PHP_STACKDRIVER_H
#define PHP_STACKDRIVER_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "stackdriver_trace.h"

#define PHP_STACKDRIVER_TRACE_VERSION "0.1.1"
#define PHP_STACKDRIVER_TRACE_EXTNAME "stackdriver_trace"

#define PHP_STACKDRIVER_MAKE_STD_ZVAL(pzv) \
  pzv = (zval *)emalloc(sizeof(zval));
#define PHP_STACKDRIVER_FREE_STD_ZVAL(pzv) efree(pzv);

PHP_FUNCTION(stackdriver_trace_version);

extern zend_module_entry stackdriver_trace_module_entry;
#define phpext_stackdriver_trace_ptr &stackdriver_trace_module_entry

PHP_MINIT_FUNCTION(stackdriver_trace);
PHP_MSHUTDOWN_FUNCTION(stackdriver_trace);
PHP_RINIT_FUNCTION(stackdriver_trace);
PHP_RSHUTDOWN_FUNCTION(stackdriver_trace);

ZEND_BEGIN_MODULE_GLOBALS(stackdriver_trace)
    // map of functions we're tracing to callbacks
    HashTable *user_traced_functions;

    // Trace context
    stackdriver_trace_span_t *current_span;
    zend_string *trace_id;
    long trace_parent_span_id;

    // List of collected spans
    HashTable *spans;
ZEND_END_MODULE_GLOBALS(stackdriver_trace)

extern ZEND_DECLARE_MODULE_GLOBALS(stackdriver_trace)

#ifdef ZTS
#define STACKDRIVER_TRACE_G(v) TSRMG(stackdriver_trace_globals_id, zend_stackdriver_trace_globals *, v)
#else
#define STACKDRIVER_TRACE_G(v) (stackdriver_trace_globals.v)
#endif

#endif /* PHP_STACKDRIVER_H */
