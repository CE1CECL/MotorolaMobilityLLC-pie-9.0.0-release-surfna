#pragma once

/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/types.h>

#include <ostream>

enum DebuggerdDumpType : uint8_t {
  kDebuggerdNativeBacktrace,
  kDebuggerdTombstone,
  kDebuggerdCrashDump,
  kDebuggerdJavaBacktrace,
  kDebuggerdAnyIntercept
};

inline std::ostream& operator<<(std::ostream& stream, const DebuggerdDumpType& rhs) {
  switch (rhs) {
    case kDebuggerdNativeBacktrace:
      stream << "kDebuggerdNativeBacktrace";
      break;
    case kDebuggerdTombstone:
      stream << "kDebuggerdTombstone";
      break;
    case kDebuggerdJavaBacktrace:
      stream << "kDebuggerdJavaBacktrace";
      break;
    case kDebuggerdAnyIntercept:
      stream << "kDebuggerdAnyIntercept";
      break;
    default:
      stream << "[unknown]";
  }

  return stream;
}
