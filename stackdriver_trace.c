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

#include "php_stackdriver_trace.h"
#include "stackdriver_trace.h"
#include "Zend/zend_compile.h"
#include "Zend/zend_closures.h"
#include "zend_extensions.h"

#if PHP_VERSION_ID < 70100
#include "standard/php_rand.h"
#endif

#ifdef _WIN32
#include "win32/time.h"
#else
#include <sys/time.h>
#endif

// True global for storing the original zend_execute_ex function pointer
void (*original_zend_execute_ex) (zend_execute_data *execute_data);
void (*original_zend_execute_internal) (zend_execute_data *execute_data, zval *return_value);

ZEND_DECLARE_MODULE_GLOBALS(stackdriver_trace)

ZEND_BEGIN_ARG_INFO_EX(arginfo_stackdriver_trace_function, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, functionName, IS_STRING, 0)
    ZEND_ARG_INFO(0, handler)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_stackdriver_trace_method, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, className, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, methodName, IS_STRING, 0)
    ZEND_ARG_INFO(0, handler)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_stackdriver_trace_begin, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, spanName, IS_STRING, 0)
    ZEND_ARG_ARRAY_INFO(0, spanOptions, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_stackdriver_trace_set_context, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, traceId, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, parentSpanId, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_stackdriver_trace_add_label, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

// List of functions provided by this extension
static zend_function_entry stackdriver_trace_functions[] = {
    PHP_FE(stackdriver_trace_version, NULL)
    PHP_FE(stackdriver_trace_function, arginfo_stackdriver_trace_function)
    PHP_FE(stackdriver_trace_method, arginfo_stackdriver_trace_method)
    PHP_FE(stackdriver_trace_list, NULL)
    PHP_FE(stackdriver_trace_begin, arginfo_stackdriver_trace_begin)
    PHP_FE(stackdriver_trace_finish, NULL)
    PHP_FE(stackdriver_trace_clear, NULL)
    PHP_FE(stackdriver_trace_set_context, arginfo_stackdriver_trace_set_context)
    PHP_FE(stackdriver_trace_context, NULL)
    PHP_FE(stackdriver_trace_add_label, arginfo_stackdriver_trace_add_label)
    PHP_FE(stackdriver_trace_add_root_label, arginfo_stackdriver_trace_add_label)
    PHP_FE_END
};

// Registers the lifecycle hooks for this extension
zend_module_entry stackdriver_trace_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_STACKDRIVER_TRACE_EXTNAME,
    stackdriver_trace_functions,
    PHP_MINIT(stackdriver_trace),
    PHP_MSHUTDOWN(stackdriver_trace),
    PHP_RINIT(stackdriver_trace),
    PHP_RSHUTDOWN(stackdriver_trace),
    NULL, // name of the MINFO function or NULL if not applicable
    PHP_STACKDRIVER_TRACE_VERSION,
    STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(stackdriver_trace)

/**
 * Return the current version of the stackdriver_trace extension
 *
 * @return string
 */
PHP_FUNCTION(stackdriver_trace_version)
{
    RETURN_STRING(PHP_STACKDRIVER_TRACE_VERSION);
}

/**
 * Add a label to the current trace span
 *
 * @param string $key
 * @param string $value
 * @return bool
 */
PHP_FUNCTION(stackdriver_trace_add_label)
{
    zend_string *k, *v;
    stackdriver_trace_span_t *span;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "SS", &k, &v) == FAILURE) {
        RETURN_FALSE;
    }

    if ((span = STACKDRIVER_TRACE_G(current_span)) == NULL) {
        RETURN_FALSE;
    }

    if (stackdriver_trace_span_add_label(span, k, v) == SUCCESS) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}

/**
 * Add a label to the root trace span
 *
 * @param string $key
 * @param string $value
 * @return bool
 */
PHP_FUNCTION(stackdriver_trace_add_root_label)
{
    zend_string *k, *v;
    stackdriver_trace_span_t *span;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "SS", &k, &v) == FAILURE) {
        RETURN_FALSE;
    }

    if (STACKDRIVER_TRACE_G(spans)->nNumUsed == 0) {
        RETURN_FALSE;
    }

    // fetch the first span
    span = Z_PTR(STACKDRIVER_TRACE_G(spans)->arData->val);

    if (stackdriver_trace_span_add_label(span, k, v) == SUCCESS) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}

// Return the current timestamp as a double
static double stackdriver_trace_now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (double) (tv.tv_sec + tv.tv_usec / 1000000.00);
}

// Call the provided Closure with the provided parameters to the traced function
static int stackdriver_trace_zend_fcall_closure(zend_execute_data *execute_data, stackdriver_trace_span_t *span, zval *closure, zval *closure_result TSRMLS_DC)
{
    int i, num_args = EX_NUM_ARGS(), has_scope = 0;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    zval *args = emalloc((num_args + 1) * sizeof(zval));

    if (getThis() == NULL) {
        ZVAL_NULL(&args[0]);
    } else {
        has_scope = 1;
        ZVAL_ZVAL(&args[0], getThis(), 0, 1);
    }

    for (i = 0; i < num_args; i++) {
        ZVAL_ZVAL(&args[i + has_scope], EX_VAR_NUM(i), 0, 1);
    }

    if (zend_fcall_info_init(
            closure,
            0,
            &fci,
            &fcc,
            NULL,
            NULL
            TSRMLS_CC
        ) != SUCCESS) {
        efree(args);
        return FAILURE;
    };

    ZVAL_NULL(closure_result);

    fci.retval = closure_result;
    fci.params = &args[0];
    fci.param_count = num_args + has_scope;

    fcc.initialized = 1;

    if (zend_call_function(&fci, &fcc TSRMLS_CC) != SUCCESS) {
        efree(args);
        return FAILURE;
    }
    efree(args);

    if (EG(exception) != NULL) {
        return FAILURE;
    }

    if (Z_TYPE_P(closure_result) != IS_ARRAY) {
        // only raise the warning if the closure succeeded
        php_error_docref(NULL, E_WARNING, "Trace callback should return array");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * Handle the callback for the traced method depending on the type
 * - if the zval is an array, then assume it's the trace span initialization options
 * - if the zval is a Closure, then execute the closure and take the results as
 *   the trace span initialization options
 */
static void stackdriver_trace_execute_callback(stackdriver_trace_span_t *span, zend_execute_data *execute_data, zval *span_options TSRMLS_DC)
{
    if (Z_TYPE_P(span_options) == IS_ARRAY) {
        stackdriver_trace_span_apply_span_options(span, span_options);
    } else if ( (Z_TYPE_P(span_options) == IS_OBJECT) &&
                (Z_OBJCE_P(span_options) == zend_ce_closure)) {
        zval closure_result;
        if (stackdriver_trace_zend_fcall_closure(execute_data, span, span_options, &closure_result TSRMLS_CC) == SUCCESS) {
            stackdriver_trace_span_apply_span_options(span, &closure_result);
        }
    }
}

// Start a new trace span. Inherit the parent span id from the curernt trace context.
static stackdriver_trace_span_t *stackdriver_trace_begin(zend_string *function_name, zend_execute_data *execute_data TSRMLS_DC)
{
    stackdriver_trace_span_t *span = stackdriver_trace_span_alloc();

    span->start = stackdriver_trace_now();
    span->name = zend_string_copy(function_name);

#if PHP_VERSION_ID < 70100
    if (!BG(mt_rand_is_seeded)) {
        php_mt_srand(GENERATE_SEED());
    }
#endif
    span->span_id = php_mt_rand();

    if (STACKDRIVER_TRACE_G(current_span)) {
        span->parent = STACKDRIVER_TRACE_G(current_span);
    }

    STACKDRIVER_TRACE_G(current_span) = span;
    zval ptr;
    ZVAL_PTR(&ptr, span);

    // add the span to the list of spans
    zend_hash_next_index_insert(STACKDRIVER_TRACE_G(spans), &ptr);

    return span;
}

// Finish the current trace span. Set the new current trace span to this span's parent if there is one.
static int stackdriver_trace_finish()
{
    stackdriver_trace_span_t *span = STACKDRIVER_TRACE_G(current_span);

    if (!span) {
        return FAILURE;
    }

    // set current time for now
    span->stop = stackdriver_trace_now();

    STACKDRIVER_TRACE_G(current_span) = span->parent;

    return SUCCESS;
}

// Given a class name and a function name, return a new string that represents the function name
static zend_string *stackdriver_trace_generate_class_name(zend_string *class_name, zend_string *function_name)
{
    int len = class_name->len + function_name->len + 2;
    zend_string *result = zend_string_alloc(len, 0);

    strcpy(ZSTR_VAL(result), class_name->val);
    strcat(ZSTR_VAL(result), "::");
    strcat(ZSTR_VAL(result), function_name->val);
    return result;
}

// Prepend the name of the scope class to the function name
static zend_string *stackdriver_trace_add_scope_name(zend_string *function_name, zend_class_entry *scope)
{
    zend_string *result;
    if (!function_name) {
        return NULL;
    }

    if (scope) {
        result = stackdriver_trace_generate_class_name(scope->name, function_name);
    } else {
        result = zend_string_copy(function_name);
    }
    return result;
}

/**
 * Start a new trace span
 *
 * @param string $spanName
 * @param array $spanOptions
 * @return bool
 */
PHP_FUNCTION(stackdriver_trace_begin)
{
    zend_string *function_name;
    zval *span_options = NULL, default_span_options;
    stackdriver_trace_span_t *span;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S|a", &function_name, &span_options) == FAILURE) {
        RETURN_FALSE;
    }

    if (span_options == NULL) {
        array_init(&default_span_options);
        span_options = &default_span_options;
    }

    span = stackdriver_trace_begin(function_name, execute_data TSRMLS_CC);
    stackdriver_trace_execute_callback(span, execute_data, span_options TSRMLS_CC);
    RETURN_TRUE;
}

/**
 * Finish the current trace span
 *
 * @return bool
 */
PHP_FUNCTION(stackdriver_trace_finish)
{
    if (stackdriver_trace_finish() == SUCCESS) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

// Reset the list of spans and free any allocated memory used
static void stackdriver_trace_clear(int reset TSRMLS_DC)
{
    stackdriver_trace_span_t *span;

    // free memory for all captured spans
    ZEND_HASH_FOREACH_PTR(STACKDRIVER_TRACE_G(spans), span) {
        stackdriver_trace_span_free(span);
    } ZEND_HASH_FOREACH_END();

    // free the hashtable
    FREE_HASHTABLE(STACKDRIVER_TRACE_G(spans));

    // reallocate and setup the hashtable for captured spans
    if (reset) {
        ALLOC_HASHTABLE(STACKDRIVER_TRACE_G(spans));
        zend_hash_init(STACKDRIVER_TRACE_G(spans), 16, NULL, ZVAL_PTR_DTOR, 0);
    }

    STACKDRIVER_TRACE_G(current_span) = NULL;
    STACKDRIVER_TRACE_G(trace_id) = NULL;
    STACKDRIVER_TRACE_G(trace_parent_span_id) = 0;
}

/**
 * Reset the list of spans
 *
 * @return bool
 */
PHP_FUNCTION(stackdriver_trace_clear)
{
    stackdriver_trace_clear(1 TSRMLS_CC);
    RETURN_TRUE;
}

/**
 * Set the initial trace context
 *
 * @param string $traceId
 * @param string $parentSpanId
 */
PHP_FUNCTION(stackdriver_trace_set_context)
{
    zend_string *trace_id;
    long parent_span_id;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S|L", &trace_id, &parent_span_id) == FAILURE) {
        RETURN_FALSE;
    }

    STACKDRIVER_TRACE_G(trace_id) = zend_string_copy(trace_id);
    STACKDRIVER_TRACE_G(trace_parent_span_id) = parent_span_id;

    RETURN_TRUE;
}

/**
 * Return the current trace context
 *
 * @return Stackdriver\Trace\Context
 */
PHP_FUNCTION(stackdriver_trace_context)
{
    stackdriver_trace_span_t *span = STACKDRIVER_TRACE_G(current_span);
    object_init_ex(return_value, stackdriver_trace_context_ce);

    if (span) {
        zend_update_property_long(stackdriver_trace_context_ce, return_value, "spanId", sizeof("spanId") - 1, span->span_id);
    } else if (STACKDRIVER_TRACE_G(trace_parent_span_id)) {
        zend_update_property_long(stackdriver_trace_context_ce, return_value, "spanId", sizeof("spanId") - 1, STACKDRIVER_TRACE_G(trace_parent_span_id));
    }
    if (STACKDRIVER_TRACE_G(trace_id)) {
        zend_update_property_str(stackdriver_trace_context_ce, return_value, "traceId", sizeof("traceId") - 1, STACKDRIVER_TRACE_G(trace_id));
    }
}

/**
 * This method replaces the internal zend_execute_ex method used to dispatch calls
 * to user space code. The original zend_execute_ex method is moved to
 * original_zend_execute_ex
 */
void stackdriver_trace_execute_ex (zend_execute_data *execute_data TSRMLS_DC) {
    zend_string *function_name = stackdriver_trace_add_scope_name(
        EG(current_execute_data)->func->common.function_name,
        EG(current_execute_data)->func->common.scope
    );
    zval *trace_handler;
    stackdriver_trace_span_t *span;

    if (function_name) {
        trace_handler = zend_hash_find(STACKDRIVER_TRACE_G(user_traced_functions), function_name);

        if (trace_handler != NULL) {
            span = stackdriver_trace_begin(function_name, execute_data TSRMLS_CC);
            original_zend_execute_ex(execute_data TSRMLS_CC);
            stackdriver_trace_execute_callback(span, execute_data, trace_handler TSRMLS_CC);
            stackdriver_trace_finish();
        } else {
            original_zend_execute_ex(execute_data TSRMLS_CC);
        }
        zend_string_release(function_name);
    } else {
        original_zend_execute_ex(execute_data TSRMLS_CC);
    }
}

/**
 * This method resumes the internal function execution.
 */
static void resume_execute_internal(INTERNAL_FUNCTION_PARAMETERS)
{
    if (original_zend_execute_internal) {
        original_zend_execute_internal(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    } else {
        execute_data->func->internal_function.handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    }
}

/**
 * This method replaces the internal zend_execute_internal method used to dispatch calls
 * to internal code. The original zend_execute_internal method is moved to
 * original_zend_execute_internal
 */
void stackdriver_trace_execute_internal(INTERNAL_FUNCTION_PARAMETERS)
{
    zend_string *function_name = stackdriver_trace_add_scope_name(
        execute_data->func->internal_function.function_name,
        execute_data->func->internal_function.scope
    );
    zval *trace_handler;
    stackdriver_trace_span_t *span;

    if (function_name) {
        trace_handler = zend_hash_find(STACKDRIVER_TRACE_G(user_traced_functions), function_name);

        if (trace_handler) {
            span = stackdriver_trace_begin(function_name, execute_data TSRMLS_CC);
            resume_execute_internal(INTERNAL_FUNCTION_PARAM_PASSTHRU);
            stackdriver_trace_execute_callback(span, execute_data, trace_handler TSRMLS_CC);
            stackdriver_trace_finish();
        } else {
            resume_execute_internal(INTERNAL_FUNCTION_PARAM_PASSTHRU);
        }
        // zend_string_release(function_name);
    } else {
        resume_execute_internal(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    }
}

/**
 * Register the provided function for tracing.
 *
 * @param string $functionName
 * @param array|Closure $handler
 * @return bool
 */
PHP_FUNCTION(stackdriver_trace_function)
{
    zend_string *function_name;
    zval *handler, *copy;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S|z", &function_name, &handler) == FAILURE) {
        RETURN_FALSE;
    }

    if (handler == NULL) {
        zval h;
        ZVAL_LONG(&h, 1);
        handler = &h;
    }

    // Note: this is freed in the RSHUTDOWN
    PHP_STACKDRIVER_MAKE_STD_ZVAL(copy);
    ZVAL_ZVAL(copy, handler, 1, 0);

    zend_hash_update(STACKDRIVER_TRACE_G(user_traced_functions), function_name, copy);
    RETURN_TRUE;
}

/**
 * Register the provided function for tracing.
 *
 * @param string $className
 * @param string $methodName
 * @param array|Closure $handler
 * @return bool
 */
PHP_FUNCTION(stackdriver_trace_method)
{
    zend_string *class_name, *function_name, *key;
    zval *handler, *copy;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "SS|z", &class_name, &function_name, &handler) == FAILURE) {
        RETURN_FALSE;
    }

    if (handler == NULL) {
        zval h;
        ZVAL_LONG(&h, 1);
        handler = &h;
    }

    // Note: this is freed in the RSHUTDOWN
    PHP_STACKDRIVER_MAKE_STD_ZVAL(copy);
    ZVAL_ZVAL(copy, handler, 1, 0);

    key = stackdriver_trace_generate_class_name(class_name, function_name);
    zend_hash_update(STACKDRIVER_TRACE_G(user_traced_functions), key, handler);

    RETURN_FALSE;
}

// Given a HashTable of labels, write the values into the provided pointer of the label_array
static int stackdriver_labels_to_zval_array(HashTable *ht, zval *label_array)
{
    ulong idx;
    zend_string *k;
    zval *v;
    HashTable *label_ht;

    array_init(label_array);
    label_ht = Z_ARRVAL_P(label_array);

    ZEND_HASH_FOREACH_KEY_VAL(ht, idx, k, v) {
        if (add_assoc_zval(label_array, ZSTR_VAL(k), v) != SUCCESS) {
            php_prinf("failed to add_assoc_zval\n");
            return FAILURE;
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}

/**
 * Return the collected list of trace spans that have been collected for this request
 *
 * @return Stackdriver\Trace\Span[]
 */
PHP_FUNCTION(stackdriver_trace_list)
{
    int i = 0;
    stackdriver_trace_span_t *trace_span;
    int num_spans = STACKDRIVER_TRACE_G(spans)->nNumUsed;
    zval *labels = emalloc(num_spans * sizeof(zval));
    zval *spans = emalloc(num_spans * sizeof(zval));

    // Set up return value to be an array of size num_spans
    array_init(return_value);

    ZEND_HASH_FOREACH_PTR(STACKDRIVER_TRACE_G(spans), trace_span) {
        object_init_ex(&spans[i], stackdriver_trace_span_ce);
        zend_update_property_long(stackdriver_trace_span_ce, &spans[i], "spanId", sizeof("spanId") - 1, trace_span->span_id);
        if (trace_span->parent) {
            zend_update_property_long(stackdriver_trace_span_ce, &spans[i], "parentSpanId", sizeof("parentSpanId") - 1, trace_span->parent->span_id);
        } else if (STACKDRIVER_TRACE_G(trace_parent_span_id)) {
            zend_update_property_long(stackdriver_trace_span_ce, &spans[i], "parentSpanId", sizeof("parentSpanId") - 1, STACKDRIVER_TRACE_G(trace_parent_span_id));
        }
        zend_update_property_str(stackdriver_trace_span_ce, &spans[i], "name", sizeof("name") - 1, trace_span->name);
        zend_update_property_double(stackdriver_trace_span_ce, &spans[i], "startTime", sizeof("startTime") - 1, trace_span->start);
        zend_update_property_double(stackdriver_trace_span_ce, &spans[i], "endTime", sizeof("endTime") - 1, trace_span->stop);

        array_init(&labels[i]);
        if (trace_span->labels) {
            stackdriver_labels_to_zval_array(trace_span->labels, &labels[i]);
        }
        zend_update_property(stackdriver_trace_span_ce, &spans[i], "labels", sizeof("labels") - 1, &labels[i]);

        add_next_index_zval(return_value, &spans[i]);

        i++;
    } ZEND_HASH_FOREACH_END();

    efree(labels);
    efree(spans);
}

// Constructor used for creating the stackdriver globals
static void php_stackdriver_trace_globals_ctor(void *pDest TSRMLS_DC)
{
    zend_stackdriver_trace_globals *stackdriver_trace_global = (zend_stackdriver_trace_globals *) pDest;
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(stackdriver_trace)
{
    // allocate global request variables
#ifdef ZTS
    ts_allocate_id(&stackdriver_trace_globals_id, sizeof(zend_stackdriver_trace_globals), php_stackdriver_trace_globals_ctor, NULL);
#else
    php_stackdriver_trace_globals_ctor(&php_stackdriver_trace_globals_ctor);
#endif

    // Save original zend execute functions and use our own to instrument function calls
    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = stackdriver_trace_execute_ex;

    original_zend_execute_internal = zend_execute_internal;
    zend_execute_internal = stackdriver_trace_execute_internal;

    stackdriver_trace_span_minit(INIT_FUNC_ARGS_PASSTHRU);
    stackdriver_trace_context_minit(INIT_FUNC_ARGS_PASSTHRU);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(stackdriver_trace)
{
    // Put the original zend execute function back.
    zend_execute_ex = original_zend_execute_ex;
    zend_execute_internal = original_zend_execute_internal;

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(stackdriver_trace)
{
    // initialize storage for user traced functions - per request basis
    ALLOC_HASHTABLE(STACKDRIVER_TRACE_G(user_traced_functions));
    zend_hash_init(STACKDRIVER_TRACE_G(user_traced_functions), 16, NULL, ZVAL_PTR_DTOR, 0);

    // initialze storage for recorded spans - per request basis
    ALLOC_HASHTABLE(STACKDRIVER_TRACE_G(spans));
    zend_hash_init(STACKDRIVER_TRACE_G(spans), 16, NULL, ZVAL_PTR_DTOR, 0);

    STACKDRIVER_TRACE_G(current_span) = NULL;
    STACKDRIVER_TRACE_G(trace_id) = NULL;
    STACKDRIVER_TRACE_G(trace_parent_span_id) = 0;

    return SUCCESS;
}

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(stackdriver_trace)
{
    zval *handler;

    stackdriver_trace_clear(0 TSRMLS_CC);

    // cleanup user_traced_functions zvals that we copied when registing
    ZEND_HASH_FOREACH_VAL(STACKDRIVER_TRACE_G(user_traced_functions), handler) {
        PHP_STACKDRIVER_FREE_STD_ZVAL(handler);
    } ZEND_HASH_FOREACH_END();
    FREE_HASHTABLE(STACKDRIVER_TRACE_G(user_traced_functions));

    return SUCCESS;
}
/* }}} */
