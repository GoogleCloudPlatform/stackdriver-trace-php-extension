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
 * This is the implementation of the Stackdriver\Trace\Span class. The PHP
 * equivalent is:
 *
 * namespace Stackdriver\Trace;
 *
 * class Span {
 *   protected $name = "unknown";
 *   protected $spanId;
 *   protected $parentSpanId;
 *   protected $startTime;
 *   protected $endTime;
 *   protected $labels;
 *
 *   public function __construct(array $spanOptions)
 *   {
 *     foreach ($spanOptions as $k => $v) {
 *       $this->__set($k, $v);
 *     }
 *   }
 *
 *   public function name()
 *   {
 *     return $this->name;
 *   }
 *
 *   public function spanId()
 *   {
 *     return $this->spanId;
 *   }
 *
 *   public function spanId()
 *   {
 *     return $this->parentSpanId;
 *   }
 *
 *   public function startTime()
 *   {
 *     return $this->startTime;
 *   }
 *
 *   public function endTime()
 *   {
 *     return $this->endTime;
 *   }
 *
 *   public function labels()
 *   {
 *     return $this->labels;
 *   }
 * }
 */

#include "stackdriver_trace_span.h"

zend_class_entry* stackdriver_trace_span_ce = NULL;

ZEND_BEGIN_ARG_INFO_EX(arginfo_StackdriverTraceSpan_construct, 0, 0, 1)
	ZEND_ARG_ARRAY_INFO(0, spanOptions, 0)
ZEND_END_ARG_INFO();

/**
 * Initializer for Stackdriver\Trace\Span
 *
 * @param array $spanOptions
 */
static PHP_METHOD(StackdriverTraceSpan, __construct) {
    zval *zval_span_options, *v;
    ulong idx;
    zend_string *k;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a",
        &zval_span_options) == FAILURE) {
        return;
    }

    zend_array *span_options = Z_ARR_P(zval_span_options);
    ZEND_HASH_FOREACH_KEY_VAL(span_options, idx, k, v) {
        zend_update_property(stackdriver_trace_span_ce, getThis(), ZSTR_VAL(k),
                             strlen(ZSTR_VAL(k)), v);
    } ZEND_HASH_FOREACH_END();
}

/**
 * Fetch the span name
 *
 * @return string
 */
static PHP_METHOD(StackdriverTraceSpan, name) {
    zval *val, rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    val = zend_read_property(stackdriver_trace_span_ce, getThis(), "name",
                             sizeof("name") - 1, 1, &rv);

    RETURN_ZVAL(val, 1, 0);
}

/**
 * Fetch the span id
 *
 * @return string
 */
static PHP_METHOD(StackdriverTraceSpan, spanId) {
    zval *val, rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    val = zend_read_property(stackdriver_trace_span_ce, getThis(), "spanId",
                             sizeof("spanId") - 1, 1, &rv);

    RETURN_ZVAL(val, 1, 0);
}

/**
 * Fetch the parent span id
 *
 * @return string
 */
static PHP_METHOD(StackdriverTraceSpan, parentSpanId) {
    zval *val, rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    val = zend_read_property(stackdriver_trace_span_ce, getThis(),
                             "parentSpanId", sizeof("parentSpanId") - 1, 1,
                             &rv);

    RETURN_ZVAL(val, 1, 0);
}

/**
 * Fetch the labels for the span
 *
 * @return array
 */
static PHP_METHOD(StackdriverTraceSpan, labels) {
    zval *val, rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    val = zend_read_property(stackdriver_trace_span_ce, getThis(), "labels",
                             sizeof("labels") - 1, 1, &rv);

    RETURN_ZVAL(val, 1, 0);
}

/**
 * Fetch the start time
 *
 * @return float
 */
static PHP_METHOD(StackdriverTraceSpan, startTime) {
    zval *val, rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    val = zend_read_property(stackdriver_trace_span_ce, getThis(), "startTime",
                             sizeof("startTime") - 1, 1, &rv);

    RETURN_ZVAL(val, 1, 0);
}

/**
 * Fetch the end time
 *
 * @return float
 */
static PHP_METHOD(StackdriverTraceSpan, endTime) {
    zval *val, rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    val = zend_read_property(stackdriver_trace_span_ce, getThis(), "endTime",
                             sizeof("endTime") - 1, 1, &rv);

    RETURN_ZVAL(val, 1, 0);
}

/* Declare method entries for the Stackdriver\Trace\Span class */
static zend_function_entry stackdriver_trace_span_methods[] = {
    PHP_ME(StackdriverTraceSpan, __construct,
           arginfo_StackdriverTraceSpan_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(StackdriverTraceSpan, name, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(StackdriverTraceSpan, spanId, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(StackdriverTraceSpan, parentSpanId, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(StackdriverTraceSpan, labels, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(StackdriverTraceSpan, startTime, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(StackdriverTraceSpan, endTime, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/* Module init handler for registering the Stackdriver\Trace\Span class */
int stackdriver_trace_span_minit(INIT_FUNC_ARGS) {
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "Stackdriver\\Trace\\Span",
                     stackdriver_trace_span_methods);
    stackdriver_trace_span_ce = zend_register_internal_class(&ce);

    zend_declare_property_string(stackdriver_trace_span_ce, "name",
                                 sizeof("name") - 1, "unknown",
                                 ZEND_ACC_PROTECTED TSRMLS_CC);
    zend_declare_property_null(stackdriver_trace_span_ce, "spanId",
                               sizeof("spanId") - 1, ZEND_ACC_PROTECTED
                               TSRMLS_CC);
    zend_declare_property_null(stackdriver_trace_span_ce, "parentSpanId",
                               sizeof("parentSpanId") - 1, ZEND_ACC_PROTECTED
                               TSRMLS_CC);
    zend_declare_property_null(stackdriver_trace_span_ce, "startTime",
                               sizeof("startTime") - 1, ZEND_ACC_PROTECTED
                               TSRMLS_CC);
    zend_declare_property_null(stackdriver_trace_span_ce, "endTime",
                               sizeof("endTime") - 1, ZEND_ACC_PROTECTED
                               TSRMLS_CC);
    zend_declare_property_null(stackdriver_trace_span_ce, "labels",
                               sizeof("labels") - 1, ZEND_ACC_PROTECTED
                               TSRMLS_CC);

    return SUCCESS;
}

/**
 * Returns an allocated initialized pointer to a stackdriver_trace_span_t struct
 * Note that you will have to call stackdriver_trace_span_free yourself when
 * it's time to clean up the memory
 */
stackdriver_trace_span_t *stackdriver_trace_span_alloc()
{
    stackdriver_trace_span_t *span = emalloc(sizeof(stackdriver_trace_span_t));
    span->name = NULL;
    span->parent = NULL;
    span->span_id = 0;
    span->start = 0;
    span->stop = 0;
    ALLOC_HASHTABLE(span->labels);
    zend_hash_init(span->labels, 4, NULL, ZVAL_PTR_DTOR, 0);
    return span;
}

/**
 * Frees the memory allocated for this stackdriver_trace_span_t struct and any
 * other allocated objects. For every call to stackdriver_trace_span_alloc(),
 * we should be calling stackdriver_trace_span_free()
 */
void stackdriver_trace_span_free(stackdriver_trace_span_t *span)
{
    /* clear any allocated labels */
    FREE_HASHTABLE(span->labels);
    if (span->name) {
        zend_string_release(span->name);
    }

    /* free the trace span */
    efree(span);
}

/* Add a label to the trace span struct */
int stackdriver_trace_span_add_label(stackdriver_trace_span_t *span,
                                     zend_string *k, zend_string *v)
{
    /* put the string value into a zval and save it in the HashTable */
    zval zv;
    ZVAL_STRING(&zv, ZSTR_VAL(v));

    if (zend_hash_update(span->labels, zend_string_copy(k), &zv) == NULL) {
        return FAILURE;
    } else {
        return SUCCESS;
    }
}

/* Add a single label to the provided trace span struct */
int stackdriver_trace_span_add_label_str(stackdriver_trace_span_t *span,
                                         char *k, zend_string *v)
{
    return stackdriver_trace_span_add_label(span,
                                            zend_string_init(k, strlen(k), 0),
                                            v);
}

/* Update the provided span with the provided zval (array) of span options */
int stackdriver_trace_span_apply_span_options(stackdriver_trace_span_t *span,
                                              zval *span_options)
{
    zend_string *k;
    zval *v;

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARR_P(span_options), k, v) {
        if (strcmp(ZSTR_VAL(k), "labels") == 0) {
            zend_hash_merge(span->labels, Z_ARRVAL_P(v), zval_add_ref, 0);
        } else if (strcmp(ZSTR_VAL(k), "startTime") == 0) {
            span->start = Z_DVAL_P(v);
        } else if (strcmp(ZSTR_VAL(k), "name") == 0) {
            span->name = zend_string_copy(Z_STR_P(v));
        }
    } ZEND_HASH_FOREACH_END();
    return SUCCESS;
}
