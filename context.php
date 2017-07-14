<?php
/**
 * Copyright 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

namespace Stackdriver\Trace;

/**
 * This is the equivalent PHP class created by the stackdriver_trace C extension
 */
class Context {
    protected $traceId;
    protected $spanId;

    public function __construct(array $contextOptions)
    {
        foreach ($contextOptions as $k => $v) {
            $this->__set($k, $v);
        }
    }

    public function spanId()
    {
        return $this->spanId;
    }

    public function traceId()
    {
        return $this->traceId;
    }
}
