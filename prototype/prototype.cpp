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

	struct StatisticVarianceCollector
	{
		size_t num_terms;
		float64_t statistic;
		float64_t variance;
		enum VarianceEstimationMethod;
		StatisticVarianceCollector() : num_terms(1), statistic(0.0), variance(0.0) {}
		static void add(StatisticVarianceCollector& c, const std::pair<float64_t,float64_t>& value)
		{
			auto delta = value.first - c.statistic;
			c.statistic += delta/c.num_terms;
			// variance computation logic..
			c.num_terms++;
		}
	};

	struct MultiKernelStatisticCollector
	{
		const SGVector<size_t> terms_counters;
		const SGVector<float64_t>& mmds;
		MultiKernelStatisticCollector(const SGVector<float64_t>& s, const SGVector<size_t>& tc)
				: mmds(s), term_counters(tc)
		{
		}
		static void add(MultiKernelStatisticCollector& c, std::pair<size_t,float64_t> value)
		{
			auto i = value.first;
			auto delta = value.second - c.mmds[i];
			c.mmds[i] += delta/c.terms_counters[i];
			c.terms_counters[i]++;
		}
	};

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

		StatisticVarianceCollector c;
		if (kernel_matrix_precomputed)
		{
			auto collection = Stream::as_collection(kernel_matrix);
			c = collection.diagonal_block_stream(blocksize)
				.map(compute_jobs)
				.collect([&c]() { return c; }, &StatisticVarianceCollector::add);
		}
		else
		{
			c = data_mgr.block_stream(blocksize)
				.map([this](const NextSamples& next_samples)
				{
					auto samples_p = next_samples.sample_at(0);
					auto samples_q = next_samples.sample_at(1);
					return kernel->get_kernel_matrix(samples_p, samples_q);
				})
				.map(compute_jobs)
				.collect([&c]() { return c; }, &StatisticVarianceCollector::add);
		}
		return make_pair(normalize_statistic(c.statistic), normalize_variance(c.variance));
	}

	static auto get_kernel_indices_by_distance_type(const KernelManager& kernel_mgr) const
			-> std::unordered_map<DistanceType,std::vector<size_t>>
	{
		return IntStream::range(0, kernel_mgr.size())
			.filter([&kernel_mgr](size_t i)
			{
				return kernel_mgr.kernel_at(i)->get_kernel_type() == SHIFT_INVARIANT;
			})
			.collect(Collectors::grouping_by([&kernel_mgr](size_t i)
			{
				return kernel_mgr.kernel_at(i)->get_distance()->get_distance_type();
			});
	}

	/**
	 * More complex example - ordering matters.
	 */
	std::pair<SGVector<float64_t>,SGMatrix<float64_t>> compute_statistic_and_Q(const KernelManager& kernel_mgr) const
	{
		SGVector<float64_t> mmds(kernel_mgr.size());
		SGMatrix<float64_t,float64_t> Q(kernel_mgr.size(), kernel_mgr.size());

		auto kernel_inds = get_kernel_indices_by_distance_type(kernel_mgr);
		auto kernel_inds_c = Stream::as_collection(kernel_inds).stream()
			.flat_map(IntStream::stream)
			.compliment(IntStream::range(0, kernel_mgr.size()))
			.collect(Collectors::to_stl_vector<size_t>());

		SGVector<size_t> term_counters_statistic(kernel_mgr.size(), 1);
		MultiKernelStatisticCollector c(mmds, term_counters_statistic);

		data_mgr.block_stream(blocksize)
			.map([&kernel_mgr,&cache](const NextSamples& next_samples)
			{
				auto samples_p = next_samples.sample_at(0);
				auto samples_q = next_samples.sample_at(0);
				auto merged = merge_samples(samples_p, samples_q);

				// compute mmds on kernels which can share distance instance
				// we precompute the distance matrix for those kernels and compute the
				// statistic on the fly.
				c = Stream::as_collection(kernel_inds).stream()
					.map([&merged,&kernel_mgr](const std::vector<size_t>& inds)
					{
						return std::make_pair(inds,kernel_mgr[inds[0]]->distance(merged, merged));
					})
					.map([](const std::pair<const std::vector<size_t>&,SGMatrix<float64_t>& inds_d)
					{
						return Stream::as_collection(inds_d.first).stream()
							.map([&inds_d.second,&kernel_mgr](size_t i)
							{
								auto kernel = kernel_mgr.kernel_at(i)->clone();
								kernel->init(inds_d.second);
								return std::make_pair(i,statistic_job(clone->get_kernel_matrix()));
							})
							.collect(Collectors::to_stl_vector<std::pair<size_t,float64_t>>());
					})
					.flat_map([](std::vector<std::pair<size_t,float64_t>> v)
					{
						return Stream::as_collection(v).stream();
					})
					.collect([&c]() { return c; }, &MultiKernelStatisticCollector::add);

				// compute mmds on instances which do not share any characteristic with
				// any which can be exploited to make the computation faster
				c = Stream::as_collection(kernel_inds_c).stream()
					.map([&merged,&kernel_mgr](const std::vector<size_t>& inds)
					{
						return Stream::as_collection(inds).stream()
							.map([&kernel_mgr,&merged](size_t i)
							{
								auto kernel = kernel_mgr.kernel_at(i)->clone();
								kernel->init(merged, merged);
								return std::make_pair(i,statistic_job(clone->get_kernel_matrix()));
							})
							.collect(Collectors::to_stl_vector<std::pair<size_t,float64_t>>());
					})
					.collect([&c]() { return c; }, &MultiKernelStatisticCollector::add);

					// we have the mmds for the current bunch, compute Q using usual logic
			})
			.collect(/* something */);
	}
}
