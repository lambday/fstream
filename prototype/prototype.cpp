class CDistance
{
	SGMatrix<float64_t> CDistance::get_distance_matrix() const
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
		auto sq_norm_functor = [](CDotFeatureSample const * sample)
		{
			return sample->dot(sample);
		};
		sq_norm_cache_lhs = lhs.parallel_strem()
			.map(sq_norm_functor)
			.collect(Collectors::to_vector<float64_t>());
		sq_norm_cache_rhs = rhs.parallel_strem()
			.map(sq_norm_functor)
			.collect(Collectors::to_vector<float64_t>());
	}
	virtual float64_t distance(CFeatureSample const * first, CFeaturesSample const * second) const override
	{
		auto first_dot_feature = static_cast<CDotFeatureSample const *>(first);
		auto second_dot_feature = static_cast<CDotFeatureSample const *>(second);
		auto result = first_dot_feature->dot(second_dot_feature);
		return CMath::sqrt(sq_norm_cache_lhs[first->index()] + sq_norm_cache_rhs[second->index()] - 2 * result);
	}
};

class CKernel
{
	SGMatrix<float64_t> CKernel::get_kernel_matrix() const
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
	float64_t compute_statistic() const
	{
		auto result = 0.0;
		if (kernel_matrix_precomputed)
		{
			auto collection = Stream::as_collection(kernel_matrix);
			result = collection.diagonal_block_stream(blocksize)
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
	std::pair<float64_t, float64_t> compute_statistic_variance() const
	{
		// punching multiple jobs together for performance reasons
		// can use it for computing both these value simultaneously
		auto compute_jobs = [](const SGMatrix<float64_t>& block)
		{
			auto mmd = statistic_job(block);
			auto var = variance_job(block);
			return std::make_pair(statistic, variance);
		};

		struct MultiCollector
		{
			size_t num_terms;
			float64_t statistic;
			float64_t variance;
			enum VarianceEstimationMethod;
			MultiCollector() : num_terms(1), statistic(0.0), variance(0.0) {}
			static void add(MultiCollector& mc, const std::pair<float64_t,float64_t>& value)
			{
				auto delta = value.first - mc.statistic;
				mc.statistic += delta/mc.num_terms;
				// variance computation logic..
				mc.num_terms++;
			}
		};

		MultiCollector mc;
		if (kernel_matrix_precomputed)
		{
			auto collection = Stream::as_collection(kernel_matrix);
			mc = collection.diagonal_block_stream(blocksize)
				.map(compute_jobs)
				.collect([&mc]() { return mc; }, &MultiCollector::add);
		}
		else
		{
			mc = data_mgr.block_stream(blocksize)
				.map([this](const NextSamples& next_samples)
				{
					auto samples_p = next_samples.sample_at(0);
					auto samples_q = next_samples.sample_at(1);
					return kernel->get_kernel_matrix(samples_p, samples_q);
				})
				.map(compute_jobs)
				.collect([&mc]() { return mc; }, &MultiCollector::add);
		}
		return make_pair(normalize_statistic(mc.statistic), normalize_variance(mc.variance));
	}

	/**
	 * More complex example - ordering matters.
	 */
	std::pair<SGVector<float64_t>,SGMatrix<float64_t>> compute_statistic_and_Q(const KernelManager& kernel_mgr) const
	{
		SGVector<float64_t> mmds(kernel_mgr.size());
		SGMatrix<float64_t,float64_t> Q(kernel_mgr.size(), kernel_mgr.size());
		SGMatrix<float64_t,float64_t> cache(Q.num_rows, Q.num_cols);

		auto categorized_kernel_inds = IntStream::range(0, kernel_mgr.size())
			.filter([&kernel_mgr](size_t i)
			{
				return kernel_mgr.kernel_at(i)->get_kernel_type() == SHIFT_INVARIANT;
			})
			.collect(Collectors::grouping_by([&kernel_mgr](size_t i)
			{
				return kernel_mgr.kernel_at(i)->get_distance()->get_distance_type();
			});

		// also, obtain the indices which are not shift invariant kernel
		Stream::as_collection(categorized_kernel_inds).stream().flat_map() // TODO

		data_mgr.block_stream(blocksize)
			.map([&kernel_mgr,&cache](const NextSamples& next_samples)
			{
				auto samples_p = next_samples.sample_at(0);
				auto samples_q = next_samples.sample_at(0);

				Stream::as_collection(categorized_kernel_inds).stream()
					.map([&samples_p,&samples_q,&kernel_mgr](const std::vector<size_t>& inds)
					{
						// there will always be at least one element in kernels, other_sample
						// there won't be an entry in the grouby return map
						auto dmatrix = kernel_mgr[inds[0]]->distance(samples_p, samples_q);
						Stream::as_collection(inds).stream()
							.map([&distance_matrix])(size_t i)
							{
								auto kernel = k->clone();
								kernel->init(distance_matrix);
								return clone->get_kernel_matrix();
							}
							.map(statistic_job)
							.collect()
					}
					.map(statistic_job)
					.collect();
			}
	}
}
