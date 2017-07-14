--TEST--
Stackdriver Trace: Clear Traces
--FILE--
<?php

require_once(__DIR__ . '/common.php');

// 1: Sanity test a simple profile run
stackdriver_trace_function("bar");
bar();
$traces = stackdriver_trace_list();
echo "Number of traces: " . count($traces) . "\n";
stackdriver_trace_clear();
$traces = stackdriver_trace_list();
echo "Number of traces: " . count($traces) . "\n";

?>
--EXPECT--
Number of traces: 1
Number of traces: 0
