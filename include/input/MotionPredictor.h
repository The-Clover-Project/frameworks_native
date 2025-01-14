/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <android-base/result.h>
#include <android-base/thread_annotations.h>
#include <android/sysprop/InputProperties.sysprop.h>
#include <input/Input.h>
#include <input/MotionPredictorMetricsManager.h>
#include <input/RingBuffer.h>
#include <input/TfLiteMotionPredictor.h>
#include <utils/Timers.h> // for nsecs_t

namespace android {

static inline bool isMotionPredictionEnabled() {
    return sysprop::InputProperties::enable_motion_prediction().value_or(true);
}

// Tracker to calculate jerk from motion position samples.
class JerkTracker {
public:
    // Initialize the tracker. If normalizedDt is true, assume that each sample pushed has dt=1.
    // alpha is the coefficient of the first-order IIR filter for jerk. A factor of 1 results
    // in no smoothing.
    JerkTracker(bool normalizedDt, float alpha);

    // Add a position to the tracker and update derivative estimates.
    void pushSample(int64_t timestamp, float xPos, float yPos);

    // Reset JerkTracker for a new motion input.
    void reset();

    // Return last jerk calculation, if enough samples have been collected.
    // Jerk is defined as the 3rd derivative of position (change in
    // acceleration) and has the units of d^3p/dt^3.
    std::optional<float> jerkMagnitude() const;

private:
    const bool mNormalizedDt;
    // Coefficient of first-order IIR filter to smooth jerk calculation.
    const float mAlpha;

    RingBuffer<int64_t> mTimestamps{4};
    std::array<float, 4> mXDerivatives{}; // [x, x', x'', x''']
    std::array<float, 4> mYDerivatives{}; // [y, y', y'', y''']
    float mJerkMagnitude;
};

/**
 * Given a set of MotionEvents for the current gesture, predict the motion. The returned MotionEvent
 * contains a set of samples in the future.
 *
 * The typical usage is like this:
 *
 * MotionPredictor predictor(offset = MY_OFFSET);
 * predictor.record(DOWN_MOTION_EVENT);
 * predictor.record(MOVE_MOTION_EVENT);
 * prediction = predictor.predict(futureTime);
 *
 * The resulting motion event will have eventTime <= (futureTime + MY_OFFSET). It might contain
 * historical data, which are additional samples from the latest recorded MotionEvent's eventTime
 * to the futureTime + MY_OFFSET.
 *
 * The offset is used to provide additional flexibility to the caller, in case the default present
 * time (typically provided by the choreographer) does not account for some delays, or to simply
 * reduce the aggressiveness of the prediction. Offset can be positive or negative.
 */
class MotionPredictor {
public:
    using ReportAtomFunction = MotionPredictorMetricsManager::ReportAtomFunction;

    /**
     * Parameters:
     * predictionTimestampOffsetNanos: additional, constant shift to apply to the target
     * prediction time. The prediction will target the time t=(prediction time +
     * predictionTimestampOffsetNanos).
     *
     * checkEnableMotionPrediction: the function to check whether the prediction should run. Used to
     * provide an additional way of turning prediction on and off. Can be toggled at runtime.
     *
     * reportAtomFunction: the function that will be called to report prediction metrics. If
     * omitted, the implementation will choose a default metrics reporting mechanism.
     */
    MotionPredictor(nsecs_t predictionTimestampOffsetNanos,
                    std::function<bool()> checkEnableMotionPrediction = isMotionPredictionEnabled,
                    ReportAtomFunction reportAtomFunction = {});

    /**
     * Record the actual motion received by the view. This event will be used for calculating the
     * predictions.
     *
     * @return empty result if the event was processed correctly, error if the event is not
     * consistent with the previously recorded events.
     */
    android::base::Result<void> record(const MotionEvent& event);

    std::unique_ptr<MotionEvent> predict(nsecs_t timestamp);

    bool isPredictionAvailable(int32_t deviceId, int32_t source);

private:
    const nsecs_t mPredictionTimestampOffsetNanos;
    const std::function<bool()> mCheckMotionPredictionEnabled;

    std::unique_ptr<TfLiteMotionPredictorModel> mModel;

    std::unique_ptr<TfLiteMotionPredictorBuffers> mBuffers;
    std::optional<MotionEvent> mLastEvent;

    std::unique_ptr<JerkTracker> mJerkTracker;

    std::unique_ptr<MotionPredictorMetricsManager> mMetricsManager;

    const ReportAtomFunction mReportAtomFunction;

    // Initialize prediction model and associated objects.
    // Called during lazy initialization.
    // TODO: b/210158587 Consider removing lazy initialization.
    void initializeObjects();
};

} // namespace android
