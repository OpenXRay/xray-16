#pragma once

#include "profile_data_types.h"

#include "profile_data_types_script.h"

namespace gamespy_profile
{
class profile_store
{
public:
    void load_current_profile(store_operation_cb progress_indicator_cb, store_operation_cb complete_cb);
    void stop_loading() {}

    [[nodiscard]]
    const auto& get_awards() const { return m_awards; }

    [[nodiscard]]
    const auto& get_best_scores() const { return m_best_scores; }

private:
    // These are dummies
    all_awards_t m_awards;
    all_best_scores_t m_best_scores;
}; // class profile_store
} // namespace gamespy_profile
