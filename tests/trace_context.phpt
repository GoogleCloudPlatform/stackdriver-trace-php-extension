--TEST--
Stackdriver Trace: Trace Context
--FILE--
<?php

require_once(__DIR__ . '/common.php');

// 1: Sanity test a simple profile run
stackdriver_trace_method("Foo", "context");
stackdriver_trace_set_context("traceid", 1234);
$context = stackdriver_trace_context();
if ($context instanceof Stackdriver\Trace\Context) {
    echo "Context is a Stackdriver\\Trace\\Context.\n";
}

$f = new Foo();
$context = $f->context();

if ($context instanceof Stackdriver\Trace\Context) {
    echo "Nested context is a Stackdriver\\Trace\\Context.\n";
}

$traces = stackdriver_trace_list();

echo "Number of traces: " . count($traces) . "\n";
$span = $traces[0];

if ($span->spanId() == $context->spanId()) {
    echo "Span id matches context's span id.\n";
}

echo "Span parent id: {$span->parentSpanId()}\n";
echo "Context trace id: {$context->traceId()}\n";
?>
--EXPECT--
Context is a Stackdriver\Trace\Context.
Nested context is a Stackdriver\Trace\Context.
Number of traces: 1
Span id matches context's span id.
Span parent id: 1234
Context trace id: traceid
