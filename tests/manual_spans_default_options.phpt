--TEST--
Stackdriver Trace: Customize the trace span options for a function
--FILE--
<?php

require_once(__DIR__ . '/common.php');

stackdriver_trace_begin('/');

stackdriver_trace_begin('inner-1');

stackdriver_trace_finish();

stackdriver_trace_finish();

$traces = stackdriver_trace_list();
echo "Number of traces: " . count($traces) . "\n";
print_r($traces);
?>
--EXPECTF--
Number of traces: 2
Array
(
    [0] => Stackdriver\Trace\Span Object
        (
            [name:protected] => /
            [spanId:protected] => %d
            [parentSpanId:protected] =>%s
            [startTime:protected] => %d.%d
            [endTime:protected] => %d.%d
            [labels:protected] => Array
                (
                )

        )

    [1] => Stackdriver\Trace\Span Object
        (
            [name:protected] => inner-1
            [spanId:protected] => %d
            [parentSpanId:protected] => %d
            [startTime:protected] => %d.%d
            [endTime:protected] => %d.%d
            [labels:protected] => Array
                (
                )

        )

)
