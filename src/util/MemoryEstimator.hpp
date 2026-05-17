//
// Created by fguld on 5/17/2026.
//

#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <utility>
#include <variant>

#include "../store/core/StoreValue.hpp"
#include "../store/types/Stream.hpp"

namespace memory_estimator {


namespace detail {
/** @brief Helper struct to create an overloaded set of lambdas for std::visit.
 * This allows us to handle each variant type in StoreValue with a separate lambda
 * while keeping the code organized and readable.
 */
  template <class... Ts>
  struct Overloaded : Ts... {
	using Ts::operator()...;
  };

  template <class... Ts>
  Overloaded(Ts...) -> Overloaded<Ts...>;
} // namespace detail

/**
 * @brief Estimates the size of stored values and datasets.
 *
 * The visitor is intentionally exhaustive: if a new StoreValue alternative is added,
 * compilation fails until this utility is updated with a matching overload.
 */
class MemoryEstimator {
public:
  [[nodiscard]] static auto estimate_value_size(const StoreValue& value) -> std::size_t {
	return std::visit(
	  detail::Overloaded{
		// For strings, the size is simply the length of the string.
	  	[](const std::string& s) -> std::size_t {
		  return s.size();
		},
	  // For lists, we sum the sizes of all contained strings.
		[](const std::deque<std::string>& items) -> std::size_t {
		  std::size_t total = 0;
		  for (const auto& item : items) {
			total += item.size();
		  }
		  return total;
		},
	  // For streams, we sum the sizes of all entry IDs and their associated field-value pairs.
		[](const Stream& stream) -> std::size_t {
		  std::size_t total = 0;
		  for (const auto& [id, fields] : stream.entries) {
			total += id.size();
			for (const auto& [field, field_value] : fields) {
			  total += field.size() + field_value.size();
			}
		  }
		  return total;
		}
	  },
	  value.get_value()
	);
  }

	/**
	 * @brief Estimates the total size of a dataset, including all keys and their associated values.
	 * @param data The dataset to estimate, represented as an unordered_map of keys to StoreValues.
	 * @return The estimated total size in bytes of the dataset.
	 */
  template <typename DataMap>
  [[nodiscard]] static auto estimate_dataset_size(const DataMap& data) -> std::size_t {
	std::size_t total = 0;
	for (const auto& [key, value] : data) {
	  total += key.size();
	  total += estimate_value_size(value);
	}
	return total;
  }
};

} // namespace memory_estimator

