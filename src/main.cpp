# include "tiny_ppl_core.h"
# include <iostream>
# include <iomanip> // For std::fixed and std::setprecision
# include <limits>  // For std::numeric_limits
 
int main() {
 
    std::cout << std::fixed << std::setprecision(3);

    // ---------------- Testing single_site_mh() (bits) ------------------
    std::cout << "\n--- Single-site MH (bits) ---" << std::endl;
    std::string bits =
        "(let [b1 (if (sample (bernoulli 0.5)) 1 0) "
        "b2 (if (sample (bernoulli 0.5)) 1 0) "
        "b3 (if (sample (bernoulli 0.5)) 1 0) "
        "b4 (if (sample (bernoulli 0.5)) 1 0) "
        "b5 (if (sample (bernoulli 0.5)) 1 0) "
        "b6 (if (sample (bernoulli 0.5)) 1 0) "
        "b7 (if (sample (bernoulli 0.5)) 1 0) "
        "b8 (if (sample (bernoulli 0.5)) 1 0) "
        "total (+ b1 b2 b3 b4 b5 b6 b7 b8)] "
        "(observe (normal 7 2) total) total)";

    std::mt19937 rng_bits(1);
    std::vector<double> ch2 = single_site_mh(bits, rng_bits, 40000, 3000);
    double mean2 = 0.0;
    for (double v : ch2) mean2 += v;
    mean2 /= ch2.size();

    double w_sum = 0.0, kw_sum = 0.0;
    for (int k = 0; k <= 8; ++k) {
        long long comb = 1;
        for (int j = 0; j < k; ++j) comb = comb * (8 - j) / (j + 1);
        double w = comb * std::exp(-0.5 * std::pow((k - 7) / 2.0, 2));
        w_sum += w;
        kw_sum += k * w;
    }
    double exact = kw_sum / w_sum;

    std::cout << "bits SSMH mean=" << mean2 << " (exact " << exact << ")" << std::endl;

    // ---------------- Testing single_site_mh() ------------------
    
    std::cout << "\n--- Single-site MH (conj) ---" << std::endl;
    std::string conj = "(let [mu (sample (normal 0 1))] (observe (normal mu 1) 2.3) mu)";
    std::mt19937 rng_mh(0);
    std::vector<double> ch = single_site_mh(conj, rng_mh, 60000, 3000);

    double mean = 0.0;
    for (double v : ch) mean += v;
    mean /= ch.size();

    double sq = 0.0;
    for (double v : ch) sq += (v - mean) * (v - mean);
    double stdv = std::sqrt(sq / ch.size());

    std::cout << "conj SSMH mean=" << mean << " std=" << stdv << " (esperado: mean~1.150, std~0.707)" << std::endl;

    // ----------------------- LW vs SMC vs SSMH sobre el mismo modelo -----------------------------------
    std::cout << "\n--- LW vs SMC vs SSMH sobre el mismo modelo ---" << std::endl;

    std::mt19937 rng_lw(2);
    auto [vals, weights] = likelihood_weighting(conj, rng_lw, 100000);
    double lw_mean = 0.0;
    for (size_t i = 0; i < vals.size(); ++i) lw_mean += weights[i] * vals[i];
    std::cout << "LW   mean = " << lw_mean << std::endl;

    std::vector<std::mt19937> rngs_smc;
    for (int i = 0; i < 20000; ++i) rngs_smc.emplace_back(1000 + i);
    std::vector<double> smc = run_smc(conj, rngs_smc, 20000);
    double smc_mean = 0.0;
    for (double v : smc) smc_mean += v;
    smc_mean /= smc.size();
    std::cout << "SMC  mean = " << smc_mean << std::endl;

    std::cout << "SSMH mean = " << mean << "   (los tres deberian rondar 1.150)" << std::endl;

    // ----------------------- Testing closures and recursion -----------------------------------

    std::cout << "\n--- Closures (scope lexico) ---" << std::endl;
    std::string shift = "(let [make-shift (fn [mu] (fn [x] (+ x mu)))  f (make-shift 10)] (f 3))";
    std::mt19937 rng_shift(0);
    auto [shift_val, shift_logw] = run_lw(shift, rng_shift);
    std::cout << "(f 3) = " << std::dynamic_pointer_cast<NumberValue>(shift_val)->value << " (esperado: 13)" << std::endl;


    std::cout << "\n--- Recursion (geom) ---" << std::endl;
    std::string geom = "(defn geom [] (if (sample (bernoulli 0.3)) 0 (+ 1 (geom)))) (geom)";
    std::mt19937 rng_geom(1);
    int trials = 200000;
    double ksum = 0.0;
    for (int i = 0; i < trials; ++i) {
        auto [v, lw] = run_lw(geom, rng_geom);
        ksum += std::dynamic_pointer_cast<NumberValue>(v)->value;
    }
    std::cout << "geom mean = " << (ksum / trials) << "   (esperado (1-p)/p = 2.333)" << std::endl;








    return 0;
}