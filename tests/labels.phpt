--TEST--
Stackdriver Trace: Test setting labels
--FILE--
<?php

stackdriver_trace_begin('root', []);
stackdriver_trace_add_label('foo', 'bar');
stackdriver_trace_begin('inner', []);
stackdriver_trace_add_root_label('asdf', 'qwer');
stackdriver_trace_add_label('abc', 'def');
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
    [foo] => bar
    [asdf] => qwer
)
Array
(
    [abc] => def
)
