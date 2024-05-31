#pragma once

#include <iostream>
#include <vector>
#include <mutex>
#include <functional>

namespace chatterino {

template <class ResultT>
class PronounsApiRequest {
public:
  std::mutex mutex;
  std::size_t outstandingSubRequests;
  std::vector<ResultT> results;

  std::function<void(std::vector<ResultT>)> onDone;

  PronounsApiRequest(std::size_t numRequests, std::function<void(std::vector<ResultT>)> onDone): outstandingSubRequests{numRequests}, onDone{onDone} {
  }

  void finishRequest(ResultT result) {
    bool done {false};

    { // lock(mutex)
      std::lock_guard<std::mutex> lock(mutex);
    
      outstandingSubRequests--;
      results.push_back(result);

      if (outstandingSubRequests == 0) {
        // Finally done.
        done = true;
      }
    } // lock(mutex)

    if (done) {
      onDone(results);
    }
  }
};

} // namespace chatterino