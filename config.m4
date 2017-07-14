PHP_ARG_ENABLE(stackdriver_trace, whether to enable my extension,
[ --enable-stackdriver-trace  Enable Stackdriver Trace])

if test "$PHP_STACKDRIVER_TRACE" = "yes"; then
  AC_DEFINE(HAVE_STACKDRIVER_TRACE, 1, [Whether you have Stackdriver])
  PHP_NEW_EXTENSION(stackdriver_trace, stackdriver_trace.c stackdriver_trace_span.c stackdriver_trace_context.c, $ext_shared)
fi
