--TEST--
Stackdriver Trace: Test setting labels
--FILE--
<?php

class FooClass
{
    function foo() {
        return 'bar';
    }
}
stackdriver_trace_method('FooClass', 'foo', ['labels' => ['int' => 1, 'float' => 0.1]]);
$foo = new FooClass();
$foo->foo();

$traces = stackdriver_trace_list();
echo "Number of traces: " . count($traces) . "\n";
$span = $traces[0];
print_r($span->labels());
?>
--EXPECT--
Number of traces: 1
Array
(
    [int] => 1
    [float] => 0.1
)
