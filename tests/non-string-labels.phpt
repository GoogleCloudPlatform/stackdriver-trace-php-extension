--TEST--
Stackdriver Trace: Test setting labels
--FILE--
<?php

stackdriver_trace_begin('root', []);
stackdriver_trace_add_label('int', 1);
stackdriver_trace_begin('inner', []);
stackdriver_trace_add_label('float', 0.1);
stackdriver_trace_finish();
stackdriver_trace_finish();

$traces = stackdriver_trace_list();
echo "Number of traces: " . count($traces) . "\n";
$span = $traces[0];
print_r($span->labels());

$span2 = $traces[1];
print_r($span2->labels());
?>
--EXPECT--
Number of traces: 2
Array
(
    [int] => 1
)
Array
(
    [float] => 0.1
)
