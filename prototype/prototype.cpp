class CDistance
{
  SGMatrix<float64_t> CDistance::get_distance_matrix()
  {
    SGMatrix<float64_t> distance_matrix(lhs.size(), rhs.size());
    for (const auto& sample : rhs)
    {
      lhs.parallel_stream()
        .map([&sample, this](CFeatureSample const * other_sample)
             {
                return this->distance(sample, other_sample);
             })
        .collect(Collectors::to_vector(distance_matrix.col(sample.index())));
    }
  }
};

class CEuclideanDistance
{
  void init(CDotFeatures const * lhs, CDotFeatures const * rhs);
  void cache_squared_norms()
  {
    auto norm_functor = [](CDotFeatureSample const * sample)
      {
        return sample->dot(sample);
      };
    norm_cache_lhs = lhs.parallel_strem().map(norm_functor).collect(Collectors::to_vector<float64_t>());
    norm_cache_rhs = rhs.parallel_strem().map(norm_functor).collect(Collectors::to_vector<float64_t>());
  }
  virtual float64_t distance(CFeatureSample const * first, CFeaturesSample const * second) override
  {
    auto first_dot_feature = static_cast<CDotFeatureSample const *>(first);
    auto second_dot_feature = static_cast<CDotFeatureSample const *>(second);
    auto result = first_dot_feature->dot(second_dot_feature);
    return CMath::sqrt(norm_cache_lhs[first->index()] + norm_cache_rhs[second->index()] - 2 * result);
  }
};

class CKernel
{
  SGMatrix<float64_t> CKernel::get_kernel_matrix()
  {
    SGMatrix<float64_t> kernel_matrix(lhs.size(), rhs.size());
    for (const auto& sample : rhs)
    {
      lhs.parallel_stream()
        .map([&sample, this](CFeatureSample const * other_sample)
             {
                return this->kernel(sample, other_sample);
             })
        .collect(Collectors::to_vector(kernel_matrix.col(sample.index())));
    }
  }
};

class CBTestMMD
{
  // punching multiple jobs together for performance reasons
  // can use it for computing both these value simultaneously
  template <class Block>
  auto compute_jobs(const Block& block)
  {
    auto mmd = statistic_job(block);
    auto var = variance_job(block);
    return std::make_pair(statistic, variance);
  }
  float64_t compute_statistic()
  {
    auto result = 0.0;
    if (kernel_matrix_precomputed)
    {
      result = kernel_matrix.diagonal_block_stream(blocksize)
        .map(statistic_job)
        .mean();
    }
    else
    {
      result = data_mgr.block_stream(blocksize)
        .map([this](const NextSamples& next_samples)
             {
                auto samples_p = next_samples.sample_at(0);
                auto samples_q = next_samples.sample_at(1);
                return kernel->get_kernel_matrix(samples_p, samples_q);
             })
        .map(statistic_job)
        .mean();
    }
    return normalize_statistic(result);
  }
  /**
   * Example of packing multiple computation job per element in the stream and reducing to multiple values
   */
  std::pair<float64_t, float64_t> compute_statistic_variance()
  {
    auto result = make_pair(0.0, 0.0);
    auto unitary = std::make_pair(0.0, size_t(1));
    // the following can also be off-the-shelf, where a supplied lambda simply helps it extract
    // the element (which, in this case, can simply be std::get<0, float64_t, float64_t>
    // signature : unitary accumulator(decltype(unitary), AnyValue)
    auto accumulator = [](const std::pair<float64_t, size_t>& value, const decltype(result)& pair)
      {
        auto& running_mean = value.first;
        const auto& num_terms_including_current = ++value.second;
        auto delta = pair.first - running_mean;
        running_mean = running_mean + delta/num_terms_including_current;
        return value;
      };
    auto finalizer = std::get<0, float64_t, size_t>; // final return value
    // should have the sinature auto finalizer(decltype(unitary))
    // optional combiner in case of parallel streams
    auto statistic_reducer = Reducers::custom(unitary, accumulator, finalizer);
    // similarly, variance reducer
    auto variance_reducer = Reducers::custom(...);

    // if saving the values are necessary
//    auto mmds = SGVector<float64_t>(num_blocks);
//    auto vars = SGVector<float64_t>(num_blocks);
//    auto statistic_collector = ...

    if (kernel_matrix_precomputed)
    {
      result = kernel_matrix.diagonal_block_stream(blocksize)
        .map(compute_jobs)
        .reduce(statistic_reducer, variance_reducer);
        // the return type is a scalar if number of reducers is 1, std::pair if number of reducers is 2, tuple if more
    }
    else
    {
      result = data_mgr.block_stream(blocksize)
        .map([this](const NextSamples& next_samples)
             {
                auto samples_p = next_samples.sample_at(0);
                auto samples_q = next_samples.sample_at(1);
                return kernel->get_kernel_matrix(samples_p, samples_q);
             })
        .map(compute_jobs)
        .reduce(statistic_reducer, variance_reducer);
    }
    return make_pair(normalize_statistic(result.first), normalize_variance(result.second));
  }
}
