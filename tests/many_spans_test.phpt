--TEST--
Stackdriver Trace: Testing many spans (previous limit of 64)
--FILE--
<?php

for ($i = 0; $i < 1024; $i++) {
    stackdriver_trace_begin("Span $i");
    stackdriver_trace_finish();
}
$traces = stackdriver_trace_list();
echo "Number of traces: " . count($traces) . "\n";
$span = $traces[0];
?>
--EXPECT--
Number of traces: 1024
