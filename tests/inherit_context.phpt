--TEST--
Stackdriver Trace: Root inherits from context
--FILE--
<?php

$res = stackdriver_trace_set_context('traceid', 1234);
echo "Set context: ${res}\n";

stackdriver_trace_begin('root', []);
stackdriver_trace_begin('inner', []);
stackdriver_trace_finish();
stackdriver_trace_finish();

$traces = stackdriver_trace_list();
echo "Number of traces: " . count($traces) . "\n";

$span = $traces[0];
echo "Parent span id: {$span->parentSpanId()}\n";

$span2 = $traces[1];
$test = $span2->parentSpanId() == $span->spanId();
echo "Nested span parent span id is root span id: $test\n";
?>
--EXPECT--
Set context: 1
Number of traces: 2
Parent span id: 1234
Nested span parent span id is root span id: 1
