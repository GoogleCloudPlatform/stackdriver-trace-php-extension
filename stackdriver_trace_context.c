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

/*
 * This is the implementation of the Stackdriver\Trace\Context class. The PHP equivalent is:
 *
 * namespace Stackdriver\Trace;
 *
 * class Context {
 *   protected $traceId;
 *   protected $spanId;
 *
 *   public function __construct(array $contextOptions)
 *   {
 *     foreach ($contextOptions as $k => $v) {
 *       $this->__set($k, $v);
 *     }
 *   }
 *
 *   public function spanId()
 *   {
 *     return $this->spanId;
 *   }
 * *
 *   public function traceId()
 *   {
 *     return $this->traceId;
 *   }
 * }
 */

#include "stackdriver_trace_context.h"

zend_class_entry* stackdriver_trace_context_ce = NULL;

ZEND_BEGIN_ARG_INFO_EX(arginfo_StackdriverTraceContext_construct, 0, 0, 1)
	ZEND_ARG_ARRAY_INFO(0, contextOptions, 0)
ZEND_END_ARG_INFO();

/**
 * Initializer for Stackdriver\Trace\Context
 *
 * @param array $contextOptions
 */
static PHP_METHOD(StackdriverTraceContext, __construct) {
    zval *zval_context_options, *v;
    ulong idx;
    zend_string *k;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &zval_context_options) == FAILURE) {
        RETURN_FALSE;
    }

    zend_array *context_options = Z_ARR_P(zval_context_options);
    ZEND_HASH_FOREACH_KEY_VAL(context_options, idx, k, v) {
        zend_update_property(stackdriver_trace_context_ce, getThis(), ZSTR_VAL(k), strlen(ZSTR_VAL(k)), v);
    } ZEND_HASH_FOREACH_END();
}

/**
 * Fetch the span id
 *
 * @return string
 */
static PHP_METHOD(StackdriverTraceContext, spanId) {
    zval *val, rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    val = zend_read_property(stackdriver_trace_context_ce, getThis(), "spanId", sizeof("spanId") - 1, 1, &rv);

    RETURN_ZVAL(val, 1, 0);
}

/**
 * Fetch the trace id
 *
 * @return string
 */
static PHP_METHOD(StackdriverTraceContext, traceId) {
    zval *val, rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    val = zend_read_property(stackdriver_trace_context_ce, getThis(), "traceId", sizeof("traceId") - 1, 1, &rv);

    RETURN_ZVAL(val, 1, 0);
}

// Declare method entries for the Stackdriver\Trace\Context class
static zend_function_entry stackdriver_trace_context_methods[] = {
    PHP_ME(StackdriverTraceContext, __construct, arginfo_StackdriverTraceContext_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(StackdriverTraceContext, spanId, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(StackdriverTraceContext, traceId, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

// Module init handler for registering the Stackdriver\Trace\Context class
int stackdriver_trace_context_minit(INIT_FUNC_ARGS) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "Stackdriver\\Trace\\Context", stackdriver_trace_context_methods);
    stackdriver_trace_context_ce = zend_register_internal_class(&ce);

    zend_declare_property_null(stackdriver_trace_context_ce, "spanId", sizeof("spanId") - 1, ZEND_ACC_PROTECTED TSRMLS_CC);
    zend_declare_property_null(stackdriver_trace_context_ce, "traceId", sizeof("traceId") - 1, ZEND_ACC_PROTECTED TSRMLS_CC);

    return SUCCESS;
}
