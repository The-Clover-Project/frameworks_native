/*
 * Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include <utils/Timers.h>

namespace android {

/*
 * Input dispatcher configuration.
 *
 * Specifies various options that modify the behavior of the input dispatcher.
 * The values provided here are merely defaults. The actual values will come from ViewConfiguration
 * and are passed into the dispatcher during initialization.
 */
struct InputDispatcherConfiguration {
    // The key repeat initial timeout.
    nsecs_t keyRepeatTimeout;

    // The key repeat inter-key delay.
    nsecs_t keyRepeatDelay;

    // Whether key repeat is enabled.
    bool keyRepeatEnabled;

    InputDispatcherConfiguration()
          : keyRepeatTimeout(500 * 1000000LL),
            keyRepeatDelay(50 * 1000000LL),
            keyRepeatEnabled(true) {}
};

} // namespace android
